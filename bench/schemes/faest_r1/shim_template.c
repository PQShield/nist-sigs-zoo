#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* The variant directory's api.h (on the include path) defines CRYPTO_BYTES,
 * CRYPTO_SECRETKEYBYTES, CRYPTO_PUBLICKEYBYTES and declares crypto_sign_keypair/
 * crypto_sign/crypto_sign_open (api.c). */
#include "api.h"
#include "../../scheme.h"

/* The layout is sm = m ‖ sig (message first, signature appended). */

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    *siglen = CRYPTO_BYTES;
    uint8_t *sm = malloc(CRYPTO_BYTES + mlen);
    if (!sm) return -1;
    unsigned long long smlen;
    int ret = crypto_sign(sm, &smlen, m, (unsigned long long)mlen, sk);
    if (ret == 0)
        memcpy(sig, sm + mlen, CRYPTO_BYTES);
    free(sm);
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    (void)siglen;
    uint8_t *sm = malloc(mlen + CRYPTO_BYTES);
    if (!sm) return -1;
    memcpy(sm, m, mlen);
    memcpy(sm + mlen, sig, CRYPTO_BYTES);
    uint8_t *m_out = malloc(mlen + 1);
    if (!m_out) { free(sm); return -1; }
    unsigned long long mlen_out;
    int ret = crypto_sign_open(m_out, &mlen_out, sm,
                               mlen + (unsigned long long)CRYPTO_BYTES, pk);
    free(m_out);
    free(sm);
    return ret;
}

/* Upstream's crypto_sign_keypair (api.c) is renamed by the Makefile
 * (-Dcrypto_sign_keypair=faest_upstream_keypair) so that this wrapper can export it.
 *
 * KEYGEN_SELFTEST (params.tsv) is 1 for FAEST-256s/256f only: in this round-1 AVX2 code,
 * faest_compute_witness() (faest.c) recovers the AES-256 key-schedule S-box outputs as
 * w[k] ^ w[k-4] (prev_word = the previous 4-word period), but AES-256 has Nk = 8, so the
 * zero-S-box-input check that rejects invalid keys tests the wrong values. The keypair loop
 * then accepts keys that the specification (and the submitted reference implementation,
 * whose KATs we reproduce) rejects, and signatures under such keys never verify. We retry
 * key generation until a sign/verify round trip succeeds, so the sign and verify timings
 * are for a valid key; the keygen timing of those two rows includes the extra work. */
#undef crypto_sign_keypair
int faest_upstream_keypair(unsigned char *pk, unsigned char *sk);

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    for (;;) {
        int ret = faest_upstream_keypair(pk, sk);
        if (ret != 0 || !@KEYGEN_SELFTEST@)
            return ret;
        static const uint8_t m[32];
        uint8_t *sig = malloc(CRYPTO_BYTES);
        if (!sig) return -1;
        size_t siglen;
        int ok = crypto_sign_signature(sig, &siglen, m, sizeof(m), sk) == 0
              && crypto_sign_verify(sig, siglen, m, sizeof(m), pk) == 0;
        free(sig);
        if (ok)
            return 0;
    }
}

static const bench_scheme_info_t INFO = {
    "@NAME@", @PK@, @SK@, @SIG@, @ITERS@
};

const bench_scheme_info_t *bench_info(void) { return &INFO; }
