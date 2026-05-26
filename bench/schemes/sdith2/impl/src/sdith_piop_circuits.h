#ifndef SDITH_PIOP_CIRCUITS_H
#define SDITH_PIOP_CIRCUITS_H

#include "vole_parameters.h"

// vole circuit gates

/**
 * prover packed secret input gate (degree 1)
 */
EXPORT void prover_cst_vole_packed_secret_input_ct_ref(  //
    const vole_parameters* vole_params,                  // dimensions
    uint64_t num_inputs,                                 // number of inputs to process
    bitvec_t* out_pub,                                   // [out] output publication (packed over f2)
    fpoly_t* out_f,                                      // [out] poly of degree 1 (cst term over f2)
    const bitvec_t* in_value,                            // [in] secret bit values (packed over f2)
    const bitvec_t* in_rvp_u,                            // [in] packed u terms
    const flambda_t* in_rvp_v);                          // [in] packed v terms

/**
 * verifier packed secret input gate (degree 1)
 */
EXPORT void verifier_cst_vole_packed_secret_input_ref(  //
    const vole_parameters* vole_params,                 // dimensions
    uint64_t num_inputs,                                // number of inputs to process
    fpoly_t* out_q,                                     // [out] vole evaluations
    const bitvec_t* in_pub,                             // [in] publications (packed over f2)
    const flambda_t* in_rvp_q,                          // [in] packed v terms
    const flambda_t* delta2);                           // [in] verifier's point

/**
 * prover check-zero gate (degree d)
 */
EXPORT void prover_cst_vole_check_zero_gate_ct_ref(  //
    const vole_parameters* vole_params,              // dimensions
    uint64_t degree,                                 // input degree
    fpoly_t* out_pub,                                // [out] output publication (degree d-1)
    const fpoly_t* in_f,                             // [in] input (degree d)
    const fpoly_t* in_rvp_f);                        // [in] rvp (degree d-1)

/**
 * verifier check-zero gate (degree d)
 */
EXPORT uint8_t verifier_cst_vole_check_zero_gate_ref(  //
    const vole_parameters* vole_params,                // dimensions
    uint64_t degree,                                   // input degree
    flambda_t* out_value,                              // [out] deduced output value
    const fpoly_t* in_pub,                             // [in] publication (degree d-1)
    const flambda_t* in_q,                             // [in] input (degree d)
    const flambda_t* in_rvp_q,                         // [in] rvp (degree d-1)
    const flambda_t* delta2);                          // [in] verifier's point

/**
 * prover xor gate
 */
EXPORT void prover_cst_vole_xor_gate_ct_ref(  //
    const vole_parameters* vole_params,       // dimensions
    fpoly_t* res_f,                           // [out] poly of degree 1 (cst term over f2)
    const fpoly_t* a_f, uint64_t a_degree,    // [in] a
    const fpoly_t* b_f, uint64_t b_degree);   // [in] b

/**
 * verifier xor gate
 */
EXPORT void verifier_cst_vole_xor_gate_ref(   //
    const vole_parameters* vole_params,       // dimensions
    flambda_t* res_f,                         // [out] poly of degree 1 (cst term over f2)
    const flambda_t* a_f, uint64_t a_degree,  // [in] a
    const flambda_t* b_f, uint64_t b_degree,  // [in] b
    const flambda_t* delta2);                 // [in] verifier's point (unused)

/**
 * prover mul gate
 */
EXPORT void prover_cst_vole_mul_gate_ct_ref(  //
    const vole_parameters* vole_params,       // dimensions
    fpoly_t* res_f,                           // [out] poly of degree 1 (cst term over f2)
    const fpoly_t* a_f, uint64_t a_degree,    // [in] a
    const fpoly_t* b_f, uint64_t b_degree);   // [in] b

/**
 * verifier mul gate
 */
EXPORT void verifier_cst_vole_mul_gate_ref(   //
    const vole_parameters* vole_params,       // dimensions
    flambda_t* res_q,                         // [out] poly of degree 1 (cst term over f2)
    const flambda_t* a_q, uint64_t a_degree,  // [in] a
    const flambda_t* b_q, uint64_t b_degree,  // [in] b
    const flambda_t* delta2);                 // [in] verifier's point (unused)

/**
 * prover echelon-pow2 gate (degree 1): sum(2^{ki}.a[i])
 */
EXPORT void prover_cst_vole_echelon_pow2_ct_ref(  //
    const vole_parameters* vole_params,           // dimensions
    uint64_t arity, uint64_t k,                   //
    fpoly_t* res_f,                               // [out] res of degree 1
    const fpoly_t* a_f);                          // [in] (arity) a of degree 1

/**
 * verifier echelon-pow2 gate (degree 1): sum(2^{ki}.a[i])
 */
EXPORT void verifier_cst_vole_echelon_pow2_ref(  //
    const vole_parameters* vole_params,          // dimensions
    uint64_t arity, uint64_t k,                  //
    flambda_t* res_q,                            // [out] res
    const flambda_t* a_q,                        // [in] (arity) a
    const flambda_t* delta2);                    // [in] verifier's point (unused)

/** @brief number of scratch bytes necessary for prover_cst_vole_qary_mux_gate_ct_ref */
EXPORT uint64_t prover_cst_vole_qary_mux_gate_ct_ref_tmp_bytes(  //
    const vole_parameters* vole_params,                          // dimensions
    uint64_t arity,                                              // mux arity >= 1
    uint64_t in_degree                                           // common input degree
);

/**
 * prover qary-mux gate: res = a[0] + sum(c[i-1] (a[i]-a[0]))
 */
EXPORT void prover_cst_vole_qary_mux_gate_ct_ref(  //
    const vole_parameters* vole_params,            // dimensions
    uint64_t arity,                                // mux arity >= 1
    uint64_t in_degree,                            // common input degree
    fpoly_t* res_f,                                // [out] poly of degree d+1
    const fpoly_t* c_f,                            // [in] (arity-1) control bits of deg. 1
    const fpoly_t* a_f,                            // [in] (arity) values of deg d
    uint8_t* tmp_space);                           // scratch space

/**
 * prover qary-mux gate: res = a[0] + sum(c[i-1] (a[i]-a[0])) using product_f2
 */
EXPORT void prover_cst_vole_qary_mux_gate_ct_f2_ref(  //
    const vole_parameters* vole_params,               // dimensions
    uint64_t arity,                                   // mux arity >= 1
    uint64_t in_degree,                               // common input degree
    fpoly_t* res_f,                                   // [out] poly of degree d+1
    const fpoly_t* c_f,                               // [in] (arity-1) control bits of deg. 1
    const fpoly_t* a_f);                              // [in] (arity) values of deg d

/** @brief number of scratch bytes necessary*/
EXPORT uint64_t verifier_cst_vole_qary_mux_gate_ref_tmp_bytes(  //
    const vole_parameters* vole_params,                         // dimensions
    uint64_t arity,                                             // mux arity >= 1
    uint64_t in_degree                                          // common input degree
);

/**
 * prover qary-mux gate: res = a[0] + sum(c[i-1] (a[i]-a[0]))
 */
EXPORT void verifier_cst_vole_qary_mux_gate_ref(  //
    const vole_parameters* vole_params,           // dimensions
    uint64_t arity,                               // mux arity >= 1
    uint64_t in_degree,                           // common input degree
    flambda_t* res_q,                             // [out] poly of degree d+1
    const flambda_t* c_q,                         // [in] (arity-1) control bits of deg. 1
    const flambda_t* a_q,                         // [in] (arity) values of deg d
    const flambda_t* delta2,                      // [in] verifier's point (unused)
    uint8_t* tmp_space);                          // scratch space

/**
 * prover qary-mux gate: res = a[0] + sum(c[i-1] (a[i]-a[0])) using product_f2
 */
EXPORT void verifier_cst_vole_qary_mux_gate_f2_ref(  //
    const vole_parameters* vole_params,              // dimensions
    uint64_t arity,                                  // mux arity >= 1
    uint64_t in_degree,                              // common input degree
    flambda_t* res_q,                                // [out] poly of degree d+1
    const flambda_t* c_q,                            // [in] (arity-1) control bits of deg. 1
    const flambda_t* a_q,                            // [in] (arity) values of deg d
    const flambda_t* delta2,                         // [in] verifier's point (unused)
    uint8_t* tmp_space);                             // scratch space

/**
 * scratch bytes required by prover_cst_check_unitary_gate_ct_ref
 */
EXPORT uint64_t prover_cst_check_unitary_gate_ct_ref_tmp_bytes(const vole_parameters* vole_params);

/**
 * prover check unitary gate:
 * res = coeff * (sum a[i].2^{i}) * (sum a[i].2^{ki}) + (sum a[i].2^{(k+1)i}) where k = arity
 */
EXPORT void prover_cst_check_unitary_gate_ct_ref(  //
    const vole_parameters* vole_params,            // dimensions
    uint64_t arity,                                // arity >= 1
    fpoly_t* res_f,                                // [out] res of degree 2
    const fpoly_t* a_f,                            // [in] (arity) a of deg 1
    const flambda_t* coeff,                        // [in] challenge coeff
    uint8_t* tmp_space);                           // scratch space

/**
 * scratch bytes required by verifier_cst_check_unitary_gate_ref
 */
EXPORT uint64_t verifier_cst_check_unitary_gate_ref_tmp_bytes(const vole_parameters* vole_params);

/**
 * verifier check unitary gate:
 * res = coeff * (sum a[i].2^{i}) * (sum a[i].2^{ki}) + (sum a[i].2^{(k+1)i}) where k = arity
 */
EXPORT void verifier_cst_check_unitary_gate_ref(  //
    const vole_parameters* vole_params,           // dimensions
    uint64_t arity,                               // mux arity >= 1
    flambda_t* res_q,                             // [out] res
    const flambda_t* a_q,                         // [in] (arity) a
    const flambda_t* coeff,                       // [in] challenge coeff
    const flambda_t* delta2,                      // [in] verifier's point (unused)
    uint8_t* tmp_space);                          // scratch space

/**
 * scratch space required by prover_cst_mux_circuit_ct_ref
 */
EXPORT uint64_t prover_cst_mux_circuit_ct_ref_tmp_bytes(  //
    const vole_parameters* vole_params,                   // dimensions
    uint64_t depth,                                       // circuit depth
    const uint64_t* arities,                              // (depth) arities (preferably in decreasing order)
    uint64_t num_inputs);                                 // scratch space

/**
 * prover mux-circuit:
 */
EXPORT void prover_cst_mux_circuit_ct_ref(  //
    const vole_parameters* vole_params,     // dimensions
    uint64_t depth,                         // circuit depth
    const uint64_t* arities,                // (depth) arities (preferably in decreasing order)
    uint64_t num_inputs,                    // number of inputs (<= product of arities)
    fpoly_t* res_f,                         // [out] res of degree depth
    const fpoly_t* c_f,                     // [in] (sum (arity[i]-1)) ctrl bits of deg 1
    const flambda_t* a,                     // [in] challenge input values
    const flambda_t* chall_c,               // [in] challenge coeff for unitary tests (unused if all muxes are binary)
    uint8_t* tmp_space);                    // [tmp] scratch space

/**
 * scratch space required by verifier_cst_mux_circuit_ref
 */
EXPORT uint64_t verifier_cst_mux_circuit_ref_tmp_bytes(  //
    const vole_parameters* vole_params,                  // dimensions
    uint64_t depth,                                      // circuit depth
    const uint64_t* arities,                             // (depth) arities (preferably in decreasing order)
    uint64_t num_inputs);                                // scratch space

/**
 * verifier mux-circuit:
 */
EXPORT void verifier_cst_mux_circuit_ref(  //
    const vole_parameters* vole_params,    // dimensions
    uint64_t depth,                        // circuit depth
    const uint64_t* arities,               // (depth) arities (preferably in decreasing order)
    uint64_t num_inputs,                   // number of inputs (<= product of arities)
    flambda_t* res_q,                      // [out] res of degree depth
    const flambda_t* c_q,                  // [in] (sum (arity[i]-1)) ctrl bits of deg 1
    const flambda_t* a,                    // [in] challenge input values
    const flambda_t* chall_c,              // [in] challenge coeff for unitary tests (unused if all muxes are binary)
    const flambda_t* delta2,               // [in] verifier's point (unused)
    uint8_t* tmp_space);                   // [tmp] scratch space

#endif  // SDITH_PIOP_CIRCUITS_H
