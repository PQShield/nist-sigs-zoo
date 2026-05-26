#include <inttypes.h>
#include <string.h>

#include "sdith_prng.h"
#include "sdith_rsd.h"
#include "vole_private.h"

/** generate a random rsd instance out of the master key entropy */
EXPORT void rsd_generate_random_instance_expanded_ref(   //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    uint64_t lambda, const rng_functions* prg,           // prng params
    bitmat_t* h, uint64_t h_slice_bytes,                 // out: (rsd_n - rsd_codim) x rsd_n matrix
    bitvec_t* y, uint64_t y_bytes,                       // out: syndrome (ceil(rsd_codim/8) bytes are set)
    uint32_t* solution,                                  // out: rsd_w integers in [0,n/w-1]
    const seed_t* sk_seed,                               // in: secret key seed (lambda bits)
    const seed_t* pk_seed                                // in: pubkey seed (lambda bits)
) {
  const uint64_t col_bytes = (rsd_codim + 7) >> 3;
  CREQUIRE(rsd_n != 0, "empty rsd_n (%" PRId64 ")", rsd_n);
  CREQUIRE(rsd_w != 0, "empty rsd_w (%" PRId64 ")", rsd_w);
  CREQUIRE(rsd_codim != 0, "empty rsd_codim (%" PRId64 ")", rsd_codim);
  CREQUIRE(h_slice_bytes >= col_bytes, "h_slice_bytes too small (%" PRId64 " vs %" PRId64 ")", h_slice_bytes,
           col_bytes);
  CREQUIRE(y_bytes >= col_bytes, "y_bytes too small (%" PRId64 " vs %" PRId64 ")", y_bytes, col_bytes);
  CREQUIRE(rsd_n % rsd_w == 0, "rsd_n (%" PRId64 ") is not a multiple of rsd_w (%" PRId64 ")", rsd_n, rsd_w);
  rng_ctx sk_rng;
  prg->rng_init_and_seed_ivzero(&sk_rng, sk_seed);
  // generate the solution vector
  const uint32_t npw = rsd_n / rsd_w;
  // hit and miss version (non-ct but it is data independent)
  const uint32_t npw_max = UINT32_MAX - (UINT32_MAX % npw);
  for (uint64_t i = 0; i < rsd_w; i++) {
    uint32_t pos;
    do {
      prg->rng_output(&sk_rng, &pos, 4);
    } while (pos >= npw_max);
    solution[i] = pos % npw;  // TODO make modulo ct
  }
  // expand h
  rng_ctx pk_rng;
  prg->rng_init_and_seed_ivzero(&pk_rng, pk_seed);
  rsd_expand_public_key_ref(rsd_w, rsd_n, rsd_codim,  //
                            lambda, prg,              //
                            h, h_slice_bytes,         //
                            pk_seed);
  // compute y (non-ct, data dependent)
  const uint8_t* const hh = (const uint8_t*)h;
  uint8_t* const yy = (uint8_t*)y;
  memset(y, 0, y_bytes);
  for (uint64_t i = 0; i < rsd_w; i++) {
    uint64_t real_index = i * npw + solution[i];
    if (real_index < rsd_codim) {
      // it falls on the "identity block" of H
      yy[real_index >> 3] ^= ((uint8_t)1 << (real_index & 7));
    } else {
      // it falls on the "random block" of H
      bitvec_xor_to_ref(col_bytes, y, hh + (real_index - rsd_codim) * h_slice_bytes);
    }
  }
}

/** expand the public key from the pubkey seed */
EXPORT void rsd_expand_public_key_ref(                   //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    uint64_t lambda, const rng_functions* prg,           // prg family
    bitmat_t* h, uint64_t h_slice_bytes,                 // out: (rsd_n - rsd_codim) x rsd_n matrix
    const seed_t* h_seed                                 // in: pubkey seed (lambda bits)
) {
  const uint64_t h_col_bytes = (rsd_codim + 7) >> 3;
  CREQUIRE(rsd_n >= rsd_codim, "rsd_n (%" PRId64 ") is not larger than codim (%" PRId64 ")", rsd_n, rsd_codim);
  CREQUIRE(rsd_w != 0, "empty rsd_w (%" PRId64 ")", rsd_w);
  CREQUIRE(rsd_codim != 0, "empty rsd_codim (%" PRId64 ")", rsd_codim);
  CREQUIRE(h_slice_bytes >= h_col_bytes,  //
           "h_slice_bytes too small (%" PRId64 " vs %" PRId64 ")", h_slice_bytes, h_col_bytes);
  const uint64_t n_minus_k = rsd_n - rsd_codim;
  const uint8_t h_last_mask = 0xFF >> (-rsd_codim & 7);
  memset(h, 0, n_minus_k * h_slice_bytes);
  rng_ctx h_rng;
  prg->rng_init_and_seed_ivzero(&h_rng, h_seed);
  uint8_t* const hh = (uint8_t*)h;
  for (uint64_t i = 0; i < n_minus_k; i++) {
    uint8_t* const hi = hh + h_slice_bytes * i;
    prg->rng_output(&h_rng, hi, h_col_bytes);
    hi[h_col_bytes - 1] &= h_last_mask;
  }
}

EXPORT uint64_t rsd_public_key_times_challenge_tmp_bytes(const vole_parameters* vole_params,  //
                                                         uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim) {
  CREQUIRE(rsd_n >= rsd_codim, "rsd_n (%" PRId64 ") is not larger than codim (%" PRId64 ")", rsd_n, rsd_codim);
  CREQUIRE(rsd_w != 0, "empty rsd_w (%" PRId64 ")", rsd_w);
  CREQUIRE(rsd_codim != 0, "empty rsd_codim (%" PRId64 ")", rsd_codim);
  const uint64_t lambda = vole_params->LAMBDA;
  const uint64_t lambda_bytes = vole_params->lambda_bytes;
  const uint64_t rsd_codim_limbs = (rsd_codim + lambda - 1) / lambda;
  COMP_SPACE_INIT();
  COMP_SPACE_MAP_ALIGNED(uint8_t*, y_full, 32, rsd_codim_limbs* lambda_bytes);
  COMP_SPACE_RETURN();
}

/** expand and multiply the public key with challenge points */
EXPORT void rsd_public_key_times_challenge_ref(          //
    const vole_parameters* vole_params,                  //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    flambda_t* chall_a_H,                                // out: n elements
    flambda_t* chall_a_y,                                // out: 1 element
    const flambda_t* chall_a,                            // in: rsd_codim_limbs elements
    const seed_t* pk_seed,                               // in: pubkey seed (lambda bits)
    const bitvec_t* y,                                   // in: pubkey y (rsd_codim bits padded with zeros)
    uint8_t* tmp_space                                   // scratch space
) {
  static const uint64_t TWO[] = {2, 0, 0, 0};
  CASSERT(rsd_n >= rsd_codim, "rsd_n (%" PRId64 ") is not larger than codim (%" PRId64 ")", rsd_n, rsd_codim);
  CASSERT(rsd_w != 0, "empty rsd_w (%" PRId64 ")", rsd_w);
  CASSERT(rsd_codim != 0, "empty rsd_codim (%" PRId64 ")", rsd_codim);
  const uint64_t lambda = vole_params->LAMBDA;
  const uint64_t lambda_bytes = vole_params->lambda_bytes;
  const uint64_t rsd_codim_limbs = (rsd_codim + lambda - 1) / lambda;
  const uint64_t rsd_codim_bytes = (rsd_codim + 7) >> 3;
  const uint64_t rsd_padding_bytes = rsd_codim_limbs * lambda_bytes - rsd_codim_bytes;
  const uint64_t n_minus_k = rsd_n - rsd_codim;
  const uint8_t h_last_mask = 0xFF >> ((-rsd_codim) & 7);
  TMP_SPACE_MAP_ALIGNED(uint8_t*, y_full, 32, rsd_codim_limbs* lambda_bytes);
  uint8_t* h_row = y_full;  // alias

#ifndef NDEBUG
  uint64_t leftover_bits = rsd_codim & 7;
  uint8_t* y_u8 = (uint8_t*)y;
  if (leftover_bits) {
    uint64_t mask = ((1 << (8 - leftover_bits)) - 1) << leftover_bits;
    uint64_t idx = ((rsd_codim + 7) >> 3) - 1;
    CASSERT((y_u8[idx] & mask) == 0, "empty bits of y are not zero-ed out");
  }
#endif  // NDEBUG

  // deal with y
  memcpy(y_full, y, rsd_codim_bytes);
  y_full[rsd_codim_bytes - 1] &= h_last_mask;
  memset(y_full + rsd_codim_bytes, 0, rsd_padding_bytes);
  vole_params->flambda_product(chall_a_y, y_full, chall_a);
  for (uint64_t i = 1; i < rsd_codim_limbs; i++) {
    vole_params->flambda_product(y_full + i * lambda_bytes, y_full + i * lambda_bytes, chall_a + i * lambda_bytes);
    vole_params->flambda_sum(chall_a_y, chall_a_y, y_full + i * lambda_bytes);
  }
  // deal with the id block of H
  uint8_t* dest = chall_a_H;
  {
    uint64_t s = rsd_codim;
    for (uint64_t i = 0; i < rsd_codim_limbs; i++) {
      uint64_t block_s = lambda < s ? lambda : s;
      vole_params->flambda_set(dest, chall_a + i * lambda_bytes);
      dest += lambda_bytes;
      for (uint64_t j = 1; j < block_s; j++) {
        vole_params->flambda_product(dest, dest - lambda_bytes, TWO);
        dest += lambda_bytes;
      }
      s -= block_s;
    }
    CASSERT(dest == chall_a_H + rsd_codim * lambda_bytes, "bug!");
    CASSERT(s == 0, "bug!");
  }
  // deal with the regular rows of H
  rng_ctx h_rng;
  vole_params->prg.rng_init_and_seed_ivzero(&h_rng, pk_seed);
  for (uint64_t i = 0; i < n_minus_k; i++) {
    // generate one row of H
    vole_params->prg.rng_output(&h_rng, h_row, rsd_codim_bytes);
    h_row[rsd_codim_bytes - 1] &= h_last_mask;
    memset(h_row + rsd_codim_bytes, 0, rsd_padding_bytes);
    // multiply by the challenge
    vole_params->flambda_product(dest, h_row, chall_a);
    for (uint64_t j = 1; j < rsd_codim_limbs; j++) {
      vole_params->flambda_product(h_row + j * lambda_bytes, h_row + j * lambda_bytes, chall_a + j * lambda_bytes);
      vole_params->flambda_sum(dest, dest, h_row + j * lambda_bytes);
    }
    dest += lambda_bytes;
  }
  CASSERT(dest == chall_a_H + rsd_n * lambda_bytes, "bug!");
}

/** expand and multiply the public key with challenge points */
EXPORT void rsd_expanded_public_key_times_challenge_ref(
    const vole_parameters* vole_params,                  //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,  // rsd dimensions
    flambda_t* chall_a_H,                                // out: n elements
    flambda_t* chall_a_y,                                // out: 1 element
    const flambda_t* chall_a,                            // in: rsd_codim_limbs elements
    const bitmat_t* h, uint64_t h_slice_bytes,           // in: pubkey H
    const bitvec_t* y, uint64_t y_slice_bytes            // in: pubkey y (rsd_codim bits padded with zeros)
) {
  static const uint64_t TWO[] = {2, 0, 0, 0};
  CASSERT(rsd_n >= rsd_codim, "rsd_n (%" PRId64 ") is not larger than codim (%" PRId64 ")", rsd_n, rsd_codim);
  CASSERT(rsd_w != 0, "empty rsd_w (%" PRId64 ")", rsd_w);
  CASSERT(rsd_codim != 0, "empty rsd_codim (%" PRId64 ")", rsd_codim);
  const uint64_t lambda = vole_params->LAMBDA;
  const uint64_t lambda_bytes = vole_params->lambda_bytes;
  const uint64_t rsd_codim_limbs = (rsd_codim + lambda - 1) / lambda;
  const uint64_t n_minus_k = rsd_n - rsd_codim;
  CREQUIRE(h_slice_bytes == rsd_codim_limbs * lambda_bytes, "wrong slice (%" PRId64 ")", h_slice_bytes);
  CREQUIRE(y_slice_bytes == rsd_codim_limbs * lambda_bytes, "wrong slice (%" PRId64 ")", y_slice_bytes);

#ifndef NDEBUG
  uint64_t idx = ((rsd_codim + 7) >> 3) - 1;
  uint64_t leftover_bits = rsd_codim & 7;
  uint8_t* y_u8 = (uint8_t*)y;
  if (leftover_bits) {
    uint64_t mask = ((1 << (8 - leftover_bits)) - 1) << leftover_bits;
    CASSERT((y_u8[idx] & mask) == 0, "empty bits of y are not zero-ed out");
  }

  uint8_t* h_u8 = (uint8_t*)h;
  for (uint64_t i = idx + 1; i < h_slice_bytes; i++) {
    CASSERT(h_u8[i] == 0,
            "empty bytes of h"
            " are not zero-ed out");
  }
  for (uint64_t i = idx + 1; i < y_slice_bytes; i++) {
    CASSERT(y_u8[i] == 0, "empty bytes of y are not zero-ed out");
  }
#endif  // NDEBUG

  flambda_max_t tmp1;

  // deal with y
  vole_params->flambda_product(chall_a_y, y, chall_a);
  for (uint64_t i = 1; i < rsd_codim_limbs; i++) {
    vole_params->flambda_product(tmp1, y + i * lambda_bytes, chall_a + i * lambda_bytes);
    vole_params->flambda_sum(chall_a_y, chall_a_y, tmp1);
  }
  // deal with the id block of H
  uint8_t* dest = chall_a_H;
  {
    uint64_t s = rsd_codim;
    for (uint64_t i = 0; i < rsd_codim_limbs; i++) {
      uint64_t block_s = lambda < s ? lambda : s;
      vole_params->flambda_set(dest, chall_a + i * lambda_bytes);
      dest += lambda_bytes;
      for (uint64_t j = 1; j < block_s; j++) {
        vole_params->flambda_product(dest, dest - lambda_bytes, TWO);
        dest += lambda_bytes;
      }
      s -= block_s;
    }
    CASSERT(dest == chall_a_H + rsd_codim * lambda_bytes, "bug!");
    CASSERT(s == 0, "bug!");
  }
  // deal with the regular rows of H
  for (uint64_t i = 0; i < n_minus_k; i++) {
    uint8_t* hi = ((uint8_t*)h) + i * h_slice_bytes;
    // multiply by the challenge
    vole_params->flambda_product(dest, hi, chall_a);
    for (uint64_t j = 1; j < rsd_codim_limbs; j++) {
      vole_params->flambda_product(tmp1, hi + j * lambda_bytes, chall_a + j * lambda_bytes);
      vole_params->flambda_sum(dest, dest, tmp1);
    }
    dest += lambda_bytes;
  }
  CASSERT(dest == chall_a_H + rsd_n * lambda_bytes, "bug!");
}

/** unary encoding of the solution (for mux circuits) */
EXPORT void rsd_encode_solution_ref(                              //
    uint64_t rsd_w, uint64_t rsd_n, uint64_t rsd_codim,           // rsd dimensions
    uint64_t mux_depth, const uint64_t* mux_arities,              // mux depth and arities
    bitvec_t* encoded_solution, uint64_t encoded_solution_bytes,  // encoded solution
    const uint32_t* solution                                      // in: solution coefficients
) {
  CREQUIRE(rsd_n != 0, "empty rsd_n (%" PRId64 ")", rsd_n);
  CREQUIRE(rsd_w != 0, "empty rsd_w (%" PRId64 ")", rsd_w);
  CREQUIRE(rsd_codim != 0, "empty rsd_codim (%" PRId64 ")", rsd_codim);
  CREQUIRE(mux_depth != 0, "empty mux_depth (%" PRId64 ")", mux_depth);
  CREQUIRE(rsd_n % rsd_w == 0, "rsd_n (%" PRId64 ") is not a multiple of rsd_w (%" PRId64 ")", rsd_n, rsd_w);
  const uint64_t npw = rsd_n / rsd_w;
  uint64_t mux_size = 1;
  uint64_t mux_inputs = 0;
  for (uint64_t j = 0; j < mux_depth; j++) {
    const uint64_t arj = mux_arities[j];
    CREQUIRE(arj >= 2, "mux_arity[%" PRId64 "]=%" PRId64 " too small", j, arj);
    mux_inputs += arj - 1;
    mux_size *= arj;
  }
  CREQUIRE(mux_size >= npw, "mux arities too small (max_tree_size: %" PRId64 " vs. %" PRId64 ")", mux_size, npw);
  uint64_t encoded_solution_actual_bytes = (mux_inputs * rsd_w + 7) >> 3;
  CREQUIRE(encoded_solution_bytes >= encoded_solution_actual_bytes,                      //
           "encoded_solutions too small (size: %" PRId64 " vs. min size: %" PRId64 ")",  //
           encoded_solution_bytes, encoded_solution_actual_bytes);
  uint8_t* const sol = encoded_solution;
  memset(sol, 0, encoded_solution_bytes);
  uint64_t bitpos = 0;
  for (uint64_t i = 0; i < rsd_w; i++) {
    uint64_t si = solution[i];
    for (uint64_t j = 0; j < mux_depth; j++) {
      const uint64_t arj = mux_arities[j];
      const uint64_t sij = si % arj;
      si /= arj;
      if (sij != 0) {
        uint64_t pos = bitpos + sij - 1;
        sol[pos / 8] |= (uint8_t)(1) << (pos % 8);  // TODO: make this ct
      }
      bitpos += arj - 1;
    }
    CASSERT(si == 0, "bug");
  }
  CASSERT(bitpos == rsd_w * mux_inputs, "bug");
}
