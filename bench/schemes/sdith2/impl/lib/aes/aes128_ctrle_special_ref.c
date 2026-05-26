#include <string.h>

#include "aes128_ctrle.h"
#include "aes_ansi_ref.h"

EXPORT void aes128_ctrle_set_key_ref(void* round_keys, const void* aes128key) {
  aes128_set_key_ref(round_keys, aes128key);
}

EXPORT void aes128_ctrle_encrypt_nblocks_ref(void* out, void* out_last128, void* in_out_ctr128,  //
                                             const void* round_keys, uint64_t nblocks) {
  ctr128_t ctr;
  uint8_t* oo = (uint8_t*)out;
  memcpy(ctr.v8, in_out_ctr128, 16);
  for (uint64_t i = 0; i < nblocks - 1; i++) {
    aes128_encrypt_1block_ref(oo + 16 * i, ctr.v8, round_keys);
    ++ctr.u128;
  }
  aes128_encrypt_1block_ref(out_last128, ctr.v8, round_keys);
  ++ctr.u128;
  memcpy(in_out_ctr128, ctr.v8, 16);
}

/** one-shot function */
EXPORT void aes128_ctrle_oneshot_encrypt_2blocks_ref(void* out256, const void* aes128key, const void* ctr128) {
  ctr128_t ctr;
  uint8_t rk[16 * 11];
  memcpy(ctr.v8, ctr128, 16);
  aes128_ctrle_set_key_ref(rk, aes128key);
  uint8_t* oo = (uint8_t*)out256;
  aes128_encrypt_1block_ref(oo, ctr.v8, rk);
  ++ctr.u128;
  aes128_encrypt_1block_ref(oo + 16, ctr.v8, rk);
}
