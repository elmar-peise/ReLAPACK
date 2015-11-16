#include "relapack.h"

void RELAPACK(cgetrf)(const int *m, const int *n,
        float *A, const int *ldA, int *ipiv, int *info) {

    // Check arguments
    *info = 0;
    if (*m < 0)
        *info = -1;
    else if (*n < 0)
        *info = -2;
    else if (*ldA < MAX(1, *n))
        *info = -4;
    if (*info) {
        const int minfo = -*info;
        LAPACK(xerbla)("CGETRF", &minfo);
        return;
    }

    if (*n <= CROSSOVER_CGETRF) {
        // Unblocked
        LAPACK(cgetf2)(m, n, A, ldA, ipiv, info);
        return;
    }

    // Recursive

    // Constants
    // 1, -1
   	const float ONE[] = {1, 0}, MONE[] = {-1, 0};
    // 1
    const int iONE[] = {1};

    // Splitting
    const int mn = MIN(*m, *n);
    const int n1 = (mn >= 16) ? ((mn + 8) / 16) * 8 : mn / 2;
    const int n2 = mn - n1;
    const int rm = *m - n1;

    // A_TL A_TR
    // A_BL A_BR
    float *const A_TL = A;
    float *const A_TR = A + 2 * *ldA * n1;
    float *const A_BL = A                 + 2 * n1;
    float *const A_BR = A + 2 * *ldA * n1 + 2 * n1;

    // ipiv_T
    // ipiv_B
    int *const ipiv_T = ipiv;
    int *const ipiv_B = ipiv + n1;

    // recursion(A_TL, ipiv_T)
    RELAPACK(cgetrf)(m, &n1, A_TL, ldA, ipiv_T, info);
    // apply pivots to A_TR
    LAPACK(claswp)(&n2, A_TR, ldA, iONE, &n1, ipiv_T, iONE);

    // A_TR = A_TL \ A_TR
    BLAS(ctrsm)("L", "L", "N", "U", &n1, &n2, ONE, A_TL, ldA, A_TR, ldA);
    // A_BR = A_BR - A_BL * A_TR
    BLAS(cgemm)("N", "N", &rm, &n2, &n1, MONE, A_BL, ldA, A_TR, ldA, ONE, A_BR, ldA);

    // recursion(A_BR, ipiv_B)
    RELAPACK(cgetrf)(&rm, &n2, A_BR, ldA, ipiv_B, info);
    if (*info)
        *info += n1;
    // apply pivots to A_BL
    LAPACK(claswp)(&n1, A_BL, ldA, iONE, &n2, ipiv_B, iONE);
    // shift pivots
    int i;
    for (i = 0; i < n2; i++)
        ipiv_B[i] += n1;

    if (*n == mn)
        return;

    // Right remainder
    const int rn = *n - mn;

    // A_S A_R
    float *const A_S = A;
    float *const A_R = A + 2 * *ldA * mn;

    // A_R = apply(ipiv, A_R)
    LAPACK(claswp)(&rn, A_R, ldA, iONE, &mn, ipiv, iONE);
    // A_R = A_S \ A_R
    BLAS(ctrsm)("L", "L", "N", "U", &mn, &rn, ONE, A_S, ldA, A_R, ldA);
}
