#pragma once
#include <stddef.h>

/*
 * Metadata exported by every KEM scheme .so via bench_info().
 * Shim files include only this header — no harness/loader dependency.
 */
typedef struct {
    const char *name;
    size_t      pk_bytes;   /* public / encapsulation key */
    size_t      sk_bytes;   /* secret / decapsulation key */
    size_t      ct_bytes;   /* ciphertext */
    size_t      ss_bytes;   /* shared secret */
    int         iters;      /* 0 → use BENCH_ITER; non-zero overrides */
} bench_scheme_info_t;
