/* PATCH (nist-sigs-zoo): the AVX2 SHAKE parameter headers define SPX_BYTES without the 2-byte TFORS length field
 * that the signer writes (its own signature_length_from_tfors() adds it), so CRYPTO_BYTES and sig_get_sn_len_bytes()
 * were 2 bytes below the maximum signature length; see docs/ngcc-submission-bugs.md item 3. CRYPTO_BYTES is used
 * only in this file, so this copy redefines it as SPX_BYTES + 2, the value of the reference and SM3 headers.
 * Nothing else is changed. Identical for all 10 AVX2 SHAKE instances (the submitted files are identical). */
/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "api.h"
#include "counter.h"
#include "params.h"
#include "gwots.h"
#include "tfors.h"
#include "octopus.h"
#include "hash.h"
#include "thash.h"
#include "address.h"
#include "randombytes.h"
#include "utils.h"
#include "merkle.h"

/* PATCH: the signature length including the 2-byte TFORS length field (see header). */
#undef CRYPTO_BYTES
#define CRYPTO_BYTES (SPX_BYTES + 2)

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

#define SIG_SUCCESS 0
#define SIG_INVALID_SIGNATURE -1
#define SIG_INVALID_ARGUMENT -2
#define SIG_RANDOM_FAILED -3
#define SIG_CRYPTO_FAILED -4

static unsigned char *alloc_message_buffer(const unsigned char *m, size_t mlen)
{
    unsigned char *mtmp;

    if (mlen > SIZE_MAX - COUNTER_SIZE) {
        return NULL;
    }

    mtmp = malloc(mlen + COUNTER_SIZE);
    if (mtmp == NULL) {
        return NULL;
    }

    memcpy(mtmp, m, mlen);
    return mtmp;
}

static int signature_length_from_tfors(size_t tfors_siglen, size_t *siglen)
{
    const size_t gwots_layer_len = SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N + COUNTER_SIZE;
    const size_t fixed_siglen = SPX_N + COUNTER_SIZE + 2 + (size_t)SPX_D * gwots_layer_len;

    if (tfors_siglen > SIZE_MAX - fixed_siglen) {
        return -1;
    }

    *siglen = fixed_siglen + tfors_siglen;
    return 0;
}

static int tfors_siglen_is_valid(size_t tfors_siglen)
{
    const size_t tfors_sig_min = (size_t)SPX_TFORS_K * SPX_N;

    return tfors_siglen >= tfors_sig_min && tfors_siglen <= SPX_TFORS_SIG_MAX;
}

/*
 * Returns the length of a secret key, in bytes
 */
unsigned long long crypto_sign_secretkeybytes(void)
{
    return CRYPTO_SECRETKEYBYTES;
}

/*
 * Returns the length of a public key, in bytes
 */
unsigned long long crypto_sign_publickeybytes(void)
{
    return CRYPTO_PUBLICKEYBYTES;
}

/*
 * Returns the length of a signature, in bytes
 */
unsigned long long crypto_sign_bytes(void)
{
    return CRYPTO_BYTES;
}

/*
 * Returns the length of the seed required to generate a key pair, in bytes
 */
unsigned long long crypto_sign_seedbytes(void)
{
    return CRYPTO_SEEDBYTES;
}

/*
 * Generates an PH key pair given a seed of length
 * Format sk: [SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [PUB_SEED || root]
 */
int crypto_sign_seed_keypair(unsigned char *pk, unsigned char *sk,
                             const unsigned char *seed)
{
    spx_ctx ctx;

    /* Initialize SK_SEED, SK_PRF and PUB_SEED from seed. */
    memcpy(sk, seed, CRYPTO_SEEDBYTES);

    memcpy(pk, sk + 2*SPX_N, SPX_N);

    memcpy(ctx.pub_seed, pk, SPX_N);
    memcpy(ctx.sk_seed, sk, SPX_N);

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    initialize_hash_function(&ctx);

    /* Compute root node of the top-most subtree. */
    merkle_gen_root(sk + 3*SPX_N, &ctx);

    memcpy(pk + SPX_N, sk + 3*SPX_N, SPX_N);

    return 0;
}

/*
 * Generates an PH key pair.
 * Format sk: [SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [PUB_SEED || root]
 */
int crypto_sign_keypair(unsigned char *pk, unsigned char *sk)
{
  unsigned char seed[CRYPTO_SEEDBYTES];
  randombytes(seed, CRYPTO_SEEDBYTES);
  crypto_sign_seed_keypair(pk, sk, seed);

  return 0;
}

/**
 * Returns an array containing a detached signature.
 */
int crypto_sign_signature(uint8_t *sig, size_t *siglen, size_t *tfslen,
                          const uint8_t *msg, size_t msglen, const uint8_t *sk)
{
    spx_ctx ctx;

    const unsigned char *sk_prf = sk + SPX_N;
    const unsigned char *pk = sk + 2*SPX_N;

    unsigned char optrand[SPX_N];
    unsigned char msghash[SPX_TFORS_MSG_BYTES];
    unsigned char root[SPX_TFORS_PK_BYTES];
    unsigned char *msgtmp;
    uint32_t i;
    uint64_t tree;
    uint32_t idx_leaf;
    uint32_t counter = 0;
    uint32_t tfors_auth_count;
    size_t tfors_siglen;
    size_t tfors_sig_max = SPX_TFORS_SIG_MAX;
    uint32_t indices[SPX_TFORS_K];
    uint32_t gwots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};

    memcpy(ctx.sk_seed, sk, SPX_N);
    memcpy(ctx.pub_seed, pk, SPX_N);

    initialize_hash_function(&ctx);

    set_addr_type(gwots_addr, SPX_ADDR_TYPE_GWOTS);
    set_addr_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);

    uint8_t *sig_origin = sig; /* mark the start of the signature for later use */
    msgtmp = alloc_message_buffer(msg, msglen);
    if (msgtmp == NULL) {
        *siglen = 0;
        *tfslen = 0;
        return -1;
    }

    randombytes(optrand, SPX_N);
    gen_message_random(sig_origin, sk_prf, optrand, msg, msglen, &ctx);

    counter = 0;
    for (counter = 0; ; counter++) {
        ull_to_bytes(msgtmp + msglen, COUNTER_SIZE, counter);
        hash_message(msghash, &tree, &idx_leaf, sig_origin, pk,
                        msgtmp, msglen + COUNTER_SIZE, &ctx);

        message_to_indices(indices, msghash, &ctx);

        octopus_compute_auth_count(indices, SPX_TFORS_K, &tfors_auth_count);
        tfors_siglen = SPX_TFORS_K * SPX_N + tfors_auth_count * SPX_N;

        if (tfors_siglen <= tfors_sig_max) {
            break;
        }
        if (counter == UINT32_MAX) {
            free(msgtmp);
            *siglen = 0;
            *tfslen = 0;
            return -1;
        }
    }

    save_tfors_counter(counter, sig_origin);
    sig += SPX_N + COUNTER_SIZE;
    // size_t siglen_offset = SPX_TFORS_SIG_MAX - tfors_siglen;
    ull_to_bytes(sig, 2, tfors_siglen);
    sig += 2;

    set_tree_addr(gwots_addr, tree);
    set_keypair_addr(gwots_addr, idx_leaf);

    tfors_sign(sig, root, indices, &ctx, gwots_addr);
    sig += tfors_siglen;

    for (i = 0; i < SPX_D; i++) {
        set_layer_addr(tree_addr, i);
        set_tree_addr(tree_addr, tree);

        copy_tree_addr(gwots_addr, tree_addr);
        set_keypair_addr(gwots_addr, idx_leaf);

        uint32_t gwots_counter;
        merkle_sign(sig, root, &ctx, gwots_addr, tree_addr, idx_leaf, &gwots_counter);
        save_gwots_counter(gwots_counter, sig);
        sig += (SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N + COUNTER_SIZE);

        idx_leaf = (tree & ((1 << SPX_TREE_HEIGHT)-1));
        tree = tree >> SPX_TREE_HEIGHT;
    }

    if (signature_length_from_tfors(tfors_siglen, siglen) != 0) {
        free(msgtmp);
        *siglen = 0;
        *tfslen = 0;
        return -1;
    }
    *tfslen = tfors_siglen;

    free(msgtmp);

    return 0;
}

/**
 * Verifies a detached signature and message under a given public key.
 */
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *msg, size_t msglen, const uint8_t *pk)
{    
    spx_ctx ctx;
    const unsigned char *pub_root = pk + SPX_N;
    unsigned char msghash[SPX_TFORS_MSG_BYTES];
    unsigned char gwots_pk[SPX_GWOTS_BYTES];
    unsigned char root[SPX_TFORS_PK_BYTES];
    unsigned char leaf[SPX_N];
    unsigned char *msgtmp;
    unsigned int i;
    int ret;
    uint64_t tree;
    uint32_t idx_leaf;
    uint32_t counter;
    uint32_t tfors_auth_count;
    uint32_t indices[SPX_TFORS_K];
    uint32_t gwots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};
    uint32_t gwots_pk_addr[8] = {0};
    size_t tfors_siglen;
    size_t expected_siglen;
    size_t expected_tfors_siglen;

    if (siglen < SPX_N + COUNTER_SIZE + 2) {
        return -1;
    }

    memcpy(ctx.pub_seed, pk, SPX_N);

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    initialize_hash_function(&ctx);

    set_addr_type(gwots_addr, SPX_ADDR_TYPE_GWOTS);
    set_addr_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);
    set_addr_type(gwots_pk_addr, SPX_ADDR_TYPE_GWOTSPK);

    counter = get_tfors_counter(sig);
    msgtmp = alloc_message_buffer(msg, msglen);
    if (msgtmp == NULL) {
        return -1;
    }
    ull_to_bytes(msgtmp + msglen, COUNTER_SIZE, counter);

    /* Derive the message digest and leaf index from R || PK || M. */
    /* The additional SPX_N is a result of the hash domain separator. */
    hash_message(msghash, &tree, &idx_leaf, sig, pk, msgtmp, msglen + COUNTER_SIZE, &ctx);
    sig += (SPX_N + COUNTER_SIZE);

    /* Extract tfors signature length from signature */
    tfors_siglen = (size_t)bytes_to_ull(sig, 2);
    if (!tfors_siglen_is_valid(tfors_siglen) ||
        signature_length_from_tfors(tfors_siglen, &expected_siglen) != 0 ||
        siglen != expected_siglen) {
        free(msgtmp);
        return -1;
    }
    sig += 2;

    message_to_indices(indices, msghash, &ctx);
    octopus_compute_auth_count(indices, SPX_TFORS_K, &tfors_auth_count);
    expected_tfors_siglen = SPX_TFORS_K * SPX_N + (size_t)tfors_auth_count * SPX_N;
    if (tfors_siglen != expected_tfors_siglen) {
        free(msgtmp);
        return -1;
    }

    /* Layer correctly defaults to 0, so no need to set_layer_addr */
    set_tree_addr(gwots_addr, tree);
    set_keypair_addr(gwots_addr, idx_leaf);

    tfors_pk_from_sig(root, sig, msghash, &ctx, gwots_addr);
    
    /* Skip TFORS signature body */
    sig += tfors_siglen;

    /* For each subtree.. */
    for (i = 0; i < SPX_D; i++) {
        set_layer_addr(tree_addr, i);
        set_tree_addr(tree_addr, tree);

        copy_tree_addr(gwots_addr, tree_addr);
        set_keypair_addr(gwots_addr, idx_leaf);

        copy_keypair_addr(gwots_pk_addr, gwots_addr);

        uint32_t gwots_counter = get_gwots_counter(sig);
        /* The GWOTS public key is only correct if the signature was correct. */
        /* Initially, root is the TFORS pk, but on subsequent iterations it is
           the root of the subtree below the currently processed subtree. */
        gwots_pk_from_sig(gwots_pk, sig, root, &ctx, gwots_addr, gwots_counter);
        sig += SPX_GWOTS_BYTES;

        /* Compute the leaf node using the GWOTS public key. */
        thash(leaf, gwots_pk, SPX_GWOTS_LEN, &ctx, gwots_pk_addr);

        /* Compute the root node of this subtree. */
        compute_root(root, leaf, idx_leaf, 0, sig, SPX_TREE_HEIGHT,
                     &ctx, tree_addr);
        sig += SPX_TREE_HEIGHT * SPX_N + COUNTER_SIZE;

        /* Update the indices for the next layer. */
        idx_leaf = (tree & ((1 << SPX_TREE_HEIGHT)-1));
        tree = tree >> SPX_TREE_HEIGHT;
    }

    if (memcmp(root, pub_root, SPX_N)) {
        ret = -1;
    } else {
        ret = 0;
    }

    free(msgtmp);
    return ret;
}

/**
 * Returns an array containing the signature followed by the message.
 */
int crypto_sign(unsigned char *sm, unsigned long long *smlen, unsigned long long *slen,
                unsigned long long *tfslen, const unsigned char *msg, unsigned long long msglen,
                const unsigned char *sk)
{
    size_t siglen;
    size_t tfors_siglen;

    if (crypto_sign_signature(sm, &siglen, &tfors_siglen, msg, (size_t)msglen, sk) != 0) {
        *smlen = 0;
        *slen = 0;
        *tfslen = 0;
        return -1;
    }

    memmove(sm + siglen, msg, msglen);
    *smlen = siglen + msglen;   
    *slen = siglen;
    *tfslen = tfors_siglen;

    return 0;
}

/**
 * Verifies a given signature-message pair under a given public key.
 */
int crypto_sign_open(unsigned char *msg, unsigned long long *msglen, unsigned long long *slen, 
                        unsigned long long *tfslen, const unsigned char *sm, 
                        unsigned long long smlen, const unsigned char *pk)
{
    unsigned long long slen_tmp = *slen;
    unsigned long long msglen_tmp;

    (void)tfslen;

    if (slen_tmp > smlen) {
        *msglen = 0;
        return -1;
    }

    msglen_tmp = smlen - slen_tmp;

    if (crypto_sign_verify(sm, (size_t)slen_tmp, sm + slen_tmp, (size_t)msglen_tmp, pk)) {
        memset(msg, 0, (size_t)msglen_tmp);
        *msglen = 0;
        return -1;
    }

    *msglen = msglen_tmp;
    memmove(msg, sm + slen_tmp, *msglen);

    return 0;
}

#ifdef SUBMISSION_DRNG

unsigned long long sig_get_pk_len_bytes(void)
{
	return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes(void)
{
	return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes(void)
{
	return CRYPTO_BYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	unsigned char seed[CRYPTO_SEEDBYTES];

	if (pk == 0 || pk_len_bytes == 0 || sk == 0 || sk_len_bytes == 0)
	{
		return SIG_INVALID_ARGUMENT;
	}

	if (get_random_number(&drng_algorithm, seed, CRYPTO_SEEDBYTES * 8) != 0)
	{
		return SIG_RANDOM_FAILED;
	}

	if (crypto_sign_seed_keypair(pk, sk, seed) != 0)
	{
		return SIG_CRYPTO_FAILED;
	}

	*pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
	*sk_len_bytes = CRYPTO_SECRETKEYBYTES;
	return SIG_SUCCESS;
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	size_t siglen;
	size_t tfors_siglen;

	if (sk == 0 || m == 0 || sn == 0 || sn_len_bytes == 0)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (sk_len_bytes != CRYPTO_SECRETKEYBYTES)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (m_len_bytes > (unsigned long long)SIZE_MAX)
	{
		return SIG_INVALID_ARGUMENT;
	}

	if (crypto_sign_signature(sn, &siglen, &tfors_siglen, m, (size_t)m_len_bytes, sk) != 0)
	{
		return SIG_CRYPTO_FAILED;
	}

	(void)tfors_siglen;
	*sn_len_bytes = siglen;
	return SIG_SUCCESS;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	int ret;

	if (pk == 0 || sn == 0 || m == 0)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (sn_len_bytes > (unsigned long long)SIZE_MAX ||
		m_len_bytes > (unsigned long long)SIZE_MAX)
	{
		return SIG_INVALID_ARGUMENT;
	}

	ret = crypto_sign_verify(sn, (size_t)sn_len_bytes, m, (size_t)m_len_bytes, pk);
	if (ret == 0)
	{
		return SIG_SUCCESS;
	}
	if (ret == -1)
	{
		return SIG_INVALID_SIGNATURE;
	}
	return SIG_CRYPTO_FAILED;
}

#endif /* SUBMISSION_DRNG */
