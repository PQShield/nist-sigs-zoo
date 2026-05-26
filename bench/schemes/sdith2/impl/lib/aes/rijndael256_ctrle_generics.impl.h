#include <string.h>

#include "rijndael256_ctrle.h"

#ifndef rijndael256_ctrle_init_key_iv_T
#error must define the function names
#endif

// PUBLIC API

EXPORT void rijndael256_ctrle_init_key_iv_T(void* rijndael256_ctrle, const void* rijndael256key,
                                            const void* rijndael256iv) {
  struct rijndael256_ctrle_state* state = (struct rijndael256_ctrle_state*)rijndael256_ctrle;
  rijndael256_ctrle_set_key_T(state->round_keys, rijndael256key);
  memcpy(state->init_ctr, rijndael256iv, 32);
  memcpy(state->next_ctr, rijndael256iv, 32);
  memset(state->current_buf, 0, 32);
  state->buf_rem_bytes = 0;
}

EXPORT void rijndael256_ctrle_init_key_ivzero_T(void* rijndael256_ctrle, const void* rijndael256key) {
  static const uint8_t ZERO[32] = {};
  rijndael256_ctrle_init_key_iv_T(rijndael256_ctrle, rijndael256key, &ZERO);
}

EXPORT void rijndael256_ctrle_seek_T(void* rijndael256_ctrle, uint64_t byte_position) {
  struct rijndael256_ctrle_state* state = (struct rijndael256_ctrle_state*)rijndael256_ctrle;
  //
  const uint64_t remain_bytes = byte_position & 31;
  const uint64_t nblocks = byte_position >> 5;
  if (remain_bytes == 0) {
    // encrypt an exact number of blocks
    ctr256_t ctr;
    memcpy(ctr.v8, state->init_ctr, 32);
    ctr.v64[0] += nblocks;
    if (ctr.v64[0] < nblocks) {
      ctr.v64[1]++;
      if (ctr.v64[1] == 0) {
        ctr.v64[2]++;
        if (ctr.v64[2] == 0) {
          ctr.v64[3]++;
        }
      }
    }
    memcpy(state->next_ctr, ctr.v8, 32);
    state->buf_rem_bytes = 0;
    return;
  } else {
    ctr256_t ctr;
    memcpy(ctr.v8, state->init_ctr, 32);
    ctr.v64[0] += nblocks;
    if (ctr.v64[0] < nblocks) {
      ctr.v64[1]++;
      if (ctr.v64[1] == 0) {
        ctr.v64[2]++;
        if (ctr.v64[2] == 0) {
          ctr.v64[3]++;
        }
      }
    }
    rijndael256_ctrle_encrypt_nblocks_T(NULL, state->current_buf, ctr.v8, state->round_keys, 1);
    memcpy(state->next_ctr, ctr.v8, 32);
    state->buf_rem_bytes = 32 - remain_bytes;
    return;
  }
}

EXPORT void rijndael256_ctrle_get_bytes_T(void* rijndael256_ctrle, void* out, uint64_t nbytes) {
  struct rijndael256_ctrle_state* state = (struct rijndael256_ctrle_state*)rijndael256_ctrle;
  uint8_t* oo = (uint8_t*)out;
  // output the remaining bytes in the buffer
  if (nbytes <= state->buf_rem_bytes) {
    memcpy(out, state->current_buf + 32 - state->buf_rem_bytes, nbytes);
    state->buf_rem_bytes -= nbytes;
    return;
  } else {
    memcpy(out, state->current_buf + 32 - state->buf_rem_bytes, state->buf_rem_bytes);
    nbytes -= state->buf_rem_bytes;
    oo += state->buf_rem_bytes;
  }
  //
  const uint64_t remain_bytes = nbytes & 31;
  const uint64_t nblocks = nbytes >> 5;
  if (remain_bytes == 0) {
    // encrypt an exact number of blocks
    rijndael256_ctrle_encrypt_nblocks_T(oo, oo + nbytes - 32, state->next_ctr, state->round_keys, nblocks);
    state->buf_rem_bytes = 0;
    return;
  } else {
    rijndael256_ctrle_encrypt_nblocks_T(oo, state->current_buf, state->next_ctr, state->round_keys, nblocks + 1);
    oo += nblocks << 5;
    memcpy(oo, state->current_buf, remain_bytes);
    state->buf_rem_bytes = 32 - remain_bytes;
    return;
  }
}
