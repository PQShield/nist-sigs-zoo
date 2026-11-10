/* Toy DRNG for the NGCC adapter tests: same interface and context layout as the
 * ICCS drng.h, but NOT the ICCS generator and not random. */
#ifndef DRNG_H
#define DRNG_H
#define SEEDLEN (55)
typedef struct {
    unsigned char V[SEEDLEN];
    unsigned char C[SEEDLEN];
    unsigned char reseed_counter[SEEDLEN];
} DRNG_ctx;
int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes);
int get_random_number(DRNG_ctx *drng, unsigned char *random_number, unsigned long long random_number_len_bits);
#endif
