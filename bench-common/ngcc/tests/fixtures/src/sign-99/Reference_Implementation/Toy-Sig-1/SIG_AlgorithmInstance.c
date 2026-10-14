/* Toy signature: pk = sk, signature = keyed mix of the message. Tests only. */
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

#define TOY 32
#ifdef TOY_LONG_PK
#define TOY_EXTRA 1   /* reports one byte more than it claims: the shim must refuse */
#else
#define TOY_EXTRA 0
#endif

unsigned long long sig_get_pk_len_bytes(void) { return TOY; }
unsigned long long sig_get_sk_len_bytes(void) { return TOY; }
unsigned long long sig_get_sn_len_bytes(void) { return TOY; }

#ifdef TOY_CXX  /* the mix comes from cxx/toy_tag.cpp (C++ build test) */
void toy_tag_cxx(const unsigned char *key, const unsigned char *m, unsigned long long mlen, unsigned char *tag, unsigned n);
static void toy_tag(const unsigned char *key, const unsigned char *m, unsigned long long mlen, unsigned char *tag) {
    toy_tag_cxx(key, m, mlen, tag, TOY);
}
#else
static void toy_tag(const unsigned char *key, const unsigned char *m, unsigned long long mlen, unsigned char *tag) {
    memcpy(tag, key, TOY);
    for (unsigned long long j = 0; j < mlen; j++)
        tag[j % TOY] = (unsigned char)(tag[j % TOY] * 31u + m[j] + 1u);
}
#endif

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes) {
    if (get_random_number(&drng_algorithm, sk, TOY * 8)) return -1;
    memcpy(pk, sk, TOY);
    *pk_len_bytes = TOY + TOY_EXTRA;
    *sk_len_bytes = TOY;
    return 0;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes) {
    if (sk_len_bytes != TOY) return -1;
    toy_tag(sk, m, m_len_bytes, sn);
    *sn_len_bytes = TOY;
    return 0;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes) {
    unsigned char t[TOY];
    if (pk_len_bytes < TOY || sn_len_bytes != TOY) return -1;
    toy_tag(pk, m, m_len_bytes, t);
    return memcmp(t, sn, TOY) ? -1 : 0;
}
