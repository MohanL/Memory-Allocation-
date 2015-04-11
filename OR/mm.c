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

/* global variables */
size_t TUAB; /* total allocated and used bytes */ 
size_t TFAB; /* total allcaoted but free bytes */
size_t TAB;  /* total allocated bytes */
char * PREV; /* previous meta data pointer */

/* self-defined macros and functions */
/* define meta data size */
#define CSIZE sizeof(char *)
#define SSIZE sizeof(size_t)
#define ISIZE sizeof(int)
#define RMSIZE (CSIZE+CSIZE+SSIZE+ISIZE)
#define MSIZE ALIGN(RMSIZE)

/* getters and setters for the meta data*/
char * metaNext(char * metaData)
{
    return *(char* *)metaData;
}
void metaSetNext(char * metaData, char * value)
{
     if(strcmp(metaData,"NULL")!=0)
           *(char* *)metaData = value;
     else
          printf("set next error, metaData is NULL\n");
}

char * metaPrev(char * metaData)
{
    char * addr = (char *)((int)metaData + CSIZE);
    return *(char* *)addr;
}
void metaSetPrev(char * metaData, char * value)
{
      
     if(strcmp(metaData,"NULL")!=0)
     { 
           char * addr = (char *)((int)metaData + CSIZE);
          *(char* *)addr = value;
     }
     else
          printf("set prev error, metaData is NULL\n");
}

size_t metaSize(char * metaData)
{
    char * addr = (char *)((int)metaData + CSIZE+CSIZE);
    return *(size_t *)addr;
}
void metaSetSize(char * metaData, size_t value)
{
     if(strcmp(metaData,"NULL")!=0)
     {
           char * addr = (char *)((int)metaData + CSIZE+CSIZE);
          *(size_t *)addr = value;
     }
     else
          printf("set size error, metaData is NULL\n");
}

int metaStatus(char * metaData)
{
    char * addr = (char *)((int)metaData + CSIZE+CSIZE+SSIZE);
    return *(int *)addr;
}
void metaSetStatus(char * metaData, int value)
{
     if(strcmp(metaData,"NULL")!=0)
     {
          char * addr = (char *)((int)metaData + CSIZE+CSIZE+SSIZE);
          *(int *)addr = value;
     }
     else
          printf("set status error, metaData is NULL\n");
}
/* return the start address of a memory block */
char *metaBlockStart(char * metaData)
{
     char * addr = (char *)((int)metaData +MSIZE);
     return addr;
}
/* return the meta data's address if given a block's address */
char *blockMetaStart(char * block)
{
     char * addr = (char *)((int)block -MSIZE);
     if( addr >= (char *)mem_heap_lo())
          return addr;
     else
          return (char *)(-1);
}

/* helper functions lv2 */
/* size is already aligned */
/* return style 1. pointer points to a metadata 2. -1*/
char * find_free_block(size_t size)
{
     char * current = mem_heap_lo();
     if(mem_heapsize() == 0)
          return (char *)(-1);
     else
     {
          while(current < (char *)mem_heap_hi())
          {
               if((metaStatus(current) == 0) && (metaSize(current) >= size))
                    return current; 
               else
               {
                    if(strcmp(current,"NULL")!=0)     
                         current = metaNext(current);
                    else
                         return (char *)(-1);
               }
          }
     }
}
/* split a free block(size_t, char * metadata) */
char * split(size_t size, char * metaData)
{
     size_t oldsize = metaSize(metaData);
     metaSetSize(metaData,size);
     metaSetStatus(metaData,1);

     char * new = (char *)((int)metaBlockStart(metaData)+size);
     metaSetNext(new, metaNext(metaData));
     metaSetPrev(new,metaData);

     metaSetNext(metaData,new);
     metaSetSize(new,oldsize-size-MSIZE);
     metaSetStatus(new,0);

     /* update global variables */
     TUAB += size+MSIZE;
     TFAB = TFAB-size-MSIZE;
     
     return metaBlockStart(metaData);
}

void leftFusion(char * left, char * current)
{
     metaSetSize(left, metaSize(left)+MSIZE+metaSize(current));
     metaSetNext(left, metaNext(current));
     /* update global variables */
     TUAB = TUAB -MSIZE -metaSize(current);
     TFAB = TFAB +MSIZE +metaSize(current);
}
void rightFusion(char *right, char * current)
{
     metaSetSize(current,metaSize(right)+MSIZE+metaSize(current));
     metaSetNext(current,metaNext(right));
     /* update global variables */
     TUAB = TUAB -MSIZE -metaSize(right);
     TFAB = TFAB +MSIZE +metaSize(right);
}

void doubleFusion(char * left, char * current, char * right)
{
     metaSetSize(left, metaSize(left)+metaSize(right)+2*MSIZE+metaSize(current));
     metaSetNext(left, metaNext(right));
     metaSetPrev(metaNext(right),left);
     /* update global variables */
     TUAB = TUAB -2*MSIZE -metaSize(current)-metaSize(right);
     TFAB = TFAB +2*MSIZE +metaSize(current)+metaSize(right);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    TUAB = 0;
    TFAB = 0;
    TAB = mem_heapsize();
    PREV = "NULL";
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
     printf("hello, I am in malloc\n");
     size_t asize = ALIGN(size);
     char * addr = find_free_block(asize);

     if(strcmp(addr,"-1") == 0)
     {
         /* allocate a new memory block at the end of the heap */
         int newsize = (int)asize + MSIZE;  
         void *p = mem_sbrk(newsize);
         if (p == (void *)-1) {
	          return NULL;
         } 
         else
         {
               /* initialize new metadata */
               char * metaData = (char*)p;
               metaSetNext(metaData,"NULL");
               metaSetPrev(metaData,PREV);
               metaSetSize(metaData,asize);
               metaSetStatus(metaData,1);
               
               /*update global variables */
               TUAB += newsize;
               TAB = mem_heapsize();
               PREV = metaData;
               return (void *)metaBlockStart(metaData);
         }
     }
     else
     {
          char * ret = split(asize, addr);
          return (void *)ret;
     }
/* original code */
/*    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1) {
	    return NULL;
    } else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
*/
}

/*
 * mm_free - Freeing a block does nothing.
 */
/* feed in the start address of a block */
void mm_free(void *ptr)
{
          char * metaData = blockMetaStart((char*)ptr);
          if(strcmp(metaData,"-1") == 0)
               return NULL;
          else
          {
               metaSetStatus(metaData,0);
               char * prev = metaPrev(metaData);
               char * next = metaNext(metaData);
               /* left block is free and right block is taken */
               /* left fusion case 1: left block is free and right block is null*/
               if( (strcmp(prev,"NULL")!=0)&&(strcmp(next,"NULL")==0)&&(metaStatus(prev)==0) )
               {
                    leftFusion(prev,metaData);          
               }
               else if( (strcmp(prev,"NULL")!=0)&&(strcmp(next,"NULL")!=0)&&(metaStatus(prev)==0)&&(metaStatus(next)==1) )
               {
                    leftFusion(prev,metaData);
                    metaSetPrev(next,prev);
               }
               else if( (strcmp(next,"NULL")!=0)&&(strcmp(prev,"NULL")==0)&&(metaStatus(next)==0) )
               {
                    rightFusion(next,metaData);
               }
               else if( (strcmp(next,"NULL")!=0)&&(strcmp(prev,"NULL")!=0)&&(metaStatus(next)==0)&&(metaStatus(prev)==1) )
               {
                    rightFusion(next,metaData);
                    char * next_next = metaNext(next);
                    if(strcmp(next_next,"NULL")!=0)
                         metaSetPrev(next_next,metaData);
               }
               /* two neightbours fusion */
               else if( (strcmp(next,"NULL")!=0)&&(strcmp(prev,"NULL")!=0)&&(metaStatus(next)==0)&&(metaStatus(prev)==0) )
               {
                    doubleFusion(prev,metaData,next);                    
               }
          }
          /*coalition process */
          
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
     
     size_t real_size = ALIGN(size);
     char * metaData = blockMetaStart((char*) ptr);
     size_t msize = metaSize(metaData);

     /* we don't need to move it around */
     if(real_size < msize)
     {
          /* insert new metadata */
          if( msize - real_size > MSIZE)
          {
               char * new =(char *)((int)metaBlockStart(metaData) + real_size);
               metaSetNext(new,metaNext(metaData));
               metaSetNext(metaData,new);
               metaSetPrev(new,metaData);
               metaSetPrev(metaNext(metaData),new);
               metaSetSize(new,metaSize(metaData)-real_size-MSIZE);
               metaSetSize(metaData,real_size);
               metaSetStatus(new,0);
           }
               return ptr;
          
     }
     else
     {    /* we could move it around */
     
          void * newptr = mm_malloc(real_size);
          if(newptr ==NULL)
          {
               return NULL;
          }
          memcpy(newptr,ptr,size);
          mm_free(ptr);
          return newptr;
     }
}
/* original code for mm_realloc*/
/*
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
*/
