#ifndef PROXY_CACHE
#define PROXY_CACHE

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_node {
	char *key;
	void *data;
	size_t sz_data;
	struct cache_node *prev;
	struct cache_node *next;

} cache_node;

typedef struct memcache {
	size_t node_count;
	size_t sz_cache;
	cache_node *head;
	cache_node *tail;
} memcache;

void cache_init(memcache *mc);
void free_cache(memcache *mc);

cache_node *cache_get(memcache *mc, char *key);
void cache_set(memcache *mc, char *key, char *data, size_t sz_data);



//test function
void test_cache();

#endif
