#ifndef SDITHER_RSD_H
#define SDITHER_RSD_H

#include "vole_parameters.h"

/// keygen stuff

/** generate a random rsd instance out of the master key entropy */
EXPORT void rsd_generate_random_instance_expanded_ref(   //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    uint64_t lambda, const rng_functions* prg,           // prng params
    bitmat_t* h, uint64_t h_slice_bytes,                 // out: (rsd_n - rsd_codim) x rsd_n matrix
    bitvec_t* y, uint64_t y_bytes,                       // out: syndrome (ceil(rsd_codim/8) bytes are set)
    uint32_t* solution,                                  // out: rsd_w integers in [0,n/w-1]
    const seed_t* sk_seed,                               // in: secret key seed (lambda bits)
    const seed_t* pk_seed                                // in: pubkey seed (lambda bits)
);

/** expand the public key from the pubkey seed */
EXPORT void rsd_expand_public_key_ref(                   //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    uint64_t lambda, const rng_functions* prg,           // prg family
    bitmat_t* h, uint64_t h_slice_bytes,                 // out: (rsd_n - rsd_codim) x rsd_n matrix
    const seed_t* h_seed                                 // in: pubkey seed (lambda bits)
);

/** unary encoding of the solution (for mux circuits) */
EXPORT void rsd_encode_solution_ref(                              //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,           // rsd dimensions
    uint64_t mux_depth, const uint64_t* mux_arities,              // mux depth and arities
    bitvec_t* encoded_solution, uint64_t encoded_solution_bytes,  // encoded solution
    const uint32_t* solution                                      // in: solution coefficients
);

EXPORT uint64_t rsd_public_key_times_challenge_tmp_bytes(  //
    const vole_parameters* vole_params,                    //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim);

/** multiply the public key with challenge points */
EXPORT void rsd_public_key_times_challenge_ref(          //
    const vole_parameters* vole_params,                  //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    flambda_t* chall_a_H,                                // out: n elements
    flambda_t* chall_a_y,                                // out: 1 element
    const flambda_t* chall_a,                            // in: rsd_codim_limbs elements
    const seed_t* pk_seed,                               // in: pubkey seed (lambda bits)
    const bitvec_t* y,                                   // in: pubkey y (rsd_codim bits padded with zeros)
    uint8_t* tmp_space                                   // scratch space
);

EXPORT void rsd_expanded_public_key_times_challenge_ref(
    const vole_parameters* vole_params,                  //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    flambda_t* chall_a_H,                                // out: n elements
    flambda_t* chall_a_y,                                // out: 1 element
    const flambda_t* chall_a,                            // in: rsd_codim_limbs elements
    const bitmat_t* h, uint64_t h_slice_bytes,           // in: pubkey H
    const bitvec_t* y, uint64_t y_slice_bytes            // in: pubkey y (rsd_codim bits padded with zeros)
);

#endif  // SDITHER_RSD_H
