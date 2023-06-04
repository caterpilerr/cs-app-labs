#include "cache.h"

/* Max size of a cacheable request */
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_SIZE 1049000

#define PAYLOAD(node_p) ((char *)node_p + sizeof(CacheNode))
#define NODE_SIZE(payload_size) (sizeof(CacheNode) + payload_size)

static void prepend_item(Cache *cache, CacheNode *item);
static void remove_item(Cache *cache, CacheNode *item);
static void append_lru(Cache *cache, LruNode *item);
static void remove_lru(Cache *cache, LruNode *item);

int cache_init(Cache *cache)
{
    cache->total_size = 0;
    cache->head = NULL;
    if (Sem_init(&cache->readers_lock, 0, 1) < 0)
        return -1;

    if (Sem_init(&cache->writers_lock, 0, 1) < 0)
        return -1;

    return 0;
}

int cache_add(Cache *cache, const char *key, const void *buf, size_t length)
{
    if (length > MAX_OBJECT_SIZE)
        return 0;

    /* Holding writers lock */
    if (Sem_wait(&cache->writers_lock) < 0)
        return -1;

    /* LRU eviction */
    while (cache->total_size + length > MAX_CACHE_SIZE)
    {
        CacheNode *to_evict = cache->lru_head->item;
        cache->total_size -= to_evict->size;
        remove_item(cache, to_evict);
        remove_lru(cache, to_evict->lru);
        printf("Evicted fromt the cache: %s, freed %d bytes.\n", to_evict->key, (int)to_evict->size);
        free(to_evict->lru);
        free(to_evict);
    }

    /* Creating a new cache node */
    CacheNode *new_item;
    if ((new_item = Malloc(NODE_SIZE(length))) < 0)
        return -1;

    /* Creating a key copy */
    char *key_copy;
    if ((key_copy = Malloc(strlen(key) + 1)) < 0)
        return -1;

    memcpy(key_copy, key, strlen(key) + 1);
    /* Allocating a new lru node */
    LruNode *new_lru;
    if ((new_lru = Malloc(sizeof(LruNode))) < 0)
        return -1;

    new_lru->item = new_item;
    /* Updating item fields */
    new_item->key = key_copy;
    new_item->size = length;
    new_item->lru = new_lru;
    /* Copying data to the cache node */
    void *data = PAYLOAD(new_item);
    memcpy(data, buf, length);
    /* Updating linked lists */
    prepend_item(cache, new_item);
    append_lru(cache, new_lru);
    printf("Writen to the cache %s. %d bytes.\n", new_item->key, (int)new_item->size);
    if (Sem_post(&cache->writers_lock) < 0)
        return -1;
    
    return 0;
}

int cache_get(Cache *cache, const char *key, void **item, size_t *length)
{
    int is_cached = 0;
    /* Aquiring writers lock if it is the first reader */
    if (Sem_wait(&cache->readers_lock) < 0)
        return -1;

    cache->readers_count++;
    if (cache->readers_count == 1)
        if (Sem_wait(&cache->writers_lock) < 0)
            return -1;

    if (Sem_post(&cache->readers_lock) < 0)
        return -1;

    /* Key sequential lookup */
    CacheNode *current;
    for (current = cache->head; current != NULL; current = current->next)
    {
        if (!strcmp(current->key, key))
        {
            /* Moving the lru node to the end of the LRU list */
            if (Sem_wait(&cache->readers_lock) < 0)
                return -1;

            remove_lru(cache, current->lru);
            append_lru(cache, current->lru);
            if (Sem_post(&cache->readers_lock) < 0)
                return -1;

            /* Copying the cached value */
            if ((*item = Malloc(current->size)) < 0)
                return -1;

            memcpy(*item, PAYLOAD(current), current->size);
            *length = current->size;
            is_cached = 1;
            break;
        }
    }

    /* Releasing writers lock if it is the last reader */
    if (Sem_wait(&cache->readers_lock) < 0)
        return -1;

    cache->readers_count--;
    if (cache->readers_count == 0)
        if (Sem_post(&cache->writers_lock) < 0)
            return -1;

    if (Sem_post(&cache->readers_lock) < 0)
        return -1;

    if (is_cached)
        printf("Cache hit %s\n", key);
    else
        printf("Cache miss %s\n", key);

    return is_cached;
}

/* Appending an item to the end of the LRU linked list */
static void append_lru(Cache *cache, LruNode *item)
{
    item->next = NULL;
    item->prev = cache->lru_tail;
    if (cache->lru_tail != NULL)
        cache->lru_tail->next = item;
    else
        cache->lru_head = item;

    cache->lru_tail = item;
}

/* Removing an item from the LRU linked list*/
static void remove_lru(Cache *cache, LruNode *item)
{
    LruNode *prev = item->prev;
    LruNode *next = item->next;
    if (prev != NULL)
        prev->next = next;
    else
        cache->lru_head = next;

    if (next != NULL)
        next->prev = prev;
    else
        cache->lru_tail = prev;
}

/* Prepending an item to the start of the cached items linked list */
static void prepend_item(Cache *cache, CacheNode *item)
{
    item->prev = NULL;
    item->next = cache->head;
    if (cache->head != NULL)
        cache->head->prev = item;

    cache->head = item;
    cache->total_size += item->size;
}

/* Removing an item from the cached items linked list */
static void remove_item(Cache *cache, CacheNode *item)
{
    CacheNode *next = item->next;
    CacheNode *prev = item->prev;
    if (prev != NULL)
        prev->next = next;
    else
        cache->head = next;

    if (next != NULL)
        next->prev = prev;
}