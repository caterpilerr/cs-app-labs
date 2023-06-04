#include "csapp.h"

typedef struct cache Cache;
typedef struct item CacheNode;
typedef struct lru LruNode;

struct cache
{
    int readers_count;
    size_t total_size;
    CacheNode *head;
    LruNode *lru_head;
    LruNode *lru_tail;
    sem_t readers_lock;
    sem_t writers_lock;
}; 

struct item
{
    char *key;
    size_t size;
    LruNode *lru;
    CacheNode *next;
    CacheNode *prev;
};

struct lru
{
    CacheNode *item;
    LruNode *prev;
    LruNode *next;
}; 

int cache_init(Cache *cache);
int cache_add(Cache *cache, const char *key, const void *buf, size_t length);
int cache_get(Cache *cache, const char *key, void **item, size_t *length);