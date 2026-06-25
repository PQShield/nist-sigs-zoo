/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 *
 * Adapts pq-code-package/mlkem-native (AVX2 backend) to the bench KEM contract.
 * mlkem-native is built one .so per security level with
 * -DMLK_CONFIG_PARAMETER_SET=@VARIANT@ and the SUPERCOP API enabled, so its
 * kem.c already exports the bare NIST symbols crypto_kem_keypair/enc/dec
 * (mapped onto the single-level namespace). The loader resolves those directly,
 * so this shim only needs to add bench_info() and a real randombytes().
 *
 * In a single-level build the exported names are identical across levels
 * (crypto_kem_keypair etc.); per-.so dlopen(RTLD_LOCAL) isolation keeps the
 * three levels (and any other scheme) from colliding.
 *
 * mlkem-native calls randombytes() during keygen/encaps; we provide a real one
 * backed by the OS RNG (the upstream test_only_rng stub is deliberately unsafe
 * and must not be linked here). */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"

#if defined(__APPLE__)
#include <sys/random.h>
#else
#include <unistd.h>
#if defined(__GLIBC__)
#include <sys/random.h>
#endif
#endif

/* mlkem-native's RNG hook (declared in mlkem/src/randombytes.h). 0 = success. */
int randombytes(uint8_t *out, size_t outlen) {
    size_t off = 0;
    while (off < outlen) {
        size_t chunk = outlen - off;
        if (chunk > 256) chunk = 256;  /* getentropy max request */
        if (getentropy(out + off, chunk) != 0) return -1;
        off += chunk;
    }
    return 0;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
