#include "cache.h"
#include "assert.h"
#include "string.h"
#include "stdlib.h"

static void free_cache_node(cache_node *cn);
static void enqueue(memcache *mc, cache_node * cn);
static void dequeue(memcache *mc, cache_node *keep_cn);
static void delete_node(memcache *mc, cache_node *cn);
static cache_node *new_cache_node();
static int is_key_equal(cache_node *nd, char *key);

//debug function
static void printf_cache_node(cache_node *cn);
static void printf_cache(memcache *mc);

void cache_init(memcache *mc) {
	assert(mc != NULL);
	mc->head = NULL;
	mc->tail = NULL;
	mc->node_count = 0;
	mc->sz_cache = 0;
}

void free_cache(memcache *mc) {
	cache_node *cp = mc->head;
	cache_node *old_cp = NULL;
	for (int i = 0; i < mc->node_count; ++i) {
		old_cp = cp;
		cp = cp->next;
		free_cache_node(old_cp);
	}
}

cache_node *cache_get(memcache *mc, char *key) {
	cache_node *ret = mc->tail;
	for(; ret != NULL; ret = ret->prev) {
		if (is_key_equal(ret, key)) {
			return ret;
		}
	}
	return NULL;
}

void cache_set(memcache *mc, char *key, char *data, size_t sz_data) {
	assert(sz_data < MAX_OBJECT_SIZE);
	cache_node *cn = cache_get(mc, key);
	if (cn != NULL) {
		delete_node(mc, cn);
	}
	// new node
	//printf("cache size: %d\n", mc->sz_cache);
	while (mc->sz_cache + sz_data > MAX_CACHE_SIZE) {
		dequeue(mc, cn);
		printf("Dequeue\n");
	}
	//printf("after cache size: %d\n", mc->sz_cache);
	cn = new_cache_node();
	cn->key =(char *)Malloc(strlen(key) + 1);
	cn->data = Malloc(sz_data);
	cn->sz_data = sz_data;
	memcpy(cn->key, key, strlen(key) + 1);
	memcpy(cn->data, data, sz_data);
	// enqueue
	enqueue(mc, cn);
}

static void delete_node(memcache *mc, cache_node *cn) {

	if (mc->head == mc->tail) {
		mc->head = NULL;
		mc->tail = NULL;
	} else if (cn->prev == NULL) {
		mc->head = cn->next;
		(cn->next)->prev = NULL;
	} else if (cn->next == NULL) {
		mc->tail = cn->prev;
		(cn->prev)->next = NULL;
	} else {
		(cn->prev)->next = cn->next;
		(cn->next)->prev = cn->prev;
	}
	mc->sz_cache -= cn->sz_data;
	mc->node_count--;
}

int is_key_equal(cache_node *nd, char *key) {
	if (strcasecmp(nd->key, key) != 0) {
		return 0;
	}
	return 1;
}

static void free_cache_node(cache_node *cn) {
	if (cn->key != NULL) {
		Free(cn->key);
		Free(cn->data);
	}
	Free(cn);
}

static cache_node *new_cache_node() {
	cache_node *cn = (cache_node *) Malloc(sizeof(cache_node));
	cn->key = NULL;
	cn->data = NULL;
	cn->sz_data = -1;
	cn->prev = NULL;
	cn->next = NULL;
	return cn;
}

static void enqueue(memcache *mc, cache_node * cn) {
	if (mc->node_count == 0) {
		mc->head = cn;
		mc->tail = cn;
		cn->next = NULL;
		cn->prev = NULL;
	} else {
		(mc->tail)->next = cn;
		cn->prev = mc->tail;
		cn->next = NULL;
		mc->tail = cn;
	}
	assert((mc->head)->prev == NULL);
	assert((mc->tail)->next == NULL);
	mc->node_count += 1;
	mc->sz_cache += cn->sz_data;
}

static void dequeue(memcache *mc, cache_node *keep_cn) {
	cache_node *expired_node;
	assert(mc->node_count >= 0);
	if (mc->node_count == 0)
		return;

	if (mc->node_count == 1) {
		expired_node = mc->head;
		assert(mc->head == mc->tail);
		mc->head = NULL;
		mc->tail = NULL;
	} else {
		expired_node = mc->head;
		(mc->head)->prev = NULL;
		mc->head = (mc->head)->next;
		assert((mc->head)->prev == NULL);
		assert((mc->tail)->next == NULL);
	}
	mc->node_count -= 1;
	mc->sz_cache -= expired_node->sz_data;
}

//debug function

static void printf_cache_node(cache_node *cn) {
	printf("  { key: %s, data_ptr:%p, sz_data:%d, prev:%p, next:%p }\n", cn->key, cn->data, cn->sz_data, cn->prev, cn->next);
}

static void printf_cache(memcache *mc) {
	printf("===== node_count:%d, total_size:%d, head:%p, tail: %p ====\n", mc->node_count, mc->sz_cache, mc->head, mc->tail);
	cache_node *ptr = mc->head;
	for (; ptr != mc->tail; ptr = ptr->next) {
		printf_cache_node(ptr);
	}
	if (mc->tail != NULL)
		printf_cache_node(ptr);
}


void test_cache() {
	memcache mc;
	cache_init(&mc);
	printf_cache(&mc);
	char *key1 = "a";
	char *key2 = "b";
	char *key3 = "c";
	char *data1 = "aaaa";
	char *data2 = "bbbb";
	char *data3 = "cccc";
	cache_set(&mc, key1, data1, 4);
	printf_cache(&mc);
	cache_set(&mc, key2, data2, 4);
	printf_cache(&mc);
	cache_set(&mc, key3, data3, 4);
	printf_cache(&mc);

	free_cache(&mc);
}
