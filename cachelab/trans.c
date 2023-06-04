#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void trans32(int M, int N, int A[N][M], int B[M][N]);
void trans64(int M, int N, int A[N][M], int B[M][N]);
void transX(int M, int N, int A[N][M], int B[M][N]);
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == 32 && M == 32)
    {
        trans32(M, N, A, B);
    }
    else if (N == 64 && M == 64)
    {
        trans64(M, N, A, B);
    }
    else
    {
        transX(M, N, A, B);
    }

    return;
}

void transX(int M, int N, int A[N][M], int B[M][N])
{
    for (int fourthI = 0; fourthI < N; fourthI += 8)
    {
        for (int fourthJ = 0; fourthJ < M; fourthJ += 8)
        {
            for (int sub = 0; sub < 8; sub += 4)
            {
                int i = fourthI;
                int iEnd = i + 8 > N ? N : i + 8;
                for (; i < iEnd; i++)
                {
                    int k = fourthJ + sub;
                    int kEnd = k + 4 > M ? M : k + 4;
                    for (; k < kEnd; k++)
                    {
                        B[k][i] = A[i][k];
                    }
                }
            }
        }
    }

    return;
}

void trans64(int M, int N, int A[N][M], int B[M][N])
{
    for (int fourthI = 0; fourthI < 64; fourthI += 8)
    {
        for (int fourthJ = 0; fourthJ < 64; fourthJ += 8)
        {
            for (int sub = 0; sub < 8; sub += 4)
            {
                for (int i = 0; i < 8; i++)
                {
                    int iEf = i + fourthI;
                    int a0 = A[iEf][fourthJ + sub];
                    int a1 = A[iEf][fourthJ + 1 + sub];
                    int a2 = A[iEf][fourthJ + 2 + sub];
                    int a3 = A[iEf][fourthJ + 3 + sub];
                    B[fourthJ + sub][iEf] = a0;
                    B[fourthJ + 1 + sub][iEf] = a1;
                    B[fourthJ + 2 + sub][iEf] = a2;
                    B[fourthJ + 3 + sub][iEf] = a3;
                }
            }
        }
    }

    return;
}

void trans32(int M, int N, int A[N][M], int B[M][N])
{
    for (int fourthI = 0; fourthI < 32; fourthI += 8)
    {
        for (int fourthJ = 0; fourthJ < 32; fourthJ += 8)
        {
            for (int i = 0; i < 8; i++)
            {
                int iEf = i + fourthI;
                int a0 = A[iEf][fourthJ];
                int a1 = A[iEf][fourthJ + 1];
                int a2 = A[iEf][fourthJ + 2];
                int a3 = A[iEf][fourthJ + 3];
                int a4 = A[iEf][fourthJ + 4];
                int a5 = A[iEf][fourthJ + 5];
                int a6 = A[iEf][fourthJ + 6];
                int a7 = A[iEf][fourthJ + 7];
                B[fourthJ][iEf] = a0;
                B[fourthJ + 1][iEf] = a1;
                B[fourthJ + 2][iEf] = a2;
                B[fourthJ + 3][iEf] = a3;
                B[fourthJ + 4][iEf] = a4;
                B[fourthJ + 5][iEf] = a5;
                B[fourthJ + 6][iEf] = a6;
                B[fourthJ + 7][iEf] = a7;
            }
        }
    }

    return;
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
