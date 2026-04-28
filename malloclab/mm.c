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

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

#define PACK(size, alloc) ((size) | (alloc))

#define PAYLOAD_TO_CHUNK(bp) ((char *)(bp) - CHUNK_HEADER_SIZE)

#define CHUNK_TO_PAYLOAD(cp) ((char *)(cp) + CHUNK_HEADER_SIZE)

#define GET_SIZE(p) (GET(p) & ~0x7)

#define GET_ALLOC(p) (GET(p) & 0x1)

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

void *get_new_chunk_from_heap(int newsize)
{
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        return p;
    }
}

void *find_free_chunk(int chunk_size)
{
    if (mem_heapsize() == 0)
    {
        return NULL;
    }

    void *ptr = mem_heap_lo();
    while (ptr < mem_heap_hi() && ptr != NULL)
    {
        int current_chunk_size = GET_SIZE(ptr);
        if (current_chunk_size >= chunk_size && !GET_ALLOC(ptr))
        {
            return ptr;
        }
        ptr = (char *)ptr + current_chunk_size;
        if (ptr >= mem_heap_hi())
        {
            break;
        }
    }
    return NULL;
}

void set_chunk_meta(void *ptr, int chunk_size, int allocated_flag)
{
    PUT(ptr, PACK(chunk_size, allocated_flag));
    PUT(ptr + chunk_size - CHUNK_FOOTER_SIZE, PACK(chunk_size, allocated_flag));
}

void clear_chunk_info(void *ptr)
{
    set_chunk_meta(ptr, 0, 0);
}

void separate_free_chunk(void *ptr, int chunk_size, int new_chunk_size)
{
    if (chunk_size >= new_chunk_size + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE + SIZE_T_SIZE)
    {
        set_chunk_meta(ptr, new_chunk_size, 0);
        void *new_chunk_ptr = (char *)ptr + new_chunk_size;
        int remaining_chunk_size = chunk_size - new_chunk_size;
        set_chunk_meta(new_chunk_ptr, remaining_chunk_size, 0);
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE);

    void *ptr = find_free_chunk(newsize);
    if (ptr == NULL)
    {
        ptr = get_new_chunk_from_heap(newsize);
        if (ptr == NULL)
        {
            return NULL;
        }
        else
        {
            set_chunk_meta(ptr, newsize, 1);
        }
    }
    else
    {
        int current_chunk_size = GET_SIZE(ptr);
        // if the remaining free chunk is big enough to hold the meta info and at least 1 bytes, then separate the chunk
        if (current_chunk_size >= newsize + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE + SIZE_T_SIZE)
        {
            separate_free_chunk(ptr, current_chunk_size, newsize);
            set_chunk_meta(ptr, newsize, 1);
        }
        else
        {
            // just allocate the whole chunk
            set_chunk_meta(ptr, current_chunk_size, 1);
        }
    }

    return CHUNK_TO_PAYLOAD(ptr);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    void *chunk_ptr = PAYLOAD_TO_CHUNK(ptr);
    int chunk_size = GET_SIZE(chunk_ptr);

    // merge with nearby free chunks
    void *next_chunk_ptr = (char *)chunk_ptr + chunk_size;
    void *prev_chunk_footer_ptr = (char *)chunk_ptr - CHUNK_FOOTER_SIZE;
    void *prev_chunk_ptr = (char *)chunk_ptr - GET_SIZE(prev_chunk_footer_ptr);

    int prev_chunk_free_flag = 0;
    int next_chunk_free_flag = 0;
    int prev_chunk_size = 0;
    int next_chunk_size = 0;

    if (prev_chunk_ptr >= mem_heap_lo() && !GET_ALLOC(prev_chunk_ptr))
    {
        prev_chunk_free_flag = GET_ALLOC(prev_chunk_ptr);
        prev_chunk_size = GET_SIZE(prev_chunk_ptr);
    }

    if (next_chunk_ptr < mem_heap_hi() && !GET_ALLOC(next_chunk_ptr))
    {
        next_chunk_free_flag = GET_ALLOC(next_chunk_ptr);
        next_chunk_size = GET_SIZE(next_chunk_ptr);
    }

    if (!prev_chunk_free_flag && !next_chunk_free_flag)
    {
        // just set current chunk to free
        set_chunk_meta(chunk_ptr, chunk_size, 0);
    }
    else if (prev_chunk_free_flag && !next_chunk_free_flag)
    {
        // merge with previous
        int merged_chunk_size = chunk_size + prev_chunk_size;
        clear_chunk_info(chunk_ptr);
        set_chunk_meta(prev_chunk_ptr, merged_chunk_size, 0);
    }
    else if (!prev_chunk_free_flag && next_chunk_free_flag)
    {
        // merge with next
        int merged_chunk_size = chunk_size + next_chunk_size;
        clear_chunk_info(chunk_ptr);
        set_chunk_meta(next_chunk_ptr, merged_chunk_size, 0);
    }
    else
    {
        // merge with both
        int merged_chunk_size = chunk_size + prev_chunk_size + next_chunk_size;
        clear_chunk_info(chunk_ptr);
        clear_chunk_info(next_chunk_ptr);
        set_chunk_meta(prev_chunk_ptr, merged_chunk_size, 0);
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
