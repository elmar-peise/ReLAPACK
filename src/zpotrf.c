#include "larpack.h"

void LARPACK(zpotrf)(const char *uplo, const int *n,
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
        LAPACK(xerbla)("CPOTRF", &minfo);
        return;
    }

    if (*n <= LARPACK_CROSSOVER) {
        // Unblocked
        LAPACK(zpotf2)(uplo, n, A, ldA, info);
        return;
    }

    // Recursive
    const int n1 = (*n >= 16) ? ((*n + 8) / 16) * 8 : *n / 2;
    const int n2 = *n - n1;

    // A_TL A_TR
    // A_BL A_BR
    double *const A_TL = A;
    double *const A_TR = A + 2 * *ldA * n1;
    double *const A_BL = A                 + 2 * n1;
    double *const A_BR = A + 2 * *ldA * n1 + 2 * n1;

    // 1, -1
   	const double z1[] = {1, 0}, zm1[] = {-1, 0};

    // recursion(A_TL)
    LARPACK(zpotrf)(uplo, &n1, A_TL, ldA, info);
    if (*info)
        return;

    if (lower) {
        // A_BL = A_BL / A_TL'
        BLAS(ztrsm)("R", "L", "C", "N", &n2, &n1, z1, A_TL, ldA, A_BL, ldA);
        // A_BR = A_BR - A_BL * A_BL'
        BLAS(zherk)("L", "N", &n2, &n1, zm1, A_BL, ldA, z1, A_BR, ldA);
    } else {
        // A_TR = A_TL' \ A_TR
        BLAS(ztrsm)("L", "U", "C", "N", &n1, &n2, z1, A_TL, ldA, A_TR, ldA);
        // A_BR = A_BR - A_TR' * A_TR
        BLAS(zherk)("U", "C", &n2, &n1, zm1, A_TR, ldA, z1, A_BR, ldA);
    }

    // recursion(A_BR)
    LARPACK(zpotrf)(uplo, &n2, A_BR, ldA, info);
    if (*info)
        *info += n1;
}