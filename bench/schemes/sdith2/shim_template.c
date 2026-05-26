/* SDitH2 bench shim for @NAME@. Generated — edit params.tsv +
 * shim_template.c, not this file.
 *
 * SDitH2 uses a custom API (sdith_keygen/sdith_sign/sdith_verify) with
 * explicit entropy and scratch-space arguments.  This shim wraps it into
 * the bench contract using getrandom() and malloc(). */
#include <stdint.h>
#include <stdlib.h>
#include <sys/random.h>
#include "impl/src/sdith_signature.h"
#include "../../scheme.h"

int randombytes(unsigned char *x, unsigned long long xlen) {
    return getrandom(x, (size_t)xlen, 0) == (ssize_t)xlen ? 0 : -1;
}

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    const signature_parameters *p = &@PARAM_SET@;
    uint64_t ent  = sdith_keygen_entropy_bytes(p);
    uint64_t tmp  = sdith_keygen_tmp_bytes(p);
    uint8_t *ebuf = malloc(ent);
    uint8_t *tbuf = malloc(tmp);
    getrandom(ebuf, ent, 0);
    sdith_keygen(p, sk, pk, ebuf, tbuf);
    free(ebuf); free(tbuf);
    return 0;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    const signature_parameters *p = &@PARAM_SET@;
    uint64_t ent  = sdith_signature_entropy_bytes(p);
    uint64_t tmp  = sdith_signature_tmp_bytes(p);
    uint8_t *ebuf = malloc(ent);
    uint8_t *tbuf = malloc(tmp);
    getrandom(ebuf, ent, 0);
    sdith_sign(p, sig, m, mlen, sk, ebuf, tbuf);
    *siglen = @SIG@;
    free(ebuf); free(tbuf);
    return 0;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    (void)siglen;
    const signature_parameters *p = &@PARAM_SET@;
    uint64_t tmp  = sdith_verify_tmp_bytes(p);
    uint8_t *tbuf = malloc(tmp);
    uint8_t ok = sdith_verify(p, sig, m, mlen, pk, tbuf);
    free(tbuf);
    return ok ? 0 : -1;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
