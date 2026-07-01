/* Shim for @NAME@. Generated — edit ecdh_nist_params.tsv + ecdh_nist_shim_template.c, not this file. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include "../../scheme.h"
#include "classic_common.h"

/* Recipient static keypair (NIST curve @CURVE@). */
static EVP_PKEY *g_key = NULL;

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    EVP_PKEY_free(g_key);
    g_key = EVP_EC_gen("@CURVE@");
    (void)pk; (void)sk;
    return g_key ? 0 : -1;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    (void)pk;                       /* recipient key is module state g_key */
    if (!g_key) return -1;
    EVP_PKEY *eph = EVP_EC_gen("@CURVE@");
    if (!eph) return -1;
    int ret = -1;
    size_t sslen = @SS@;
    unsigned char *enc = NULL;
    size_t enclen = EVP_PKEY_get1_encoded_public_key(eph, &enc);  /* uncompressed point */
    if (enc && enclen == @CT@) {
        memcpy(ct, enc, enclen);
        if (ecdh_derive(eph, g_key, ss, &sslen) == 0) ret = 0;
    }
    OPENSSL_free(enc);
    EVP_PKEY_free(eph);
    return ret;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    (void)sk;
    if (!g_key) return -1;
    EVP_PKEY *peer = ec_pub_from_encoded("@CURVE@", ct, @CT@);
    if (!peer) return -1;
    size_t sslen = @SS@;
    int ret = ecdh_derive(g_key, peer, ss, &sslen) == 0 ? 0 : -1;
    EVP_PKEY_free(peer);
    return ret;
}
