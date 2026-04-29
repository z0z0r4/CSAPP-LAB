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

#define CHUNK_NEXT_FREE_P_SIZE 8
#define CHUNK_PREV_FREE_P_SIZE 8

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

#define GET_PTR(p) (*(void **)(p))
#define PUT_PTR(p, val) (*(void **)(p) = (void *)(val))

#define PACK(size, alloc) ((size) | (alloc))

#define PAYLOAD_TO_CHUNK(bp) ((char *)(bp) - CHUNK_HEADER_SIZE)

#define CHUNK_TO_PAYLOAD(cp) ((char *)(cp) + CHUNK_HEADER_SIZE)

#define GET_SIZE(p) (GET(p) & ~0x7)

#define GET_ALLOC(p) (GET(p) & 0x1)

// chunk: [size|allocated][next_free_chunk_p][prev_free_chunk_p]
#define GET_NEXT_FREE_CHUNK(p) ((void *)GET_PTR((char *)p + CHUNK_HEADER_SIZE))

#define GET_PREV_FREE_CHUNK(p) ((void *)GET_PTR((char *)p + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE))

#define SMALLEST_CHUNK_SIZE (CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE + CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE)

static void *sentinel_chunk_ptr = NULL;

static void *rover = NULL;

void set_chunk_meta(void *ptr, size_t chunk_size, int allocated_flag)
{
    // set header
    PUT(ptr, PACK(chunk_size, allocated_flag));

    // set footer
    PUT(ptr + chunk_size - CHUNK_FOOTER_SIZE, PACK(chunk_size, allocated_flag));
}

void set_chunk_next_p(void *ptr, void *next_ptr)
{
    // PUT(ptr + CHUNK_HEADER_SIZE, next_ptr);
    PUT_PTR((char *)ptr + CHUNK_HEADER_SIZE, next_ptr);
}

void set_chunk_prev_p(void *ptr, void *prev_ptr)
{
    // PUT(ptr + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE, prev_ptr);
    PUT_PTR((char *)ptr + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE, prev_ptr);
}

void *get_new_chunk_from_heap(size_t newsize)
{
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        return p;
    }
}

void *find_free_chunk(size_t chunk_size)
{
    if (sentinel_chunk_ptr == NULL)
    {
        return NULL;
    }

    if (rover == NULL || rover == sentinel_chunk_ptr)
    {
        rover = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
    }

    void *start_ptr = rover;

    void *ptr = rover;
    do
    {
        size_t current_chunk_size = GET_SIZE(ptr);
        if (current_chunk_size >= chunk_size && !GET_ALLOC(ptr))
        {
            rover = GET_NEXT_FREE_CHUNK(ptr);
            return ptr;
        }
        ptr = GET_NEXT_FREE_CHUNK(ptr);
    } while (ptr != start_ptr);

    return NULL;
}

void insert_chunk_to_free_linked_list(void *ptr)
{
    void *sentinel_chunk_next_chunk_ptr = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
    // set self
    set_chunk_next_p(ptr, sentinel_chunk_next_chunk_ptr);
    set_chunk_prev_p(ptr, sentinel_chunk_ptr);

    // set sentinel_chunk
    set_chunk_next_p(sentinel_chunk_ptr, ptr);

    // set sentinel_chunk next
    set_chunk_prev_p(sentinel_chunk_next_chunk_ptr, ptr);
}

void remove_chunk_from_free_linked_list(void *ptr)
{
    if (rover == ptr)
    {
        rover = GET_NEXT_FREE_CHUNK(ptr);
    }

    void *prev_chunk_ptr = GET_PREV_FREE_CHUNK(ptr);
    void *next_chunk_ptr = GET_NEXT_FREE_CHUNK(ptr);

    // set prev chunk next p
    set_chunk_next_p(prev_chunk_ptr, next_chunk_ptr);

    // set next chunk prev p
    set_chunk_prev_p(next_chunk_ptr, prev_chunk_ptr);
}

void separate_free_chunk(void *ptr, size_t new_chunk_size)
{
    size_t current_chunk_size = GET_SIZE(ptr);
    if (current_chunk_size >= new_chunk_size + SMALLEST_CHUNK_SIZE)
    {
        set_chunk_meta(ptr, new_chunk_size, 0);
        void *new_chunk_ptr = (char *)ptr + new_chunk_size;
        size_t remaining_chunk_size = current_chunk_size - new_chunk_size;
        set_chunk_meta(new_chunk_ptr, remaining_chunk_size, 0);
        insert_chunk_to_free_linked_list(new_chunk_ptr);
    }
}

int mm_checkheap(int verbose)
{
    void *bp;
    size_t implicit_free_count = 0;
    size_t explicit_free_count = 0;

    // ---------------------------------------------------------
    // 1. 隐式遍历：从 Sentinel Chunk 开始，按内存地址顺序遍历整个堆
    // ---------------------------------------------------------
    bp = sentinel_chunk_ptr;

    // 假设 sentinel_chunk 是堆的最开头，遍历直到堆顶
    while (bp != NULL && bp < mem_heap_hi())
    {
        size_t size = GET_SIZE(bp);
        int alloc = GET_ALLOC(bp);

        // 检查1：块大小不能为0，否则会导致死循环
        if (size == 0 && bp != sentinel_chunk_ptr)
        {
            printf("Heap Error: Chunk size is 0 at %p\n", bp);
            return 0;
        }

        // 检查2：Header 和 Footer 是否匹配
        size_t header = GET(bp);
        size_t footer = GET((char *)bp + size - CHUNK_FOOTER_SIZE);
        if (header != footer)
        {
            printf("Heap Error: Header and Footer mismatch at %p\n", bp);
            return 0;
        }

        // 统计真实的空闲块数量（跳过哨兵块）
        if (!alloc && bp != sentinel_chunk_ptr)
        {
            implicit_free_count++;
        }

        // 步进到下一个块
        bp = (char *)bp + size;
    }

    // ---------------------------------------------------------
    // 2. 显式遍历：沿着双向链表的 next 指针进行遍历
    // ---------------------------------------------------------
    if (sentinel_chunk_ptr != NULL)
    {
        void *free_bp = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);

        while (free_bp != sentinel_chunk_ptr && free_bp != NULL)
        {
            // 检查3：在空闲链表中的块，其 allocated 标志位必须是 0
            if (GET_ALLOC(free_bp))
            {
                printf("Heap Error: Allocated block found in free list at %p\n", free_bp);
                return 0;
            }

            // 检查4：双向链表指针的一致性 (A->next->prev 必须等于 A)
            void *next = GET_NEXT_FREE_CHUNK(free_bp);
            void *prev = GET_PREV_FREE_CHUNK(free_bp);

            if (GET_PREV_FREE_CHUNK(next) != free_bp)
            {
                printf("Heap Error: next->prev != current at %p\n", free_bp);
                return 0;
            }
            if (GET_NEXT_FREE_CHUNK(prev) != free_bp)
            {
                printf("Heap Error: prev->next != current at %p\n", free_bp);
                return 0;
            }

            explicit_free_count++;
            free_bp = next;
        }
    }

    // ---------------------------------------------------------
    // 3. 交叉比对：检查是否有空闲块“迷路”了
    // ---------------------------------------------------------
    if (implicit_free_count != explicit_free_count)
    {
        printf("Heap Error: Free block count mismatch! Implicit: %d, Explicit: %d\n",
               implicit_free_count, explicit_free_count);
        return 0;
    }

    if (verbose)
    {
        printf("Heap is consistent. Total free blocks: %d\n", explicit_free_count);
    }

    return 1;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // sentinel chunk
    size_t sentinel_chunk_size = SMALLEST_CHUNK_SIZE;
    sentinel_chunk_ptr = get_new_chunk_from_heap(sentinel_chunk_size);
    set_chunk_meta(sentinel_chunk_ptr, sentinel_chunk_size, 1);
    set_chunk_next_p(sentinel_chunk_ptr, sentinel_chunk_ptr);
    set_chunk_prev_p(sentinel_chunk_ptr, sentinel_chunk_ptr);
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize = (size >= (CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE)) ? ALIGN(size + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE) : SMALLEST_CHUNK_SIZE;

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
        size_t current_chunk_size = GET_SIZE(ptr);
        // if the remaining free chunk is big enough to hold the meta info and at least 1 bytes, then separate the chunk
        remove_chunk_from_free_linked_list(ptr);
        if (current_chunk_size >= newsize + SMALLEST_CHUNK_SIZE)
        {
            separate_free_chunk(ptr, newsize);
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
    size_t chunk_size = GET_SIZE(chunk_ptr);

    // merge with nearby free chunks
    void *next_chunk_ptr = (char *)chunk_ptr + chunk_size;
    void *prev_chunk_footer_ptr = (char *)chunk_ptr - CHUNK_FOOTER_SIZE;
    void *prev_chunk_ptr = (char *)chunk_ptr - GET_SIZE(prev_chunk_footer_ptr);

    int prev_chunk_free_flag = 0;
    int next_chunk_free_flag = 0;
    size_t prev_chunk_size = 0;
    size_t next_chunk_size = 0;

    void *new_free_chunk_ptr = NULL;

    if (prev_chunk_ptr >= mem_heap_lo())
    {
        prev_chunk_free_flag = !GET_ALLOC(prev_chunk_ptr);
        prev_chunk_size = GET_SIZE(prev_chunk_ptr);
    }

    if (next_chunk_ptr < mem_heap_hi())
    {
        next_chunk_free_flag = !GET_ALLOC(next_chunk_ptr);
        next_chunk_size = GET_SIZE(next_chunk_ptr);
    }

    if (!prev_chunk_free_flag && !next_chunk_free_flag)
    {
        // just set current chunk to free
        set_chunk_meta(chunk_ptr, chunk_size, 0);
        new_free_chunk_ptr = chunk_ptr;
    }
    else if (prev_chunk_free_flag && !next_chunk_free_flag)
    {
        // merge with previous
        size_t merged_chunk_size = chunk_size + prev_chunk_size;
        set_chunk_meta(prev_chunk_ptr, merged_chunk_size, 0);
        remove_chunk_from_free_linked_list(prev_chunk_ptr);
        new_free_chunk_ptr = prev_chunk_ptr;
    }
    else if (!prev_chunk_free_flag && next_chunk_free_flag)
    {
        // merge with next
        size_t merged_chunk_size = chunk_size + next_chunk_size;
        set_chunk_meta(chunk_ptr, merged_chunk_size, 0);
        remove_chunk_from_free_linked_list(next_chunk_ptr);
        new_free_chunk_ptr = chunk_ptr;
    }
    else
    {
        // merge with both
        size_t merged_chunk_size = chunk_size + prev_chunk_size + next_chunk_size;
        set_chunk_meta(prev_chunk_ptr, merged_chunk_size, 0);
        remove_chunk_from_free_linked_list(prev_chunk_ptr);
        remove_chunk_from_free_linked_list(next_chunk_ptr);
        new_free_chunk_ptr = prev_chunk_ptr;
    }

    if (new_free_chunk_ptr != NULL)
    {
        insert_chunk_to_free_linked_list(new_free_chunk_ptr);
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = PAYLOAD_TO_CHUNK(ptr);
    void *newptr;
    size_t copySize;
    size_t oldsize = GET_SIZE(oldptr);
    size_t oldPayloadSize = oldsize - CHUNK_HEADER_SIZE - CHUNK_FOOTER_SIZE;

    size_t req_chunk_size = ALIGN(size + CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE);

    // big chunk -> small chunk, in-place shrink
    if (req_chunk_size <= oldsize)
    {
        if (oldsize >= req_chunk_size + SMALLEST_CHUNK_SIZE)
        {
            separate_free_chunk(oldptr, req_chunk_size);
            set_chunk_meta(oldptr, req_chunk_size, 1);
        }
        // mm_checkheap(0);
        return CHUNK_TO_PAYLOAD(oldptr);
    }

    // 有下一个块且下一个块是空闲的
    if ((void *)((char *)oldptr + oldsize) < mem_heap_hi() && !GET_ALLOC((char *)oldptr + oldsize))
    {
        void *next_free_chunk_ptr = (char *)oldptr + oldsize;
        size_t next_free_chunk_size = GET_SIZE(next_free_chunk_ptr);

        // 如果当前块和下一个空闲块合并后足够大，在原地扩展
        if (oldsize + next_free_chunk_size >= req_chunk_size)
        {
            // merge with next free chunk
            remove_chunk_from_free_linked_list(next_free_chunk_ptr);
            size_t merged_chunk_size = oldsize + next_free_chunk_size;

            // 如果合并后的块足够大，可以分割出一个新的空闲块
            if (merged_chunk_size >= req_chunk_size + SMALLEST_CHUNK_SIZE)
            {
                // separate the merged chunk into new allocated chunk and free chunk
                set_chunk_meta(oldptr, req_chunk_size, 1);
                void *new_chunk_ptr = (char *)oldptr + req_chunk_size;
                size_t remaining_chunk_size = merged_chunk_size - req_chunk_size;
                set_chunk_meta(new_chunk_ptr, remaining_chunk_size, 0);
                insert_chunk_to_free_linked_list(new_chunk_ptr);
            }
            else
            {
                set_chunk_meta(oldptr, merged_chunk_size, 1);
            }

            return CHUNK_TO_PAYLOAD(oldptr);
        }
    }

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = oldPayloadSize;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, CHUNK_TO_PAYLOAD(oldptr), copySize);
    mm_free(CHUNK_TO_PAYLOAD(oldptr));
    return newptr;
}
