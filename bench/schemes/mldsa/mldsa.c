/*
 * Adapters mapping bench_scheme_t to the pq-crystals/dilithium reference API.
 * Compiled against three static archives (one per DILITHIUM_MODE).
 */
#include <stddef.h>
#include <stdint.h>
#include "mldsa.h"

/* Forward-declare the three sets of functions without pulling in api.h
 * (which redefines DILITHIUM_* macros based on mode and can't be included
 * three times cleanly).  Sizes are from FIPS 204 / pq-crystals api.h. */

/* ML-DSA-44 (dilithium2) */
int pqcrystals_dilithium2_ref_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_dilithium2_ref_signature(uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *sk);
int pqcrystals_dilithium2_ref_verify(const uint8_t *sig, size_t siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *pk);

/* ML-DSA-65 (dilithium3) */
int pqcrystals_dilithium3_ref_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_dilithium3_ref_signature(uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *sk);
int pqcrystals_dilithium3_ref_verify(const uint8_t *sig, size_t siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *pk);

/* ML-DSA-87 (dilithium5) */
int pqcrystals_dilithium5_ref_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_dilithium5_ref_signature(uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *sk);
int pqcrystals_dilithium5_ref_verify(const uint8_t *sig, size_t siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *pk);

/* ---------- thin wrappers (empty context) ---------- */

static int mldsa44_keygen(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_dilithium2_ref_keypair(pk, sk);
}
static int mldsa44_sign(uint8_t *sig, size_t *siglen,
                        const uint8_t *msg, size_t msglen,
                        const uint8_t *sk) {
    return pqcrystals_dilithium2_ref_signature(sig, siglen, msg, msglen, NULL, 0, sk);
}
static int mldsa44_verify(const uint8_t *sig, size_t siglen,
                          const uint8_t *msg, size_t msglen,
                          const uint8_t *pk) {
    return pqcrystals_dilithium2_ref_verify(sig, siglen, msg, msglen, NULL, 0, pk);
}

static int mldsa65_keygen(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_dilithium3_ref_keypair(pk, sk);
}
static int mldsa65_sign(uint8_t *sig, size_t *siglen,
                        const uint8_t *msg, size_t msglen,
                        const uint8_t *sk) {
    return pqcrystals_dilithium3_ref_signature(sig, siglen, msg, msglen, NULL, 0, sk);
}
static int mldsa65_verify(const uint8_t *sig, size_t siglen,
                          const uint8_t *msg, size_t msglen,
                          const uint8_t *pk) {
    return pqcrystals_dilithium3_ref_verify(sig, siglen, msg, msglen, NULL, 0, pk);
}

static int mldsa87_keygen(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_dilithium5_ref_keypair(pk, sk);
}
static int mldsa87_sign(uint8_t *sig, size_t *siglen,
                        const uint8_t *msg, size_t msglen,
                        const uint8_t *sk) {
    return pqcrystals_dilithium5_ref_signature(sig, siglen, msg, msglen, NULL, 0, sk);
}
static int mldsa87_verify(const uint8_t *sig, size_t siglen,
                          const uint8_t *msg, size_t msglen,
                          const uint8_t *pk) {
    return pqcrystals_dilithium5_ref_verify(sig, siglen, msg, msglen, NULL, 0, pk);
}

/* ---------- exported scheme table ---------- */

bench_scheme_t mldsa_schemes[] = {
    {
        .name       = "ML-DSA-44",
        .pk_bytes   = 1312,
        .sk_bytes   = 2560,
        .sig_bytes  = 2420,
        .keygen_fn  = mldsa44_keygen,
        .sign_fn    = mldsa44_sign,
        .verify_fn  = mldsa44_verify,
    },
    {
        .name       = "ML-DSA-65",
        .pk_bytes   = 1952,
        .sk_bytes   = 4032,
        .sig_bytes  = 3309,
        .keygen_fn  = mldsa65_keygen,
        .sign_fn    = mldsa65_sign,
        .verify_fn  = mldsa65_verify,
    },
    {
        .name       = "ML-DSA-87",
        .pk_bytes   = 2592,
        .sk_bytes   = 4896,
        .sig_bytes  = 4627,
        .keygen_fn  = mldsa87_keygen,
        .sign_fn    = mldsa87_sign,
        .verify_fn  = mldsa87_verify,
    },
};

const size_t n_mldsa_schemes = sizeof(mldsa_schemes) / sizeof(mldsa_schemes[0]);
