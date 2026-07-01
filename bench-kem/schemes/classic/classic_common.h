#pragma once
/*
 * Helpers shared by the OpenSSL-based ECDH KEM shims.
 *
 * ECDH is modelled as a KEM with a static recipient keypair:
 *   keypair : generate recipient (pk, sk)            → stored in module g_key
 *   encaps  : ephemeral keypair e; ct = e_pub;        ss = DH(e_priv, recipient_pub)
 *   decaps  : parse e_pub from ct;                     ss = DH(recipient_priv, e_pub)
 */
#include <stdint.h>
#include <stddef.h>
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/params.h>

/* Shared-secret derivation between our private key and the peer public key. */
static inline int ecdh_derive(EVP_PKEY *priv, EVP_PKEY *peer,
                              uint8_t *out, size_t *outlen) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(priv, NULL);
    if (!ctx) return -1;
    int ret = -1;
    if (EVP_PKEY_derive_init(ctx) > 0 &&
        EVP_PKEY_derive_set_peer(ctx, peer) > 0 &&
        EVP_PKEY_derive(ctx, out, outlen) > 0)
        ret = 0;
    EVP_PKEY_CTX_free(ctx);
    return ret;
}

/* Import a NIST-curve public key from its encoded (uncompressed point) form,
 * without generating a throwaway keypair — keeps decaps timing clean. */
static inline EVP_PKEY *ec_pub_from_encoded(const char *curve,
                                            const uint8_t *pt, size_t ptlen) {
    OSSL_PARAM params[3] = {
        OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME,
                                         (char *)curve, 0),
        OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PUB_KEY,
                                          (void *)pt, ptlen),
        OSSL_PARAM_construct_end()
    };
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_from_name(NULL, "EC", NULL);
    EVP_PKEY *k = NULL;
    if (ctx && EVP_PKEY_fromdata_init(ctx) > 0)
        EVP_PKEY_fromdata(ctx, &k, EVP_PKEY_PUBLIC_KEY, params);
    EVP_PKEY_CTX_free(ctx);
    return k;
}
