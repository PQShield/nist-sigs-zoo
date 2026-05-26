#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Generated per-variant header defines CRYPTO_BYTES, CRYPTO_SECRETKEYBYTES,
 * CRYPTO_PUBLICKEYBYTES and declares crypto_sign_keypair/crypto_sign/
 * crypto_sign_open (extern "C" in the companion api.cpp). */
#include "@FILE@_api.h"
#include "../../scheme.h"

/* crypto_sign_keypair is already provided by the companion api.cpp.
 * The arch-opt layout is sm = m ‖ sig (message first, signature appended). */

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    *siglen = CRYPTO_BYTES;
    uint8_t *sm = malloc(CRYPTO_BYTES + mlen);
    if (!sm) return -1;
    unsigned long long smlen;
    int ret = crypto_sign(sm, &smlen, m, (unsigned long long)mlen, sk);
    if (ret == 0)
        memcpy(sig, sm + mlen, CRYPTO_BYTES);
    free(sm);
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    (void)siglen;
    uint8_t *sm = malloc(mlen + CRYPTO_BYTES);
    if (!sm) return -1;
    memcpy(sm, m, mlen);
    memcpy(sm + mlen, sig, CRYPTO_BYTES);
    uint8_t *m_out = malloc(mlen + 1);
    if (!m_out) { free(sm); return -1; }
    unsigned long long mlen_out;
    int ret = crypto_sign_open(m_out, &mlen_out, sm,
                               mlen + (unsigned long long)CRYPTO_BYTES, pk);
    free(m_out);
    free(sm);
    return ret;
}

static const bench_scheme_info_t INFO = {
    "@NAME@", @PK@, @SK@, @SIG@, @ITERS@
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }
