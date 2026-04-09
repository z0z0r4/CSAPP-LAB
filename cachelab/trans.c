/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";

void transpose_block_8(int M, int N, int A[N][M], int B[M][N], int si, int sj, int block_length)
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    for (int i = si; i < si + block_length && i < N; i++)
    {
        a0 = A[i][sj];
        a1 = A[i][sj + 1];
        a2 = A[i][sj + 2];
        a3 = A[i][sj + 3];
        a4 = A[i][sj + 4];
        a5 = A[i][sj + 5];
        a6 = A[i][sj + 6];
        a7 = A[i][sj + 7];

        B[sj][i] = a0;
        B[sj + 1][i] = a1;
        B[sj + 2][i] = a2;
        B[sj + 3][i] = a3;
        B[sj + 4][i] = a4;
        B[sj + 5][i] = a5;
        B[sj + 6][i] = a6;
        B[sj + 7][i] = a7;
    }
}

void transpose_block_4(int M, int N, int A[N][M], int B[M][N], int si, int sj, int di, int dj, int block_length)
{
    int a0, a1, a2, a3;
    for (int offset_i = 0; offset_i < block_length && si + offset_i < N; offset_i++)
    {
        a0 = A[si + offset_i][sj];
        a1 = A[si + offset_i][sj + 1];
        a2 = A[si + offset_i][sj + 2];
        a3 = A[si + offset_i][sj + 3];

        B[dj][di + offset_i] = a0;
        B[dj + 1][di + offset_i] = a1;
        B[dj + 2][di + offset_i] = a2;
        B[dj + 3][di + offset_i] = a3;
    }
}

void transpose_block_8_for_64_64(int M, int N, int A[N][M], int B[M][N], int si, int sj)
{
    int a0, a1, a2, a3, a4, a5, a6, a7;
    for (int i = si; i < si + 4; i++)
    {
        a0 = A[i][sj];
        a1 = A[i][sj + 1];
        a2 = A[i][sj + 2];
        a3 = A[i][sj + 3];

        a4 = A[i][sj + 4];
        a5 = A[i][sj + 5];
        a6 = A[i][sj + 6];
        a7 = A[i][sj + 7];

        B[sj][i] = a0;
        B[sj + 1][i] = a1;
        B[sj + 2][i] = a2;
        B[sj + 3][i] = a3;

        // A12 -> B12，先转置，后续直接平移列到 B21
        B[sj + 0][i + 4] = a4;
        B[sj + 1][i + 4] = a5;
        B[sj + 2][i + 4] = a6;
        B[sj + 3][i + 4] = a7;
    }

    /* 这个版本会在 B 的 0~3 和 4~7 之间读写，缓存失效 */
    // for (int i = si + 4; i < si + 8; i++)
    // {
    //     // read A21 line
    //     a0 = A[i][sj];
    //     a1 = A[i][sj + 1];
    //     a2 = A[i][sj + 2];
    //     a3 = A[i][sj + 3];

    //     // read B12 column
    //     a4 = B[sj][i];
    //     a5 = B[sj + 1][i];
    //     a6 = B[sj + 2][i];
    //     a7 = B[sj + 3][i];

    //     // write A21 line to B12 column
    //     B[sj][i] = a0;
    //     B[sj + 1][i] = a1;
    //     B[sj + 2][i] = a2;
    //     B[sj + 3][i] = a3;

    //     // write B21 column
    //     B[sj + 4][i - 4] = a4;
    //     B[sj + 5][i - 4] = a5;
    //     B[sj + 6][i - 4] = a6;
    //     B[sj + 7][i - 4] = a7;
    // }

    /* 这个版本按列读取 A 然后按行写入 B 失效数量明显减少 */
    for (int j = sj; j < sj + 4; j++)
    {
        // read A21 column
        a0 = A[si + 4][j];
        a1 = A[si + 5][j];
        a2 = A[si + 6][j];
        a3 = A[si + 7][j];

        // read B12 row
        a4 = B[j][si + 4];
        a5 = B[j][si + 5];
        a6 = B[j][si + 6];
        a7 = B[j][si + 7];

        // write A21 column to B12 row
        B[j][si + 4] = a0;
        B[j][si + 5] = a1;
        B[j][si + 6] = a2;
        B[j][si + 7] = a3;

        // write B12 row to B21 row
        B[j + 4][si] = a4;
        B[j + 4][si + 1] = a5;
        B[j + 4][si + 2] = a6;
        B[j + 4][si + 3] = a7;
    }

    // A22 -> B22, 直接转置
    for (int i = si + 4; i < si + 8; i++)
    {
        a0 = A[i][sj + 4];
        a1 = A[i][sj + 4 + 1];
        a2 = A[i][sj + 4 + 2];
        a3 = A[i][sj + 4 + 3];

        B[sj + 4][i] = a0;
        B[sj + 4 + 1][i] = a1;
        B[sj + 4 + 2][i] = a2;
        B[sj + 4 + 3][i] = a3;
    }
}

void transpose_64_64(int M, int N, int A[N][M], int B[M][N])
{
    int block_length = 8;
    for (int i = 0; i < N; i += block_length)
    {
        for (int j = 0; j < M; j += block_length)
        {
            transpose_block_8_for_64_64(M, N, A, B, i, j);
        }
    }
}

void transpose_32_32(int M, int N, int A[N][M], int B[M][N])
{
    int block_length = 8;
    for (int i = 0; i < N; i += block_length)
    {
        for (int j = 0; j < M; j += block_length)
        {
            transpose_block_8(M, N, A, B, i, j, block_length);
        }
    }
}

void transpose_61_67(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, r, c;
    int bsize = 32;

    for (i = 0; i < N; i += bsize)
    {
        for (j = 0; j < M; j += bsize)
        {
            for (r = i; r < i + bsize && r < N; r++)
            {
                for (c = j; c < j + bsize && c < M; c++)
                {
                    B[c][r] = A[r][c];
                }
            }
        }
    }
}

void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32)
    {
        transpose_32_32(M, N, A, B);
    }
    else if (M == 64 && N == 64)
    {
        transpose_64_64(M, N, A, B);
    }
    else
    {
        transpose_61_67(M, N, A, B);
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
