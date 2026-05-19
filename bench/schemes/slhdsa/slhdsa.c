/*
 * SLH-DSA (FIPS 205) adapters built on top of PQClean's namespaced
 * SPHINCS+ reference ("simple" tweak variant) implementations.
 *
 * All 12 parameter sets: {sha2,shake} × {128,192,256} × {s,f}.
 */
#include <stddef.h>
#include <stdint.h>
#include "slhdsa.h"

/* ---- forward declarations ---- */

#define DECL(NS) \
    int NS##crypto_sign_keypair(uint8_t *pk, uint8_t *sk); \
    int NS##crypto_sign_signature(uint8_t *sig, size_t *siglen, \
        const uint8_t *m, size_t mlen, const uint8_t *sk); \
    int NS##crypto_sign_verify(const uint8_t *sig, size_t siglen, \
        const uint8_t *m, size_t mlen, const uint8_t *pk)

DECL(PQCLEAN_SPHINCSSHA2128SSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHA2128FSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHA2192SSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHA2192FSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHA2256SSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHA2256FSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHAKE128SSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHAKE192SSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHAKE192FSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHAKE256SSIMPLE_CLEAN_);
DECL(PQCLEAN_SPHINCSSHAKE256FSIMPLE_CLEAN_);

#undef DECL

/* ---- thin wrappers ---- */

/* Generates static keygen/sign/verify wrappers named <short>_{kg,sg,vf}. */
#define WRAPPERS(short, NS)                                                  \
static int short##_kg(uint8_t *pk, uint8_t *sk) {                           \
    return NS##crypto_sign_keypair(pk, sk);                                  \
}                                                                            \
static int short##_sg(uint8_t *sig, size_t *slen,                           \
                      const uint8_t *m, size_t ml, const uint8_t *sk) {     \
    return NS##crypto_sign_signature(sig, slen, m, ml, sk);                  \
}                                                                            \
static int short##_vf(const uint8_t *sig, size_t slen,                      \
                      const uint8_t *m, size_t ml, const uint8_t *pk) {     \
    return NS##crypto_sign_verify(sig, slen, m, ml, pk);                     \
}

WRAPPERS(sha2_128s, PQCLEAN_SPHINCSSHA2128SSIMPLE_CLEAN_)
WRAPPERS(sha2_128f, PQCLEAN_SPHINCSSHA2128FSIMPLE_CLEAN_)
WRAPPERS(sha2_192s, PQCLEAN_SPHINCSSHA2192SSIMPLE_CLEAN_)
WRAPPERS(sha2_192f, PQCLEAN_SPHINCSSHA2192FSIMPLE_CLEAN_)
WRAPPERS(sha2_256s, PQCLEAN_SPHINCSSHA2256SSIMPLE_CLEAN_)
WRAPPERS(sha2_256f, PQCLEAN_SPHINCSSHA2256FSIMPLE_CLEAN_)
WRAPPERS(shake_128s, PQCLEAN_SPHINCSSHAKE128SSIMPLE_CLEAN_)
WRAPPERS(shake_128f, PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_)
WRAPPERS(shake_192s, PQCLEAN_SPHINCSSHAKE192SSIMPLE_CLEAN_)
WRAPPERS(shake_192f, PQCLEAN_SPHINCSSHAKE192FSIMPLE_CLEAN_)
WRAPPERS(shake_256s, PQCLEAN_SPHINCSSHAKE256SSIMPLE_CLEAN_)
WRAPPERS(shake_256f, PQCLEAN_SPHINCSSHAKE256FSIMPLE_CLEAN_)

#undef WRAPPERS

/* ---- scheme table ---- */

/* iters: 0 = use global BENCH_ITER; reduced for slow -s variants */
#define ENTRY(short, label, pkb, skb, sigb, its) { \
    .name      = label, \
    .pk_bytes  = pkb, \
    .sk_bytes  = skb, \
    .sig_bytes = sigb, \
    .iters     = its, \
    .keygen_fn = short##_kg, \
    .sign_fn   = short##_sg, \
    .verify_fn = short##_vf, \
}

bench_scheme_t slhdsa_schemes[] = {
    /* SHA2 variants — ordered by security level then speed class.
     * -s variants use fewer iterations: clean-ref signing is expensive
     * (keygen for -256s can exceed 1B cycles on the reference impl). */
    ENTRY(sha2_128s,  "SLH-DSA-SHA2-128s",   32,  64,  7856,  20),
    ENTRY(sha2_128f,  "SLH-DSA-SHA2-128f",   32,  64, 17088, 200),
    ENTRY(sha2_192s,  "SLH-DSA-SHA2-192s",   48,  96, 16224,  20),
    ENTRY(sha2_192f,  "SLH-DSA-SHA2-192f",   48,  96, 35664, 200),
    ENTRY(sha2_256s,  "SLH-DSA-SHA2-256s",   64, 128, 29792,  20),
    ENTRY(sha2_256f,  "SLH-DSA-SHA2-256f",   64, 128, 49856, 200),
    /* SHAKE variants */
    ENTRY(shake_128s, "SLH-DSA-SHAKE-128s",  32,  64,  7856,  20),
    ENTRY(shake_128f, "SLH-DSA-SHAKE-128f",  32,  64, 17088, 200),
    ENTRY(shake_192s, "SLH-DSA-SHAKE-192s",  48,  96, 16224,  20),
    ENTRY(shake_192f, "SLH-DSA-SHAKE-192f",  48,  96, 35664, 200),
    ENTRY(shake_256s, "SLH-DSA-SHAKE-256s",  64, 128, 29792,  20),
    ENTRY(shake_256f, "SLH-DSA-SHAKE-256f",  64, 128, 49856, 200),
};

#undef ENTRY

const size_t n_slhdsa_schemes = sizeof(slhdsa_schemes) / sizeof(slhdsa_schemes[0]);
