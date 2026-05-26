#include "vole_private.h"

void vole_parameters_init_avx(vole_parameters* vole_params, uint64_t lambda, uint64_t tau, uint64_t kappa) {
  CASSERT(lambda > 0 && lambda >= tau * kappa, "bug? lambda is not larger than tau*kappa (check parameter order)");
  vole_params->KAPPA = kappa;
  vole_params->TAU = tau;
  vole_params->LAMBDA = lambda;
  vole_params->lambda_bytes = (lambda + 7) >> 3;
  vole_params->bitvec_xor = bitvec_xor_ref;
  vole_params->bitvec_xor_to = bitvec_xor_to_ref;
  vole_params->matrix_f2_times_vector_flambda = matrix_f2_times_vector_flambda_avx2;
  vole_params->matrix_vector_product_f2 = matrix_vector_product_f2_avx2;
  switch (lambda) {
    case 128:
      // crypto
      vole_params->proofow_rng = proofow_rng_cat1_shake128;
      vole_params->ggm_seed_rng_lr = ggm_seed_rng_lr_cat1_aes128_avx2;
      vole_params->ggm_commit_rng = ggm_commit_rng_cat1_aes128_avx2;
      vole_params->vole_rng = vole_rng_cat1_aes128_ctrle_avx2;
      vole_params->xof = xof_shake128;
      vole_params->prg = rng_cat1_aes128_ctrle_avx2;
      // arithmetic
      vole_params->flambda_set = (FLAMBDA_SET_F*)gf128_set_ref;
      vole_params->flambda_inverse = (FLAMBDA_INVERSE_F*)gf128_inverse_pclmul;
      vole_params->flambda_sum_pow2 = (FLAMBDA_SUM_POW2_F*)gf128_sum_pow2_ref;
      vole_params->flambda_sum = (FLAMBDA_SUM_F*)gf128_sum_avx2;
      vole_params->flambda_product = (FLAMBDA_PRODUCT_F*)gf128_product_pclmul;
      vole_params->flambda_product_f2 = (FLAMBDA_PRODUCT_F*)gf128_product_f2_ref;
      vole_params->flambda_echelon_pow2 = (FLAMBDA_ECHELON_POW2_F*)gf128_echelon_pow2_avx;
      vole_params->matrix_lambda_transpose = transpose_128_L_ref;
      break;
    case 192:
      // crypto
      vole_params->proofow_rng = proofow_rng_cat3_shake256;
      vole_params->ggm_seed_rng_lr = ggm_seed_rng_lr_cat3_rijndael256_avx2;
      vole_params->ggm_commit_rng = ggm_commit_rng_cat3_rijndael256_avx2;
      vole_params->vole_rng = vole_rng_cat3_rijndael256_ctrle_avx2;
      vole_params->xof = xof_shake256;
      vole_params->prg = rng_cat3_rijndael256_ctrle_avx2;
      // arithmetic
      vole_params->flambda_set = (FLAMBDA_SET_F*)gf192_set_ref;
      vole_params->flambda_inverse = (FLAMBDA_INVERSE_F*)gf192_inverse_pclmul;
      vole_params->flambda_sum_pow2 = (FLAMBDA_SUM_POW2_F*)gf192_sum_pow2_ref;
      vole_params->flambda_sum = (FLAMBDA_SUM_F*)gf192_sum_ref;
      vole_params->flambda_product = (FLAMBDA_PRODUCT_F*)gf192_product_pclmul;
      vole_params->flambda_product_f2 = (FLAMBDA_PRODUCT_F*)gf192_product_f2_ref;
      vole_params->flambda_echelon_pow2 = (FLAMBDA_ECHELON_POW2_F*)gf192_echelon_pow2_avx;
      vole_params->matrix_lambda_transpose = transpose_192_L_ref;
      break;
    case 256:
      // crypto
      vole_params->proofow_rng = proofow_rng_cat5_shake256;
      vole_params->ggm_seed_rng_lr = ggm_seed_rng_lr_cat5_rijndael256_avx2;
      vole_params->ggm_commit_rng = ggm_commit_rng_cat5_rijndael256_avx2;
      vole_params->vole_rng = vole_rng_cat5_rijndael256_ctrle_avx2;
      vole_params->xof = xof_shake256;
      vole_params->prg = rng_cat5_rijndael256_ctrle_avx2;
      // arithmetic
      vole_params->flambda_set = (FLAMBDA_SET_F*)gf256_set_ref;
      vole_params->flambda_inverse = (FLAMBDA_INVERSE_F*)gf256_inverse_pclmul;
      vole_params->flambda_sum_pow2 = (FLAMBDA_SUM_POW2_F*)gf256_sum_pow2_ref;
      vole_params->flambda_sum = (FLAMBDA_SUM_F*)gf256_sum_avx2;
      vole_params->flambda_product = (FLAMBDA_PRODUCT_F*)gf256_product_pclmul;
      vole_params->flambda_product_f2 = (FLAMBDA_PRODUCT_F*)gf256_product_pclmul_f2;
      vole_params->flambda_echelon_pow2 = (FLAMBDA_ECHELON_POW2_F*)gf256_echelon_pow2_avx;
      vole_params->matrix_lambda_transpose = transpose_256_L_ref;
      break;
    default:
      CREQUIRE(0, "lambda not supported");
  }
}
