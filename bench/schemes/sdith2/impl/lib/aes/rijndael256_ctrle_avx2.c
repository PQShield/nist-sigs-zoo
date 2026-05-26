#define rijndael256_ctrle_init_key_iv_T rijndael256_ctrle_init_key_iv_avx2
#define rijndael256_ctrle_set_key_T rijndael256_ctrle_set_key_avx2
#define rijndael256_ctrle_init_key_ivzero_T rijndael256_ctrle_init_key_ivzero_avx2
#define rijndael256_ctrle_seek_T rijndael256_ctrle_seek_avx2
#define rijndael256_ctrle_encrypt_nblocks_T rijndael256_ctrle_encrypt_nblocks_avx2
#define rijndael256_ctrle_get_bytes_T rijndael256_ctrle_get_bytes_avx2

#include "rijndael256_avx.h"
#include "rijndael256_ctrle_generics.impl.h"

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

EXPORT void rijndael256_ctrle_oneshot_encrypt_2blocks_avx2(void* out256, const void* rijndael256key,
                                                           const void* ctr256) {
  rijndael256_ctr_encrypt_2blocks_avx(out256, (uint8_t*)ctr256, rijndael256key);
}

EXPORT void rijndael256_ctrle_set_key_avx2(void* round_keys, const void* rijndael256key) {
  rijndael256_key_schedule_avx(round_keys, rijndael256key);
}

EXPORT void rijndael256_ctrle_encrypt_nblocks_avx2(void* out, void* out_last256, void* in_out_ctr256,  //
                                                   const void* round_keys, uint64_t nblocks) {
  ctr256_t ctr[4];
  uint8_t* oo = (uint8_t*)out;
  memcpy(ctr[0].v8, in_out_ctr256, 32);
  while (nblocks > 4) {
    ctr[1] = ctr[0];
    ctr256_increment_nonct(&ctr[1]);
    ctr[2] = ctr[1];
    ctr256_increment_nonct(&ctr[2]);
    ctr[3] = ctr[2];
    ctr256_increment_nonct(&ctr[3]);
    rijndael256_ecb_encrypt_4blocks_avx(oo + 0, ctr[0].v8, round_keys);
    oo += 128;
    nblocks -= 4;
    ctr[0] = ctr[3];
    ctr256_increment_nonct(&ctr[0]);
  }
  while (nblocks > 2) {
    ctr[1] = ctr[0];
    ctr256_increment_nonct(&ctr[1]);
    rijndael256_ecb_encrypt_2blocks_avx(oo + 0, ctr[0].v8, round_keys);
    oo += 64;
    nblocks -= 2;
    ctr[0] = ctr[1];
    ctr256_increment_nonct(&ctr[0]);
  }
  while (nblocks > 1) {
    rijndael256_ecb_encrypt_1block_avx(oo + 0, ctr[0].v8, round_keys);
    oo += 32;
    nblocks -= 1;
    ctr256_increment_nonct(&ctr[0]);
  }
  // last block
  {
    rijndael256_ecb_encrypt_1block_avx(out_last256, ctr[0].v8, round_keys);
    oo += 32;
    nblocks -= 1;
    ctr256_increment_nonct(&ctr[0]);
    memcpy(in_out_ctr256, ctr[0].v8, 16);
  }
}
