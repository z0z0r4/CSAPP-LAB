#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    "咕咕咕",
    "z0z0r4",
    "z0z0r4@outlook.com",
    "",
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define BUCKET_SIZE 24

#define CHUNK_HEADER_SIZE 4
#define CHUNK_FOOTER_SIZE 4

#define CHUNK_NEXT_FREE_OFFSET_SIZE 4
#define CHUNK_PREV_FREE_OFFSET_SIZE 4

/* 用 unsigned int (4字节) */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

#define ADDR_TO_OFFSET(p) ((p) == NULL ? 0 : (unsigned int)((char *)(p) - (char *)mem_heap_lo()))
#define OFFSET_TO_ADDR(off) ((off) == 0 ? NULL : (void *)((char *)mem_heap_lo() + (off)))

#define GET_OFFSET(p) (*(unsigned int *)(p))
#define PUT_OFFSET(p, off) (*(unsigned int *)(p) = (unsigned int)(off))

#define PACK(size, alloc, prev_alloc) ((size) | (alloc) | ((prev_alloc) << 1))

#define PAYLOAD_TO_CHUNK(bp) ((char *)(bp) - CHUNK_HEADER_SIZE)
#define CHUNK_TO_PAYLOAD(cp) ((char *)(cp) + CHUNK_HEADER_SIZE)

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

#define GET_NEXT_FREE_CHUNK(p) (OFFSET_TO_ADDR(GET_OFFSET((char *)(p) + CHUNK_HEADER_SIZE)))
#define GET_PREV_FREE_CHUNK(p) (OFFSET_TO_ADDR(GET_OFFSET((char *)(p) + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_OFFSET_SIZE)))

/* 4 + 4 + 4 + 4 = 16 字节 */
#define SMALLEST_CHUNK_SIZE (CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE + CHUNK_NEXT_FREE_OFFSET_SIZE + CHUNK_PREV_FREE_OFFSET_SIZE)

#define PRE_EXTEND_SIZE 4096

static void *bucket[BUCKET_SIZE];

int mm_checkheap(int verbose);

/**
 * 混合桶索引计算 (Smallbins + Largebins)
 */
int get_slot_index(size_t size)
{
    if (size <= 72)
    {
        return (size - 16) / 8;
    }

    int class_index = 8;
    size_t class_size = 128;

    while (class_index < BUCKET_SIZE - 1 && size > class_size)
    {
        class_size <<= 1;
        class_index++;
    }
    return class_index;
}

void *get_slot_ptr(size_t size)
{
    int slot_index = get_slot_index(size);
    return bucket[slot_index];
}

void set_current_chunk_prev_alloc(void *ptr, int flag)
{
    unsigned int current = GET(ptr);
    current = (current & ~2U) | ((unsigned int)flag << 1);
    PUT(ptr, current);
}

void set_current_chunk_alloc(void *ptr, int flag)
{
    unsigned int current = GET(ptr);
    current = (current & ~1U) | (unsigned int)flag;
    PUT(ptr, current);
}

void set_next_chunk_prev_alloc(void *ptr, int flag)
{
    void *next_chunk_ptr = (char *)ptr + GET_SIZE(ptr);
    if (next_chunk_ptr <= mem_heap_hi())
    {
        set_current_chunk_prev_alloc(next_chunk_ptr, flag);
    }
}

void set_chunk_size(void *ptr, size_t size)
{
    unsigned int back_3_bit = GET(ptr) & 0x7;
    PUT(ptr, (unsigned int)size | back_3_bit);
}

void copy_header_to_footer(void *ptr)
{
    size_t new_free_chunk_size = GET_SIZE(ptr);
    void *new_free_chunk_footer_ptr = (char *)ptr + new_free_chunk_size - CHUNK_FOOTER_SIZE;
    PUT(new_free_chunk_footer_ptr, GET(ptr));
}

void set_chunk_meta(void *ptr, size_t chunk_size, int allocated_flag, int prev_allocated_flag)
{
    PUT(ptr, PACK(chunk_size, allocated_flag, prev_allocated_flag));
    if (!allocated_flag)
    {
        copy_header_to_footer(ptr);
    }
}

void set_chunk_next_p(void *ptr, void *next_ptr)
{
    unsigned int offset = ADDR_TO_OFFSET(next_ptr);
    PUT_OFFSET((char *)ptr + CHUNK_HEADER_SIZE, offset);
}

void set_chunk_prev_p(void *ptr, void *prev_ptr)
{
    unsigned int offset = ADDR_TO_OFFSET(prev_ptr);
    PUT_OFFSET((char *)ptr + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_OFFSET_SIZE, offset);
}

int get_last_chunk_alloc()
{
    return GET_PREV_ALLOC((char *)mem_heap_hi() - CHUNK_HEADER_SIZE + 1);
}

int is_nearby_epilogue_chunk(void *ptr)
{
    size_t size = GET_SIZE(ptr);
    void *epilogue_chunk_ptr = (char *)mem_heap_hi() - CHUNK_HEADER_SIZE + 1;
    return (char *)ptr + size == epilogue_chunk_ptr;
}

void set_epilogue_chunk(int prev_alloc)
{
    void *epilogue_ptr = (char *)mem_heap_hi() - CHUNK_HEADER_SIZE + 1;
    PUT(epilogue_ptr, PACK(0, 1, prev_alloc));
}

void *get_new_chunk_from_heap(size_t new_size)
{
    void *p = mem_sbrk(new_size);
    if (p == (void *)-1)
        return NULL;
    else
        return p;
}

/**
 * Best-Fit
 */
void *find_free_chunk(size_t chunk_size, void *sentinel_chunk_ptr)
{
    if (sentinel_chunk_ptr == NULL)
        return NULL;

    size_t best_fit_size = ~0UL; // 初始化为 size_t 的最大值
    void *best_fit_chunk_ptr = NULL;

    void *ptr = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
    while (ptr != sentinel_chunk_ptr)
    {
        size_t current_chunk_size = GET_SIZE(ptr);
        if (!GET_ALLOC(ptr) && current_chunk_size >= chunk_size)
        {
            if (current_chunk_size - chunk_size < best_fit_size - chunk_size)
            {
                best_fit_size = current_chunk_size;
                best_fit_chunk_ptr = ptr;

                if (best_fit_size == chunk_size)
                {
                    break;
                }
            }
        }
        ptr = GET_NEXT_FREE_CHUNK(ptr);
    }

    return best_fit_chunk_ptr;
}

void *find_free_chunk_in_bucket(size_t chunk_size)
{
    int slot_index = get_slot_index(chunk_size);
    void *result_ptr = NULL;
    while (slot_index < BUCKET_SIZE && result_ptr == NULL)
    {
        void *sentinel_chunk_ptr = bucket[slot_index];
        result_ptr = find_free_chunk(chunk_size, sentinel_chunk_ptr);
        slot_index++;
    }
    return result_ptr;
}

void insert_chunk_to_free_linked_list(void *ptr, void *sentinel_chunk_ptr)
{
    void *sentinel_chunk_next_chunk_ptr = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
    set_chunk_next_p(ptr, sentinel_chunk_next_chunk_ptr);
    set_chunk_prev_p(ptr, sentinel_chunk_ptr);
    set_chunk_next_p(sentinel_chunk_ptr, ptr);
    set_chunk_prev_p(sentinel_chunk_next_chunk_ptr, ptr);
}

void insert_chunk_to_bucket(void *ptr)
{
    size_t chunk_size = GET_SIZE(ptr);
    insert_chunk_to_free_linked_list(ptr, get_slot_ptr(chunk_size));
}

void remove_chunk_from_free_linked_list(void *ptr)
{
    void *prev_chunk_ptr = GET_PREV_FREE_CHUNK(ptr);
    void *next_chunk_ptr = GET_NEXT_FREE_CHUNK(ptr);
    set_chunk_next_p(prev_chunk_ptr, next_chunk_ptr);
    set_chunk_prev_p(next_chunk_ptr, prev_chunk_ptr);
}

void *separate_chunk(void *ptr, size_t current_chunk_size, size_t new_chunk_size)
{
    if (current_chunk_size >= new_chunk_size + SMALLEST_CHUNK_SIZE)
    {
        set_chunk_size(ptr, new_chunk_size);
        void *remaining_chunk_ptr = (char *)ptr + new_chunk_size;
        size_t remaining_chunk_size = current_chunk_size - new_chunk_size;
        set_chunk_meta(remaining_chunk_ptr, remaining_chunk_size, 0, GET_ALLOC(ptr));
        return remaining_chunk_ptr;
    }
    return NULL;
}

void separate_free_chunk(void *ptr, size_t current_chunk_size, size_t new_chunk_size)
{
    void *remaining_chunk_ptr = separate_chunk(ptr, current_chunk_size, new_chunk_size);
    if (remaining_chunk_ptr != NULL)
    {
        insert_chunk_to_bucket(remaining_chunk_ptr);
    }
}

void *create_sentinel_chunk()
{
    size_t size = SMALLEST_CHUNK_SIZE;
    void *ptr = get_new_chunk_from_heap(size);
    set_chunk_meta(ptr, size, 1, 1);
    set_chunk_next_p(ptr, ptr);
    set_chunk_prev_p(ptr, ptr);
    return ptr;
}

void *create_epilogue_chunk()
{
    size_t size = CHUNK_HEADER_SIZE;
    void *ptr = get_new_chunk_from_heap(size);
    PUT(ptr, PACK(0, 1, 1));
    return ptr;
}

/*
 * mm_init
 */
int mm_init(void)
{
    mem_sbrk(4); // padding to ensure payload 8-byte alignment
    for (int i = 0; i < BUCKET_SIZE; i++)
    {
        bucket[i] = create_sentinel_chunk();
    }
    create_epilogue_chunk();
    return 0;
}

/*
 * mm_malloc
 */
void *mm_malloc(size_t size)
{
    mm_checkheap(1);
    if (size == 0)
        return NULL;

    size_t new_size = ALIGN(size + CHUNK_HEADER_SIZE);
    if (new_size < SMALLEST_CHUNK_SIZE)
    {
        new_size = SMALLEST_CHUNK_SIZE;
    }

    void *ptr = find_free_chunk_in_bucket(new_size);

    if (ptr == NULL)
    {
        size_t extend_size = new_size > PRE_EXTEND_SIZE ? new_size : PRE_EXTEND_SIZE;
        int last_alloc_flag = get_last_chunk_alloc();

        ptr = get_new_chunk_from_heap(extend_size);
        if (ptr == NULL)
            return NULL;

        ptr = (char *)ptr - CHUNK_HEADER_SIZE;
        set_epilogue_chunk(1);
        set_chunk_meta(ptr, extend_size, 0, last_alloc_flag);
    }
    else
    {
        remove_chunk_from_free_linked_list(ptr);
    }

    size_t current_chunk_size = GET_SIZE(ptr);
    void *allocated_ptr = ptr;

    if (current_chunk_size >= new_size + SMALLEST_CHUNK_SIZE)
    {
        size_t remaining_chunk_size = current_chunk_size - new_size;

        if (new_size > 96)
        {
            allocated_ptr = ptr;
            set_chunk_meta(allocated_ptr, new_size, 1, GET_PREV_ALLOC(ptr));

            void *remaining_ptr = (char *)allocated_ptr + new_size;
            set_chunk_meta(remaining_ptr, remaining_chunk_size, 0, 1);
            insert_chunk_to_bucket(remaining_ptr);
        }
        else
        {
            void *remaining_ptr = ptr;
            set_chunk_meta(remaining_ptr, remaining_chunk_size, 0, GET_PREV_ALLOC(ptr));
            insert_chunk_to_bucket(remaining_ptr);

            allocated_ptr = (char *)ptr + remaining_chunk_size;
            set_chunk_meta(allocated_ptr, new_size, 1, 0);
            set_next_chunk_prev_alloc(allocated_ptr, 1);
        }
    }
    else
    {
        set_current_chunk_alloc(ptr, 1);
        set_next_chunk_prev_alloc(ptr, 1);
        allocated_ptr = ptr;
    }

    mm_checkheap(1);
    return CHUNK_TO_PAYLOAD(allocated_ptr);
}

/*
 * mm_free
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)
        return;

    void *chunk_ptr = PAYLOAD_TO_CHUNK(ptr);
    size_t chunk_size = GET_SIZE(chunk_ptr);
    size_t prev_chunk_size = 0;
    size_t next_chunk_size = 0;

    int prev_chunk_free_flag = 0;
    int next_chunk_free_flag = 0;

    void *next_chunk_ptr = (char *)chunk_ptr + chunk_size;
    void *prev_chunk_footer_ptr = (char *)chunk_ptr - CHUNK_FOOTER_SIZE;
    void *prev_chunk_ptr = NULL;

    void *new_free_chunk_ptr = NULL;

    if (!GET_PREV_ALLOC(chunk_ptr))
    {
        prev_chunk_free_flag = 1;
        prev_chunk_size = GET_SIZE(prev_chunk_footer_ptr);
        prev_chunk_ptr = (char *)chunk_ptr - prev_chunk_size;
    }

    if (next_chunk_ptr < mem_heap_hi())
    {
        next_chunk_free_flag = !GET_ALLOC(next_chunk_ptr);
        next_chunk_size = GET_SIZE(next_chunk_ptr);
    }

    if (!prev_chunk_free_flag && !next_chunk_free_flag)
    {
        set_current_chunk_alloc(chunk_ptr, 0);
        set_current_chunk_prev_alloc(chunk_ptr, 1);
        set_next_chunk_prev_alloc(chunk_ptr, 0);
        new_free_chunk_ptr = chunk_ptr;
    }
    else if (prev_chunk_free_flag && !next_chunk_free_flag)
    {
        size_t merged_chunk_size = chunk_size + prev_chunk_size;
        set_chunk_size(prev_chunk_ptr, merged_chunk_size);
        set_current_chunk_alloc(prev_chunk_ptr, 0);
        remove_chunk_from_free_linked_list(prev_chunk_ptr);
        set_current_chunk_prev_alloc(prev_chunk_ptr, GET_PREV_ALLOC(prev_chunk_ptr));
        set_next_chunk_prev_alloc(prev_chunk_ptr, 0);
        new_free_chunk_ptr = prev_chunk_ptr;
    }
    else if (!prev_chunk_free_flag && next_chunk_free_flag)
    {
        size_t merged_chunk_size = chunk_size + next_chunk_size;
        set_chunk_size(chunk_ptr, merged_chunk_size);
        set_current_chunk_alloc(chunk_ptr, 0);
        remove_chunk_from_free_linked_list(next_chunk_ptr);
        set_current_chunk_prev_alloc(chunk_ptr, 1);
        set_next_chunk_prev_alloc(chunk_ptr, 0);
        new_free_chunk_ptr = chunk_ptr;
    }
    else
    {
        size_t merged_chunk_size = chunk_size + prev_chunk_size + next_chunk_size;
        int prev_chunk_prev_alloc = GET_PREV_ALLOC(prev_chunk_ptr);
        set_current_chunk_prev_alloc(prev_chunk_ptr, 0);
        set_chunk_size(prev_chunk_ptr, merged_chunk_size);
        remove_chunk_from_free_linked_list(prev_chunk_ptr);
        remove_chunk_from_free_linked_list(next_chunk_ptr);
        set_current_chunk_prev_alloc(prev_chunk_ptr, prev_chunk_prev_alloc);
        set_next_chunk_prev_alloc(prev_chunk_ptr, 0);
        new_free_chunk_ptr = prev_chunk_ptr;
    }

    if (new_free_chunk_ptr != NULL)
    {
        insert_chunk_to_bucket(new_free_chunk_ptr);
        copy_header_to_footer(new_free_chunk_ptr);
    }
}

/*
 * mm_realloc
 */
void *mm_realloc(void *ptr, size_t size)
{
    mm_checkheap(1);
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *old_ptr = PAYLOAD_TO_CHUNK(ptr);
    size_t old_size = GET_SIZE(old_ptr);
    size_t old_payload_size = old_size - CHUNK_HEADER_SIZE;
    size_t req_chunk_size = ALIGN(size + CHUNK_HEADER_SIZE);
    
    if (req_chunk_size < SMALLEST_CHUNK_SIZE) 
        req_chunk_size = SMALLEST_CHUNK_SIZE;

    // 原地缩容逻辑
    if (req_chunk_size <= old_size)
    {
        void *remaining_chunk_ptr = separate_chunk(old_ptr, old_size, req_chunk_size);
        if (remaining_chunk_ptr != NULL)
        {
            set_chunk_meta(remaining_chunk_ptr, old_size - req_chunk_size, 0, 1);
            insert_chunk_to_bucket(remaining_chunk_ptr);
            set_next_chunk_prev_alloc(remaining_chunk_ptr, 0);
        }
        return ptr;
    }

    // 堆顶扩展 (Trace 9)
    // 如果当前块本身就是堆里的最后一个块（后面紧跟着 epilogue）
    void *next_physical_chunk = (char *)old_ptr + old_size;
    if (is_nearby_epilogue_chunk(old_ptr) || 
       (!GET_ALLOC(next_physical_chunk) && is_nearby_epilogue_chunk(next_physical_chunk)))
    {
        size_t next_size = GET_ALLOC(next_physical_chunk) ? 0 : GET_SIZE(next_physical_chunk);
        size_t total_tail_size = old_size + next_size;

        if (total_tail_size < req_chunk_size)
        {
            size_t extend_size = req_chunk_size - total_tail_size;
            if (mem_sbrk(extend_size) == (void *)-1) return NULL;
            total_tail_size = req_chunk_size;
            // 更新 epilogue
            set_epilogue_chunk(1);
        }

        if (!GET_ALLOC(next_physical_chunk) && next_size > 0)
        {
            remove_chunk_from_free_linked_list(next_physical_chunk);
        }

        set_chunk_meta(old_ptr, total_tail_size, 1, GET_PREV_ALLOC(old_ptr));
        mm_checkheap(1);
        return ptr;
    }

    // 双向合并 (Trace 10)
    int prev_is_free = !GET_PREV_ALLOC(old_ptr);
    int next_is_free = !GET_ALLOC(next_physical_chunk);
    
    size_t next_size = next_is_free ? GET_SIZE(next_physical_chunk) : 0;
    size_t prev_size = prev_is_free ? GET_SIZE((char *)old_ptr - CHUNK_FOOTER_SIZE) : 0;
    
    size_t total_avail = old_size + next_size + prev_size;

    if (total_avail >= req_chunk_size)
    {
        void *new_base = prev_is_free ? (char *)old_ptr - prev_size : old_ptr;
        int original_prev_alloc_flag = GET_PREV_ALLOC(new_base);

        if (prev_is_free)
        {
            // 必须提前保存
            void *p_prev_chunk = GET_PREV_FREE_CHUNK(new_base);
            void *n_next_chunk = GET_NEXT_FREE_CHUNK(new_base);
            set_chunk_next_p(p_prev_chunk, n_next_chunk);
            set_chunk_prev_p(n_next_chunk, p_prev_chunk);

            // 再覆盖指针
            memmove(CHUNK_TO_PAYLOAD(new_base), ptr, old_payload_size);
        }
        
        if (next_is_free)
        {
            remove_chunk_from_free_linked_list(next_physical_chunk);
        }

        // 设置新块元数据
        set_chunk_meta(new_base, total_avail, 1, original_prev_alloc_flag);

        // 分割
        void *rem_ptr = separate_chunk(new_base, total_avail, req_chunk_size);
        if (rem_ptr != NULL)
        {
            set_chunk_meta(rem_ptr, total_avail - req_chunk_size, 0, 1);
            insert_chunk_to_bucket(rem_ptr);
            set_next_chunk_prev_alloc(rem_ptr, 0);
        }
        else
        {
            set_next_chunk_prev_alloc(new_base, 1);
        }

        mm_checkheap(1);
        return CHUNK_TO_PAYLOAD(new_base);
    }

    // 兜底 (malloc + memcpy + free)
    void *newptr = mm_malloc(size);
    if (newptr == NULL) return NULL;
    
    memcpy(newptr, ptr, old_payload_size);
    mm_free(ptr);
    mm_checkheap(1);
    
    return newptr;
}

int mm_checkheap(int verbose)
{
    return 0; // 默认跳过检查以保证速度，开启调试可将其注释
}