#include <string.h>
#include <immintrin.h>

#include "vole_private.h"


// Compute parity of a 64-bit value
static inline uint8_t parity64(uint64_t x) {
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    return (uint8_t)((0x6996 >> (x & 0xF)) & 1);
}

// Reduce a __m256i to a 64-bit XOR of all its bits
static inline uint64_t avx2_reduce_to_64(__m256i x) {
    __m128i x128 = _mm_xor_si128(_mm256_castsi256_si128(x),
                                 _mm256_extracti128_si256(x, 1));
    x128 = _mm_xor_si128(x128, _mm_srli_si128(x128, 8));
    uint64_t val64 = (uint64_t)_mm_cvtsi128_si64(x128);
    return val64;
}

// Reduce a __m128i to a 64-bit XOR of all its bits
static inline uint64_t sse2_reduce_to_64(__m128i x) {
    x = _mm_xor_si128(x, _mm_srli_si128(x, 8));
    uint64_t val64 = (uint64_t)_mm_cvtsi128_si64(x);
    return val64;
}


EXPORT void matrix_vector_product_f2_avx2(
    uint64_t nrows, uint64_t ncols,
    bitvec_t* res,
    const bitmat_t* a,
    const bitvec_t* b)
{
    memset(res, 0, (nrows+7) >> 3);

    uint64_t n_256 = ncols >> 8; 
    uint64_t r = ncols & 0xFF;

    uint64_t n_128 = r >> 7;
    r &= 0x7F;

    uint64_t n_64 = r >> 6;
    r &= 0x3F;

    uint64_t n_32 = r >> 5;
    r &= 0x1F;

    uint64_t n_8 = r >> 3; // remainder in bytes

    for (uint64_t i = 0; i < nrows; i++) {
        uint64_t accum = 0ULL; // accumulate XOR of all bits

        uint64_t row_offset = i * ncols;

        // 1. Process 256-bit blocks with AVX2
        if (n_256 > 0) {
            __m256i avx_accum = _mm256_setzero_si256();
            for (uint64_t c = 0; c < n_256; c++) {
                uint64_t bitpos = row_offset + (c << 8);
                const uint8_t* A_ptr = a + (bitpos >> 3);
                const uint8_t* B_ptr = b + c*32;

                __m256i A_vec = _mm256_loadu_si256((const __m256i*)A_ptr);
                __m256i B_vec = _mm256_loadu_si256((const __m256i*)B_ptr);
                __m256i and_vec = _mm256_and_si256(A_vec, B_vec);
                avx_accum = _mm256_xor_si256(avx_accum, and_vec);
            }
            accum ^= avx2_reduce_to_64(avx_accum);
        }

        uint64_t used_bits = n_256 << 8;
        uint64_t offset_after_256 = row_offset + used_bits;

        // 2. Process 128-bit blocks with SSE2
        if (n_128 > 0) {
            __m128i sse_accum = _mm_setzero_si128();
            for (uint64_t c = 0; c < n_128; c++) {
                uint64_t bitpos = offset_after_256 + (c << 7);
                const uint8_t* A_ptr = a + (bitpos >> 3);
                const uint8_t* B_ptr = b + (used_bits >> 3) + (c << 4); 

                __m128i A_vec = _mm_loadu_si128((const __m128i*)A_ptr);
                __m128i B_vec = _mm_loadu_si128((const __m128i*)B_ptr);
                __m128i and_vec = _mm_and_si128(A_vec, B_vec);
                sse_accum = _mm_xor_si128(sse_accum, and_vec);
            }
            accum ^= sse2_reduce_to_64(sse_accum);
        }

        used_bits += n_128 << 7;
        uint64_t offset_after_128 = row_offset + used_bits;

        // 3. Process 64-bit blocks
        for (uint64_t c = 0; c < n_64; c++) {
            uint64_t bitpos = offset_after_128 + (c << 6);
            const uint8_t* A_ptr = a + (bitpos >> 3);
            const uint8_t* B_ptr = b + (used_bits >> 3) + (c << 3);

            uint64_t A_val = *(const uint64_t*)A_ptr;
            uint64_t B_val = *(const uint64_t*)B_ptr;
            accum ^= (A_val & B_val);
        }

        used_bits += n_64 << 6;
        uint64_t offset_after_64 = row_offset + used_bits;

        // 4. Process 32-bit blocks
        for (uint64_t c = 0; c < n_32; c++) {
            uint64_t bitpos = offset_after_64 + (c << 5);
            const uint8_t* A_ptr = a + (bitpos >> 3);
            const uint8_t* B_ptr = b + (used_bits >> 3) + (c << 2);

            uint32_t A_val = *(const uint32_t*)A_ptr;
            uint32_t B_val = *(const uint32_t*)B_ptr;
            uint32_t and_val = A_val & B_val;
            accum ^= (uint64_t)and_val;
        }

        used_bits += n_32 << 5;
        uint64_t offset_after_32 = row_offset + used_bits;

        // 5. Process 8-bit blocks
        for (uint64_t c = 0; c < n_8; c++) {
            uint64_t bitpos = offset_after_32 + (c << 3);
            const uint8_t* A_ptr = a + (bitpos >> 3);
            const uint8_t* B_ptr = b + (used_bits >> 3) + c;

            uint8_t A_val = *A_ptr;
            uint8_t B_val = *B_ptr;
            accum ^= (A_val & B_val);
        }

        // Compute parity of accum
        uint8_t parity = parity64(accum);

        // Set bit in res if parity=1
        if (parity) {
            uint64_t byte_idx = i >> 3;
            uint64_t bit_idx = i & 7;
            ((uint8_t*)res)[byte_idx] ^= (uint8_t)(1 << bit_idx);
        }
    }
}






// Specialized function for lambda=128 bits (16 bytes)
static void matrix_vector_product_f2_block_128(
    uint64_t nrows,
    uint64_t ncols,
    uint8_t* res,
    const uint8_t* A,
    const uint8_t* B
) {
    const uint64_t lambda = 16;
    memset(res, 0, nrows * lambda);

    uint64_t full_bytes = ncols / 8;
    uint64_t leftover_bits = ncols % 8;

    for (uint64_t i = 0; i < nrows; i++) {
        __m256i acc256_0 = _mm256_setzero_si256();

        const uint8_t* A_row = A + (i * ncols) / 8;
        for (uint64_t byte_i = 0; byte_i < full_bytes; byte_i++) {
            uint8_t a_byte = A_row[byte_i];
            if (a_byte == 0)
                continue;
            for (int bit = 0; bit < 8; bit++) {
                if (a_byte & (1 << bit)) {
                    uint64_t col = byte_i * 8 + bit;
                    const uint8_t* b_j = B + col * lambda;
                    __m128i tmp128 = _mm_loadu_si128((const __m128i*)b_j);
                    __m256i tmp256 = _mm256_zextsi128_si256(tmp128);
                    acc256_0 = _mm256_xor_si256(acc256_0, tmp256);
                }
            }
        }

        if (leftover_bits > 0) {
            uint64_t start_col = full_bytes * 8;
            uint64_t bit_index = i * ncols + start_col;
            uint8_t a_last_byte = A[bit_index / 8];
            a_last_byte >>= (bit_index % 8);
            a_last_byte &= (uint8_t)((1 << leftover_bits) - 1);
            if (a_last_byte) {
                for (int bit = 0; bit < (int)leftover_bits; bit++) {
                    if (a_last_byte & (1 << bit)) {
                        uint64_t col = start_col + bit;
                        const uint8_t* b_j = B + col * lambda;
                        __m128i tmp128 = _mm_loadu_si128((const __m128i*)b_j);
                        __m256i tmp256 = _mm256_zextsi128_si256(tmp128);
                        acc256_0 = _mm256_xor_si256(acc256_0, tmp256);
                    }
                }
            }
        }

        uint8_t* r_i = res + i * lambda;
        __m128i out128 = _mm256_castsi256_si128(acc256_0);
        _mm_storeu_si128((__m128i*)r_i, out128);
    }
}


static void matrix_vector_product_f2_block_192(
    uint64_t nrows,
    uint64_t ncols,
    uint8_t* res,
    const uint8_t* A,
    const uint8_t* B
) {
    const uint64_t lambda = 24;
    memset(res, 0, nrows * lambda);

    uint64_t full_bytes = ncols / 8;
    uint64_t leftover_bits = ncols % 8;

    for (uint64_t i = 0; i < nrows; i++) {
        __m256i acc256_0 = _mm256_setzero_si256();
        __m256i acc256_1 = _mm256_setzero_si256();

        const uint8_t* A_row = A + (i * ncols) / 8;

        // Process full bytes
        for (uint64_t byte_i = 0; byte_i < full_bytes; byte_i++) {
            uint8_t a_byte = A_row[byte_i];
            if (a_byte == 0) continue;

            for (int bit = 0; bit < 8; bit++) {
                if (a_byte & (1 << bit)) {
                    uint64_t col = byte_i * 8 + bit;
                    const uint8_t* b_j = B + col * lambda;

                    // Load low 16 bytes of the block
                    __m128i low128 = _mm_loadu_si128((const __m128i*)b_j);
                    __m256i low256 = _mm256_zextsi128_si256(low128);

                    // Load the next 8 bytes (bytes 16..23)
                    __m128i high128 = _mm_setzero_si128();
                    high128 = _mm_loadl_epi64((const __m128i*)(b_j + 16)); 
                    // Now high128 contains only the 8 bytes needed, upper 8 bytes are zero.
                    __m256i high256 = _mm256_zextsi128_si256(high128);

                    acc256_0 = _mm256_xor_si256(acc256_0, low256);
                    acc256_1 = _mm256_xor_si256(acc256_1, high256);
                }
            }
        }

        // Process leftover bits
        if (leftover_bits > 0) {
            uint64_t start_col = full_bytes * 8;
            uint64_t bit_index = i * ncols + start_col;
            uint8_t a_last_byte = A[bit_index / 8];
            a_last_byte >>= (bit_index % 8); // This is zero since start_col is multiple of 8
            a_last_byte &= (uint8_t)((1 << leftover_bits) - 1);

            if (a_last_byte) {
                for (int bit = 0; bit < (int)leftover_bits; bit++) {
                    if (a_last_byte & (1 << bit)) {
                        uint64_t col = start_col + bit;
                        const uint8_t* b_j = B + col * lambda;

                        __m128i low128 = _mm_loadu_si128((const __m128i*)b_j);
                        __m256i low256 = _mm256_zextsi128_si256(low128);

                        __m128i high128 = _mm_setzero_si128();
                        high128 = _mm_loadl_epi64((const __m128i*)(b_j + 16));
                        __m256i high256 = _mm256_zextsi128_si256(high128);

                        acc256_0 = _mm256_xor_si256(acc256_0, low256);
                        acc256_1 = _mm256_xor_si256(acc256_1, high256);
                    }
                }
            }
        }

        // Store the result: 16 bytes from acc256_0 and 8 bytes from acc256_1
        uint8_t* r_i = res + i * lambda;

        __m128i out_low128 = _mm256_castsi256_si128(acc256_0);
        _mm_storeu_si128((__m128i*)r_i, out_low128);

        __m128i out_high128 = _mm256_castsi256_si128(acc256_1);
        // Extract the low 64 bits
        uint64_t lower64 = (uint64_t)_mm_cvtsi128_si64(out_high128);
        *(uint64_t*)(r_i + 16) = lower64;
    }
}


// Specialized function for lambda=256 bits (32 bytes)
static void matrix_vector_product_f2_block_256(
    uint64_t nrows,
    uint64_t ncols,
    uint8_t* res,
    const uint8_t* A,
    const uint8_t* B
) {
    const uint64_t lambda = 32;
    memset(res, 0, nrows * lambda);

    uint64_t full_bytes = ncols / 8;
    uint64_t leftover_bits = ncols % 8;

    for (uint64_t i = 0; i < nrows; i++) {
        __m256i acc256_0 = _mm256_setzero_si256();

        const uint8_t* A_row = A + (i * ncols) / 8;
        for (uint64_t byte_i = 0; byte_i < full_bytes; byte_i++) {
            uint8_t a_byte = A_row[byte_i];
            if (a_byte == 0)
                continue;
            for (int bit = 0; bit < 8; bit++) {
                if (a_byte & (1 << bit)) {
                    uint64_t col = byte_i * 8 + bit;
                    const uint8_t* b_j = B + col * lambda;
                    __m256i tmp256 = _mm256_loadu_si256((const __m256i*)b_j);
                    acc256_0 = _mm256_xor_si256(acc256_0, tmp256);
                }
            }
        }

        if (leftover_bits > 0) {
            uint64_t start_col = full_bytes * 8;
            uint64_t bit_index = i * ncols + start_col;
            uint8_t a_last_byte = A[bit_index / 8];
            a_last_byte >>= (bit_index % 8);
            a_last_byte &= (uint8_t)((1 << leftover_bits) - 1);
            if (a_last_byte) {
                for (int bit = 0; bit < (int)leftover_bits; bit++) {
                    if (a_last_byte & (1 << bit)) {
                        uint64_t col = start_col + bit;
                        const uint8_t* b_j = B + col * lambda;
                        __m256i tmp256 = _mm256_loadu_si256((const __m256i*)b_j);
                        acc256_0 = _mm256_xor_si256(acc256_0, tmp256);
                    }
                }
            }
        }

        uint8_t* r_i = res + i * lambda;
        _mm256_storeu_si256((__m256i*)r_i, acc256_0);
    }
}

// Dispatcher function
void matrix_f2_times_vector_flambda_avx2(
    uint64_t lambda_bits, uint64_t nrows, uint64_t ncols,  //
    flambda_t* res,                                   // vector of size nrows. res = a*b
    const bitmat_t* a,                                // nrows x ncols matrix
    const flambda_t* b
) {
    uint64_t lambda = (uint64_t)lambda_bits >> 3; // bytes
    switch (lambda) {
        case 16:
            matrix_vector_product_f2_block_128(nrows, ncols, res, a, b);
            break;
        case 24:
            matrix_vector_product_f2_block_192(nrows, ncols, res, a, b);
            break;
        case 32:
            matrix_vector_product_f2_block_256(nrows, ncols, res, a, b);
            break;
        default:
            fprintf(stderr, "Unsupported lambda size\n");
            break;
    }
}

