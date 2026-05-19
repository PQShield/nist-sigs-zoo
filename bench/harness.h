#pragma once
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

    if (!pk || !sk || !sig || !kg_cyc || !sg_cyc || !vf_cyc) {
        fprintf(stderr, "bench_run: allocation failed for %s\n", s->name);
        goto cleanup;
    }

    /* warm up */
    s->keygen_fn(pk, sk);
    s->sign_fn(sig, &siglen, msg, sizeof(msg), sk);

    for (int i = 0; i < n; i++) {
        uint64_t t0, t1;

        t0 = cpucycles();
        s->keygen_fn(pk, sk);
        t1 = cpucycles();
        kg_cyc[i] = t1 - t0;

        t0 = cpucycles();
        s->sign_fn(sig, &siglen, msg, sizeof(msg), sk);
        t1 = cpucycles();
        sg_cyc[i] = t1 - t0;

        t0 = cpucycles();
        s->verify_fn(sig, siglen, msg, sizeof(msg), pk);
        t1 = cpucycles();
        vf_cyc[i] = t1 - t0;
    }

    printf("%-24s  %16" PRIu64 "  %16" PRIu64 "  %16" PRIu64 "\n",
           s->name,
           bench_median(kg_cyc, n),
           bench_median(sg_cyc, n),
           bench_median(vf_cyc, n));
    fflush(stdout);

cleanup:
    free(pk); free(sk); free(sig);
    free(kg_cyc); free(sg_cyc); free(vf_cyc);
}
