#include "relapack.h"

void RELAPACK(dlauum)(const char *uplo, const int *n,
        double *A, const int *ldA, int *info) {

    // Check arguments
    const int lower = LAPACK(lsame)(uplo, "L");
    const int upper = LAPACK(lsame)(uplo, "U");
    *info = 0;
    if (!lower && !upper)
        *info = -1;
    else if (*n < 0)
        *info = -2;
    else if (*ldA < MAX(1, *n))
        *info = -4;
    if (*info) {
        const int minfo = -*info;
        LAPACK(xerbla)("DLAUUM", &minfo);
        return;
    }

    if (*n <= CROSSOVER_DLAUUM) {
        // Unblocked
        LAPACK(dlauu2)(uplo, n, A, ldA, info);
        return;
    }

    // Recursive

    // Constants
    // 1
   	const double ONE[] = {1};

    // Splitting
    const int n1 = (*n >= 16) ? ((*n + 8) / 16) * 8 : *n / 2;
    const int n2 = *n - n1;

    // A_TL A_TR
    // A_BL A_BR
    double *const A_TL = A;
    double *const A_TR = A + *ldA * n1;
    double *const A_BL = A             + n1;
    double *const A_BR = A + *ldA * n1 + n1;

    // recursion(A_TL)
    RELAPACK(dlauum)(uplo, &n1, A_TL, ldA, info);

    if (lower) {
        // A_TL = A_TL + A_BL' * A_BL
        BLAS(dsyrk)("L", "T", &n1, &n2, ONE, A_BL, ldA, ONE, A_TL, ldA);
        // A_BL = A_BR' * A_BL
        BLAS(dtrmm)("L", "L", "T", "N", &n2, &n1, ONE, A_BR, ldA, A_BL, ldA);
    } else {
        // A_TL = A_TL + A_TR * A_TR'
        BLAS(dsyrk)("U", "N", &n1, &n2, ONE, A_TR, ldA, ONE, A_TL, ldA);
        // A_TR = A_TR * A_BR'
        BLAS(dtrmm)("R", "U", "T", "N", &n1, &n2, ONE, A_BR, ldA, A_TR, ldA);
    }

    // recursion(A_BR)
    RELAPACK(dlauum)(uplo, &n2, A_BR, ldA, info);
}
