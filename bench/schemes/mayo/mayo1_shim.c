#define MAYO_VARIANT    MAYO_1
#include "mayo.h"
#include "../../scheme.h"

static const bench_scheme_info_t INFO = {
    "MAYO-one",
    MAYO_1_cpk_bytes,
    MAYO_1_csk_bytes,
    MAYO_1_sig_bytes,
    0
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return mayo_keypair(NULL, pk, sk);
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    return mayo_sign_signature(NULL, sig, siglen, m, mlen, sk);
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (siglen != MAYO_1_sig_bytes) return -1;
    return mayo_verify(NULL, m, mlen, sig, pk);
}
