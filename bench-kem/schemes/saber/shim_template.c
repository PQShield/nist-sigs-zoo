/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>
#include "../../scheme.h"

/*
 * SABER's AVX2 reference already exports the bare NIST KEM API
 * (crypto_kem_keypair/enc/dec) from its own objects compiled into this .so, so
 * the loader resolves those directly — no wrapping needed. The shim only:
 *   1. provides bench_info() metadata, and
 *   2. seeds the global AES256-CTR-DRBG behind randombytes() via
 *      randombytes_init, which upstream otherwise expects the NIST KAT
 *      harness to call — without it every draw comes from a zero key/IV.
 * RTLD_LOCAL keeps the three parameter sets' identical bare symbols isolated.
 */
void randombytes_init(unsigned char *entropy_input, unsigned char *personalization_string, int security_strength);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

__attribute__((constructor))
static void seed_prng(void) {
    unsigned char entropy[48];
    if (getrandom(entropy, sizeof entropy, 0) != (ssize_t)sizeof entropy) {
        for (size_t i = 0; i < sizeof entropy; i++) entropy[i] = (unsigned char)(i + 1);
    }
    randombytes_init(entropy, NULL, 256);
}
