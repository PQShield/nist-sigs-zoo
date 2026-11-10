/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stdint.h>
#include <errno.h>
#include <sys/random.h>
#include "hawk.h"
#include "../../scheme.h"

#define LOGN @LOGN@

static void rng_cb(void *ctx, void *dst, size_t len) {
    (void)ctx;
    uint8_t *p = dst;
    while (len > 0) {
        ssize_t r = getrandom(p, len, 0);
        if (r > 0) { p += (size_t)r; len -= (size_t)r; }
        else if (errno != EINTR) break;
    }
}

static const bench_scheme_info_t INFO = {
    "@NAME@",
    HAWK_PUBKEY_SIZE(LOGN),
    HAWK_PRIVKEY_SIZE(LOGN),
    HAWK_SIG_SIZE(LOGN),
    @ITERS@
};
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    uint8_t tmp[HAWK_TMPSIZE_KEYGEN(LOGN)];
    return hawk_keygen(LOGN, sk, pk, rng_cb, NULL, tmp, sizeof tmp) == 1 ? 0 : -1;
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    uint8_t tmp[HAWK_TMPSIZE_SIGN(LOGN)];
    shake_context sc;
    hawk_sign_start(&sc);
    shake_inject(&sc, m, mlen);
    if (!hawk_sign_finish(LOGN, rng_cb, NULL, sig, &sc, sk, tmp, sizeof tmp))
        return -1;
    *siglen = HAWK_SIG_SIZE(LOGN);
    return 0;
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    uint8_t tmp[HAWK_TMPSIZE_VERIFY_FAST(LOGN)];
    shake_context sc;
    hawk_verify_start(&sc);
    shake_inject(&sc, m, mlen);
    return hawk_verify_finish(LOGN, sig, siglen, &sc,
                              pk, HAWK_PUBKEY_SIZE(LOGN),
                              tmp, sizeof tmp) == 1 ? 0 : -1;
}
