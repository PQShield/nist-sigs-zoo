#include <inttypes.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vole_private.h"

const gf256 GF256_ZERO = {.v64 = {0, 0, 0, 0}};
const gf256 GF256_ONE = {.v64 = {1, 0, 0, 0}};

__always_inline uint8_t gf256p_equals(const gf256* const a, const gf256* const b) {
  return memcmp(a->v, b->v, 32) == 0;
}

__always_inline void gf256p_sum(gf256* const res, const gf256* const a, const gf256* const b) {
  gf256_sum_ref(res, a, b);
}

__always_inline uint8_t gf256p_bitof(const gf256* const a, const uint64_t position) {
  CREQUIRE(position >= 0 && position < 256, "bad bit position %" PRIu64, position);
  const uint64_t q8 = position >> 3;
  const uint64_t r8 = position & 7;
  return (a->v[q8] >> r8) & 1;
}

void gf256p_lsh(gf256* const res, const gf256* const a, const uint64_t amount) {
  CREQUIRE(amount < 256, "bad left shift amount %" PRIu64, amount);
  if (amount == 0) {
    memcpy(res->v, a->v, 32);
    return;
  }
  const int64_t q8 = amount >> 3;
  const int64_t r8 = amount & 7;
  // shift by an exact amount of bytes
  for (int64_t i = 31; i >= q8; i--) {
    res->v[i] = a->v[i - q8];
  }
  memset(res->v, 0, q8);
  // shift by less than a byte
  if (r8 != 0) {
    for (int64_t i = 31; i > q8; i--) {
      res->v[i] = (res->v[i] << r8) | (res->v[i - 1] >> (8 - r8));
    }
    if (q8 != 0) {
      res->v[q8] = (res->v[q8] << r8) | (res->v[q8 - 1] >> (8 - r8));
    } else {
      res->v[0] = (res->v[0] << r8);
    }
  }
}

void gf256p_rsh(gf256* const res, const gf256* const a, const uint64_t amount) {
  CREQUIRE(amount < 256, "bad right shift amount %" PRIu64, amount);
  if (amount == 0) {
    memcpy(res->v, a->v, 32);
    return;
  }
  const int64_t q8 = amount >> 3;
  const int64_t _32_minus_q8 = 32 - q8;
  const int64_t r8 = amount & 7;
  // shift by an exact amount of bytes
  for (int64_t i = 0; i < _32_minus_q8; i++) {
    res->v[i] = a->v[i + q8];
  }
  memset(res->v + _32_minus_q8, 0, q8);
  // shift by less than a byte
  if (r8 != 0) {
    for (int64_t i = 0; i < _32_minus_q8 - 1; i++) {
      res->v[i] = (res->v[i] >> r8) | (res->v[i + 1] << (8 - r8));
    }
    if (q8 != 0) {
      res->v[_32_minus_q8 - 1] = (res->v[_32_minus_q8 - 1] >> r8) | (res->v[_32_minus_q8] << (8 - r8));
    } else {
      res->v[31] = (res->v[31] >> r8);
    }
  }
}

__always_inline void gf256p_mul(gf256* const res, const gf256* const a, const gf256* const b) {
  gf256_product_ref(res, a, b);
}

uint8_t gf256v_equals(const gf256 a, const gf256 b) { return gf256p_equals(&a, &b); }

gf256 gf256v_sum(const gf256 a, const gf256 b) {
  gf256 res;
  gf256p_sum(&res, &a, &b);
  return res;
}

uint8_t gf256v_bitof(const gf256 a, const uint64_t position) { return gf256p_bitof(&a, position); }

gf256 gf256v_lsh(const gf256 a, const uint64_t amount) {
  gf256 res;
  gf256p_lsh(&res, &a, amount);
  return res;
}

gf256 gf256v_rsh(const gf256 a, const uint64_t amount) {
  gf256 res;
  gf256p_rsh(&res, &a, amount);
  return res;
}

gf256 gf256v_mul(const gf256 a, const gf256 b) {
  gf256 res;
  gf256p_mul(&res, &a, &b);
  return res;
}

EXPORT void gf256_set_ref(gf256* res, const gf256* a) { *res = *a; }

EXPORT void gf256_sum_ref(gf256* res, const gf256* a, const gf256* b) {
  for (uint64_t i = 0; i < 32; ++i) res->v[i] = a->v[i] ^ b->v[i];
}

EXPORT void gf256_product_ref(gf256* res, const gf256* a, const gf256* b) {
  // deal with the in-place issue
  if (res == b) {
    gf256 tmp;
    gf256p_mul(&tmp, a, b);
    memcpy(res->v, tmp.v, 32);
    return;
  }
  // out of place product
  gf256 aa = *a;
  memset(res->v, 0, 32);
  for (uint64_t i = 0; i < 256; ++i) {
    if (gf256p_bitof(b, i)) {
      gf256p_sum(res, res, &aa);
    }
    if (gf256p_bitof(&aa, 255)) {
      gf256p_lsh(&aa, &aa, 1);
      aa.v64[0] ^= GF256_P;
    } else {
      gf256p_lsh(&aa, &aa, 1);
    }
  }
}

EXPORT void gf256_product_f2_ref(gf256* res, const gf256* a, const gf256* b_f2) {
  const uint64_t mask = -b_f2->v64[0];
  for (uint64_t i = 0; i < 4; ++i) res->v64[i] = a->v64[i] & mask;
}

EXPORT void gf256_inverse_ref(gf256* res, const gf256* a) {
  // this is the euclidian algorithm to compute the extended gcd
  gf256 aa = *a;
  CREQUIRE(!gf256p_equals(&aa, &GF256_ZERO), "inverse of 0");
  gf256 uu = GF256_ONE;
  uint64_t log2a = 255;
  while (gf256p_bitof(&aa, log2a) == 0) --log2a;
  gf256 bb;
  gf256p_lsh(&bb, &aa, 256 - log2a);
  bb.v64[0] ^= GF256_P;
  gf256 vv;
  gf256p_lsh(&vv, &uu, 256 - log2a);
  uint64_t log2b = 255;
  while (log2a != 0) {
    CASSERT(gf256v_equals(gf256v_mul(*a, uu), aa), "bug1");
    CASSERT(gf256v_equals(gf256v_mul(*a, vv), bb), "bug2");
    while (log2b >= log2a) {
      if (gf256p_bitof(&bb, log2b)) {
        gf256 tmp;
        gf256p_lsh(&tmp, &aa, log2b - log2a);
        gf256p_sum(&bb, &bb, &tmp);
        gf256p_lsh(&tmp, &uu, log2b - log2a);
        gf256p_sum(&vv, &vv, &tmp);
      }
      --log2b;
    }
    // swap aa <-> bb, uu <-> vv
    gf256 tmp = aa;
    aa = bb;
    bb = tmp;
    tmp = uu;
    uu = vv;
    vv = tmp;
    uint64_t tmp2 = log2a;
    log2a = log2b;
    log2b = tmp2;
    // update log2a
    CASSERT(!gf256p_equals(&aa, &GF256_ZERO), "bug3");
    while (gf256p_bitof(&aa, log2a) == 0) --log2a;
  }
  CASSERT(gf256p_equals(&aa, &GF256_ONE), "bug4");
  *res = uu;
}

EXPORT void gf256_sum_pow2_naive(gf256* res, const gf256* x) {
  gf256 ONE = GF256_ONE;
  gf256 r = GF256_ZERO;
  gf256 tmp;
  for (uint64_t i = 0; i < 256; ++i) {
    gf256p_lsh(&tmp, &ONE, i);
    gf256p_mul(&tmp, &tmp, x + i);
    gf256p_sum(&r, &r, &tmp);
  }
  *res = r;
}

EXPORT void gf256_sum_pow2_ref(gf256* res, const gf256* x) {
  static const gf256 PP = {.v64 = {GF256_P, 0, 0, 0}};
  gf256 left = x[0];
  gf256 right = GF256_ZERO;
  gf256 tmp;
  for (uint64_t i = 1; i < 256; ++i) {
    gf256p_lsh(&tmp, x + i, i);
    gf256p_sum(&left, &left, &tmp);
    gf256p_rsh(&tmp, x + i, 256 - i);
    gf256p_sum(&right, &right, &tmp);
  }
  gf256p_mul(&tmp, &right, &PP);
  gf256p_sum(res, &left, &tmp);
}

void gf256_echelon_pow2_naive(uint64_t k, gf256* res, const gf256* x, uint64_t x_size, uint64_t x_byte_slice) {
  CREQUIRE(x_byte_slice % sizeof(gf256) == 0, "byte slice not supported");
  CREQUIRE(k > 0, "k not supported");
  CREQUIRE(k * x_size <= 256, "echelon width not supported");
  const uint64_t xbs = x_byte_slice / sizeof(gf256);
  gf256 ONE = GF256_ONE;
  gf256 r = GF256_ZERO;
  gf256 tmp;
  for (uint64_t i = 0; i < x_size; ++i) {
    gf256p_lsh(&tmp, &ONE, k * i);
    gf256p_mul(&tmp, &tmp, x + i * xbs);
    gf256p_sum(&r, &r, &tmp);
  }
  *res = r;
}

void gf256_echelon_pow2_ref(uint64_t k, gf256* res, const gf256* x, uint64_t x_size, uint64_t x_byte_slice) {
  CREQUIRE(x_byte_slice % sizeof(gf256) == 0, "byte slice not supported");
  CASSERT(k > 0, "k not supported");
  CASSERT(k * x_size <= 256, "echelon width not supported");
  if (x_size == 0) {
    *res = GF256_ZERO;
    return;
  }
  const uint64_t xbs = x_byte_slice / sizeof(gf256);
  static const gf256 PP = {.v64 = {GF256_P, 0, 0, 0}};
  gf256 left = x[0];
  gf256 right = GF256_ZERO;
  gf256 tmp;
  for (uint64_t i = 1; i < x_size; ++i) {
    gf256p_lsh(&tmp, x + i * xbs, k * i);
    gf256p_sum(&left, &left, &tmp);
    gf256p_rsh(&tmp, x + i * xbs, 256 - k * i);
    gf256p_sum(&right, &right, &tmp);
  }
  gf256p_mul(&tmp, &right, &PP);
  gf256p_sum(res, &left, &tmp);
}
