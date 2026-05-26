/* SDitH Threshold shim for @NAME@. Generated — edit params_threshold.tsv +
 * shim_threshold_template.c, not this file.
 *
 * The Threshold variant exposes crypto_sign_signature / crypto_sign_verify with
 * unsigned-long-long length types (NIST PQC convention), but the harness uses
 * size_t.  This shim provides size_t-typed wrappers, plus randombytes().
 * crypto_sign_keypair (from keygen.c in the library) is exported as-is. */
#include <stdint.h>
#include <sys/random.h>
#include "parameters.h"
#include "sign-mpcith.h"
#include "../../scheme.h"

/* Provided to satisfy references from keygen.c / sign-mpcith-threshold-nfpr.c. */
int randombytes(unsigned char *x, unsigned long long xlen) {
    return getrandom(x, (size_t)xlen, 0) == (ssize_t)xlen ? 0 : -1;
}

/* crypto_sign_keypair: defined in keygen.c (linked via library), exported as-is. */

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    uint8_t salt[PARAM_SALT_SIZE];
    randombytes(salt, PARAM_SALT_SIZE);
    uint8_t seed[PARAM_SEED_SIZE];
    randombytes(seed, PARAM_SEED_SIZE);
    size_t sl = 0;
    int ret = sdith_sign(sig, &sl, m, mlen, sk, salt, seed);
    *siglen = sl;
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    return sdith_sign_verify(sig, siglen, m, mlen, pk);
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
