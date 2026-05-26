#ifndef SDITH_ARITHMETIC_H
#define SDITH_ARITHMETIC_H

// This file contains the declaration of all aritmetic functions over F_2^128, F_2^192, and F_2^256

#include "commons.h"

typedef union gf128_t gf128 __attribute((aligned(16)));
typedef union gf192_t gf192 __attribute((aligned(8)));
typedef union gf256_t gf256 __attribute((aligned(32)));

/* res = a + b */
EXPORT void gf128_set_ref(gf128* res, const gf128* a);
EXPORT void gf192_set_ref(gf192* res, const gf192* a);
EXPORT void gf256_set_ref(gf256* res, const gf256* a);

/* res = a + b */
EXPORT void gf128_sum_ref(gf128* res, const gf128* a, const gf128* b);
EXPORT void gf128_sum_avx2(gf128* res, const gf128* a, const gf128* b);
EXPORT void gf192_sum_ref(gf192* res, const gf192* a, const gf192* b);
EXPORT void gf256_sum_ref(gf256* res, const gf256* a, const gf256* b);
EXPORT void gf256_sum_avx2(gf256* res, const gf256* a, const gf256* b);

/* res = a * b */
EXPORT void gf128_product_ref(gf128* res, const gf128* a, const gf128* b);
EXPORT void gf128_product_f2_ref(gf128* res, const gf128* a, const gf128* b_f2);
EXPORT void gf128_product_pclmul(gf128* res, const gf128* a, const gf128* b);
EXPORT void gf128_product_pclmul_f2(gf128* res, const gf128* a, const gf128* b);
EXPORT void gf192_product_ref(gf192* res, const gf192* a, const gf192* b);
EXPORT void gf192_product_f2_ref(gf192* res, const gf192* a, const gf192* b);
EXPORT void gf192_product_pclmul(gf192* res, const gf192* a, const gf192* b);
EXPORT void gf256_product_ref(gf256* res, const gf256* a, const gf256* b);
EXPORT void gf256_product_f2_ref(gf256* res, const gf256* a, const gf256* b);
EXPORT void gf256_product_pclmul(gf256* res, const gf256* a, const gf256* b);
EXPORT void gf256_product_pclmul_f2(gf256* res, const gf256* a, const gf256* b);

/* res = a * b when a has less than 64 bits */
EXPORT void gf128_product_small_ref(gf128* res, const uint64_t* a, const gf128* b);
EXPORT void gf128_product_small_pclmul(gf128* res, const uint64_t* a, const gf128* b);
EXPORT void gf192_product_small_ref(gf192* res, const uint64_t* a, const gf192* b);
EXPORT void gf192_product_small_pclmul(gf192* res, const uint64_t* a, const gf192* b);
EXPORT void gf256_product_small_ref(gf256* res, const uint64_t* a, const gf256* b);
EXPORT void gf256_product_small_pclmul(gf256* res, const uint64_t* a, const gf256* b);

/* res = a^-1 */
EXPORT void gf128_inverse_ref(gf128* res, const gf128* a);
EXPORT void gf128_inverse_pclmul(gf128* res, const gf128* a);
EXPORT void gf192_inverse_ref(gf192* res, const gf192* a);
EXPORT void gf192_inverse_pclmul(gf192* res, const gf192* a);
EXPORT void gf256_inverse_ref(gf256* res, const gf256* a);
EXPORT void gf256_inverse_pclmul(gf256* res, const gf256* a);

/** Combine lambda bit Vole into a big field Vole */
/** res = sum 2^i.x_i */
EXPORT void gf128_sum_pow2_naive(gf128* res, const gf128* x);
EXPORT void gf128_sum_pow2_ref(gf128* res, const gf128* x);
EXPORT void gf128_sum_pow2_avx2(gf128* res, const gf128* x);
EXPORT void gf192_sum_pow2_naive(gf192* res, const gf192* x);
EXPORT void gf192_sum_pow2_ref(gf192* res, const gf192* x);
EXPORT void gf192_sum_pow2_avx2(gf192* res, const gf192* x);
EXPORT void gf256_sum_pow2_naive(gf256* res, const gf256* x);
EXPORT void gf256_sum_pow2_ref(gf256* res, const gf256* x);
EXPORT void gf256_sum_pow2_avx2(gf256* res, const gf256* x);

/** sum_{i=0 to k-1} x_i . 2^{k.i} */
EXPORT void gf128_echelon_pow2_naive(uint64_t k, gf128* res, const gf128* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf128_echelon_pow2_ref(uint64_t k, gf128* res, const gf128* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf128_echelon_pow2_avx(uint64_t k, gf128* res, const gf128* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf192_echelon_pow2_naive(uint64_t k, gf192* res, const gf192* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf192_echelon_pow2_ref(uint64_t k, gf192* res, const gf192* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf192_echelon_pow2_avx(uint64_t k, gf192* res, const gf192* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf256_echelon_pow2_naive(uint64_t k, gf256* res, const gf256* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf256_echelon_pow2_ref(uint64_t k, gf256* res, const gf256* x, uint64_t x_size, uint64_t x_byte_slice);
EXPORT void gf256_echelon_pow2_avx(uint64_t k, gf256* res, const gf256* x, uint64_t x_size, uint64_t x_byte_slice);

/** This function takes a matrix of B x B bits, and transposes it */
EXPORT void transpose_128_128_naive(void* out, const void* in);
EXPORT void transpose_192_192_naive(void* out, const void* in);
EXPORT void transpose_256_256_naive(void* out, const void* in);
EXPORT void transpose_128_128_ref(void* x);
EXPORT void transpose_192_192_ref(void* x);
EXPORT void transpose_256_256_ref(void* x);
EXPORT void transpose_128_L_naive(void* out, const void* in, uint64_t L);
EXPORT void transpose_192_L_naive(void* out, const void* in, uint64_t L);
EXPORT void transpose_256_L_naive(void* out, const void* in, uint64_t L);
EXPORT void transpose_128_L_ref(void* out, const void* in, uint64_t L);
EXPORT void transpose_192_L_ref(void* out, const void* in, uint64_t L);
EXPORT void transpose_256_L_ref(void* out, const void* in, uint64_t L);

#endif  // SDITH_ARITHMETIC_H
