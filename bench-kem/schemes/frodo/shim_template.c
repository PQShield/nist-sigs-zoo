/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stdint.h>
#include "../../scheme.h"

/*
 * FrodoKEM (Microsoft/PQCrypto-LWEKE) renames its KEM API per parameter set via
 * macros in frodo<level>.c, so each object exports crypto_kem_*_@SUFFIX@ rather
 * than the bare NIST names. The shim re-exports the bare crypto_kem_* that the
 * loader expects. RTLD_LOCAL keeps the six variants' symbols isolated, and the
 * AES vs SHAKE matrix-A method is selected per .so by a compile-time define.
 *
 * Upstream's randombytes (common/random/random.c, /dev/urandom-backed) is linked
 * in and self-initialises, so no constructor seeding is needed here.
 */
int crypto_kem_keypair_@SUFFIX@(unsigned char *pk, unsigned char *sk);
int crypto_kem_enc_@SUFFIX@(unsigned char *ct, unsigned char *ss, const unsigned char *pk);
int crypto_kem_dec_@SUFFIX@(unsigned char *ss, const unsigned char *ct, const unsigned char *sk);

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    return crypto_kem_keypair_@SUFFIX@(pk, sk);
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return crypto_kem_enc_@SUFFIX@(ct, ss, pk);
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return crypto_kem_dec_@SUFFIX@(ss, ct, sk);
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
