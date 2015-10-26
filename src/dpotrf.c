#include "larpack.h"

void LARPACK(dpotrf)(const char *uplo, const int *n,
        double *A, const int *ldA, int *info) {
    *info = 0;

    // Check arguments
    int lower = LAPACK(lsame)(uplo, "L");
    int upper = LAPACK(lsame)(uplo, "U");
    if (!upper && !lower)
        *info = -1;
    else if (*n < 0)
        *info = -2;
    else if (*ldA < MAX(1, *n))
        *info = -4;
    if (*info != 0) {
        int minfo = -*info;
        LAPACK(xerbla)("DPOTRF", &minfo);
        return;
    }

    if (*n <= LARPACK_CROSSOVER) {
        // Unblocked
        LAPACK(dpotf2)(uplo, n, A, ldA, info);
        return;
    }

    // Recursive
    const int n1 = (*n >= 16) ? ((*n + 8) / 16) * 8 : *n / 2;
    const int n2 = *n - n1;

    // A_TL A_TR
    // A_BL A_BR
    double *const A_TL = A;
    double *const A_TR = A + *ldA * n1;
    double *const A_BL = A             + n1;
    double *const A_BR = A + *ldA * n1 + n1;

    // 1, -1
   	const double d1[] = {1}, dm1[] = {-1};

    // recursion(A_TL)
    LARPACK(dpotrf)(uplo, &n1, A_TL, ldA, info);
    if (*info)
        return;

    if (lower) {
        // A_BL = A_BL / A_TL'
        BLAS(dtrsm)("R", "L", "T", "N", &n2, &n1, d1, A_TL, ldA, A_BL, ldA);
        // A_BR = A_BR - A_BL * A_BL'
        BLAS(dsyrk)("L", "N", &n2, &n1, dm1, A_BL, ldA, d1, A_BR, ldA);
    } else {
        // A_TR = A_TL' \ A_TR
        BLAS(dtrsm)("L", "U", "T", "N", &n1, &n2, d1, A_TL, ldA, A_TR, ldA);
        // A_BR = A_BR - A_TR' * A_TR
        BLAS(dsyrk)("U", "T", &n2, &n1, dm1, A_TR, ldA, d1, A_BR, ldA);
    }

    // recursion(A_BR)
    LARPACK(dpotrf)(uplo, &n2, A_BR, ldA, info);
    if (*info)
        *info += n1;
}
