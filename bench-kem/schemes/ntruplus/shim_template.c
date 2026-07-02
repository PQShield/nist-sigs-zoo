/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

/*
 * NTRU+ already exports the bare NIST KEM API (crypto_kem_keypair/enc/dec) from its
 * own objects compiled into this .so, so the loader resolves those directly — no
 * wrapping needed. Its bundled randombytes.c is getrandom-backed and self-seeding,
 * so no PRNG constructor is needed either. The shim only supplies bench_info().
 * RTLD_LOCAL keeps the three sets' identical bare symbols isolated.
 */
static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
