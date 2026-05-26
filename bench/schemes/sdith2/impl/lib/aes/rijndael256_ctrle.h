#ifndef RIJNDAEL256_CTRLE_H
#define RIJNDAEL256_CTRLE_H

#ifdef __cplusplus
#define EXPORT extern "C"
#include <cstdint>
#else
#define EXPORT
#include "stdint.h"
#endif

#define RIJNDAEL256_RK_BYTES 480
#define RIJNDAEL256_RNG_BYTES 608

/**
 * (special API) expands the key into round keys
 * @param round_keys [out] round key must be a 480 byte array
 * @param rijndael256key [in] the 256 bit rijndael key
 */
EXPORT void rijndael256_ctrle_set_key_ref(void* round_keys, const void* rijndael256key);
EXPORT void rijndael256_ctrle_set_key_avx2(void* round_keys, const void* rijndael256key);

/**
 * (special API) ase256 encrypts an exact number of blocks in ctrle mode
 * @param out [out] the first nblocks-1 output blocks
 * @param out_last256 [out] the last output blocks
 * @param in_out_ctr256 [in/out] the current counter (at the end, it is input_ctr + nblocks)
 * @param round_keys [in] round key must be a 16*11 byte array
 * @param nblocks [in] the number of blocks to encrypt
 */
EXPORT void rijndael256_ctrle_encrypt_nblocks_ref(void* out, void* out_last256, void* in_out_ctr256,  //
                                                  const void* round_keys, uint64_t nblocks);
EXPORT void rijndael256_ctrle_encrypt_nblocks_avx2(void* out, void* out_last256, void* in_out_ctr256,  //
                                                   const void* round_keys, uint64_t nblocks);

/** (special API) one-shot encryption function
 * @param out256 [out] 2 blocks
 * @param rijndael256key [in] the 256 bit aes key
 * @param ctr256 [in] start counter
 */
EXPORT void rijndael256_ctrle_oneshot_encrypt_2blocks_ref(void* out256, const void* rijndael256key, const void* ctr256);
EXPORT void rijndael256_ctrle_oneshot_encrypt_2blocks_avx2(void* out256, const void* rijndael256key,
                                                           const void* ctr256);

/** typical rng mono-thread rng */
struct rijndael256_ctrle_state {
  uint8_t round_keys[RIJNDAEL256_RK_BYTES];
  uint8_t init_ctr[32];
  uint8_t next_ctr[32];
  uint8_t current_buf[32];
  uint64_t buf_rem_bytes;
};

typedef union {
  uint64_t v64[4];
  uint8_t v8[32];
} ctr256_t;

// PUBLIC AES_CTRLE PRNG API

/** initializes an rijndael256_ctrle prng with key and iv */
EXPORT void rijndael256_ctrle_init_key_iv_ref(void* rijndael256_ctrle, const void* rijndael256key,
                                              const void* rijndael256iv);
EXPORT void rijndael256_ctrle_init_key_iv_avx2(void* rijndael256_ctrle, const void* rijndael256key,
                                               const void* rijndael256iv);

/** initializes an rijndael256_ctrle prng with key only (iv is zero) */
EXPORT void rijndael256_ctrle_init_key_ivzero_ref(void* rijndael256_ctrle, const void* rijndael256key);
EXPORT void rijndael256_ctrle_init_key_ivzero_avx2(void* rijndael256_ctrle, const void* rijndael256key);

/** seek the rijndael256_ctrle prng to the given byte position */
EXPORT void rijndael256_ctrle_seek_ref(void* rijndael256_ctrle, uint64_t byte_position);
EXPORT void rijndael256_ctrle_seek_avx2(void* rijndael256_ctrle, uint64_t byte_position);

/** outputs the next nbytes from the rijndael256_ctrle prng  */
EXPORT void rijndael256_ctrle_get_bytes_ref(void* rijndael256_ctrle, void* out, uint64_t nbytes);
EXPORT void rijndael256_ctrle_get_bytes_avx2(void* rijndael256_ctrle, void* out, uint64_t nbytes);

#endif  // RIJNDAEL256_CTRLE_H
