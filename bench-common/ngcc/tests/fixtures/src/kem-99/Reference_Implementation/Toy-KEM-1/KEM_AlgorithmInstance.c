/* Toy KEM: pk = sk, ct = ss XOR pk. Tests only. */
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

#define TOY 32

unsigned long long kem_get_pk_len_bytes(void) { return TOY; }
unsigned long long kem_get_sk_len_bytes(void) { return TOY; }
unsigned long long kem_get_ss_len_bytes(void) { return TOY; }
unsigned long long kem_get_ct_len_bytes(void) { return TOY; }

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes) {
    if (get_random_number(&drng_algorithm, sk, TOY * 8)) return -1;
    memcpy(pk, sk, TOY);
    *pk_len_bytes = TOY;
    *sk_len_bytes = TOY;
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
    if (pk_len_bytes != TOY || get_random_number(&drng_algorithm, ss, TOY * 8)) return -1;
    for (int i = 0; i < TOY; i++) ct[i] = ss[i] ^ pk[i];
    *ss_len_bytes = TOY;
    *ct_len_bytes = TOY;
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
    if (sk_len_bytes != TOY || ct_len_bytes != TOY) return -1;
    for (int i = 0; i < TOY; i++) ss[i] = ct[i] ^ sk[i];
    *ss_len_bytes = TOY;
    return 0;
}
