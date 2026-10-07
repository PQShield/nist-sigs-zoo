/* Toy NGCC signature instance for the adapter tests (insecure by design). */
#ifndef SIG_ALGORITHM_INSTANCE_H
#define SIG_ALGORITHM_INSTANCE_H
#define OUTPUT_BLANK_TEST_VECTORS 1
#define ALGORITHM_INSTANCE "Toy-Sig-1"
unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes);
int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes);
int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes);
#endif
