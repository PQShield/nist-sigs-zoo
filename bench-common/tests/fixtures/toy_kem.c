/* Toy KEM for the harness crash-isolation tests (bench-common/tests).
 * Build: cc -shared -fPIC -I bench-kem -DTOY_NAME='"Toy-Ok"' -DTOY_MODE=TOY_OK toy_kem.c
 * pk == sk, ss = ct XOR pk. */
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scheme.h"
#include "toy_modes.h"

#define N 32

static const bench_scheme_info_t info = {TOY_NAME, N, N, N, N, 0};
const bench_scheme_info_t *bench_info(void) { return &info; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    TOY_CHATTER();
    for (int i = 0; i < N; i++) pk[i] = sk[i] = (uint8_t)(i * 7 + 1);
    return 0;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    TOY_MISBEHAVE();
    for (int i = 0; i < N; i++) {
        ct[i] = (uint8_t)(i * 3);
        ss[i] = ct[i] ^ pk[i];
    }
    return 0;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    for (int i = 0; i < N; i++) ss[i] = ct[i] ^ sk[i];
    if (TOY_MODE == TOY_BADCHECK) ss[0] ^= 1;  /* shared secrets never agree */
    return 0;
}
