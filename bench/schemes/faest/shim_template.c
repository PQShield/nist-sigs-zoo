#include <stddef.h>
#include <stdint.h>
#include "faest_@PARAM_L@.h"
#include "../../scheme.h"

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return faest_@PARAM_L@_keygen(pk, sk);
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    *siglen = @SIG@;
    return faest_@PARAM_L@_sign(sk, m, mlen, sig, siglen);
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    return faest_@PARAM_L@_verify(pk, m, mlen, sig, siglen);
}

static const bench_scheme_info_t INFO = {
    "@NAME@", @PK@, @SK@, @SIG@, @ITERS@
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }
