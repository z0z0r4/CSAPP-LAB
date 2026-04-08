#include <time.h>
#include <stdlib.h>
#include <stdio.h>

void merge(long src1[], long src2[], long dest[], int n)
{
    int i = 0, j = 0, k = 0;
    while (i < n && j < n)
    {
        if (src1[i] < src2[j])
        {
            dest[k++] = src1[i++];
        }
        else
        {
            dest[k++] = src2[j++];
        }
    }
    while (i < n)
    {
        dest[k++] = src1[i++];
    }
    while (j < n)
    {
        dest[k++] = src2[j++];
    }
}

void mergesort(long arr[], int n)
{
    if (n < 2)
        return;

    int mid = n / 2;
    long *left = (long *)malloc(mid * sizeof(long));
    long *right = (long *)malloc((n - mid) * sizeof(long));

    for (int i = 0; i < mid; ++i)
    {
        left[i] = arr[i];
    }
    for (int i = mid; i < n; ++i)
    {
        right[i - mid] = arr[i];
    }

    mergesort(left, mid);
    mergesort(right, n - mid);
    merge(left, right, arr, mid);

    free(left);
    free(right);
}

void test1merge(long src1[], long src2[], long dest[], int n)
{
    int i = 0, j = 0, k = 0;
    while (i < n && j < n)
    {
        if (src1[i] < src2[j])
        {
            dest[k++] = src1[i++];
        }
        else
        {
            dest[k++] = src2[j++];
        }
    }
    while (i < n)
    {
        dest[k++] = src1[i++];
    }
    while (j < n)
    {
        dest[k++] = src2[j++];
    }
}

void test2merge(long src1[], long src2[], long dest[], int n)
{
    int i = 0, j = 0, k = 0;
    while (i < n && j < n)
    {
        // if (src1[i] < src2[j]) {
        //     dest[k++] = src1[i++];
        // } else {
        //     dest[k++] = src2[j++];
        // }
        dest[k++] = (src1[i] < src2[j]) ? src1[i++] : src2[j++];
    }
    while (i < n)
    {
        dest[k++] = src1[i++];
    }
    while (j < n)
    {
        dest[k++] = src2[j++];
    }
}

int main()
{
    const int n = 10000000;
    long *src1 = (long *)malloc(n * sizeof(long));
    long *src2 = (long *)malloc(n * sizeof(long));
    long *dest = (long *)malloc(2 * n * sizeof(long));

    // Initialize src1 and src2
    for (int i = 0; i < n; ++i)
    {
        src1[i] = rand();
        src2[i] = rand();
    }

    printf("Sorting source arrays...\n");

    mergesort(src1, n);
    mergesort(src2, n);

    printf("Starting test1merge...\n");
    int start_time = clock();
    test1merge(src1, src2, dest, n);
    int end_time = clock();

    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Elapsed time: %.6f seconds\n", elapsed_time);

    printf("Starting test2merge...\n");
    start_time = clock();
    test2merge(src1, src2, dest, n);
    end_time = clock();

    elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Elapsed time: %.6f seconds\n", elapsed_time);

    // Clean up
    free(src1);
    free(src2);
    free(dest);

    return 0;
}