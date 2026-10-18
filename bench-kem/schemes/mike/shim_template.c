/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../../scheme.h"

/*
 * MIKE is a non-interactive key exchange, not a KEM. Standard NIKE-to-KEM
 * wrapping (as for ECDH in ../classic): encaps generates an ephemeral keypair
 * (ct = ephemeral pk) and derives ss = exchange(pk, esk); decaps derives
 * ss = exchange(ct, sk). So encaps costs keygen + one exchange, decaps one
 * exchange. Upstream symbols are namespaced per prime and build type, and
 * upstream returns 1 on success / 0 on failure (the opposite of the NIST KEM API).
 */
int mike_@VARIANT@_broadwell_mike_keypair(unsigned char *pk, unsigned char *sk);
int mike_@VARIANT@_broadwell_mike_exchange(unsigned char *shared, const unsigned char *pk, const unsigned char *sk);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    return mike_@VARIANT@_broadwell_mike_keypair(pk, sk) ? 0 : -1;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    uint8_t esk[@SK@];
    if (!mike_@VARIANT@_broadwell_mike_keypair(ct, esk)) return -1;
    return mike_@VARIANT@_broadwell_mike_exchange(ss, pk, esk) ? 0 : -1;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    return mike_@VARIANT@_broadwell_mike_exchange(ss, ct, sk) ? 0 : -1;
}
