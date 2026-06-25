/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 *
 * HQC's reference (https://gitlab.com/pqc-hqc/hqc) exports the BARE,
 * non-namespaced NIST KEM symbols (crypto_kem_keypair / crypto_kem_enc /
 * crypto_kem_dec). Each HQC variant is built into its own .so and loaded with
 * dlopen(RTLD_LOCAL), so the identical bare symbols across HQC-128/192/256 (and
 * any other scheme) never collide. The upstream object files are linked into
 * this .so, so the three required crypto_kem_* names are already exported; we
 * only add bench_info() and PRNG seeding.
 *
 * Seeding: HQC keeps a global PRNG that MUST be seeded via prng_init() before
 * crypto_kem_keypair(); otherwise the generated keys are predictable. The bench
 * harness calls crypto_kem_keypair() directly, so we seed once at .so load via
 * a constructor using OS entropy. This avoids renaming the bare upstream
 * crypto_kem_keypair (no symbol clash).
 *
 * If a future upstream namespaces its symbols (e.g. PQCLEAN_HQC128_CLEAN_*),
 * add an @NS_PREFIX@ column to params.tsv, declare the namespaced symbols
 * `extern` here, and wrap them with the bare names below. */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

/* Upstream PRNG (declared in src/common/symmetric.h). */
void prng_init(uint8_t *entropy_input, uint8_t *personalization_string,
               uint32_t enlen, uint32_t perlen);

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

/* OS entropy via getentropy() (libc on Linux glibc >= 2.25 and macOS). */
#if defined(__APPLE__)
#include <sys/random.h>
#else
#include <unistd.h>
#if defined(__GLIBC__)
#include <sys/random.h>
#endif
#endif

__attribute__((constructor))
static void seed_hqc_prng(void) {
    uint8_t entropy[48] = {0};
    if (getentropy(entropy, sizeof(entropy)) != 0) {
        /* Last-resort: leave the zero buffer (deterministic but functional). */
    }
    prng_init(entropy, NULL, (uint32_t)sizeof(entropy), 0);
}
