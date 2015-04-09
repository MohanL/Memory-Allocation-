/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
    "bteam",
    /* First member's full name */
    "Eugene H. Krabs",
    /* First member's email address */
    "krabs@cs.rochester.edu",
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

/* self defined macros */
/* meta data size */
#define MSIZE 32

/* true or false value */
#define TRUE 1
#define FALSE 0

size_t TUAB; /*  Total used and allocated bytes */
size_t TFAB; /*  Total free but allocated bytes */
size_t TAB;  /*  Total allocated bytes */

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    TUAB = 0; 
    TFAB = 0;
    TAB = mem_heapsize();
    printf("hi, we're init'd\n"); 
    return 0;

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1) {
	    return NULL; } else { *(size_t *)p = size; return (void *)((char *)p + SIZE_T_SIZE); } } /* * mm_free - Freeing a block does nothing.  */
void mm_free(void *ptr) { }

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL) {
      return NULL;
    }

    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize) {
      copySize = size;
    }

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}

/* self-defined helper function */
/* find a free block */
void *looking_for_free_block(size_t size) {

     int size_looking_for = ALIGN(size);
     char *current =(char *) mem_heap_lo();/* getting the first meta data */
     if((TAB = mem_heapsize()) == 0)
     {
          printf("srsly, someting is wrong\n");
          return (void *)(-1);
     }
     while((current != NULL)&&(current < mem_heap_hi()))
     {
          /* check if meta data is free or taken */
          if((current + 24) == (int)0) /* case that the block is free*/
          {
               if((size_t)(current+16)>= size_looking_for){
                    printf("%s <= %s\n",size_looking_for,(size_t)(current+1)6);
                    return (void *)(current+MSIZE);
                 }
          }
            /* we jump to the next memory block if it has a valid next  */
               current = (char)( *current);  /* mohan's version of code Q */
               current = (char *)(current + (size_t)(current+16)+ MSIZE); /* Evan's idea */
      }          
      return (void *)(-1);

/*  original malloc function's code, used as reference
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1) {
	    return NULL;
    } else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
*/
}
