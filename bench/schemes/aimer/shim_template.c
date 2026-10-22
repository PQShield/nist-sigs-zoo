/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 * PARAMS=@PARAM@ is set by the Makefile via -D; api.h maps crypto_sign_* to the
 * namespaced samsungsds_aimer_@PARAM@_ref_* functions. The upstream FIPS 204-style
 * API takes a context string: we pass an empty one. */
#include <sys/random.h>
#include "api.h"
#include "../../scheme.h"

/* sign.c calls randombytes (keygen; signing is deterministic by default). The
 * upstream common/rng.c is the NIST AES-CTR KAT DRBG and is not linked. */
int randombytes(unsigned char *x, unsigned long long xlen) {
    size_t off = 0;
    while (off < xlen) {
        ssize_t r = getrandom(x + off, (size_t)xlen - off, 0);
        if (r <= 0) return -1;
        off += (size_t)r;
    }
    return 0;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

/* Undo api.h's renames for the bench contract's plain names. */
#undef crypto_sign_keypair
#undef crypto_sign_signature
#undef crypto_sign_verify

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return AIMER_NAMESPACE(crypto_sign_keypair)(pk, sk);
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    return AIMER_NAMESPACE(crypto_sign_signature)(sig, siglen, m, mlen, NULL, 0, sk);
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    return AIMER_NAMESPACE(crypto_sign_verify)(sig, siglen, m, mlen, NULL, 0, pk);
}
