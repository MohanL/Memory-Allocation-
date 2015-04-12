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
/* comment section */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
int64_t TUAB; /* total allocated and used bytes */
int64_t TFAB; /* total allcaoted but free bytes */
int64_t TAB;  /* total allocated bytes */
char * PREV; /* the last meta data pointer */


/* self-defined macros and functions */
/* define meta data size */
#define CSIZE sizeof(char *)
#define SSIZE sizeof(size_t)
#define BSIZE sizeof(int64_t)
#define RMSIZE (CSIZE+CSIZE+SSIZE+BSIZE)
#define MSIZE ALIGN(RMSIZE)
#define NVALUE (mem_heap_lo()-1)

/* helper function lv1 */
/* getters and setters for the meta data*/
char * metaNext(char * metaData)
{
    return *(char* *)metaData;
}
void metaSetNext(char * metaData, char * value)
{

	if(metaData == value){
		//printf("ckpt28: ERROR :metadata next points to itself\n");\
	     return NULL;
     }
     /* this case is about this is now prev for the value */
     if(metaData != NVALUE)
    	 *(char* *)metaData = value;
     else
          //printf("set next error, metaData is NULL\n");
     if((value!=NVALUE)&&(PREV==metaData))
          PREV = value;
}

char * metaPrev(char * metaData)
{
    char * addr = (char *)((int)metaData + CSIZE);
    return *(char* *)addr;
}
/* NOTICE: any case we need alter global variable here */
void metaSetPrev(char * metaData, char * value)
{

	if(metaData == value){
			//printf("ckpt27: ERROR: metadata prev points to itself\n");
               return NULL;
		}

     if(metaData != NVALUE)
     {
           char * addr = (char *)((int)metaData + CSIZE);
          *(char* *)addr = value;
     }
     else{
     //     //printf("set prev error, metaData is NULL\n");
     }
}

int64_t metaSize(char * metaData)
{
    char * addr = (char *)((int)metaData + CSIZE+CSIZE);
    return *(int64_t *)addr;
}
void metaSetSize(char * metaData, int64_t value)
{
     if(value > mem_heapsize()||value < 0)
     {         
           //printf("ckpt50: feed in size field corrupted size : %d\n",value);
           exit(0);
	}
     if(metaData != NVALUE)
     {
           char * addr = (char *)((int)metaData + CSIZE+CSIZE);
          *(int64_t *)addr = value;
     }
     else{
          //printf("set size error, metaData is NULL\n");
     }
}

int metaStatus(char * metaData)
{
    char * addr = (char *)((int)metaData + CSIZE+CSIZE+SSIZE);
    return *(int *)addr;
}
void metaSetStatus(char * metaData, int value)
{
     if(metaData != NVALUE)
     {
          char * addr = (char *)((int)metaData + CSIZE+CSIZE+SSIZE);
          *(int *)addr = value;
     }
     else{
          //printf("set status error, metaData is NULL\n");
     }
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
     else{
          return (char *)(-1);
     }
}
/* check heap function, print out error if the heap is not linked by the linked list */
void checkHeap()
{
     //printf("ckpt34: checking heap\n");
     int flag = 0;
     char * current= (char *)mem_heap_lo();
     //printf("ckpt34.0\n");
     while(metaNext(current) != NVALUE)
     {
          current = metaNext(current);
          //printf("%p ",current);
     }
     //printf("\nckpt34.1\n");
     char * end = PREV;
     while(metaPrev(end) != NVALUE)
          end = metaPrev(end);
     
     //printf("ckpt34.2\n");
     if(end != mem_heap_lo())
     {
           //printf("ckpt31: the negative direction of the linked list has an error\n");
           flag = 1;
     }
     if(current != PREV)
     {
           //printf("ckpt32: the positive direction of linked list has an error\n");
           flag = 1;
     }
     if(flag == 0){
          //printf("ckpt33: the heap is fine\nExit from checkHeap function\n");
     }
     printHeapF();
     printHeapB();

}
/* helper function lv1.5 */
void printHeapF()
{
     //printf("ckpt43: traversing heap forwardly\n");
     char * current = (char*)mem_heap_lo();
     //printf("%p(%d,%d)",current,metaSize(current),metaStatus(current));
     while(metaNext(current)!=NVALUE)
     {
          current = metaNext(current);
          //printf("---->%p(%d,%d)",current,metaSize(current),metaStatus(current));
     }
     //printf("(%p)\n",PREV);
}
void printHeapB()
{
     //printf("ckpt43: traversing heap backwardly\n");
     char * current = PREV;
     //printf("%p(%d,%d)",current,metaSize(current),metaStatus(current));
     while(metaPrev(current)!=NVALUE)
     {
          current = metaPrev(current);
          //printf("---->%p(%d,%d)",current,metaSize(current),metaStatus(current));
     }
     //printf("(%p)\n",(char *)mem_heap_lo());
}

/* helper functions lv2 */
/* find_free_block - called w/ malloc, attempts to find a space to insert new data. */
char * find_free_block(size_t size)
{
	 //printf("ckpt6 - enter find_free_block\n");
	// //printf("line about to get mem_heap_lo\n");
     char * current = mem_heap_lo();
     if(mem_heapsize() == 0){
    	 // //printf("mem heap size = 0, RETURN WITH -1\n");
          // return (char *)('b'); THIS ALSO WORKS

    	 	 //printf("ckpt7 - find_free_block cannot find a value free block\n");
    	 	 return (char *) NVALUE;
     }
     else
     {

    	     //printf("ckpt8 - starting to find a free block\n");
          while(current < (char *)mem_heap_hi())
          {
        	/* modified by Mohan Liu */
          //if (current == metaNext(current))
        	//	 exit(0);
        	 // //printf("ckpt9 - status of metadata we're inspecting\n");
        	  //mm_mallocStatus(current, NULL);
        	  // added in new conditions
        	  int64_t a= metaSize(current);
        	  int64_t b = a-size-MSIZE;
            /* Evan's wrong ideaaaa, fixed one trace file, failed majority of the others. lool*/
        	  //int64_t b = a-size-MSIZE; 
              if((metaStatus(current) == 0) && (a == size))
              {
                    return current;
              }
              else if((metaStatus(current) == 0) && (a >= size) && (b == ALIGN(b))&&(b>=0))
              {
                  //printf("ckpt20: free block found in the heap\n");
            	   //printf("ckpt17: original memroy block size : %d\n",a);
            	   //printf("ckpt18: after split, the old block size should be : %d\n",size);
            	   //printf("ckpt18.1: after split, the new block size should be : %d\n",b);
                 return current;
              }
               else
               {
                    //if(strcmp(current,"NULL")!=0)
            	    ////printf("Current Pointer: %p\n", current);
            	    if(current != (char *)NVALUE)
                         current = metaNext(current);
                    else
                         return (char *)NVALUE;
               }
          }
          //printf("ckpt21: no valid free block found in find free block function\n");
          return (char * )NVALUE;
     }
}

/* split a free block(size_t, char * metadata) */
char * split(size_t size, char * metaData)
{

     //printf("ckpt38: preprocessing heap checking - split()\n");
     checkHeap();
	
     if(size> mem_heapsize()){
          //printf("ckpt51: object size field corrupted size : %d\n",size);
     }
     if((metaSize(metaData) > mem_heapsize())||(metaSize(metaData) < 0)){
          //printf("ckpt52: metaData size field corrupted size : %d\n",metaSize(metaData));
     }

     if(size < 0){
		 //printf("chpt23: OMFG!!!! SIZE IS LESS THAN ZERO\n");
	 	 return NULL;
	}
     if(size == metaSize(metaData))
     {
           //printf("ckpt45: no need to split block,perfect match in split()\n");
           metaSetStatus(metaData,1);
           return metaBlockStart(metaData);
     }
	//printf("ckpt14 : beginning of the split function\n");
	int64_t oldsize = metaSize(metaData);
     metaSetSize(metaData,size);
     metaSetStatus(metaData,1);

     //printf("ckpt15: setting up new metaData in split\n");
     char * new = (char *)((int)metaBlockStart(metaData)+size);
     if(PREV == metaData)
          PREV = new;
     metaSetNext(new, metaNext(metaData));
     metaSetPrev(metaNext(metaData),new);
     metaSetPrev(new,metaData);
     metaSetNext(metaData,new);
     metaSetSize(new,oldsize-size-MSIZE);
     metaSetStatus(new,0);

     /* update global variables */
     TUAB += size+MSIZE;
     TFAB = TFAB-size-MSIZE;
     //printf("ckpt16: end of split function\n");

     //printf("ckpt39: postprocessing heapchecking - split()\n");
     checkHeap();
     return metaBlockStart(metaData);
}
/* print out the status of the block status allocated by malloc */
void mm_mallocStatus(char *  metaData, size_t size){
    //printf("\n******Insertion Status (End of Heap, blocksize: %d)******\n", size);
    //printf("inserted MD at %p\n", metaData);
    //printf("metaPrev = %p\n", metaPrev(metaData));
    //printf("metaNext = %p\n", metaNext(metaData));
    //printf("metaSize = %d\n", metaSize(metaData));
}


/*helper functions for free*/
void leftFusion(char * left, char * current)
{
     //printf("ckpt36: preprocessing heap check - leftFusion()\n");
     checkHeap();
	//printf("leftFusion called\n");
     int64_t current_size = metaSize(current);
     int64_t left_size = metaSize(left);

	//printf("ORIGINAL CURRENT SIZE = %d\n", current_size);
	//printf("ORIGINAL LEFT SIZE = %d\n", left_size);
     metaSetSize(left, left_size+MSIZE+current_size);
     //printf("NEW SIZE = %d\n", metaSize(left));
     metaSetNext(left, metaNext(current));
     /* update global variables */
     TUAB = TUAB -MSIZE -metaSize(current);
     TFAB = TFAB +MSIZE +metaSize(current);
     /* modified by Mohan Liu*/
     if(PREV == current)
          PREV = left;
     //printf("ckpt37: postprocessing heap check - leftFusion()\n");
     checkHeap();

}
void rightFusion(char *right, char * current)
{
     //printf("ckpt38: preprocessing heap check - rightFusion()\n");
     checkHeap();
	//printf("rightFusion called\n");
	//printf("ORIGINAL CURRENT SIZE = %d\n", metaSize(current));
	//printf("ORIGINAL RIGHT SIZE = %d\n", metaSize(right));
     metaSetSize(current,metaSize(right)+MSIZE+metaSize(current));
     //printf("NEW SIZE = %d\n", metaSize(current));
     metaSetNext(current,metaNext(right));
     /* update global variables */
     TUAB = TUAB -MSIZE -metaSize(right);
     TFAB = TFAB +MSIZE +metaSize(right);
     if(PREV == right)
          PREV = current;
     //printf("ckpt39: postprocessing heap check - rightFusion()\n");
     checkHeap();
}

void doubleFusion(char * left, char * current, char * right)
{
     //printf("ckpt40: preprocessing heap check - doubleFusion()\n");
     checkHeap();
	//printf("doubleFusion called\n");
	//printf("ORIGINAL left SIZE = %d\n", metaSize(left));
	//printf("ORIGINAL current SIZE = %d\n", metaSize(current));
	//printf("ORIGINAL right SIZE = %d\n", metaSize(right));
     metaSetSize(left, metaSize(left)+metaSize(right)+2*MSIZE+metaSize(current));
     metaSetNext(left, metaNext(right));
     metaSetPrev(metaNext(right),left);
     //printf("NEW SIZE = %d\n", metaSize(left));
     /* update global variables */
     TUAB = TUAB -2*MSIZE -metaSize(current)-metaSize(right);
     TFAB = TFAB +2*MSIZE +metaSize(current)+metaSize(right);
     if(PREV == right)
          PREV = left;
     //printf("ckpt41: postprocessing heap check - doubleFusion()\n");
     checkHeap();
}
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    //printf("\nckpt0: mm_init() is called\n");
    TUAB = 0;
    TFAB = 0;
    TAB = mem_heapsize();
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
	//printf("\nckpt1 start of malloc \n");
	//printf("Malloc called with size: %d\n", size);
	int64_t size2 = (unsigned int)size;

	int64_t asize = ALIGN(size2);
		////printf("about to call find_free_block\n");
	     char * addr = find_free_block(asize);

	     ////printf("find a freed block actuall returned with -1, like it's supposd to\n");
	     ////printf("addr = %s", addr);
	     ////printf("the RETURNED %p", &addr);
	     ////printf("the RETURNED %p", addr);

	     //if(strcmp(&addr,"b") == 0) THIS IS A WORKING THING.

	     if(addr == (char *) NVALUE) 
	     {
	    	     //printf("ckpt1.5: No block big enough for insertion found\n");
	          /* allocate a new memory block at the end of the heap */
           
           if((mem_heapsize()>0) && (metaStatus(PREV) == 0)&&((int64_t)asize > metaSize(PREV)))
           { 
               //printf("ckpt53: extending allocated heap size case 1\n");
               void * p = mem_sbrk((int64_t)asize - metaSize(PREV));
               if( p == (void*)-1 )
               {
                    return NULL;
               }
               else
               {
                    //printf("ckpt 52: simple extension heap\n");
                    metaSetSize(PREV,asize);
                    metaSetStatus(PREV,1);
                    checkHeap();
	               //printf("SUCCESSFUL SIMPLE EXTENSION\n\n");
                    return (void *)metaBlockStart(PREV);
               }
           }
           else
           { 
	    	    int64_t newsize = (int64_t)asize + MSIZE;
	        	//This is for the first metaData we insert into the heap:
	           //printf("ckpt2: extending allocated heap size case 2\n");
	        	 //printf("ckpt2.1\n");
	        	 //printf("ckpt2.2\n");
	        	 if(mem_heapsize() == 0){
                            void *p = mem_sbrk(newsize);
	                       if (p == (void *)-1) {
		                     return NULL;
	                       }
                            char * metaData = (char*)p;
		                  //printf("ckpt3: insert in the first block in the heap\n");
					   /* initialize new metadata */
					   metaSetNext(metaData,(char *)NVALUE);
					   metaSetPrev(metaData,(char *)NVALUE);
					   metaSetSize(metaData,asize);
					   metaSetStatus(metaData,1);
                            PREV = metaData;
                    checkHeap();
	               /*status*/
	               mm_mallocStatus(metaData, asize);
	               //printf("SUCCESSFUL INSERT\n\n");
	               return (void *)metaBlockStart(metaData);
	        	 }
	        	 else{
                            void *p = mem_sbrk(newsize);
	                       if (p == (void *)-1) {
		                    return NULL;
	                       }
                            char * metaData = (char*)p;
		                  //printf("ckpt4:insert in the 2nd or more block in the heap, current heapsize: %d\n",mem_heapsize());
	        		        //2nd or more metadata
					   metaSetNext(metaData,(char * )NVALUE);
					   metaSetPrev(metaData,PREV);
					   metaSetSize(metaData,asize);
					   metaSetStatus(metaData,1);
		                  /* update the previous meta data */
		                  /* this line of code generate a segmentation error */
		                  metaSetNext(PREV,metaData);
                            PREV = metaData;
                    checkHeap();
	               /*status*/
	               mm_mallocStatus(metaData, asize);
	               //printf("SUCCESSFUL INSERT\n\n");
	               return (void *)metaBlockStart(metaData);
	        	 }
               
                     
	               /*update global variables */
	               //TUAB += newsize;
	               //TAB = mem_heapsize();
           }
	     }
          /* valid block inside the heap for split or perfect match */
	     else
	     {
	    	     //printf("ckpt19: found a free block in malloc function, about to call split\n");
               char * ret = split(asize, addr);
	          checkHeap();
               return (void *)ret;
	     }
}
	//ORIGINAL MALLOC
  /*  int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);

    if (p == (void *)-1) {
	    return NULL;
    } else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
    */




/* mm_free - feed in the start address of a block */
void mm_free(void *ptr)
{

		//printf("\nfree called, trying to free block at %p\n", ptr);
		char * metaData = blockMetaStart((char*)ptr);
		//printf("free called, trying to free metadata of the corresponding block at %p\n", metaData);

          //Invalid Address
         if((metaData < (char *) mem_heap_lo())|| (metaData > (char *) mem_heap_hi())){
        	    //printf("ckpt10 - invalid md address in free.\n");
              return NULL;
         }
         //Valid Address
         else
         {
        	    //printf("ckpt11 - about to update metadata at %p & about to fusion\n", metaData);

              metaSetStatus(metaData,0);					 //change status of block to free
              char * prev = metaPrev(metaData);
              char * next = metaNext(metaData);

              //printf("ckpt11.1: pre-fusion heap checking\n");
              checkHeap();
              // left block is free and right block is taken /
              // left fusion case 1: left block is free and right block is null/

              /* double fusion first  */
              if( (next != NVALUE)&&( prev != NVALUE )&&(metaStatus(next)==0)&&(metaStatus(prev)==0) )
              {
                   doubleFusion(prev,metaData,next);
              }
              else if( ( prev != NVALUE )&&( next != NVALUE )&&(metaStatus(prev)==0)&&(metaStatus(next)==1) )
              {

                   //printf("ckpt11.1.2: left fusion second case\n");
                   leftFusion(prev,metaData);
                   metaSetPrev(next,prev);
              }
              else if( ( next != NVALUE )&&(prev != NVALUE)&&(metaStatus(next)==0)&&(metaStatus(prev)==1) )
              {
                   //printf("ckpt11.1.3: right fusion second case\n");
                   rightFusion(next,metaData);
                   char * next_next = metaNext(next);
                   //if(next_next != NVALUE)
                   metaSetPrev(next_next,metaData);
              }
              else if( (prev != NVALUE)&&(next == NVALUE)&&(metaStatus(prev)==0) )
              {
                   //printf("ckpt11.1.1: left fusion first case\n");
                   leftFusion(prev,metaData);

              }
              else if( (next != NVALUE )&&( prev == NVALUE )&&(metaStatus(next)==0) )
              {
                   //printf("ckpt11.1.4: right fusion first case\n");
                   rightFusion(next,metaData);
                   char * next_next = metaNext(next);
                   //if(next_next != NVALUE)
                   metaSetPrev(next_next,metaData);
              }
              else{
            	  //printf("ckpt12 -Fusion: No conditions hit.\n");
              }
         }
         //printf("ckpt11.2: post-fusion heap checking\n");
         checkHeap();
         //coalition process /
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
 /* modified version of code */
	//printf("ckpt25 - enter realloc\n");
	//printf("ENTERED REALLOC\n");
	if((ptr < mem_heap_lo())||(ptr > mem_heap_hi())){

		//printf("chpk 25: realloc pointer NOT in range.\n");

		//printf("EXIT REALLOC\n");
		return mm_malloc(size);
	}
	if (size == 0)
	{

		//printf("chpk 26: realloc size == 0.\n");
		mm_free(ptr);

		//printf("EXIT REALLOC\n");
		return NULL;
	}
	char * metaData = blockMetaStart((char *)ptr);
	int64_t asize = ALIGN(size);

	if((ptr > mem_heap_lo())&&(ptr < mem_heap_hi()) && asize == metaSize(metaData))
	{

		//printf("chpk 27: realloc don't move the block.\n");

		//printf("EXIT REALLOC\n");
		return NULL;
	}
	else if ((ptr > mem_heap_lo())&&(ptr < mem_heap_hi())&& asize != metaSize(metaData))
	{

		//printf("chpk 28: realloc calls malloc to copy block to a different location.\n");
		void * addr = mm_malloc(asize);
		memcpy(addr,ptr,asize);
		/* free the original block is the original is actually moved */
		mm_free(ptr);
		//printf("EXIT REALLOC\n");


		return addr;
	}
}




	/* old version of code */
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

