#include <string.h>

#include "ggm.h"
#include "sdith_prng.h"
#include "vole_private.h"

#ifndef NDEBUG
#define MSTACK_DEF(bytelen)                             \
  uint8_t stack_var[bytelen] __attribute((aligned(8))); \
  uint8_t* stack_cur = stack_var;                       \
  uint8_t* const stack_end = stack_var + bytelen

#define MSTACK_ALLOC(bytelen) \
  (void*)stack_cur;           \
  stack_cur += bytelen;       \
  CREQUIRE(stack_cur <= stack_end, "manual stack overflow!!")
#else
#define MSTACK_DEF(bytelen)                             \
  uint8_t stack_var[bytelen] __attribute((aligned(8))); \
  uint8_t* stack_cur = stack_var

#define MSTACK_ALLOC(bytelen) \
  (void*)stack_cur;           \
  stack_cur += bytelen
#endif

/** @brief converts delta0 to delta1
 * delta0 has kappa*tau bits, encoding tau grey code encodings
 * delta1 has lambda bits, only the lowest kappa*tau are used, the rest is zero.
 */
EXPORT void delta1_from_delta0_ref(const vole_parameters* vole_params, flambda_t* delta1, const bitvec_t* delta0) {
  // bit_vector delta1_from_delta0(uint64_t kappa, uint64_t tau, uint64_t lambda, const bit_vector& delta0) {
  const uint64_t kappa = vole_params->KAPPA;
  const uint64_t tau = vole_params->TAU;
  CASSERT(vole_params->LAMBDA >= tau * kappa, "invalid lambda");
  CASSERT(tau * kappa > 0, "invalid kappa, tau");
  CASSERT(delta0 != delta1, "in_place not supported");
  memset(delta1, 0, vole_params->lambda_bytes);
  for (uint64_t i = 0; i < tau; i++) {
    uint64_t pos = extract_kappabit_uint(kappa, i * kappa, delta0);
    xorto_kappabit_uint(kappa, i * kappa, delta1, pos ^ (pos >> 1));
  }
}

/**
 * Generate L vole pairs over (F2,F2^KAPPA) (grey code version)
 * @param seeds (input): a matrix of N=2^KAPPA times L bits
 * @param u (output): L bits:  Xor of all ri's,
 * @param v (output): matrix KAPPA x L bits: Xor of ri.grey(i)
 */
EXPORT void prover_generate_midsize_grey_vole_from_seeds_ct_ref(  //
    const vole_parameters* vole_params,                           // dimensions
    uint64_t L,                                                   // number of pairs to generate
    xof_ctx* commits,                                             // [out] where to hash the leaves commit
    bitmat_t* u,                                                  // [out] TAU x L bits
    bitmat_t* v,                                                  // [out] TAU x KAPPA x L bits row major
    full_ggm_tree* tree)                                          // [in]  ggm tree
{
  const uint64_t Lbytes = L >> 3;
  const uint64_t TAU = vole_params->TAU;
  const uint64_t KAPPA = vole_params->KAPPA;
  const uint64_t LAMBDA_BYTES = vole_params->lambda_bytes;
  const uint64_t nleaves = UINT64_C(1) << KAPPA;
  CASSERT(L % 8 == 0, "currently, we support only L multiple of 8");
  // tmp space of 3*LAMBDA_BYTES + 2*TAU*8 + Lbytes
  MSTACK_DEF(1024);
  seed_t* seed = MSTACK_ALLOC(LAMBDA_BYTES);
  commit_t* commit = MSTACK_ALLOC(2 * LAMBDA_BYTES);
  bitvec_t** uu = (bitvec_t**)MSTACK_ALLOC(TAU * sizeof(bitvec_t*));
  bitmat_t** vv = (bitmat_t**)MSTACK_ALLOC(TAU * sizeof(bitmat_t*));
  bitvec_t* tmp_r = MSTACK_ALLOC(Lbytes);

  for (uint64_t i = 0; i < TAU; ++i) {
    uu[i] = u + i * Lbytes;
    vv[i] = v + i * KAPPA * Lbytes;
  }

  memset(u, 0, TAU * Lbytes);
  memset(v, 0, TAU * KAPPA * Lbytes);
  uint64_t leaf_idx = 0;
  for (uint64_t l = 0; l < nleaves; ++l) {
    uint64_t loc = (l + 1 < nleaves) ? binval_of(l + 1) : KAPPA - 1;
    uint64_t loc_offset = loc * Lbytes;
    for (uint64_t i = 0; i < TAU; ++i, ++leaf_idx) {
      full_ggm_tree_get_leaf_seed_commit(tree, leaf_idx, seed, commit);
      vole_params->xof.xof_seed(commits, commit, 2 * LAMBDA_BYTES);
      vole_params->vole_rng(tmp_r, Lbytes, seed);
      vole_params->bitvec_xor_to(Lbytes, uu[i], tmp_r);
      vole_params->bitvec_xor_to(Lbytes, vv[i] + loc_offset, uu[i]);
    }
  }
}

/**
 * Open L vole pairs over (F2,F2^KAPPA) (grey code version)
 */
EXPORT void verifier_open_midsize_grey_vole_from_seeds_ref(  //
    const vole_parameters* vole_params,                      // dimensions
    uint64_t L,                                              // number of pairs to generate
    xof_ctx* commits,                                        // [out] where to hash the leaves commits
    bitmat_t* q,                                             // [out] TAU x KAPPA x L row major
    bitmat_t* coor_terms,                                    // [in/out] TAU x L bits
    ggm_multi_sibling_tree* stree,                           // multi-sibling-tree
    const flambda_t* delta1)                                 // verifier's point delta1
{
  const uint64_t Lbytes = L >> 3;
  const uint64_t TAU = vole_params->TAU;
  const uint64_t KAPPA = vole_params->KAPPA;
  const uint64_t LAMBDA_BYTES = vole_params->lambda_bytes;
  const uint64_t nleaves = UINT64_C(1) << KAPPA;
  CASSERT(L % 8 == 0, "currently, we support only L multiple of 8");
  // tmp space of 3*LAMBDA_BYTES + 2*TAU*8 + Lbytes + TAU*lByte
  MSTACK_DEF(1024);
  seed_t* seed_commit = MSTACK_ALLOC(3 * LAMBDA_BYTES);
  bitmat_t** ss = (bitvec_t**)MSTACK_ALLOC(TAU * sizeof(bitvec_t*));
  bitmat_t** qq = (bitmat_t**)MSTACK_ALLOC(TAU * sizeof(bitmat_t*));
  bitvec_t* tmp_r = MSTACK_ALLOC(1 * Lbytes);
  for (uint64_t i = 0; i < TAU; ++i) {
    qq[i] = ((uint8_t*)q) + i * KAPPA * Lbytes;
    ss[i] = ((uint8_t*)coor_terms) + i * Lbytes;
  }
  memset(q, 0, TAU * KAPPA * Lbytes);
  uint64_t leaf_idx = 0;
  const seed_t* out_seed;
  const commit_t* out_commit;
  for (uint64_t l = 0; l < nleaves; ++l) {
    uint64_t loc = (l + 1 < nleaves) ? binval_of(l + 1) : KAPPA - 1;
    uint64_t loc_offset = loc * Lbytes;
    for (uint64_t i = 0; i < TAU; ++i, ++leaf_idx) {
      ggm_multi_sibling_tree_get_leaf_seed_commit(stree, leaf_idx, &out_seed, &out_commit, seed_commit);
      if (out_seed) {
        // normal leaf
        vole_params->xof.xof_seed(commits, out_commit, 2 * LAMBDA_BYTES);
        vole_params->vole_rng(tmp_r, Lbytes, out_seed);
        vole_params->bitvec_xor_to(Lbytes, ss[i], tmp_r);
        vole_params->bitvec_xor_to(Lbytes, qq[i] + loc_offset, ss[i]);
      } else {
        // hidden leaf
        vole_params->xof.xof_seed(commits, out_commit, 2 * LAMBDA_BYTES);
        vole_params->bitvec_xor_to(Lbytes, qq[i] + loc_offset, ss[i]);
      }
    }
  }
  // apply the corrective terms and the delta terms
  const uint64_t* deltap64 = delta1;
  for (uint64_t i = 0; i < TAU; ++i) {
    uint32_t delta_i = extract_kappabit_uint(KAPPA, i * KAPPA, deltap64);
    for (uint64_t j = 0; j < KAPPA; ++j) {
      if ((delta_i >> j) & 1) {
        vole_params->bitvec_xor_to(Lbytes, qq[i] + j * Lbytes, ss[i]);
      }
    }
  }
}

EXPORT void prover_midsize_to_fullsize_std_vole_ct_ref(const vole_parameters* vole_params, uint64_t L, bitvec_t* out_u,
                                                       bitmat_t* out_corr, flambda_t* out_v, const bitmat_t* in_u,
                                                       const bitmat_t* in_v) {
  // in_v points to a lambda x L_byte region, and the last lambda - tau.kappa rows are set to zero

  const uint64_t Lbytes = (L + 7) / 8;

  // out_v = transpose(in_v)
  (*vole_params->matrix_lambda_transpose)(out_v, in_v, L);

  // out_u = first L bits of in_u
  memcpy(out_u, in_u, Lbytes);

  // out_corr = in_u[i+1] ^ in_u[0]
  for (uint64_t i = 0; i < vole_params->TAU - 1; i++) {
    vole_params->bitvec_xor(Lbytes, out_corr + i * Lbytes, in_u + (i + 1) * Lbytes, in_u);
  }
}

EXPORT void verifier_midsize_to_fullsize_std_vole_ref(const vole_parameters* vole_params, uint64_t L, flambda_t* out_q,
                                                      const bitmat_t* in_q) {
  // out_q = transpose(in_q)
  (*vole_params->matrix_lambda_transpose)(out_q, in_q, L);
}

EXPORT uint64_t vole_consistency_check_matrix_nrows(  //
    const vole_parameters* vole_params)  // dimensions                                      // number of pairs to treat
{
  return (vole_params->KAPPA * vole_params->TAU + 16 + 7) & UINT64_C(-8);
}

EXPORT uint64_t vole_consistency_check_matrix_ncols(  //
    const vole_parameters* vole_params,               // dimensions
    uint64_t L) {
  return L - vole_consistency_check_matrix_nrows(vole_params);
}

EXPORT void both_vole_consistency_check_matrix(         //
    const vole_parameters* vole_params,                 // dimensions
    uint64_t L,                                         // number of pairs to treat
    bitmat_t* chk_matrix,                               // [out] cst_check_dim x (L - cst_check_dim)
    const hash_t* chk_seed1, uint64_t chk_seed1_bytes)  // [in] rng seed and tweak
{
  const uint64_t cchk_nrows = vole_consistency_check_matrix_nrows(vole_params);
  const uint64_t cchk_ncols = vole_consistency_check_matrix_ncols(vole_params, L);
  const uint64_t cchk_col_bytes = (cchk_ncols + 7) >> 3;
  xof_ctx cchk_matrix_rng;
  vole_params->xof.xof_init_and_seed(&cchk_matrix_rng, chk_seed1, chk_seed1_bytes);
  vole_params->xof.xof_finalize_and_output(&cchk_matrix_rng, chk_matrix, cchk_nrows * cchk_col_bytes);
  // mask last columns with zeroes if needed
  uint8_t cchk_last_mask = 0xFF >> ((-cchk_ncols) & 7);
  uint8_t* const cm = (uint8_t*)chk_matrix;
  for (uint64_t i = 0; i < cchk_nrows; i++) {
    cm[i * cchk_col_bytes + cchk_col_bytes - 1] &= cchk_last_mask;
  }
}

EXPORT void prover_vole_consistency_check(  //
    const vole_parameters* vole_params,     // dimensions
    uint64_t L,                             // number of pairs to treat
    bitvec_t* chk_u,                        // [out] cst check of size KAPPA.TAU+B
    flambda_t* chk_v,                       // [out] chk_v
    const bitvec_t* u,                      // [in] vector of length L
    const flambda_t* v,                     // [in] vector of length L
    const bitvec_t* chk_matrix)             // [in] cst check matrix
{
  const uint64_t lambda = vole_params->LAMBDA;
  const uint64_t lambda_bytes = vole_params->lambda_bytes;
  const uint64_t cchk_nrows = vole_consistency_check_matrix_nrows(vole_params);
  const uint64_t cchk_ncols = vole_consistency_check_matrix_ncols(vole_params, L);
  const uint64_t cchk_nrows_bytes = cchk_nrows >> 3;
  const uint8_t* const uu = (const uint8_t*)u;
  const uint8_t* const vv = (const uint8_t*)v;
  vole_params->matrix_vector_product_f2(cchk_nrows, cchk_ncols, chk_u, chk_matrix, uu + cchk_nrows_bytes);
  vole_params->bitvec_xor_to(cchk_nrows_bytes, chk_u, uu);
  vole_params->matrix_f2_times_vector_flambda(lambda, cchk_nrows, cchk_ncols, chk_v, chk_matrix,
                                              vv + cchk_nrows * lambda_bytes);
  vole_params->bitvec_xor_to(cchk_nrows * lambda_bytes, chk_v, vv);
}

EXPORT void verifier_vole_consistency_check(  //
    const vole_parameters* vole_params,       // dimensions
    uint64_t L,                               // number of pairs to treat
    flambda_t* chk_v,                         // [out] chk_v
    const bitvec_t* chk_u,                    // [in] cst check of size KAPPA.TAU+B
    const flambda_t* q,                       // [in] vector of length L
    const bitvec_t* chk_matrix,               // [in] cst check matrix (L-cst_check_dim) x L
    const bitvec_t* delta1)                   // [in] delta1 (kappa.tau bits)
{
  const uint64_t lambda = vole_params->LAMBDA;
  const uint64_t lambda_bytes = vole_params->lambda_bytes;
  const uint64_t cchk_nrows = vole_consistency_check_matrix_nrows(vole_params);
  const uint64_t cchk_ncols = vole_consistency_check_matrix_ncols(vole_params, L);
  const uint8_t* const qq = (const uint8_t*)q;
  vole_params->matrix_f2_times_vector_flambda(lambda, cchk_nrows, cchk_ncols, chk_v, chk_matrix,
                                              qq + cchk_nrows * lambda_bytes);
  vole_params->bitvec_xor_to(cchk_nrows * lambda_bytes, chk_v, qq);
  const uint8_t* const cu = (uint8_t*)chk_u;
  uint8_t* const cv = (uint8_t*)chk_v;
  for (uint64_t i = 0; i < cchk_nrows; ++i) {
    if ((cu[i >> 3] >> (i & 7)) & 1) {
      vole_params->flambda_sum(cv + i * lambda_bytes, cv + i * lambda_bytes, delta1);
    }
  }
}
