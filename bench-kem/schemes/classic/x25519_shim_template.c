/* Shim for @NAME@. Generated — edit x25519_params.tsv + x25519_shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <openssl/evp.h>
#include "../../scheme.h"
#include "classic_common.h"

/* Recipient static keypair (X25519 raw keys). */
static EVP_PKEY *g_key = NULL;

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

static EVP_PKEY *gen_x25519(void) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!ctx) return NULL;
    EVP_PKEY *k = NULL;
    if (EVP_PKEY_keygen_init(ctx) <= 0 || EVP_PKEY_keygen(ctx, &k) <= 0) k = NULL;
    EVP_PKEY_CTX_free(ctx);
    return k;
}

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    EVP_PKEY_free(g_key);
    g_key = gen_x25519();
    (void)pk; (void)sk;
    return g_key ? 0 : -1;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    (void)pk;                       /* recipient key is module state g_key */
    if (!g_key) return -1;
    EVP_PKEY *eph = gen_x25519();
    if (!eph) return -1;
    int ret = -1;
    size_t ctlen = @CT@, sslen = @SS@;
    if (EVP_PKEY_get_raw_public_key(eph, ct, &ctlen) > 0 && ctlen == @CT@ &&
        ecdh_derive(eph, g_key, ss, &sslen) == 0)
        ret = 0;
    EVP_PKEY_free(eph);
    return ret;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    (void)sk;
    if (!g_key) return -1;
    EVP_PKEY *peer = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, ct, @CT@);
    if (!peer) return -1;
    size_t sslen = @SS@;
    int ret = ecdh_derive(g_key, peer, ss, &sslen) == 0 ? 0 : -1;
    EVP_PKEY_free(peer);
    return ret;
}
