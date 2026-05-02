// /*
//  * mm-naive.c - The fastest, least memory-efficient malloc package.
//  *
//  * In this naive approach, a block is allocated by simply incrementing
//  * the brk pointer.  A block is pure payload. There are no headers or
//  * footers.  Blocks are never coalesced or reused. Realloc is
//  * implemented directly using mm_malloc and mm_free.
//  *
//  * NOTE TO STUDENTS: Replace this header comment with your own header
//  * comment that gives a high level description of your solution.
//  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <assert.h>
// #include <unistd.h>
// #include <string.h>

// #include "mm.h"
// #include "memlib.h"

// /*********************************************************
//  * NOTE TO STUDENTS: Before you do anything else, please
//  * provide your team information in the following struct.
//  ********************************************************/
// team_t team = {
//     /* Team name */
//     "LoL",
//     /* First member's full name */
//     "z0z0r4",
//     /* First member's email address */
//     "z0z0r4@outlook.com",
//     /* Second member's full name (leave blank if none) */
//     "",
//     /* Second member's email address (leave blank if none) */
//     ""};

// /* single word (4) or double word (8) alignment */
// #define ALIGNMENT 8

// /* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// #define BUCKET_SIZE 20
// // #define BUCKET_STEP 32

// #define CHUNK_HEADER_SIZE 8
// #define CHUNK_FOOTER_SIZE 8

// #define CHUNK_NEXT_FREE_P_SIZE 8
// #define CHUNK_PREV_FREE_P_SIZE 8

// #define GET(p) (*(size_t *)(p))
// #define PUT(p, val) (*(size_t *)(p) = (val))

// #define GET_PTR(p) (*(void **)(p))
// #define PUT_PTR(p, val) (*(void **)(p) = (void *)(val))

// #define PACK(size, alloc, prev_alloc) ((size) | (alloc) | (prev_alloc << 1))

// #define PAYLOAD_TO_CHUNK(bp) ((char *)(bp) - CHUNK_HEADER_SIZE)

// #define CHUNK_TO_PAYLOAD(cp) ((char *)(cp) + CHUNK_HEADER_SIZE)

// #define GET_SIZE(p) (GET(p) & ~0x7)

// #define GET_ALLOC(p) (GET(p) & 0x1)

// #define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

// // chunk: [size|allocated][next_free_chunk_p][prev_free_chunk_p]
// #define GET_NEXT_FREE_CHUNK(p) ((void *)GET_PTR((char *)p + CHUNK_HEADER_SIZE))

// #define GET_PREV_FREE_CHUNK(p) ((void *)GET_PTR((char *)p + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE))

// #define SMALLEST_CHUNK_SIZE (CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE + CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE)
// // #define SMALLEST_CHUNK_SIZE (CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE)

// // static void *sentinel_chunk_ptr = NULL;

// // static void *rover = NULL;

// // 32 * 128 = 4096
// static void *bucket[BUCKET_SIZE];

// int mm_checkheap(int verbose);
// void mm_display_layout();

// // linear
// // int get_slot_index(size_t size) {
// //     int slot_index = size / BUCKET_STEP;
// //     if (slot_index >= BUCKET_SIZE)
// //     {
// //         slot_index = BUCKET_SIZE - 1;
// //     }
// //     return slot_index;
// // }

// // 2 pow
// int get_slot_index(size_t size)
// {
//     int class_index = 0;
//     size_t class_size = 32; // 从最小块大小开始

//     while (class_index < BUCKET_SIZE - 1 && size > class_size)
//     {
//         class_size <<= 1;
//         class_index++;
//     }
//     return class_index;
// }

// void *get_slot_ptr(size_t size)
// {
//     int slot_index = get_slot_index(size);
//     return bucket[slot_index];
// }

// void set_current_chunk_prev_alloc(void *ptr, int flag)
// {
//     *(size_t *)ptr = (GET(ptr) & ~((size_t)1 << 1)) | ((size_t)(flag) << 1);
// }

// void set_current_chunk_alloc(void *ptr, int flag)
// {
//     *(size_t *)ptr = GET(ptr) & ~(size_t)1 | (size_t)(flag);
// }

// void set_next_chunk_prev_alloc(void *ptr, int flag)
// {
//     void *next_chunk_ptr = (char *)ptr + GET_SIZE(ptr);
//     if (next_chunk_ptr <= mem_heap_hi())
//     {
//         set_current_chunk_prev_alloc(next_chunk_ptr, flag);
//     }
// }

// void set_chunk_size(void *ptr, size_t size)
// {
//     size_t back_3_bit = *(size_t *)ptr & 0x7;
//     PUT(ptr, size);
//     *(size_t *)ptr = *(size_t *)ptr | back_3_bit;
// }

// void copy_header_to_footer(void *ptr)
// {
//     // copy header to footer
//     size_t new_free_chunk_size = GET_SIZE(ptr);
//     void *new_free_chunk_footer_ptr = (char *)ptr + new_free_chunk_size - CHUNK_FOOTER_SIZE;
//     PUT(new_free_chunk_footer_ptr, GET(ptr));
// }

// void set_chunk_meta(void *ptr, size_t chunk_size, int allocated_flag, int prev_allocated_flag)
// {
//     // set header
//     PUT(ptr, PACK(chunk_size, allocated_flag, prev_allocated_flag));

//     copy_header_to_footer(ptr);
// }

// void set_chunk_next_p(void *ptr, void *next_ptr)
// {
//     PUT_PTR((char *)ptr + CHUNK_HEADER_SIZE, next_ptr);
// }

// void set_chunk_prev_p(void *ptr, void *prev_ptr)
// {
//     PUT_PTR((char *)ptr + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE, prev_ptr);
// }

// void *get_new_chunk_from_heap(size_t new_size)
// {
//     void *p = mem_sbrk(new_size);
//     if (p == (void *)-1)
//         return NULL;
//     else
//     {
//         return p;
//     }
// }

// // first-fit
// void *find_free_chunk(size_t chunk_size, void *sentinel_chunk_ptr)
// {
//     if (sentinel_chunk_ptr == NULL)
//     {
//         return NULL;
//     }

//     void *ptr = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
//     while (ptr != sentinel_chunk_ptr)
//     {
//         size_t current_chunk_size = GET_SIZE(ptr);
//         if (current_chunk_size >= chunk_size && !GET_ALLOC(ptr))
//         {
//             return ptr;
//         }
//         ptr = GET_NEXT_FREE_CHUNK(ptr);
//     }

//     return NULL;
// }

// // bucket first-fit
// void *find_free_chunk_in_bucket(size_t chunk_size)
// {
//     int slot_index = get_slot_index(chunk_size);
//     void *result_ptr = NULL;
//     while (slot_index < BUCKET_SIZE && result_ptr == NULL)
//     {
//         void *sentinel_chunk_ptr = bucket[slot_index];
//         result_ptr = find_free_chunk(chunk_size, sentinel_chunk_ptr);
//         slot_index++;
//     }
//     return result_ptr;
// }

// // // next-fit
// // void *find_free_chunk(size_t chunk_size, void *sentinel_chunk_ptr, void *rover)
// // {
// //     if (sentinel_chunk_ptr == NULL)
// //     {
// //         return NULL;
// //     }
// //
// //     if (rover == NULL || rover == sentinel_chunk_ptr)
// //     {
// //         rover = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
// //     }
// //
// //     void *start_ptr = rover;
// //
// //     void *ptr = rover;
// //     do
// //     {
// //         size_t current_chunk_size = GET_SIZE(ptr);
// //         if (current_chunk_size >= chunk_size && !GET_ALLOC(ptr))
// //         {
// //             rover = GET_NEXT_FREE_CHUNK(ptr);
// //             return ptr;
// //         }
// //         ptr = GET_NEXT_FREE_CHUNK(ptr);
// //     } while (ptr != start_ptr);
// //
// //     return NULL;
// // }

// // // best-fit
// // void *find_free_chunk(size_t chunk_size)
// // {
// //     if (sentinel_chunk_ptr == NULL)
// //     {
// //         return NULL;
// //     }
// //
// //     size_t best_fit_size = -1;
// //     void *best_fit_chunk_ptr = NULL;
// //     void *ptr = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
// //     while (ptr != sentinel_chunk_ptr)
// //     {
// //         size_t current_chunk_size = GET_SIZE(ptr);
// //         if (current_chunk_size >= chunk_size && current_chunk_size - chunk_size < best_fit_size - chunk_size && !GET_ALLOC(ptr))
// //         {
// //             best_fit_size = current_chunk_size;
// //             best_fit_chunk_ptr = ptr;
// //             if (best_fit_size == chunk_size) {
// //                 break;
// //             }
// //         }
// //         ptr = GET_NEXT_FREE_CHUNK(ptr);
// //     }
// //
// //     if (best_fit_chunk_ptr == sentinel_chunk_ptr)
// //     {
// //         return NULL;
// //     }
// //
// //     return best_fit_chunk_ptr;
// // }

// void insert_chunk_to_free_linked_list(void *ptr, void *sentinel_chunk_ptr)
// {
//     void *sentinel_chunk_next_chunk_ptr = GET_NEXT_FREE_CHUNK(sentinel_chunk_ptr);
//     // set self
//     set_chunk_next_p(ptr, sentinel_chunk_next_chunk_ptr);
//     set_chunk_prev_p(ptr, sentinel_chunk_ptr);

//     // set sentinel_chunk
//     set_chunk_next_p(sentinel_chunk_ptr, ptr);

//     // set sentinel_chunk next
//     set_chunk_prev_p(sentinel_chunk_next_chunk_ptr, ptr);
// }

// void insert_chunk_to_bucket(void *ptr)
// {
//     size_t chunk_size = GET_SIZE(ptr);
//     insert_chunk_to_free_linked_list(ptr, get_slot_ptr(chunk_size));
// }

// void remove_chunk_from_free_linked_list(void *ptr)
// {
//     // if (rover == ptr)
//     // {
//     //     rover = GET_NEXT_FREE_CHUNK(ptr);
//     // }

//     void *prev_chunk_ptr = GET_PREV_FREE_CHUNK(ptr);
//     void *next_chunk_ptr = GET_NEXT_FREE_CHUNK(ptr);

//     // set prev chunk next p
//     set_chunk_next_p(prev_chunk_ptr, next_chunk_ptr);

//     // set next chunk prev p
//     set_chunk_prev_p(next_chunk_ptr, prev_chunk_ptr);
// }

// // void separate_free_chunk(void *ptr, size_t current_chunk_size, size_t new_chunk_size)
// // {
// //     if (current_chunk_size >= new_chunk_size + SMALLEST_CHUNK_SIZE)
// //     {
// //         int original_prev_alloc = GET_PREV_ALLOC(ptr);
// //         // set_chunk_meta(ptr, new_chunk_size, 0, 1);
// //         set_chunk_size(ptr, new_chunk_size);
// //         set_current_chunk_alloc(ptr, 0);
// //         void *new_chunk_ptr = (char *)ptr + new_chunk_size;
// //         size_t remaining_chunk_size = current_chunk_size - new_chunk_size;
// //         set_chunk_meta(new_chunk_ptr, remaining_chunk_size, 0, 1);
// //         insert_chunk_to_bucket(new_chunk_ptr);
// //     }
// // }

// /**
//  * 切割 chunk
//  **/
// void *separate_chunk(void *ptr, size_t current_chunk_size, size_t new_chunk_size)
// {
//     if (current_chunk_size >= new_chunk_size + SMALLEST_CHUNK_SIZE)
//     {
//         // 仅更新 size
//         set_chunk_size(ptr, new_chunk_size);

//         // 后半部分（新切出来的空闲块）的位置和大小
//         void *remaining_chunk_ptr = (char *)ptr + new_chunk_size;
//         size_t remaining_chunk_size = current_chunk_size - new_chunk_size;

//         // 它的 prev_alloc 必然等于前半部分现在的 alloc 状态
//         set_chunk_meta(remaining_chunk_ptr, remaining_chunk_size, 0, GET_ALLOC(ptr));

//         return remaining_chunk_ptr;
//     }
//     return NULL;
// }

// /**
//  * 针对空闲块的切割：切割 + 插入空闲链表
//  */
// void separate_free_chunk(void *ptr, size_t current_chunk_size, size_t new_chunk_size)
// {
//     void *remaining_chunk_ptr = separate_chunk(ptr, current_chunk_size, new_chunk_size);
//     if (remaining_chunk_ptr != NULL)
//     {
//         insert_chunk_to_bucket(remaining_chunk_ptr);
//     }
// }

// void *create_sentinel_chunk()
// {
//     size_t size = SMALLEST_CHUNK_SIZE;
//     void *ptr = get_new_chunk_from_heap(size);
//     set_chunk_meta(ptr, size, 1, 1);
//     set_chunk_next_p(ptr, ptr);
//     set_chunk_prev_p(ptr, ptr);
//     return ptr;
// }

// void *create_epilogue_chunk()
// {
//     size_t size = CHUNK_HEADER_SIZE;
//     void *ptr = mem_sbrk(size);
//     PUT(ptr, PACK(0, 1, 1));
//     return ptr;
// }

// /*
//  * mm_init - initialize the malloc package.
//  */
// int mm_init(void)
// {
//     for (int i = 0; i < BUCKET_SIZE; i++)
//     {
//         bucket[i] = create_sentinel_chunk();
//     }
//     create_epilogue_chunk();
//     return 0;
// }
// int get_last_chunk_alloc()
// {
//     return GET_PREV_ALLOC(mem_heap_hi() - CHUNK_HEADER_SIZE + 1);
// }

// void set_last_chunk_alloc(int flag)
// {
//     PUT(mem_heap_hi() - CHUNK_HEADER_SIZE + 1, PACK(0, 1, flag));
// }

// /*
//  * mm_malloc - Allocate a block by incrementing the brk pointer.
//  *     Always allocate a block whose size is a multiple of the alignment.
//  */
// void *mm_malloc(size_t size)
// {
//     mm_checkheap(1);
//     size_t new_size;
//     // 确保可以塞入两个指针
//     if (size >= (CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE + CHUNK_FOOTER_SIZE))
//     {
//         new_size = ALIGN(size + CHUNK_HEADER_SIZE);
//     }
//     else
//     {
//         new_size = SMALLEST_CHUNK_SIZE;
//     }

//     void *ptr = find_free_chunk_in_bucket(new_size);
//     if (ptr == NULL)
//     // 没找到 free_chunk, 直接分配新的
//     {
//         // 必须在 mem_sbrk 之前获取 last_alloc_flag
//         int last_alloc_flag = get_last_chunk_alloc();
//         ptr = get_new_chunk_from_heap(new_size);
//         if (ptr == NULL)
//         {
//             return NULL;
//         }
//         else
//         {
//             // move epilogue chunk
//             ptr = ptr - CHUNK_HEADER_SIZE;
//             PUT(ptr + new_size, PACK(0, 1, 1));
//             set_chunk_meta(ptr, new_size, 1, last_alloc_flag);
//         }
//     }
//     else
//     // 找到了 free_chunk
//     {
//         size_t current_chunk_size = GET_SIZE(ptr);
//         // if the remaining free chunk is big enough to hold the meta info and at least 1 bytes, then separate the chunk
//         remove_chunk_from_free_linked_list(ptr);
//         if (current_chunk_size >= new_size + SMALLEST_CHUNK_SIZE)
//         {
//             // 拆分 free_chunk
//             set_current_chunk_alloc(ptr, 1);
//             separate_free_chunk(ptr, current_chunk_size, new_size);
//             // set_chunk_meta(ptr, new_size, 1);
//             // set_chunk_size(ptr, new_size);
//             // set_next_chunk_prev_alloc(ptr, 1);
//             // 切分后的新的 free_chunk 的下一个 chunk 的 prev_alloc 保持不用改
//         }
//         else
//         {
//             // 直接分配
//             // set_chunk_meta(ptr, current_chunk_size, 1);
//             set_current_chunk_alloc(ptr, 1);
//             // 修改下一个  chunk 的 prev_alloc
//             set_next_chunk_prev_alloc(ptr, 1);
//         }
//     }

//     mm_checkheap(1);
//     return CHUNK_TO_PAYLOAD(ptr);
// }

// /*
//  * mm_free - Freeing a block does nothing.
//  */
// void mm_free(void *ptr)
// {
//     mm_checkheap(1);
//     void *chunk_ptr = PAYLOAD_TO_CHUNK(ptr);
//     size_t chunk_size = GET_SIZE(chunk_ptr);
//     size_t prev_chunk_size = 0;
//     size_t next_chunk_size = 0;

//     int prev_chunk_free_flag = 0;
//     int next_chunk_free_flag = 0;

//     // nearby chunks
//     void *next_chunk_ptr = (char *)chunk_ptr + chunk_size;
//     void *prev_chunk_footer_ptr = (char *)chunk_ptr - CHUNK_FOOTER_SIZE;
//     void *prev_chunk_ptr = NULL;

//     void *new_free_chunk_ptr = NULL;

//     if (!GET_PREV_ALLOC(chunk_ptr))
//     {
//         prev_chunk_free_flag = 1;
//         prev_chunk_size = GET_SIZE(prev_chunk_footer_ptr);
//         prev_chunk_ptr = (char *)chunk_ptr - prev_chunk_size;
//     }

//     // if (prev_chunk_ptr >= mem_heap_lo())
//     // {
//     //     prev_chunk_free_flag = !GET_PREV_ALLOC(chunk_ptr);
//     //     if (prev_chunk_free_flag)
//     //     {
//     //         prev_chunk_ptr = (char *)chunk_ptr - GET_SIZE(prev_chunk_footer_ptr);
//     //         prev_chunk_size = GET_SIZE(prev_chunk_ptr);
//     //     }
//     // }

//     if (next_chunk_ptr < mem_heap_hi())
//     {
//         next_chunk_free_flag = !GET_ALLOC(next_chunk_ptr);
//         next_chunk_size = GET_SIZE(next_chunk_ptr);
//     }

//     if (!prev_chunk_free_flag && !next_chunk_free_flag)
//     {
//         // just set current chunk to free
//         // set_chunk_meta(chunk_ptr, chunk_size, 0);
//         set_current_chunk_alloc(chunk_ptr, 0);
//         set_current_chunk_prev_alloc(chunk_ptr, 1);
//         set_next_chunk_prev_alloc(chunk_ptr, 0);
//         new_free_chunk_ptr = chunk_ptr;
//     }
//     else if (prev_chunk_free_flag && !next_chunk_free_flag)
//     {
//         // merge with previous
//         size_t merged_chunk_size = chunk_size + prev_chunk_size;
//         // set_chunk_meta(prev_chunk_ptr, merged_chunk_size, 0);
//         set_chunk_size(prev_chunk_ptr, merged_chunk_size);
//         set_current_chunk_alloc(prev_chunk_ptr, 0);
//         remove_chunk_from_free_linked_list(prev_chunk_ptr);
//         set_current_chunk_prev_alloc(prev_chunk_ptr, GET_PREV_ALLOC(prev_chunk_ptr));
//         set_next_chunk_prev_alloc(prev_chunk_ptr, 0);
//         new_free_chunk_ptr = prev_chunk_ptr;
//     }
//     else if (!prev_chunk_free_flag && next_chunk_free_flag)
//     {
//         // merge with next
//         size_t merged_chunk_size = chunk_size + next_chunk_size;
//         // set_chunk_meta(chunk_ptr, merged_chunk_size, 0);
//         set_chunk_size(chunk_ptr, merged_chunk_size);
//         set_current_chunk_alloc(chunk_ptr, 0);
//         remove_chunk_from_free_linked_list(next_chunk_ptr);
//         set_current_chunk_prev_alloc(chunk_ptr, 1);
//         set_next_chunk_prev_alloc(chunk_ptr, 0);
//         new_free_chunk_ptr = chunk_ptr;
//     }
//     else
//     {
//         // merge with both
//         size_t merged_chunk_size = chunk_size + prev_chunk_size + next_chunk_size;
//         // set_chunk_meta(prev_chunk_ptr, merged_chunk_size, 0);
//         int prev_chunk_prev_alloc = GET_PREV_ALLOC(prev_chunk_ptr);
//         set_current_chunk_prev_alloc(prev_chunk_ptr, 0);
//         set_chunk_size(prev_chunk_ptr, merged_chunk_size);
//         remove_chunk_from_free_linked_list(prev_chunk_ptr);
//         remove_chunk_from_free_linked_list(next_chunk_ptr);
//         set_current_chunk_prev_alloc(prev_chunk_ptr, prev_chunk_prev_alloc);
//         set_next_chunk_prev_alloc(prev_chunk_ptr, 0);
//         new_free_chunk_ptr = prev_chunk_ptr;
//     }

//     if (new_free_chunk_ptr != NULL)
//     {
//         insert_chunk_to_bucket(new_free_chunk_ptr);
//         copy_header_to_footer(new_free_chunk_ptr);
//     }
//     mm_checkheap(1);
// }

// /*
//  * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
//  */
// void *mm_realloc(void *ptr, size_t size)
// {
//     void *oldptr = PAYLOAD_TO_CHUNK(ptr);
//     void *newptr;
//     size_t copySize;
//     size_t old_size = GET_SIZE(oldptr);
//     size_t old_payload_size = old_size - CHUNK_HEADER_SIZE;

//     size_t req_chunk_size = ALIGN(size + CHUNK_HEADER_SIZE);

//     // big chunk -> small chunk, in-place shrink
//     if (req_chunk_size <= old_size)
//     {
//         // if (old_size >= req_chunk_size + SMALLEST_CHUNK_SIZE)
//         // {
//         //     separate_free_chunk(oldptr, old_size, req_chunk_size);
//         //     // set_chunk_meta(oldptr, req_chunk_size, 1);
//         //     set_chunk_size(oldptr, req_chunk_size);
//         //     set_current_chunk_prev_alloc(oldptr, 1);
//         //     set_next_chunk_prev_alloc(oldptr, 1);
//         //     void *remained_free_chunk_ptr = (char *)oldptr + req_chunk_size;
//         //     set_current_chunk_alloc(remained_free_chunk_ptr, 0);
//         //     set_current_chunk_prev_alloc(remained_free_chunk_ptr, 1);
//         //     set_next_chunk_prev_alloc(remained_free_chunk_ptr, 0);
//         //     copy_header_to_footer(remained_free_chunk_ptr);
//         // }

//         // 尝试切割
//         void *remaining_chunk_ptr = separate_chunk(oldptr, old_size, req_chunk_size);

//         if (remaining_chunk_ptr != NULL)
//         {
//             // 如果切割成功，将剩余部分放入空闲链表
//             // insert_chunk_to_bucket(remaining_chunk_ptr);
//             // 必须修改紧跟在剩余块后面的那个块
//             // set_next_chunk_prev_alloc(remaining_chunk_ptr, 0);

//             // 假装已分配让 mm_free 处理，因为缩容前的下一个 chunk 可能是 free chunk
//             // 所以 remaining_chunk 可能下一个 chunk 也是 free，可以合并
//             set_current_chunk_alloc(remaining_chunk_ptr, 1);
//             set_next_chunk_prev_alloc(remaining_chunk_ptr, 1);
//             mm_free(CHUNK_TO_PAYLOAD(remaining_chunk_ptr));
//         }
//         // mm_checkheap(0);
//         return CHUNK_TO_PAYLOAD(oldptr);
//     }

//     // 有下一个块且下一个块是空闲的
//     if ((void *)((char *)oldptr + old_size) < mem_heap_hi() && !GET_ALLOC((char *)oldptr + old_size))
//     {
//         void *next_free_chunk_ptr = (char *)oldptr + old_size;
//         size_t next_free_chunk_size = GET_SIZE(next_free_chunk_ptr);
//         size_t merged_chunk_size = old_size + next_free_chunk_size;

//         // 如果当前块和下一个空闲块合并后足够大，在原地扩展
//         if (old_size + next_free_chunk_size >= req_chunk_size)
//         {
//             // merge with next free chunk
//             remove_chunk_from_free_linked_list(next_free_chunk_ptr);

//             // // 如果合并后的块足够大，可以分割出一个新的空闲块
//             // if (merged_chunk_size >= req_chunk_size + SMALLEST_CHUNK_SIZE)
//             // {
//             //     // separate the merged chunk into new allocated chunk and free chunk
//             //     // set_chunk_meta(oldptr, req_chunk_size, 1);
//             //     void *new_chunk_ptr = (char *)oldptr + req_chunk_size;
//             //     separate_free_chunk(oldptr, merged_chunk_size, req_chunk_size);
//             //     set_chunk_size(oldptr, req_chunk_size);
//             //     set_current_chunk_alloc(oldptr, 1);
//             //     set_current_chunk_prev_alloc(oldptr, prev_alloc);
//             //     set_current_chunk_prev_alloc(new_chunk_ptr, 1);
//             //     set_next_chunk_prev_alloc(new_chunk_ptr, 0);
//             //     copy_header_to_footer(new_chunk_ptr);
//             // }
//             // else
//             // {
//             //     // set_chunk_meta(oldptr, merged_chunk_size, 1);
//             //     set_chunk_size(oldptr, merged_chunk_size);
//             //     set_current_chunk_alloc(oldptr, 1);
//             //     set_next_chunk_prev_alloc(oldptr, 1);
//             // }

//             // 暴力合并物理大小：
//             // 因为 oldptr 本身就是 allocated，我们只需将它的 size 扩大，
//             // 你的 set_chunk_size 函数会完美保留 oldptr 原本的 alloc 和 prev_alloc 状态！
//             set_chunk_size(oldptr, merged_chunk_size);

//             // 对这个合并后的大块尝试进行切割
//             void *remaining_chunk_ptr = separate_chunk(oldptr, merged_chunk_size, req_chunk_size);

//             if (remaining_chunk_ptr != NULL)
//             {
//                 // 成功切割出多余空间，放入空闲链表并更新后面的 prev_alloc
//                 insert_chunk_to_bucket(remaining_chunk_ptr);
//                 set_next_chunk_prev_alloc(remaining_chunk_ptr, 0);
//             }
//             else
//             {
//                 // 没有切割空间（完全吃掉了原来的空闲块），通知物理内存上位于合并块后方的块
//                 set_next_chunk_prev_alloc(oldptr, 1);
//             }

//             return CHUNK_TO_PAYLOAD(oldptr);
//         }
//     }

//     newptr = mm_malloc(size);
//     if (newptr == NULL)
//         return NULL;
//     copySize = old_payload_size;
//     if (size < copySize)
//         copySize = size;
//     memcpy(newptr, CHUNK_TO_PAYLOAD(oldptr), copySize);
//     mm_free(CHUNK_TO_PAYLOAD(oldptr));
//     return newptr;
// }


/*
 * mm-naive.c - Optimized with Smallbins and Best-Fit Segregated List.
 *
 * 优化说明:
 * 1. 采用分离空闲链表 (Segregated Free List) 维护空闲内存块。
 * 2. 对小内存块 (<= 88 bytes) 采用精确大小分类桶 (Smallbins)，步长为 8。
 * 3. 对大内存块采用 2的幂次分类桶 (Largebins)，并在桶内使用 Best-Fit 策略查找。
 * 4. 优化了边界标记 (Boundary Tags)，仅空闲块包含 Footer，提升 Payload 比例。
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

#define BUCKET_SIZE 24

#define CHUNK_HEADER_SIZE 8
#define CHUNK_FOOTER_SIZE 8

#define CHUNK_NEXT_FREE_P_SIZE 8
#define CHUNK_PREV_FREE_P_SIZE 8

#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

#define GET_PTR(p) (*(void **)(p))
#define PUT_PTR(p, val) (*(void **)(p) = (void *)(val))

#define PACK(size, alloc, prev_alloc) ((size) | (alloc) | (prev_alloc << 1))

#define PAYLOAD_TO_CHUNK(bp) ((char *)(bp) - CHUNK_HEADER_SIZE)
#define CHUNK_TO_PAYLOAD(cp) ((char *)(cp) + CHUNK_HEADER_SIZE)

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

// chunk: [size|allocated][next_free_chunk_p][prev_free_chunk_p]
#define GET_NEXT_FREE_CHUNK(p) ((void *)GET_PTR((char *)p + CHUNK_HEADER_SIZE))
#define GET_PREV_FREE_CHUNK(p) ((void *)GET_PTR((char *)p + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE))

#define SMALLEST_CHUNK_SIZE (CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE + CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE)

#define PRE_EXTEND_SIZE 4096

static void *bucket[BUCKET_SIZE];

int mm_checkheap(int verbose);
void mm_display_layout();

/**
 * 混合桶索引计算 (Smallbins + Largebins)
 */
int get_slot_index(size_t size)
{
    // 针对小块内存：步长为 8 的精确桶 (Smallbins)
    // 从最小块大小 32 开始：32, 40, 48, 56, 64, 72, 80, 88，总共 8 个 slot
    if (size <= 88)
    {
        return (size - 32) / 8;
    }

    // 针对大块内存：按 2 的幂次划分桶 (Largebins)
    int class_index = 8;
    size_t class_size = 128; // 从 128 字节开始指数递增

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
    *(size_t *)ptr = (GET(ptr) & ~((size_t)1 << 1)) | ((size_t)(flag) << 1);
}

void set_current_chunk_alloc(void *ptr, int flag)
{
    *(size_t *)ptr = GET(ptr) & ~(size_t)1 | (size_t)(flag);
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
    size_t back_3_bit = *(size_t *)ptr & 0x7;
    PUT(ptr, size);
    *(size_t *)ptr = *(size_t *)ptr | back_3_bit;
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
    copy_header_to_footer(ptr);
}

void set_chunk_next_p(void *ptr, void *next_ptr)
{
    PUT_PTR((char *)ptr + CHUNK_HEADER_SIZE, next_ptr);
}

void set_chunk_prev_p(void *ptr, void *prev_ptr)
{
    PUT_PTR((char *)ptr + CHUNK_HEADER_SIZE + CHUNK_NEXT_FREE_P_SIZE, prev_ptr);
}

int get_last_chunk_alloc()
{
    return GET_PREV_ALLOC(mem_heap_hi() - CHUNK_HEADER_SIZE + 1);
}

int is_nearby_epilogue_chunk(void *ptr)
{
    size_t size = GET_SIZE(ptr);
    void *epilogue_chunk_ptr = mem_heap_hi() - CHUNK_HEADER_SIZE + 1;
    return (char *)ptr + size == epilogue_chunk_ptr;
}

void set_epilogue_chunk(int prev_alloc)
{
    void *epilogue_ptr = mem_heap_hi() - CHUNK_HEADER_SIZE + 1;
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
        // 保证空闲且大小足够
        if (!GET_ALLOC(ptr) && current_chunk_size >= chunk_size)
        {
            // 如果比目前的最优解更好，则更新
            if (current_chunk_size - chunk_size < best_fit_size - chunk_size)
            {
                best_fit_size = current_chunk_size;
                best_fit_chunk_ptr = ptr;

                // 如果大小完美匹配，直接返回（无需继续寻找，提前结束）
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
    // 从匹配的最小桶开始往上找，只要找到符合要求的 Best-Fit 就立刻返回
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
    for (int i = 0; i < BUCKET_SIZE; i++)
    {
        bucket[i] = create_sentinel_chunk();
    }
    create_epilogue_chunk();
    return 0;
}

// /*
//  * mm_malloc
//  */
// void *mm_malloc(size_t size)
// {
//     if (size == 0)
//         return NULL;

//     size_t new_size;
//     if (size >= (CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE + CHUNK_FOOTER_SIZE))
//     {
//         new_size = ALIGN(size + CHUNK_HEADER_SIZE);
//     }
//     else
//     {
//         new_size = SMALLEST_CHUNK_SIZE;
//     }

//     void *ptr = find_free_chunk_in_bucket(new_size);

//     if (ptr == NULL)
//     {
//         int last_alloc_flag = get_last_chunk_alloc();
//         ptr = get_new_chunk_from_heap(new_size);
//         if (ptr == NULL)
//             return NULL;

//         ptr = (char *)ptr - CHUNK_HEADER_SIZE;
//         PUT((char *)ptr + new_size, PACK(0, 1, 1)); // 重新设置 Epilogue
//         set_chunk_meta(ptr, new_size, 1, last_alloc_flag);
//     }
//     else
//     {
//         size_t current_chunk_size = GET_SIZE(ptr);
//         remove_chunk_from_free_linked_list(ptr);

//         if (current_chunk_size >= new_size + SMALLEST_CHUNK_SIZE)
//         {
//             set_current_chunk_alloc(ptr, 1);
//             separate_free_chunk(ptr, current_chunk_size, new_size);
//         }
//         else
//         {
//             set_current_chunk_alloc(ptr, 1);
//             set_next_chunk_prev_alloc(ptr, 1);
//         }
//     }
//     return CHUNK_TO_PAYLOAD(ptr);
// }

void *mm_malloc(size_t size)
{
    mm_checkheap(1);
    if (size == 0)
        return NULL;

    size_t new_size;
    if (size >= (CHUNK_NEXT_FREE_P_SIZE + CHUNK_PREV_FREE_P_SIZE + CHUNK_FOOTER_SIZE))
    {
        new_size = ALIGN(size + CHUNK_HEADER_SIZE);
    }
    else
    {
        new_size = SMALLEST_CHUNK_SIZE;
    }

    void *ptr = find_free_chunk_in_bucket(new_size);

    if (ptr == NULL)
    {
        // 预分配大块，便于优化 binary-bal.rep
        size_t extend_size = new_size > PRE_EXTEND_SIZE ? new_size : PRE_EXTEND_SIZE;
        int last_alloc_flag = get_last_chunk_alloc();
        
        ptr = get_new_chunk_from_heap(extend_size);
        if (ptr == NULL)
            return NULL;

        // 新申请的 ptr 前还有旧的 epilogue_chunk
        ptr = (char *)ptr - CHUNK_HEADER_SIZE;
        // 设置新的 epilogue_chunk
        set_epilogue_chunk(1);
        
        // 设置整个新块为空闲块
        set_chunk_meta(ptr, extend_size, 0, last_alloc_flag); 
    }
    else
    {
        remove_chunk_from_free_linked_list(ptr);
    }

    // 拿到空闲块，小块左大块右
    size_t current_chunk_size = GET_SIZE(ptr);
    void *allocated_ptr = ptr; // 最终要返回的已分配指针

    if (current_chunk_size >= new_size + SMALLEST_CHUNK_SIZE)
    {
        size_t remaining_chunk_size = current_chunk_size - new_size;

        // 96 区分小块和大块
        if (new_size <= 96) 
        {
            // 小块切左边
            allocated_ptr = ptr;
            set_chunk_meta(allocated_ptr, new_size, 1, GET_PREV_ALLOC(ptr));
            
            // 剩下的右边设为空闲块
            void *remaining_ptr = (char *)allocated_ptr + new_size;
            set_chunk_meta(remaining_ptr, remaining_chunk_size, 0, 1);
            insert_chunk_to_bucket(remaining_ptr);
        }
        else
        {
            // 大块切右边
            // 左边设为空闲块
            void *remaining_ptr = ptr;
            set_chunk_meta(remaining_ptr, remaining_chunk_size, 0, GET_PREV_ALLOC(ptr));
            insert_chunk_to_bucket(remaining_ptr);
            
            // 设置 allocated_ptr
            allocated_ptr = (char *)ptr + remaining_chunk_size;

            set_chunk_meta(allocated_ptr, new_size, 1, 0); 
            set_next_chunk_prev_alloc(allocated_ptr, 1);
        }
    }
    else
    {
        // 空间不够切分，直接整个吃掉
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
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *oldptr = PAYLOAD_TO_CHUNK(ptr);
    size_t old_size = GET_SIZE(oldptr);
    size_t old_payload_size = old_size - CHUNK_HEADER_SIZE;
    size_t req_chunk_size = ALIGN(size + CHUNK_HEADER_SIZE);

    // 原地缩容
    if (req_chunk_size <= old_size)
    {
        void *remaining_chunk_ptr = separate_chunk(oldptr, old_size, req_chunk_size);
        if (remaining_chunk_ptr != NULL)
        {
            set_current_chunk_alloc(remaining_chunk_ptr, 1);
            set_next_chunk_prev_alloc(remaining_chunk_ptr, 1);
            mm_free(CHUNK_TO_PAYLOAD(remaining_chunk_ptr));
        }
        mm_checkheap(1);
        return CHUNK_TO_PAYLOAD(oldptr);
    }

    void *next_chunk_ptr = (char *)oldptr + old_size;
    // 原地向后扩容
    if (!GET_ALLOC(next_chunk_ptr)) // 下一个 chunk 空闲
    {
        size_t next_free_chunk_size = GET_SIZE(next_chunk_ptr);
        size_t merged_chunk_size = old_size + next_free_chunk_size;

        if (merged_chunk_size >= req_chunk_size)
        {
            remove_chunk_from_free_linked_list(next_chunk_ptr);
            set_chunk_size(oldptr, merged_chunk_size);

            void *remaining_chunk_ptr = separate_chunk(oldptr, merged_chunk_size, req_chunk_size);
            if (remaining_chunk_ptr != NULL)
            {
                insert_chunk_to_bucket(remaining_chunk_ptr);
                set_next_chunk_prev_alloc(remaining_chunk_ptr, 0);
            }
            else
            {
                set_next_chunk_prev_alloc(oldptr, 1);
            }
            mm_checkheap(1);
            return CHUNK_TO_PAYLOAD(oldptr);
        }
        else if (is_nearby_epilogue_chunk(next_chunk_ptr))
        { // 虽然下一个 free_chunk 的空间不够，但是它位于末尾，直接扩容
            size_t more_size = req_chunk_size - merged_chunk_size;
            mem_sbrk(more_size);
            set_chunk_size(oldptr, req_chunk_size);
            // 记得移动 epilogue_chunk
            set_epilogue_chunk(1);
            mm_checkheap(1);
            return CHUNK_TO_PAYLOAD(oldptr);
        }
    }
    else if (is_nearby_epilogue_chunk(oldptr))
    { // 若原来的 chunk 位于末尾，直接扩容 mem_sbrk
        mm_checkheap(1);
        size_t more_size = req_chunk_size - old_size;
        mem_sbrk(more_size);
        set_chunk_size(oldptr, req_chunk_size);
        // 记得移动 epilogue_chunk
        set_epilogue_chunk(1);
        mm_checkheap(1);
        return CHUNK_TO_PAYLOAD(oldptr);
    }

    // 常规 realloc (malloc + memcpy + free)
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;

    size_t copySize = old_payload_size;
    if (size < copySize)
        copySize = size;
    memcpy(newptr, CHUNK_TO_PAYLOAD(oldptr), copySize);
    mm_free(CHUNK_TO_PAYLOAD(oldptr));
    mm_checkheap(1);
    return newptr;
}


int mm_checkheap(int verbose)
{
    return 0;
    void *bp;
    int prev_alloc_flag = 1;
    size_t implicit_free_count = 0;
    size_t explicit_free_count = 0;

    // ---------------------------------------------------------
    // 1. 隐式遍历：从 Sentinel Chunk 开始，按内存地址顺序遍历整个堆
    // ---------------------------------------------------------
    bp = bucket[0] + 32 * BUCKET_SIZE;

    void *epilogue_chunk_ptr = (char *)mem_heap_hi() - CHUNK_HEADER_SIZE + 1;

    // 假设 sentinel_chunk 是堆的最开头，遍历直到堆顶
    while (bp != NULL && bp < mem_heap_hi())
    {
        size_t size = GET_SIZE(bp);
        int alloc = GET_ALLOC(bp);

        // 检查1：块大小不能为0
        if (size == 0)
        {
            if (bp == epilogue_chunk_ptr)
            {
                if (get_last_chunk_alloc() != prev_alloc_flag)
                {
                    printf("Heap Error: Epilogue chunk prev alloc flag %d not equal to prev_alloc_flag %d\n", get_last_chunk_alloc(), prev_alloc_flag);
                }
                break;
            }
            else
            {

                printf("Heap Error: Chunk size is 0 at %p\n", bp);
                return 0;
            }
        }

        // 检查2：Header 和 Footer 是否匹配
        size_t header = GET(bp);
        if (!GET_ALLOC(bp))
        {
            void *footer_ptr = (char *)bp + size - CHUNK_FOOTER_SIZE;
            size_t footer = GET(footer_ptr);
            if (header != footer)
            {
                printf("Heap Error: Header and Footer mismatch at %p\n", bp);
                return 0;
            }
        }

        if (GET_PREV_ALLOC(bp) != prev_alloc_flag)
        {
            printf("Heap Error: Header prev alloc flag error at %p\n", bp);
            return 0;
        }

        // 统计真实的空闲块数量（跳过哨兵块）
        if (!alloc)
        {
            implicit_free_count++;
        }

        prev_alloc_flag = GET_ALLOC(bp);

        // 步进到下一个块
        bp = (char *)bp + size;
    }

    // ---------------------------------------------------------
    // 2. 显式遍历：沿着双向链表的 next 指针进行遍历
    // ---------------------------------------------------------
    for (int index = 0; index < BUCKET_SIZE; index++)
    {
        void *sentinel_chunk_ptr = bucket[index];
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