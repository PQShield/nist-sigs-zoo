/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

/* SMAUG-T namespaces its API per mode: @NS@crypto_kem_* (set via
 * -DSMAUG_MODE / -DTiMER_MODE in the Makefile). RTLD_LOCAL keeps the .so's
 * identical bare-named wrappers below isolated. */
/* The upstream keypair is defined `void` (kem/src/kem.c) although api.h declares
 * it `int`: declare what is defined and report success, instead of returning an
 * undefined value. */
void @NS@crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
int @NS@crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int @NS@crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    @NS@crypto_kem_keypair(pk, sk);
    return 0;
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return @NS@crypto_kem_enc(ct, ss, pk);
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return @NS@crypto_kem_dec(ss, ct, sk);
}
