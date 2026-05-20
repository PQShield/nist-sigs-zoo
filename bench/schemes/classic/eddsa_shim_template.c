/* Shim for @NAME@. Generated — edit eddsa_params.tsv + eddsa_shim_template.c, not this file. */
#include <openssl/evp.h>
#include "../../scheme.h"
#include "classic_common.h"

static EVP_PKEY *g_key = NULL;

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @SIG@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    EVP_PKEY_free(g_key); g_key = NULL;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(@KEY_TYPE@, NULL);
    if (!ctx) return -1;
    int ret = -1;
    if (EVP_PKEY_keygen_init(ctx) > 0 && EVP_PKEY_keygen(ctx, &g_key) > 0) ret = 0;
    EVP_PKEY_CTX_free(ctx);
    (void)pk; (void)sk;
    return ret;
}
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen, const uint8_t *sk) {
    if (!g_key) return -1;
    /* EdDSA uses NULL digest — hashing is internal. */
    EVP_MD_CTX *ctx = sign_ctx_new(g_key, NULL, NULL);
    if (!ctx) return -1;
    *siglen = @SIG@;
    int ret = EVP_DigestSign(ctx, sig, siglen, m, mlen) > 0 ? 0 : -1;
    EVP_MD_CTX_free(ctx);
    (void)sk;
    return ret;
}
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen, const uint8_t *pk) {
    if (!g_key) return -1;
    EVP_MD_CTX *ctx = vrfy_ctx_new(g_key, NULL, NULL);
    if (!ctx) return -1;
    int ret = EVP_DigestVerify(ctx, sig, siglen, m, mlen) == 1 ? 0 : -1;
    EVP_MD_CTX_free(ctx);
    (void)pk;
    return ret;
}
