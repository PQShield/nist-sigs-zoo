/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stdint.h>
#include "../../scheme.h"

/*
 * NTRU (jschanck/ntru) renames the NIST KEM API via CRYPTO_NAMESPACE: with
 * NTRU_NAMESPACE=ntru_ the exported symbols are ntru_{keypair,enc,dec} (and the
 * generated AVX2 assembly carries the same prefix). The shim re-exports the bare
 * crypto_kem_* names the loader expects. enc/dec already match the NIST argument
 * order; keypair returns int. RTLD_LOCAL isolates the four sets.
 */
int ntru_keypair(unsigned char *pk, unsigned char *sk);
int ntru_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int ntru_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    return ntru_keypair(pk, sk);
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return ntru_enc(ct, ss, pk);
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return ntru_dec(ss, ct, sk);
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
