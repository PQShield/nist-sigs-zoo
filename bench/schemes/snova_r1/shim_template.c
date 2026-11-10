/* SNOVA Round 1 shim for @NAME@. Generated — do not edit. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/types.h>
#include "api.h"
#include "../../scheme.h"

/* Sizes from the code; params.tsv holds the YAML (spec) values and must agree. */
_Static_assert(CRYPTO_PUBLICKEYBYTES == @PK@, "pk size differs from params.tsv");
_Static_assert(CRYPTO_SECRETKEYBYTES == @SK@, "sk size differs from params.tsv");
_Static_assert(CRYPTO_BYTES == @SIG@, "sig size differs from params.tsv");

int randombytes(unsigned char *x, unsigned long long xlen) {
    return getrandom(x, (size_t)xlen, 0) == (ssize_t)xlen ? 0 : -1;
}

/* Round 1 needs its GF(16) and S tables built once before any call (upstream's
 * PQCgenKAT_sign.c and tests call snova_init() first); without it signing never
 * finds a pivot and loops forever. */
__attribute__((constructor)) static void shim_snova_init(void) { snova_init(); }

/* crypto_sign_keypair: defined in sign.c (library), exported as-is. */

/* Old NIST combined API: sm = sig || m. */
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    unsigned char *sm = malloc(CRYPTO_BYTES + mlen);
    unsigned long long smlen;
    if (!sm) return -1;
    int ret = crypto_sign(sm, &smlen, m, (unsigned long long)mlen, sk);
    if (ret == 0) {
        *siglen = (size_t)smlen - mlen;
        memcpy(sig, sm, *siglen);
    }
    free(sm);
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    unsigned char *sm = malloc(siglen + mlen);
    unsigned char *mout = malloc(siglen + mlen);
    unsigned long long moutlen;
    int ret = -1;
    if (sm && mout) {
        memcpy(sm, sig, siglen);
        memcpy(sm + siglen, m, mlen);
        ret = crypto_sign_open(mout, &moutlen, sm,
                               (unsigned long long)(siglen + mlen), pk);
    }
    free(sm);
    free(mout);
    return ret;
}

static const bench_scheme_info_t INFO = {
    "@NAME@", CRYPTO_PUBLICKEYBYTES, CRYPTO_SECRETKEYBYTES, CRYPTO_BYTES, @ITERS@
};
const bench_scheme_info_t *bench_info(void) { return &INFO; }
