#pragma once
#include <stddef.h>
#include <stdint.h>

/*
 * Pluggable scheme interface.
 *
 * Each scheme provides one or more bench_scheme_t instances (one per
 * parameter set). Adapters implement the three function pointers and expose
 * a scheme array + count. Only scheme.h needs to be included by adapters.
 */
typedef struct {
    const char *name;
    size_t      pk_bytes;
    size_t      sk_bytes;
    size_t      sig_bytes;

    int (*keygen_fn)(uint8_t *pk, uint8_t *sk);
    int (*sign_fn)(uint8_t *sig, size_t *siglen,
                   const uint8_t *msg, size_t msglen,
                   const uint8_t *sk);
    int (*verify_fn)(const uint8_t *sig, size_t siglen,
                     const uint8_t *msg, size_t msglen,
                     const uint8_t *pk);
} bench_scheme_t;
