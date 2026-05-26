#define aes128_ctrle_init_key_iv_T aes128_ctrle_init_key_iv_avx2
#define aes128_ctrle_set_key_T aes128_ctrle_set_key_avx2
#define aes128_ctrle_init_key_ivzero_T aes128_ctrle_init_key_ivzero_avx2
#define aes128_ctrle_seek_T aes128_ctrle_seek_avx2
#define aes128_ctrle_encrypt_nblocks_T aes128_ctrle_encrypt_nblocks_avx2
#define aes128_ctrle_get_bytes_T aes128_ctrle_get_bytes_avx2

#include "aes128_ctrle_generics.impl.h"

EXPORT void aes128_encrypt_1block_avx2(void* out128, const void *src128, const void* round_keys);
EXPORT void aes128_encrypt_4blocks_avx2(void* out128, const void *src128, const void* round_keys);
EXPORT void aes128_encrypt_2blocks_avx2(void* out128, const void *src128, const void* round_keys);

EXPORT void aes128_ctrle_encrypt_nblocks_avx2(void* out, void* out_last128, void* in_out_ctr128,  //
                                             const void* round_keys, uint64_t nblocks) {
  ctr128_t ctr[4];
  uint8_t* oo = (uint8_t*)out;
  memcpy(ctr[0].v8, in_out_ctr128, 16);
  while (nblocks > 4) {
    ctr[1]=ctr[0];
    if (++ctr[1].v64[0] == 0) ++ctr[1].v64[1];
    ctr[2]=ctr[1];
    if (++ctr[2].v64[0] == 0) ++ctr[2].v64[1];
    ctr[3]=ctr[2];
    if (++ctr[3].v64[0] == 0) ++ctr[3].v64[1];
    aes128_encrypt_4blocks_avx2(oo + 0, ctr[0].v8, round_keys);
    oo += 64;
    nblocks -= 4;
    ctr[0]=ctr[3];
    if (++ctr[0].v64[0] == 0) ++ctr[0].v64[1];
  }
  while (nblocks > 2) {
    ctr[1]=ctr[0];
    if (++ctr[1].v64[0] == 0) ++ctr[1].v64[1];
    aes128_encrypt_2blocks_avx2(oo + 0, ctr[0].v8, round_keys);
    oo += 32;
    nblocks -= 2;
    ctr[0]=ctr[1];
    if (++ctr[0].v64[0] == 0) ++ctr[0].v64[1];
  }
  while (nblocks > 1) {
    aes128_encrypt_1block_avx2(oo + 0, ctr[0].v8, round_keys);
    oo += 16;
    nblocks -= 1;
    if (++ctr[0].v64[0] == 0) ++ctr[0].v64[1];
  }
  // last block
  {
    aes128_encrypt_1block_avx2(out_last128, ctr[0].v8, round_keys);
    oo += 16;
    nblocks -= 1;
    if (++ctr[0].v64[0] == 0) ++ctr[0].v64[1];
    memcpy(in_out_ctr128, ctr[0].v8, 16);
  }
}
