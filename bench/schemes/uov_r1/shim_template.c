/* UOV Round 1 shim for @NAME@. Generated — do not edit. */
/* This pqov version has the old NIST combined API (crypto_sign / crypto_sign_open,
 * sm = m || sig) and no namespacing. The Makefile renames upstream's three API
 * symbols to uov_* when compiling upstream's sources (not this shim), so that this
 * shim can export the bench contract's plain names. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/random.h>
#include "params.h"
#include "../../scheme.h"

_Static_assert(@PK@ == OV_PUBLICKEYBYTES, "params.tsv pk != OV_PUBLICKEYBYTES");
_Static_assert(@SK@ == OV_SECRETKEYBYTES, "params.tsv sk != OV_SECRETKEYBYTES");
_Static_assert(@SIG@ == OV_SIGNATUREBYTES, "params.tsv sig != OV_SIGNATUREBYTES");

/* utils_randombytes.c is not built: upstream calls randombytes(), provided here. */
void randombytes(unsigned char *x, unsigned long long xlen) {
    uint8_t *p = x; size_t len = (size_t)xlen;
    while (len > 0) {
        ssize_t r = getrandom(p, len, 0);
        if (r > 0) { p += (size_t)r; len -= (size_t)r; }
        else if (errno != EINTR) break;
    }
}

extern int uov_crypto_sign_keypair(unsigned char *pk, unsigned char *sk);
extern int uov_crypto_sign(unsigned char *sm, unsigned long long *smlen,
                           const unsigned char *m, unsigned long long mlen,
                           const unsigned char *sk);
extern int uov_crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                                const unsigned char *sm, unsigned long long smlen,
                                const unsigned char *pk);

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return uov_crypto_sign_keypair(pk, sk);
}

/* sm = m || sig (upstream's sign.c puts the signature at sm + mlen). */
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    unsigned char *sm = malloc(mlen + OV_SIGNATUREBYTES);
    if (!sm) return -1;
    unsigned long long smlen;
    int ret = uov_crypto_sign(sm, &smlen, m, (unsigned long long)mlen, sk);
    memcpy(sig, sm + mlen, OV_SIGNATUREBYTES);
    *siglen = OV_SIGNATUREBYTES;
    free(sm);
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    if (siglen != OV_SIGNATUREBYTES) return -1;
    unsigned char *sm = malloc(mlen + siglen);
    unsigned char *mo = malloc(mlen + siglen);
    if (!sm || !mo) { free(sm); free(mo); return -1; }
    memcpy(sm, m, mlen);
    memcpy(sm + mlen, sig, siglen);
    unsigned long long molen;
    int ret = uov_crypto_sign_open(mo, &molen, sm, (unsigned long long)(mlen + siglen), pk);
    free(sm);
    free(mo);
    return ret;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
