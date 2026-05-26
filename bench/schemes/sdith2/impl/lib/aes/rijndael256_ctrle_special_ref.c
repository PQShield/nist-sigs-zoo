#include <string.h>

#include "rijndael256.h"
#include "rijndael256_ctrle.h"

#if defined(_WIN32) || defined(__APPLE__)
#define __always_inline inline __attribute((always_inline))
#endif

__always_inline void ctr256_increment_nonct(ctr256_t* ctr) {
  ++ctr->v64[0];
  if (ctr->v64[0] == 0) {
    ++ctr->v64[1];
    if (ctr->v64[1] == 0) {
      ++ctr->v64[2];
      if (ctr->v64[2] == 0) {
        ++ctr->v64[3];
      }
    }
  }
}

EXPORT void rijndael256_ctrle_set_key_ref(void* round_keys, const void* rijndael256key) {
  rijndael256_key_schedule_ref(round_keys, rijndael256key);
}

EXPORT void rijndael256_ctrle_encrypt_nblocks_ref(void* out, void* out_last256, void* in_out_ctr256,  //
                                                  const void* round_keys, uint64_t nblocks) {
  ctr256_t ctr;
  uint8_t* oo = (uint8_t*)out;
  memcpy(ctr.v8, in_out_ctr256, 32);
  for (uint64_t i = 0; i < nblocks - 1; i++) {
    rijndael256_ecb_encrypt_1block_ref(oo + 32 * i, ctr.v8, round_keys);
    ctr256_increment_nonct(&ctr);
  }
  rijndael256_ecb_encrypt_1block_ref(out_last256, ctr.v8, round_keys);
  ctr256_increment_nonct(&ctr);
  memcpy(in_out_ctr256, ctr.v8, 32);
}

/** one-shot function */
EXPORT void rijndael256_ctrle_oneshot_encrypt_2blocks_ref(void* out256, const void* rijndael256key,
                                                          const void* ctr256) {
  ctr256_t ctr;
  rijndael256_rk_t rk;
  memcpy(ctr.v8, ctr256, 32);
  rijndael256_ctrle_set_key_ref(&rk, rijndael256key);
  uint8_t* oo = (uint8_t*)out256;
  rijndael256_ecb_encrypt_1block_ref(oo, ctr.v8, &rk);
  ctr256_increment_nonct(&ctr);
  rijndael256_ecb_encrypt_1block_ref(oo + 32, ctr.v8, &rk);
}
