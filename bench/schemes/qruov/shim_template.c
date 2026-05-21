/* QR-UOV shim for @NAME@. Generated — do not edit. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include "../../scheme.h"

/* Replace NIST KAT DRBG with getrandom(). */
int randombytes(unsigned char *x, unsigned long long xlen) {
    getrandom(x, (size_t)xlen, 0);
    return 0;
}

/* Combined-API symbols from sign.c (compiled into libqruov.a). */
extern int crypto_sign_keypair(unsigned char *pk, unsigned char *sk);
extern int crypto_sign(unsigned char *sm, unsigned long long *smlen,
                       const unsigned char *m, unsigned long long mlen,
                       const unsigned char *sk);
extern int crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                            const unsigned char *sm, unsigned long long smlen,
                            const unsigned char *pk);

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    uint8_t *sm = malloc(@SIG@ + mlen);
    if (!sm) return -1;
    unsigned long long sml;
    int ret = crypto_sign(sm, &sml, m, (unsigned long long)mlen, sk);
    if (ret == 0) {
        memcpy(sig, sm, @SIG@);
        *siglen = @SIG@;
    }
    free(sm);
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    (void)siglen;
    uint8_t *sm = malloc(@SIG@ + mlen);
    if (!sm) return -1;
    uint8_t *out = malloc(@SIG@ + mlen);
    if (!out) { free(sm); return -1; }
    memcpy(sm, sig, @SIG@);
    memcpy(sm + @SIG@, m, mlen);
    unsigned long long msglen;
    int ret = crypto_sign_open(out, &msglen, sm,
                               (unsigned long long)(@SIG@ + mlen), pk);
    free(sm);
    free(out);
    return ret;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
