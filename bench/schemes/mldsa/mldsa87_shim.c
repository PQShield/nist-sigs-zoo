/* Shim for ML-DSA-87 (dilithium mode 5). */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

int pqcrystals_dilithium5_ref_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_dilithium5_ref_signature(uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *sk);
int pqcrystals_dilithium5_ref_verify(const uint8_t *sig, size_t siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *ctx, size_t ctxlen,
    const uint8_t *pk);

static const bench_scheme_info_t INFO = {
    "ML-DSA-87", 2592, 4896, 4627, 0
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_dilithium5_ref_keypair(pk, sk);
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    return pqcrystals_dilithium5_ref_signature(sig, siglen, m, mlen, NULL, 0, sk);
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    return pqcrystals_dilithium5_ref_verify(sig, siglen, m, mlen, NULL, 0, pk);
}
