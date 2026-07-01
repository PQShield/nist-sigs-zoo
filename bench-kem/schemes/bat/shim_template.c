/* Shim for @NAME@. Generated — edit params.tsv + shim_template.c, not this file. */
#include <stdint.h>
#include "bat.h"
#include "../../scheme.h"

/*
 * BAT (pornin/BAT) has a Pornin-style struct API, not the bare NIST one: keygen
 * produces a private_key struct, encapsulate/decapsulate work on public_key /
 * ciphertext structs, and the transmitted byte forms come from separate
 * encode/decode calls. Every operation takes a caller-provided scratch
 * buffer. The shim adapts this to crypto_kem_* over encoded byte arrays — so the
 * pk/ct sizes match what is actually sent, and the measured timings include the
 * (de)serialisation that a real deployment pays. RTLD_LOCAL isolates the two
 * sets' bare symbols; randomness comes from BAT's own bat_get_seed (OS RNG).
 *
 * One scratch buffer sized to the largest tmp (keygen) serves all operations.
 */
static uint8_t g_tmp[BAT_@Q@_@N@_TMP_KEYGEN];

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    bat_@Q@_@N@_private_key skp;
    bat_@Q@_@N@_public_key pkp;
    if (bat_@Q@_@N@_keygen(&skp, g_tmp, sizeof g_tmp) != 0) return -1;
    bat_@Q@_@N@_get_public_key(&pkp, &skp);
    if (bat_@Q@_@N@_encode_public_key(pk, @PK@, &pkp) != @PK@) return -1;
    if (bat_@Q@_@N@_encode_private_key(sk, @SK@, &skp, 0) != @SK@) return -1;
    return 0;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    bat_@Q@_@N@_public_key pkp;
    bat_@Q@_@N@_ciphertext ctp;
    if (bat_@Q@_@N@_decode_public_key(&pkp, pk, @PK@) == 0) return -1;
    if (bat_@Q@_@N@_encapsulate(ss, @SS@, &ctp, &pkp, g_tmp, sizeof g_tmp) != 0) return -1;
    if (bat_@Q@_@N@_encode_ciphertext(ct, @CT@, &ctp) != @CT@) return -1;
    return 0;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    bat_@Q@_@N@_private_key skp;
    bat_@Q@_@N@_ciphertext ctp;
    if (bat_@Q@_@N@_decode_private_key(&skp, sk, @SK@, g_tmp, sizeof g_tmp) == 0) return -1;
    if (bat_@Q@_@N@_decode_ciphertext(&ctp, ct, @CT@) == 0) return -1;
    if (bat_@Q@_@N@_decapsulate(ss, @SS@, &ctp, &skp, g_tmp, sizeof g_tmp) != 0) return -1;
    return 0;
}

static const bench_scheme_info_t INFO = { "@NAME@", @PK@, @SK@, @CT@, @SS@, @ITERS@ };
const bench_scheme_info_t *bench_info(void) { return &INFO; }
