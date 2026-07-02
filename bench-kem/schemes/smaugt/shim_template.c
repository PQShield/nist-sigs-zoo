/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

/* SMAUG-T namespaces its API per mode: cryptolab_smaug@MODE@_crypto_kem_*
 * (set via -DSMAUG_MODE=@MODE@). RTLD_LOCAL keeps the three .so's identical
 * bare-named wrappers below isolated. */
int cryptolab_smaug@MODE@_crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
int cryptolab_smaug@MODE@_crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int cryptolab_smaug@MODE@_crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    return cryptolab_smaug@MODE@_crypto_kem_keypair(pk, sk);
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return cryptolab_smaug@MODE@_crypto_kem_enc(ct, ss, pk);
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return cryptolab_smaug@MODE@_crypto_kem_dec(ss, ct, sk);
}
