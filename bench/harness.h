#pragma once
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
    return (n & 1) ? arr[n / 2] : (arr[n / 2 - 1] + arr[n / 2]) / 2;
}

/* ---------- benchmark runner ---------- */

#ifndef BENCH_ITER
#define BENCH_ITER 1000
#endif

#define BENCH_FMT "%-26s  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f\n"
#define BENCH_HDR "%-26s  %14s  %10s  %14s  %10s  %14s  %10s\n"

static void bench_print_header(void) {
    printf(BENCH_HDR,
           "scheme",
           "keygen (cyc)", "keygen (us)",
           "sign (cyc)",   "sign (us)",
           "verify (cyc)", "verify (us)");
    printf(BENCH_HDR,
           "------",
           "------------", "-----------",
           "----------",   "---------",
           "------------", "----------");
    fflush(stdout);
}

static void bench_run(const bench_scheme_t *s) {
    int      n    = s->iters > 0 ? s->iters : BENCH_ITER;
    uint8_t *pk   = malloc(s->pk_bytes);
    uint8_t *sk   = malloc(s->sk_bytes);
    uint8_t *sig  = malloc(s->sig_bytes);
    uint8_t  msg[32] = {0};
    size_t   siglen  = 0;

    uint64_t *kg_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *sg_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *vf_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *kg_ns  = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *sg_ns  = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *vf_ns  = malloc((size_t)n * sizeof(uint64_t));

    if (!pk || !sk || !sig || !kg_cyc || !sg_cyc || !vf_cyc
                           || !kg_ns  || !sg_ns  || !vf_ns) {
        fprintf(stderr, "bench_run: allocation failed for %s\n", s->name);
        goto cleanup;
    }

    /* warm up */
    s->keygen_fn(pk, sk);
    s->sign_fn(sig, &siglen, msg, sizeof(msg), sk);

/* Time one operation into cyc[i] and ns[i]. */
#define TIME_OP(cyc, ns, i, op) do {                                      \
    struct timespec _ts0, _ts1;                                           \
    uint64_t _t0 = cpucycles();                                           \
    clock_gettime(CLOCK_MONOTONIC, &_ts0);                                \
    (op);                                                                 \
    uint64_t _t1 = cpucycles();                                           \
    clock_gettime(CLOCK_MONOTONIC, &_ts1);                                \
    (cyc)[i] = _t1 - _t0;                                                \
    (ns)[i]  = (uint64_t)(_ts1.tv_sec  - _ts0.tv_sec)  * 1000000000ULL  \
             + (uint64_t)(_ts1.tv_nsec - _ts0.tv_nsec);                  \
} while (0)

    for (int i = 0; i < n; i++) {
        TIME_OP(kg_cyc, kg_ns, i, s->keygen_fn(pk, sk));
        TIME_OP(sg_cyc, sg_ns, i, s->sign_fn(sig, &siglen, msg, sizeof(msg), sk));
        TIME_OP(vf_cyc, vf_ns, i, s->verify_fn(sig, siglen, msg, sizeof(msg), pk));
    }

#undef TIME_OP

    printf(BENCH_FMT,
           s->name,
           bench_median(kg_cyc, n), bench_median(kg_ns, n) / 1000.0,
           bench_median(sg_cyc, n), bench_median(sg_ns, n) / 1000.0,
           bench_median(vf_cyc, n), bench_median(vf_ns, n) / 1000.0);
    fflush(stdout);

cleanup:
    free(pk); free(sk); free(sig);
    free(kg_cyc); free(sg_cyc); free(vf_cyc);
    free(kg_ns);  free(sg_ns);  free(vf_ns);
}
