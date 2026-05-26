#ifndef VOLE_GENERATION_H
#define VOLE_GENERATION_H

/// This header declares the functions needed to generate vole pairs securely

#include "vole_parameters.h"

/** @brief converts delta0 to delta1
 * delta0 has kappa*tau bits, encoding tau grey code encodings
 * delta1 has lambda bits, only the lowest kappa*tau are used, the rest is zero.
 */
EXPORT void delta1_from_delta0_ref(const vole_parameters* vole_params, flambda_t* delta1, const bitvec_t* delta0);

/** @brief converts delta1 to delta2
 * delta2 is the field inverse of delta1
 */
EXPORT void delta2_from_delta1_ref(const vole_parameters* vole_params, flambda_t* delta2, const flambda_t* delta1);

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
    full_ggm_tree* tree);                                         // [in]  ggm tree

/**
 * Open L vole pairs over (F2,F2^KAPPA) (grey code version)
 */
EXPORT void verifier_open_midsize_grey_vole_from_seeds_ref(  //
    const vole_parameters* vole_params,                      // dimensions
    uint64_t L,                                              // number of pairs to generate
    xof_ctx* commits,                                        // [out] where to hash the leaves commit
    bitmat_t* out_q,                                         // [out] TAU x KAPPA x L row major
    bitmat_t* coor_terms,                                    // [in/out] TAU x L bits
    ggm_multi_sibling_tree* stree,                           // multi-sibling-tree
    const flambda_t* delta1);                                // verifier's point delta1

/**
 * Concatenate and transpose L mid-size std vole pairs over (F2,F2^KAPPA)
 * To form L full-size std vole pairs over (F2,F2^LAMBDA)
 * // acceptable in_place mode:
 * // either
 * //   out_u == in_u and out_corr == in_u[1]
 * // otherwise, out_u, out_corr, in_u shall be completely disjoint
 * // also: either
 * //   out_v == in_v
 * // or otherwise, out_v, in_v must be completely disjoint
 */
EXPORT void prover_midsize_to_fullsize_std_vole_ct_ref(  //
    const vole_parameters* vole_params,                  // dimensions
    uint64_t L,                                          // number of pairs to treat
    bitvec_t* out_u,                                     // [out] L bits
    bitmat_t* out_corr,                                  // [out] (TAU-1 x L) row major
    flambda_t* out_v,                                    // [out] gflambda[L] contiguous
    const bitmat_t* in_u,                                // [in] TAU x L row major
    const bitmat_t* in_v);                               // [in] (TAU*KAPPA) x L row major

/**
 * Concatenate and transpose L mid-size std vole pairs over (F2,F2^KAPPA)
 * To form L full-size std vole pairs over (F2,F2^LAMBDA)
 * // acceptable in_place mode:
 * // also: either
 * //   out_q == in_a
 * // or otherwise, out_q, in_q must be completely disjoint
 */
EXPORT void verifier_midsize_to_fullsize_std_vole_ref(  //
    const vole_parameters* vole_params,                 // dimensions
    uint64_t L,                                         // number of pairs to treat
    flambda_t* out_q,                                   // [out] gflambda[L]
    const bitmat_t* in_q);                              // [in] TAU*KAPPA x L row major

EXPORT uint64_t vole_consistency_check_matrix_nrows(  //
    const vole_parameters* vole_params);              // dimensions

EXPORT uint64_t vole_consistency_check_matrix_ncols(  //
    const vole_parameters* vole_params,               // dimensions
    uint64_t L);                                      // number of pairs to treat

EXPORT void both_vole_consistency_check_matrix(          //
    const vole_parameters* vole_params,                  // dimensions
    uint64_t L,                                          // number of pairs to treat
    bitmat_t* chk_matrix,                                // [out] cst_check_dim x (L - cst_check_dim)
    const hash_t* chk_seed1, uint64_t chk_seed1_bytes);  // [in] rng seed and tweak

EXPORT void prover_vole_consistency_check(  //
    const vole_parameters* vole_params,     // dimensions
    uint64_t L,                             // number of pairs to treat
    bitvec_t* chk_u,                        // [out] cst check of size KAPPA.TAU+B
    flambda_t* chk_v,                       // [out] chk_v
    const bitvec_t* u,                      // [in] vector of length L
    const flambda_t* v,                     // [in] vector of length L
    const bitvec_t* chk_matrix);            // [in] cst check matrix

EXPORT void verifier_vole_consistency_check(  //
    const vole_parameters* vole_params,       // dimensions
    uint64_t L,                               // number of pairs to treat
    flambda_t* chk_v,                         // [out] chk_v
    const bitvec_t* chk_u,                    // [in] cst check of size KAPPA.TAU+B
    const flambda_t* q,                       // [in] vector of length L
    const bitvec_t* chk_matrix,               // [in] cst check matrix (L-cst_check_dim) x L
    const bitvec_t* delta1);                  // [in] delta1 (kappa.tau bits)

#endif  // VOLE_GENERATION_H
