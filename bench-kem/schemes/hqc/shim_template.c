/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>
#include "../../scheme.h"

/*
 * HQC v5.1 exports hqc_kem_keygen/encaps/decaps and expects the integrator to
 * provide randombytes() from a secure RBG. The shim:
 *   1. provides bench_info() metadata,
 *   2. maps the bare NIST KEM API (crypto_kem_keypair/enc/dec) that the loader
 *      resolves onto HQC's functions (note HQC's argument order), and
 *   3. backs randombytes() with getrandom().
 * RTLD_LOCAL keeps the three variants' identical bare symbols isolated.
 */
int8_t hqc_kem_keygen(uint8_t *ek_kem, uint8_t *dk_kem);
int8_t hqc_kem_encaps(uint8_t *K, uint8_t *c_kem, const uint8_t *ek_kem);
int8_t hqc_kem_decaps(uint8_t *K_prime, const uint8_t *dk_kem, const uint8_t *c_kem);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int randombytes(uint8_t *out, size_t outlen) {
    while (outlen > 0) {
        ssize_t n = getrandom(out, outlen, 0);
        if (n <= 0) return -1;
        out += n;
        outlen -= (size_t)n;
    }
    return 0;
}

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) { return hqc_kem_keygen(pk, sk); }
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) { return hqc_kem_encaps(ss, ct, pk); }
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) { return hqc_kem_decaps(ss, sk, ct); }
