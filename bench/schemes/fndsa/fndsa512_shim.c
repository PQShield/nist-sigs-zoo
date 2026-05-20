/* Shim for FN-DSA-512 (logn=9, Falcon-512). */
#include <stddef.h>
#include <stdint.h>
#include "../../scheme.h"
#include "ref/fndsa.h"

#define SK_LEN  FNDSA_SIGN_KEY_SIZE(9)   /* 1281 */
#define PK_LEN  FNDSA_VRFY_KEY_SIZE(9)   /* 897  */
#define SIG_LEN FNDSA_SIGNATURE_SIZE(9)  /* 666  */

static const bench_scheme_info_t INFO = { "FN-DSA-512", PK_LEN, SK_LEN, SIG_LEN, 0 };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    /* fndsa_keygen: sign_key (sk) before vrfy_key (pk); returns 1 on success */
    return fndsa_keygen(FNDSA_LOGN_512, sk, pk) == 1 ? 0 : -1;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    *siglen = fndsa_sign(sk, SK_LEN, NULL, 0, FNDSA_HASH_ID_RAW, m, mlen,
                         sig, SIG_LEN);
    return *siglen == 0 ? -1 : 0;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    return fndsa_verify(sig, siglen, pk, PK_LEN, NULL, 0,
                        FNDSA_HASH_ID_RAW, m, mlen) == 1 ? 0 : -1;
}
