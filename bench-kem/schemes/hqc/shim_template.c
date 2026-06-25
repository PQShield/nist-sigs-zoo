/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>
#include "../../scheme.h"

/*
 * HQC already exports the bare NIST KEM API (crypto_kem_keypair/enc/dec) from its
 * own objects compiled into this .so, so the loader resolves those directly — no
 * wrapping needed. The shim only:
 *   1. provides bench_info() metadata, and
 *   2. seeds HQC's SHAKE-based PRNG (prng_init), which the upstream otherwise
 *      expects the NIST KAT harness to initialise. Without seeding, prng_get_bytes
 *      squeezes an unfinalised state.
 * RTLD_LOCAL keeps the three variants' identical bare symbols isolated.
 */
void prng_init(uint8_t *entropy, uint8_t *perso, uint32_t enlen, uint32_t perlen);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

__attribute__((constructor))
static void seed_prng(void) {
    uint8_t entropy[48];
    if (getrandom(entropy, sizeof entropy, 0) != (ssize_t)sizeof entropy) {
        /* Fallback: non-zero deterministic seed if getrandom is unavailable. */
        for (size_t i = 0; i < sizeof entropy; i++) entropy[i] = (uint8_t)(i + 1);
    }
    prng_init(entropy, entropy, (uint32_t)sizeof entropy, 0);
}
