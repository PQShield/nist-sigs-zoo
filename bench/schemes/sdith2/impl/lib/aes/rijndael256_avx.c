#include "rijndael256_avx.h"
#include <string.h>

// The following part is adapted from a work by Nir Drucker and Shay Gueron:
// Software optimization of Rijndael for Modern x86_64 platforms,
// in ITNG 2022 - 19th International Conference on Information Technology - New Generations (pp 147-153)
// Original code: Copyright Nir Drucker and Shay Gueron All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// in our signature, we do not need a constant time counter increment since the counter is always known publicly
/*
static void _increment_ctr_ct(uint8_t res[32], const uint8_t ctr[32])
{
  uint64_t tmp = ctr[0] | (((uint32_t)ctr[1])<<8) | (((uint32_t)ctr[2])<<16) | (((uint32_t)ctr[3])<<24);
  tmp += 1;
  res[0] = (tmp >>  0) & 0xFF;
  res[1] = (tmp >>  8) & 0xFF;
  res[2] = (tmp >> 16) & 0xFF;
  res[3] = (tmp >> 24) & 0xFF;
  for (size_t i = 1; i < 8; i++)
  {
    tmp >>= 32;
    tmp += ctr[4*i] | (((uint32_t)ctr[4*i+1])<<8) | (((uint32_t)ctr[4*i+2])<<16) | (((uint32_t)ctr[4*i+3])<<24);
    res[4*i+0] = (tmp >>  0) & 0xFF;
    res[4*i+1] = (tmp >>  8) & 0xFF;
    res[4*i+2] = (tmp >> 16) & 0xFF;
    res[4*i+3] = (tmp >> 24) & 0xFF;
  }
}
*/

// we are going to use a non-ct counter increment since the counter is public
static __always_inline void _increment_ctr_256_nonct(uint64_t* ctr256) {
  if (++(ctr256[0]) == 0) {
    if (++(ctr256[1]) == 0) {
      if (++(ctr256[2]) == 0) {
        ++(ctr256[3]);
      }
    }
  }
}

#define BLOCK_BYTES 32ULL
#define KEY_BYTES 32ULL
#define ROUNDS 14ULL
#define ALIGN(n) __attribute__((aligned(n)))

typedef ALIGN(16) struct key_s {
  uint8_t raw[KEY_BYTES];
} r256_key_t;

typedef ALIGN(16) struct key_schedule_s {
  union {
    r256_key_t keys[ROUNDS + 1];
    uint8_t raw[(ROUNDS + 1) * sizeof(r256_key_t)];
  };
} key_schedule_t;

// AVX instrucitons
#define SET(v1, v2, v3, v4) _mm_set_epi32((v1), (v2), (v3), (v4))
#define LOAD(mem) _mm_loadu_si128((const __m128i*)(mem))
#define STORE(mem, reg) _mm_storeu_si128((__m128i*)(mem), (reg))

#define AESENC(in, key) _mm_aesenc_si128((in), (key))
#define AESENCLAST(in, key) _mm_aesenclast_si128((in), (key))
#define AESKEYGEN(reg, mask) _mm_aeskeygenassist_si128((reg), (mask))

#define SLL(reg, n) _mm_slli_si128((reg), (n))

#define BLEND(in1, in2, mask) _mm_blendv_epi8((in1), (in2), (mask))
#define SHUF8(in, mask) _mm_shuffle_epi8((in), (mask))
#define SHUF32(in, mask) _mm_shuffle_epi32((in), (mask))

// AVX512 instrucitons
#define VAESENC(in, key) _mm512_aesenc_epi128((in), (key))
#define VAESENCLAST(in, key) _mm512_aesenclast_epi128((in), (key))
#define PERM8(idx, a) _mm512_permutexvar_epi8((idx), (a))
#define SETR32(...) _mm512_setr_epi32(__VA_ARGS__)
#define SETR64(...) _mm512_setr_epi64(__VA_ARGS__)

#define LOAD512(mem) _mm512_loadu_si512((const __m512i*)(mem))
#define STORE512(mem, reg) _mm512_storeu_si512((__m512i*)(mem), (reg))

static __always_inline void encrypt_one_block(__m128i* out128, const __m128i* in128, const __m128i* ks128) {
  const __m128i RMASK = SET(0x03020d0c, 0x0f0e0908, 0x0b0a0504, 0x07060100);
  const __m128i BMASK = SET(0x80000000, 0x80800000, 0x80800000, 0x80808000);

  // Load one block
  __m128i in1 = LOAD(&in128[0]);
  __m128i in2 = LOAD(&in128[1]);

  // Round 0: Initial xor
  in1 = _mm_xor_si128(in1, _mm_loadu_si128(&ks128[0]));
  in2 = _mm_xor_si128(in2, _mm_loadu_si128(&ks128[1]));

  // Perform (rounds-1) AES rounds
  for (size_t j = 1; j < ROUNDS; j++) {
    // Blend to compensate for the shift rows shifts bytes between two 128 bit
    // blocks
    __m128i t1 = BLEND(in1, in2, BMASK);
    __m128i t2 = BLEND(in2, in1, BMASK);

    // Shuffle that compensates for the additional shift in rows 3 and 4 as
    // opposed to rijndael128 (AES)
    t1 = SHUF8(t1, RMASK);
    t2 = SHUF8(t2, RMASK);

    // Encryption step: sub bytes, shift rows, mix columns, and xor with round key
    in1 = AESENC(t1, _mm_loadu_si128(&ks128[j * 2]));
    in2 = AESENC(t2, _mm_loadu_si128(&ks128[j * 2 + 1]));
  }

  // Last AES round
  __m128i t1 = BLEND(in1, in2, BMASK);
  __m128i t2 = BLEND(in2, in1, BMASK);
  t1 = AESENCLAST(SHUF8(t1, RMASK), _mm_loadu_si128(&ks128[ROUNDS * 2 + 0]));
  t2 = AESENCLAST(SHUF8(t2, RMASK), _mm_loadu_si128(&ks128[ROUNDS * 2 + 1]));

  // Store the encrypted block
  STORE(&out128[0], t1);
  STORE(&out128[1], t2);
}

static __always_inline void encrypt_multi_blocks(__m128i* out128, const __m128i* in128, const __m128i* ks128,
                                                 const size_t num_of_blocks)  // always known at compile time
{
  const __m128i RMASK = SET(0x03020d0c, 0x0f0e0908, 0x0b0a0504, 0x07060100);
  const __m128i BMASK = SET(0x80000000, 0x80800000, 0x80800000, 0x80808000);

  // Load blocks
  __m128i in1[num_of_blocks];
  __m128i in2[num_of_blocks];
  for (size_t k = 0; k < num_of_blocks; k++) {
    in1[k] = LOAD(&in128[2 * k + 0]);
    in2[k] = LOAD(&in128[2 * k + 1]);
  }

  // Round 0: Initial xor
  for (size_t k = 0; k < num_of_blocks; k++) {
    in1[k] = _mm_xor_si128(in1[k], _mm_loadu_si128(ks128 + 0));
    in2[k] = _mm_xor_si128(in2[k], _mm_loadu_si128(ks128 + 1));
  }

  __m128i t1[num_of_blocks];
  __m128i t2[num_of_blocks];

  // Perform (rounds-1) AES rounds
  for (size_t j = 1; j < ROUNDS; j++) {
    // Blend to compensate for the shift rows shifts bytes between two 128 bit
    // blocks
    for (size_t k = 0; k < num_of_blocks; k++) {
      t1[k] = BLEND(in1[k], in2[k], BMASK);
      t2[k] = BLEND(in2[k], in1[k], BMASK);
    }

    // Shuffle that compensates for the additional shift in rows 3 and 4 as
    // opposed to rijndael128 (AES)
    for (size_t k = 0; k < num_of_blocks; k++) {
      t1[k] = SHUF8(t1[k], RMASK);
      t2[k] = SHUF8(t2[k], RMASK);
    }

    // Encryption step: sub bytes, shift rows, mix columns, and xor with round key
    for (size_t k = 0; k < num_of_blocks; k++) {
      in1[k] = AESENC(t1[k], _mm_loadu_si128(&ks128[j * 2]));
      in2[k] = AESENC(t2[k], _mm_loadu_si128(&ks128[j * 2 + 1]));
    }
  }

  // Last AES round
  for (size_t k = 0; k < num_of_blocks; k++) {
    t1[k] = BLEND(in1[k], in2[k], BMASK);
    t2[k] = BLEND(in2[k], in1[k], BMASK);
  }

  for (size_t k = 0; k < num_of_blocks; k++) {
    t1[k] = AESENCLAST(SHUF8(t1[k], RMASK), _mm_loadu_si128(&ks128[ROUNDS * 2 + 0]));
    t2[k] = AESENCLAST(SHUF8(t2[k], RMASK), _mm_loadu_si128(&ks128[ROUNDS * 2 + 1]));
  }

  // Store the encrypted block
  for (size_t k = 0; k < num_of_blocks; k++) {
    STORE(&out128[2 * k + 0], t1[k]);
    STORE(&out128[2 * k + 1], t2[k]);
  }
}

//////  key expansion

#define ADD_CASE(v, k)    \
  case (v):               \
    t1 = AESKEYGEN(k, v); \
    break

#define KEY_SCHEDULE_ROUNDS 14

static inline __m128i round1(__m128i* in1, const __m128i* in2, const int idx, const int mask_ff) {
  __m128i t1;
  switch (idx) {
    ADD_CASE(0x0, *in2);
    ADD_CASE(0x1, *in2);
    ADD_CASE(0x2, *in2);
    ADD_CASE(0x4, *in2);
    ADD_CASE(0x8, *in2);
    ADD_CASE(0x10, *in2);
    ADD_CASE(0x20, *in2);
    ADD_CASE(0x40, *in2);
    ADD_CASE(0x80, *in2);
    ADD_CASE(0x1b, *in2);
    ADD_CASE(0x36, *in2);
    ADD_CASE(0x6c, *in2);
    ADD_CASE(0xd8, *in2);
    ADD_CASE(0xab, *in2);
    ADD_CASE(0x4d, *in2);
    default:
      abort();  // assert("Bad value");
  }

  t1 = mask_ff ? SHUF32(t1, 0xff) : SHUF32(t1, 0xaa);

  __m128i t2 = SLL(*in1, 0x4);
  *in1 ^= t2;
  t2 = SLL(t2, 0x4);
  *in1 ^= t2;
  t2 = SLL(t2, 0x4);
  *in1 ^= t2 ^ t1;

  return *in1;
}

static inline void interleave_round(__m128i out[2], __m128i in[2], const int idx) {
  _mm_storeu_si128(&out[0], round1(&in[0], &in[1], idx, 1));
  _mm_storeu_si128(&out[1], round1(&in[1], &in[0], 0, 0));
}

void rijndael256_key_expansion(key_schedule_t* ks, const r256_key_t* key) {
  const uint8_t key_assist_vals[KEY_SCHEDULE_ROUNDS] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
                                                        0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d};  // NOLINT
  __m128i* ks128 = (__m128i*)ks;

  __m128i tmp[2] = {LOAD(key->raw), LOAD(&key->raw[16])};

  _mm_storeu_si128(&ks128[0], tmp[0]);
  _mm_storeu_si128(&ks128[1], tmp[1]);

  for (size_t i = 0; i < KEY_SCHEDULE_ROUNDS; i++) {
    interleave_round(&ks128[2 * (i + 1)], tmp, key_assist_vals[i]);
  }
}

EXPORT void rijndael256_key_schedule_avx(rijndael256_avx_rk_t *roundkeys, const uint8_t key[32])
{
  rijndael256_key_expansion((key_schedule_t*)roundkeys, (r256_key_t*)key);
}

void rijndael256_ecb_encrypt_1block_avx(uint8_t res[32], const uint8_t x[32], const rijndael256_avx_rk_t* roundkeys) {
  encrypt_one_block((__m128i*)res, (const __m128i*)x, (const __m128i*)roundkeys);
}

void rijndael256_ecb_encrypt_2blocks_avx(uint8_t res[64], const uint8_t x[64], const rijndael256_avx_rk_t* roundkeys) {
  encrypt_multi_blocks((__m128i*)res, (const __m128i*)x, (const __m128i*)roundkeys, 2);
}

void rijndael256_ecb_encrypt_4blocks_avx(uint8_t res[128], const uint8_t x[128],
                                         const rijndael256_avx_rk_t* roundkeys) {
  encrypt_multi_blocks((__m128i*)res, (const __m128i*)x, (const __m128i*)roundkeys, 4);
}

void rijndael256_ctr_encrypt_2blocks_avx(uint8_t res[64], uint8_t ctr[32], const uint8_t key[32]) {
  // Here: we are interleaving key expansion and encryption to avoid using memory for rk's
  // instead of:
  /*
  // expand key
  rijndael256_avx_rk_t roundkeys;
  //_keyschedule(&roundkeys, key);
  rijndael256_key_expansion((key_schedule_t*)roundkeys.rk, (r256_key_t*)key);

  // encrypt
  encrypt_multi_blocks((__m128i*)res, (const __m128i*)cur_enc, (const __m128i*)roundkeys.rk, 2);
  */

  const uint8_t key_assist_vals[KEY_SCHEDULE_ROUNDS] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
                                                        0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d};  // NOLINT

  // counter
  __m128i cur_enc[4];
  memcpy(cur_enc, ctr, 32);
  memcpy(cur_enc + 2, ctr, 32);
  _increment_ctr_256_nonct((uint64_t*)(cur_enc + 2));

  const __m128i RMASK = SET(0x03020d0c, 0x0f0e0908, 0x0b0a0504, 0x07060100);
  const __m128i BMASK = SET(0x80000000, 0x80800000, 0x80800000, 0x80808000);

  // Round 0: Initial xor
  __m128i cur_rk[2] = {LOAD(key), LOAD(key + 16)};

  cur_enc[0] = _mm_xor_si128(cur_enc[0], cur_rk[0]);
  cur_enc[1] = _mm_xor_si128(cur_enc[1], cur_rk[1]);
  cur_enc[2] = _mm_xor_si128(cur_enc[2], cur_rk[0]);
  cur_enc[3] = _mm_xor_si128(cur_enc[3], cur_rk[1]);

  __m128i t1[2];
  __m128i t2[2];

  // Perform (rounds-1) AES rounds
  for (size_t j = 1; j < ROUNDS; j++) {
    // Blend to compensate for the shift rows shifts bytes between two 128 bit
    // blocks
    {
      t1[0] = BLEND(cur_enc[0], cur_enc[1], BMASK);
      t2[0] = BLEND(cur_enc[1], cur_enc[0], BMASK);
      t1[1] = BLEND(cur_enc[2], cur_enc[3], BMASK);
      t2[1] = BLEND(cur_enc[3], cur_enc[2], BMASK);
    }

    // Shuffle that compensates for the additional shift in rows 3 and 4 as
    // opposed to rijndael128 (AES)
    {
      t1[0] = SHUF8(t1[0], RMASK);
      t2[0] = SHUF8(t2[0], RMASK);
      t1[1] = SHUF8(t1[1], RMASK);
      t2[1] = SHUF8(t2[1], RMASK);
    }

    // Encryption step: sub bytes, shift rows, mix columns, and xor with round key
    {
      __m128i t = round1(&cur_rk[0], &cur_rk[1], key_assist_vals[j - 1], 1);
      cur_rk[1] = round1(&cur_rk[1], &cur_rk[0], key_assist_vals[0], 0);
      cur_rk[0] = t;
    }

    {
      cur_enc[0] = AESENC(t1[0], cur_rk[0]);
      cur_enc[1] = AESENC(t2[0], cur_rk[1]);
      cur_enc[2] = AESENC(t1[1], cur_rk[0]);
      cur_enc[3] = AESENC(t2[1], cur_rk[1]);
    }
  }

  // Last AES round
  {
    t1[0] = BLEND(cur_enc[0], cur_enc[1], BMASK);
    t2[0] = BLEND(cur_enc[1], cur_enc[0], BMASK);
    t1[1] = BLEND(cur_enc[2], cur_enc[3], BMASK);
    t2[1] = BLEND(cur_enc[3], cur_enc[2], BMASK);
  }

  {
    __m128i t = round1(&cur_rk[0], &cur_rk[1], key_assist_vals[ROUNDS - 1], 1);
    cur_rk[1] = round1(&cur_rk[1], &cur_rk[0], key_assist_vals[0], 0);
    cur_rk[0] = t;
  }

  {
    t1[0] = AESENCLAST(SHUF8(t1[0], RMASK), cur_rk[0]);
    t2[0] = AESENCLAST(SHUF8(t2[0], RMASK), cur_rk[1]);
    t1[1] = AESENCLAST(SHUF8(t1[1], RMASK), cur_rk[0]);
    t2[1] = AESENCLAST(SHUF8(t2[1], RMASK), cur_rk[1]);
  }

  // Store the encrypted block
  {
    STORE(&res[0], t1[0]);
    STORE(&res[16], t2[0]);
    STORE(&res[32], t1[1]);
    STORE(&res[48], t2[1]);
  }
}
