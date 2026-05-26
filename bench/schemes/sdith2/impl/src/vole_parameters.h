#ifndef VOLE_PARAMETERS_H
#define VOLE_PARAMETERS_H

#include "sdith_prng.h"

typedef struct full_ggm_tree_t full_ggm_tree;
typedef struct ggm_multi_sibling_tree_t ggm_multi_sibling_tree;

typedef void BITVEC_XOR_TO_F(uint64_t bytelen, bitvec_t* res, const bitvec_t* b);
typedef void BITVEC_XOR_F(uint64_t bytelen, bitvec_t* res, const bitvec_t* a, const bitvec_t* b);
typedef void FLAMBDA_PRODUCT_F(flambda_t* res, const flambda_t* a, const flambda_t* b);
typedef void FLAMBDA_PRODUCT_F2_F(flambda_t* res, const flambda_t* a, const flambda_t* b_f2);
typedef void FLAMBDA_SUM_F(flambda_t* res, const flambda_t* a, const flambda_t* b);
typedef void FLAMBDA_SUM_POW2_F(flambda_t* res, const flambda_t* a);
typedef void FLAMBDA_ECHELON_POW2_F(uint64_t k, flambda_t* res, const flambda_t* a, uint64_t a_size,
                                    uint64_t a_byte_slice);
typedef void FLAMBDA_SET_F(flambda_t* res, const flambda_t* a);
typedef void FLAMBDA_INVERSE_F(flambda_t* res, const flambda_t* a);
typedef void MATRIX_LAMBDA_TRANSPOSE_F(bitmat_t* out, const bitmat_t* in, uint64_t L);
typedef void MATRIX_VECTOR_PRODUCT_F2(                //
    uint64_t nrows, uint64_t ncols,                   // dimensions
    bitvec_t* res,                                    // bit vector of size nrows. res=a*b
    const bitmat_t* a,                                // nrows x ncols matrix
    const bitvec_t* b);                               // bit vector of size ncols
typedef void MATRIX_F2_TIMES_VECTOR_FLAMBDA(          //
    uint64_t lambda, uint64_t nrows, uint64_t ncols,  //
    flambda_t* res,                                   // vector of size nrows. res = a*b
    const bitmat_t* a,                                // nrows x ncols matrix
    const flambda_t* b);

typedef void GGM_SEED_RNG_LR_F(void* lr_out, const void* salt, const void* key, uint64_t node_idx);
typedef void GGM_COMMIT_RNG_F(void* out, const void* salt, const void* key, uint64_t node_idx);

typedef struct vole_parameters_t vole_parameters;
struct vole_parameters_t {
  uint64_t TAU;
  uint64_t KAPPA;
  uint64_t LAMBDA;
  uint64_t lambda_bytes;
  // ptr to the prng function?
  BITVEC_XOR_TO_F* bitvec_xor_to;
  BITVEC_XOR_F* bitvec_xor;
  FLAMBDA_SET_F* flambda_set;
  FLAMBDA_INVERSE_F* flambda_inverse;
  FLAMBDA_PRODUCT_F* flambda_product;
  FLAMBDA_PRODUCT_F* flambda_product_f2;
  FLAMBDA_SUM_F* flambda_sum;
  FLAMBDA_SUM_POW2_F* flambda_sum_pow2;
  FLAMBDA_ECHELON_POW2_F* flambda_echelon_pow2;
  MATRIX_LAMBDA_TRANSPOSE_F* matrix_lambda_transpose;
  MATRIX_VECTOR_PRODUCT_F2* matrix_vector_product_f2;
  MATRIX_F2_TIMES_VECTOR_FLAMBDA* matrix_f2_times_vector_flambda;
  // rng functions
  PROOFOW_RNG_F* proofow_rng;
  GGM_SEED_RNG_LR_F* ggm_seed_rng_lr;
  GGM_COMMIT_RNG_F* ggm_commit_rng;
  VOLE_RNG_F* vole_rng;
  xof_functions xof;
  rng_functions prg;
};

EXPORT void vole_parameters_init_ref(vole_parameters* vole_params, uint64_t lambda, uint64_t tau, uint64_t kappa);

EXPORT void vole_parameters_init_avx(vole_parameters* vole_params, uint64_t lambda, uint64_t tau, uint64_t kappa);

EXPORT void vole_parameters_init(vole_parameters* vole_params, uint64_t lambda, uint64_t tau, uint64_t kappa);

#endif  // VOLE_PARAMETERS_H
