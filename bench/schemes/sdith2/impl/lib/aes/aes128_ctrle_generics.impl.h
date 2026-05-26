#include <string.h>

#include "aes128_ctrle.h"

#ifndef aes128_ctrle_init_key_iv_T
#error must define the function names
#endif

// PUBLIC API

EXPORT void aes128_ctrle_init_key_iv_T(void* aes128_ctrle, const void* aes128key, const void* aes128iv) {
  struct aes128_ctrle_state* state = (struct aes128_ctrle_state*)aes128_ctrle;
  aes128_ctrle_set_key_T(state->round_keys, aes128key);
  memcpy(state->init_ctr, aes128iv, 16);
  memcpy(state->next_ctr, aes128iv, 16);
  memset(state->current_buf, 0, 16);
  state->buf_rem_bytes = 0;
}

EXPORT void aes128_ctrle_init_key_ivzero_T(void* aes128_ctrle, const void* aes128key) {
  static const __uint128_t ZERO = 0;
  aes128_ctrle_init_key_iv_T(aes128_ctrle, aes128key, &ZERO);
}

EXPORT void aes128_ctrle_seek_T(void* aes128_ctrle, uint64_t byte_position) {
  struct aes128_ctrle_state* state = (struct aes128_ctrle_state*)aes128_ctrle;
  //
  const uint64_t remain_bytes = byte_position & 15;
  const uint64_t nblocks = byte_position >> 4;
  if (remain_bytes == 0) {
    // encrypt an exact number of blocks
    ctr128_t ctr;
    memcpy(ctr.v8, state->init_ctr, 16);
    ctr.u128 += nblocks;
    memcpy(state->next_ctr, ctr.v8, 16);
    state->buf_rem_bytes = 0;
    return;
  } else {
    ctr128_t ctr;
    memcpy(ctr.v8, state->init_ctr, 16);
    ctr.u128 += nblocks;
    aes128_ctrle_encrypt_nblocks_T(NULL, state->current_buf, ctr.v8, state->round_keys, 1);
    memcpy(state->next_ctr, ctr.v8, 16);
    state->buf_rem_bytes = 16 - remain_bytes;
    return;
  }
}

EXPORT void aes128_ctrle_get_bytes_T(void* aes128_ctrle, void* out, uint64_t nbytes) {
  struct aes128_ctrle_state* state = (struct aes128_ctrle_state*)aes128_ctrle;
  uint8_t* oo = (uint8_t*)out;
  // output the remaining bytes in the buffer
  if (nbytes <= state->buf_rem_bytes) {
    memcpy(out, state->current_buf + 16 - state->buf_rem_bytes, nbytes);
    state->buf_rem_bytes -= nbytes;
    return;
  } else {
    memcpy(out, state->current_buf + 16 - state->buf_rem_bytes, state->buf_rem_bytes);
    nbytes -= state->buf_rem_bytes;
    oo += state->buf_rem_bytes;
  }
  //
  const uint64_t remain_bytes = nbytes & 15;
  const uint64_t nblocks = nbytes >> 4;
  if (remain_bytes == 0) {
    // encrypt an exact number of blocks
    aes128_ctrle_encrypt_nblocks_T(oo, oo + nbytes - 16, state->next_ctr, state->round_keys, nblocks);
    state->buf_rem_bytes = 0;
    return;
  } else {
    aes128_ctrle_encrypt_nblocks_T(oo, state->current_buf, state->next_ctr, state->round_keys, nblocks + 1);
    oo += nblocks << 4;
    memcpy(oo, state->current_buf, remain_bytes);
    state->buf_rem_bytes = 16 - remain_bytes;
    return;
  }
}
