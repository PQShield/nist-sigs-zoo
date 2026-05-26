/*
 * Common helpers for OpenSSL-backed classic scheme shims.
 *
 * Each shim defines its own static g_key (EVP_PKEY *) for the key pair.
 * crypto_sign_keypair stores the generated key there; sign/verify use it
 * directly, avoiding per-call DER serialization overhead.
 *
 * The pk/sk byte buffers allocated by the harness are unused — their sizes
 * in bench_scheme_info_t are set to the real DER-encoded sizes for
 * documentation, but nothing is written to them.
 */

#ifndef CLASSIC_COMMON_H
#define CLASSIC_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <openssl/evp.h>

static inline EVP_MD_CTX *sign_ctx_new(EVP_PKEY *key, const EVP_MD *md,
                                        EVP_PKEY_CTX **pctx_out) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return NULL;
    if (EVP_DigestSignInit(ctx, pctx_out, md, NULL, key) <= 0) {
        EVP_MD_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

static inline EVP_MD_CTX *vrfy_ctx_new(EVP_PKEY *key, const EVP_MD *md,
                                        EVP_PKEY_CTX **pctx_out) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return NULL;
    if (EVP_DigestVerifyInit(ctx, pctx_out, md, NULL, key) <= 0) {
        EVP_MD_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

#endif /* CLASSIC_COMMON_H */
