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
    "LoL",
    /* First member's full name */
    "z0z0r4",
    /* First member's email address */
    "z0z0r4@outlook.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define CHUNK_HEADER_SIZE 8
#define CHUNK_FOOTER_SIZE 8

#define CHUNK_SIZE(payload_size) (ALIGN(payload_size) + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE)

// ptr 指的是 payload 的地址
#define GET_PAYLOAD_SIZE(ptr) (GET_SIZE(ptr) - CHUNK_HEADER_SIZE - CHUNK_FOOTER_SIZE)

#define GET_PAYLOAD_PTR(ptr) ((char *)ptr + CHUNK_HEADER_SIZE)

// ptr 指的是 payload 的地址
#define GET_HEADER_VAL(ptr) (*(size_t *)((char *)ptr - CHUNK_HEADER_SIZE))

#define GET_FOOTER_VAL(ptr) (*(size_t *)((char *)ptr + GET_PAYLOAD_SIZE(ptr) - CHUNK_FOOTER_SIZE))

// ptr 指的是 payload 的地址
#define GET_SIZE(ptr) (GET_HEADER_VAL(ptr) & ~0x7)

// ptr 指的是 payload 的地址
#define GET_PREV_CHUNK_SIZE(ptr) (*(size_t *)((char *)ptr - CHUNK_HEADER_SIZE - CHUNK_FOOTER_SIZE))

// ptr 指的是 payload 的地址
#define GET_NEXT_CHUNK_SIZE(ptr) (*(size_t *)((char *)ptr + GET_PAYLOAD_SIZE(ptr) + CHUNK_FOOTER_SIZE))

// ptr 指的是 payload 的地址
#define IS_ALLOCATED(ptr) (GET_HEADER_VAL(ptr) & 0x1)

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

void set_chunk_size(void *ptr, int chunk_size)
{
    *(size_t *)((char *)ptr - CHUNK_HEADER_SIZE) = chunk_size;
    *(size_t *)((char *)ptr + chunk_size - CHUNK_FOOTER_SIZE) = chunk_size;
}


int prev_chunk_is_free(void *ptr)
{
    if (ptr == mem_heap_lo())
        return 0;
    else
    {
        int prev_chunk_size = GET_PREV_CHUNK_SIZE(ptr);
        void *prev_chunk_ptr = (char *)ptr - prev_chunk_size;
        return !IS_ALLOCATED(prev_chunk_ptr);
    }
}

int next_chunk_is_free(void *ptr)
{
    if (ptr == mem_heap_hi())
        return 0;
    else
    {
        void *next_chunk_ptr = (char *)ptr + GET_SIZE(ptr);
        return !IS_ALLOCATED(next_chunk_ptr);
    }
}

void clear_chunk_info(void *ptr)
{
    // header mask to 0
    // footer mask to 0
    GET_HEADER_VAL(ptr) &= ~0x7;
    GET_FOOTER_VAL(ptr) &= ~0x7;
}

void set_allocated(void *ptr)
{
    *(size_t *)((char *)ptr - CHUNK_HEADER_SIZE) |= 0x1;
}

void set_free(void *ptr)
{
    *(size_t *)((char *)ptr - CHUNK_HEADER_SIZE) &= ~0x1;
}

// void *get_next_chunk(void *ptr)
// {
//     int current_chunk_size = GET_SIZE(ptr);
//     void *maybe_next_chunk_ptr = ((char *)ptr + current_chunk_size);
//     if (maybe_next_chunk_ptr >= mem_heap_hi())
//         return NULL;
//     else
//     {
//         return (void *)((char *)ptr + current_chunk_size);
//     }
// }

void *get_chunk_from_heap(int newsize)
{
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        return p;
    }
}

void *find_free_chunk(int newsize)
{
    if (mem_heapsize() == 0)
        return NULL;

    void *ptr = mem_heap_lo();
    while (ptr < mem_heap_hi() && ptr != NULL)
    {
        int chunk_size = GET_SIZE(GET_PAYLOAD_PTR(ptr));
        if (chunk_size >= newsize && !IS_ALLOCATED(GET_PAYLOAD_PTR(ptr))) {
            return ptr;
        }
        ptr = (char *)ptr + chunk_size;
        if (ptr >= mem_heap_hi()) {
            break;
        }
    }
    return NULL;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t payload_size)
{
    // int newsize = ALIGN(size + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE);
    int newsize = CHUNK_SIZE(payload_size);
    // find free chunk, if not found, get chunk from heap
    void *ptr = find_free_chunk(newsize);
    if (ptr == NULL)
    {
        ptr = get_chunk_from_heap(newsize);
        if (ptr == NULL)
        {
            return NULL;
        }
        else
        {
            // new chunk needs to write header
            // *(size_t *)((char *)ptr - CHUNK_HEADER_SIZE) = newsize;
            set_chunk_size(GET_PAYLOAD_PTR(ptr), newsize);
        }
    }

    // all free chunk needs to set allocated flag
    // *(size_t *)((char *)ptr - CHUNK_HEADER_SIZE) |= 0x1;
    set_allocated(GET_PAYLOAD_PTR(ptr));
    return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    void *next_chunk_ptr = (void *)(char*)ptr + GET_SIZE(ptr) - CHUNK_HEADER_SIZE;
    void *prev_chunk_ptr = (void *)(char*)ptr - GET_PREV_CHUNK_SIZE(ptr) + CHUNK_HEADER_SIZE;

    int prev_free_flag = prev_chunk_is_free(ptr);
    int next_free_flag = next_chunk_is_free(ptr);
    
    if (prev_free_flag && !next_free_flag)
    {
        int new_size = GET_SIZE(ptr) + GET_PREV_CHUNK_SIZE(ptr);
        void *new_chunk_ptr = (char *)ptr - GET_PREV_CHUNK_SIZE(ptr);
        clear_chunk_info(ptr);
        clear_chunk_info(next_chunk_ptr);
        set_chunk_size(new_chunk_ptr, new_size);
    }
    else if (!prev_free_flag && next_free_flag)
    {
        int new_size = GET_SIZE(ptr) + GET_NEXT_CHUNK_SIZE(ptr);
        void *new_chunk_ptr = ptr;
        clear_chunk_info(ptr);
        clear_chunk_info(next_chunk_ptr);
        set_chunk_size(new_chunk_ptr, new_size);
    }
    else if (prev_free_flag && next_free_flag)
    {
        int new_size = GET_SIZE(ptr) + GET_PREV_CHUNK_SIZE(ptr) + GET_NEXT_CHUNK_SIZE(ptr);
        void *new_chunk_ptr = (char *)ptr - GET_PREV_CHUNK_SIZE(ptr);
        clear_chunk_info(ptr);
        clear_chunk_info(next_chunk_ptr);
        clear_chunk_info(prev_chunk_ptr);
        set_chunk_size(new_chunk_ptr, new_size);
    }
    else
    {
        // *(size_t *)((char *)ptr - CHUNK_HEADER_SIZE) &= ~1;
        set_free(ptr);
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
