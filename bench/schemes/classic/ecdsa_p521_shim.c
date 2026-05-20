/* ECDSA P-521, SHA-512. */
#include <openssl/evp.h>
#include <openssl/ec.h>
#include "../../scheme.h"
#include "classic_common.h"

static EVP_PKEY *g_key = NULL;

static const bench_scheme_info_t INFO = { "ECDSA-P521 (SHA-512)", 133, 223, 139, 0 };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    EVP_PKEY_free(g_key);
    g_key = EVP_EC_gen("P-521");
    (void)pk; (void)sk;
    return g_key ? 0 : -1;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    if (!g_key) return -1;
    EVP_MD_CTX *ctx = sign_ctx_new(g_key, EVP_sha512(), NULL);
    if (!ctx) return -1;
    *siglen = 139;
    int ret = EVP_DigestSign(ctx, sig, siglen, m, mlen) > 0 ? 0 : -1;
    EVP_MD_CTX_free(ctx);
    (void)sk;
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (!g_key) return -1;
    EVP_MD_CTX *ctx = vrfy_ctx_new(g_key, EVP_sha512(), NULL);
    if (!ctx) return -1;
    int ret = EVP_DigestVerify(ctx, sig, siglen, m, mlen) == 1 ? 0 : -1;
    EVP_MD_CTX_free(ctx);
    (void)pk;
    return ret;
}
