/* Toy signature scheme for the harness crash-isolation tests (bench-common/tests).
 * Build: cc -shared -fPIC -I bench -DTOY_NAME='"Toy-Ok"' -DTOY_MODE=TOY_OK toy_sig.c
 * sig = sk XOR msg, pk == sk, so verification and tamper checks work. */
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scheme.h"
#include "toy_modes.h"

#define N 32

static const bench_scheme_info_t info = {TOY_NAME, N, N, N, 0};
const bench_scheme_info_t *bench_info(void) { return &info; }

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    TOY_CHATTER();
    for (int i = 0; i < N; i++) pk[i] = sk[i] = (uint8_t)(i * 7 + 1);
    return 0;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m, size_t mlen,
                          const uint8_t *sk) {
    TOY_MISBEHAVE();
    for (size_t i = 0; i < N; i++) sig[i] = sk[i] ^ (i < mlen ? m[i] : 0);
    *siglen = N;
    return 0;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m, size_t mlen,
                       const uint8_t *pk) {
    if (TOY_MODE == TOY_BADCHECK) return -1;  /* rejects every signature */
    if (siglen != N) return -1;
    for (size_t i = 0; i < N; i++)
        if (sig[i] != (pk[i] ^ (i < mlen ? m[i] : 0))) return -1;
    return 0;
}
