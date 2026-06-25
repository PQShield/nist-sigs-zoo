/* Shim for @NAME@ (ECDH used as a KEM, DHKEM-style).
 * Generated — edit params.tsv + shim_template.c, not this file.
 *
 * Modelling ECDH as a KEM:
 *   keypair : generate the recipient's static keypair.
 *   enc     : generate an ephemeral keypair, ECDH(ephemeral_sk, recipient_pk)
 *             → shared secret; ciphertext = ephemeral public key.
 *   dec     : ECDH(recipient_sk, ephemeral_pk) → shared secret.
 *
 * The raw X25519/X448 DH output is used directly as the shared secret (no KDF),
 * which is the operation we want to time. Keys are stored at module level; the
 * harness-provided pk/sk buffers are unused for the static key but ct carries
 * the ephemeral public key between enc and dec.
 */
#include <string.h>
#include <openssl/evp.h>
#include "../../scheme.h"

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }

static EVP_PKEY *g_recipient = NULL;   /* static recipient keypair */

static EVP_PKEY *gen_key(void) {
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(@KEY_TYPE@, NULL);
    if (!ctx) return NULL;
    EVP_PKEY *k = NULL;
    if (EVP_PKEY_keygen_init(ctx) <= 0 || EVP_PKEY_keygen(ctx, &k) <= 0) k = NULL;
    EVP_PKEY_CTX_free(ctx);
    return k;
}

/* Raw ECDH: derive shared secret between our_key and their raw public key. */
static int ecdh_derive(EVP_PKEY *our_key, const uint8_t *peer_pub, size_t peer_len,
                       uint8_t *ss, size_t ss_len) {
    EVP_PKEY *peer = EVP_PKEY_new_raw_public_key(@KEY_TYPE@, NULL, peer_pub, peer_len);
    if (!peer) return -1;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(our_key, NULL);
    int ret = -1;
    size_t out = ss_len;
    if (ctx && EVP_PKEY_derive_init(ctx) > 0
            && EVP_PKEY_derive_set_peer(ctx, peer) > 0
            && EVP_PKEY_derive(ctx, ss, &out) > 0
            && out == ss_len) {
        ret = 0;
    }
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(peer);
    return ret;
}

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    EVP_PKEY_free(g_recipient);
    g_recipient = gen_key();
    if (!g_recipient) return -1;
    size_t len = @PK@;
    if (EVP_PKEY_get_raw_public_key(g_recipient, pk, &len) <= 0) return -1;
    (void)sk;
    return 0;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    /* Ephemeral keypair; ct = ephemeral public key. */
    EVP_PKEY *eph = gen_key();
    if (!eph) return -1;
    size_t len = @CT@;
    int ret = -1;
    if (EVP_PKEY_get_raw_public_key(eph, ct, &len) > 0)
        ret = ecdh_derive(eph, pk, @PK@, ss, @SS@);
    EVP_PKEY_free(eph);
    return ret;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    if (!g_recipient) return -1;
    (void)sk;
    return ecdh_derive(g_recipient, ct, @CT@, ss, @SS@);
}
