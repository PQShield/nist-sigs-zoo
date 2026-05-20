/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>
#include "../../scheme.h"
#include "ref/slh_dsa.h"

static int rbg(uint8_t *x, size_t xlen) {
    return (ssize_t)getrandom(x, xlen, 0) == (ssize_t)xlen ? 0 : -1;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return slh_keygen(sk, pk, rbg, &@PARAM@);
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    *siglen = slh_sign(sig, m, mlen, NULL, 0, sk, NULL, &@PARAM@);
    return *siglen == 0 ? -1 : 0;
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    return slh_verify(m, mlen, sig, siglen, NULL, 0, pk, &@PARAM@) == 1 ? 0 : -1;
}
