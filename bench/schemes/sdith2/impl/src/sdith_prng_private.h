#ifndef RNG_PRIVATE_H
#define RNG_PRIVATE_H

#include "sdith_prng.h"

// union that represents an aes128 little endian counter
typedef union {
  __uint128_t u128;
  uint64_t v64[2];
} aesblk;

#endif  // RNG_PRIVATE_H
