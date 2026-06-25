/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

/* pq-crystals/kyber avx2 API — namespace @NS@ selected by -DKYBER_K=@K@. */
int pqcrystals_kyber@NS@_avx2_keypair(uint8_t *pk, uint8_t *sk);
int pqcrystals_kyber@NS@_avx2_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int pqcrystals_kyber@NS@_avx2_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    return pqcrystals_kyber@NS@_avx2_keypair(pk, sk);
}
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    return pqcrystals_kyber@NS@_avx2_enc(ct, ss, pk);
}
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return pqcrystals_kyber@NS@_avx2_dec(ss, ct, sk);
}
