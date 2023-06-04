#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

/* Max size of a cacheable request */
#define MAX_OBJECT_SIZE 102400

/* HTTP requset line limits */
#define MAX_HOSTNAME_LEN 256
#define MAX_PORT_LEN 6
#define MAX_QUERY_LEN 2048

/* HTTP max header limit */
#define MAX_HEADERS_NUMBER 200

/* Thread pool constants */
#define NTHREADS 4
#define SBUFSIZE 16

/* Task required proxy headers */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

typedef struct uri_info
{
    char hostname[MAX_HOSTNAME_LEN];
    char port[MAX_PORT_LEN];
    char query[MAX_QUERY_LEN];
} Uri_info;

typedef struct headers
{
    int cout;
    char *data[MAX_HEADERS_NUMBER];
} Headers;

static void proxy(int fd);
static int parse_headers(rio_t *rp, Headers *headers);
static int parse_uri(const char *uri, Uri_info *uri_info);
static void forward(int fd, Uri_info *uri_info, Headers *headers);
static void clear_headers(Headers *headers);
static void *thread(void *vargp);
static int build_cache_key(char key[], size_t length, Uri_info *uri_info);
static void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
static void servererror(int fd);

/* Thread safe buffer */
static sbuf_t sbuf;
/* Local proxy cache */
static Cache cache;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* Blocking SIGPIPE signal */
    /* SIGPIPE signal will be send by write() when
     *  the connection socket closes prematurely.
     * An unhandled SIGPIPE signal terminates the program */
    sigset_t mask;
    if (Sigemptyset(&mask) < 0)
        exit(1);

    if (Sigaddset(&mask, SIGPIPE) < 0)
        exit(1);

    if (Sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
        exit(1);

    /* Creatin listening socket */
    int listenfd;
    if ((listenfd = Open_listenfd(argv[1])) < 0)
        exit(1);

    /* Creating thread pool */
    if (sbuf_init(&sbuf, SBUFSIZE) < 0)
        exit(1);

    if (cache_init(&cache) < 0)
        exit(1);

    int i;
    pthread_t tid;
    for (i = 0; i < NTHREADS; i++)
        if (Pthread_create(&tid, NULL, thread, NULL) != 0)
            exit(1);

    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd;
    while (1)
    {
        if ((connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0)
            continue;

        /* Logging client info */
        char client_host[MAXLINE];
        char port[MAXLINE];
        Getnameinfo((SA *)&clientaddr, clientlen, client_host, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", client_host, port);
        /* Adding request to the buffer */
        if (sbuf_insert(&sbuf, connfd) < 0)
            exit(1);
    }

    return 0;
}

static void *thread(void *vargp)
{
    if (Pthread_detach(pthread_self()) != 0)
        exit(1);

    int connfd;
    while (1)
    {
        /* Dequeuing a request from the buffer */
        if (sbuf_remove(&sbuf, &connfd) < 0)
            exit(1);

        /* Request pipeline */
        proxy(connfd);
        Close(connfd);
    }
}

/* This is the main request pipeline routine.
 * It gets a connected socket descriptor. Parses the request,
 * sends a proxy request to the requested server and
 * forwards the response back to the client. */
static void proxy(int fd)
{
    rio_t rio;
    Rio_readinitb(&rio, fd);
    char buf[MAXLINE];
    /* Reading an HTTP request line from the connected socket */
    if (Rio_readlineb(&rio, buf, MAXLINE) < 0)
    {
        servererror(fd);
        return;
    }

    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];
    if (sscanf(buf, "%s %s %s", method, uri, version) == EOF)
    {
        clienterror(fd, "", "400", "Bad request",
                    "Request is empty");
        return;
    }

    printf("%s", buf);
    if (strcasecmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not implemented",
                    "Proxy does not implement this method");
        return;
    }

    if (strcasecmp(version, "HTTP/1.0") &&
        strcasecmp(version, "HTTP/1.1"))
    {
        clienterror(fd, version, "501", "Not implemented",
                    "Proxy supports only HTTP/1.0(1.1) protocol versions");
    }

    Uri_info reciever;
    if (parse_uri(uri, &reciever) < 0)
    {
        clienterror(fd, uri, "400", "Bad request",
                    "Invalid request URI. "
                    "URI must have the following structure: "
                    "http[s]://{hostname}[:{port}]/{location}");
        return;
    }

    Headers headers;
    int status;
    if ((status = parse_headers(&rio, &headers)) < 0)
    {
        switch (status)
        {
        case -1:
            clienterror(fd, "", "413", "Entity is too large",
                        "Maximum header count or maximum header length is exceeded");
            break;
        case -2:
            servererror(fd);
            break;
        }

        clear_headers(&headers);
        return;
    }

    forward(fd, &reciever, &headers);
    clear_headers(&headers);
}

/* This is the part of the request pipeline, where the request is being send to the server
 * and the response is being forwarded to the client */
static void forward(int fd, Uri_info *uri_info, Headers *headers)
{
    jmp_buf fd_closed;
    if (setjmp(fd_closed))
        return;

    /* Returning cached data if present */
    void *cached = NULL;
    size_t cached_length;
    char key[MAXLINE];
    if (build_cache_key(key, MAXLINE, uri_info))
    {
        if (cache_get(&cache, key, &cached, &cached_length) > 0)
        {
            Rio_writen(fd, cached, cached_length, fd_closed);
            return;
        }
    }

    rio_t rio;
    int clientfd;
    if ((clientfd = Open_clientfd(uri_info->hostname, uri_info->port)) < 0)
    {
        switch (clientfd)
        {
        case -1:
            servererror(fd);
            break;
        case -2:
            clienterror(fd, uri_info->hostname, "400", "Host not found",
                        "The DNS entry for the hostname was not resolved");
            break;
        }

        return;
    }

    /* jmp if clientfd closes connection */
    jmp_buf client_close_conn;
    if (setjmp(client_close_conn))
    {
        Close(clientfd);
        servererror(fd);
        return;
    }

    /* Sending the HTTP request line to the client socket */
    Rio_readinitb(&rio, clientfd);
    char buf[MAXLINE];
    sprintf(buf, "GET ");
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
    sprintf(buf, "%s", uri_info->query);
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
    sprintf(buf, " HTTP/1.0\r\n");
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
    /* Sending the HTTP headers from the request to the client socket */
    int host_in_headers = 0;
    int i;
    for (i = 0; i < headers->cout; i++)
    {
        if (!host_in_headers && strstr(headers->data[i], "Host:"))
            host_in_headers = 1;
        else if (strstr(headers->data[i], "User-Agent:") ||
                 strstr(headers->data[i], "Connection:") ||
                 strstr(headers->data[i], "Proxy-Connection"))
            continue;

        Rio_writen(clientfd, headers->data[i], strlen(headers->data[i]), client_close_conn);
    }

    if (!host_in_headers)
    {
        /* Sending a Host header */
        sprintf(buf, "Host: ");
        Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
        sprintf(buf, "%s", uri_info->hostname);
        Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
        sprintf(buf, "\r\n");
    }

    /* Sending a User-Agent header */
    sprintf(buf, "%s", user_agent_hdr);
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
    /* Sending a Connection header */
    sprintf(buf, "%s", connection_hdr);
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
    /* Sending a Proxy-Connection header */
    sprintf(buf, "%s", proxy_connection_hdr);
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);
    /* Sending the end of the request */
    sprintf(buf, "\r\n");
    Rio_writen(clientfd, buf, strlen(buf), client_close_conn);

    /* jmp is the client close connection while the request is being forwarded */
    jmp_buf fd_closed_and_client_opened;
    if (setjmp(fd_closed_and_client_opened))
    {
        Close(clientfd);
        return;
    }

    /* Creating a temp cache buffer */
    size_t cached_len = 0;
    int cacheable = 1;
    char cache_buffer[MAX_OBJECT_SIZE];
    char *cache_free = cache_buffer;
    /* Transmitting the response from the client socket to the listening socket*/
    Rio_readinitb(&rio, clientfd);
    size_t response_len = 0;
    while ((response_len = Rio_readnb(&rio, buf, MAXLINE)) > 0)
    {
        if (cacheable)
        {
            if (cached_len + response_len > MAX_OBJECT_SIZE)
            {
                cacheable = 0;
            }
            else
            {
                memcpy(cache_free, buf, response_len);
                cached_len += response_len;
                cache_free += response_len;
            }
        }

        Rio_writen(fd, buf, response_len, fd_closed_and_client_opened);
    }

    Close(clientfd);
    /* Caching the respone */
    if (!(response_len < 0) && cacheable)
    {
        char key[MAXLINE];
        if (build_cache_key(key, MAXLINE, uri_info))
            cache_add(&cache, key, cache_buffer, cached_len);
    }
}

static int parse_uri(const char *uri, Uri_info *uri_info)
{
    char buf[MAXLINE];
    char *index = buf;
    /* Parsing a protocol */
    while (*uri != ':')
    {
        if (*uri == '\0')
            return -1;

        *index++ = *uri++;
    }

    *index = '\0';
    if (strcasecmp(buf, "http") && strcasecmp(buf, "https"))
        return -1;

    /* Omitting duplicated slashes */
    uri++;
    while (*uri == '/')
        uri++;

    /* Parsing a hostname */
    index = buf;
    while (*uri != ':' && *uri != '/')
    {
        if (*uri == '\0')
            return -1;

        *index++ = *uri++;
    }

    *index = '\0';
    /* Filling Uri_info with the hostname */
    size_t hostname_len = strlen(buf) + 1;
    if (hostname_len > MAX_HOSTNAME_LEN)
        return -1;

    memcpy(uri_info->hostname, buf, hostname_len);
    /* Parsing a port value */
    index = buf;
    if (*uri == ':')
    {
        uri++;
        while (*uri != '/')
        {
            if (*uri == '\0')
                return -1;

            *index++ = *uri++;
        }

        *index = '\0';
        /* Filling Uri_info with the port value*/
        size_t port_len = strlen(buf) + 1;
        if (port_len > MAX_PORT_LEN)
            return -1;

        memcpy(uri_info->port, buf, port_len);
    }
    else
    {
        char *default_port = "80";
        strcpy(uri_info->port, default_port);
    }

    uri++;
    /* Omitting duplicated slashes*/
    while (*uri == '/')
        uri++;

    /* Parsing a query */
    uri--;
    index = buf;
    while (*uri != '\0')
        *index++ = *uri++;

    *index = '\0';
    /* Filling Uri_info with query data */
    size_t query_len = strlen(buf) + 1;
    if (query_len > MAX_QUERY_LEN)
        return -1;

    memcpy(uri_info->query, buf, query_len);

    return 0;
}

static int parse_headers(rio_t *rp, Headers *headers)
{
    headers->cout = 0;
    char buf[MAXLINE];
    ssize_t line_len;
    /* Reading first header line from the connected socket*/
    if ((line_len = Rio_readlineb(rp, buf, MAXLINE)) < 0)
        return -2;

    /* Checking if the header line length is less than MAXLINE */
    if (line_len == MAXLINE - 1)
        if (buf[line_len - 1] != '\n')
            return -1;

    /* While the line is not the end of the request */
    while (strcmp(buf, "\r\n"))
    {
        if (headers->cout > MAX_HEADERS_NUMBER)
            return -1;

        /* Copying the header to the Headers struct*/
        char *header;
        if ((header = Malloc(line_len + 1)) == NULL)
            return -2;

        memcpy(header, buf, line_len + 1);
        headers->data[headers->cout++] = header;

        /* Reading next line */
        if ((line_len = Rio_readlineb(rp, buf, MAXLINE)) < 0)
            return -2;

        /* Checking if the header line length is less than MAXLINE */
        if (line_len == MAXLINE - 1)
            if (buf[line_len - 1] != '\n')
                return -1;
    }

    return 0;
}

static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg)
{
    jmp_buf fd_closed;
    if (setjmp(fd_closed))
        return;

    char buf[MAXLINE];
    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf), fd_closed);
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf), fd_closed);

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Proxy Error</title>");
    Rio_writen(fd, buf, strlen(buf), fd_closed);
    sprintf(buf, "<body bgcolor="
                 "ffffff"
                 ">\r\n");
    Rio_writen(fd, buf, strlen(buf), fd_closed);
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf), fd_closed);
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf), fd_closed);
    sprintf(buf, "<hr><em>The Proxy</em>\r\n");
    Rio_writen(fd, buf, strlen(buf), fd_closed);
}

static void servererror(int fd)
{
    clienterror(fd, "", "500", "Internal server error",
                "Something went wrong");
}

static void clear_headers(Headers *headers)
{
    int i;
    for (i = 0; i < headers->cout; i++)
        free(headers->data[i]);
}

static int build_cache_key(char key[], size_t length, Uri_info *uri_info)
{
    size_t key_length = strlen(uri_info->hostname) +
                        strlen(uri_info->port) +
                        strlen(uri_info->query);
    if (length < key_length + 1)
        return 0;

    memset(key, 0, length);
    strcat(key, uri_info->hostname);
    strcat(key, uri_info->port);
    strcat(key, uri_info->query);
    return 1;
}