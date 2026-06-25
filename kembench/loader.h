#pragma once
#include <stddef.h>
#include <stdint.h>
#include "scheme.h"

/*
 * A loaded KEM scheme: metadata copied from bench_info() + function pointers
 * resolved via dlsym.  Every scheme .so must export:
 *
 *   const bench_scheme_info_t *bench_info(void);
 *
 *   int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
 *   int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
 *   int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
 *
 * These are the standard NIST KEM API names.  All return 0 on success.
 *
 * The .so is opened with RTLD_LOCAL so non-namespaced implementations
 * (e.g. a bare crypto_kem_keypair in ML-KEM and another in HQC) cannot
 * pollute each other's symbols.
 */
typedef struct {
    /* from bench_info() */
    const char *name;
    size_t      pk_bytes;
    size_t      sk_bytes;
    size_t      ct_bytes;
    size_t      ss_bytes;
    int         iters;
    /* from dlsym */
    int (*keygen_fn)(uint8_t *pk, uint8_t *sk);
    int (*enc_fn)(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
    int (*dec_fn)(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
    /* internal */
    void *dl_handle;
} bench_scheme_t;

/* Returns NULL and prints an error on failure. */
bench_scheme_t *bench_load(const char *so_path);
void            bench_unload(bench_scheme_t *s);
