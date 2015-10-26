#include "larpack.h"

void LARPACK(dtrtri)(const char *uplo, const char *diag, const int *n,
        double *A, const int *ldA, int *info) {
    *info = 0;

    // Check arguments
    int lower = LAPACK(lsame)(uplo, "L");
    int upper = LAPACK(lsame)(uplo, "U");
    int nounit = LAPACK(lsame)(diag, "N");
    int unit = LAPACK(lsame)(diag, "U");
    if (!upper && !lower)
        *info = -1;
    else if (!nounit && !unit)
        *info = -2;
    else if (*n < 0)
        *info = -3;
    else if (*ldA < MAX(1, *n))
        *info = -5;
    if (*info != 0) {
        int minfo = -*info;
        LAPACK(xerbla)("DTRTRI", &minfo);
        return;
    }

    if (*n <= LARPACK_CROSSOVER) {
        // Unblocked
        LAPACK(dtrti2)(uplo, diag, n, A, ldA, info);
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
    LARPACK(dtrtri)(uplo, diag, &n1, A_TL, ldA, info);
    if (*info)
        return;

    if (lower) {
        // A_BL = - A_BL * A_TL
        BLAS(dtrmm)("R", "L", "N", diag, &n2, &n1, dm1, A_TL, ldA, A_BL, ldA);
        // A_BL = A_BR \ A_BL
        BLAS(dtrsm)("L", "L", "N", diag, &n2, &n1, d1, A_BR, ldA, A_BL, ldA);
    } else {
        // A_TR = - A_TL * A_TR
        BLAS(dtrmm)("L", "U", "N", diag, &n1, &n2, dm1, A_TL, ldA, A_TR, ldA);
        // A_TR = A_TR / A_BR
        BLAS(dtrsm)("R", "U", "N", diag, &n1, &n2, d1, A_BR, ldA, A_TR, ldA);
    }

    // recursion(A_BR)
    LARPACK(dtrtri)(uplo, diag, &n2, A_BR, ldA, info);
    if (*info)
        *info += n1;
}
