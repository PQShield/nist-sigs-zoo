#pragma once
#include "loader.h"
#include "../bench-common/harness_common.h"

/* ---------- benchmark runner ----------
 *
 * Signature op contract: keygen / sign / verify. The cycle counter, stats and
 * BENCH_TIME_OP macro are shared with bench-kem/ via harness_common.h. */

#define BENCH_FMT "%-26s  %6d  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f  %14" PRIu64 "  %10.1f\n"
#define BENCH_HDR "%-26s  %6s  %14s  %10s  %14s  %10s  %14s  %10s\n"
/* DNF row: the six timing columns read "DNF", followed by a one-word reason. */
#define BENCH_DNF_FMT "%-26s  %6s  %14s  %10s  %14s  %10s  %14s  %10s  %s\n"

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

/* A scheme that did not finish: crashed, timed out or failed a correctness check. */
static void bench_print_dnf(const char *name, int iters, const char *reason) {
    char it[16] = "-";
    if (iters > 0) snprintf(it, sizeof it, "%d", iters);
    printf(BENCH_DNF_FMT, name, it, "DNF", "DNF", "DNF", "DNF", "DNF", "DNF", reason);
    fflush(stdout);
    fprintf(stderr, "DNF %s: %s\n", name, reason);
}

/* Iteration count: BENCH_ITER env var, else the per-scheme override, else the
 * compile-time default. */
static int bench_iters(const bench_scheme_t *s) {
    const char *e = getenv("BENCH_ITER");
    int env_n = (e && atoi(e) > 0) ? atoi(e) : 0;
    return env_n > 0 ? env_n : (s->iters > 0 ? s->iters : BENCH_ITER);
}

/* Benchmark one scheme and write its results row, prefixed "R ", to `out`.
 * Returns NULL on success, or a one-word DNF reason if the scheme failed a
 * correctness check (then no row is written, so a broken build can't be
 * imported with timings). */
static const char *bench_run(const bench_scheme_t *s, FILE *out) {
    const char *why = NULL;
    int      n    = bench_iters(s);
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
        why = "alloc-failed";
        goto cleanup;
    }

    /* Pin this worker thread to one core and open the core-cycles PMC (or fall
     * back to rdtsc). Per-scheme because each scheme runs on its own thread. */
    cycles_setup();

    /* Warm up, and check the build actually works: a fresh signature must
     * verify, and a tampered signature or message must be rejected. */
    if (s->keygen_fn(pk, sk) != 0)
        why = "keygen-failed";
    else if (s->sign_fn(sig, &siglen, msg, sizeof(msg), sk) != 0)
        why = "sign-failed";
    else if (siglen == 0 || siglen > s->sig_bytes)
        why = "bad-signature-length";
    else if (s->verify_fn(sig, siglen, msg, sizeof(msg), pk) != 0)
        why = "valid-signature-rejected";
    else {
        sig[siglen / 2] ^= 1;
        if (s->verify_fn(sig, siglen, msg, sizeof(msg), pk) == 0)
            why = "tampered-signature-accepted";
        sig[siglen / 2] ^= 1;
        msg[0] ^= 1;
        if (!why && s->verify_fn(sig, siglen, msg, sizeof(msg), pk) == 0)
            why = "tampered-message-accepted";
        msg[0] ^= 1;
    }
    if (why) goto cleanup;

    int rc_kg = 0, rc_sg = 0, rc_vf = 0;
    for (int i = 0; i < n; i++) {
        BENCH_TIME_OP(kg_cyc, kg_ns, i, rc_kg |= s->keygen_fn(pk, sk));
        BENCH_TIME_OP(sg_cyc, sg_ns, i, rc_sg |= s->sign_fn(sig, &siglen, msg, sizeof(msg), sk));
        BENCH_TIME_OP(vf_cyc, vf_ns, i, rc_vf |= s->verify_fn(sig, siglen, msg, sizeof(msg), pk));
    }
    if (rc_kg || rc_sg || rc_vf) {
        why = rc_kg ? "keygen-error-in-timed-loop"
            : rc_sg ? "sign-error-in-timed-loop" : "verify-error-in-timed-loop";
        goto cleanup;
    }

    fprintf(out, "R " BENCH_FMT,
           s->name, n,
           bench_median(kg_cyc, n), bench_median(kg_ns, n) / 1000.0,
           bench_median(sg_cyc, n), bench_median(sg_ns, n) / 1000.0,
           bench_median(vf_cyc, n), bench_median(vf_ns, n) / 1000.0);
    fflush(out);

cleanup:
    cycles_teardown();
    free(pk); free(sk); free(sig);
    free(kg_cyc); free(sg_cyc); free(vf_cyc);
    free(kg_ns);  free(sg_ns);  free(vf_ns);
    return why;
}
