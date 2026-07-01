/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stdint.h>
#include "../../scheme.h"

/*
 * libntruprime exports per-parameter-set, IFUNC-dispatched symbols
 * ntruprime_kem_<set>_{keypair,enc,dec} (AVX2 selected at runtime). The enc/dec
 * argument order already matches the NIST KEM API; all three return void, so the
 * shim wraps them to return 0. libntruprime.a + librandombytes-kernel.a are
 * statically linked into this .so, so the bare crypto_kem_* names below are the
 * only KEM symbols it exports and RTLD_LOCAL keeps the parameter sets isolated.
 */
extern void ntruprime_kem_@SET@_keypair(unsigned char *pk, unsigned char *sk);
extern void ntruprime_kem_@SET@_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
extern void ntruprime_kem_@SET@_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    ntruprime_kem_@SET@_keypair(pk, sk);
    return 0;
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    ntruprime_kem_@SET@_enc(ct, ss, pk);
    return 0;
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    ntruprime_kem_@SET@_dec(ss, ct, sk);
    return 0;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
