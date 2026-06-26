#pragma once
#include "loader.h"
#include "../bench-common/harness_common.h"

/* ---------- benchmark runner ----------
 *
 * KEM op contract: keygen / encaps / decaps, with an encaps/decaps shared-secret
 * round-trip check. The cycle counter, stats and BENCH_TIME_OP macro are shared
 * with bench/ via harness_common.h. */

#define BENCH_FMT "%-26s  %6d  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f  %6s\n"
#define BENCH_HDR "%-26s  %6s  %14s  %10s  %14s  %10s  %14s  %10s  %6s\n"

static void bench_print_header(void) {
    printf(BENCH_HDR,
           "scheme",        "iters",
           "keygen (cyc)",  "keygen (us)",
           "encaps (cyc)",  "encaps (us)",
           "decaps (cyc)",  "decaps (us)",
           "ok");
    printf(BENCH_HDR,
           "------",        "-----",
           "------------",  "-----------",
           "------------",  "-----------",
           "------------",  "-----------",
           "--");
    fflush(stdout);
}

static void bench_run(const bench_scheme_t *s) {
    /* BENCH_ITER env var overrides per-scheme iters and compile-time default */
    static int env_n = -1;
    if (env_n < 0) {
        const char *e = getenv("BENCH_ITER");
        env_n = (e && atoi(e) > 0) ? atoi(e) : 0;
    }
    int      n   = env_n > 0 ? env_n : (s->iters > 0 ? s->iters : BENCH_ITER);
    uint8_t *pk  = malloc(s->pk_bytes);
    uint8_t *sk  = malloc(s->sk_bytes);
    uint8_t *ct  = malloc(s->ct_bytes);
    uint8_t *ss1 = malloc(s->ss_bytes);   /* shared secret from encaps */
    uint8_t *ss2 = malloc(s->ss_bytes);   /* shared secret from decaps */
    int      mismatches = 0;

    uint64_t *kg_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *en_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *de_cyc = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *kg_ns  = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *en_ns  = malloc((size_t)n * sizeof(uint64_t));
    uint64_t *de_ns  = malloc((size_t)n * sizeof(uint64_t));

    if (!pk || !sk || !ct || !ss1 || !ss2 || !kg_cyc || !en_cyc || !de_cyc
                                          || !kg_ns  || !en_ns  || !de_ns) {
        fprintf(stderr, "bench_run: allocation failed for %s\n", s->name);
        goto cleanup;
    }

    /* Pin this worker thread to one core and open the core-cycles PMC (or fall
     * back to rdtsc). Per-scheme because each scheme runs on its own thread. */
    cycles_setup();

    /* warm up: full keygen → encaps → decaps round-trip */
    s->keygen_fn(pk, sk);
    s->encaps_fn(ct, ss1, pk);
    s->decaps_fn(ss2, ct, sk);

    for (int i = 0; i < n; i++) {
        BENCH_TIME_OP(kg_cyc, kg_ns, i, s->keygen_fn(pk, sk));
        BENCH_TIME_OP(en_cyc, en_ns, i, s->encaps_fn(ct, ss1, pk));
        BENCH_TIME_OP(de_cyc, de_ns, i, s->decaps_fn(ss2, ct, sk));
        /* Correctness: encaps and decaps must agree on the shared secret.
         * Checked outside the timed region; a mismatch is tallied, not fatal,
         * so a broken scheme still reports its timing numbers. */
        if (memcmp(ss1, ss2, s->ss_bytes) != 0) mismatches++;
    }

    printf(BENCH_FMT,
           s->name, n,
           bench_median(kg_cyc, n), bench_median(kg_ns, n) / 1000.0,
           bench_median(en_cyc, n), bench_median(en_ns, n) / 1000.0,
           bench_median(de_cyc, n), bench_median(de_ns, n) / 1000.0,
           mismatches == 0 ? "yes" : "FAIL");
    if (mismatches != 0)
        fprintf(stderr, "  ! %s: %d/%d shared-secret mismatches\n",
                s->name, mismatches, n);
    fflush(stdout);

cleanup:
    cycles_teardown();
    free(pk); free(sk); free(ct); free(ss1); free(ss2);
    free(kg_cyc); free(en_cyc); free(de_cyc);
    free(kg_ns);  free(en_ns);  free(de_ns);
}
