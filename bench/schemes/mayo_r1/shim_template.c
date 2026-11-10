/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file.
 * MAYO_VARIANT is provided by the Makefile via -D (compile-time parameters; the params
 * pointer is unused then, &@VARIANT@ is what upstream's api.c passes). Round-1 MAYO-C has
 * no detached signing call: wrap mayo_sign (sm = sig || m). mayo_verify is detached. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mayo.h"
#include "../../scheme.h"

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    return mayo_keypair(&@VARIANT@, pk, sk);
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    unsigned char *sm = malloc(@SIG@ + mlen);
    if (!sm) return -1;
    unsigned long long smlen;
    int ret = mayo_sign(&@VARIANT@, sm, &smlen, m, (unsigned long long)mlen, sk);
    if (ret == MAYO_OK) {
        memcpy(sig, sm, @SIG@);
        *siglen = @SIG@;
    }
    free(sm);
    return ret;
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (siglen != (size_t)@SIG@) return -1;
    return mayo_verify(&@VARIANT@, m, (unsigned long long)mlen, sig,
                       (unsigned long long)siglen, pk);
}
