/* RSA-4096, PSS padding, SHA-512. */
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include "../../scheme.h"
#include "classic_common.h"

static EVP_PKEY *g_key = NULL;

static const bench_scheme_info_t INFO = { "RSA-4096 (PSS)", 550, 2350, 512, 20 };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    EVP_PKEY_free(g_key); g_key = NULL;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) return -1;
    int ret = -1;
    if (EVP_PKEY_keygen_init(ctx) > 0 &&
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 4096) > 0 &&
        EVP_PKEY_keygen(ctx, &g_key) > 0) ret = 0;
    EVP_PKEY_CTX_free(ctx);
    (void)pk; (void)sk;
    return ret;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    if (!g_key) return -1;
    EVP_PKEY_CTX *pctx = NULL;
    EVP_MD_CTX *ctx = sign_ctx_new(g_key, EVP_sha512(), &pctx);
    if (!ctx) return -1;
    int ret = -1;
    *siglen = 512;
    if (EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) > 0 &&
        EVP_DigestSign(ctx, sig, siglen, m, mlen) > 0) ret = 0;
    EVP_MD_CTX_free(ctx);
    (void)sk;
    return ret;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (!g_key) return -1;
    EVP_PKEY_CTX *pctx = NULL;
    EVP_MD_CTX *ctx = vrfy_ctx_new(g_key, EVP_sha512(), &pctx);
    if (!ctx) return -1;
    int ret = -1;
    if (EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) > 0 &&
        EVP_DigestVerify(ctx, sig, siglen, m, mlen) == 1) ret = 0;
    EVP_MD_CTX_free(ctx);
    (void)pk;
    return ret;
}
