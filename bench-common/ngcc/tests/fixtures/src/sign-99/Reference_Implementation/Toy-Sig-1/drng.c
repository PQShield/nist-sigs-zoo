#include <string.h>
#include "drng.h"

int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes) {
    memset(drng, 0, sizeof *drng);
    for (unsigned long long i = 0; i < seed_len_bytes; i++)
        drng->V[i % SEEDLEN] = (unsigned char)(drng->V[i % SEEDLEN] * 31u + seed[i]);
    return 0;
}

int get_random_number(DRNG_ctx *drng, unsigned char *out, unsigned long long bits) {
    unsigned long long n = (bits + 7) / 8;
    for (unsigned long long i = 0; i < n; i++) {
        unsigned char *v = &drng->V[drng->C[0] % SEEDLEN];
        *v = (unsigned char)(*v * 167u + 13u + drng->C[1]);
        out[i] = *v;
        if (++drng->C[0] == 0) drng->C[1]++;
    }
    return 0;
}
