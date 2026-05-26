#include <immintrin.h>

#include "vole_private.h"

#define PCLMUL_LO_X_LO 0
#define PCLMUL_LO_X_HI 16
#define PCLMUL_HI_X_LO 1
#define PCLMUL_HI_X_HI 17

static void gf256_square_pclmul(gf256* res, const gf256* a) {
  const __m128i p = _mm_set_epi64x(0, GF256_P);
  __m128i a01 = _mm_loadu_si128((__m128i*)a);
  __m128i a23 = _mm_loadu_si128((__m128i*)&a->v64[2]);
  // multiply a and b
  // we have a = (a3X^192 + a2X^128 + a1X^64 + a0)
  // then c = c4X^256 + c3X^192 + c2X^128 + c1X^64 + c0
  // with c6 = a3a3
  // and  c4 = a2a2
  // and  c2 = a1a1
  // and  c0 = a0a0
  __m128i tmp[9];

  tmp[0] = _mm_clmulepi64_si128(a01, a01, PCLMUL_LO_X_LO);  // tmp0 = a0b0
  tmp[1] = _mm_clmulepi64_si128(a01, a01, PCLMUL_HI_X_HI);  // tmp3 = a1b1
  tmp[2] = _mm_clmulepi64_si128(a23, a23, PCLMUL_LO_X_LO);  // tmp8 = a2b2
  tmp[3] = _mm_clmulepi64_si128(a23, a23, PCLMUL_HI_X_HI);  // tmp6 = a3b3

  // reduction:
  // (1) multiply high result (in tmp2 and tmp3) by p, return 256 MSBs, then
  // (2) multiply that by p again, return 256 LSBs as remainder
  // (3) add remainder to 256 LSBs of multiplication result

  // Step 1: multiply high result (in tmp2 and tmp3) by p, return 256 MSBs
  // (c2L + c2HX^64 + c3LX^128 + c3HX^192) * (p + X^256) = p*c2L + p*c2HX^64 + p*c3LX^128 + p*c3HX^192 + c2LX^256 +
  // c2HX^320 + c3LX^384 + c3HX^448 but we only care about the high result: p*c2L + p*c2HX^64 + p*c3LX^128 are
  // completely in the low half the upper half of p*c3HX^192 is the first term of the high result c2LX^256 + c2HX^320 +
  // c3LX^384 + c3HX^448 is added to that
  tmp[4] = _mm_clmulepi64_si128(tmp[3], p, PCLMUL_HI_X_LO);
  tmp[4] = _mm_bsrli_si128(tmp[4], 8);
  tmp[2] = _mm_xor_si128(tmp[4], tmp[2]);

  // tmp2 and tmp3 now hold the high multiplication result, and now we multiply p again
  // but now we want the LSBs of (c2L + c2HX^64 + c3LX^128 + c3HX^192) * p
  tmp[4] = _mm_clmulepi64_si128(tmp[2], p, PCLMUL_LO_X_LO);  // tmp4 = p * c2L (X^0)
  tmp[5] = _mm_clmulepi64_si128(tmp[2], p, PCLMUL_HI_X_LO);  // tmp5 = p * c2H (X^64)
  tmp[6] = _mm_clmulepi64_si128(tmp[3], p, PCLMUL_LO_X_LO);  // tmp6 = p * c3L (X^128)
  tmp[7] = _mm_clmulepi64_si128(tmp[3], p, PCLMUL_HI_X_LO);  // tmp7 = p * c3H (X^192)
  tmp[0] = _mm_xor_si128(tmp[0], tmp[4]);                    // reduce LOW first step
  tmp[1] = _mm_xor_si128(tmp[1], tmp[6]);                    // reduce HIGH first step
  tmp[2] = _mm_bslli_si128(tmp[5], 8);
  tmp[3] = _mm_bsrli_si128(tmp[5], 8);
  tmp[4] = _mm_bslli_si128(tmp[7], 8);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[2]);  // final LOW reduction
  tmp[1] = _mm_xor_si128(tmp[1], tmp[3]);  // HIGH second step
  tmp[1] = _mm_xor_si128(tmp[1], tmp[4]);  // HIGH final step

  _mm_storeu_si128((__m128i*)res, tmp[0]);
  _mm_storeu_si128((__m128i*)&res->v64[2], tmp[1]);
}

EXPORT void gf256_product_pclmul(gf256* res, const gf256* a, const gf256* b) {
  const __m128i p = _mm_set_epi64x(0, GF256_P);
  __m128i a01 = _mm_loadu_si128((__m128i*)a);
  __m128i b01 = _mm_loadu_si128((__m128i*)b);
  __m128i a23 = _mm_loadu_si128((__m128i*)&a->v64[2]);
  __m128i b23 = _mm_loadu_si128((__m128i*)&b->v64[2]);
  // multiply a and b
  // we have a = (a3X^192 + a2X^128 + a1X^64 + a0) and b analogous
  // then c = c4X^256 + c3X^192 + c2X^128 + c1X^64 + c0
  // with c6 = a3b3
  // and  c5 = a3b2 + a2b3
  // and  c4 = a3b1 + a2b2 + a1b3
  // and  c3 = a3b0 + a2b1 + a1b2 + a0b3
  // and  c2 = a2b0 + a1b1 + a0b2
  // and  c1 = a1b0 + a0b1
  // and  c0 = a0b0
  __m128i tmp[9];

  tmp[0] = _mm_clmulepi64_si128(a01, b01, PCLMUL_LO_X_LO);  // tmp0 = a0b0
  tmp[1] = _mm_clmulepi64_si128(a01, b01, PCLMUL_LO_X_HI);  // tmp1 = a0b1
  tmp[2] = _mm_clmulepi64_si128(a01, b01, PCLMUL_HI_X_LO);  // tmp2 = a1b0
  tmp[3] = _mm_clmulepi64_si128(a01, b01, PCLMUL_HI_X_HI);  // tmp3 = a1b1
  tmp[4] = _mm_clmulepi64_si128(a01, b23, PCLMUL_LO_X_LO);  // tmp4 = a0b2
  tmp[5] = _mm_clmulepi64_si128(a01, b23, PCLMUL_LO_X_HI);  // tmp5 = a0b3
  tmp[6] = _mm_clmulepi64_si128(a01, b23, PCLMUL_HI_X_LO);  // tmp6 = a1b2
  tmp[7] = _mm_clmulepi64_si128(a01, b23, PCLMUL_HI_X_HI);  // tmp7 = a1b3
  tmp[8] = _mm_clmulepi64_si128(a23, b23, PCLMUL_LO_X_LO);  // tmp8 = a2b2

  // a01 is now free to be used as tmp register

  tmp[1] = _mm_xor_si128(tmp[1], tmp[2]);  // tmp1 = a0b1 + a1b0
  tmp[2] = _mm_xor_si128(tmp[3], tmp[4]);  // tmp2 = a1b1 + a0b2
  tmp[3] = _mm_xor_si128(tmp[5], tmp[6]);  // tmp3 = a0b3 + a1b2
  tmp[4] = _mm_xor_si128(tmp[7], tmp[8]);  // tmp4 = a1b3 + a2b2

  tmp[5] = _mm_clmulepi64_si128(a23, b23, PCLMUL_LO_X_HI);  // tmp5 = a2b3
  tmp[7] = _mm_clmulepi64_si128(a23, b23, PCLMUL_HI_X_LO);  // tmp7 = a3b2
  tmp[6] = _mm_clmulepi64_si128(a23, b23, PCLMUL_HI_X_HI);  // tmp6 = a3b3
  // b23 is now free to be used as tmp register
  tmp[8] = _mm_clmulepi64_si128(a23, b01, PCLMUL_LO_X_LO);  // tmp8 = a2b0
  a01 = _mm_clmulepi64_si128(a23, b01, PCLMUL_LO_X_HI);     // a01 = a2b1
  b23 = _mm_clmulepi64_si128(a23, b01, PCLMUL_HI_X_LO);     // b23 = a3b0
  a23 = _mm_clmulepi64_si128(a23, b01, PCLMUL_HI_X_HI);     // a23 = a3b1

  tmp[2] = _mm_xor_si128(tmp[2], tmp[8]);  // tmp2 = a1b1 + a0b2 + a2b0
  tmp[3] = _mm_xor_si128(tmp[3], a01);     // tmp3 = a0b3 + a1b2 + a2b1
  tmp[4] = _mm_xor_si128(tmp[4], a23);     // tmp4 = a1b3 + a2b2 + a3b1
  tmp[5] = _mm_xor_si128(tmp[5], tmp[7]);  // tmp5 = a2b3 + a3b2
  tmp[3] = _mm_xor_si128(tmp[3], b23);     // tmp3 = a0b3 + a1b2 + a2b1 + a3b0

  // condense to a 4-register representation where two hold the low result and two hold the high result
  // currently we have
  // c0c0
  //   c1c1
  //     c2c2
  //       c3 c3
  //          c4c4
  //            c5c5
  //              c6c6
  // and we want c0||c1 to hold the low result and c2||c3 to hold the high result
  tmp[7] = _mm_bslli_si128(tmp[1], 8);
  tmp[1] = _mm_bsrli_si128(tmp[1], 8);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[7]);  // tmp0 = c0 + c1L<<64
  tmp[1] = _mm_xor_si128(tmp[1], tmp[2]);  // tmp1 = c2 + c1H
  tmp[7] = _mm_bslli_si128(tmp[3], 8);     // tmp7 = c3L<<64
  tmp[2] = _mm_bsrli_si128(tmp[3], 8);     // tmp2 = c3H
  tmp[1] = _mm_xor_si128(tmp[1], tmp[7]);  // tmp1 = c2 + c1H + c3L<<64
  tmp[2] = _mm_xor_si128(tmp[2], tmp[4]);  // tmp2 = c3H + c4
  tmp[4] = _mm_bslli_si128(tmp[5], 8);     // tmp4 = c5L<<64
  tmp[5] = _mm_bsrli_si128(tmp[5], 8);     // tmp5 = c5H
  tmp[2] = _mm_xor_si128(tmp[2], tmp[4]);  // tmp2 = c3H + c4 + c5L<<64
  tmp[3] = _mm_xor_si128(tmp[6], tmp[5]);  // tmp3 = c6 + c5H

  // reduction:
  // (1) multiply high result (in tmp2 and tmp3) by p, return 256 MSBs, then
  // (2) multiply that by p again, return 256 LSBs as remainder
  // (3) add remainder to 256 LSBs of multiplication result

  // Step 1: multiply high result (in tmp2 and tmp3) by p, return 256 MSBs
  // (c2L + c2HX^64 + c3LX^128 + c3HX^192) * (p + X^256) = p*c2L + p*c2HX^64 + p*c3LX^128 + p*c3HX^192 + c2LX^256 +
  // c2HX^320 + c3LX^384 + c3HX^448 but we only care about the high result: p*c2L + p*c2HX^64 + p*c3LX^128 are
  // completely in the low half the upper half of p*c3HX^192 is the first term of the high result c2LX^256 + c2HX^320 +
  // c3LX^384 + c3HX^448 is added to that
  tmp[4] = _mm_clmulepi64_si128(tmp[3], p, PCLMUL_HI_X_LO);
  tmp[4] = _mm_bsrli_si128(tmp[4], 8);
  tmp[2] = _mm_xor_si128(tmp[4], tmp[2]);

  // tmp2 and tmp3 now hold the high multiplication result, and now we multiply p again
  // but now we want the LSBs of (c2L + c2HX^64 + c3LX^128 + c3HX^192) * p
  tmp[4] = _mm_clmulepi64_si128(tmp[2], p, PCLMUL_LO_X_LO);  // tmp4 = p * c2L (X^0)
  tmp[5] = _mm_clmulepi64_si128(tmp[2], p, PCLMUL_HI_X_LO);  // tmp5 = p * c2H (X^64)
  tmp[6] = _mm_clmulepi64_si128(tmp[3], p, PCLMUL_LO_X_LO);  // tmp6 = p * c3L (X^128)
  tmp[7] = _mm_clmulepi64_si128(tmp[3], p, PCLMUL_HI_X_LO);  // tmp7 = p * c3H (X^192)
  tmp[0] = _mm_xor_si128(tmp[0], tmp[4]);                    // reduce LOW first step
  tmp[1] = _mm_xor_si128(tmp[1], tmp[6]);                    // reduce HIGH first step
  tmp[2] = _mm_bslli_si128(tmp[5], 8);
  tmp[3] = _mm_bsrli_si128(tmp[5], 8);
  tmp[4] = _mm_bslli_si128(tmp[7], 8);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[2]);  // final LOW reduction
  tmp[1] = _mm_xor_si128(tmp[1], tmp[3]);  // HIGH second step
  tmp[1] = _mm_xor_si128(tmp[1], tmp[4]);  // HIGH final step

  _mm_storeu_si128((__m128i*)res, tmp[0]);
  _mm_storeu_si128((__m128i*)&res->v64[2], tmp[1]);
}

EXPORT void gf256_product_small_pclmul(gf256* res, const uint64_t* a, const gf256* b) {
  const __m128i ap = _mm_set_epi64x(*a, GF256_P);
  __m128i b01 = _mm_loadu_si128((__m128i*)b);
  __m128i b23 = _mm_loadu_si128((__m128i*)&b->v64[2]);
  // multiply a and b
  // we have b = (b3X^192 + b2X^128 + b1X^64 + b0)
  // then c = c3X^192 + c2X^128 + c1X^64 + c0
  // with c0 = a b0
  // and  c1 = a b1
  // and  c2 = a b2
  // and  c3 = a b3
  __m128i tmp[5];

  tmp[0] = _mm_clmulepi64_si128(ap, b01, PCLMUL_HI_X_LO);  // tmp0 = a b0
  tmp[1] = _mm_clmulepi64_si128(ap, b01, PCLMUL_HI_X_HI);  // tmp1 = a b1
  tmp[2] = _mm_clmulepi64_si128(ap, b23, PCLMUL_HI_X_LO);  // tmp2 = a b2
  tmp[3] = _mm_clmulepi64_si128(ap, b23, PCLMUL_HI_X_HI);  // tmp3 = a b3

  // condense to a 3-register representation where two hold the low result and two hold the high result
  // currently we have
  // c0c0
  //   c1c1
  //     c2c2
  //       c3 c3
  // and we want c0||c1 to hold the low result and c2 to hold the high result
  tmp[4] = _mm_bslli_si128(tmp[1], 8);
  tmp[1] = _mm_bsrli_si128(tmp[1], 8);
  tmp[0] = _mm_xor_si128(tmp[0], tmp[4]);  // tmp0 = c0 + c1L<<64
  tmp[1] = _mm_xor_si128(tmp[1], tmp[2]);  // tmp1 = c2 + c1H
  tmp[4] = _mm_bslli_si128(tmp[3], 8);     // tmp4 = c3L<<64
  tmp[2] = _mm_bsrli_si128(tmp[3], 8);     // tmp2 = c3H
  tmp[1] = _mm_xor_si128(tmp[1], tmp[4]);  // tmp1 = c2 + c1H + c3L<<64

  // reduction:
  // (1) multiply high result (in tmp2) by p, return 256 MSBs, then
  // (2) multiply that by p again, return 256 LSBs as remainder
  // (3) add remainder to 256 LSBs of multiplication result

  // Step 1: multiply high result (in tmp2) by p, return 256 MSBs
  // c2L * (p + X^256) = p*c2L + c2LX^256
  // we do not need to do anything as we only need the high result

  // tmp2 now hold the high multiplication result, and now we multiply p again
  // but now we want the LSBs of c2L * p
  tmp[2] = _mm_clmulepi64_si128(tmp[2], ap, PCLMUL_LO_X_LO);  // tmp4 = p * c2L
  tmp[0] = _mm_xor_si128(tmp[0], tmp[2]);

  _mm_storeu_si128((__m128i*)res, tmp[0]);
  _mm_storeu_si128((__m128i*)&res->v64[2], tmp[1]);
}

EXPORT void gf256_product_pclmul_f2(gf256* res, const gf256* a, const gf256* b) {
  __m256i bb = _mm256_set1_epi64x(-b->v64[0]);
  __m256i aa = _mm256_loadu_si256((__m256i*)a);
  bb = _mm256_and_si256(bb, aa);
  _mm256_storeu_si256((__m256i*)res, bb);
}

EXPORT void gf256_sum_avx2(gf256* res, const gf256* a, const gf256* b) {
  __m256i acc0, acc1, acc2;
  acc0 = _mm256_loadu_si256((__m256i*)(a->v));
  acc1 = _mm256_loadu_si256((__m256i*)(b->v));
  acc2 = _mm256_xor_si256(acc0, acc1);
  _mm256_storeu_si256((__m256i*)(res->v), acc2);
}

void gf256_inverse_pclmul(gf256* res, const gf256* a) {
  gf256 tmp, pow_a[7];  // pow_a[i] = a**(2**(2**(i+1))-1) --> a**3, a**15, a**255, a**65536, ...
  gf256* pow_a_ptr = &pow_a[0];

  gf256_square_pclmul(&tmp, a);
  gf256_product_pclmul(&pow_a[0], &tmp, a);  // pow_a[0] = a^3

  for (size_t i = 2; i < 128; i <<= 1) {
    gf256_square_pclmul(&tmp, pow_a_ptr);
    for (size_t j = 1; j < i; j++) {
      gf256_square_pclmul(&tmp, &tmp);
    }
    gf256_product_pclmul(pow_a_ptr + 1, &tmp, pow_a_ptr);
    pow_a_ptr++;
  }

  pow_a_ptr = &pow_a[5];
  for (size_t i = 64; i > 1; i >>= 1) {
    for (size_t j = 0; j < i; j++) {
      gf256_square_pclmul(&pow_a[6], &pow_a[6]);
    }
    gf256_product_pclmul(&pow_a[6], &pow_a[6], pow_a_ptr);
    pow_a_ptr--;
  }

  // pow_a[6] = a**(2**254-1)

  gf256_square_pclmul(&pow_a[6], &pow_a[6]);
  gf256_product_pclmul(&pow_a[6], &pow_a[6], a);

  // pow_a[6] = a**(2**255-1)

  gf256_square_pclmul(res, &pow_a[6]);  // res = (a**(2**255-1))**2 = a**(2*2**255-2) = a**(2**256-2) = a**-1
}

void gf256_echelon_pow2_avx(uint64_t k, gf256* res, const gf256* x, uint64_t x_size, uint64_t x_byte_slice) {
  CREQUIRE(x_byte_slice % sizeof(gf256) == 0, "byte slice not supported");
  CASSERT(k > 0, "k not supported");
  CASSERT(k * x_size <= 256, "echelon width not supported");
  CASSERT_ALIGNMENT(res, 32);

  if (x_size == 0) {
    *res = GF256_ZERO;
    return;
  }
  CASSERT_ALIGNMENT(x, 32);
  const uint64_t xbs = x_byte_slice / sizeof(gf256);
  static const gf256 PP = {.v64 = {GF256_P, 0, 0, 0}};
  gf256 left = x[0];
  gf256 right = GF256_ZERO;
  gf256 tmp;
  for (uint64_t i = 1; i < x_size; ++i) {
    gf256p_lsh(&tmp, x + i * xbs, k * i);
    gf256_sum_avx2(&left, &left, &tmp);
    gf256p_rsh(&tmp, x + i * xbs, 256 - k * i);
    gf256_sum_avx2(&right, &right, &tmp);
  }
  gf256_product_pclmul(&tmp, &right, &PP);
  gf256_sum_avx2(res, &left, &tmp);
}
