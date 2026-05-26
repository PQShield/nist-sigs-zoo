#ifndef RIJNDAEL256_AVX2__H
#define RIJNDAEL256_AVX2__H

#ifdef __cplusplus
#define EXPORT extern "C"
#include <cstdint>
#include <cstdlib>
#else
#define EXPORT
#include "stdint.h"
#endif
#include <immintrin.h>
#include <wmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>

#define Nb 8
#define Nk 8
#define Nbytes (Nb*4)
#define Nr 14

typedef union {
  __m128i v[2];
  uint8_t b[Nbytes];
 } rijndael256_avx_state_t;

#if Nb != Nk
#error "This implementation expects Nb == Nk."
#endif

typedef union {
   uint8_t v[2][16];  // no alignment assumption shall be made here
   uint8_t b[Nbytes];
 } rijndael256_avx_key_t;

typedef struct {
  rijndael256_avx_key_t rk[Nr+1];
} rijndael256_avx_rk_t;

EXPORT void rijndael256_key_schedule_avx(rijndael256_avx_rk_t *roundkeys, const uint8_t key[32]);
EXPORT void rijndael256_ecb_encrypt_1block_avx(uint8_t res[ 32], const uint8_t x[ 32], const rijndael256_avx_rk_t *roundkeys);
EXPORT void rijndael256_ecb_encrypt_2blocks_avx(uint8_t res[ 64], const uint8_t x[ 64], const rijndael256_avx_rk_t *roundkeys);
EXPORT void rijndael256_ecb_encrypt_4blocks_avx(uint8_t res[128], const uint8_t x[128], const rijndael256_avx_rk_t *roundkeys);
EXPORT void rijndael256_ctr_encrypt_2blocks_avx(uint8_t res[ 64], uint8_t ctr[32], const uint8_t key[32]);

#endif
