#pragma once
#include <stddef.h>

/*
 * Metadata exported by every scheme .so via bench_info().
 * Shim files include only this header — no harness/loader dependency.
 */
typedef struct {
    const char *name;
    size_t      pk_bytes;
    size_t      sk_bytes;
    size_t      sig_bytes;
    int         iters;    /* 0 → use BENCH_ITER; non-zero overrides */
} bench_scheme_info_t;
