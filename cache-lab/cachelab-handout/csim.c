#define _GNU_SOURCE
#include "cachelab.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>

extern char *optarg;

typedef struct {
	uint32_t hits;
	uint32_t misses;
	uint32_t evictions;
} CacheStat;

typedef struct {
	bool h;
	bool v;
	uint32_t s;
	uint32_t E;
	uint32_t b;
	char *file;
} Option;

typedef struct {
	uint32_t S;
	uint32_t E;
	uint32_t s_bits;
	uint32_t b_bits;
	uint32_t t_bits;
} LRUCacheConfig;

typedef struct {
	bool is_valid;
	uint64_t tag;
	uint32_t time_stamp;
	uint32_t data[0]; // dummy data
} CacheLine;

typedef struct {
	char op;
	uint64_t address;
	uint64_t size;
} MemoryOperation;

// Global value
CacheLine **cache = NULL;
uint32_t time_stamp = 0;
const char* help_str =\
"Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
"Options:\n"
"  -h         Print this help message.\n"
"  -v         Optional verbose flag.\n"
"  -s <num>   Number of set index bits.\n"
"  -E <num>   Number of lines per set.\n"
"  -b <num>   Number of block offset bits.\n"
"  -t <file>  Trace file.\n"
"\n"
"Examples:\n"
"  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
"  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";


int arg_parse(int argc, char* argv[], Option *option);

void print_args(Option *args);
void print_cache_config(LRUCacheConfig *config);
void print_cache_content(LRUCacheConfig *config);
void print_cache_line(CacheLine *line);

void init_LRUCache(LRUCacheConfig *config);
void free_LRUCache(LRUCacheConfig *config);

uint64_t get_tag(const LRUCacheConfig* config, uint64_t address);
uint64_t get_set_idx(const LRUCacheConfig* config, uint64_t address);

void LRUCacheAccess(MemoryOperation *op, LRUCacheConfig *cache_config, CacheStat *cache_stats, bool verbose);

int main(int argc, char* argv[])
{
	Option args;
	int r;
	r = arg_parse(argc, argv, &args);
	if ( r != 0 ){
		return -1;
	}

	LRUCacheConfig cache_config;
	cache_config.s_bits = args.s;
	cache_config.b_bits = args.b;
	cache_config.t_bits = 64 - (args.s + args.b);
	cache_config.S = 1 << args.s;
	cache_config.E = args.E;
	//print_args(&args);
	CacheStat cache_stats;
	cache_stats.hits = 0;
	cache_stats.misses = 0;
	cache_stats.evictions = 0;

	//init cache
	init_LRUCache(&cache_config);

	//read cache
	MemoryOperation op;
	FILE *infile = fopen(args.file, "r");
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, infile)) != -1) {
		if(line[0] != ' ') continue;
		sscanf(line+1, "%c %lx,%lu", &op.op, &op.address, &op.size);
		LRUCacheAccess(&op, &cache_config, &cache_stats, args.v);
	}

	free(line);
	//print_cache_content(&cache_config);
	printSummary(cache_stats.hits, cache_stats.misses, cache_stats.evictions);
	free_LRUCache(&cache_config);
	return 0;
}

void LRUCacheAccess(MemoryOperation *op, LRUCacheConfig *cache_config, CacheStat *cache_stats, bool verbose) {
	uint64_t set_idx = get_set_idx(cache_config, op->address);
	CacheLine *current_set = cache[set_idx];
	uint64_t tag = get_tag(cache_config, op->address);
	//printf("Tag = %ld, Set Index = %ld\n", tag, set_idx);
	bool is_miss = true;
	uint64_t oldest_line_idx = 0;
	int i;
	for (i = 0; i < cache_config->E; i++) {
		uint64_t oldest_line_timestamp = current_set[oldest_line_idx].time_stamp;
		uint64_t curr_line_timestamp = current_set[i].time_stamp;
		oldest_line_idx = oldest_line_timestamp > curr_line_timestamp ? i : oldest_line_idx;
		if (current_set[i].is_valid && current_set[i].tag == tag) {
			is_miss = false;
			break;
		}
	}

	time_stamp++;
	// update cache
	bool need_eviction = false;
	if (is_miss) {
		need_eviction = current_set[oldest_line_idx].is_valid;
		current_set[oldest_line_idx].is_valid = true;
		current_set[oldest_line_idx].tag = tag;
		current_set[oldest_line_idx].time_stamp = time_stamp;
	} else {
		//if not cache miss, only update the timestamp
		current_set[i].time_stamp = time_stamp;
	}

	//handle statistic
	if (is_miss) {
		cache_stats->misses++;
	} else {
		cache_stats->hits++;
	}

	if (need_eviction) {
		cache_stats->evictions++;
	}

	if (op->op == 'M') {
		cache_stats->hits++;
	}

	// handle verbose output
	if (verbose) {
		printf("%c %lx, %lu", op->op, op->address, op->size);
		if (is_miss) {
			printf(" miss");
		} else {
			printf(" hit");
		}
		if (need_eviction) {
			printf(" eviction");
		}
		if (op->op == 'M') {
			printf(" hit");
		}
		printf("\n");
	}
}

uint64_t get_tag(const LRUCacheConfig* config, uint64_t address) {
	return (address >> (64 - config->t_bits));
}

uint64_t get_set_idx(const LRUCacheConfig* config, uint64_t address) {
	uint64_t mask = (1 << (64 - config->t_bits)) - 1;
	return (address & mask) >> config->b_bits;
}

void init_LRUCache(LRUCacheConfig *config) {
	//alloc cache row
	cache = (CacheLine **)malloc(config->S * sizeof(CacheLine *));

	//alloc cache column
	for (int i = 0; i < config->S; i++) {
		cache[i] = (CacheLine *)calloc(config->E,  sizeof(CacheLine));
	}
}

void free_LRUCache(LRUCacheConfig *config) {
	for (int i = 0; i < config->S; i++) {
		free(cache[i]);
	}
	free(cache);
}

int arg_parse(int argc, char* argv[], Option *args) {
	if (argc == 1) {
		fprintf(stderr, "%s: Missing required command line argument\n", argv[0]);
		fprintf(stderr, "%s", help_str);
		return -1;
	}

	args->h = false;
	args->v = false;
	args->s = 0;
	args->E = 0;
	args->b = 0;
	args->file = NULL;

	char c;
	while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
		switch (c) {
			case 'h':
				args->h = true;
				break;
			case 'v':
				args->v = true;
				break;
			case 's':
				args->s = atoi(optarg);
				break;
			case 'E':
				args->E = atoi(optarg);
				break;
			case 'b':
				args->b = atoi(optarg);
				break;
			case 't':
				args->file = optarg;
				break;
			default:
				return false;
		}
	}

	if (args->h) {
		printf("%s", help_str);
		return -1;
	}

	if (args->s == 0 || args->E == 0 ||
			args->b == 0 || args->file == NULL) {
		fprintf(stderr, "%s: Missing required command line argument\n", argv[0]);
		fprintf(stderr, "%s", help_str);
		return -1;
	}

	return 0;
}

void print_args(Option *args) {
	if (args->h) printf("-h\n");
	if (args->v) printf("-v\n");
	printf("-s %d\n", args->s);
	printf("-E %d\n", args->E);
	printf("-b %d\n", args->b);
	printf("-t %s\n", args->file);
}

void print_cache_config(LRUCacheConfig *config) {
	printf("Set bits: %u\n", config->s_bits);
	printf("Block bits: %u\n", config->b_bits);
	printf("Tag bits: %u\n", config->t_bits);
	printf("Number of sets: S = %u\n", config->S);
	printf("Number of Lines: E = %u\n", config->E);
}

void print_cache_content(LRUCacheConfig *config) {
	for (int i = 0; i < config->S; i++) {
		for (int j = 0; j < config->E; j++) {
			printf("i=%d, j=%d\n",i,j);
			print_cache_line(&cache[i][j]);
		}
	}
}

void print_cache_line(CacheLine *line) {
	printf("valid bit: %d\n", line->is_valid);
	printf("tag: 0x%8lx\n", line->tag);
	printf("time stamp: %d\n", line->time_stamp);
}
