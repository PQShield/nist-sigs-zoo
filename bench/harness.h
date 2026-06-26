#pragma once
#include "loader.h"
#include "../bench-common/harness_common.h"

/* ---------- benchmark runner ----------
 *
 * Signature op contract: keygen / sign / verify. The cycle counter, stats and
 * BENCH_TIME_OP macro are shared with bench-kem/ via harness_common.h. */

#define BENCH_FMT "%-26s  %6d  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f\n"
#define BENCH_HDR "%-26s  %6s  %14s  %10s  %14s  %10s  %14s  %10s\n"

static void bench_print_header(void) {
    printf(BENCH_HDR,
           "scheme",        "iters",
           "keygen (cyc)", "keygen (us)",
           "sign (cyc)",   "sign (us)",
           "verify (cyc)", "verify (us)");
    printf(BENCH_HDR,
           "------",        "-----",
           "------------", "-----------",
           "----------",   "---------",
           "------------", "----------");
    fflush(stdout);
}

static void bench_run(const bench_scheme_t *s) {
    /* BENCH_ITER env var overrides per-scheme iters and compile-time default */
    static int env_n = -1;
    if (env_n < 0) {
        const char *e = getenv("BENCH_ITER");
        env_n = (e && atoi(e) > 0) ? atoi(e) : 0;
    }
    int      n    = env_n > 0 ? env_n : (s->iters > 0 ? s->iters : BENCH_ITER);
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

    /* Pin this worker thread to one core and open the core-cycles PMC (or fall
     * back to rdtsc). Per-scheme because each scheme runs on its own thread. */
    cycles_setup();

    /* warm up */
    s->keygen_fn(pk, sk);
    s->sign_fn(sig, &siglen, msg, sizeof(msg), sk);

    for (int i = 0; i < n; i++) {
        BENCH_TIME_OP(kg_cyc, kg_ns, i, s->keygen_fn(pk, sk));
        BENCH_TIME_OP(sg_cyc, sg_ns, i, s->sign_fn(sig, &siglen, msg, sizeof(msg), sk));
        BENCH_TIME_OP(vf_cyc, vf_ns, i, s->verify_fn(sig, siglen, msg, sizeof(msg), pk));
    }

    printf(BENCH_FMT,
           s->name, n,
           bench_median(kg_cyc, n), bench_median(kg_ns, n) / 1000.0,
           bench_median(sg_cyc, n), bench_median(sg_ns, n) / 1000.0,
           bench_median(vf_cyc, n), bench_median(vf_ns, n) / 1000.0);
    fflush(stdout);

cleanup:
    cycles_teardown();
    free(pk); free(sk); free(sig);
    free(kg_cyc); free(sg_cyc); free(vf_cyc);
    free(kg_ns);  free(sg_ns);  free(vf_ns);
}
