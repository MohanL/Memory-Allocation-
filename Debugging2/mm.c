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
    "K",
    /* First member's email address */
    "k@gmail.com",
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
#define OVERHEAD 8 //extra bytes for header and footer

#define MAX(x, y) ((x)>(y)?(x):(y))

//pack a size and allocated bit into a word
#define PACK(size, alloc) ((size)|(alloc))

//read and write a word at address p
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

//read the size and allocated fields from address p
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

//given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE)

//given block ptr bp, compute address of next and prev
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))


//given free block ptr bp, compute address of next and prev free blocks
#define NEXT_FREE_BLKP(bp) (*(void **)(bp+WSIZE))
#define PREV_FREE_BLKP(bp) (*(void **)(bp))
void *NEXT_FREE_BLKP(void * bp)
{
     if (bp == NULL)
     {
          printf("
     }
}


#define MIN_BLOCK_SIZE CHUNKSIZE/WSIZE

//initialize the pointer to the first block of the heap
static char *heap;
static char *free_ptr;

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
     printf("\n\nckpt1: init() called\n");
    //create the initial empty heap; return -1 if heap space unavailable
    if((heap = mem_sbrk(8*WSIZE)) == NULL) return -1;

    PUT(heap, 0); //alignment padding
    printf("ckpt2 - init()\n");
    PUT(heap + (1*WSIZE), PACK(DSIZE, 1)); //prologue header
    PUT(heap + (2*WSIZE), PACK(DSIZE, 1)); //prologue footer
    PUT(heap + (3*WSIZE), PACK(0, 1)); //epilogue header
    heap += (2*WSIZE);
    free_ptr = heap;

    printf("ckpt3 - init()\n");
    //extend the empty heap with a free block of CHUNKSIZE bytes
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
     {
           printf("ckpt3.1 Initalization - init()\n"); 
           return -1;
     }
    printf("ckpt5: EXIT init()\n");
    return 0;
}

/*
 * mm_malloc - Allocate a block from the free list
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    
    printf("\nckpt9.1.1 - START mm_malloc()\n");
    size_t newsize; //size of block to mm_malloc
    void *bp;

    //error checking
    if(size<=0) return NULL;

    //adjust block size to account for overhead and alignment
    if(size<=DSIZE) newsize = 2*DSIZE;
    else newsize = DSIZE * ((size+DSIZE+(DSIZE-1))/DSIZE);

    //try to find a suitable block in the free list
    printf("ckpt9.1.2 - mm_malloc()\n");
    if((bp = find_fit(newsize))!=NULL){
        //place the block

        printf("ckpt9.1.2.1 - mm_malloc()\n");
        place(bp, newsize);
        printf("ckpt9.1.2.2 - mm_malloc()\n");
        return bp;
    }

    //if a suitable block does not exist, then expand the heap
    printf("ckpt9.1.3 - mm_malloc()\n");
    size_t extension = MAX(newsize, CHUNKSIZE);

    //get more memory
    //error handling; unable to allocate more space to heap; return NULL
    if((bp = extend_heap(extension/WSIZE))==NULL) return NULL;

    printf("ckpt9.1.4 - mm_malloc()\n");
    //place the block
    place(bp, newsize);

    printf("ckpt9.1.5 - EXIT mm_malloc()\n");
    return bp;
}

/*
 * mm_free - frees a block and uses boundary-tag coalescing to merge
 * it with any adjacent free blocks in constant time
 */
void mm_free(void *ptr) {
    //error checking
    if(ptr==NULL) return;

    size_t size = GET_SIZE(HDRP(ptr));

    //set allocation status to free for header and footer of the block to be freed
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    //coalesce the new freed block with surrounding free blocks if possible
    coalesce(ptr);
}

/*
 * mm_realloc
 * takes in block pointer and new size
 * changes given block to new size
 * if remainder of block is at least minimum block size, make a new free block
 * returns pointer to newly allocated block
 */
void *mm_realloc(void *ptr, size_t size) {
    void *newptr;
    size_t newsize;
    size_t oldsize;

    //account for alignment
    if(size <= DSIZE) newsize = 2*DSIZE;
    else newsize = DSIZE*((size+DSIZE+(DSIZE-1))/DSIZE);

    //case 1: the size is less than or equal to zero
    //same as free
    if(size<=0){
        mm_free(ptr);
        return NULL;
    }

    //case 2: the old pointer is null
    //same as mm_malloc
    if(ptr==NULL) return mm_malloc(size);

    //get the size of the block passed in
    oldsize = GET_SIZE(HDRP(ptr));

    //case 3: the size is the same as the size of the block passed in
    //return the original block pointer
    if(newsize == oldsize) return ptr;

    //case 4: the size is smaller than the size of the block passed in
    //shrink the block and return the pointer
    if(newsize <= oldsize){
        //case 1: the remainder after shrinking the block is too small to create a new block
        if((oldsize - newsize) <= MIN_BLOCK_SIZE) return ptr;

        //case 2: the remainder after shrinking the block is large enough to create a new block

        //allocate the new block
        PUT(HDRP(ptr), PACK(newsize, 1));
        PUT(FTRP(ptr), PACK(newsize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK((oldsize-newsize), 1));

        //free the remainder
        mm_free(NEXT_BLKP(ptr));

        return ptr;
    }

    //error checking; if unable to mm_realloc(), then exit without doing anything
    if(!(newptr = mm_malloc(size))) return 0;

    //copy the data over from the old block to the new
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    //free the old block
    mm_free(ptr);

    return newptr;
}

//extends the heap with a new free block
static void *extend_heap(size_t words) {

    printf("ckpt4: extend_heap() called\n");
    char *bp;
    size_t size;
    
    //allocate an even number of words to maintain alignment
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;
    printf("ckpt4.1 - extend_heap()\n");
    //initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size, 0)); //free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //new epilogue header
    printf("ckpt4.2 - extend_heap()\n");
    //coalesce if the previous block was free
    printf("ckpt4.3 EXIT extend_heap()\n");
    return coalesce(bp);
}

//boundary tag coalescing
//takes in pointer to a newly freed block
//joins adjacent free blocks together
//updates the free list
//returns pointer to the newly created coalesced free block
static void *coalesce(void *bp) {

    printf("ckpt6: coalesce() called\n");
    //get the allocation status of the previous and next blocks
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    //get the size of the current block
    size_t size = GET_SIZE(HDRP(bp));

    //case 1: next and free blocks both allocated; no coalescing possible
    if(prev_alloc && next_alloc){
        printf("ckpt6.1.1: Start case 1 - coalesce()\n");
        //add a block to the free list
        add_free_block(bp);
        printf("ckpt6.1.2: End case 1 - coalesce()\n");
        return bp;
    }

    //case 2: next block is free but previous is not
    else if(prev_alloc && !next_alloc){
        printf("ckpt6.2.1 Start case 2 - coalesce()\n");
        //delete next block from free list
        delete_free_block(NEXT_BLKP(bp));

        //extend the size of the given block by the size of the next block
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        //set allocation status to free for header and footer of the coalesced block
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        //add the extended block to the free list
        add_free_block(bp);
        printf("ckpt6.2.2: End case 2 - coalesce()\n");
    }

    //case 3: previous block is free but next is not
    else if(!prev_alloc && next_alloc){
        printf("ckpt6.3.1: Start case 3 - coalesce()\n");
        //delete previous block from free list
        delete_free_block(PREV_BLKP(bp));
        
        //extend the size of the given block by the size of the previous block
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     
        printf("ckpt6.3.1.1 -  coalesce()\n");
        //set allocation status to free for header and footer of the coalesced block
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        //set pointer of the coalesced block to pointer to previous block
        bp = PREV_BLKP(bp);

        printf("ckpt6.3.1.2 -  coalesce()\n");
        //add the coalesced block to the free list
        add_free_block(bp);

        printf("ckpt6.3.2: End case 3 - coalesce()\n");
    }

    //case 4: both the previous and next blocks are free
    else{
        //delete the next and previous blocks from the free list
        printf("ckpt6.4.1: Start case 4 - coalesce()\n");
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
        printf("ckpt6.4.2: End case 4 - coalesce()\n");
    }
    printf("ckpt6.5 EXIT coalesce()\n");
    return bp;
}

//add a block to the front of the free list
//parameter: block to be added to the free list
static void add_free_block(void *bp) {
    printf("ckpt8.1.1 - START add_free_block()\n");
    //set the next of the new free block to point at the start of the current free list
    NEXT_FREE_BLKP(bp) = free_ptr;

    printf("ckpt8.1.2 -  add_free_block()\n");
    //set the previous of the current free list to point at the new free block
    //ERROR 2? check if free is NULL
    if(free_ptr != NULL)
    {
         PREV_FREE_BLKP(free_ptr) = bp;
         printf("ckpt8.1.3 -add_free_block()\n");
    }
    //set the previous of the new free block to NULL (it is the first element, so has no previous)
    PREV_FREE_BLKP(bp) = NULL;
    printf("ckpt8.1.4 -  add_free_block()\n");
    //set the start of the free list to point to the new free block
    free_ptr = bp;
}

//delete a block from the free list
//parameter: block to be removed from the free list
static void delete_free_block(void *bp) {
    printf("ckpt7.1: delete_free_block() \n");
    //case 1: the block is not the head of the free list
    if(PREV_FREE_BLKP(bp)){
        printf("ckpt7.1.1: case 1 -  delete_free_block() \n");
        //set the next pointer of the previous block to point to the next block
        NEXT_FREE_BLKP(PREV_FREE_BLKP(bp)) = NEXT_FREE_BLKP(bp);

        //set the previous pointer of the next block to point at the previous block
        PREV_FREE_BLKP(NEXT_FREE_BLKP(bp)) = PREV_FREE_BLKP(bp);
    }

    //case 2: the block is the head of the free list
    else{
            printf("ckpt7.1.2: case 2 - delete_free_block() \n");
            //set the start of the free list to point at the next free block
            free_ptr = NEXT_FREE_BLKP(bp);
            printf("ckpt7.1.2.1 - delete_free_block() \n");
      
            //ERROR1. Make sure there is a valid next block
            //set the previous pointer of the next block to NULL
            if(free_ptr!=NULL)
            { 
               PREV_FREE_BLKP(NEXT_FREE_BLKP(bp)) = NULL;
               printf("ckpt7.1.2.2 - delete_free_block() \n");
            }
            printf("ckpt7.1.2.3 - delete_free_block() \n");
    }

    printf("ckpt7.2: EXIT delete_free_block() \n");
}

//find a free block of a certain size
static void *find_fit(size_t asize) {
    void *bp;

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
    
    printf("ckpt10.1.1 - START place()\n");
    size_t oldsize = GET_SIZE(HDRP(bp));

    //case 1: the remainder is large enough to create a new free block
    if((oldsize-asize) >= 2*DSIZE){
        printf("ckpt10.1.1.1 - START case 1 - place()\n");
        //set allocation status of given block to not free
        
        //ERROR3 ? Cause segmetation error 
        if(bp == NULL)
        {
             printf("ckpt10.1.1.2.0 - place()\n");
        }
        if(HDRP(bp)!=NULL)
        {

             printf("ckpt10.1.1.2.1.0 - place()\n");
             // ERROR3 details
             size_t val = PACK(asize,1);
             PUT(HDRP(bp), val);
             printf("ckpt10.1.1.2.1.1 - place()\n");
        }
        else
        {
             printf("ckpt10.1.1.2.2 - ERROR - place()\n");
        }
        if(FTRP(bp)!=NULL)
        {
             printf("ckpt10.1.1.3.1.0 - place()\n");
             PUT(FTRP(bp), PACK(asize, 1));
             printf("ckpt10.1.1.3.1.1 - place()\n");
        }
        else
        {
             printf("ckpt10.1.1.3.2 - ERROR - place()\n");
        }
        printf("ckpt10.1.1.4 - place()\n");
        //delete the allocated block from the free list
        delete_free_block(bp);

        //create a new free block starting right after the newly allocated block ends
        bp = NEXT_BLKP(bp);

        printf("ckpt10.1.1.5 - place()\n");
        //set the allocation status of the new free block to free
        PUT(HDRP(bp), PACK(oldsize-asize, 0));
        PUT(FTRP(bp), PACK(oldsize-asize, 0));

        //add the new free block to the free list
        add_free_block(bp);
        printf("ckpt10.1.1.6 - END case 1 - place()\n");
    }

    //case 2: the remainder is too small to create a new free block
    else{
        printf("ckpt10.1.2.1 - START case 2 - place()\n");
        //set the allocation status of the newly allocated block to not free
        PUT(HDRP(bp), PACK(oldsize, 1));
        PUT(FTRP(bp), PACK(oldsize, 1));

        //delete the newly allocated block from the free list
        delete_free_block(bp);
        printf("ckpt10.1.2.2 - END case 2 - place()\n");
      }
}
