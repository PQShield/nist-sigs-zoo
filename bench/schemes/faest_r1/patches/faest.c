/* PATCH (nist-sigs-zoo, fix): copy of the submitted FAEST v1.0 (NIST round 1) AVX2
 * faest.c, used for FAEST-256s/256f only, with one fix in faest_compute_witness().
 *
 * Defect: key generation rejects secret keys for which an S-box input in the AES key
 * schedule is zero. It recovers each key-schedule S-box output as w[k] ^ w[k-Nk] (^ Rcon),
 * but takes w[k-Nk] to be the previously extracted word, which is OWF_KEY_SCHEDULE_PERIOD
 * bytes back. That is right for AES-128 (period 16 bytes = Nk 4 words) and AES-192 (24 =
 * 6), but AES-256 extracts a word every 16 bytes while Nk = 8: from the second extracted
 * word on, the check uses w[k-4] instead of w[k-8]. Key generation then accepts some keys
 * that the specification (and the submitted reference implementation) rejects, and
 * rejects some valid ones; signatures under an accepted invalid key never verify (the
 * bench run fails on them) and the generated KATs differ from the submitted ones.
 *
 * Fix: read w[k-Nk] from the expanded key, SECURITY_PARAM / 8 bytes before w[k]. For
 * AES-128/192 this is the same word as before. Nothing else is changed. With it, key
 * validity agrees with the reference implementation and the build reproduces the
 * submitted FAEST-256s/256f KATs. */
#include "faest.h"
#include "faest_details.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdlib.h>
#include "hash.h"
#include "owf_proof.h"
#include "small_vole.h"
#include "vole_commit.h"
#include "util.h"


bool faest_unpack_secret_key(secret_key* unpacked, const uint8_t* packed)
{
	memcpy(&unpacked->pk.owf_input, packed, sizeof(unpacked->pk.owf_input));
	memcpy(&unpacked->sk, packed + sizeof(unpacked->pk.owf_input), sizeof(unpacked->sk));

#if defined(OWF_AES_CTR)
	aes_keygen(&unpacked->round_keys, unpacked->sk);
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR)
	rijndael_keygen(&unpacked->pk.fixed_key, unpacked->pk.owf_input[0]);
#endif

	return faest_compute_witness(unpacked);
}

void faest_pack_public_key(uint8_t* packed, const public_key* unpacked)
{
	memcpy(packed, &unpacked->owf_input, sizeof(unpacked->owf_input));
	memcpy(packed + sizeof(unpacked->owf_input), &unpacked->owf_output[0], sizeof(unpacked->owf_output));
}

void faest_unpack_public_key(public_key* unpacked, const uint8_t* packed)
{
	memcpy(&unpacked->owf_input, packed, sizeof(unpacked->owf_input));
	memcpy(&unpacked->owf_output[0], packed + sizeof(unpacked->owf_input), sizeof(unpacked->owf_output));
#if defined(OWF_RIJNDAEL_EVEN_MANSOUR)
	rijndael_keygen(&unpacked->fixed_key, unpacked->owf_input[0]);
#endif
}

bool faest_compute_witness(secret_key* sk)
{
	uint8_t* w_ptr = (uint8_t*) &sk->witness;

	memcpy(w_ptr, &sk->sk, sizeof(sk->sk));
	w_ptr += sizeof(sk->sk);

#if defined(OWF_AES_CTR)
	// Extract witness for key schedule.
	for (size_t i = SECURITY_PARAM / 8; i < OWF_BLOCK_SIZE * (OWF_ROUNDS + 1);
	     i += OWF_KEY_SCHEDULE_PERIOD, w_ptr += 4)
	{
		uint32_t word;
		memcpy(&word, ((uint8_t*) &sk->round_keys.keys[0]) + i, 4);
		memcpy(w_ptr, &word, 4);

		// PATCH: w[k - Nk], Nk = SECURITY_PARAM / 32 words (was: the previous extracted word).
		uint32_t prev_word;
		memcpy(&prev_word, ((uint8_t*) &sk->round_keys.keys[0]) + i - SECURITY_PARAM / 8, 4);

		uint32_t sbox_output = word ^ prev_word;
		if (SECURITY_PARAM != 256 || i % (SECURITY_PARAM / 8) == 0)
			sbox_output ^= aes_round_constants[i / (SECURITY_PARAM / 8) - 1];

		// https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
		sbox_output ^= 0x63636363; // AES SBox maps 0 to 0x63.
		if ((sbox_output - 0x01010101) & ~sbox_output & 0x80808080)
			return false;
	}
#endif

#if defined(OWF_AES_CTR)
	for (uint32_t i = 0; i < OWF_BLOCKS; ++i)
		sk->pk.owf_output[i] =
			owf_block_xor(sk->round_keys.keys[0], sk->pk.owf_input[i]);
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR)
	static_assert(OWF_BLOCKS == 1, "");
	sk->pk.owf_output[0] = owf_block_xor(sk->pk.fixed_key.keys[0], sk->sk);
#endif

	for (unsigned int round = 1; round <= OWF_ROUNDS; ++round)
	{
		for (uint32_t i = 0; i < OWF_BLOCKS; ++i)
		{
			// The block is about to go into the SBox, so check for zeros.
			if (owf_block_any_zeros(sk->pk.owf_output[i]))
				return false;

			owf_block after_sbox;
#if defined(OWF_AES_CTR)
			aes_round_function(&sk->round_keys, &sk->pk.owf_output[i], &after_sbox, round);
#elif defined(OWF_RIJNDAEL_EVEN_MANSOUR)
#if SECURITY_PARAM == 128
			aes_round_function(&sk->pk.fixed_key, &sk->pk.owf_output[i], &after_sbox, round);
#elif SECURITY_PARAM == 192
			rijndael192_round_function(&sk->pk.fixed_key, &sk->pk.owf_output[i], &after_sbox, round);
#elif SECURITY_PARAM == 256
			rijndael256_round_function(&sk->pk.fixed_key, &sk->pk.owf_output[i], &after_sbox, round);
#endif
#endif

			if (round < OWF_ROUNDS)
				memcpy(w_ptr + i * sizeof(owf_block) * (OWF_ROUNDS - 1), &after_sbox, sizeof(owf_block));
		}

		if (round < OWF_ROUNDS)
			w_ptr += sizeof(owf_block);
	}

	w_ptr += (OWF_BLOCKS - 1) * sizeof(owf_block) * (OWF_ROUNDS - 1);
	assert(w_ptr - (uint8_t*) &sk->witness == WITNESS_BITS / 8);
	memset(w_ptr, 0, sizeof(sk->witness) - WITNESS_BITS / 8);

#if defined(OWF_RIJNDAEL_EVEN_MANSOUR)
	for (uint32_t i = 0; i < OWF_BLOCKS; ++i)
		sk->pk.owf_output[i] = owf_block_xor(sk->pk.owf_output[i], sk->sk);
#endif

	return true;
}

bool faest_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed, secret_key* sk)
{
	if (!faest_unpack_secret_key(sk, sk_packed))
		return false;

	faest_pack_public_key(pk_packed, &sk->pk);
	return true;
}

bool faest_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed)
{
	secret_key sk;
	return faest_unpack_sk_and_get_pubkey(pk_packed, sk_packed, &sk);
}

bool faest_sign(
	uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
	const uint8_t* random_seed, size_t random_seed_len)
{
	secret_key sk;
	uint8_t pk_packed[FAEST_PUBLIC_KEY_BYTES];
	if (!faest_unpack_sk_and_get_pubkey(pk_packed, sk_packed, &sk))
		return false;

	// TODO: Do we need to domain separate by the faest parameters?

	block_2secpar mu;
	hash_state hasher;
	hash_init(&hasher);
	hash_update(&hasher, pk_packed, FAEST_PUBLIC_KEY_BYTES);
	hash_update(&hasher, msg, msg_len);
	hash_update_byte(&hasher, 1);
	hash_final(&hasher, &mu, sizeof(mu));

	block_secpar seed;
	block128 iv;
	uint8_t seed_iv[sizeof(seed) + sizeof(iv)];
	hash_init(&hasher);
	hash_update(&hasher, &sk.sk, sizeof(sk.sk));
	hash_update(&hasher, &mu, sizeof(mu));
	if (random_seed)
		hash_update(&hasher, random_seed, random_seed_len);
	hash_update_byte(&hasher, 3);
	hash_final(&hasher, seed_iv, sizeof(seed_iv));

	memcpy(&seed, seed_iv, sizeof(seed));
	memcpy(&iv, &seed_iv[sizeof(seed)], sizeof(iv));

	block_secpar* forest =
		aligned_alloc(alignof(block_secpar), VECTOR_COMMIT_NODES * sizeof(block_secpar));
	block_2secpar* hashed_leaves =
		aligned_alloc(alignof(block_2secpar), VECTOR_COMMIT_LEAVES * sizeof(block_2secpar));
	vole_block* u =
		aligned_alloc(alignof(vole_block), VOLE_COL_BLOCKS * sizeof(vole_block));
	vole_block* v =
		aligned_alloc(alignof(vole_block), SECURITY_PARAM * VOLE_COL_BLOCKS * sizeof(vole_block));
	uint8_t vole_commit_check[VOLE_COMMIT_CHECK_SIZE];

	vole_commit(seed, iv, forest, hashed_leaves, u, v, signature, vole_commit_check);

	uint8_t chal1[VOLE_CHECK_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, &mu, sizeof(mu));
	hash_update(&hasher, vole_commit_check, VOLE_COMMIT_CHECK_SIZE);
	hash_update(&hasher, signature, VOLE_COMMIT_SIZE);
	hash_update(&hasher, &iv, sizeof(iv));
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal1[0], sizeof(chal1));

	uint8_t* vole_check_proof = signature + VOLE_COMMIT_SIZE;
	uint8_t vole_check_check[VOLE_CHECK_CHECK_BYTES];
	vole_check_sender(u, v, chal1, vole_check_proof, vole_check_check);

	uint8_t* correction = vole_check_proof + VOLE_CHECK_PROOF_BYTES;
	size_t remainder = (WITNESS_BITS / 8) % (16 * VOLE_BLOCK);
	for (size_t i = 0; i < WITNESS_BLOCKS - (remainder != 0); ++i)
	{
		vole_block correction_i = vole_block_xor(u[i], sk.witness[i]);
		memcpy(correction + i * sizeof(vole_block), &correction_i, sizeof(vole_block));
	}
	if (remainder)
	{
		vole_block correction_i = vole_block_xor(u[WITNESS_BLOCKS - 1], sk.witness[WITNESS_BLOCKS - 1]);
		memcpy(correction + (WITNESS_BLOCKS - 1) * sizeof(vole_block), &correction_i, remainder);
	}

	uint8_t chal2[QUICKSILVER_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, chal1, sizeof(chal1));
    hash_update(&hasher, vole_check_proof, VOLE_CHECK_PROOF_BYTES);
    hash_update(&hasher, vole_check_check, VOLE_CHECK_CHECK_BYTES);
    hash_update(&hasher, correction, WITNESS_BITS / 8);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal2[0], sizeof(chal2));

	block_secpar* macs =
		aligned_alloc(alignof(block_secpar), QUICKSILVER_ROWS_PADDED * sizeof(block_secpar));

	memcpy(&u[0], &sk.witness[0], WITNESS_BITS / 8);
	static_assert(QUICKSILVER_ROWS_PADDED % TRANSPOSE_BITS_ROWS == 0, "");
	transpose_secpar(v, macs, VOLE_COL_STRIDE, QUICKSILVER_ROWS_PADDED);
	free(v);

	quicksilver_state qs;
	quicksilver_init_prover(&qs, (uint8_t*) &u[0], macs, OWF_NUM_CONSTRAINTS, chal2);
	owf_constraints_prover(&qs, &sk.pk);

	uint8_t* qs_proof = correction + WITNESS_BITS / 8;
	uint8_t qs_check[QUICKSILVER_CHECK_BYTES];
	quicksilver_prove(&qs, WITNESS_BITS, qs_proof, qs_check);
	free(macs);
	free(u);

	uint8_t* veccom_open_start = qs_proof + QUICKSILVER_PROOF_BYTES;
	uint8_t* delta = veccom_open_start + VECTOR_OPEN_SIZE;
	hash_init(&hasher);
	hash_update(&hasher, &chal2, sizeof(chal2));
	hash_update(&hasher, qs_proof, QUICKSILVER_PROOF_BYTES);
	hash_update(&hasher, qs_check, QUICKSILVER_CHECK_BYTES);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, delta, sizeof(block_secpar));

	uint8_t delta_bytes[SECURITY_PARAM];
	for (size_t i = 0; i < SECURITY_PARAM; ++i)
		delta_bytes[i] = expand_bit_to_byte(delta[i / 8], i % 8);

	vector_open(forest, hashed_leaves, delta_bytes, veccom_open_start);
	free(forest);
	free(hashed_leaves);

	uint8_t* iv_dst = delta + sizeof(block_secpar);
	memcpy(iv_dst, &iv, sizeof(iv));

	assert(iv_dst + sizeof(iv) == signature + FAEST_SIGNATURE_BYTES);

	return true;
}

bool faest_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len,
                  const uint8_t* pk_packed)
{
	block_2secpar mu;
	hash_state hasher;
	hash_init(&hasher);
	hash_update(&hasher, pk_packed, FAEST_PUBLIC_KEY_BYTES);
	hash_update(&hasher, msg, msg_len);
	hash_update_byte(&hasher, 1);
	hash_final(&hasher, &mu, sizeof(mu));

	const uint8_t* vole_check_proof = signature + VOLE_COMMIT_SIZE;
	const uint8_t* correction = vole_check_proof + VOLE_CHECK_PROOF_BYTES;
	const uint8_t* qs_proof = correction + WITNESS_BITS / 8;
	const uint8_t* veccom_open_start = qs_proof + QUICKSILVER_PROOF_BYTES;
	const uint8_t* delta = veccom_open_start + VECTOR_OPEN_SIZE;
	const uint8_t* iv_ptr = delta + sizeof(block_secpar);

	uint8_t delta_bytes[SECURITY_PARAM];
	for (size_t i = 0; i < SECURITY_PARAM; ++i)
		delta_bytes[i] = expand_bit_to_byte(delta[i / 8], i % 8);

	vole_block* q =
		aligned_alloc(alignof(vole_block), SECURITY_PARAM * VOLE_COL_BLOCKS * sizeof(vole_block));
	uint8_t vole_commit_check[VOLE_COMMIT_CHECK_SIZE];

	block128 iv;
	memcpy(&iv, iv_ptr, sizeof(iv));
	vole_reconstruct(iv, q, delta_bytes, signature, veccom_open_start, vole_commit_check);

	uint8_t chal1[VOLE_CHECK_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, &mu, sizeof(mu));
	hash_update(&hasher, vole_commit_check, VOLE_COMMIT_CHECK_SIZE);
	hash_update(&hasher, signature, VOLE_COMMIT_SIZE);
	hash_update(&hasher, &iv, sizeof(iv));
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal1[0], sizeof(chal1));

	uint8_t vole_check_check[VOLE_CHECK_CHECK_BYTES];
	vole_check_receiver(q, delta_bytes, chal1, vole_check_proof, vole_check_check);

	uint8_t chal2[QUICKSILVER_CHALLENGE_BYTES];
	hash_init(&hasher);
	hash_update(&hasher, &chal1, sizeof(chal1));
	hash_update(&hasher, vole_check_proof, VOLE_CHECK_PROOF_BYTES);
	hash_update(&hasher, vole_check_check, VOLE_CHECK_CHECK_BYTES);
	hash_update(&hasher, correction, WITNESS_BITS / 8);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &chal2[0], sizeof(chal2));

	vole_block correction_blocks[WITNESS_BLOCKS];
	memcpy(&correction_blocks, correction, WITNESS_BITS / 8);
	memset(((uint8_t*) &correction_blocks) + WITNESS_BITS / 8, 0,
	       sizeof(correction_blocks) - WITNESS_BITS / 8);
	vole_receiver_apply_correction(WITNESS_BLOCKS, SECURITY_PARAM, correction_blocks, q, delta_bytes);

	block_secpar* macs =
		aligned_alloc(alignof(block_secpar), VOLE_ROWS_PADDED * sizeof(block_secpar));
	transpose_secpar(q, macs, VOLE_COL_STRIDE, QUICKSILVER_ROWS_PADDED);
	free(q);

	block_secpar delta_block;
	memcpy(&delta_block, delta, sizeof(delta_block));

	public_key pk;
	faest_unpack_public_key(&pk, pk_packed);

	quicksilver_state qs;
	quicksilver_init_verifier(&qs, macs, OWF_NUM_CONSTRAINTS, delta_block, chal2);
	owf_constraints_verifier(&qs, &pk);

	uint8_t qs_check[QUICKSILVER_CHECK_BYTES];
	quicksilver_verify(&qs, WITNESS_BITS, qs_proof, qs_check);
	free(macs);

	block_secpar delta_check;
	hash_init(&hasher);
	hash_update(&hasher, &chal2, sizeof(chal2));
	hash_update(&hasher, qs_proof, QUICKSILVER_PROOF_BYTES);
	hash_update(&hasher, qs_check, QUICKSILVER_CHECK_BYTES);
	hash_update_byte(&hasher, 2);
	hash_final(&hasher, &delta_check, sizeof(delta_check));

	return memcmp(delta, &delta_check, sizeof(delta_check)) == 0;
}
