/* SDitH Hypercube shim for @NAME@. Generated — edit params_hypercube.tsv +
 * shim_hypercube_template.c, not this file.
 *
 * The Hypercube variant only exposes the combined NIST API (crypto_sign /
 * crypto_sign_open); crypto_sign_keypair is provided by sign.c in the library.
 * This shim adds the detached crypto_sign_signature / crypto_sign_verify wrappers
 * needed by the harness, and provides randombytes() via getrandom(). */
#include <stdint.h>
#include <string.h>
#include <sys/random.h>
#include "api.h"
#include "../../scheme.h"

/* Provided to satisfy references from sdith.c / sign.c internals. */
int randombytes(unsigned char *x, unsigned long long xlen) {
    return getrandom(x, (size_t)xlen, 0) == (ssize_t)xlen ? 0 : -1;
}

/* crypto_sign_keypair: defined in sign.c (linked via library), exported as-is. */

/* Wrap combined sign → detached sign used by harness. */
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

/* Wrap combined open → detached verify used by harness. */
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
