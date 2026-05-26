#include <immintrin.h>

#include "vole_private.h"

#define PCLMUL_LO_X_LO 0
#define PCLMUL_LO_X_HI 16
#define PCLMUL_HI_X_LO 1
#define PCLMUL_HI_X_HI 17

static void gf128_square_pclmul(gf128* res, const gf128* a) {
  const __m128i p = _mm_set_epi64x(0, GF128_P);
  __m128i aa = _mm_loadu_si128((__m128i*)a);
  // square a
  __m128i u = _mm_clmulepi64_si128(aa, aa, PCLMUL_LO_X_LO);
  __m128i c = _mm_clmulepi64_si128(aa, aa, PCLMUL_HI_X_HI);
  // multiply upper half by p
  __m128i d = _mm_clmulepi64_si128(c, p, 1);
  u = _mm_xor_si128(u, _mm_clmulepi64_si128(c, p, 0));
  u = _mm_xor_si128(u, _mm_slli_si128(d, 8));
  c = _mm_srli_si128(d, 8);
  // multiply by c by p
  u = _mm_xor_si128(u, _mm_clmulepi64_si128(c, p, 0));
  _mm_storeu_si128((__m128i*)res, u);
}

void gf128_product_pclmul(gf128* res, const gf128* a, const gf128* b) {
  const __m128i p = _mm_set_epi64x(0, GF128_P);
  __m128i aa = _mm_loadu_si128((__m128i*)a);
  __m128i bb = _mm_loadu_si128((__m128i*)b);
  // multiply a and b
  __m128i u = _mm_clmulepi64_si128(aa, bb, 0);
  __m128i d1 = _mm_clmulepi64_si128(aa, bb, 1);
  __m128i d2 = _mm_clmulepi64_si128(aa, bb, 16);
  __m128i c = _mm_clmulepi64_si128(aa, bb, 17);
  __m128i d = _mm_xor_si128(d1, d2);
  u = _mm_xor_si128(u, _mm_slli_si128(d, 8));
  c = _mm_xor_si128(c, _mm_srli_si128(d, 8));
  // multiply by c by p
  d = _mm_clmulepi64_si128(c, p, 1);
  u = _mm_xor_si128(u, _mm_clmulepi64_si128(c, p, 0));
  u = _mm_xor_si128(u, _mm_slli_si128(d, 8));
  c = _mm_srli_si128(d, 8);
  // multiply by c by p
  u = _mm_xor_si128(u, _mm_clmulepi64_si128(c, p, 0));
  _mm_storeu_si128((__m128i*)res, u);
}

void gf128_product_pclmul_f2(gf128* res, const gf128* a, const gf128* b) {
  __m128i aa = _mm_loadu_si128((__m128i*)a);
  //__m128i bb = _mm_loadu_si128((__m128i*)b);

  __m128i u = _mm_set1_epi32(-b->v[0]);
  u = _mm_and_si128(u, aa);

  _mm_storeu_si128((__m128i*)res, u);
}

void gf128_product_small_pclmul(gf128* res, const uint64_t* a, const gf128* b) {
  const __m128i ap = _mm_set_epi64x(*a, GF128_P);
  __m128i reg[4];
  reg[0] = _mm_loadu_si128((__m128i*)b);
  // multiply a and b
  reg[2] = _mm_clmulepi64_si128(ap, reg[0], PCLMUL_HI_X_HI);
  reg[1] = _mm_clmulepi64_si128(ap, reg[0], PCLMUL_HI_X_LO);
  reg[3] = _mm_bslli_si128(reg[2], 8);  // low half of high result shifted up
  reg[2] = _mm_bsrli_si128(reg[2], 8);  // high half of high result shifted down
  reg[1] = _mm_xor_si128(reg[1], reg[3]);
  // now, reg[1] is the 128-bit low result and reg[2] has the high 64-bit result
  // all other registers may be used

  // reduction: Step 1. multiply high result in reg[2] by (p + X^128), return the high result
  // Since reg[2] holds only 64 bits, we have no wrap into the X^128 part
  // Step 2. Multiply by p and return the first 128 bit as remainder
  reg[0] = _mm_clmulepi64_si128(reg[2], ap, PCLMUL_LO_X_LO);
  // add remainder to the low multiplication result
  reg[1] = _mm_xor_si128(reg[0], reg[1]);
  _mm_storeu_si128((__m128i*)res, reg[1]);
}

void gf128_sum_avx2(gf128* res, const gf128* a, const gf128* b) {
  __m128i aa = _mm_loadu_si128((__m128i*)a);
  __m128i bb = _mm_loadu_si128((__m128i*)b);
  aa = _mm_xor_si128(aa, bb);
  _mm_storeu_si128((__m128i*)res, aa);
}

void gf128_inverse_pclmul(gf128* res, const gf128* a) {
  gf128 tmp, pow_a[6];  // pow_a[i] = a**(2**(2**(i+1))-1) --> a**3, a**15, a**255, a**65536, ...
  gf128* pow_a_ptr = &pow_a[0];

  gf128_square_pclmul(&tmp, a);
  gf128_product_pclmul(&pow_a[0], &tmp, a);  // pow_a[0] = a^3

  for (size_t i = 2; i < 64; i <<= 1) {
    gf128_square_pclmul(&tmp, pow_a_ptr);
    for (size_t j = 1; j < i; j++) {
      gf128_square_pclmul(&tmp, &tmp);
    }
    gf128_product_pclmul(pow_a_ptr + 1, &tmp, pow_a_ptr);
    pow_a_ptr++;
  }

  pow_a_ptr = &pow_a[4];
  for (size_t i = 32; i > 1; i >>= 1) {
    for (size_t j = 0; j < i; j++) {
      gf128_square_pclmul(&pow_a[5], &pow_a[5]);
    }
    gf128_product_pclmul(&pow_a[5], &pow_a[5], pow_a_ptr);
    pow_a_ptr--;
  }

  // pow_a[5] = a**(2**126-1)

  gf128_square_pclmul(&pow_a[5], &pow_a[5]);
  gf128_product_pclmul(&pow_a[5], &pow_a[5], a);

  // pow_a[5] = a**(2**127-1)

  gf128_square_pclmul(res, &pow_a[5]);  // res = (a**(2**127-1))**2 = a**(2*2**127-2) = a**(2**128-2) = a**-1
}

void gf128_echelon_pow2_avx(uint64_t k, gf128* res, const gf128* x, uint64_t x_size, uint64_t x_byte_slice) {
  CREQUIRE(x_byte_slice % sizeof(gf128) == 0, "byte slice not supported");
  CASSERT(k > 0, "k not supported");
  CASSERT(k * x_size <= 128, "echelon width not supported");
  if (x_size == 0) {
    *res = GF128_ZERO;
    return;
  }
  const uint64_t xbs = x_byte_slice / sizeof(gf128);
  static const gf128 PP = {.v128 = GF128_P};
  gf128 left = x[0];
  gf128 right = GF128_ZERO;
  gf128 tmp;
  for (uint64_t i = 1; i < x_size; ++i) {
    gf128p_lsh(&tmp, x + i * xbs, k * i);
    gf128_sum_avx2(&left, &left, &tmp);
    gf128p_rsh(&tmp, x + i * xbs, 128 - k * i);
    gf128_sum_avx2(&right, &right, &tmp);
  }
  gf128_product_pclmul(&tmp, &right, &PP);
  gf128_sum_avx2(res, &left, &tmp);
}
