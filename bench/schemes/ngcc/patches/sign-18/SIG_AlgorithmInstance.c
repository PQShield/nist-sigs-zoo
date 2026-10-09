/* PATCH (nist-sigs-zoo): sig_sign returned -4 in about 7% of calls: the tail zone has no variables to resample,
 * so all MAX_SIGN_ATTEMPTS retries rebuild the same singular matrix; see docs/ngcc-submission-bugs.md item 16.
 * The submitter's own benchmark driver (Common/bench_sig.c, sign_with_retry) and comm_cost.c call sig_sign again
 * on -4, which draws a fresh salt from drng_algorithm. This copy does that retry inside sig_sign (same limit,
 * 10000 calls). The first attempt is unchanged, so a call that succeeded before gives the same signature.
 * Nothing else is changed. Identical for Origami-128/256/384/512 (the four submitted files are identical). */
#include <stddef.h>
#include <stdint.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "auxfunc.h"
#include "origami.h"

/* Retry limit of the submitter's Common/bench_sig.c (MAX_SIGN_ATTEMPTS). */
#define ORIGAMI_SIGN_RETRIES 10000ULL

extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes(void) {
    return BYTES_PK;
}

unsigned long long sig_get_sk_len_bytes(void) {
    return BYTES_SK;
}

unsigned long long sig_get_sn_len_bytes(void) {
    return BYTES_SIGNATURE;
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    uint8_t seed[SEED_LENGTH];
    if (get_random_number(&drng_algorithm, seed, SEED_LENGTH * 8ULL) != 0) {
        return -1;
    }
    if (ORIGAMI_NAMESPACE(genkeys)(pk, sk, seed) != 0) {
        return -2;
    }
    *pk_len_bytes = BYTES_PK;
    *sk_len_bytes = BYTES_SK;
    return 0;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    uint8_t digest[BYTES_DIGEST];
    uint8_t salt[BYTES_SALT];
    ph_expanded_SK skx;
    if (sk_len_bytes != BYTES_SK) {
        return -5;
    }

    if (pseudohash(BYTES_DIGEST * 8, m, m_len_bytes * 8ULL, digest) != 0) {
        return -1;
    }
    if (get_random_number(&drng_algorithm, salt, BYTES_SALT * 8ULL) != 0) {
        return -2;
    }
    if (ORIGAMI_NAMESPACE(sk_expand)(&skx, sk) != 0) {
        return -3;
    }
    /* PATCH: retry with a fresh salt on -4, as the submitter's Common/bench_sig.c sign_with_retry() does. */
    for (unsigned long long attempt = 1ULL;
         ORIGAMI_NAMESPACE(sign)(&skx, sn, digest, BYTES_DIGEST, salt) != 0; attempt++) {
        if (attempt >= ORIGAMI_SIGN_RETRIES ||
            get_random_number(&drng_algorithm, salt, BYTES_SALT * 8ULL) != 0) {
            ORIGAMI_NAMESPACE(sk_free)(&skx);
            return -4;
        }
    }
    ORIGAMI_NAMESPACE(sk_free)(&skx);
    *sn_len_bytes = BYTES_SIGNATURE;
    return 0;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    uint8_t digest[BYTES_DIGEST];
    ph_expanded_PK pkx;
    if (pk_len_bytes != BYTES_PK || sn_len_bytes != BYTES_SIGNATURE) {
        return -3;
    }

    if (pseudohash(BYTES_DIGEST * 8, m, m_len_bytes * 8ULL, digest) != 0) {
        return -1;
    }
    if (ORIGAMI_NAMESPACE(pk_expand)(&pkx, pk) != 0) {
        return -2;
    }
    if (ORIGAMI_NAMESPACE(verify)(&pkx, sn, digest, BYTES_DIGEST) != 0) {
        ORIGAMI_NAMESPACE(pk_free)(&pkx);
        return -1;
    }
    ORIGAMI_NAMESPACE(pk_free)(&pkx);
    return 0;
}
