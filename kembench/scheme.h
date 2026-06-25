#pragma once
#include <stddef.h>

/*
 * Metadata exported by every KEM scheme .so via bench_info().
 * Shim files include only this header — no harness/loader dependency.
 *
 * Sizes mirror the NIST KEM API constants:
 *   pk_bytes  = CRYPTO_PUBLICKEYBYTES
 *   sk_bytes  = CRYPTO_SECRETKEYBYTES
 *   ct_bytes  = CRYPTO_CIPHERTEXTBYTES
 *   ss_bytes  = CRYPTO_BYTES (shared-secret length)
 */
typedef struct {
    const char *name;
    size_t      pk_bytes;
    size_t      sk_bytes;
    size_t      ct_bytes;
    size_t      ss_bytes;
    int         iters;    /* 0 → use BENCH_ITER; non-zero overrides */
} bench_scheme_info_t;
