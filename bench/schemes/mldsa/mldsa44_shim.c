/*
 * Shim for ML-DSA-44 (dilithium mode 2, pq-crystals/dilithium AVX2).
 * Exports the standard crypto_sign_* names + bench_info().
 * Built into a self-contained .so; dilithium mode-2 objects are linked in.
 */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

int pqcrystals_dilithium2_avx2_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_dilithium2_avx2_signature(uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *sk);
int pqcrystals_dilithium2_avx2_verify(const uint8_t *sig, size_t siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *pk);

static const bench_scheme_info_t INFO = {
    "ML-DSA-44", 1312, 2560, 2420, 0
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_dilithium2_avx2_keypair(pk, sk);
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    return pqcrystals_dilithium2_avx2_signature(sig, siglen, m, mlen, NULL, 0, sk);
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    return pqcrystals_dilithium2_avx2_verify(sig, siglen, m, mlen, NULL, 0, pk);
}
