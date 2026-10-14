/* PATCH (nist-sigs-zoo, fix): sig_verify copied the uninitialised stmp buffer over the aux part of the signature
 * (memcpy(sn+pos, stmp, ...)) instead of reading the aux part into stmp, so it decoded uninitialised memory and
 * overwrote the caller's signature. This copy reads memcpy(stmp, sn+pos, ...),
 * the inverse of sig_sign's packing. Nothing else is changed. */
/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_TINS512.h"
#include "drng.h"
#include "auxfunc.h"

#include <memory.h>
#include <stdio.h>

#include "params.h"
#include "ff_arith.h"
#include "bavc_commit.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long sig_get_pk_len_bytes()
{
	return PK_SIZE;
}

unsigned long long sig_get_sk_len_bytes()
{
	return SK_SIZE;
}

unsigned long long sig_get_sn_len_bytes()
{
	return SIG_SIZE;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	*pk_len_bytes = PK_SIZE;
	*sk_len_bytes = SK_SIZE;

	get_random_number(&drng_algorithm, pk, PK_SEEDLEN * 8);
	get_random_number(&drng_algorithm, sk, SK_SEEDLEN * 8);

	DRNG_ctx sk_expander;
	DRNG_ctx pk_expander;

	init_random_number(&pk_expander, pk, PK_SEEDLEN);
	init_random_number(&sk_expander, sk, SK_SEEDLEN);

	fe v[N_TUPLE], u[N_TUPLE];
	unsigned char alpha[N_TUPLE_SIZE], beta[N_TUPLE_SIZE];
	memset(v, 0, sizeof(fe) * N_TUPLE);
	memset(u, 0, sizeof(fe) * N_TUPLE);


	unsigned char u_buffer[(N_TUPLE*K+7) /8], v_buffer[((N_TUPLE - 1)*K+7) /8];
	get_random_number(&pk_expander, u_buffer, N_TUPLE*K);
	get_random_number(&pk_expander, v_buffer, (N_TUPLE - 1)*K);
	fe_decompress(u_buffer, u, N_TUPLE);
	fe_decompress(v_buffer, v, N_TUPLE-1);



	// unsigned char u_buffer[(N_TUPLE) * 8 * FE_BYTES], v_buffer[(N_TUPLE - 1) * 8 * FE_BYTES];
	// get_random_number(&pk_expander, u_buffer, (N_TUPLE) * 8 * FE_BYTES);
	// get_random_number(&pk_expander, v_buffer, (N_TUPLE - 1) * 8 * FE_BYTES);
	// fe_decompress(u_buffer, u, N_TUPLE);
	// fe_decompress(v_buffer, v, N_TUPLE-1);


restart:
	get_random_number(&sk_expander, alpha, N_TUPLE - 2);
	get_random_number(&sk_expander, beta, N_TUPLE - 2);

	fe tmp, sum[4];

	// sum[3] = (u, alpha) + u[n-2]
	fe_f2_inner(u, alpha, N_TUPLE - 2, sum[3]);
	fe_add(sum[3], u[N_TUPLE - 2], sum[3]);

	// test if the denominator == zero
	if (deg(sum[3]) == -1)
	{
		goto restart;
	}

	// sum[0] = (u, beta) + u[n-1]
	fe_f2_inner(u, beta, N_TUPLE - 2, sum[0]);
	fe_add(sum[0], u[N_TUPLE - 1], sum[0]);

	// sum[1] = (v, alpha) + v[n-2]
	fe_f2_inner(v, alpha, N_TUPLE - 2, sum[1]);
	fe_add(sum[1], v[N_TUPLE - 2], sum[1]);

	// sum[2] = (v, beta)
	fe_f2_inner(v, beta, N_TUPLE - 2, sum[2]);

	fe_inv(sum[3], v[N_TUPLE - 1]);

	fe_mul(sum[0], sum[1], tmp);
	fe_mul(v[N_TUPLE - 1], tmp, v[N_TUPLE - 1]);
	fe_add(v[N_TUPLE - 1], sum[2], v[N_TUPLE - 1]);

	// test if u==v or u==0, v==0;
	memset(tmp, 0, sizeof(fe));
	memset(sum, 0, sizeof(fe) * 4);

	for (int i = 0; i < N_TUPLE; i++)
	{
		fe_add(u[i], v[i], tmp);
		fe_or(sum[0], tmp, sum[0]);
		fe_or(sum[1], u[i], sum[1]);
		fe_or(sum[2], v[i], sum[2]);
	}
	if ((deg(sum[0]) == -1) || (deg(sum[1]) == -1) || (deg(sum[2]) == -1))
	{
		goto restart;
	}

	// // test the correctness of the generated key
	// fe_f2_inner(u, alpha, N_TUPLE-2, sum[0]);
	// fe_add(sum[0], u[N_TUPLE-2], sum[0]);
	// fe_f2_inner(v, beta, N_TUPLE-2, sum[1]);
	// fe_add(sum[1], v[N_TUPLE-1], sum[1]);
	// fe_f2_inner(u, beta, N_TUPLE-2, sum[2]);
	// fe_add(sum[2], u[N_TUPLE-1], sum[2]);
	// fe_f2_inner(v, alpha, N_TUPLE-2, sum[3]);
	// fe_add(sum[3], v[N_TUPLE-2], sum[3]);
	// fe_mul(sum[0], sum[1], sum[0]);
	// fe_mul(sum[2], sum[3], sum[2]);
	// fe_add(sum[0], sum[2], tmp);
	// if(deg(tmp)==-1){
	// 	printf("OK!!!\n\n");
	// }

	fe_compress(v+N_TUPLE-1, 1, pk+PK_SEEDLEN);

	// for(int j=0; j<EXT_DEGREE; j++){
	// 		printf("%02x,", v[N_TUPLE-1][j]);
	// 	}
	// 	printf("\n");

	memcpy(sk + SK_SEEDLEN, pk, PK_SEEDLEN);

	return 0;
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *msg, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{

	DRNG_ctx sk_expander;
	DRNG_ctx pk_expander;

	init_random_number(&pk_expander, sk + SK_SEEDLEN, PK_SEEDLEN);
	init_random_number(&sk_expander, sk, SK_SEEDLEN);

	fe v[N_TUPLE], u[N_TUPLE];

	memset(v, 0, sizeof(fe) * N_TUPLE);
	memset(u, 0, sizeof(fe) * N_TUPLE);

	unsigned char alpha[N_TUPLE_SIZE], beta[N_TUPLE_SIZE];

	unsigned char u_buffer[(N_TUPLE*K+7) /8], v_buffer[((N_TUPLE - 1)*K+7) /8];
	get_random_number(&pk_expander, u_buffer, N_TUPLE*K);
	get_random_number(&pk_expander, v_buffer, (N_TUPLE - 1)*K);
	fe_decompress(u_buffer, u, N_TUPLE);
	fe_decompress(v_buffer, v, N_TUPLE-1);



restart:
	get_random_number(&sk_expander, alpha, N_TUPLE - 2);
	get_random_number(&sk_expander, beta, N_TUPLE - 2);

	fe tmp, sum[4];

	// sum[3] = (u, alpha) + u[n-2]
	fe_f2_inner(u, alpha, N_TUPLE - 2, sum[3]);
	fe_add(sum[3], u[N_TUPLE - 2], sum[3]);

	// test if the denominator == zero
	if (deg(sum[3]) == -1)
	{
		goto restart;
	}

	// sum[0] = (u, beta) + u[n-1]
	fe_f2_inner(u, beta, N_TUPLE - 2, sum[0]);
	fe_add(sum[0], u[N_TUPLE - 1], sum[0]);

	// sum[1] = (v, alpha) + v[n-2]
	fe_f2_inner(v, alpha, N_TUPLE - 2, sum[1]);
	fe_add(sum[1], v[N_TUPLE - 2], sum[1]);

	// sum[2] = (v, beta)
	fe_f2_inner(v, beta, N_TUPLE - 2, sum[2]);

	fe_inv(sum[3], v[N_TUPLE - 1]);

	fe_mul(sum[0], sum[1], tmp);
	fe_mul(v[N_TUPLE - 1], tmp, v[N_TUPLE - 1]);
	fe_add(v[N_TUPLE - 1], sum[2], v[N_TUPLE - 1]);

	// test if u==v or u==0, v==0;
	memset(tmp, 0, sizeof(fe));
	memset(sum, 0, sizeof(fe) * 4);

	for (int i = 0; i < N_TUPLE; i++)
	{
		fe_add(u[i], v[i], tmp);
		fe_or(sum[0], tmp, sum[0]);
		fe_or(sum[1], u[i], sum[1]);
		fe_or(sum[2], v[i], sum[2]);
	}
	if ((deg(sum[0]) == -1) || (deg(sum[1]) == -1) || (deg(sum[2]) == -1))
	{
		goto restart;
	}

	unsigned char salt[SALT_SIZE], rseed[RSEED_SIZE];

	//commitment comms[TAU][N];
	commitment (*comms)[N] = calloc(TAU, sizeof(commitment[N]));
    if (!comms) {        
        return -1;
    }

	unsigned char aux[TAU][N_TUPLE_SIZE * 2];
	ff12b base[TAU][2 * N_TUPLE - 3];
	ff12b delta[TAU];
	hash_t h_sh, h_piop;
	fe p_mid[TAU], p_base[TAU];

	memset(p_base, 0, sizeof(fe) * TAU);
	memset(p_mid, 0, sizeof(fe) * TAU);
	memset(base, 0, sizeof(ff12b) * TAU * (2 * N_TUPLE - 3));
	memset(delta, 0, sizeof(ff12b) * TAU);

	get_random_number(&drng_algorithm, salt, SALT_SIZE * 8);
	get_random_number(&drng_algorithm, rseed, RSEED_SIZE * 8);

	//ggm_tree tree;
	node *tree = malloc((2 * N_LEAVES - 1) * sizeof(node));
    if (!tree){
		free(comms);
		return -1;
	}

	CommitPoly(salt, rseed, tree, comms, alpha, beta, aux, base, delta, h_sh);

	for (int e = 0; e < TAU; e++)
	{
		ComputePoly(alpha, beta, base[e], delta[e], u, v, p_mid[e], p_base[e]);
	}

	int NBytes = 1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes + sizeof(hash_t) + (K*2*TAU+K+7)/8;
	unsigned char *nonce = (unsigned char *)calloc(NBytes, sizeof(unsigned char));

	
	nonce[0] = 2;
	memcpy(nonce + 1, sk + SK_SEEDLEN, PK_SEEDLEN);
	memcpy(nonce + 1 + PK_SEEDLEN, salt, SALT_SIZE);
	memcpy(nonce + 1 + PK_SEEDLEN + SALT_SIZE, msg, m_len_bytes);
	memcpy(nonce + 1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes, h_sh, sizeof(hash_t));
	int off = 1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes + sizeof(hash_t);

	fe p_tmp[2*TAU+1];
	memcpy(p_tmp, p_mid, TAU*sizeof(fe));
	memcpy(p_tmp+TAU, p_base, TAU*sizeof(fe));
	memcpy(p_tmp+TAU*2, v+N_TUPLE-1, sizeof(fe));

	fe_compress(p_tmp, 2*TAU+1, nonce+off);

	// fe_compress(p_mid, TAU, nonce+off);
	// fe_compress(p_base, TAU, nonce+off+(K*TAU)/8);
	// fe_compress(v+N_TUPLE-1, 1, nonce+off + (K*TAU*2)/8);

	pseudohash(sizeof(hash_t)*8, nonce, (1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes + sizeof(hash_t))*8+K*2*TAU+K, h_piop);

	if (nonce != NULL)
	{
		free(nonce);
		nonce = NULL;
	}

	long long ctr;
	int path_size;
	node path[T_OPEN];
	commitment proof[TAU];
	OpenRandomEva(tree, comms, h_piop, &ctr, path, &path_size, proof);

	// BAVC=path[path_size] || proof
	// sn = salt || ctr || h_piop || BAVC  || p_mid || aux
	int pos = 0;
	memcpy(sn+pos, salt, SALT_SIZE);
	pos+=SALT_SIZE;
	memcpy(sn + pos, &ctr, sizeof(long long));
	pos+=sizeof(long long);
	memcpy(sn + pos, h_piop, sizeof(hash_t));
	pos+=sizeof(hash_t);

	memcpy(sn + pos, path, NODE_SIZE * path_size);
	pos+=NODE_SIZE * path_size;
	memcpy(sn + pos, proof, sizeof(commitment) * TAU);
	pos+=sizeof(commitment) * TAU;

	fe_compress(p_mid, TAU, sn + pos);
	pos+= (TAU*K+7)/8;

	unsigned char stmp[(TAU*2*(N_TUPLE-2)+7)/8];
	compress_aux(aux, TAU, stmp);

	sn[pos-1] ^= stmp[(TAU*2*(N_TUPLE-2)+7)/8-1] >> 4;
	memcpy(sn+pos, stmp, (TAU*2*(N_TUPLE-2)+7)/8-1);

	
	*sn_len_bytes = pos + ((N_TUPLE-2)*TAU*2+7)/8-1;

	free(tree);
	free(comms);
	return 0;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{

	DRNG_ctx pk_expander;

	unsigned char salt[SALT_SIZE], rseed[RSEED_SIZE];

	
	unsigned char aux[TAU][N_TUPLE_SIZE * 2];
	ff12b base[TAU][2 * N_TUPLE - 3];
	ff12b delta[TAU];
	hash_t h_sh, h_piop;
	fe p_mid[TAU], p_base[TAU];

	long long ctr;
	int path_size;
	node path[T_OPEN];
	commitment proof[TAU];

	int NTemp = SALT_SIZE + sizeof(long long) + sizeof(hash_t) + sizeof(commitment) * TAU + ((N_TUPLE-2)*TAU*2+ K * TAU+7)/8 ;

	path_size = (sn_len_bytes - NTemp) / NODE_SIZE;

	int pos = 0;
	memcpy(salt, sn, SALT_SIZE);
	pos+=SALT_SIZE;
	memcpy(&ctr, sn + pos, sizeof(long long));
	pos += sizeof(long long);
	memcpy(h_piop, sn + pos, sizeof(hash_t));
	pos += sizeof(hash_t);
	memcpy(path, sn + pos, NODE_SIZE * path_size);
	pos += NODE_SIZE * path_size;
	memcpy(proof, sn + pos, sizeof(commitment) * TAU);
	pos += sizeof(commitment) * TAU;
	fe_decompress(sn + pos, p_mid, TAU);
	pos += (K*TAU+7)/8;


	unsigned char stmp[(TAU*2*(N_TUPLE-2)+7)/8];
	stmp[(TAU*2*(N_TUPLE-2)+7)/8-1] = (sn[pos-1] & 0xf)<<4;
	memcpy(stmp, sn+pos, (TAU*2*(N_TUPLE-2)+7)/8-1);
	decompress_aux(stmp, aux, TAU);


	fe v[N_TUPLE], u[N_TUPLE];
	memset(v, 0, sizeof(fe) * N_TUPLE);
	memset(u, 0, sizeof(fe) * N_TUPLE);


	
	init_random_number(&pk_expander, pk, PK_SEEDLEN);

	unsigned char u_buffer[(N_TUPLE*K+7) /8], v_buffer[((N_TUPLE - 1)*K+7) /8];
	get_random_number(&pk_expander, u_buffer, N_TUPLE*K);
	get_random_number(&pk_expander, v_buffer, (N_TUPLE - 1)*K);
	fe_decompress(u_buffer, u, N_TUPLE);
	fe_decompress(v_buffer, v, N_TUPLE-1);


	fe_decompress(pk + PK_SEEDLEN, v+N_TUPLE-1, 1);


	unsigned char v_grinding;
	int points[TAU];
	ff12b evals[TAU][2 * N_TUPLE - 3];

	memset(p_base, 0, sizeof(fe) * TAU);

	memset(evals, 0, sizeof(ff12b) * TAU * (2 * N_TUPLE - 3));

	if (-1 == ComputeEva(salt, ctr, h_piop, path, path_size, proof, aux, &v_grinding, points, evals, h_sh))
	{
		printf("ComputeEva Wrong\n");
		return -1;
	}
	for (int e = 0; e < TAU; e++)
	{
		RecomputePolyProof(points[e], evals[e], v, u, p_mid[e], p_base[e]);
	}

	hash_t test_h_piop;

	int NBytes = 1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes + sizeof(hash_t) + (K*2*TAU+K+7)/8;
	unsigned char *nonce = (unsigned char *)calloc(NBytes, sizeof(unsigned char));

	nonce[0] = 2;
	memcpy(nonce + 1, pk, PK_SEEDLEN);
	memcpy(nonce + 1 + PK_SEEDLEN, salt, SALT_SIZE);
	memcpy(nonce + 1 + PK_SEEDLEN + SALT_SIZE, m, m_len_bytes);
	memcpy(nonce + 1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes, h_sh, sizeof(hash_t));
	int off = 1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes + sizeof(hash_t);

	fe p_tmp[2*TAU+1];
	memcpy(p_tmp, p_mid, TAU*sizeof(fe));
	memcpy(p_tmp+TAU, p_base, TAU*sizeof(fe));
	memcpy(p_tmp+TAU*2, v+N_TUPLE-1, sizeof(fe));
	fe_compress(p_tmp, 2*TAU+1, nonce+off);

	pseudohash(sizeof(hash_t)*8, nonce, (1 + PK_SEEDLEN + SALT_SIZE + m_len_bytes + sizeof(hash_t))*8+K*2*TAU+K, test_h_piop);





	unsigned char ch = 0;
	for (int i = 0; i < sizeof(hash_t); i++)
	{
		ch = ch | (h_piop[i] ^ test_h_piop[i]);
	}
	free(nonce);
	return !(ch == 0);
}