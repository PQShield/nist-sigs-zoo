#include <aes128_ctrle.h>
#include <memory.h>
#include <rijndael256_ctrle.h>

#include "sdith_prng_private.h"
#include "vole_private.h"

EXPORT void ggm_seed_rng_lr_cat1_aes128_avx2(void* lr_out256, const void* salt128, const void* key128,
uint64_t node_idx) {
  CASSERT((node_idx & 1) == 0, "bug! this function must be called on the left child index");
  aesblk ctr;
  memcpy(ctr.v64, salt128, 16);
  ctr.v64[0] ^= node_idx;
  aes128_ctrle_oneshot_encrypt_2blocks_avx2(lr_out256, key128, ctr.v64);
}

EXPORT void ggm_seed_rng_lr_cat3_rijndael256_avx2(  //
    void* lr_out384, const void* salt192, const void* key192, uint64_t node_idx) {
  CASSERT((node_idx & 1) == 0, "bug! this function must be called on the left child index");
  uint8_t out512[64] = {};
  uint8_t key256[32] = {};
  ctr256_t ctr256 = {.v8 = {}};
  memcpy(ctr256.v8 + 8, salt192, 24);
  ctr256.v64[0] ^= node_idx;
  memcpy(key256 + 8, key192, 24);
  rijndael256_ctrle_oneshot_encrypt_2blocks_avx2(out512, key256, ctr256.v8);
  memcpy(lr_out384, out512 + 8, 48);
}

EXPORT void ggm_seed_rng_lr_cat5_rijndael256_avx2(  //
    void* lr_out512, const void* salt256, const void* key256, uint64_t node_idx) {
  CASSERT((node_idx & 1) == 0, "bug! this function must be called on the left child index");
  ctr256_t ctr256 = {.v8 = {}};
  memcpy(ctr256.v8, salt256, 32);
  ctr256.v64[0] ^= node_idx;
  rijndael256_ctrle_oneshot_encrypt_2blocks_avx2(lr_out512, key256, ctr256.v8);
}

EXPORT void ggm_commit_rng_cat1_aes128_avx2(void* output256, const void* salt128, const void* key128,
                                            uint64_t node_idx) {
  aesblk ctr;
  memcpy(ctr.v64, salt128, 16);
  ctr.v64[0] ^= node_idx << 1;
  aes128_ctrle_oneshot_encrypt_2blocks_avx2(output256, key128, ctr.v64);
}
EXPORT void ggm_commit_rng_cat3_rijndael256_avx2(  //
    void* output384, const void* salt192, const void* key192, uint64_t node_idx) {
  uint8_t key256[32] = {};       // pad with zeroes
  uint8_t output512[64] = {};    // pad with zeroes
  ctr256_t ctr256 = {.v8 = {}};  // pad with zeroes
  memcpy(key256 + 8, key192, 24);
  memcpy(ctr256.v8 + 8, salt192, 24);
  ctr256.v64[0] ^= node_idx << 1;
  rijndael256_ctrle_oneshot_encrypt_2blocks_avx2(output512, key256, ctr256.v8);
  memcpy(output384, output512 + 8, 48);
}
EXPORT void ggm_commit_rng_cat5_rijndael256_avx2(  //
    void* output512, const void* salt256, const void* key256, uint64_t node_idx) {
  ctr256_t ctr256 = {.v8 = {}};  // pad with zeroes
  memcpy(ctr256.v8, salt256, 32);
  ctr256.v64[0] ^= node_idx << 1;
  rijndael256_ctrle_oneshot_encrypt_2blocks_avx2(output512, key256, ctr256.v8);
}
EXPORT void vole_rng_cat1_aes128_ctrle_avx2(void* out, uint64_t out_bytes, const void* seed128) {
  rng_ctx ctx;
  aes128_ctrle_init_key_ivzero_avx2(&ctx, seed128);
  aes128_ctrle_get_bytes_avx2(&ctx, out, out_bytes);
}

EXPORT void vole_rng_cat3_rijndael256_ctrle_avx2(void* out, uint64_t out_bytes, const void* seed192) {
  uint8_t key256[32] = {};  // pad with zeroes
  memcpy(key256 + 8, seed192, 24);
  rng_ctx ctx;
  rijndael256_ctrle_init_key_ivzero_avx2(&ctx, key256);
  rijndael256_ctrle_get_bytes_avx2(&ctx, out, out_bytes);
}
EXPORT void vole_rng_cat5_rijndael256_ctrle_avx2(void* out, uint64_t out_bytes, const void* seed256) {
  rng_ctx ctx;
  rijndael256_ctrle_init_key_ivzero_avx2(&ctx, seed256);
  rijndael256_ctrle_get_bytes_avx2(&ctx, out, out_bytes);
}

EXPORT void rng_init_and_seed_iv_cat1_aes128_ctrle_avx2(rng_ctx* rng, const void* seed, const void* iv) {
  struct aes128_ctrle* inst = (struct aes128_ctrle*)rng;
  aes128_ctrle_init_key_iv_avx2(inst, seed, iv);
}
EXPORT void rng_init_and_seed_iv_cat3_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed, const void* iv) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  uint8_t seed256[32] = {};  // pad with zeroes in lsb positions
  uint8_t iv256[32] = {};    // pad with zeroes in lsb positions
  memcpy(seed256 + 8, seed, 24);
  memcpy(iv256 + 8, iv, 24);
  rijndael256_ctrle_init_key_iv_avx2(inst, seed256, iv256);
}
EXPORT void rng_init_and_seed_iv_cat5_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed, const void* iv) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_init_key_iv_avx2(inst, seed, iv);
}
EXPORT void rng_init_and_seed_ivzero_cat1_aes128_ctrle_avx2(rng_ctx* rng, const void* seed) {
  struct aes128_ctrle* inst = (struct aes128_ctrle*)rng;
  aes128_ctrle_init_key_ivzero_avx2(inst, seed);
}
EXPORT void rng_init_and_seed_ivzero_cat3_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  uint8_t seed256[32] = {};  // pad with zeroes in lsb positions
  memcpy(seed256 + 8, seed, 24);
  rijndael256_ctrle_init_key_ivzero_avx2(inst, seed256);
}
EXPORT void rng_init_and_seed_ivzero_cat5_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_init_key_ivzero_avx2(inst, seed);
}
EXPORT void rng_output_cat1_aes128_ctrle_avx2(rng_ctx* rng, void* out, uint64_t out_bytes) {
  struct aes128_ctrle* inst = (struct aes128_ctrle*) rng;
  aes128_ctrle_get_bytes_avx2(inst, out, out_bytes);
}
EXPORT void rng_output_cat3_rijndael256_ctrle_avx2(rng_ctx* rng, void* out, uint64_t out_bytes) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_get_bytes_avx2(inst, out, out_bytes);
}
EXPORT void rng_output_cat5_rijndael256_ctrle_avx2(rng_ctx* rng, void* out, uint64_t out_bytes) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_get_bytes_avx2(inst, out, out_bytes);
}

