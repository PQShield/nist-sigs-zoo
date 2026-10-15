/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 * HAETAE_CONFIG_MODE=@MODE@ is set by the Makefile via -D; api.h maps crypto_sign_*
 * to the namespaced cryptolab_haetae_modeN_* functions. The upstream API takes a
 * context string: we pass an empty one. */
#include "api.h"
#include "../../scheme.h"

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

/* Undo api.h's renames for the bench contract's plain names. */
#undef crypto_sign_keypair
#undef crypto_sign_signature
#undef crypto_sign_verify

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return HAETAE_NAMESPACE(keypair)(pk, sk);
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    return HAETAE_NAMESPACE(signature)(sig, siglen, m, mlen, NULL, 0, sk);
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    return HAETAE_NAMESPACE(verify)(sig, siglen, m, mlen, NULL, 0, pk);
}
