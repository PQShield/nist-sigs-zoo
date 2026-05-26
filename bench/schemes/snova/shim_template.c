/* SNOVA shim for @NAME@. Generated — do not edit. */
#include <stdint.h>
#include <string.h>
#include <sys/random.h>
#include "ref/src/api.h"
#include "../../scheme.h"

int randombytes(unsigned char *x, unsigned long long xlen) {
    return getrandom(x, (size_t)xlen, 0) == (ssize_t)xlen ? 0 : -1;
}

/* crypto_sign_keypair: defined in sign.c (library), exported as-is. */

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    unsigned char sm[@SIG@ + 64];
    unsigned long long smlen;
    int ret = crypto_sign(sm, &smlen, m, (unsigned long long)mlen, sk);
    if (ret) return ret;
    size_t sl = (size_t)smlen - mlen;
    memcpy(sig, sm, sl);
    *siglen = sl;
    return 0;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    unsigned char sm[@SIG@ + 64];
    unsigned char mout[64];
    unsigned long long moutlen;
    memcpy(sm, sig, siglen);
    memcpy(sm + siglen, m, mlen);
    return crypto_sign_open(mout, &moutlen, sm,
                            (unsigned long long)(siglen + mlen), pk);
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
