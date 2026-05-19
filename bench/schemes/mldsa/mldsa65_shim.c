/* Shim for ML-DSA-65 (dilithium mode 3). */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

int pqcrystals_dilithium3_avx2_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_dilithium3_avx2_signature(uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *sk);
int pqcrystals_dilithium3_avx2_verify(const uint8_t *sig, size_t siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *pk);

static const bench_scheme_info_t INFO = {
    "ML-DSA-65", 1952, 4032, 3309, 0
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_dilithium3_avx2_keypair(pk, sk);
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    return pqcrystals_dilithium3_avx2_signature(sig, siglen, m, mlen, NULL, 0, sk);
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    return pqcrystals_dilithium3_avx2_verify(sig, siglen, m, mlen, NULL, 0, pk);
}
