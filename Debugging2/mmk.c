/*
 * Sources:
 * https://github.com/manandhawan/Allocator/blob/master/mm.c
 * https://github.com/ltganesan/mm_malloc/blob/master/mm.c
 * https://github.com/SRP2504/malloc_lab/blob/master/mm.c
 * https://github.com/wuyanna/mm-mm_malloc/blob/master/mm.c
 * Computer Systems: A Programmer's Perspective 2nd Edition
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "something random",
    /* First member's full name */
    "Katarzyna Nicole Ruszowska",
    /* First member's email address */
    "kruszows@u.rochester.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* 16 byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT) -1) & ~(ALIGNMENT- 1))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//basic constants and macros
#define WSIZE sizeof(void *) //word and header/footer size (bytes)
#define DSIZE (2*WSIZE) //double word size (bytes)
#define CHUNKSIZE (1<<12) //extend heap by this amount (bytes)

#define MAX(x, y) ((x)>(y)?(x):(y))

//pack a size and allocated bit into a word
#define PACK(size, alloc) ((size)|(alloc))

//read and write a word at address p
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

//read the size and allocated fields from address p
#define GET_SIZE(p) (GET(p) & ~(DSIZE-1))
#define GET_ALLOC(p) (GET(p) & 0x1)

//given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((void *)(bp)-WSIZE)
#define FTRP(bp) ((void *)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

//given block ptr bp, compute address of next and prev
#define NEXT_BLKP(bp) ((void *)(bp)+GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((void *)(bp)-GET_SIZE(HDRP(bp)-WSIZE))

//given free block ptr bp, compute address of next and prev free blocks
#define NEXT_FREE_BLKP(bp) (*(void **)(bp+WSIZE))
#define PREV_FREE_BLKP(bp) (*(void **)(bp))

#define MIN_BLOCK_SIZE CHUNKSIZE/WSIZE

//initialize the pointer to the first block of the heap
static char *heap = NULL;
static char *free_ptr = NULL;

//function prototypes
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void delete_free_block(void *bp);
static void add_free_block(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/*
 * mm_init - initialize the mm_malloc package.
 * creates a heap with an initial free block
 */
int mm_init(void) {
    printf("init()\tSTART\n");
    //create the initial empty heap; return -1 if heap space unavailable
    if((heap = mem_sbrk(4*WSIZE)) == (void *)-1) return -1;

    PUT(heap, 0); //alignment padding
    PUT(heap + (1*WSIZE), PACK(DSIZE, 1)); //prologue header
    PUT(heap + (2*WSIZE), PACK(DSIZE, 1)); //prologue footer
    PUT(heap + (3*WSIZE), PACK(0, 1)); //epilogue header
    heap += (2*WSIZE);
    free_ptr = heap;

    //extend the empty heap with a free block of CHUNKSIZE bytes
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL) return -1;
    printf("init()\tEND\n");
    return 0;
}

/*
 * mm_malloc - Allocate a block from the free list
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    printf("\nmm_malloc()\tSTART\n");
    size_t newsize; //size of block to mm_malloc
    void *bp = NULL;

    //error checking
    if(size<=0) return NULL;

    //adjust block size to account for overhead and alignment
    if(size<=DSIZE) newsize = 2*DSIZE;
    else newsize = DSIZE * ((size+DSIZE+(DSIZE-1))/DSIZE);

    //try to find a suitable block in the free list
    if((bp = find_fit(newsize))!=NULL){
        //place the block
        printf("mm_malloc()\tBEFORE PLACE\n");
        place(bp, newsize);
        printf("mm_malloc()\tAFTER PLACE\n");
        return bp;
    }

    //if a suitable block does not exist, then expand the heap
    size_t extension = MAX(newsize, CHUNKSIZE);

    //get more memory
    //error handling; unable to allocate more space to heap; return NULL
    if((bp = extend_heap(extension/WSIZE))==NULL) return NULL;

    //place the block
    place(bp, newsize);

    printf("mm_malloc()\tEND\n");
    return bp;
}

/*
 * mm_free - frees a block and uses boundary-tag coalescing to merge
 * it with any adjacent free blocks in constant time
 */
void mm_free(void *ptr) {
    printf("mm_free\tSTART");
    //error checking
    if(ptr==NULL) return;

    size_t size = GET_SIZE(HDRP(ptr));

    //set allocation status to free for header and footer of the block to be freed
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    //coalesce the new freed block with surrounding free blocks if possible
    coalesce(ptr);
    printf("mm_free\tEND");
}

/*
 * mm_realloc
 * takes in block pointer and new size
 * changes given block to new size
 * if remainder of block is at least minimum block size, make a new free block
 * returns pointer to newly allocated block
 */
void *mm_realloc(void *ptr, size_t size) {
    printf("mm_realloc\tSTART");
    void *newptr = NULL;
    size_t newsize;
    size_t oldsize;

    //account for alignment
    if(size <= DSIZE) newsize = 2*DSIZE;
    else newsize = DSIZE*((size+DSIZE+(DSIZE-1))/DSIZE);

    //case 1: the size is less than or equal to zero
    //same as free
    if(size<=0){
        printf("mm_realloc\tSTART\t1");
        mm_free(ptr);
        return NULL;
        printf("mm_realloc\tEND\t1");
    }

    //case 2: the old pointer is null
    //same as mm_malloc
    if(ptr==NULL){
        printf("mm_realloc\tSTART\t2");
        return mm_malloc(size);
        printf("mm_realloc\tEND\t2");
    }

    //get the size of the block passed in
    oldsize = GET_SIZE(HDRP(ptr));

    //case 3: the size is the same as the size of the block passed in
    //return the original block pointer
    if(newsize == oldsize){
        printf("mm_realloc\tSTART\t3");
        return ptr;
        printf("mm_realloc\tEND\t3");
    }

    //case 4: the size is smaller than the size of the block passed in
    //shrink the block and return the pointer
    if(newsize <= oldsize){
        printf("mm_realloc\tSTART\t4");
        //case 1: the remainder after shrinking the block is too small to create a new block
        if((oldsize - newsize) <= MIN_BLOCK_SIZE) return ptr;

        //case 2: the remainder after shrinking the block is large enough to create a new block

        //allocate the new block
        PUT(HDRP(ptr), PACK(newsize, 1));
        PUT(FTRP(ptr), PACK(newsize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK((oldsize-newsize), 1));

        //free the remainder
        mm_free(NEXT_BLKP(ptr));
        printf("mm_realloc\tEND\t4");
        return ptr;
    }

    //error checking; if unable to mm_realloc(), then exit without doing anything
    if(!(newptr = mm_malloc(size))) return 0;

    //copy the data over from the old block to the new
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    //free the old block
    mm_free(ptr);
    printf("mm_realloc\tEND");
    return newptr;
}

//extends the heap with a new free block
static void *extend_heap(size_t words) {
    printf("extend_heap()\tSTART\n");
    void *bp = NULL;
    size_t size;

    //account for alignment
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    //error checking; if adding more space fails, return NULL
    if ((bp = mem_sbrk(size)) == (void *) -1) return NULL;

    //set allocation status to free for new block header and footer
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    //try to coalesce new free block with surrounding blocks
    printf("extend_heap()\tEND\n");
    return coalesce(bp);
}

//boundary tag coalescing
//takes in pointer to a newly freed block
//joins adjacent free blocks together
//updates the free list
//returns pointer to the newly created coalesced free block
static void *coalesce(void *bp) {
    printf("coalesce()\tSTART\n");
    //get the allocation status of the previous and next blocks
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    //get the size of the current block
    size_t size = GET_SIZE(HDRP(bp));

    //case 1: next and free blocks both allocated; no coalescing possible
    if(prev_alloc && next_alloc){
        printf("coalesce()\tSTART\t1\n");
        //add a block to the free list
        add_free_block(bp);
        printf("coalesce()\tEND\t1\n");
        return bp;
    }

    //case 2: next block is free but previous is not
    else if(prev_alloc && !next_alloc){
        printf("coalesce()\tSTART\t2\n");
        //delete next block from free list
        delete_free_block(NEXT_BLKP(bp));

        //extend the size of the given block by the size of the next block
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        //set allocation status to free for header and footer of the coalesced block
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        //add the extended block to the free list
        add_free_block(bp);
        printf("coalesce()\tEND\t2\n");
    }

    //case 3: previous block is free but next is not
    else if(!prev_alloc && next_alloc){
        printf("coalesce()\tSTART\t3\n");
        //delete previous block from free list
        delete_free_block(PREV_BLKP(bp));

        //extend the size of the given block by the size of the previous block
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        //set allocation status to free for header and footer of the coalesced block
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        //set pointer of the coalesced block to pointer to previous block
        bp = PREV_BLKP(bp);

        //add the coalesced block to the free list
        add_free_block(bp);

        printf("coalesce()\tEND\t3\n");
    }

    //case 4: both the previous and next blocks are free
    else{
        //delete the next and previous blocks from the free list
        printf("coalesce()\tSTART\t4\n");
        delete_free_block(PREV_BLKP(bp));
        delete_free_block(NEXT_BLKP(bp));

        //extend the size of the previous by the sizes of the current and next blocks
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        //set allocation status to free for header and footer of the coalesced block
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        //set pointer of the coalesced block to pointer to previous block
        bp = PREV_BLKP(bp);

        //add the coalesced block to the free list
        add_free_block(bp);
        printf("coalesce()\tEND\t4\n");
    }
    printf("coalesce()\tEND\n");
    return bp;
}

//add a block to the front of the free list
//parameter: block to be added to the free list
static void add_free_block(void *bp) {
    printf("add_free_block()\tSTART\n");
    //set the next of the new free block to point at the start of the current free list
    NEXT_FREE_BLKP(bp) = free_ptr;
    printf("add_free_block()\tnext of bp set to free_ptr\n");
    //set the previous of the current free list to point at the new free block
    //ERROR 2? check if free is NULL
    if(free_ptr != NULL)
    {
        printf("add_free_block()\tfree_ptr IS NOT NULL!!!\n");
        PREV_FREE_BLKP(free_ptr) = bp;
    }
    //set the previous of the new free block to NULL (it is the first element, so has no previous)
    PREV_FREE_BLKP(bp) = NULL;
    printf("add_free_block()\tprev of bp set to NULL\n");

    //set the start of the free list to point to the new free block
    free_ptr = bp;
    printf("add_free_block()\tEND\n");
}

//delete a block from the free list
//parameter: block to be removed from the free list
static void delete_free_block(void *bp) {
    printf("delete_free_block()\tSTART\n");
    //case 1: the block is not the head of the free list
    if(PREV_FREE_BLKP(bp)){
        printf("delete_free_block()\tSTART\t1\n");
        //set the next pointer of the previous block to point to the next block
        NEXT_FREE_BLKP(PREV_FREE_BLKP(bp)) = NEXT_FREE_BLKP(bp);

        //set the previous pointer of the next block to point at the previous block
        PREV_FREE_BLKP(NEXT_FREE_BLKP(bp)) = PREV_FREE_BLKP(bp);
        printf("delete_free_block()\tEND\t1");
    }

    //case 2: the block is the head of the free list
    else{
            printf("delete_free_block()\tSTART\t2\n");
            //set the start of the free list to point at the next free block
            free_ptr = NEXT_FREE_BLKP(bp);

            //set the previous pointer of the next block to NULL
            if(free_ptr!=NULL) PREV_FREE_BLKP(NEXT_FREE_BLKP(bp)) = NULL;
            printf("delete_free_block()\tEND\t2\n");
    }
    printf("delete_free_block()\tEND\n");
}

//find a free block of a certain size
static void *find_fit(size_t asize) {
    void *bp = NULL;

    //traverse the free list until a large enough block is found
    //return block if found; else return NULL
    for(bp = free; GET_ALLOC(HDRP(bp))==0; bp = NEXT_FREE_BLKP(bp)){
        if(GET_SIZE(HDRP(bp)) >= asize) return bp;
    }
    return NULL;
}

//allocate a block newsize bytes from free block pointed to by bp
//create a new free block if the remainder of the old free block is large enough
static void place(void *bp, size_t asize){
    printf("place()\tSTART\n");
    size_t oldsize = GET_SIZE(HDRP(bp));
    printf("oldsize:\t%zd\n",oldsize);
    printf("asize:\t%zd\n",asize);
    printf("oldsize-asize:\t%zd\n",(oldsize-asize));

    //case 1: the remainder is large enough to create a new free block
    if((oldsize-asize) >= MIN_BLOCK_SIZE){
        printf("place()\tSTART\t1\n");
        //set allocation status of given block to not free
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        //delete the allocated block from the free list
        delete_free_block(bp);

        //create a new free block starting right after the newly allocated block ends
        bp = NEXT_BLKP(bp);

        //set the allocation status of the new free block to free
        PUT(HDRP(bp), PACK(oldsize-asize, 0));
        PUT(FTRP(bp), PACK(oldsize-asize, 0));

        //add the new free block to the free list
        coalesce(bp);
        printf("place()\tEND\t1\n");
    }

    //case 2: the remainder is too small to create a new free block
    else{
        printf("place()\tSTART\t2\n");
        //set the allocation status of the newly allocated block to not free
        PUT(HDRP(bp), PACK(oldsize, 1));
        PUT(FTRP(bp), PACK(oldsize, 1));

        //delete the newly allocated block from the free list
        delete_free_block(bp);
        printf("place()\tEND\t2\n");
    }
    printf("place()\tEND");
}
