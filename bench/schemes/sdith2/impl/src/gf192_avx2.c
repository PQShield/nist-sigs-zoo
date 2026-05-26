#include <immintrin.h>

#include "vole_private.h"

#define PCLMUL_LO_X_LO 0
#define PCLMUL_LO_X_HI 16
#define PCLMUL_HI_X_LO 1
#define PCLMUL_HI_X_HI 17

static void gf192_square_pclmul(gf192* res, const gf192* a) {
  const __m128i pa2 = _mm_set_epi64x(a->v64[2], GF192_P);
  __m128i a01 = _mm_loadu_si128((__m128i*)a);
  // multiply a and b
  // we have a = (a2X^128 + a1X^64 + a0)
  // then c = c2X^256 + c1X^128 + c0
  // with c2 = a2a2
  // and  c1 = a1a1
  // and  c0 = a0a0
  __m128i c0, c1, c2, c3, tmp[4];

  c0 = _mm_clmulepi64_si128(a01, a01, PCLMUL_LO_X_LO);  // c0 = a0a0
  c1 = _mm_clmulepi64_si128(a01, a01, PCLMUL_HI_X_HI);  // c1 = a1a1
  c2 = _mm_clmulepi64_si128(pa2, pa2, PCLMUL_HI_X_HI);  // c2 = a2a2

  // condense to a 4-register representation where two hold the low result and two hold the high result
  // currently we have
  // c0c0
  //     c1c1
  //         c2c2
  // and we want c0||c1 to hold the low result and c2||c3 to hold the high result
  tmp[0] = _mm_setzero_si128();
  c3 = _mm_srli_si128(c2, 8);                // upper half of c2 goes to c3 lower half
  c2 = _mm_slli_si128(c2, 8);                // lower half of c2 goes to c2 upper half
  tmp[1] = _mm_srli_si128(c1, 8);            // move upper half of c1 down to tmp1
  c2 = _mm_blend_epi32(c2, tmp[1], 0b0011);  // upper half stays in c2, lower half taken from tmp1

  // c1 still has its upper bits that are moved to c2!

  // reduction:
  // (1) multiply high result (in c2 and c3) by p, return 192 MSBs, then
  // (2) multiply that by p again, return 192 LSBs as remainder
  // (3) add remainder to 192 LSBs of multiplication result

  // Step 1: multiply high result (in c2 and c3) by p, return 192 MSBs
  // (c2L + c2HX^64 + c3LX^128) * (p + X^192) = p*c2L + p*c2HX^64 + p*c3LX^128 + c2LX^192 + c2HX^256 + c3LX^384
  // but we only care about the high result:
  // p*c2L + p*c2HX^64 are completely in the low half
  // the upper half of p*c3LX^128 is the first term of the high result
  // c2LX^192 + c2HX^256 + c3LX^384 is added to that
  tmp[1] = _mm_clmulepi64_si128(c3, pa2, PCLMUL_LO_X_LO);
  tmp[1] = _mm_bsrli_si128(tmp[1], 8);
  c2 = _mm_xor_si128(tmp[1], c2);

  // c2 and c3 now hold the high multiplication result, and now we multiply p again
  // but now we want the LSBs of (c2L + c2HX^64 + c3LX^128) * p
  tmp[0] = _mm_clmulepi64_si128(c3, pa2, PCLMUL_LO_X_LO);  // tmp0 = p * c3  (HIGH)
  tmp[2] = _mm_clmulepi64_si128(c2, pa2, PCLMUL_LO_X_LO);  // tmp1 = p * c2L (LOW)
  tmp[3] = _mm_clmulepi64_si128(c2, pa2, PCLMUL_HI_X_LO);  // tmp2 = p * c2H (MID)
  c1 = _mm_xor_si128(c1, tmp[0]);                          // reduce HIGH
  c0 = _mm_xor_si128(c0, tmp[2]);                          // reduce LOW
  tmp[0] = _mm_bsrli_si128(tmp[3], 8);                     // tmp0 = (p*c2H) >> 64
  tmp[2] = _mm_bslli_si128(tmp[3], 8);                     // tmp2 = (p*c2H) << 64
  c1 = _mm_xor_si128(c1, tmp[0]);                          // reduce MID
  c0 = _mm_xor_si128(c0, tmp[2]);                          // reduce MID

  _mm_storeu_si128((__m128i*)res, c0);
  res->v64[2] = _mm_extract_epi64(c1, 0);
}

EXPORT void gf192_product_pclmul(gf192* res, const gf192* a, const gf192* b) {
  const __m128i p = _mm_set_epi64x(0, GF192_P);
  __m128i a01 = _mm_loadu_si128((__m128i*)a);
  __m128i b01 = _mm_loadu_si128((__m128i*)b);
  __m128i a2b2 = _mm_set_epi64x(a->v64[2], b->v64[2]);
  // multiply a and b
  // we have a = (a2X^128 + a1X^64 + a0) and b analogous
  // then c = c4X^256 + c3X^192 + c2X^128 + c1X^64 + c0
  // with c4 = a2b2
  // and  c3 = a2b1 + a1b2
  // and  c2 = a2b0 + a1b1 + a0b2
  // and  c1 = a1b0 + a0b1
  // and  c0 = a0b0
  __m128i c0, c1, c2, c3, c4, tmp[4];

  c0 = _mm_clmulepi64_si128(a01, b01, PCLMUL_LO_X_LO);       // c0 = a0b0
  c1 = _mm_clmulepi64_si128(a01, b01, PCLMUL_HI_X_LO);       // c1 = a1b0
  c2 = _mm_clmulepi64_si128(a2b2, b01, PCLMUL_HI_X_LO);      // c2 = a2b0
  c3 = _mm_clmulepi64_si128(a2b2, b01, PCLMUL_HI_X_HI);      // c3 = a2b1
  c4 = _mm_clmulepi64_si128(a2b2, a2b2, PCLMUL_HI_X_LO);     // c4 = a2b2
  tmp[0] = _mm_clmulepi64_si128(a01, b01, PCLMUL_LO_X_HI);   // tmp0 = a0b1
  tmp[1] = _mm_clmulepi64_si128(a01, b01, PCLMUL_HI_X_HI);   // tmp1 = a1b1
  tmp[2] = _mm_clmulepi64_si128(a01, a2b2, PCLMUL_LO_X_LO);  // tmp2 = a0b2
  tmp[3] = _mm_clmulepi64_si128(a01, a2b2, PCLMUL_HI_X_LO);  // tmp3 = a1b2

  c1 = _mm_xor_si128(c1, tmp[0]);  // c1 = a1b0 + a0b1
  c2 = _mm_xor_si128(c2, tmp[1]);  // c2 = a2b0 + a1b1
  c3 = _mm_xor_si128(c3, tmp[3]);  // c3 = a2b1 + a1b2
  c2 = _mm_xor_si128(c2, tmp[2]);  // c2 = a2b0 + a1b1 + a0b2

  // condense to a 4-register representation where two hold the low result and two hold the high result
  // currently we have
  // c0c0
  //   c1c1
  //     c2c2
  //       c3c3
  //         c4c4
  // and we want c0||c1 to hold the low result and c2||c3 to hold the high result
  // so we need to do:
  // - c0 += c1L<<64
  // - c1 := c1H + c2L
  // - c2 := c3 + c2H + (c4L<<64)
  // - c3 := c4H
  tmp[0] = _mm_setzero_si128();
  c1 = _mm_shuffle_epi32(c1, 0b01001110);        // c1 := c1 with swapped 64-bit words
  c4 = _mm_shuffle_epi32(c4, 0b01001110);        // c4 := c4 with swapped 64-bit words
  tmp[2] = _mm_shuffle_epi32(c2, 0b01001110);    // tmp2 := c2 with swapped 64-bit words
  tmp[3] = _mm_blend_epi32(c1, tmp[0], 0b0011);  // tmp3 only has the high half of c1, which is now c1L
  tmp[1] = _mm_blend_epi32(tmp[2], c4, 0b1100);  // tmp1 has now c2H + (c4L<<64)
  c1 = _mm_xor_si128(c1, c2);                    // c1 := c1H + c2L (plus nonsense higher 64 bits)
  c0 = _mm_xor_si128(c0, tmp[3]);                // c0 := c0 + (c1L<<64)
  c2 = _mm_xor_si128(c3, tmp[1]);                // c2 := c3 + c2H + (c4L<<64)
  c3 = _mm_blend_epi32(c4, tmp[0], 0b1100);      // c3 := c4H
  c1 = _mm_blend_epi32(c1, tmp[0], 0b1100);      // c1 := c1H + c2L

  // c4 is now free to be used as tmp registers

  // reduction:
  // (1) multiply high result (in c2 and c3) by p, return 192 MSBs, then
  // (2) multiply that by p again, return 192 LSBs as remainder
  // (3) add remainder to 192 LSBs of multiplication result

  // Step 1: multiply high result (in c2 and c3) by p, return 192 MSBs
  // (c2L + c2HX^64 + c3LX^128) * (p + X^192) = p*c2L + p*c2HX^64 + p*c3LX^128 + c2LX^192 + c2HX^256 + c3LX^384
  // but we only care about the high result:
  // p*c2L + p*c2HX^64 are completely in the low half
  // the upper half of p*c3LX^128 is the first term of the high result
  // c2LX^192 + c2HX^256 + c3LX^384 is added to that
  tmp[1] = _mm_clmulepi64_si128(c3, p, PCLMUL_LO_X_LO);
  tmp[1] = _mm_bsrli_si128(tmp[1], 8);
  c2 = _mm_xor_si128(tmp[1], c2);

  // c2 and c3 now hold the high multiplication result, and now we multiply p again
  // but now we want the LSBs of (c2L + c2HX^64 + c3LX^128) * p
  tmp[0] = _mm_clmulepi64_si128(c3, p, PCLMUL_LO_X_LO);  // tmp0 = p * c3  (HIGH)
  tmp[2] = _mm_clmulepi64_si128(c2, p, PCLMUL_LO_X_LO);  // tmp1 = p * c2L (LOW)
  tmp[3] = _mm_clmulepi64_si128(c2, p, PCLMUL_HI_X_LO);  // tmp2 = p * c2H (MID)
  c1 = _mm_xor_si128(c1, tmp[0]);                        // reduce HIGH
  c0 = _mm_xor_si128(c0, tmp[2]);                        // reduce LOW
  tmp[0] = _mm_bsrli_si128(tmp[3], 8);                   // tmp0 = (p*c2H) >> 64
  tmp[2] = _mm_bslli_si128(tmp[3], 8);                   // tmp2 = (p*c2H) << 64
  c1 = _mm_xor_si128(c1, tmp[0]);                        // reduce MID
  c0 = _mm_xor_si128(c0, tmp[2]);                        // reduce MID

  _mm_storeu_si128((__m128i*)res, c0);
  res->v64[2] = _mm_extract_epi64(c1, 0);
}

EXPORT void gf192_product_small_pclmul(gf192* res, const uint64_t* a, const gf192* b) {
  const __m128i ap = _mm_set_epi64x(*a, GF192_P);
  __m128i b01 = _mm_loadu_si128((__m128i*)b);
  __m128i b2 = _mm_set_epi64x(0, b->v64[2]);
  // multiply a and b
  // we have a = (a2X^128 + a1X^64 + a0) and b analogous
  // then c = c4X^256 + c3X^192 + c2X^128 + c1X^64 + c0
  // with c2 = a b2
  // and  c1 = a b1
  // and  c0 = a b0
  __m128i c0, c1, c2, tmp;

  c0 = _mm_clmulepi64_si128(ap, b01, PCLMUL_HI_X_LO);  // c0 = a b0
  c1 = _mm_clmulepi64_si128(ap, b01, PCLMUL_HI_X_HI);  // c1 = a b1
  c2 = _mm_clmulepi64_si128(ap, b2, PCLMUL_HI_X_LO);   // c2 = a b2

  // condense to a 3-register representation (2 regs hold low mul res, 1 hold high mul res)
  // currently we have
  // c0c0
  //   c1c1
  //     c2c2
  // and we want c0||c1 to hold the low result and c2 to hold the high result
  // so we need to do:
  // - c0 += c1L<<64
  // - c1 := c1H + c2L
  // - c2 := c2H
  tmp = _mm_bslli_si128(c1, 8);  // tmp := c1L<<64
  c1 = _mm_bsrli_si128(c1, 8);   // c1 := c1H>>64
  c1 = _mm_xor_si128(c1, c2);    // c1 := (c1H>>64) + c2L (high bits are garbage)
  c2 = _mm_bsrli_si128(c2, 8);   // c2 := c2H>>64
  c0 = _mm_xor_si128(c0, tmp);

  // reduction:
  // (1) multiply high result (in c2) by p, return 192 MSBs, then
  // (2) multiply that by p again, return 192 LSBs as remainder
  // (3) add remainder to 192 LSBs of multiplication result

  // Step 1: multiply high result (in c2) by p, return 192 MSBs
  // c2L * (p + X^192) = c2L * p + C2LX^192
  // but we only care about the high result, so we just do nothing, since the high result is already there

  // c2 now holds the high multiplication result, and now we multiply p again
  // but now we want the LSBs of c2L * p
  tmp = _mm_clmulepi64_si128(c2, ap, PCLMUL_LO_X_LO);  // tmp = p * c2L
  c0 = _mm_xor_si128(c0, tmp);                         // reduce

  res->v64[2] = _mm_extract_epi64(c1, 0);
  _mm_storeu_si128((__m128i*)res, c0);
}

EXPORT void gf192_sum_pow2_avx2(gf192* res, const gf192* x) { gf192_sum_pow2_ref(res, x); }

void gf192_inverse_pclmul(gf192* res, const gf192* a) {
  gf192 tmp, pow_a[7];  // pow_a[i] = a**(2**(2**(i+1))-1) --> a**3, a**15, a**255, a**65536, ...
  gf192* pow_a_ptr = &pow_a[0];

  gf192_square_pclmul(&tmp, a);
  gf192_product_pclmul(&pow_a[0], &tmp, a);  // pow_a[0] = a^3

  for (size_t i = 2; i < 128; i <<= 1) {
    gf192_square_pclmul(&tmp, pow_a_ptr);
    for (size_t j = 1; j < i; j++) {
      gf192_square_pclmul(&tmp, &tmp);
    }
    gf192_product_pclmul(pow_a_ptr + 1, &tmp, pow_a_ptr);
    pow_a_ptr++;
  }

  pow_a_ptr = &pow_a[4];
  for (size_t i = 32; i > 1; i >>= 1) {
    for (size_t j = 0; j < i; j++) {
      gf192_square_pclmul(&pow_a[6], &pow_a[6]);
    }
    gf192_product_pclmul(&pow_a[6], &pow_a[6], pow_a_ptr);
    pow_a_ptr--;
  }

  // pow_a[6] = a**(2**190-1)

  gf192_square_pclmul(&pow_a[6], &pow_a[6]);
  gf192_product_pclmul(&pow_a[6], &pow_a[6], a);

  // pow_a[6] = a**(2**191-1)

  gf192_square_pclmul(res, &pow_a[6]);  // res = (a**(2**191-1))**2 = a**(2*2**191-2) = a**(2**192-2) = a**-1
}

void gf192_echelon_pow2_avx(uint64_t k, gf192* res, const gf192* x, uint64_t x_size, uint64_t x_byte_slice) {
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
    gf192_sum_ref(&left, &left, &tmp);
    gf192p_rsh(&tmp, x + i * xbs, 192 - k * i);
    gf192_sum_ref(&right, &right, &tmp);
  }
  gf192_product_pclmul(&tmp, &right, &PP);
  gf192_sum_ref(res, &left, &tmp);
}
