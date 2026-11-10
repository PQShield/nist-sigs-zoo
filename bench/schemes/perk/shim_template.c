/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 * Built from Optimized_Implementation/aes_aes/@DIR@. Upstream's sign.c provides
 * crypto_sign_keypair and the combined NIST API crypto_sign/crypto_sign_open with
 * size_t lengths (sm = sig || m); we wrap the latter two. lib/randombytes/rng.c (NIST
 * AES-CTR DRBG, OpenSSL) is not compiled; randombytes() comes from getrandom() here
 * (prototype from src/randombytes.h). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include "api.h"
#include "randombytes.h"
#include "../../scheme.h"

int randombytes(uint8_t *out, size_t outlen) {
    while (outlen > 0) {
        ssize_t r = getrandom(out, outlen, 0);
        if (r <= 0) return -1;
        out += r;
        outlen -= (size_t)r;
    }
    return 0;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    size_t smlen;
    uint8_t *sm = malloc(CRYPTO_BYTES + mlen);
    if (!sm) return -1;
    int ret = crypto_sign(sm, &smlen, m, mlen, sk);
    if (ret == 0) {
        memcpy(sig, sm, CRYPTO_BYTES);
        *siglen = CRYPTO_BYTES;
    }
    free(sm);
    return ret;
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (siglen != CRYPTO_BYTES) return -1;
    size_t outlen;
    uint8_t *sm = malloc(CRYPTO_BYTES + mlen);
    uint8_t *out = malloc(CRYPTO_BYTES + mlen);
    int ret = -1;
    if (sm && out) {
        memcpy(sm, sig, CRYPTO_BYTES);
        memcpy(sm + CRYPTO_BYTES, m, mlen);
        ret = crypto_sign_open(out, &outlen, sm, CRYPTO_BYTES + mlen, pk);
    }
    free(sm);
    free(out);
    return ret;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
