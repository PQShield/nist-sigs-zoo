#ifndef AES128_CTRLE_H
#define AES128_CTRLE_H

#ifdef __cplusplus
#define EXPORT extern "C"
#include <cstdint>
#else
#define EXPORT
#include "stdint.h"
#endif

/**
 * (special API) expands the key into round keys
 * @param round_keys [out] round key must be a 16*11 byte array
 * @param aes128key [in] the 128 bit aes key
 */
EXPORT void aes128_ctrle_set_key_ref(void* round_keys, const void* aes128key);
EXPORT void aes128_ctrle_set_key_avx2(void* round_keys, const void* aes128key);

/**
 * (special API) ase128 encrypts an exact number of blocks in ctrle mode
 * @param out [out] the first nblocks-1 output blocks
 * @param out_last128 [out] the last output blocks
 * @param in_out_ctr128 [in/out] the current counter (at the end, it is input_ctr + nblocks)
 * @param round_keys [in] round key must be a 16*11 byte array
 * @param nblocks [in] the number of blocks to encrypt
 */
EXPORT void aes128_ctrle_encrypt_nblocks_ref(void* out, void* out_last128, void* in_out_ctr128,  //
                                             const void* round_keys, uint64_t nblocks);
EXPORT void aes128_ctrle_encrypt_nblocks_avx2(void* out, void* out_last128, void* in_out_ctr128,  //
                                              const void* round_keys, uint64_t nblocks);

/** (special API) one-shot encryption function
 * @param out256 [out] 2 blocks
 * @param aes128key [in] the 128 bit aes key
 * @param ctr128 [in] start counter
 */
EXPORT void aes128_ctrle_oneshot_encrypt_2blocks_ref(void* out256, const void* aes128key, const void* ctr128);
EXPORT void aes128_ctrle_oneshot_encrypt_2blocks_avx2(void* out256, const void* aes128key, const void* ctr128);

/** typical rng mono-thread rng */
struct aes128_ctrle_state {
  uint8_t round_keys[16 * 11];
  uint8_t init_ctr[16];
  uint8_t next_ctr[16];
  uint8_t current_buf[16];
  uint64_t buf_rem_bytes;
};

typedef union {
  __uint128_t u128;
  uint64_t v64[2];
  uint8_t v8[16];
} ctr128_t;

// PUBLIC AES_CTRLE PRNG API

/** initializes an aes128_ctrle prng with key and iv */
EXPORT void aes128_ctrle_init_key_iv_ref(void* aes128_ctrle, const void* aes128key, const void* aes128iv);
EXPORT void aes128_ctrle_init_key_iv_avx2(void* aes128_ctrle, const void* aes128key, const void* aes128iv);

/** initializes an aes128_ctrle prng with key only (iv is zero) */
EXPORT void aes128_ctrle_init_key_ivzero_ref(void* aes128_ctrle, const void* aes128key);
EXPORT void aes128_ctrle_init_key_ivzero_avx2(void* aes128_ctrle, const void* aes128key);

/** seek the aes128_ctrle prng to the given byte position */
EXPORT void aes128_ctrle_seek_ref(void* aes128_ctrle, uint64_t byte_position);
EXPORT void aes128_ctrle_seek_avx2(void* aes128_ctrle, uint64_t byte_position);

/** outputs the next nbytes from the aes128_ctrle prng  */
EXPORT void aes128_ctrle_get_bytes_ref(void* aes128_ctrle, void* out, uint64_t nbytes);
EXPORT void aes128_ctrle_get_bytes_avx2(void* aes128_ctrle, void* out, uint64_t nbytes);

#endif  // AES128_CTRLE_H
