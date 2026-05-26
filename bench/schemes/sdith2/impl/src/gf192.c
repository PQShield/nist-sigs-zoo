#include <inttypes.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vole_private.h"

const gf192 GF192_ZERO = {.v64 = {0, 0, 0}};
const gf192 GF192_ONE = {.v64 = {1, 0, 0}};

__always_inline uint8_t gf192p_equals(const gf192* const a, const gf192* const b) {
  return memcmp(a->v, b->v, 24) == 0;
}

__always_inline void gf192p_sum(gf192* const res, const gf192* const a, const gf192* const b) {
  gf192_sum_ref(res, a, b);
}

__always_inline uint8_t gf192p_bitof(const gf192* const a, const uint64_t position) {
  CREQUIRE(position >= 0 && position < 192, "bad bit position %" PRIu64, position);
  const uint64_t q8 = position >> 3;
  const uint64_t r8 = position & 7;
  return (a->v[q8] >> r8) & 1;
}

void gf192p_lsh(gf192* const res, const gf192* const a, const uint64_t amount) {
  CREQUIRE(amount < 192, "bad left shift amount %" PRIu64, amount);
  if (amount == 0) {
    memcpy(res->v, a->v, 24);
    return;
  }
  const int64_t q8 = amount >> 3;
  const int64_t r8 = amount & 7;
  // shift by an exact amount of bytes
  for (int64_t i = 23; i >= q8; i--) {
    res->v[i] = a->v[i - q8];
  }
  memset(res->v, 0, q8);
  // shift by less than a byte
  if (r8 != 0) {
    for (int64_t i = 23; i > q8; i--) {
      res->v[i] = (res->v[i] << r8) | (res->v[i - 1] >> (8 - r8));
    }
    if (q8 != 0) {
      res->v[q8] = (res->v[q8] << r8) | (res->v[q8 - 1] >> (8 - r8));
    } else {
      res->v[0] = (res->v[0] << r8);
    }
  }
}

void gf192p_rsh(gf192* const res, const gf192* const a, const uint64_t amount) {
  CREQUIRE(amount < 192, "bad right shift amount %" PRIu64, amount);
  if (amount == 0) {
    memcpy(res->v, a->v, 24);
    return;
  }
  const int64_t q8 = amount >> 3;
  const int64_t _24_minus_q8 = 24 - q8;
  const int64_t r8 = amount & 7;
  // shift by an exact amount of bytes
  for (int64_t i = 0; i < _24_minus_q8; i++) {
    res->v[i] = a->v[i + q8];
  }
  memset(res->v + _24_minus_q8, 0, q8);
  // shift by less than a byte
  if (r8 != 0) {
    for (int64_t i = 0; i < _24_minus_q8 - 1; i++) {
      res->v[i] = (res->v[i] >> r8) | (res->v[i + 1] << (8 - r8));
    }
    if (q8 != 0) {
      res->v[_24_minus_q8 - 1] = (res->v[_24_minus_q8 - 1] >> r8) | (res->v[_24_minus_q8] << (8 - r8));
    } else {
      res->v[23] = (res->v[23] >> r8);
    }
  }
}

__always_inline void gf192p_mul(gf192* const res, const gf192* const a, const gf192* const b) {
  gf192_product_ref(res, a, b);
}

uint8_t gf192v_equals(const gf192 a, const gf192 b) { return gf192p_equals(&a, &b); }

gf192 gf192v_sum(const gf192 a, const gf192 b) {
  gf192 res;
  gf192p_sum(&res, &a, &b);
  return res;
}

uint8_t gf192v_bitof(const gf192 a, const uint64_t position) { return gf192p_bitof(&a, position); }

gf192 gf192v_lsh(const gf192 a, const uint64_t amount) {
  gf192 res;
  gf192p_lsh(&res, &a, amount);
  return res;
}

gf192 gf192v_rsh(const gf192 a, const uint64_t amount) {
  gf192 res;
  gf192p_rsh(&res, &a, amount);
  return res;
}

gf192 gf192v_mul(const gf192 a, const gf192 b) {
  gf192 res;
  gf192p_mul(&res, &a, &b);
  return res;
}

EXPORT void gf192_set_ref(gf192* res, const gf192* a) { *res = *a; }

EXPORT void gf192_sum_ref(gf192* res, const gf192* a, const gf192* b) {
  for (uint64_t i = 0; i < 3; ++i) res->v64[i] = a->v64[i] ^ b->v64[i];
}

EXPORT void gf192_product_ref(gf192* res, const gf192* a, const gf192* b) {
  // deal with the in-place issue
  if (res == b) {
    gf192 tmp;
    gf192p_mul(&tmp, a, b);
    memcpy(res->v, tmp.v, 24);
    return;
  }
  // out of place product
  gf192 aa = *a;
  memset(res->v, 0, 24);
  for (uint64_t i = 0; i < 192; ++i) {
    if (gf192p_bitof(b, i)) {
      gf192p_sum(res, res, &aa);
    }
    if (gf192p_bitof(&aa, 191)) {
      gf192p_lsh(&aa, &aa, 1);
      aa.v64[0] ^= GF192_P;
    } else {
      gf192p_lsh(&aa, &aa, 1);
    }
  }
}

EXPORT void gf192_product_f2_ref(gf192* res, const gf192* a, const gf192* b_f2) {
  const uint64_t mask = -b_f2->v64[0];
  for (uint64_t i = 0; i < 3; ++i) res->v64[i] = a->v64[i] & mask;
}

EXPORT void gf192_inverse_ref(gf192* res, const gf192* a) {
  // this is the euclidian algorithm to compute the extended gcd
  gf192 aa = *a;
  CREQUIRE(!gf192p_equals(&aa, &GF192_ZERO), "inverse of 0");
  gf192 uu = GF192_ONE;
  uint64_t log2a = 191;
  while (gf192p_bitof(&aa, log2a) == 0) --log2a;
  gf192 bb;
  gf192p_lsh(&bb, &aa, 192 - log2a);
  bb.v64[0] ^= GF192_P;
  gf192 vv;
  gf192p_lsh(&vv, &uu, 192 - log2a);
  uint64_t log2b = 191;
  while (log2a != 0) {
    CASSERT(gf192v_equals(gf192v_mul(*a, uu), aa), "bug1");
    CASSERT(gf192v_equals(gf192v_mul(*a, vv), bb), "bug2");
    while (log2b >= log2a) {
      if (gf192p_bitof(&bb, log2b)) {
        gf192 tmp;
        gf192p_lsh(&tmp, &aa, log2b - log2a);
        gf192p_sum(&bb, &bb, &tmp);
        gf192p_lsh(&tmp, &uu, log2b - log2a);
        gf192p_sum(&vv, &vv, &tmp);
      }
      --log2b;
    }
    // swap aa <-> bb, uu <-> vv
    gf192 tmp = aa;
    aa = bb;
    bb = tmp;
    tmp = uu;
    uu = vv;
    vv = tmp;
    uint64_t tmp2 = log2a;
    log2a = log2b;
    log2b = tmp2;
    // update log2a
    CASSERT(!gf192p_equals(&aa, &GF192_ZERO), "bug3");
    while (gf192p_bitof(&aa, log2a) == 0) --log2a;
  }
  CASSERT(gf192p_equals(&aa, &GF192_ONE), "bug4");
  *res = uu;
}

EXPORT void gf192_sum_pow2_naive(gf192* res, const gf192* x) {
  gf192 ONE = GF192_ONE;
  gf192 r = GF192_ZERO;
  gf192 tmp;
  for (uint64_t i = 0; i < 192; ++i) {
    gf192p_lsh(&tmp, &ONE, i);
    gf192p_mul(&tmp, &tmp, x + i);
    gf192p_sum(&r, &r, &tmp);
  }
  *res = r;
}

EXPORT void gf192_sum_pow2_ref(gf192* res, const gf192* x) {
  static const gf192 PP = {.v64 = {GF192_P, 0, 0}};
  gf192 left = x[0];
  gf192 right = GF192_ZERO;
  gf192 tmp;
  for (uint64_t i = 1; i < 192; ++i) {
    gf192p_lsh(&tmp, x + i, i);
    gf192p_sum(&left, &left, &tmp);
    gf192p_rsh(&tmp, x + i, 192 - i);
    gf192p_sum(&right, &right, &tmp);
  }
  gf192p_mul(&tmp, &right, &PP);
  gf192p_sum(res, &left, &tmp);
}

void gf192_echelon_pow2_naive(uint64_t k, gf192* res, const gf192* x, uint64_t x_size, uint64_t x_byte_slice) {
  CREQUIRE(x_byte_slice % sizeof(gf192) == 0, "byte slice not supported");
  CREQUIRE(k > 0, "k not supported");
  CREQUIRE(k * x_size <= 192, "echelon width not supported");
  const uint64_t xbs = x_byte_slice / sizeof(gf192);
  gf192 ONE = GF192_ONE;
  gf192 r = GF192_ZERO;
  gf192 tmp;
  for (uint64_t i = 0; i < x_size; ++i) {
    gf192p_lsh(&tmp, &ONE, k * i);
    gf192p_mul(&tmp, &tmp, x + i * xbs);
    gf192p_sum(&r, &r, &tmp);
  }
  *res = r;
}

void gf192_echelon_pow2_ref(uint64_t k, gf192* res, const gf192* x, uint64_t x_size, uint64_t x_byte_slice) {
  CREQUIRE(x_byte_slice % sizeof(gf192) == 0, "byte slice not supported");
  CASSERT(k > 0, "k not supported");
  CASSERT(k * x_size <= 192, "echelon width not supported");
  if (x_size == 0) {
    *res = GF192_ZERO;
    return;
  }
  const uint64_t xbs = x_byte_slice / sizeof(gf192);
  static const gf192 PP = {.v64 = {GF192_P, 0, 0}};
  gf192 left = x[0];
  gf192 right = GF192_ZERO;
  gf192 tmp;
  for (uint64_t i = 1; i < x_size; ++i) {
    gf192p_lsh(&tmp, x + i * xbs, k * i);
    gf192p_sum(&left, &left, &tmp);
    gf192p_rsh(&tmp, x + i * xbs, 192 - k * i);
    gf192p_sum(&right, &right, &tmp);
  }
  gf192p_mul(&tmp, &right, &PP);
  gf192p_sum(res, &left, &tmp);
}
