/*
 * mm.c - The seggrated segregated free list based malloc
 * the block layout is as follows:
 * [Header: <size>(31 bits) <is_allocated>(1 bits), *prev, *next]
 * [payload]
 * [Footer: <size>(30 bits) <is_allocated>(1 bits)]
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
	/* Team name */
	"ateam",
	/* First member's full name */
	"Yuyang Huang",
	/* First member's email address */
	"sigefiredhyy@gmail.com",
	/* Second member's full name (leave blank if none) */
	"",
	/* Second member's email address (leave blank if none) */
	""
};

//#define DEBUG

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */
#define FREE 0
#define ALLOCATED 1

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET_BYTE(p)       (*(char *)(p))
#define GET_WORD(p)       (*(unsigned int *)(p))
#define PUT_WORD(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET_WORD(p) & ~0x7)
#define GET_ALLOC(p) (GET_WORD(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// given the block ptr bp, compute address of the next and previous free block
#define GET_SUCC(bp)  (*(unsigned int *)(bp))
#define GET_PRED(bp)  (*(unsigned int *)((char *)(bp) + WSIZE))
#define PUT_SUCC(bp, val)  (*(unsigned int *)(bp) = ((unsigned int) val))
#define PUT_PRED(bp, val)  (*(unsigned int *)((char *)(bp) + WSIZE) = ((unsigned int) val))


// free list manipulation function
#define GET_FREE_LIST_PTR(i) (*(free_list + i))
#define SET_FREE_LIST_PTR(i, val) (*(free_list + i) = (val))

// math
// /* Round up to even */
#define ROUNDUP_EVEN(x) ((x + 1) & ~1)

/* Global variables */
static char *heap_listp;  /* Pointer to first block */
static char **free_list;
// free list
static int free_list_size = 10;

// Function Declartion
static char *extend_heap(size_t words);
static char *coalesce(void *bp);
static void place(void *bp, size_t asize);
static char *find_fit(size_t asize);
static void insert_node(void *bp);
static void delete_node(void *bp, size_t size_of_blk);
static int get_free_list_idx(size_t size);


//debug function
int mm_check(int verbose, char *msg);
static void baisc_block_check(int verbose);
static int check_free_blocks_marked_free(char *bp);
static int check_free_list_marked_free();
static int check_free_list_asending_order();
static int check_free_blocks_asending_order(char *bp);
static void check_block(char *bp);
static void printblock(void *bp, char *msg);

void unix_error(char *msg)
{
	fprintf(stdout, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

void app_error(char *msg)
{
	fprintf(stdout, "%s\n", msg);
	exit(1);
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
	// init the free list
	void *new_blk;
	int roundup_free_list_size = ROUNDUP_EVEN(free_list_size);
	if ((free_list = (char **)mem_sbrk(roundup_free_list_size * sizeof(char *))) == (char **) -1)
		return -1;

	//init free list
	for (int i = 0; i < free_list_size; ++i) {
		SET_FREE_LIST_PTR(i, NULL);
	}

	/* Create the initial empty heap */
	if ((heap_listp = (char *) mem_sbrk(4*WSIZE)) == (void *)-1) //line:vm:mm:begininit
		return -1;
	PUT_WORD(heap_listp, 0);                          /* Alignment padding */
	PUT_WORD(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
	PUT_WORD(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
	PUT_WORD(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
	heap_listp += (2*WSIZE);                     //line:vm:mm:endinit
	/* $end mminit */


	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if ((new_blk = extend_heap(CHUNKSIZE/WSIZE)) == NULL) {
		return -1;
	}
	//mm_check(1, "check after mm_init");
	return 0;

}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	char *bp;

	/* $end mmmalloc */
	if (heap_listp == NULL){
		mm_init();
	}
	/* $begin mmmalloc */
	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
		asize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); //line:vm:mm:sizeadjust3

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
		//mm_check(1, "check at mm_malloc fit");
		place(bp, asize);                  //line:vm:mm:findfitplace
		return bp;
	}

	//mm_check(1, "check at mm_malloc extend");
	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;                                  //line:vm:mm:growheap2
	place(bp, asize);                                 //line:vm:mm:growheap3
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	if (ptr == NULL) return;

	/* $end mmfree */
	if (heap_listp == 0){
		mm_init();
	}

	/* $begin mmfree */
	size_t size = GET_SIZE(HDRP(ptr));

	PUT_WORD(HDRP(ptr), PACK(size, 0));
	PUT_WORD(FTRP(ptr), PACK(size, 0));

	//mm_check(1, "check at mm_free before coalesce");
	coalesce(ptr);

	return;

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	size_t oldsize;
	void *newptr;

	/* If size == 0 then this is just free, and we return NULL. */
	if(size == 0) {
		mm_free(ptr);
		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(ptr == NULL) {
		return mm_malloc(size);
	}

	size_t reqsize;
	if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
		reqsize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
	else
		reqsize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); //line:vm:mm:sizeadjust3

	oldsize = GET_SIZE(HDRP(ptr));

	//if (oldsize > reqsize)
	//	printf ("old size:%d, reqsize:%d, diff:%d\n", oldsize, reqsize,oldsize-reqsize);

	if (reqsize == oldsize) {
		return ptr;
	} else if ( reqsize < oldsize ) {
		if ((oldsize - reqsize) < 2 * DSIZE )
			return ptr;

		PUT_WORD(HDRP(ptr), PACK(reqsize, 1));
		PUT_WORD(FTRP(ptr), PACK(reqsize, 1));
		void *nbp = NEXT_BLKP(ptr);
		PUT_WORD(HDRP(nbp), PACK(oldsize - reqsize, 1));
		PUT_WORD(FTRP(nbp), PACK(oldsize - reqsize, 1));
		mm_free(nbp);
		return ptr;
	} else {
		// check next block is free or not
		void *nbp = NEXT_BLKP(ptr);
		if (!GET_ALLOC(HDRP(nbp)) || !GET_SIZE(HDRP(nbp))) {
			size_t nsize = oldsize + GET_SIZE(HDRP(nbp));
			if (!GET_SIZE(HDRP(nbp))) {
				int rem = reqsize - nsize;
				int extendsize = MAX(rem,CHUNKSIZE);                 //line:vm:mm:growheap1
				if ((nbp = extend_heap(extendsize/WSIZE)) == NULL)
					return NULL;                                  //line:vm:mm:growheap2
				nsize += extendsize;
			}
			if (nsize > reqsize) {
				//mm_check(1, "old before realloc");
				delete_node(nbp, GET_SIZE(HDRP(nbp)));
				PUT_WORD(HDRP(ptr), PACK(nsize, 1));
				PUT_WORD(FTRP(ptr), PACK(nsize, 1));
				//mm_check(1, "old after realloc");
				memcpy(ptr, ptr, oldsize);

				return ptr;
			}

		}

	}

	newptr = mm_malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if(!newptr) {
		return 0;
	}

	/* Copy the old data. */
	if(size < oldsize) oldsize = size;
	memcpy(newptr, ptr, oldsize);

	/* Free the old block. */
	mm_free(ptr);

	return newptr;
}



static char *extend_heap(size_t words) {
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
	if ((long)(bp = (char *)mem_sbrk(size)) == -1)
		return NULL;                                        //line:vm:mm:endextend

	/* Initialize free block header/footer and the epilogue header */
	PUT_WORD(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
	PUT_WORD(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
	PUT_WORD(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr

	/* Coalesce if the previous block was free */
	//mm_check(1, "check at extend_heap before coalesce");
	return coalesce(bp);

}

static char *coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) {            /* Case 1 */
		// alloc -> NODE -> alloc
		// do nothing
	}

	else if (prev_alloc && !next_alloc) {      /* Case 2 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		delete_node(NEXT_BLKP(bp),GET_SIZE(HDRP(NEXT_BLKP(bp))));
		PUT_WORD(HDRP(bp), PACK(size, 0));
		PUT_WORD(FTRP(bp), PACK(size,0));
	}

	else if (!prev_alloc && next_alloc) {      /* Case 3 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		bp = PREV_BLKP(bp);
		delete_node(bp,GET_SIZE(HDRP(bp)));
		PUT_WORD(HDRP(bp), PACK(size, 0));
		PUT_WORD(FTRP(bp), PACK(size, 0));
	}

	else {                                     /* Case 4 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
			GET_SIZE(HDRP(NEXT_BLKP(bp)));
		void *pbp = PREV_BLKP(bp);
		delete_node(pbp, GET_SIZE(HDRP(pbp)));
		void *nbp = NEXT_BLKP(bp);
		delete_node(nbp, GET_SIZE(HDRP(nbp)));
		PUT_WORD(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT_WORD(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	/* $end mmfree */
	//mm_check(1, "check at coalesce before insert node");
	insert_node(bp);
	//mm_check(1, "check after coalesce");

	return (char *)bp;
}

static void place(void *bp, size_t asize) {
	size_t csize = GET_SIZE(HDRP(bp));
	char *next_bp;

	delete_node(bp, csize);

	if ((csize - asize) >= (2*DSIZE)) {
		PUT_WORD(HDRP(bp), PACK(asize, 1));
		PUT_WORD(FTRP(bp), PACK(asize, 1));
		next_bp = NEXT_BLKP(bp);
		PUT_WORD(HDRP(next_bp), PACK(csize-asize, 0));
		PUT_WORD(FTRP(next_bp), PACK(csize-asize, 0));
		coalesce(next_bp);
	}
	else {
		PUT_WORD(HDRP(bp), PACK(csize, 1));
		PUT_WORD(FTRP(bp), PACK(csize, 1));
	}
	//mm_check(1, "check after place");

}

static char *find_fit(size_t asize) {
	// using first fit strategy
	char *bp;
	int list_idx = get_free_list_idx(asize);
	for (; list_idx < free_list_size; ++list_idx) {
		for (bp = GET_FREE_LIST_PTR(list_idx); bp != NULL && GET_ALLOC(HDRP(bp)) == 0; bp = (char *)GET_SUCC(bp)) {
			if (asize <= GET_SIZE(HDRP(bp))) {
				return bp;
			}
		}
	}
	return NULL;
}

static int get_free_list_idx(size_t size_of_blk) {
	int list_idx = 0;
	while ((list_idx < free_list_size - 1) && (size_of_blk > 1)) {
		size_of_blk >>= 1;
		list_idx++;
	}
	//assert(list_idx - 3 >= 0);
	return list_idx - 3;
}

static void delete_node(void *bp, size_t size_of_blk) {
	int list_idx = 0;

	list_idx = get_free_list_idx(size_of_blk);

	if ((void *) GET_PRED(bp) != NULL) {
		if ((void *) GET_SUCC(bp) != NULL) {
			// pred -> NODE -> succ
			PUT_SUCC(GET_PRED(bp), GET_SUCC(bp));
			PUT_PRED(GET_SUCC(bp), GET_PRED(bp));
		} else {
			// pred -> NODE -> NULL
			PUT_SUCC(GET_PRED(bp), NULL);
		}
	} else {
		if ((void *) GET_SUCC(bp) != NULL) {
			// NULL-> NODE -> succ
			//assert(GET_FREE_LIST_PTR(list_idx) == bp);
			PUT_PRED(GET_SUCC(bp), NULL);
			SET_FREE_LIST_PTR(list_idx, (char *)GET_SUCC(bp));
		} else {
			// NULL-> NODE -> NULL
			SET_FREE_LIST_PTR(list_idx, NULL);
		}
	}
}

static void insert_node(void *bp) {
	size_t size_of_blk = GET_SIZE(HDRP(bp));
	int list_idx = 0;

	list_idx = get_free_list_idx(size_of_blk);
	char *header_bp = GET_FREE_LIST_PTR(list_idx);

	if (header_bp != NULL) {
		PUT_PRED(header_bp, bp);
	}

	PUT_PRED(bp, NULL);
	PUT_SUCC(bp, header_bp);
	SET_FREE_LIST_PTR(list_idx, (char *) bp);

	/* asending list */
	//if (header_bp == NULL) {
	//	// empty
	//	PUT_PRED(bp, NULL);
	//	PUT_SUCC(bp, NULL);
	//	SET_FREE_LIST_PTR(list_idx, (char *) bp);
	//} else {
	//	char *pbp, *nbp;
	//	pbp = NULL;
	//	nbp = header_bp;
	//	while ((void *)nbp != NULL && GET_SIZE(HDRP(bp)) > GET_SIZE(HDRP(nbp))) {
	//		pbp = nbp;
	//		nbp = (char *)GET_SUCC(nbp);
	//	}
	//	if ((void *) pbp != NULL) {
	//		assert(!GET_ALLOC(HDRP(pbp)));
	//		PUT_SUCC(pbp, bp);
	//	}
	//	if ((void *) nbp != NULL) {
	//		assert(!GET_ALLOC(HDRP(nbp)));
	//		PUT_PRED(nbp, bp);
	//	}
	//	PUT_PRED(bp, pbp);
	//	PUT_SUCC(bp, nbp);

	//	if (pbp == NULL ) {
	//		SET_FREE_LIST_PTR(list_idx, (char *) bp);
	//	}
	//}

}

int mm_check(int verbose, char *msg) {

	if (verbose)
		printf("\n-------------\n%s\n", msg);

	int ret;

	baisc_block_check(verbose);

	// check free list
	ret = check_free_list_marked_free();
	if (ret < 0) {
		printf("mm_check: free list is not all marked free\n");
		return ret;
	}
	// NOT IMPLEMENTED YET!
	//ret = check_free_list_asending_order();
	//if (ret < 0) {
	//	return ret;
	//}

	//printf("\nmm_check PASSED!\n");
	return 0;
}
void baisc_block_check(int verbose) {
	char *bp = heap_listp;
	//verbose = 0;

	if (verbose)
		printf("Heap (%p):\n", heap_listp);

	if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
		printf("Bad prologue header\n");
	check_block(heap_listp);

	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose)
			printblock(bp," ");
		check_block(bp);
	}

	if (verbose)
		printblock(bp," ");
	if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
		printf("Bad epilogue header\n");
}

void check_block(char *bp) {
	if ((size_t)bp % 8)
		printf("Error: %p is not doubleword aligned\n", bp);
	if (GET_WORD(HDRP(bp)) != GET_WORD(FTRP(bp)))
		printf("Error: header does not match footer\n");
}

int check_free_list_asending_order() {
	char *bp;
	for (int i = 0; i < free_list_size; ++i) {
		if ((bp = GET_FREE_LIST_PTR(i)) != NULL) {
			printf("[[list idx %d, size %d]]\n", i, 2 << i);
			if (check_free_blocks_asending_order(bp) < 0) {
				app_error("check_free_list_asending_order:blocks in the free list is not all free\n");
				printf("check_free_list_asending_order:free list idx %d header: %p\n", i, bp);
				return -1;
			}
		}
	}
	return 0;
}



int check_free_blocks_asending_order(char *bp) {
	char *prev = bp;
	char *next = (char *)GET_SUCC(bp);
	while (next) {
		int prev_size = GET_SIZE(HDRP(prev));
		int next_size = GET_SIZE(HDRP(next));
		if (prev_size > next_size) {
			app_error("check_free_blocks_asending_order:blocks is not in asending order");
			return -1;
		}
		prev = next;
		next = (char *)GET_SUCC(bp);
	}
	return 0;
}


int check_free_blocks_marked_free(char *bp) {
	while (bp) {
		if (GET_ALLOC(bp) == ALLOCATED) {
			printblock(bp,"free block");
			//app_error("check_free_blocks_marked_free:blocks in free list is not all marked free\n");
			return -1;
		}
		printblock(bp, "free block");
		bp = (char *)GET_SUCC(bp);
	}
	return 0;
}


int check_free_list_marked_free() {
	char *bp;
	for (int i = 0; i < free_list_size; ++i) {
		if ((bp = GET_FREE_LIST_PTR(i)) != NULL) {
			printf("\n[[list idx %d, size %d]]\n", i, 2 << i);
			if (check_free_blocks_marked_free(bp) < 0) {
				app_error("check_free_list_marked_free:blocks in the free list is not all free\n");
				printf("check_free_list_marked_free: list idx %d header: %p\n", i, bp);
				return -1;
			}
		}
	}
	return 0;
}

static void printblock(void *bp, char *msg) {
	size_t h_size, h_flag, f_size, f_flag;
	h_size = GET_SIZE(HDRP(bp));
	h_flag = GET_ALLOC(HDRP(bp));
	f_size = GET_SIZE(FTRP(bp));
	f_flag = GET_ALLOC(FTRP(bp));

	if (strcmp(" ", msg) != 0){
		printf ("===%s==\n", msg);

	}
	if (h_size == 0) {
		printf("%p: not initialized region\n", bp);
		return;
	}
	if (h_flag) {
		// allocated block
		printf("+[%p: header: ( size: %u, alloc: %d ) footer: ( size: %u, alloc: %d )]+\n" ,
				bp, h_size, h_flag, f_size, f_flag);
	} else {
		// free block
		printf("-{%p: header: ( size: %u, alloc: %d ) footer: ( size: %u, alloc: %d )}-\n" ,
				bp, h_size, h_flag, f_size, f_flag);
		void *succ = (void *)GET_SUCC(bp);
		void *pred = (void *)GET_PRED(bp);
		printf("    { succ: %p, pred: %p }\n", succ, pred);

	}
}
