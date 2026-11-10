/* PATCH (nist-sigs-zoo, fix): sig_keygen never read the ICCS DRNG drng_algorithm: unless the non-standard hook
 * galas_set_kat_seed() had been called, it seeded a local DRNG with a constant all-zero seed, so every key pair
 * generated through the ICCS API was the same. This copy draws k and x
 * from drng_algorithm instead (the galas_set_kat_seed() path is unchanged). Because the ICCS KAT driver seeds
 * drng_algorithm with the record seed that kat_gen.c passes to galas_set_kat_seed(), the draws are the same as in
 * the submitted KAT generator. Nothing else is changed. Identical for all 8 instances (the submitted files are identical). */
/*
 * SIG_AlgorithmInstance.c - GALAS signature scheme over the NGCC interface.
 *
 * KeyGen samples (x, k), rejects invalid Gala OWF inputs, and outputs
 * pk = x || y and sk = x || k with y = Gala_k(x).
 *
 * Sign assembles the FAEST-style VOLEitH transcript:
 * BAVC/VOLE commitment, VOLE consistency proof, masked Gala witness,
 * degree-3 QuickSilver proof, final Delta/grinding counter, and BAVC opening.
 *
 * Verify reconstructs the verifier VOLE view and checks the VOLE, QuickSilver,
 * Fiat-Shamir, and BAVC opening equations against the serialized transcript.
 */
#include "SIG_AlgorithmInstance.h"
#include "../galas/instances.h"
#include "../galas/gf2n.h"
#include "../galas/owf.h"
#include "../galas/xof.h"
#include "../galas/bavc.h"
#include "../galas/vole.h"
#include "../galas/random_oracle.h"
#include "../galas/sign_helpers.h"
#include "../galas/challenge.h"
#include "../galas/universal_hashing.h"
#include "../galas/vole_check_ref.h"
#include "../galas/qs_proof_ref.h"
#include "../galas/quicksilver.h"
#include "../galas/qs_gadgets.h"
#include "../galas/qs_constraint.h"
#include "../galas/qs_full_constraints.h"
#include "../galas/bf.h"
#include "drng.h"
#include "auxfunc.h"
#include <string.h>
#include <stdlib.h>

extern DRNG_ctx drng_algorithm; /* PATCH: defined by the ICCS driver */

static const galas_paramset_t* active_ps(void) { return galas_get_paramset(GALAS_INSTANCE); }

/* FAEST-style key layout for Galas:
   public key  pk = x || y
   secret key  sk = x || k */
#define SK_INPUT(sk) (sk)
#define SK_KEY(sk, lb) ((sk) + (lb))
#define PK_INPUT(pk) (pk)
#define PK_OUTPUT(pk, lb) ((pk) + (lb))

/* Optional KAT seed: when set (non-NULL), sig_keygen seeds the DRNG from it
   instead of the default zero seed, making PK/SK reproducible from the KAT
   "Seed" field. kat_gen.c calls galas_set_kat_seed before each keygen. */
static const uint8_t* g_kat_seed = NULL;
static unsigned long long g_kat_seed_len = 0;
void galas_set_kat_seed(const uint8_t* seed, unsigned long long len) {
    g_kat_seed = seed; g_kat_seed_len = len;
}

unsigned long long sig_get_pk_len_bytes(void)  { return active_ps()->p.pk_bytes; }
unsigned long long sig_get_sk_len_bytes(void)  { return active_ps()->p.sk_bytes; }
unsigned long long sig_get_sn_len_bytes(void)  { return active_ps()->p.sig_bytes; }

int sig_keygen(unsigned char* pk, unsigned long long* pk_len_bytes,
               unsigned char* sk, unsigned long long* sk_len_bytes) {
    const galas_paramset_t* ps = active_ps();
    unsigned lambda = ps->p.lambda;
    unsigned lb = lambda / 8;
    const galas_owf_params* P = galas_owf_params_for(lambda);
    gf_ctx ctx;
    if (gf_init(&ctx, lambda) != 0 || !P) return -1;

    /* PATCH: without a galas_set_kat_seed() seed, draw from the ICCS DRNG drng_algorithm (was: all-zero seed). */
    DRNG_ctx drng;
    DRNG_ctx* rng = &drng_algorithm;
    if (g_kat_seed && g_kat_seed_len) {
        init_random_number(&drng, g_kat_seed, g_kat_seed_len);
        rng = &drng;
    }

    uint8_t* k = malloc(lb);
    uint8_t* x = malloc(lb);
    uint8_t* y = malloc(lb);

    int ok = 0;
    for (int tries = 0; tries < 10000 && !ok; ++tries) {
        get_random_number(rng, k, (unsigned long long)lb * 8);
        get_random_number(rng, x, (unsigned long long)lb * 8);
        /* KeyGen constraint 1: k[0]=k[1]=1 (first two bits set), for the QROM
           proof (FAESTv2 spec). Reject and resample if not satisfied. */
        if ((k[0] & 0x03) != 0x03) continue;
        /* KeyGen constraint 2: all S-box inputs must be nonzero. galas_owf_eval
           returns -1 if any wide-S-box input (a0,a1,a2,z) is zero. */
        if (galas_owf_eval(P, &ctx, y, x, k) == 0) ok = 1;
    }
    if (!ok) { free(k); free(x); free(y); return -2; }

    memcpy(PK_INPUT(pk), x, lb);
    memcpy(PK_OUTPUT(pk, lb), y, lb);
    *pk_len_bytes = 2 * lb;
    memcpy(SK_INPUT(sk), x, lb);
    memcpy(SK_KEY(sk, lb), k, lb);
    *sk_len_bytes = 2 * lb;
    free(k); free(x); free(y);
    return 0;
}

/* FAEST-style signature layout:
 *   c                  (tau-1) * ell_hat_bytes
 *   vole_check_proof   lambda/8 + 2
 *   d                  ell/8
 *   qs_proof           2 * lambda/8     (degree-3 QuickSilver proof slot)
 *   decom              tau * 2*lambda/8 + T_open * lambda/8
 *   delta              lambda/8
 *   iv_pre             20
 *   ctr                4
 */
static unsigned witness_bits(unsigned lambda) { return (5u * lambda) / 2u; }
static unsigned witness_bytes(unsigned lambda) { return witness_bits(lambda) / 8u; }
static unsigned ell_hat_bits(unsigned lambda) {
    return witness_bits(lambda) + 3u * lambda + GALAS_UNIVERSAL_HASH_B_BITS;
}
static unsigned ell_hat_bytes(unsigned lambda) { return (ell_hat_bits(lambda) + 7u) / 8u; }
static unsigned vole_check_bytes(unsigned lambda) {
    return lambda / 8u + GALAS_UNIVERSAL_HASH_B;
}
static unsigned qs_proof_bytes(unsigned lambda) { return 2u * (lambda / 8u); }
static unsigned challenge1_bytes(unsigned lambda) { return 5u * (lambda / 8u) + 8u; }
static unsigned challenge2_bytes(unsigned lambda) { return 3u * (lambda / 8u) + 8u; }

static size_t sig_c_len(const galas_paramset_t* ps) {
    return (size_t)(ps->p.tau - 1u) * ell_hat_bytes(ps->p.lambda);
}
static size_t sig_decom_len(const galas_paramset_t* ps) {
    const unsigned lb = galas_lambda_bytes(ps);
    return (size_t)ps->p.tau * 2u * lb + (size_t)ps->p.T_open * lb;
}
static size_t sig_off_vole_check(const galas_paramset_t* ps) { return sig_c_len(ps); }
static size_t sig_off_d(const galas_paramset_t* ps) {
    return sig_off_vole_check(ps) + vole_check_bytes(ps->p.lambda);
}
static size_t sig_off_qs(const galas_paramset_t* ps) {
    return sig_off_d(ps) + witness_bytes(ps->p.lambda);
}
static size_t sig_off_decom(const galas_paramset_t* ps) {
    return sig_off_qs(ps) + qs_proof_bytes(ps->p.lambda);
}
static size_t sig_off_delta(const galas_paramset_t* ps) {
    return sig_off_decom(ps) + sig_decom_len(ps);
}
static size_t sig_off_iv_pre(const galas_paramset_t* ps) {
    return sig_off_delta(ps) + galas_lambda_bytes(ps);
}
static size_t sig_off_ctr(const galas_paramset_t* ps) {
    return sig_off_iv_pre(ps) + GALAS_IV_SIZE;
}
static size_t sig_total_len(const galas_paramset_t* ps) {
    return sig_off_ctr(ps) + sizeof(uint32_t);
}

static void le32_store(uint8_t out[4], uint32_t v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

static uint32_t le32_load(const uint8_t in[4]) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
           ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

static void hash_iv_from_pre(uint8_t* iv, const uint8_t* iv_pre, unsigned lambda) {
    galas_H4_ctx ctx;
    galas_H4_init(&ctx, lambda);
    galas_H4_update(&ctx, iv_pre);
    galas_H4_final(&ctx, iv);
}

static void hash_r_iv_pre(uint8_t* rootkey, uint8_t* iv_pre,
                          const uint8_t* sk_key, unsigned sk_len,
                          const uint8_t* mu, unsigned mu_len,
                          unsigned lambda) {
    galas_H3_ctx ctx;
    galas_H3_init(&ctx, lambda);
    galas_H3_update(&ctx, sk_key, sk_len);
    galas_H3_update(&ctx, mu, mu_len);
    galas_H3_final(&ctx, rootkey, lambda / 8u, iv_pre);
}

static void hash_challenge_1(uint8_t* chall_1, const uint8_t* mu,
                             const uint8_t* commit_check, const uint8_t* c,
                             const uint8_t* iv, const galas_paramset_t* ps) {
    const unsigned lambda = ps->p.lambda;
    const unsigned lb = lambda / 8u;
    galas_H2_ctx ctx;
    galas_H2_init(&ctx, lambda);
    galas_H2_update(&ctx, mu, 2u * lb);
    galas_H2_update(&ctx, commit_check, 2u * lb);
    galas_H2_update(&ctx, c, sig_c_len(ps));
    galas_H2_update(&ctx, iv, GALAS_IV_SIZE);
    galas_H2_1_final(&ctx, chall_1, challenge1_bytes(lambda));
}

static void hash_challenge_2_init(galas_H2_ctx* ctx, const uint8_t* chall_1,
                                  unsigned lambda) {
    galas_H2_init(ctx, lambda);
    galas_H2_update(ctx, chall_1, challenge1_bytes(lambda));
}

static void hash_challenge_2_final(galas_H2_ctx* ctx, uint8_t* chall_2,
                                   const uint8_t* d, unsigned lambda) {
    galas_H2_update(ctx, d, witness_bytes(lambda));
    galas_H2_2_final(ctx, chall_2, challenge2_bytes(lambda));
}

static void hash_challenge_3(uint8_t* delta, const uint8_t* chall_2,
                             const uint8_t* qs_check, const uint8_t* qs_proof,
                             uint32_t ctr, unsigned lambda) {
    uint8_t ctrbuf[4];
    le32_store(ctrbuf, ctr);
    galas_H2_ctx ctx;
    galas_H2_init(&ctx, lambda);
    galas_H2_update(&ctx, chall_2, challenge2_bytes(lambda));
    galas_H2_update(&ctx, qs_check, lambda / 8u);
    galas_H2_update(&ctx, qs_proof, qs_proof_bytes(lambda));
    galas_H2_update(&ctx, ctrbuf, sizeof(ctrbuf));
    galas_H2_3_final(&ctx, delta, lambda / 8u);
}

static void build_galas_witness(uint8_t* w, const gf_ctx* ctx,
                                const galas_owf_params* P,
                                const uint8_t* x, const uint8_t* k) {
    const unsigned lb = ctx->nbytes;
    const unsigned m_bytes = lb / 2u;

    uint8_t rho0[GALAS_MAX_LAMBDA_BYTES] = {0};
    uint8_t rho1[GALAS_MAX_LAMBDA_BYTES] = {0};
    uint8_t rho2[GALAS_MAX_LAMBDA_BYTES] = {0};
    for (unsigned i = 0; i < lb; ++i) {
        rho0[i] = x[i] ^ k[i] ^ P->c0[i];
        rho1[i] = k[i] ^ P->c1[i];
        rho2[i] = k[i] ^ P->c2[i];
    }

    gf_limb_t r0[GF_LIMBS(512)], r1[GF_LIMBS(512)], r2[GF_LIMBS(512)];
    gf_limb_t a0[GF_LIMBS(512)], a1[GF_LIMBS(512)], a2[GF_LIMBS(512)];
    uint8_t a0_bytes[GALAS_MAX_LAMBDA_BYTES] = {0};
    uint8_t a1_bytes[GALAS_MAX_LAMBDA_BYTES] = {0};
    uint8_t a2_bytes[GALAS_MAX_LAMBDA_BYTES] = {0};

    gf_from_bytes(ctx, r0, rho0);
    gf_from_bytes(ctx, r1, rho1);
    gf_from_bytes(ctx, r2, rho2);
    galas_apply_lin_map(ctx, a0, r0, P->M0);
    galas_apply_lin_map(ctx, a1, r1, P->M1);
    galas_apply_lin_map(ctx, a2, r2, P->M2);
    gf_to_bytes(ctx, a0_bytes, a0);
    gf_to_bytes(ctx, a1_bytes, a1);
    gf_to_bytes(ctx, a2_bytes, a2);

    memset(w, 0, witness_bytes(ctx->n));
    memcpy(w, k, lb);
    galas_compute_sigma(w + lb, a0_bytes, ctx, P);
    galas_compute_sigma(w + lb + m_bytes, a1_bytes, ctx, P);
    galas_compute_sigma(w + lb + 2u * m_bytes, a2_bytes, ctx, P);
}

int sig_sign(unsigned char* sk, unsigned long long sk_len_bytes,
             unsigned char* m, unsigned long long m_len_bytes,
             unsigned char* sn, unsigned long long* sn_len_bytes) {
    const galas_paramset_t* ps = active_ps();
    unsigned lambda = ps->p.lambda;
    unsigned lb = lambda / 8;
    unsigned tau = ps->p.tau;
    const galas_owf_params* P = galas_owf_params_for(lambda);
    gf_ctx ctx; gf_init(&ctx, lambda);

    if (sk_len_bytes != 2u * lb) return -1;
    if (sig_total_len(ps) != ps->p.sig_bytes) return -2;

    const uint8_t* sk_x = SK_INPUT(sk);
    const uint8_t* sk_k = SK_KEY(sk, lb);

    uint8_t* pk_x = malloc(lb);
    uint8_t* pk_y = malloc(lb);
    memcpy(pk_x, sk_x, lb);
    if (galas_owf_eval(P, &ctx, pk_y, pk_x, sk_k) != 0) {
        free(pk_x); free(pk_y);
        return -3;
    }
    uint8_t* pk = malloc(2 * lb);
    memcpy(pk, pk_x, lb); memcpy(pk + lb, pk_y, lb);

    /* mu = H2_0(pk || m) */
    uint8_t* mu = malloc(2 * lb);
    galas_hash_mu(mu, pk, 2 * lb, m, (unsigned)m_len_bytes, lambda);

    /* rootKey, iv_pre from H3(sk || mu), then iv = H4(iv_pre). */
    uint8_t* rootkey = malloc(lb);
    uint8_t iv_pre[GALAS_IV_SIZE];
    uint8_t iv[GALAS_IV_SIZE];
    hash_r_iv_pre(rootkey, iv_pre, sk_k, lb, mu, 2 * lb, lambda);
    hash_iv_from_pre(iv, iv_pre, lambda);

    /* VOLE commit (runs BAVC internally) */
    unsigned wb = witness_bytes(lambda);
    unsigned ell_hat = ell_hat_bits(lambda);
    unsigned eb = ell_hat_bytes(lambda);
    galas_bavc_t vc;
    uint8_t* c = sn;
    uint8_t* u = malloc(eb);
    uint8_t* vbuf = malloc((size_t)lambda * eb);
    uint8_t** v = malloc(lambda * sizeof(uint8_t*));
    for (unsigned i = 0; i < lambda; ++i) v[i] = vbuf + (size_t)i * eb;
    galas_vole_commit(rootkey, iv, ell_hat, ps, &vc, c, u, v);

    uint8_t* chall_1 = malloc(challenge1_bytes(lambda));
    hash_challenge_1(chall_1, mu, vc.h, c, iv, ps);

    uint8_t* vole_check_proof = sn + sig_off_vole_check(ps);

    uint8_t* witness = malloc(wb);
    build_galas_witness(witness, &ctx, P, pk_x, sk_k);

    const unsigned m_bytes = lb / 2u;
    const uint8_t* s0 = witness + lb;
    const uint8_t* s1 = witness + lb + m_bytes;
    const uint8_t* s2 = witness + lb + 2u * m_bytes;

    /* verify all constraints pass (prover self-check) */
    if (!galas_constraints_eval(&ctx, P, sk_k, s0, s1, s2, pk_x, pk_y)) {
        free(pk_x); free(pk_y); free(pk); free(mu); free(rootkey);
        free(u); free(vbuf); free(v); free(chall_1); free(witness);
        galas_bavc_clear(&vc);
        return -5;  /* constraint check failed: witness invalid */
    }

    uint8_t* d = sn + sig_off_d(ps);
    for (unsigned i = 0; i < wb; ++i) d[i] = witness[i] ^ u[i];

    galas_H2_ctx c2ctx;
    hash_challenge_2_init(&c2ctx, chall_1, lambda);
    galas_vole_check_sender(vole_check_proof, &c2ctx, chall_1, u, v, ps);
    uint8_t* chall_2 = malloc(challenge2_bytes(lambda));
    hash_challenge_2_final(&c2ctx, chall_2, d, lambda);

    uint8_t* qs_proof = sn + sig_off_qs(ps);
    uint8_t qs_check[GALAS_MAX_LAMBDA_BYTES];
    memcpy(u, witness, wb);
    if (galas_qs_prove(qs_proof, qs_check, u, v, chall_2, pk_x, pk_y, ps) != 0) {
        free(pk_x); free(pk_y); free(pk); free(mu); free(rootkey);
        free(u); free(vbuf); free(v); free(chall_1); free(chall_2);
        free(witness);
        galas_bavc_clear(&vc);
        return -6;
    }

    uint8_t* decom = sn + sig_off_decom(ps);
    uint8_t* delta = sn + sig_off_delta(ps);
    uint8_t* iv_pre_dst = sn + sig_off_iv_pre(ps);
    uint8_t* ctr_dst = sn + sig_off_ctr(ps);
    memcpy(iv_pre_dst, iv_pre, GALAS_IV_SIZE);

    uint16_t i_delta[GALAS_MAX_TAU];
    uint32_t ctr = 0;
    int opened = 0;
    for (; ctr != UINT32_MAX && !opened; ++ctr) {
        hash_challenge_3(delta, chall_2, qs_check, qs_proof, ctr, lambda);
        if (!galas_check_chall_3(delta, lambda, ps->p.w_grind)) continue;
        if (!galas_decode_all_chall_3(i_delta, delta, ps)) continue;
        if (galas_bavc_open(&vc, i_delta, decom, ps)) opened = 1;
    }
    if (!opened) {
        free(pk_x); free(pk_y); free(pk); free(mu); free(rootkey);
        free(u); free(vbuf); free(v); free(chall_1); free(chall_2);
        free(witness);
        galas_bavc_clear(&vc);
        return -4;
    }
    le32_store(ctr_dst, ctr - 1u);

    *sn_len_bytes = (unsigned long long)sig_total_len(ps);

    free(pk_x); free(pk_y); free(pk); free(mu); free(rootkey);
    free(u); free(vbuf); free(v); free(chall_1); free(chall_2);
    free(witness);
    galas_bavc_clear(&vc);
    (void)tau;
    return 0;
}

int sig_verify(unsigned char* pk, unsigned long long pk_len_bytes,
               unsigned char* sn, unsigned long long sn_len_bytes,
               unsigned char* m, unsigned long long m_len_bytes) {
    const galas_paramset_t* ps = active_ps();
    unsigned lambda = ps->p.lambda;
    unsigned lb = lambda / 8;
    unsigned eb = ell_hat_bytes(lambda);

    if (pk_len_bytes != 2u * lb) return -1;
    if (sn_len_bytes != sig_total_len(ps) || sn_len_bytes != ps->p.sig_bytes) return -2;

    const uint8_t* c = sn;
    const uint8_t* vole_check_proof = sn + sig_off_vole_check(ps);
    const uint8_t* d = sn + sig_off_d(ps);
    const uint8_t* qs_proof = sn + sig_off_qs(ps);
    const uint8_t* decom = sn + sig_off_decom(ps);
    const uint8_t* delta = sn + sig_off_delta(ps);
    const uint8_t* iv_pre = sn + sig_off_iv_pre(ps);
    const uint8_t* ctr_ptr = sn + sig_off_ctr(ps);
    uint32_t ctr = le32_load(ctr_ptr);

    uint8_t iv[GALAS_IV_SIZE];
    hash_iv_from_pre(iv, iv_pre, lambda);

    /* recompute mu */
    uint8_t mu[2 * 64];
    galas_hash_mu(mu, pk, (unsigned)pk_len_bytes, m, (unsigned)m_len_bytes, lambda);

    if (!galas_check_chall_3(delta, lambda, ps->p.w_grind)) return -3;

    uint16_t i_delta[GALAS_MAX_TAU];
    if (!galas_decode_all_chall_3(i_delta, delta, ps)) return -4;

    uint8_t** q = malloc(lambda * sizeof(uint8_t*));
    uint8_t* qbuf = calloc(lambda, eb);
    if (!q || !qbuf) { free(q); free(qbuf); return -5; }
    for (unsigned i = 0; i < lambda; ++i) q[i] = qbuf + (size_t)i * eb;

    uint8_t hcom[2 * GALAS_MAX_LAMBDA_BYTES];
    if (!galas_vole_reconstruct(hcom, q, iv, i_delta, decom, c, ell_hat_bits(lambda), ps)) {
        free(qbuf); free(q);
        return -5;
    }

    uint8_t* chall_1 = malloc(challenge1_bytes(lambda));
    hash_challenge_1(chall_1, mu, hcom, c, iv, ps);

    galas_H2_ctx c2ctx;
    hash_challenge_2_init(&c2ctx, chall_1, lambda);
    galas_H2_update(&c2ctx, vole_check_proof, vole_check_bytes(lambda));
    galas_vole_check_receiver(&c2ctx, chall_1, vole_check_proof, q, delta, ps);

    uint8_t* chall_2 = malloc(challenge2_bytes(lambda));
    hash_challenge_2_final(&c2ctx, chall_2, d, lambda);

    uint8_t qs_check[GALAS_MAX_LAMBDA_BYTES];
    uint8_t delta_check[GALAS_MAX_LAMBDA_BYTES];
    if (galas_qs_verify(qs_check, qs_proof, q, d, delta, chall_2,
                        PK_INPUT(pk), PK_OUTPUT(pk, lb), ps) != 0) {
        free(chall_1); free(chall_2); free(qbuf); free(q);
        return -7;
    }
    hash_challenge_3(delta_check, chall_2, qs_check, qs_proof, ctr, lambda);

    int ok = (memcmp(delta, delta_check, lb) == 0);
    free(chall_1); free(chall_2); free(qbuf); free(q);
    return ok ? 0 : -8;
}
