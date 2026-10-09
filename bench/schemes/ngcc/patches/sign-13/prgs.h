/* PATCH (nist-sigs-zoo): in the SECURITY_PARAM == 512 branch, prg_*_gen copies keys into a malloc() buffer of
 * block_secpar = block512 (two __m256i, 32-byte alignment). malloc() only guarantees 16 bytes, so the aligned AVX2
 * stores fault (SIGSEGV in prg_tree_gen in the first sig_sign; see docs/ngcc-submission-bugs.md U10). This copy
 * allocates that buffer with aligned_alloc(_Alignof(block_secpar), ...), as the rest of the tree (faest.c,
 * vole_commit.c) does. Nothing else is changed. It is reached through patches/sign-13/vole_params.h, which the
 * wrap_*.c files in this directory include first (same PRGS_H guard), so the submitted prgs.h is skipped in those
 * translation units. Identical for 512f/512s. */
#ifndef PRGS_H
#define PRGS_H

#include "aes.h"
#include "hash.h"
#if SECURITY_PARAM == 512

#include <stdlib.h>

#define DEFINE_PRG_AES_CTR(name) \
	typedef struct { block_secpar key; block128 iv; } prg_##name##_key; \
	typedef block128 prg_##name##_iv; \
	typedef block128 prg_##name##_block; \
	typedef char prg_##name##_fixed_key; \
	static ALWAYS_INLINE void prg_##name##_shake( \
		const block_secpar* keys, const block128* ivs, size_t num_keys, \
		uint32_t num_blocks, uint32_t counter, block128* output) \
	{ \
		size_t n = num_keys ? num_keys : 1; \
		block128* domain_ivs = (block128*)malloc(n * sizeof(*domain_ivs)); \
		for (size_t i = 0; i < num_keys; i++) \
			domain_ivs[i] = block128_xor(ivs[i], block128_set_low32(counter)); \
		shake_prg(keys, domain_ivs, num_keys, num_blocks * sizeof(block128), (uint8_t*)output); \
		free(domain_ivs); \
	} \
	static ALWAYS_INLINE void prg_##name##_init( \
		prg_##name##_key* prgs, const prg_##name##_fixed_key* fixed_key, \
		const block_secpar* keys, const prg_##name##_iv* ivs, size_t num_keys, \
		uint32_t num_blocks, uint32_t counter, prg_##name##_block* output) \
	{ \
		(void)fixed_key; \
		for (size_t i = 0; i < num_keys; i++) { \
			prgs[i].key = keys[i]; \
			prgs[i].iv = ivs[i]; \
		} \
		prg_##name##_shake(keys, ivs, num_keys, num_blocks, counter, output); \
	} \
	static ALWAYS_INLINE void prg_##name##_gen( \
		const prg_##name##_key* prgs, const prg_##name##_fixed_key* fixed_key, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* output) \
	{ \
		(void)fixed_key; \
		size_t n = num_keys ? num_keys : 1; \
		block_secpar* keys = (block_secpar*)aligned_alloc(_Alignof(block_secpar), n * sizeof(*keys)); /* PATCH */ \
		block128* ivs = (block128*)malloc(n * sizeof(*ivs)); \
		for (size_t i = 0; i < num_keys; i++) { \
			keys[i] = prgs[i].key; \
			ivs[i] = prgs[i].iv; \
		} \
		prg_##name##_shake(keys, ivs, num_keys, num_blocks, counter, output); \
		free(keys); \
		free(ivs); \
	}

#else
#define DEFINE_PRG_AES_CTR(name) \
	typedef aes_round_keys prg_##name##_key; \
	typedef block128 prg_##name##_iv; \
	typedef block128 prg_##name##_block; \
	typedef char prg_##name##_fixed_key; \
 \
	static ALWAYS_INLINE void prg_##name##_init( \
		prg_##name##_key* restrict prgs, const prg_##name##_fixed_key* restrict fixed_key, \
		const block_secpar* restrict keys, const prg_##name##_iv* restrict ivs, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* restrict output) \
	{ \
		(void) fixed_key; \
		aes_keygen_ctr(prgs, keys, ivs, num_keys, num_blocks, counter, output); \
	} \
	static ALWAYS_INLINE void prg_##name##_gen( \
		const prg_##name##_key* restrict prgs, const prg_##name##_fixed_key* restrict fixed_key, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* restrict output) \
	{ \
		(void) fixed_key; \
		aes_ctr(prgs, num_keys, num_blocks, counter, output); \
	}
#endif

#define DEFINE_PRG_RIJNDAEL_FIXED_KEY_CTR(name) \
	typedef block_secpar prg_##name##_key; \
	typedef char prg_##name##_iv; \
	typedef block_secpar prg_##name##_block; \
	typedef rijndael_round_keys prg_##name##_fixed_key; \
 \
	static ALWAYS_INLINE void prg_##name##_init( \
		prg_##name##_key* restrict prgs, const prg_##name##_fixed_key* restrict fixed_key, \
		const block_secpar* restrict keys, const prg_##name##_iv* restrict ivs, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* restrict output) \
	{ \
		(void) ivs; \
		memcpy(prgs, keys, num_keys * sizeof(keys[0])); \
		rijndael_fixed_key_ctr(fixed_key, prgs, num_keys, num_blocks, counter, output); \
	} \
	static ALWAYS_INLINE void prg_##name##_gen( \
		const prg_##name##_key* restrict prgs, const prg_##name##_fixed_key* restrict fixed_key, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* restrict output) \
	{ \
		rijndael_fixed_key_ctr(fixed_key, prgs, num_keys, num_blocks, counter, output); \
	}

#define DEFINE_PRG_SHAKE(name) \
	typedef char prg_##name##_key; \
	typedef block128 prg_##name##_iv; \
	typedef block_secpar prg_##name##_block; \
	typedef char prg_##name##_fixed_key; \
	static ALWAYS_INLINE void prg_##name##_init( \
		prg_##name##_key* restrict prgs, const prg_##name##_fixed_key* restrict fixed_key, \
		const block_secpar* restrict keys, const prg_##name##_iv* restrict ivs, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* restrict output) \
	{ \
		(void) prgs; \
		(void) fixed_key; \
		(void) counter; \
		assert(counter == 0); \
		shake_prg(keys, ivs, num_keys, num_blocks * sizeof(block_secpar), (uint8_t*) output); \
	} \
	static ALWAYS_INLINE void prg_##name##_gen( \
		const prg_##name##_key* restrict prgs, const prg_##name##_fixed_key* restrict fixed_key, \
		size_t num_keys, uint32_t num_blocks, uint32_t counter, prg_##name##_block* restrict output) \
	{ \
 \
 \
		(void) prgs; \
		(void) fixed_key; \
		(void) counter; \
		(void) output; \
		(void) num_keys; \
		(void) num_blocks; \
		assert(num_keys == 0 || num_blocks == 0); \
	}

#if defined(PRG_AES_CTR)
#define PRG_VOLE_PREFERRED_WIDTH AES_PREFERRED_WIDTH
#define PRG_VOLE_PREFERRED_WIDTH_SHIFT AES_PREFERRED_WIDTH_SHIFT
DEFINE_PRG_AES_CTR(vole)
#elif defined(PRG_RIJNDAEL_EVEN_MANSOUR)
#define PRG_VOLE_PREFERRED_WIDTH FIXED_KEY_PREFERRED_WIDTH
#define PRG_VOLE_PREFERRED_WIDTH_SHIFT FIXED_KEY_PREFERRED_WIDTH_SHIFT
DEFINE_PRG_RIJNDAEL_FIXED_KEY_CTR(vole)
#endif

#if defined(TREE_PRG_AES_CTR)
#define PRG_TREE_PREFERRED_WIDTH AES_PREFERRED_WIDTH
#define PRG_TREE_PREFERRED_WIDTH_SHIFT AES_PREFERRED_WIDTH_SHIFT
DEFINE_PRG_AES_CTR(tree)
#elif defined(TREE_PRG_RIJNDAEL_EVEN_MANSOUR)
#define PRG_TREE_PREFERRED_WIDTH FIXED_KEY_PREFERRED_WIDTH
#define PRG_TREE_PREFERRED_WIDTH_SHIFT FIXED_KEY_PREFERRED_WIDTH_SHIFT
DEFINE_PRG_RIJNDAEL_FIXED_KEY_CTR(tree)
#endif

#if defined(LEAF_PRG_AES_CTR)
#define PRG_LEAF_PREFERRED_WIDTH AES_PREFERRED_WIDTH
#define PRG_LEAF_PREFERRED_WIDTH_SHIFT AES_PREFERRED_WIDTH_SHIFT
DEFINE_PRG_AES_CTR(leaf)
#elif defined(LEAF_PRG_RIJNDAEL_EVEN_MANSOUR)
#define PRG_LEAF_PREFERRED_WIDTH FIXED_KEY_PREFERRED_WIDTH
#define PRG_LEAF_PREFERRED_WIDTH_SHIFT FIXED_KEY_PREFERRED_WIDTH_SHIFT
DEFINE_PRG_RIJNDAEL_FIXED_KEY_CTR(leaf)
#elif defined(LEAF_PRG_SHAKE)
#define PRG_LEAF_PREFERRED_WIDTH (1 << PRG_LEAF_PREFERRED_WIDTH_SHIFT)
#define PRG_LEAF_PREFERRED_WIDTH_SHIFT 3
DEFINE_PRG_SHAKE(leaf)
#endif

#undef DEFINE_PRG_AES_CTR
#undef DEFINE_PRG_RIJNDAEL_FIXED_KEY_CTR

inline void init_fixed_keys(
	prg_tree_fixed_key* fixed_key_tree, prg_leaf_fixed_key* fixed_key_leaf,
	block_secpar iv)
{
	(void) fixed_key_tree, (void) fixed_key_leaf, (void) iv;
#if defined(TREE_PRG_RIJNDAEL_EVEN_MANSOUR)
	rijndael_keygen(fixed_key_tree, iv);
#endif
#if defined(LEAF_PRG_RIJNDAEL_EVEN_MANSOUR)
	rijndael_keygen(fixed_key_leaf, iv);
#endif
}


#endif
