#include "sbuf.h"

int sbuf_init(sbuf_t *sp, int n)
{
    if ((sp->buf = Calloc(n, sizeof(int))) < 0)
        return -1;

    sp->n = n;
    sp->front = sp->rear = 0;
    if (Sem_init(&sp->mutex, 0, 1) < 0)
        return -1;

    if (Sem_init(&sp->slots, 0, n) < 0)
        return -1;

    if (Sem_init(&sp->items, 0, 0) < 0)
        return -1;

    return 0;
}

void sbuf_free(sbuf_t *sp)
{
    Free(sp->buf);
}

int sbuf_insert(sbuf_t *sp, int item)
{
    if (Sem_wait(&sp->slots) < 0)
        return -1;

    if (Sem_wait(&sp->mutex) < 0)
        return -1;

    sp->buf[(++sp->rear) % (sp->n)] = item;
    if (Sem_post(&sp->mutex) < 0)
        return -1;

    if (Sem_post(&sp->items) < 0)
        return -1;

    return 0;
}

int sbuf_remove(sbuf_t *sp, int *item)
{
    if (Sem_wait(&sp->items) < 0)
        return -1;

    if (Sem_wait(&sp->mutex) < 0)
        return -1;

    *item = sp->buf[(++sp->front) % (sp->n)];
    if (Sem_post(&sp->mutex) < 0)
        return -1;

    if (Sem_post(&sp->slots) < 0)
        return -1;

    return 0;
}