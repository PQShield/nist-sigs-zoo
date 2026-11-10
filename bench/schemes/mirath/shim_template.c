/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 * Built from Optimized_Implementation/mirath_tcith/@DIR@ (Rijndael commitments).
 * Calls mirath_keygen/mirath_sign/mirath_verify directly: upstream's sign.c only
 * wraps them in the combined NIST API (mirath_sign already writes a detached
 * signature to its first argument). rng.c (NIST AES-CTR DRBG, OpenSSL) is not
 * compiled; randombytes() comes from getrandom() here. */
#include <stdint.h>
#include <stddef.h>
#include <sys/random.h>
#include "mirath_keygen.h"
#include "mirath_sign.h"
#include "mirath_verify.h"
#include "../../scheme.h"

int randombytes(unsigned char *x, unsigned long long xlen) {
    while (xlen > 0) {
        ssize_t r = getrandom(x, (size_t)xlen, 0);
        if (r <= 0) return -1;
        x += r;
        xlen -= (unsigned long long)r;
    }
    return 0;
}

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return mirath_keygen(pk, sk) == 0 ? 0 : -1;
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    if (mirath_sign(sig, (uint8_t *)m, mlen, (uint8_t *)sk) != 0) return -1;
    *siglen = @SIG@;
    return 0;
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (siglen != @SIG@) return -1;
    return mirath_verify((uint8_t *)m, mlen, (uint8_t *)sig, (uint8_t *)pk) == 0 ? 0 : -1;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
