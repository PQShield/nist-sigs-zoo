/* UOV shim for @NAME@. Generated — do not edit. */
#include <stdint.h>
#include <errno.h>
#include <sys/random.h>
/* utils_randombytes.h defines: #define randombytes PQOV_NAMESPACE(randombytes)
 * so our definition below gets the correct namespaced name. */
#include "ref/utils/utils_randombytes.h"
#include "../../scheme.h"

void randombytes(unsigned char *x, unsigned long long xlen) {
    uint8_t *p = x; size_t len = (size_t)xlen;
    while (len > 0) {
        ssize_t r = getrandom(p, len, 0);
        if (r > 0) { p += (size_t)r; len -= (size_t)r; }
        else if (errno != EINTR) break;
    }
}

/* Declare the namespaced functions exported by the compiled library. */
extern int PQOV_NAMESPACE(keypair)(unsigned char *pk, unsigned char *sk);
extern int PQOV_NAMESPACE(signature)(unsigned char *sig, unsigned long long *siglen,
                                     const unsigned char *m, unsigned long long mlen,
                                     const unsigned char *sk);
extern int PQOV_NAMESPACE(verify)(const unsigned char *sig, unsigned long long siglen,
                                   const unsigned char *m, unsigned long long mlen,
                                   const unsigned char *pk);

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return PQOV_NAMESPACE(keypair)(pk, sk);
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    unsigned long long sl;
    int ret = PQOV_NAMESPACE(signature)(sig, &sl, m, (unsigned long long)mlen, sk);
    *siglen = (size_t)sl;
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    return PQOV_NAMESPACE(verify)(sig, (unsigned long long)siglen,
                                   m, (unsigned long long)mlen, pk);
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
