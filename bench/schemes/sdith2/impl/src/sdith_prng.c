#include "sdith_prng.h"

#include <stdlib.h>
#include <string.h>

#include "../aes/aes128_ctrle.h"
#include "../sha3/KeccakHash.h"
#include "../sha3/KeccakHashtimes4.h"
#include "rijndael256_ctrle.h"
#include "sdith_prng_private.h"
#include "vole_private.h"

#define STATIC_ASSERT(assert_name, condition) \
  uint8_t assert_name[(condition)?1:-1]

// this is used during the proof of work to draw delta0
EXPORT void proofow_rng_cat1_shake128(  //
    void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop, uint32_t counter) {
  xof_ctx ctx __attribute((aligned(16)));
  xof_init_and_seed_shake128(&ctx, hash_piop, 32);
  xof_seed_shake128(&ctx, &counter, 4);
  xof_finalize_and_output_shake128(&ctx, delta0_and_vgrind, out_bytes);
}
EXPORT void proofow_rng_cat3_shake256(  //
    void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop, uint32_t counter) {
  xof_ctx ctx __attribute((aligned(16)));
  xof_init_and_seed_shake256(&ctx, hash_piop, 48);
  xof_seed_shake256(&ctx, &counter, 4);
  xof_finalize_and_output_shake256(&ctx, delta0_and_vgrind, out_bytes);
}
EXPORT void proofow_rng_cat5_shake256(  //
    void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop, uint32_t counter) {
  xof_ctx ctx __attribute((aligned(16)));
  xof_init_and_seed_shake256(&ctx, hash_piop, 64);
  xof_seed_shake256(&ctx, &counter, 4);
  xof_finalize_and_output_shake256(&ctx, delta0_and_vgrind, out_bytes);
}

// this is used during ggm tree operations to derive seeds
EXPORT void ggm_seed_rng_lr_cat1_aes128_ref(void* lr_out256, const void* salt128, const void* key128,
                                            uint64_t node_idx) {
  CASSERT((node_idx & 1) == 0, "bug! this function must be called on the left child index");
  aesblk ctr;
  memcpy(ctr.v64, salt128, 16);
  ctr.v64[0] ^= node_idx;
  aes128_ctrle_oneshot_encrypt_2blocks_ref(lr_out256, key128, ctr.v64);
}
EXPORT void ggm_seed_rng_lr_cat3_rijndael256_ref(  //
    void* lr_out384, const void* salt192, const void* key192, uint64_t node_idx) {
  CASSERT((node_idx & 1) == 0, "bug! this function must be called on the left child index");
  uint8_t out512[64] = {};
  uint8_t key256[32] = {};
  ctr256_t ctr256 = {.v8 = {}};
  memcpy(ctr256.v8 + 8, salt192, 24);
  ctr256.v64[0] ^= node_idx;
  memcpy(key256 + 8, key192, 24);
  rijndael256_ctrle_oneshot_encrypt_2blocks_ref(out512, key256, ctr256.v8);
  memcpy(lr_out384, out512 + 8, 48);
}
EXPORT void ggm_seed_rng_lr_cat5_rijndael256_ref(  //
    void* lr_out512, const void* salt256, const void* key256, uint64_t node_idx) {
  CASSERT((node_idx & 1) == 0, "bug! this function must be called on the left child index");
  ctr256_t ctr256 = {.v8 = {}};
  memcpy(ctr256.v8, salt256, 32);
  ctr256.v64[0] ^= node_idx;
  rijndael256_ctrle_oneshot_encrypt_2blocks_ref(lr_out512, key256, ctr256.v8);
}
EXPORT void ggm_seed_rng_lr_cat1_shake128(void* lr_out256, const void* salt128, const void* key128, uint64_t node_idx) {
  xof_ctx xof;
  xof_init_and_seed_shake128(&xof, key128, 16);
  xof_seed_shake128(&xof, salt128, 16);
  xof_seed_shake128(&xof, &node_idx, 4);  // taking the node idx over 4 bytes
  xof_finalize_and_output_shake128(&xof, lr_out256, 32);
}
EXPORT void ggm_seed_rng_lr_cat3_rijndael256(void* lr_out384, const void* salt192, const void* key192,
                                             uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat3_shake256(void* lr_out384, const void* salt192, const void* key192, uint64_t node_idx) {
  xof_ctx xof;
  xof_init_and_seed_shake256(&xof, key192, 24);
  xof_seed_shake256(&xof, salt192, 24);
  xof_seed_shake256(&xof, &node_idx, 4);  // taking the node idx over 4 bytes
  xof_finalize_and_output_shake256(&xof, lr_out384, 48);
}
EXPORT void ggm_seed_rng_lr_cat5_rijndael256(void* lr_out512, const void* salt256, const void* key256,
                                             uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat5_shake256(void* lr_out512, const void* salt256, const void* key256, uint64_t node_idx) {
  xof_ctx xof;
  xof_init_and_seed_shake256(&xof, key256, 32);
  xof_seed_shake256(&xof, salt256, 32);
  xof_seed_shake256(&xof, &node_idx, 4);  // taking the node idx over 4 bytes
  xof_finalize_and_output_shake256(&xof, lr_out512, 64);
}

typedef void GGM_COMMIT_RNG_F(void* out, const void* salt, const void* key, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat1_aes128_ref(void* output256, const void* salt128, const void* key128, uint64_t node_idx) {
  aesblk ctr;
  memcpy(ctr.v64, salt128, 16);
  ctr.v64[0] ^= node_idx << 1;
  aes128_ctrle_oneshot_encrypt_2blocks_ref(output256, key128, ctr.v64);
}
EXPORT void ggm_commit_rng_cat3_rijndael256_ref(  //
    void* output384, const void* salt192, const void* key192, uint64_t node_idx) {
  uint8_t key256[32] = {};       // pad with zeroes
  uint8_t output512[64] = {};    // pad with zeroes
  ctr256_t ctr256 = {.v8 = {}};  // pad with zeroes
  memcpy(key256 + 8, key192, 24);
  memcpy(ctr256.v8 + 8, salt192, 24);
  ctr256.v64[0] ^= node_idx << 1;
  rijndael256_ctrle_oneshot_encrypt_2blocks_ref(output512, key256, ctr256.v8);
  memcpy(output384, output512 + 8, 48);
}
EXPORT void ggm_commit_rng_cat5_rijndael256_ref(  //
    void* output512, const void* salt256, const void* key256, uint64_t node_idx) {
  ctr256_t ctr256 = {.v8 = {}};
  memcpy(ctr256.v8, salt256, 32);
  ctr256.v64[0] ^= node_idx << 1;
  rijndael256_ctrle_oneshot_encrypt_2blocks_ref(output512, key256, ctr256.v8);
}


EXPORT void ggm_commit_rng_cat1_shake128(void* output256, const void* salt128, const void* key128, uint64_t node_idx) {
  node_idx |= UINT64_C(1) << 31;  // differentiator
  xof_ctx xof;
  xof_init_and_seed_shake128(&xof, key128, 16);
  xof_seed_shake128(&xof, salt128, 16);
  xof_seed_shake128(&xof, &node_idx, 4);  // taking the node idx over 4 bytes
  xof_finalize_and_output_shake128(&xof, output256, 32);
}
EXPORT void ggm_commit_rng_cat3_shake256(void* output384, const void* salt192, const void* key192, uint64_t node_idx) {
  node_idx |= UINT64_C(1) << 31;  // differentiator
  xof_ctx xof;
  xof_init_and_seed_shake256(&xof, key192, 24);
  xof_seed_shake256(&xof, salt192, 24);
  xof_seed_shake256(&xof, &node_idx, 4);  // taking the node idx over 4 bytes
  xof_finalize_and_output_shake256(&xof, output384, 48);
}
EXPORT void ggm_commit_rng_cat5_shake256(void* output512, const void* salt256, const void* key256, uint64_t node_idx) {
  node_idx |= UINT64_C(1) << 31;  // differentiator
  xof_ctx xof;
  xof_init_and_seed_shake256(&xof, key256, 32);
  xof_seed_shake256(&xof, salt256, 32);
  xof_seed_shake256(&xof, &node_idx, 4);  // taking the node idx over 4 bytes
  xof_finalize_and_output_shake256(&xof, output512, 64);
}

EXPORT void vole_rng_cat1_aes128_ctrle_ref(void* out, uint64_t out_bytes, const void* seed128) {
  rng_ctx ctx;
  aes128_ctrle_init_key_ivzero_ref(&ctx, seed128);
  aes128_ctrle_get_bytes_ref(&ctx, out, out_bytes);
}
EXPORT void vole_rng_cat3_rijndael256_ctrle_ref(void* out, uint64_t out_bytes, const void* seed192) {
  uint8_t key256[32] = {};  // pad with zeroes
  memcpy(key256 + 8, seed192, 24);
  rng_ctx ctx;
  rijndael256_ctrle_init_key_ivzero_ref(&ctx, key256);
  rijndael256_ctrle_get_bytes_ref(&ctx, out, out_bytes);
}
EXPORT void vole_rng_cat5_rijndael256_ctrle_ref(void* out, uint64_t out_bytes, const void* seed256) {
  rng_ctx ctx;
  rijndael256_ctrle_init_key_ivzero_ref(&ctx, seed256);
  rijndael256_ctrle_get_bytes_ref(&ctx, out, out_bytes);
}

EXPORT void xof_init_shake128(xof_ctx* xof) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashInitialize_SHAKE128(inst);
}
EXPORT void xof_init_shake256(xof_ctx* xof) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashInitialize_SHAKE256(inst);
}

EXPORT void xof_seed_shake128(xof_ctx* xof, const void* in, uint64_t in_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashUpdate(inst, in, in_bytes << 3);
}
EXPORT void xof_seed_shake256(xof_ctx* xof, const void* in, uint64_t in_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashUpdate(inst, in, in_bytes << 3);
}

EXPORT void xof_finalize_shake128(xof_ctx* xof) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashFinal(inst, NULL);
}
EXPORT void xof_finalize_shake256(xof_ctx* xof) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashFinal(inst, NULL);
}

EXPORT void xof_output_shake128(xof_ctx* xof, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}
EXPORT void xof_output_shake256(xof_ctx* xof, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}

// shortcut combo
EXPORT void xof_init_and_seed_shake128(xof_ctx* xof, const void* in, uint64_t in_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashInitialize_SHAKE128(inst);
  Keccak_HashUpdate(inst, in, in_bytes << 3);
}
EXPORT void xof_init_and_seed_shake256(xof_ctx* xof, const void* in, uint64_t in_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashInitialize_SHAKE256(inst);
  Keccak_HashUpdate(inst, in, in_bytes << 3);
}

EXPORT void xof_finalize_and_output_shake128(xof_ctx* xof, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashFinal(inst, NULL);
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}
EXPORT void xof_finalize_and_output_shake256(xof_ctx* xof, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)xof;
  Keccak_HashFinal(inst, NULL);
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}

EXPORT void rng_init_and_seed_iv_cat1_shake128(rng_ctx* rng, const void* seed, const void* iv) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashInitialize_SHAKE128(inst);
  Keccak_HashUpdate(inst, seed, 128);
  Keccak_HashUpdate(inst, iv, 128);
  Keccak_HashFinal(inst, NULL);
}
EXPORT void rng_init_and_seed_iv_cat3_shake256(rng_ctx* rng, const void* seed, const void* iv) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashInitialize_SHAKE256(inst);
  Keccak_HashUpdate(inst, seed, 192);
  Keccak_HashUpdate(inst, iv, 192);
  Keccak_HashFinal(inst, NULL);
}
EXPORT void rng_init_and_seed_iv_cat5_shake256(rng_ctx* rng, const void* seed, const void* iv) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashInitialize_SHAKE256(inst);
  Keccak_HashUpdate(inst, seed, 256);
  Keccak_HashUpdate(inst, iv, 256);
  Keccak_HashFinal(inst, NULL);
}
EXPORT void rng_init_and_seed_iv_cat1_aes128_ctrle_ref(rng_ctx* rng, const void* seed, const void* iv) {
  struct aes128_ctrle* inst = (struct aes128_ctrle*)rng;
  aes128_ctrle_init_key_iv_ref(inst, seed, iv);
}
EXPORT void rng_init_and_seed_iv_cat3_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed, const void* iv) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  uint8_t seed256[32] = {};  // pad with zeroes in lsb positions
  uint8_t iv256[32] = {};    // pad with zeroes in lsb positions
  memcpy(seed256 + 8, seed, 24);
  memcpy(iv256 + 8, iv, 24);
  rijndael256_ctrle_init_key_iv_ref(inst, seed256, iv256);
}
EXPORT void rng_init_and_seed_iv_cat5_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed, const void* iv) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_init_key_iv_ref(inst, seed, iv);
}

EXPORT void rng_init_and_seed_ivzero_cat1_shake128(rng_ctx* rng, const void* seed) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashInitialize_SHAKE128(inst);
  Keccak_HashUpdate(inst, seed, 128);
  Keccak_HashFinal(inst, NULL);
}

EXPORT void rng_init_and_seed_ivzero_cat3_shake256(rng_ctx* rng, const void* seed) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashInitialize_SHAKE256(inst);
  Keccak_HashUpdate(inst, seed, 192);
  Keccak_HashFinal(inst, NULL);
}

EXPORT void rng_init_and_seed_ivzero_cat5_shake256(rng_ctx* rng, const void* seed) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashInitialize_SHAKE256(inst);
  Keccak_HashUpdate(inst, seed, 256);
  Keccak_HashFinal(inst, NULL);
}

EXPORT void rng_init_and_seed_ivzero_cat1_aes128_ctrle_ref(rng_ctx* rng, const void* seed) {
  struct aes128_ctrle* inst = (struct aes128_ctrle*)rng;
  aes128_ctrle_init_key_ivzero_ref(inst, seed);
}
EXPORT void rng_init_and_seed_ivzero_cat3_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  uint8_t seed256[32] = {};  // pad with zeroes in lsb positions
  memcpy(seed256 + 8, seed, 24);
  rijndael256_ctrle_init_key_ivzero_ref(inst, seed256);
}
EXPORT void rng_init_and_seed_ivzero_cat5_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_init_key_ivzero_ref(inst, seed);
}


EXPORT void rng_output_cat1_shake128(rng_ctx* rng, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}
EXPORT void rng_output_cat3_shake256(rng_ctx* rng, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}
EXPORT void rng_output_cat5_shake256(rng_ctx* rng, void* out, uint64_t out_bytes) {
  Keccak_HashInstance* const inst = (Keccak_HashInstance*)rng;
  Keccak_HashSqueeze(inst, out, out_bytes << 3);
}
EXPORT void rng_output_cat1_aes128_ctrle_ref(rng_ctx* rng, void* out, uint64_t out_bytes) {
  struct aes128_ctrle* inst = (struct aes128_ctrle*)rng;
  aes128_ctrle_get_bytes_ref(inst, out, out_bytes);
}

EXPORT void rng_output_cat3_rijndael256_ctrle_ref(rng_ctx* rng, void* out, uint64_t out_bytes) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_get_bytes_ref(inst, out, out_bytes);
}
EXPORT void rng_output_cat5_rijndael256_ctrle_ref(rng_ctx* rng, void* out, uint64_t out_bytes) {
  struct rijndael256_ctrle_state* inst = (struct rijndael256_ctrle_state*)rng;
  rijndael256_ctrle_get_bytes_ref(inst, out, out_bytes);
}

