/* PATCH (nist-sigs-zoo): sig_verify stored the result of uvw_verify in `result` but always returned 0, so every
 * signature was accepted for every message; see docs/ngcc-submission-bugs.md item 15. This copy returns `result`
 * (0 = valid, -1 = invalid, as computed by the submitted code); nothing else is changed. */
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "uvw.h"

DRNG_ctx *uvw_drng = &drng_algorithm;

static void serialize_param(unsigned char *buf, const uvw_param *p) {
    uint16_t v;
    int pos = 0;
    v = p->lambda; memcpy(buf + pos, &v, 2); pos += 2;
    v = p->n;     memcpy(buf + pos, &v, 2); pos += 2;
    v = p->k;     memcpy(buf + pos, &v, 2); pos += 2;
    v = p->k1;    memcpy(buf + pos, &v, 2); pos += 2;
    v = p->k2;    memcpy(buf + pos, &v, 2); pos += 2;
    v = p->w;     memcpy(buf + pos, &v, 2); pos += 2;
}

static void deserialize_param(uvw_param *p, const unsigned char *buf) {
    uint16_t v;
    int pos = 0;
    memcpy(&v, buf + pos, 2); p->lambda = v; pos += 2;
    memcpy(&v, buf + pos, 2); p->n     = v; pos += 2;
    memcpy(&v, buf + pos, 2); p->k     = v; pos += 2;
    memcpy(&v, buf + pos, 2); p->k1    = v; pos += 2;
    memcpy(&v, buf + pos, 2); p->k2    = v; pos += 2;
    memcpy(&v, buf + pos, 2); p->w     = v; pos += 2;
}

static unsigned long long pk_serialized_size(const uvw_param *p) {
    size_t cols_per_word = WORD_LENGTH;
    size_t alloc_per_row = 1 + ((p->k - 1) / cols_per_word);
    return 12 + (unsigned long long)(p->n - p->k) * alloc_per_row * sizeof(wave_word) * 2;
}

static unsigned long long sn_serialized_size(const uvw_param *p) {
    size_t cols_per_word = WORD_LENGTH;
    size_t alloc_e = 1 + ((p->k - 1) / cols_per_word);
    return 12 + (unsigned long long)(p->lambda / 8) + (unsigned long long)alloc_e * sizeof(wave_word) * 2;
}

static void serialize_pk(unsigned char *buf, const uvw_public_key *pk) {
    int pos = 0;
    serialize_param(buf, &pk->param);
    pos = 12;
    for (size_t i = 0; i < pk->r->n_row; i++) {
        unsigned long long row_bytes = pk->r->rows[i].alloc * sizeof(uint64_t);
        memcpy(buf + pos, pk->r->rows[i].r0, row_bytes);
        pos += row_bytes;
        memcpy(buf + pos, pk->r->rows[i].r1, row_bytes);
        pos += row_bytes;
    }
}

static void deserialize_pk(uvw_public_key *pk, const unsigned char *buf) {
    deserialize_param(&pk->param, buf);
    int pos = 12;
    pk->r = mf3_alloc(pk->param.n - pk->param.k, pk->param.k);
    for (size_t i = 0; i < pk->r->n_row; i++) {
        unsigned long long row_bytes = pk->r->rows[i].alloc * sizeof(uint64_t);
        memcpy(pk->r->rows[i].r0, buf + pos, row_bytes);
        pos += row_bytes;
        memcpy(pk->r->rows[i].r1, buf + pos, row_bytes);
        pos += row_bytes;
    }
}

static void serialize_sn(unsigned char *buf, const uvw_signature *sn) {
    int pos = 0;
    serialize_param(buf, &sn->param);
    pos = 12;
    memcpy(buf + pos, sn->r, sn->param.lambda / 8);
    pos += sn->param.lambda / 8;
    unsigned long long e_bytes = sn->e->alloc * sizeof(uint64_t);
    memcpy(buf + pos, sn->e->r0, e_bytes);
    pos += e_bytes;
    memcpy(buf + pos, sn->e->r1, e_bytes);
    pos += e_bytes;
}

static void deserialize_sn(uvw_signature *sn, const unsigned char *buf) {
    deserialize_param(&sn->param, buf);
    int pos = 12;
    sn->r = malloc(sn->param.lambda / 8);
    memcpy(sn->r, buf + pos, sn->param.lambda / 8);
    pos += sn->param.lambda / 8;
    sn->e = vf3_alloc(sn->param.k);
    unsigned long long e_bytes = sn->e->alloc * sizeof(uint64_t);
    memcpy(sn->e->r0, buf + pos, e_bytes);
    pos += e_bytes;
    memcpy(sn->e->r1, buf + pos, e_bytes);
    pos += e_bytes;
}

static void init_seeded_drng_nonce(DRNG_ctx *drng, const unsigned char *seed, uint16_t nonce) {
    unsigned char seeded[SEED_LEN_BYTES + 2];
    memcpy(seeded, seed, SEED_LEN_BYTES);
    seeded[SEED_LEN_BYTES]     = (unsigned char)((nonce >> 8) & 0xFF);
    seeded[SEED_LEN_BYTES + 1] = (unsigned char)(nonce & 0xFF);
    init_random_number(drng, seeded, SEED_LEN_BYTES + 2);
}

static void generate_f3_matrix(mf3_e *M, const unsigned char *seed, uint16_t nonce) {
    DRNG_ctx local_drng;
    DRNG_ctx *saved_drng = uvw_drng;
    uvw_drng = &local_drng;
    init_seeded_drng_nonce(&local_drng, seed, nonce);
    mf3_random(M);
    uvw_drng = saved_drng;
}

static bool mf3_is_row_full_rank(const mf3_e *m) {
    if (m->n_row > m->n_col) return false;
    mf3_e *copy = mf3_copy(m);
    mf3_e *result;
    size_t rank = gauss_jordan_elimination(copy, &result, NULL, NULL);
    mf3_free(copy);
    mf3_free(result);
    return rank == m->n_row;
}

static void generate_dp_matrix(dp_e *D, const unsigned char *seed, uint16_t nonce) {
    DRNG_ctx local_drng;
    DRNG_ctx *saved_drng = uvw_drng;
    uvw_drng = &local_drng;
    init_seeded_drng_nonce(&local_drng, seed, nonce);
    dp_random_non_zero(D);
    dp_shuffle(D);
    uvw_drng = saved_drng;
}

static bool row_reduce_to_identity(mf3_e *H, mf3_e *T) {
    size_t n = H->n_row;
    if (H->n_col != n) return false;
    mf3_set_to_identity(T);
    mf3_e *work = mf3_copy(H);
    size_t pivot_row = 0;
    for (size_t col = 0; col < n && pivot_row < n; col++) {
        size_t found = SIZE_MAX;
        for (size_t r = pivot_row; r < n; r++) {
            if (mf3_coeff(work, r, col)) { found = r; break; }
        }
        if (found == SIZE_MAX) continue;
        if (found != pivot_row) {
            mf3_swap_rows(work, pivot_row, found);
            mf3_swap_rows(T, pivot_row, found);
        }
        if (mf3_coeff(work, pivot_row, col) != 1) {
            uint8_t inv = mf3_coeff(work, pivot_row, col);
            vf3_vector_scalarmul_nonzero_inplace(&work->rows[pivot_row], inv);
            vf3_vector_scalarmul_nonzero_inplace(&T->rows[pivot_row], inv);
        }
        for (size_t r = 0; r < n; r++) {
            if (r == pivot_row) continue;
            uint8_t scalar = mf3_coeff(work, r, col);
            if (!scalar) continue;
            vf3_e *tmp_T = vf3_alloc(n);
            vf3_e *tmp_W = vf3_alloc(n);
            vf3_vector_scalarmul(tmp_T, scalar, &T->rows[pivot_row]);
            vf3_vector_scalarmul(tmp_W, scalar, &work->rows[pivot_row]);
            vf3_vector_sub_inplace(&T->rows[r], tmp_T);
            vf3_vector_sub_inplace(&work->rows[r], tmp_W);
            vf3_free(tmp_T);
            vf3_free(tmp_W);
        }
        pivot_row++;
    }
    mf3_free(work);
    return (pivot_row == n);
}

static bool try_build_keypair(
    const uvw_param param,
    const unsigned char *seed,
    uint16_t nonce_x, uint16_t nonce_y, uint16_t nonce_z, uint16_t nonce_d,
    uvw_public_key *out_pk,
    mf3_e **out_sti)
{
    mf3_e *h_x = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_e *h_y = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_e *h_z = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    generate_f3_matrix(h_x, seed, nonce_x);
    generate_f3_matrix(h_y, seed, nonce_y);
    generate_f3_matrix(h_z, seed, nonce_z);

    dp_e *d_raw = dp_alloc(param.n);
    generate_dp_matrix(d_raw, seed, nonce_d);

    mf3_e *h_z_neg = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    mf3_matrix_neg(h_z_neg, h_z);

    mf3_e *h_sk_left = mf3_alloc(param.n - param.k, param.n);
    for (size_t i = 0; i < param.n / 2 - param.k1; i++) {
        vf3_vector_cat(&h_sk_left->rows[i], &h_x->rows[i], &h_y->rows[i]);
    }
    for (size_t i = param.n / 2 - param.k1; i < param.n - param.k; i++) {
        vf3_vector_cat(
            &h_sk_left->rows[i],
            &h_z_neg->rows[i - (param.n / 2 - param.k1)],
            &h_z->rows[i - (param.n / 2 - param.k1)]
        );
    }
    mf3_free(h_z_neg);

    mf3_e *h_sk = mf3_alloc(param.n - param.k, param.n);
    dp_right_product_matrix(h_sk, d_raw, h_sk_left);
    dp_free(d_raw);
    mf3_free(h_sk_left);

    mf3_e *H1 = mf3_alloc(param.n - param.k, param.n - param.k);
    mf3_e *R = mf3_alloc(param.n - param.k, param.k);
    const size_t h1_words_per_row = (param.n - param.k + WORD_LENGTH - 1) / WORD_LENGTH;
    vf3_e *tmp = vf3_alloc(param.n - param.k);
    for (size_t r = 0; r < param.n - param.k; r++) {
        memcpy(H1->rows[r].r0, h_sk->rows[r].r0, h1_words_per_row * sizeof(wave_word));
        memcpy(H1->rows[r].r1, h_sk->rows[r].r1, h1_words_per_row * sizeof(wave_word));
        vf3_trim(&H1->rows[r]);
        vf3_vector_split(tmp, &R->rows[r], &h_sk->rows[r]);
    }
    vf3_free(tmp);
    mf3_free(h_sk);
    mf3_free(h_x);
    mf3_free(h_y);
    mf3_free(h_z);

    mf3_e *H1_orig = mf3_copy(H1);

    mf3_e *T = mf3_alloc(param.n - param.k, param.n - param.k);
    bool ok = row_reduce_to_identity(H1, T);
    mf3_free(H1);

    if (!ok) {
        mf3_free(R);
        mf3_free(T);
        mf3_free(H1_orig);
        return false;
    }

    mf3_e *R_corrected = mf3_alloc(param.n - param.k, param.k);
    for (size_t r = 0; r < param.n - param.k; r++) {
        mf3_product_vector_matrix(&R_corrected->rows[r], &T->rows[r], R);
    }
    mf3_free(R);

    out_pk->param = param;
    out_pk->r = R_corrected;

    mf3_e *h1t = mf3_alloc(param.n - param.k, param.n - param.k);
    mf3_transpose_and_copy(h1t, H1_orig);
    mf3_free(H1_orig);
    mf3_free(T);

    *out_sti = h1t;
    return true;
}

/**
 * @brief 获取公钥序列化后的字节长度
 * @return 公钥缓冲区所需的最小字节数
 * @note 公钥包含：参数(12B) + H1矩阵((n-k)行 × k列的F₃矩阵)
 */
unsigned long long sig_get_pk_len_bytes() {
    uvw_param p = uvw_param_from_level(3);
    return pk_serialized_size(&p);
}

/**
 * @brief 获取私钥序列化后的字节长度
 * @return 私钥缓冲区所需的最小字节数
 * @note 私钥结构：seed(SEED_LEN_BYTES) + nonce_x(2B) + nonce_y(2B) + nonce_z(2B) + nonce_d(2B)
 *       对于UVW-128/256: SEED_LEN_BYTES=32, 总计40字节
 *       对于UVW-512: SEED_LEN_BYTES=64, 总计72字节
 */
unsigned long long sig_get_sk_len_bytes() {
    return SEED_LEN_BYTES + 4 * 2;
}

/**
 * @brief 获取签名序列化后的字节长度
 * @return 签名缓冲区所需的最小字节数
 * @note 签名包含：参数(12B) + 随机数r(λ/8 B) + 稀疏向量e(k维F₃向量)
 */
unsigned long long sig_get_sn_len_bytes() {
    uvw_param p = uvw_param_from_level(3);
    return sn_serialized_size(&p);
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    if (!uvw_drng) return -1;
    uvw_param param = uvw_param_from_level(3);

    unsigned char seed[SEED_LEN_BYTES];
    get_random_number(uvw_drng, seed, SEED_LEN_BYTES * 8);

    uint16_t nonce_x = 0, nonce_y = 0, nonce_z = 0, nonce_d = 0;

    mf3_e *h_x = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    int max_retry = 10000;
    for (int retry = 0; retry < max_retry; retry++) {
        get_random_number(uvw_drng, (unsigned char *)&nonce_x, 16);
        generate_f3_matrix(h_x, seed, nonce_x);
        if (mf3_is_row_full_rank(h_x)) break;
    }
    if (!mf3_is_row_full_rank(h_x)) { mf3_free(h_x); return -1; }

    mf3_e *h_y = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    for (int retry = 0; retry < max_retry; retry++) {
        get_random_number(uvw_drng, (unsigned char *)&nonce_y, 16);
        generate_f3_matrix(h_y, seed, nonce_y);
        if (mf3_is_row_full_rank(h_y)) break;
    }
    if (!mf3_is_row_full_rank(h_y)) { mf3_free(h_x); mf3_free(h_y); return -1; }

    mf3_e *h_z = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    for (int retry = 0; retry < max_retry; retry++) {
        get_random_number(uvw_drng, (unsigned char *)&nonce_z, 16);
        generate_f3_matrix(h_z, seed, nonce_z);
        if (mf3_is_row_full_rank(h_z)) break;
    }
    if (!mf3_is_row_full_rank(h_z)) { mf3_free(h_x); mf3_free(h_y); mf3_free(h_z); return -1; }

    uvw_public_key result_pk;
    mf3_e *sti = NULL;
    int max_d_retry = 10000;
    int found = 0;

    for (int dr = 0; dr < max_d_retry; dr++) {
        get_random_number(uvw_drng, (unsigned char *)&nonce_d, 16);
        if (try_build_keypair(param, seed, nonce_x, nonce_y, nonce_z, nonce_d, &result_pk, &sti)) {
            found = 1;
            break;
        }
    }

    mf3_free(h_x);
    mf3_free(h_y);
    mf3_free(h_z);

    if (!found) {
        fprintf(stderr, "ERROR: Failed after %d D-matrix retries\n", max_d_retry);
        return -1;
    }

    serialize_pk(pk, &result_pk);
    *pk_len_bytes = pk_serialized_size(&param);

    memcpy(sk, seed, SEED_LEN_BYTES);
    sk[SEED_LEN_BYTES]     = (unsigned char)((nonce_x >> 8) & 0xFF);
    sk[SEED_LEN_BYTES + 1] = (unsigned char)(nonce_x & 0xFF);
    sk[SEED_LEN_BYTES + 2] = (unsigned char)((nonce_y >> 8) & 0xFF);
    sk[SEED_LEN_BYTES + 3] = (unsigned char)(nonce_y & 0xFF);
    sk[SEED_LEN_BYTES + 4] = (unsigned char)((nonce_z >> 8) & 0xFF);
    sk[SEED_LEN_BYTES + 5] = (unsigned char)(nonce_z & 0xFF);
    sk[SEED_LEN_BYTES + 6] = (unsigned char)((nonce_d >> 8) & 0xFF);
    sk[SEED_LEN_BYTES + 7] = (unsigned char)(nonce_d & 0xFF);
    *sk_len_bytes = SEED_LEN_BYTES + 8;

    mf3_free(sti);
    uvw_free_public_key(result_pk);
    return 0;
}

static void rebuild_full_keypair(
    const uvw_param param,
    const unsigned char *seed,
    uint16_t nonce_x, uint16_t nonce_y, uint16_t nonce_z, uint16_t nonce_d,
    uvw_secret_key *out_sk,
    uvw_public_key *out_pk)
{
    mf3_e *h_x = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_e *h_y = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_e *h_z = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    generate_f3_matrix(h_x, seed, nonce_x);
    generate_f3_matrix(h_y, seed, nonce_y);
    generate_f3_matrix(h_z, seed, nonce_z);

    dp_e *d_raw = dp_alloc(param.n);
    generate_dp_matrix(d_raw, seed, nonce_d);

    out_sk->param = param;
    out_sk->h_x = h_x;
    out_sk->h_y = h_y;
    out_sk->h_z = h_z;
    out_sk->d = d_raw;

    mf3_e *h_z_neg = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    mf3_matrix_neg(h_z_neg, h_z);

    mf3_e *h_sk_left = mf3_alloc(param.n - param.k, param.n);
    for (size_t i = 0; i < param.n / 2 - param.k1; i++) {
        vf3_vector_cat(&h_sk_left->rows[i], &h_x->rows[i], &h_y->rows[i]);
    }
    for (size_t i = param.n / 2 - param.k1; i < param.n - param.k; i++) {
        vf3_vector_cat(
            &h_sk_left->rows[i],
            &h_z_neg->rows[i - (param.n / 2 - param.k1)],
            &h_z->rows[i - (param.n / 2 - param.k1)]
        );
    }
    mf3_free(h_z_neg);

    mf3_e *h_sk = mf3_alloc(param.n - param.k, param.n);
    dp_right_product_matrix(h_sk, d_raw, h_sk_left);
    mf3_free(h_sk_left);

    mf3_e *H1 = mf3_alloc(param.n - param.k, param.n - param.k);
    mf3_e *R = mf3_alloc(param.n - param.k, param.k);
    for (size_t r = 0; r < param.n - param.k; r++) {
        vf3_e *dummy = vf3_alloc(param.n - param.k);
        vf3_vector_split(dummy, &R->rows[r], &h_sk->rows[r]);
        for (size_t c = 0; c < param.n - param.k; c++) {
            mf3_setcoeff(H1, r, c, vf3_get_element(c, dummy));
        }
        vf3_free(dummy);
    }
    mf3_free(h_sk);

    mf3_e *H1_orig = mf3_copy(H1);

    mf3_e *T = mf3_alloc(param.n - param.k, param.n - param.k);
    row_reduce_to_identity(H1, T);
    mf3_free(H1);

    mf3_e *R_corrected = mf3_alloc(param.n - param.k, param.k);
    for (size_t r = 0; r < param.n - param.k; r++) {
        mf3_product_vector_matrix(&R_corrected->rows[r], &T->rows[r], R);
    }
    mf3_free(R);

    out_pk->param = param;
    out_pk->r = R_corrected;

    mf3_e *h1t = mf3_alloc(param.n - param.k, param.n - param.k);
    mf3_transpose_and_copy(h1t, H1_orig);
    mf3_free(H1_orig);
    mf3_free(T);

    out_sk->sti = h1t;
}

static void rebuild_for_sign(
    const uvw_param param,
    const unsigned char *seed,
    uint16_t nonce_x, uint16_t nonce_y, uint16_t nonce_z, uint16_t nonce_d,
    uvw_secret_key *out_sk)
{
    mf3_e *h_x = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_e *h_y = mf3_alloc(param.n / 2 - param.k1, param.n / 2);
    mf3_e *h_z = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    generate_f3_matrix(h_x, seed, nonce_x);
    generate_f3_matrix(h_y, seed, nonce_y);
    generate_f3_matrix(h_z, seed, nonce_z);

    dp_e *d_raw = dp_alloc(param.n);
    generate_dp_matrix(d_raw, seed, nonce_d);

    out_sk->param = param;
    out_sk->h_x = h_x;
    out_sk->h_y = h_y;
    out_sk->h_z = h_z;
    out_sk->d = d_raw;

    mf3_e *h_z_neg = mf3_alloc(param.n / 2 - param.k2, param.n / 2);
    mf3_matrix_neg(h_z_neg, h_z);

    mf3_e *h_sk_left = mf3_alloc(param.n - param.k, param.n);
    for (size_t i = 0; i < param.n / 2 - param.k1; i++) {
        vf3_vector_cat(&h_sk_left->rows[i], &h_x->rows[i], &h_y->rows[i]);
    }
    for (size_t i = param.n / 2 - param.k1; i < param.n - param.k; i++) {
        vf3_vector_cat(
            &h_sk_left->rows[i],
            &h_z_neg->rows[i - (param.n / 2 - param.k1)],
            &h_z->rows[i - (param.n / 2 - param.k1)]
        );
    }
    mf3_free(h_z_neg);

    mf3_e *h_sk = mf3_alloc(param.n - param.k, param.n);
    dp_right_product_matrix(h_sk, d_raw, h_sk_left);
    mf3_free(h_sk_left);

    mf3_e *H1 = mf3_alloc(param.n - param.k, param.n - param.k);
    const size_t h1_words_per_row = (param.n - param.k + WORD_LENGTH - 1) / WORD_LENGTH;
    for (size_t r = 0; r < param.n - param.k; r++) {
        memcpy(H1->rows[r].r0, h_sk->rows[r].r0, h1_words_per_row * sizeof(wave_word));
        memcpy(H1->rows[r].r1, h_sk->rows[r].r1, h1_words_per_row * sizeof(wave_word));
        vf3_trim(&H1->rows[r]);
    }
    mf3_free(h_sk);

    out_sk->sti = mf3_alloc(param.n - param.k, param.n - param.k);
    mf3_transpose_and_copy(out_sk->sti, H1);
    mf3_free(H1);
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    if (!uvw_drng || sk_len_bytes != SEED_LEN_BYTES + 8) return -1;

    uvw_param param = uvw_param_from_level(3);
    const unsigned char *seed = sk;
    uint16_t nonce_x = ((uint16_t)sk[SEED_LEN_BYTES] << 8) | sk[SEED_LEN_BYTES + 1];
    uint16_t nonce_y = ((uint16_t)sk[SEED_LEN_BYTES + 2] << 8) | sk[SEED_LEN_BYTES + 3];
    uint16_t nonce_z = ((uint16_t)sk[SEED_LEN_BYTES + 4] << 8) | sk[SEED_LEN_BYTES + 5];
    uint16_t nonce_d = ((uint16_t)sk[SEED_LEN_BYTES + 6] << 8) | sk[SEED_LEN_BYTES + 7];

    uvw_secret_key sk_obj;
    rebuild_for_sign(param, seed, nonce_x, nonce_y, nonce_z, nonce_d, &sk_obj);

    shake256_ctx ctx;
    uvw_hash_init(&ctx);
    uvw_hash_update(&ctx, m, m_len_bytes);
    uvw_signature sign = uvw_sign(sk_obj, &ctx);
    serialize_sn(sn, &sign);
    *sn_len_bytes = sn_serialized_size(&sign.param);
    uvw_hash_free(&ctx);
    uvw_free_signature(sign);
    uvw_free_secret_key(sk_obj);
    return 0;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    uvw_public_key pk_obj;
    deserialize_pk(&pk_obj, pk);
    uvw_signature sn_obj;
    deserialize_sn(&sn_obj, sn);
    shake256_ctx ctx;
    uvw_hash_init(&ctx);
    uvw_hash_update(&ctx, m, m_len_bytes);
    int result = uvw_verify(pk_obj, &ctx, sn_obj) ? 0 : -1;
    uvw_hash_free(&ctx);
    uvw_free_public_key(pk_obj);
    uvw_free_signature(sn_obj);
    return result;
}
