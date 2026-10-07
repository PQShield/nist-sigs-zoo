/*
 * ngcc_kat: regenerate ICCS-format known-answer records from an NGCC instance .so.
 *
 * Our own driver. It replays the seed schedule of the ICCS KAT_SIG.c / KAT_KEM.c
 * generators shipped with every submission, so that its output can be compared
 * with the submitted KAT_SIG_<instance>.txt / KAT_KEM_<instance>.txt (kat_compare.py):
 *   - a seed DRNG initialised with "seed" x 16 yields one 64-byte seed per record;
 *   - drng_algorithm (the instance's DRNG) is re-initialised with that seed;
 *   - signatures sign a message from a DRNG initialised with "msg" x 21 + "m",
 *     56 bytes in record 0 and 8 bytes longer in each later record;
 *   - length variables start at the getter values once, then carry whatever the
 *     implementation wrote, exactly as in the ICCS drivers.
 * Lengths are always printed as numbers; kat_compare.py drops them when the
 * submitted file was generated in template mode.
 *
 * Usage: ngcc_kat sig|kem LIB.so OUT.txt [records]      (records 1-10, default 10)
 * Exit: 0 ok, 1 usage/IO, 2 load failure, 3 crypto failure (error return, a length
 * over its claim, or KEM shared secrets that differ).
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long ull;
typedef int (*init_fn)(void *drng, const unsigned char *seed, ull seed_len);
typedef int (*rand_fn)(void *drng, unsigned char *out, ull bits);
typedef ull (*len_fn)(void);
typedef int (*keygen_fn)(unsigned char *, ull *, unsigned char *, ull *);
typedef int (*sig_sign_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *);
typedef int (*sig_verify_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull);
typedef int (*kem_enc_fn)(unsigned char *, ull, unsigned char *, ull *, unsigned char *, ull *);
typedef int (*kem_dec_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *);

#define SEED_LEN 64
/* Storage for the driver's own DRNG contexts (the ICCS DRNG_ctx is 165 bytes). */
typedef struct { _Alignas(64) unsigned char bytes[4096]; } drng_storage;

static void *lib;

static void *sym(const char *name) {
    void *p = dlsym(lib, name);
    if (!p) {
        fprintf(stderr, "ngcc_kat: missing symbol %s\n", name);
        exit(2);
    }
    return p;
}
#define SYM(type, name) ((type)(uintptr_t)sym(name))

static void pstr(FILE *f, const char *id, const unsigned char *b, ull n) {
    fputs(id, f);
    for (ull i = 0; i < n; i++) fprintf(f, "%02X", b[i]);
    fputc('\n', f);
}

static unsigned char *buf(ull n) {
    unsigned char *p = calloc(n ? n : 1, 1);
    if (!p) { perror("ngcc_kat: calloc"); exit(1); }
    return p;
}

static int fail(const char *what, int record) {
    fprintf(stderr, "ngcc_kat: %s in record %d\n", what, record);
    return 3;
}

static void seed_nonce(unsigned char *nonce) {
    for (int k = 0; k < SEED_LEN / 4; k++) memcpy(nonce + 4 * k, "seed", 4);
}

static int run_sig(FILE *out, int records) {
    init_fn init = SYM(init_fn, "init_random_number");
    rand_fn rnd = SYM(rand_fn, "get_random_number");
    void *drng_alg = sym("drng_algorithm");
    keygen_fn keygen = SYM(keygen_fn, "sig_keygen");
    sig_sign_fn sign = SYM(sig_sign_fn, "sig_sign");
    sig_verify_fn verify = SYM(sig_verify_fn, "sig_verify");
    const ull pk_cap = SYM(len_fn, "sig_get_pk_len_bytes")();
    const ull sk_cap = SYM(len_fn, "sig_get_sk_len_bytes")();
    const ull sn_cap = SYM(len_fn, "sig_get_sn_len_bytes")();

    drng_storage seed_drng, msg_drng;
    unsigned char nonce1[SEED_LEN], nonce2[SEED_LEN] = {0}, seed[SEED_LEN];
    seed_nonce(nonce1);
    for (int k = 0; k < SEED_LEN / 3; k++) memcpy(nonce2 + 3 * k, "msg", 3);
    nonce2[SEED_LEN - 1] = 'm';
    init(&seed_drng, nonce1, SEED_LEN);
    init(&msg_drng, nonce2, SEED_LEN);

    ull pk_len = pk_cap, sk_len = sk_cap, sn_len = sn_cap;
    unsigned char *pk = buf(pk_cap), *sk = buf(sk_cap), *sn = buf(sn_cap), *m = buf(128);
    int m_len = 56;
    for (int i = 0; i < records; i++) {
        fprintf(out, "Count = %d\n", i);
        rnd(&seed_drng, seed, SEED_LEN * 8);
        rnd(&msg_drng, m, (ull)m_len * 8);
        fprintf(out, "Seed_Len = %d\n", SEED_LEN);
        pstr(out, "Seed = ", seed, SEED_LEN);
        init(drng_alg, seed, SEED_LEN);
        if (keygen(pk, &pk_len, sk, &sk_len)) return fail("sig_keygen failed", i);
        if (pk_len > pk_cap || sk_len > sk_cap) return fail("key longer than claimed", i);
        fprintf(out, "PK_Len = %llu\n", pk_len);
        pstr(out, "PK = ", pk, pk_len);
        fprintf(out, "SK_Len = %llu\n", sk_len);
        pstr(out, "SK = ", sk, sk_len);
        fprintf(out, "M_Len = %d\n", m_len);
        pstr(out, "M = ", m, (ull)m_len);
        if (sign(sk, sk_len, m, (ull)m_len, sn, &sn_len)) return fail("sig_sign failed", i);
        if (sn_len > sn_cap) return fail("signature longer than claimed", i);
        fprintf(out, "Sn_Len = %llu\n", sn_len);
        pstr(out, "Sn = ", sn, sn_len);
        if (verify(pk, pk_len, sn, sn_len, m, (ull)m_len)) return fail("sig_verify rejected", i);
        fputc('\n', out);
        fflush(out);
        m_len += 8;
    }
    return 0;
}

static int run_kem(FILE *out, int records) {
    init_fn init = SYM(init_fn, "init_random_number");
    rand_fn rnd = SYM(rand_fn, "get_random_number");
    void *drng_alg = sym("drng_algorithm");
    keygen_fn keygen = SYM(keygen_fn, "kem_keygen");
    kem_enc_fn enc = SYM(kem_enc_fn, "kem_enc");
    kem_dec_fn dec = SYM(kem_dec_fn, "kem_dec");
    const ull pk_cap = SYM(len_fn, "kem_get_pk_len_bytes")();
    const ull sk_cap = SYM(len_fn, "kem_get_sk_len_bytes")();
    const ull ss_cap = SYM(len_fn, "kem_get_ss_len_bytes")();
    const ull ct_cap = SYM(len_fn, "kem_get_ct_len_bytes")();

    drng_storage seed_drng;
    unsigned char nonce[SEED_LEN], seed[SEED_LEN];
    seed_nonce(nonce);
    init(&seed_drng, nonce, SEED_LEN);

    ull pk_len = pk_cap, sk_len = sk_cap, ss_len = ss_cap, ct_len = ct_cap;
    unsigned char *pk = buf(pk_cap), *sk = buf(sk_cap), *ss = buf(ss_cap), *ss1 = buf(ss_cap), *ct = buf(ct_cap);
    for (int i = 0; i < records; i++) {
        fprintf(out, "Count = %d\n", i);
        rnd(&seed_drng, seed, SEED_LEN * 8);
        fprintf(out, "Seed_Len = %d\n", SEED_LEN);
        pstr(out, "Seed = ", seed, SEED_LEN);
        init(drng_alg, seed, SEED_LEN);
        if (keygen(pk, &pk_len, sk, &sk_len)) return fail("kem_keygen failed", i);
        if (pk_len > pk_cap || sk_len > sk_cap) return fail("key longer than claimed", i);
        fprintf(out, "PK_Len = %llu\n", pk_len);
        pstr(out, "PK = ", pk, pk_len);
        fprintf(out, "SK_Len = %llu\n", sk_len);
        pstr(out, "SK = ", sk, sk_len);
        if (enc(pk, pk_len, ss, &ss_len, ct, &ct_len)) return fail("kem_enc failed", i);
        if (ss_len > ss_cap || ct_len > ct_cap) return fail("output longer than claimed", i);
        fprintf(out, "CT_Len = %llu\n", ct_len);
        pstr(out, "CT = ", ct, ct_len);
        fprintf(out, "SS_Len = %llu\n", ss_len);
        pstr(out, "SS = ", ss, ss_len);
        if (dec(sk, sk_len, ct, ct_len, ss1, &ss_len)) return fail("kem_dec failed", i);
        if (ss_len > ss_cap || memcmp(ss, ss1, ss_len)) return fail("shared secrets differ", i);
        fputc('\n', out);
        fflush(out);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5 || (strcmp(argv[1], "sig") && strcmp(argv[1], "kem"))) {
        fprintf(stderr, "usage: ngcc_kat sig|kem LIB.so OUT.txt [records]\n");
        return 1;
    }
    int records = argc == 5 ? atoi(argv[4]) : 10;
    if (records < 1 || records > 10) {
        fprintf(stderr, "ngcc_kat: records must be 1-10\n");
        return 1;
    }
    lib = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        fprintf(stderr, "ngcc_kat: %s\n", dlerror());
        return 2;
    }
    FILE *out = fopen(argv[3], "w");
    if (!out) {
        perror(argv[3]);
        return 1;
    }
    int rc = strcmp(argv[1], "sig") == 0 ? run_sig(out, records) : run_kem(out, records);
    fclose(out);
    return rc;
}
