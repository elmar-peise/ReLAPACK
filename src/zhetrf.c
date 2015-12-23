#include "relapack.h"
#include <stdlib.h>

void RELAPACK(zhetrf)(const char *uplo, const int *n,
        double *A, const int *ldA, int *ipiv,
        double *Work, const int *lWork, int *info) {

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
    else if (*lWork < 1 && *lWork != -1)
        *info = -7;
    if (*info) {
        const int minfo = -*info;
        LAPACK(xerbla)("ZHETRF", &minfo);
        return;
    }

    if (*lWork == -1) {
        // Work size query
        *Work = *ldA * *n;
        return;
    }

    double *W = Work;
    if (*lWork < *ldA * *n)
        W = malloc(*ldA * *n * 2 * sizeof(double));

    int nout;
    RELAPACK(zhetrf_rec)(uplo, n, n, &nout, A, ldA, ipiv, W, ldA, info);

    if (W != Work)
        free(W);
}
