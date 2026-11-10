/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 * MAYO_VARIANT is provided by the Makefile via -D; mayo_keypair/sign/verify expand
 * to the namespaced AVX2 functions via the MAYO_NAMESPACE() macro in mayo.h. */
#include "mayo.h"
#include "../../scheme.h"

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
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
    if (siglen != (size_t)@SIG@) return -1;
    return mayo_verify(NULL, m, mlen, sig, pk);
}
