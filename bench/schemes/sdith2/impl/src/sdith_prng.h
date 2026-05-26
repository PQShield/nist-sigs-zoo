#ifndef AQ_VOLE_IMPL_RNG_H
#define AQ_VOLE_IMPL_RNG_H

#include "commons.h"

/** proof of work function: aliasing out = key is allowed */
typedef void PROOFOW_RNG_F(void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop, uint32_t counter);
EXPORT void proofow_rng_cat1_shake128(void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop,
                                      uint32_t counter);
EXPORT void proofow_rng_cat3_shake256(void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop,
                                      uint32_t counter);
EXPORT void proofow_rng_cat5_shake256(void* delta0_and_vgrind, uint64_t out_bytes, const void* hash_piop,
                                      uint32_t counter);

typedef void GGM_SEED_RNG_LR_F(void* lr_out, const void* salt, const void* key, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat1_aes128_ref(  //
    void* lr_out256, const void* salt128, const void* key128, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat1_aes128_avx2(  //
    void* lr_out256, const void* salt128, const void* key128, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat1_shake128(  //
    void* lr_out256, const void* salt128, const void* key128, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat3_rijndael256_ref(  //
    void* lr_out384, const void* salt192, const void* key192, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat3_rijndael256_avx2(  //
    void* lr_out384, const void* salt192, const void* key192, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat3_shake256(  //
    void* lr_out384, const void* salt192, const void* key192, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat5_rijndael256_ref(  //
    void* lr_out512, const void* salt256, const void* key256, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat5_rijndael256_avx2(  //
    void* lr_out512, const void* salt256, const void* key256, uint64_t node_idx);
EXPORT void ggm_seed_rng_lr_cat5_shake256(  //
    void* lr_out512, const void* salt256, const void* key256, uint64_t node_idx);

typedef void GGM_COMMIT_RNG_F(void* out, const void* salt, const void* key, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat1_aes128_ref(  //
    void* output256, const void* salt128, const void* key128, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat1_aes128_avx2(  //
    void* output256, const void* salt128, const void* key128, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat3_rijndael256_ref(  //
    void* output384, const void* salt192, const void* key192, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat3_rijndael256_avx2(  //
    void* output384, const void* salt192, const void* key192, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat5_rijndael256_ref(  //
    void* output512, const void* salt256, const void* key256, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat5_rijndael256_avx2(  //
    void* output512, const void* salt256, const void* key256, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat1_shake128(void* output256, const void* salt128, const void* key128, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat3_shake256(void* output384, const void* salt192, const void* key192, uint64_t node_idx);
EXPORT void ggm_commit_rng_cat5_shake256(void* output512, const void* salt256, const void* key256, uint64_t node_idx);

typedef void VOLE_RNG_F(void* out, uint64_t out_bytes, const void* node_seed);
EXPORT void vole_rng_cat1_aes128_ctrle_ref(void* out, uint64_t out_bytes, const void* seed128);
EXPORT void vole_rng_cat1_aes128_ctrle_avx2(void* out, uint64_t out_bytes, const void* seed128);
EXPORT void vole_rng_cat3_rijndael256_ctrle_ref(void* out, uint64_t out_bytes, const void* seed192);
EXPORT void vole_rng_cat3_rijndael256_ctrle_avx2(void* out, uint64_t out_bytes, const void* seed192);
EXPORT void vole_rng_cat5_rijndael256_ctrle_ref(void* out, uint64_t out_bytes, const void* seed192);
EXPORT void vole_rng_cat5_rijndael256_ctrle_avx2(void* out, uint64_t out_bytes, const void* seed192);

typedef struct xof_ctx_t {
  uint64_t DUMMY[224 / 8];
} xof_ctx;

typedef void XOF_INIT_F(xof_ctx* xof);
EXPORT void xof_init_shake128(xof_ctx* xof);
EXPORT void xof_init_shake256(xof_ctx* xof);

typedef void XOF_SEED_F(xof_ctx* xof, const void* in, uint64_t in_bytes);
EXPORT void xof_seed_shake128(xof_ctx* xof, const void* in, uint64_t in_bytes);
EXPORT void xof_seed_shake256(xof_ctx* xof, const void* in, uint64_t in_bytes);

typedef void XOF_FINALIZE_F(xof_ctx* xof);
EXPORT void xof_finalize_shake128(xof_ctx* xof);
EXPORT void xof_finalize_shake256(xof_ctx* xof);

typedef void XOF_OUTPUT_F(xof_ctx* xof, void* out, uint64_t out_bytes);
EXPORT void xof_output_shake128(xof_ctx* xof, void* out, uint64_t out_bytes);
EXPORT void xof_output_shake256(xof_ctx* xof, void* out, uint64_t out_bytes);

// shortcut combo
typedef void XOF_INIT_AND_SEED_F(xof_ctx* xof, const void* in, uint64_t in_bytes);
EXPORT void xof_init_and_seed_shake128(xof_ctx* xof, const void* in, uint64_t in_bytes);
EXPORT void xof_init_and_seed_shake256(xof_ctx* xof, const void* in, uint64_t in_bytes);

typedef void XOF_FINALIZE_AND_OUTPUT_F(xof_ctx* xof, void* out, uint64_t out_bytes);
EXPORT void xof_finalize_and_output_shake128(xof_ctx* xof, void* out, uint64_t out_bytes);
EXPORT void xof_finalize_and_output_shake256(xof_ctx* xof, void* out, uint64_t out_bytes);

typedef struct xof_functions_t {
  XOF_INIT_F* xof_init;
  XOF_SEED_F* xof_seed;
  XOF_FINALIZE_F* xof_finalize;
  XOF_OUTPUT_F* xof_output;
  XOF_INIT_AND_SEED_F* xof_init_and_seed;
  XOF_FINALIZE_AND_OUTPUT_F* xof_finalize_and_output;
} xof_functions;

static const xof_functions xof_shake128 = {
    xof_init_shake128,                //
    xof_seed_shake128,                //
    xof_finalize_shake128,            //
    xof_output_shake128,              //
    xof_init_and_seed_shake128,       //
    xof_finalize_and_output_shake128  //
};
static const xof_functions xof_shake256 = {
    xof_init_shake256,                //
    xof_seed_shake256,                //
    xof_finalize_shake256,            //
    xof_output_shake256,              //
    xof_init_and_seed_shake256,       //
    xof_finalize_and_output_shake256  //
};

// must be large enough to store: a shake ctx, an aes128_ctrle state, or a rijndael256 ctrle state
typedef struct rng_ctx_t {
  uint64_t DUMMY[608];
} rng_ctx;

typedef void RNG_INIT_AND_SEED_IV_F(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat1_shake128(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat3_shake256(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat5_shake256(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat1_aes128_ctrle_ref(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat1_aes128_ctrle_avx2(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat3_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat3_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat5_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed, const void* iv);
EXPORT void rng_init_and_seed_iv_cat5_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed, const void* iv);

typedef void RNG_INIT_AND_SEED_IVZERO_F(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat1_shake128(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat3_shake256(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat5_shake256(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat1_aes128_ctrle_ref(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat1_aes128_ctrle_avx2(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat3_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat3_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat5_rijndael256_ctrle_ref(rng_ctx* rng, const void* seed);
EXPORT void rng_init_and_seed_ivzero_cat5_rijndael256_ctrle_avx2(rng_ctx* rng, const void* seed);

typedef void RNG_OUTPUT_F(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat1_shake128(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat3_shake256(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat5_shake256(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat1_aes128_ctrle_ref(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat1_aes128_ctrle_avx2(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat3_rijndael256_ctrle_ref(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat3_rijndael256_ctrle_avx2(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat5_rijndael256_ctrle_ref(rng_ctx* rng, void* out, uint64_t out_bytes);
EXPORT void rng_output_cat5_rijndael256_ctrle_avx2(rng_ctx* rng, void* out, uint64_t out_bytes);

typedef struct rng_functions_t {
  RNG_INIT_AND_SEED_IV_F* rng_init_and_seed_iv;
  RNG_INIT_AND_SEED_IVZERO_F* rng_init_and_seed_ivzero;
  RNG_OUTPUT_F* rng_output;
} rng_functions;

static const rng_functions rng_cat1_shake128 = {
    rng_init_and_seed_iv_cat1_shake128,      //
    rng_init_and_seed_ivzero_cat1_shake128,  //
    rng_output_cat1_shake128                 //
};
static const rng_functions rng_cat3_shake256 = {
    rng_init_and_seed_iv_cat3_shake256,      //
    rng_init_and_seed_ivzero_cat3_shake256,  //
    rng_output_cat3_shake256                 //
};
static const rng_functions rng_cat5_shake256 = {
    rng_init_and_seed_iv_cat5_shake256,      //
    rng_init_and_seed_ivzero_cat5_shake256,  //
    rng_output_cat5_shake256                 //
};
static const rng_functions rng_cat1_aes128_ctrle_ref = {
    rng_init_and_seed_iv_cat1_aes128_ctrle_ref,      //
    rng_init_and_seed_ivzero_cat1_aes128_ctrle_ref,  //
    rng_output_cat1_aes128_ctrle_ref                 //
};
static const rng_functions rng_cat3_rijndael256_ctrle_ref = {
    rng_init_and_seed_iv_cat3_rijndael256_ctrle_ref,      //
    rng_init_and_seed_ivzero_cat3_rijndael256_ctrle_ref,  //
    rng_output_cat3_rijndael256_ctrle_ref                 //
};
static const rng_functions rng_cat5_rijndael256_ctrle_ref = {
    rng_init_and_seed_iv_cat5_rijndael256_ctrle_ref,      //
    rng_init_and_seed_ivzero_cat5_rijndael256_ctrle_ref,  //
    rng_output_cat5_rijndael256_ctrle_ref                 //
};
#ifdef __x86_64__
static const rng_functions rng_cat1_aes128_ctrle_avx2 = {
    rng_init_and_seed_iv_cat1_aes128_ctrle_avx2,      //
    rng_init_and_seed_ivzero_cat1_aes128_ctrle_avx2,  //
    rng_output_cat1_aes128_ctrle_avx2                 //
};
static const rng_functions rng_cat3_rijndael256_ctrle_avx2 = {
    rng_init_and_seed_iv_cat3_rijndael256_ctrle_avx2,      //
    rng_init_and_seed_ivzero_cat3_rijndael256_ctrle_avx2,  //
    rng_output_cat3_rijndael256_ctrle_avx2                 //
};
static const rng_functions rng_cat5_rijndael256_ctrle_avx2 = {
    rng_init_and_seed_iv_cat5_rijndael256_ctrle_avx2,      //
    rng_init_and_seed_ivzero_cat5_rijndael256_ctrle_avx2,  //
    rng_output_cat5_rijndael256_ctrle_avx2                 //
};
#endif

#endif  // AQ_VOLE_IMPL_RNG_H
