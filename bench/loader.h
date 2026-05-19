#pragma once
#include <stddef.h>
#include <stdint.h>
#include "scheme.h"

/*
 * A loaded scheme: metadata copied from bench_info() + function pointers
 * resolved via dlsym.  Every scheme .so must export:
 *
 *   const bench_scheme_info_t *bench_info(void);
 *   int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
 *   int crypto_sign_signature(uint8_t *sig, size_t *siglen,
 *                             const uint8_t *m, size_t mlen,
 *                             const uint8_t *sk);
 *   int crypto_sign_verify(const uint8_t *sig, size_t siglen,
 *                          const uint8_t *m, size_t mlen,
 *                          const uint8_t *pk);
 *
 * The .so is opened with RTLD_LOCAL so non-namespaced implementations
 * cannot pollute each other's symbols.
 */
typedef struct {
    /* from bench_info() */
    const char *name;
    size_t      pk_bytes;
    size_t      sk_bytes;
    size_t      sig_bytes;
    int         iters;
    /* from dlsym */
    int (*keygen_fn)(uint8_t *pk, uint8_t *sk);
    int (*sign_fn)(uint8_t *sig, size_t *siglen,
                   const uint8_t *msg, size_t msglen,
                   const uint8_t *sk);
    int (*verify_fn)(const uint8_t *sig, size_t siglen,
                     const uint8_t *msg, size_t msglen,
                     const uint8_t *pk);
    /* internal */
    void *dl_handle;
} bench_scheme_t;

/* Returns NULL and prints an error on failure. */
bench_scheme_t *bench_load(const char *so_path);
void            bench_unload(bench_scheme_t *s);
