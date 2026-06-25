#pragma once
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "loader.h"

/* ---------- cycle counter ---------- */

#if defined(__x86_64__) || defined(__i386__)
static inline uint64_t cpucycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__(
        "lfence\n\t"
        "rdtsc"
        : "=a"(lo), "=d"(hi)
        :: "memory"
    );
    return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__)
/* cntvct_el0 is virtual timer counter; may need perf_event for true cycles */
static inline uint64_t cpucycles(void) {
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#else
#error "cpucycles: unsupported architecture"
#endif

/* ---------- stats ---------- */

static int bench_cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t bench_median(uint64_t *arr, int n) {
    qsort(arr, (size_t)n, sizeof(uint64_t), bench_cmp_u64);
    return (n & 1) ? arr[n / 2] : arr[n/2-1] + (arr[n/2] - arr[n/2-1]) / 2;
}

/* ---------- benchmark runner ---------- */

#ifndef BENCH_ITER
#define BENCH_ITER 1000
#endif

#define BENCH_FMT "%-30s  %6d  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f\n"
#define BENCH_HDR "%-30s  %6s  %14s  %10s  %14s  %10s  %14s  %10s\n"

static void bench_print_header(void) {
    printf(BENCH_HDR,
           "scheme",         "iters",
           "keygen (cyc)",  "keygen (us)",
           "encaps (cyc)",  "encaps (us)",
           "decaps (cyc)",  "decaps (us)");
    printf(BENCH_HDR,
           "------",          "-----",
           "------------",   "-----------",
           "------------",   "-----------",
           "------------",   "-----------");
    fflush(stdout);
}

static void bench_run(const bench_scheme_t *s) {
    /* BENCH_ITER env var overrides per-scheme iters and compile-time default */
    static int env_n = -1;
    if (env_n < 0) {
        const char *e = getenv("BENCH_ITER");
        env_n = (e && atoi(e) > 0) ? atoi(e) : 0;
    }
    int      n     = env_n > 0 ? env_n : (s->iters > 0 ? s->iters : BENCH_ITER);
    uint8_t *pk    = malloc(s->pk_bytes);
    uint8_t *sk    = malloc(s->sk_bytes);
    uint8_t *ct    = malloc(s->ct_bytes);
    uint8_t *ss_e  = malloc(s->ss_bytes);   /* shared secret from encaps */
    uint8_t *ss_d  = malloc(s->ss_bytes);   /* shared secret from decaps */

    uint64_t *kg_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *en_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *de_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *kg_ns  = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *en_ns  = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *de_ns  = malloc((size_t)n * sizeof(uint64_t));

    if (!pk || !sk || !ct || !ss_e || !ss_d || !kg_cyc || !en_cyc || !de_cyc
                                            || !kg_ns  || !en_ns  || !de_ns) {
        fprintf(stderr, "bench_run: allocation failed for %s\n", s->name);
        goto cleanup;
    }

    /* warm up + correctness check: decapsulated secret must match encapsulated */
    s->keygen_fn(pk, sk);
    s->enc_fn(ct, ss_e, pk);
    s->dec_fn(ss_d, ct, sk);
    if (memcmp(ss_e, ss_d, s->ss_bytes) != 0) {
        fprintf(stderr, "bench_run: %s: shared secrets DIFFER (correctness fail)\n",
                s->name);
        /* still report timings — a broken/badly-behaved impl is allowed */
    }

/* Time one operation into cyc[i] and ns[i]. */
#define TIME_OP(cyc, ns, i, op) do {                                      \
    struct timespec _ts0, _ts1;                                           \
    uint64_t _t0 = cpucycles();                                           \
    clock_gettime(CLOCK_MONOTONIC, &_ts0);                                \
    (op);                                                                 \
    uint64_t _t1 = cpucycles();                                           \
    clock_gettime(CLOCK_MONOTONIC, &_ts1);                                \
    (cyc)[i] = _t1 - _t0;                                                  \
    (ns)[i]  = (uint64_t)(_ts1.tv_sec  - _ts0.tv_sec)  * 1000000000ULL    \
             + (uint64_t)(_ts1.tv_nsec - _ts0.tv_nsec);                   \
} while (0)

    for (int i = 0; i < n; i++) {
        TIME_OP(kg_cyc, kg_ns, i, s->keygen_fn(pk, sk));
        TIME_OP(en_cyc, en_ns, i, s->enc_fn(ct, ss_e, pk));
        TIME_OP(de_cyc, de_ns, i, s->dec_fn(ss_d, ct, sk));
    }

#undef TIME_OP

    printf(BENCH_FMT,
           s->name, n,
           bench_median(kg_cyc, n), bench_median(kg_ns, n) / 1000.0,
           bench_median(en_cyc, n), bench_median(en_ns, n) / 1000.0,
           bench_median(de_cyc, n), bench_median(de_ns, n) / 1000.0);
    fflush(stdout);

cleanup:
    free(pk); free(sk); free(ct); free(ss_e); free(ss_d);
    free(kg_cyc); free(en_cyc); free(de_cyc);
    free(kg_ns);  free(en_ns);  free(de_ns);
}
