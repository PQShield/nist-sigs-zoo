/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

/* pq-code-package/mlkem-native API. The security level is fixed per .so at build
 * time (-DMLK_CONFIG_PARAMETER_SET=@LVL@); with namespace prefix "mlkem" the symbols
 * are mlkem_keypair/enc/dec (no level suffix). RTLD_LOCAL keeps the three .so's
 * identical names isolated. */
int mlkem_keypair(uint8_t *pk, uint8_t *sk);
int mlkem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int mlkem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    return mlkem_keypair(pk, sk);
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return mlkem_enc(ct, ss, pk);
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return mlkem_dec(ss, ct, sk);
}
