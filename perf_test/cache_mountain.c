#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXELEMS 10000000
#define ITERATIONS 100

long data[MAXELEMS];

void init_data() {
    for (int i = 0; i < MAXELEMS; i++) data[i] = i;
}

int test(int elems, int stride)
{
    int i = 0;
    long sx1 = stride;
    long sx2 = stride * 2;
    long sx3 = stride * 3;
    long sx4 = stride * 4;
    long acc1 = 0, acc2 = 0, acc3 = 0, acc4 = 0;

    for (i = 0; i <= elems - sx4; i += sx4)
    {
        acc1 += data[i];
        acc2 += data[i + sx1];
        acc3 += data[i + sx2];
        acc4 += data[i + sx3];
    }
    for (; i < elems; i += stride) {
        acc1 += data[i];
    }

    return (int)((acc1 + acc2) + (acc3 + acc4));
}

// 测量并打印吞吐量的函数
void run_test(int elems, int stride) {
    struct timespec start, end;
    
    // 预热缓存（Warm up）
    test(elems, stride);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ITERATIONS; i++) {
        test(elems, stride);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 计算总耗时（秒）
    double seconds = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // 计算读取的总字节数 (每个 long 占 8 字节)
    // 注意：这里的字节数要乘以迭代次数
    double actual_access_count = (double)elems / stride; 
    double total_bytes = actual_access_count * sizeof(long) * ITERATIONS;
    // double total_bytes = (double)elems * sizeof(long) * ITERATIONS;
    
    // 计算吞吐量 (MB/s)
    double throughput = (total_bytes / seconds) / 1e6;

    printf("%d,%d,%.2f\n", (int)(elems * sizeof(long) / 1024), stride, throughput);
}

int main() {
    init_data();

    printf("Starting Memory Mountain Test...\n");
    // 模拟存储器山的步进逻辑
    // 改变工作集大小 (Size)
    for (int elems = 1024; elems <= MAXELEMS; elems *= 2) {
        // 改变步长 (Stride)
        for (int stride = 1; stride <= 16; stride++) {
            run_test(elems, stride);
        }    }

    return 0;
}