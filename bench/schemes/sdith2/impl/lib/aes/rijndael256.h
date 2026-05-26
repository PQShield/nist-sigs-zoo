#ifndef RIJNDAEL256__H
#define RIJNDAEL256__H

#ifdef __cplusplus
#define EXPORT extern "C"
#include <cstdint>
#include <cstdlib>
#else
#define EXPORT
#include "stdint.h"
#endif

#define Nb 8
#define Nk 8
#define Nbytes (Nb*4)
#define Nr 14

typedef uint8_t rijndael256_state_t[Nbytes];

#if Nb != Nk
#error "This implementation expects Nb == Nk."
#endif

typedef uint8_t rijndael256_key_t[Nbytes];

typedef struct {
  rijndael256_key_t rk[Nr+1];
} rijndael256_rk_t;

EXPORT void rijndael256_key_schedule_ref(rijndael256_rk_t *roundkeys, const uint8_t key[32]);
EXPORT void rijndael256_ecb_encrypt_1block_ref(uint8_t res[ 32], const uint8_t x[ 32], const rijndael256_rk_t *roundkeys);
EXPORT void rijndael256_ecb_encrypt_2blocks_ref(uint8_t res[ 64], const uint8_t x[ 64], const rijndael256_rk_t *roundkeys);
EXPORT void rijndael256_ecb_encrypt_4blocks_ref(uint8_t res[128], const uint8_t x[128], const rijndael256_rk_t *roundkeys);
EXPORT void rijndael256_ctr_encrypt_2blocks_ref(uint8_t res[64], const uint8_t ctr[32], const uint8_t key[32]);

#endif
