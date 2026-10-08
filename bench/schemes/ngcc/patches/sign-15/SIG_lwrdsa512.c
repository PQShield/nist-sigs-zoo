/* PATCH (nist-sigs-zoo): sig_sign wrote a detached signature of CRYPTO_BYTES bytes (pack_sig) but reported
 * *sn_len_bytes = m_len_bytes + CRYPTO_BYTES, as if the message were appended. The reported length exceeds the
 * sig_get_sn_len_bytes() buffer, so callers read m_len_bytes bytes past it (docs/ngcc-submission-bugs.md item 2).
 * This copy reports CRYPTO_BYTES, the bytes actually written; nothing else is changed. */
/*
The software is provided by the Institute of Commercial Cryptography Standards (ICCS), and is used for algorithm submissions in the Next-generation Commercial Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be uninterrupted or error-free in all cases. ICCS will take no responsibility for the use of the software or the results thereof, if the software is used for any other purposes.
*/
#include <stdint.h>
#include "api.h"
#include "params.h"
#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "stdio.h"
#include <stdlib.h>
#include "auxfunc.h"

#define ROUND(a) ((((a) & (Q - 1)) + (1 << (QBITS - PBITS - 1))) >> (QBITS - PBITS))

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes()
{
  return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
  return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes(){
  return CRYPTO_BYTES;
}

/*************************************************
 * Name:        expand_mat
 *
 * Description: Implementation of ExpandA. Generates matrix A with uniformly
 *              random coefficients a_{i,j} by performing rejection sampling
 *              on the output stream of SM3_XOF_168(rho|i|j).
 *
 * Arguments:   - polyvecl mat[K]: output matrix
 *              - const unsigned char rho[]: byte array containing seed rho
 **************************************************/
void expand_mat(polyvecl mat[K], const unsigned char rho[SEEDBYTES]) {
  unsigned int i, j, pos, ctr;
  unsigned char inbuf[SEEDBYTES + 1];
  /* Don't change this to smaller values,
   * sampling later assumes sufficient SM3_XOF_168 output!
   * Probability that we need more than 5 blocks: < 2^{-132}.
   * Probability that we need more than 6 blocks: < 2^{-546}.
   */
  unsigned char outbuf[15*XOF_168];
  uint32_t val;

  for(i = 0; i < SEEDBYTES; ++i)
    inbuf[i] = rho[i];

  for(i = 0; i < K; ++i) {
    for(j = 0; j < L; ++j) {
      ctr = pos = 0;
      inbuf[SEEDBYTES] = i + (j << 4);
      pseudoXOF(
          sizeof(outbuf) * 8,
          inbuf,
          (SEEDBYTES + 1) * 8,
          outbuf
          );

      while(ctr < N) {
        val  = outbuf[pos++];
        val |= (uint32_t)outbuf[pos++] << 8;
        val |= (uint32_t)outbuf[pos++] << 16;
        val &= 0x1FFFFFF;

        /* Rejection sampling */
        if(val < Q)
        {
          mat[i].vec[j].coeffs[ctr++] = val;
        }

      }
    }
  }

}

/*************************************************
 * Name:        challenge
 *
 * Description: Implementation of H. Samples polynomial with kappa
 *              nonzero coefficients in {-1,1} using the output stream
 *              of SM3_XOF_136(mu|w1).
 *
 * Arguments:   - poly *c: pointer to output polynomial
 *              - const unsigned char mu[]: byte array containing mu
 *              - const polyveck *w1: pointer to vector w1
 **************************************************/
void challenge(poly *c,
    const unsigned char mu[CRHBYTES],
    const polyveck *w1)
{
  unsigned int i, b, pos;
  unsigned char inbuf[CRHBYTES + K*POLW1_SIZE_PACKED];

  /* Large buffer to simulate XOF stream */
  unsigned char outbuf[XOF_136];  // must be large enough!
  uint64_t signs, mask;

  /* Prepare input = mu || w1 */
  for(i = 0; i < CRHBYTES; ++i)
    inbuf[i] = mu[i];

  for(i = 0; i < K; ++i)
    polyw1_pack(inbuf + CRHBYTES + i*POLW1_SIZE_PACKED, w1->vec+i);

  /* Replace SHAKE256 with pseudoXOF */
  pseudoXOF(
      sizeof(outbuf) * 8,
      inbuf,
      sizeof(inbuf) * 8,
      outbuf
      );

  /* Extract signs */
  signs = 0;
  for(i = 0; i < 8; ++i)
    signs |= (uint64_t)outbuf[i] << (8*i);

  pos = 8;
  mask = 1;

  for(i = 0; i < N; ++i)
    c->coeffs[i] = 0;

  for(i = N-KAPPA; i < N; ++i) {
    do {
      if(pos >= sizeof(outbuf)) {
        pseudoXOF(
            sizeof(outbuf) * 8,
            inbuf,
            (sizeof(inbuf)) * 8,
            outbuf
            );

        pos = 0;
      }

      b = outbuf[pos++];
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = (signs & mask) ? Q - 1 : 1;
    mask <<= 1;
  }
}

/*************************************************
 * Name:        sig_keygen
 *
 * Description: Generates public and private key
 *
 * Arguments:   - unsigned char *pk: pointer to output public key (allocated
 *                                   array of CRYPTO_PUBLICKEYBYTES bytes)
 *              - unsigned char *sk: pointer to output private key (allocated
 *                                   array of CRYPTO_SECRETKEYBYTES bytes)
 *
 * Returns 0 (success)
 **************************************************/
int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes) {
  unsigned int i;
  unsigned char seedbuf[3*SEEDBYTES];
  unsigned char tr[CRHBYTES];
  unsigned char *rho, *rhoprime, *key;
  uint16_t nonce = 0;
  polyvecl mat[K];
  polyvecl s1;
  polyveck t, t1, t0;


  *pk_len_bytes = sig_get_pk_len_bytes();
  *sk_len_bytes = sig_get_sk_len_bytes();
  /* Expand 32 bytes of randomness into rho, rhoprime and key */
  /// @brief Generate Pseudo Random Number (PRN)
  /// @param[in] drng Initialized DRNG
  /// @param[out] random_number The base address to save the generated PRN
  /// @param[in] random_number_len_bits The valid BITS of output PRN
  /// @return 0 for success, others for error
  if(get_random_number(&drng_algorithm, seedbuf, SEEDBYTES * 8) != 0) {
    fprintf(stderr, "Error: random generation failed\n");
    exit(EXIT_FAILURE);
  }



  pseudoXOF(
      3 * SEEDBYTES * 8,   // output bits
      seedbuf,             // input
      SEEDBYTES * 8,       // input bits
      seedbuf              // output (overwrite)
      );
  rho = seedbuf;
  rhoprime = rho + SEEDBYTES;
  key = rho + 2*SEEDBYTES;

  /* Expand matrix */
  expand_mat(mat, rho);

  /* Sample short vectors s1 and s2 */
  for(i = 0; i < L; ++i)
    poly_uniform_eta(&s1.vec[i], rhoprime, nonce++);

  polyvec_l_mul(mat,&s1,&t);//Newly added

  for (i = 0; i < K; ++i)
    for (unsigned int j = 0; j < N; ++j)
    {
      uint32_t temp = ROUND(t.vec[i].coeffs[j]);// Newly Added
                                                //s2.vec[i].coeffs[j] = (temp << (QBITS - PBITS)) - t.vec[i].coeffs[j];// Newly Added
      t.vec[i].coeffs[j] = temp;
    }

  /* Extract t1 and write public key */
  polyveck_power2round(&t1, &t0, &t);
  pack_pk(pk, rho, &t1);//Complete

  polyveck temp = t1;
  polyveck_shiftl(&temp, D);
  polyveck_add(&temp, &temp, &t0);
  polyveck_freeze(&temp, Q);

  /* Compute CRH(rho, t1) and write secret key */
  pseudoXOF(
      CRHBYTES * 8,   // output bits
      pk,             // input
      CRYPTO_PUBLICKEYBYTES * 8,       // input bits
      tr              // output
      );

  pack_sk(sk, rho, key, tr, &s1, &t0);

  return 0;
}

/*************************************************
 * Name:        crypto_sign
 *
 * Description: Compute signed message
 *
 * Arguments:   - const unsigned char *sk: pointer to bit-packed secret key
 *              - unsigned long long sk_len_bytes: length of secret key
 *              - const unsigned char *m: pointer to message to be signed
 *              - unsigned long long m_len_bytes: length of message
 *              - unsigned char *sn: pointer to output signed message (allocated
 *                                   array with CRYPTO_BYTES + mlen bytes)
 *              - unsigned long long *smsn_len_bytes: pointer to output length
 *                                                    of signed message
 *
 *
 *
 *
 * Returns 0 (success)
 **************************************************/
int sig_sign(const unsigned char *sk, unsigned long long sk_len_bytes, const unsigned char *m, unsigned long long m_len_bytes, unsigned char *sn, unsigned long long *sn_len_bytes)
{
  unsigned long long i, j;
  unsigned int n;
  unsigned char seedbuf[2*SEEDBYTES + CRHBYTES];
  unsigned char *rho, *key, *mu, *buf;
  uint16_t nonce = 0;
  poly     c, chat;
  polyvecl mat[K], s1, y, yhat, z;
  polyveck s2, t0, w, w1;
  polyveck h, wcs2, wcs20, ct0, tmp;
  polyveck xi1, xi2, nu;// NewlyAdded
  polyveck t;

  rho = seedbuf;
  key = seedbuf + SEEDBYTES;
  mu = seedbuf + 2*SEEDBYTES;

  buf = (unsigned char *)calloc(CRHBYTES + m_len_bytes, sizeof(unsigned char));

  unpack_sk(rho, key, buf, &s1, &t0, sk);

//printf("In sign1 \n");
  /* Copy message at the end of the sn buffer */
  for(i = 0; i < m_len_bytes; ++i)
    buf[CRHBYTES + i] = m[i];

  /* Compute CRH(tr, msg) */
  pseudoXOF(
      CRHBYTES * 8,   // output bits
      buf,            // input
      (CRHBYTES + m_len_bytes) * 8,       // input bits
      mu              // output
      );

  /* Expand matrix and transform vectors */
  expand_mat(mat, rho); // The matrix A

//printf("In sign2 \n");
  /*----------------------- Calculate s2 as given in Step 3 of Sign algorithm -----------------------------*/

  polyvec_l_mul(mat, &s1, &t); // t = A*s1

  for (i = 0; i < K; ++i)
    for (unsigned int j = 0; j < N; ++j)
    {
      uint32_t temp = ROUND(t.vec[i].coeffs[j]);// Newly Added
      s2.vec[i].coeffs[j] = (temp << (QBITS - PBITS)) - t.vec[i].coeffs[j];// Newly Added
      t.vec[i].coeffs[j] = temp;
    }

  /*
   * Now t = round(A*s1)
   * And s2 = q/p*t - A*s1
   */


rej:
  /* Sample intermediate vector y */
  for(i = 0; i < L; ++i)
    poly_uniform_gamma1m1(y.vec+i, key, nonce++); // Sampling y

  /* Matrix-vector multiplication */
  yhat = y;

	//printf("In sign3 \n");

  polyvec_l_mul(mat,&yhat,&w);//Newly added
                              // Now w = A*y

	//printf("In sign4 \n");

  /*----------------STEP NUMBER 7(Rounding) HAS TO BE DONE HERE-----*/
  for (i = 0; i < K; ++i)
    for (j = 0; j < N; ++j)
    {
      uint32_t temp = ROUND(w.vec[i].coeffs[j]);
      xi1.vec[i].coeffs[j] = (temp << (QBITS - PBITS)) - w.vec[i].coeffs[j];
      w.vec[i].coeffs[j] = temp;
    }

  /*
   * Now w = round(A*y)
   * And xi1 = q/p*w - A*y
   */

  /* Decompose w and call the random oracle */
  polyveck_freeze(&w, P);
  polyveck_decompose(&w1, &tmp, &w); // w = p/gamma1_bar*w1+tmp
  challenge(&c, mu, &w1); // c = H(mu||w1)


  /* Compute z, reject if it reveals secret */
  chat = c;

  for(i = 0; i < L; ++i) {
    pol_mul(z.vec+i, &chat, s1.vec+i);//Newly added
  }
  // z = c*s1

  polyvecl_add(&z, &z, &y); // z = y+cs1
  polyvecl_freeze(&z, Q);
  //printf("In sign5 \n");
  if(polyvecl_chknorm(&z, GAMMA1 - BETA1, Q))//Changed
    goto rej;


  /*----------------STEP NUMBER 11 and 12 HAS TO BE DONE HERE-----*/
  /* Compute w - cs2, reject if w1 can not be computed from it */
  for(i = 0; i < K; ++i) {
    pol_mul(wcs2.vec+i, &chat, s2.vec+i);
  }
  // wcs2 = c*s2

  for (i = 0; i < K; ++i)
    for (j = 0; j < N; ++j) {
      uint32_t temp = ROUND(wcs2.vec[i].coeffs[j]);
      xi2.vec[i].coeffs[j] = (temp << (QBITS - PBITS)) - wcs2.vec[i].coeffs[j];
      // The subtraction is in mod Q
      nu.vec[i].coeffs[j] = ROUND(xi2.vec[i].coeffs[j] + Q - xi1.vec[i].coeffs[j]);
      wcs2.vec[i].coeffs[j] = temp;
    }

  /*
   * Now wcs2 = round(c*s2)
   * And xi2 = q/p*wcs2 - c*s2
   * And nu = round(xi2 - xi1)
   */

  polyveck_sub(&wcs2, &w, &wcs2); // wcs2 = w-round(c*s2)
  polyveck_add(&wcs2, &wcs2, &nu);// Newly Added and wcs2 = w-round(c*s2)+nu
  polyveck_freeze(&wcs2, P);
  polyveck_decompose(&tmp, &wcs20, &wcs2); // wcs2 = p/gamma1_bar*tmp+wcs20
  polyveck_freeze(&wcs20, P);

  if(polyveck_chknorm(&wcs20, GAMMA2_BAR - BETA2, P)) //Changed
    goto rej;

  for(i = 0; i < K; ++i)
    for(j = 0; j < N; ++j)
      if(tmp.vec[i].coeffs[j] != w1.vec[i].coeffs[j])
        goto rej;


  /* Compute hints for w1 */
  for(i = 0; i < K; ++i) {
    pol_mul(ct0.vec+i, &chat, t0.vec+i);

  }
  // ct0 = c*t0

  polyveck_freeze(&ct0, Q);
  if(polyveck_chknorm(&ct0, GAMMA2_BAR, Q)) 
    goto rej;


  polyveck_add(&tmp, &wcs2, &ct0); // tmp = wcs2 + ct0
  polyveck_neg(&ct0); // ct0 = -c*t0
  polyveck_freeze(&tmp, P);
  n = polyveck_make_hint(&h, &tmp, &ct0);
  if(n > OMEGA)
    goto rej;


  /* Write signature */
  pack_sig(sn, &z, &h, &c);

  *sn_len_bytes = CRYPTO_BYTES;

  return 0;
}

/*************************************************
 * Name:        crypto_sign_open
 *
 * Description: Verify signed message
 *
 * Arguments:   - const unsigned char *pk: pointer to bit-packed public key
 *              - unsigned long long pk_len_bytes: length of public key
 *              - const unsigned char *sn: pointer to signed message
 *              - unsigned long long sn_len_bytes: length of signed message
 *              - unsigned char *m: pointer to output message (allocated
 *                                  array with CRYPTO_BYTES + mlen bytes),
 *                                  can be equal to sm
 *              - unsigned long long *m_len_bytes: pointer to output length of message
 *
 *
 * Returns 0 if signed message could be verified correctly and -1 otherwise
 **************************************************/
int sig_verify(const unsigned char *pk, unsigned long long pk_len_bytes, const unsigned char *sn, unsigned long long sn_len_bytes,unsigned char *m, unsigned long long m_len_bytes)
{
  unsigned long long i;
  unsigned char rho[SEEDBYTES];
  unsigned char mu[CRHBYTES];
  poly     c, chat, cp;
  polyvecl mat[K], z;
  polyveck t1, w1, h, tmp1, tmp2;
  unsigned char *buf = NULL;

  //*pk_len_bytes = sig_get_pk_len_bytes();

  if(sn_len_bytes < CRYPTO_BYTES)
    goto badsig;

  buf = (unsigned char *)calloc(CRHBYTES + m_len_bytes, sizeof(unsigned char));

  unpack_pk(rho, &t1, pk);
  unpack_sig(&z, &h, &c, sn);

  if(polyvecl_chknorm(&z, GAMMA1 - BETA1, Q))//Changed
    goto badsig;

  /* Compute CRH(CRH(rho, t1), msg) using buf as "playground" buffer */

  if(sn != m)
    for(i = 0; i < m_len_bytes; ++i)
      buf[CRHBYTES + i] = m[i];


  pseudoXOF(
      CRHBYTES * 8,   // output bits
      pk,             // input
      (CRYPTO_PUBLICKEYBYTES) * 8,       // input bits
      buf             // output
      );

  pseudoXOF(
      CRHBYTES * 8,   // output bits
      buf,            // input
      (CRHBYTES + m_len_bytes) * 8,       // input bits
      mu              // output
      );
  expand_mat(mat, rho);

  polyvec_l_mul(mat, &z, &tmp1);//Newly added

  /*---------VERIFY PARTIAL STEP 2(ROUNDING) HAS TO BE DONE HERE------*/
  for (i = 0; i < K; i++)
    for (unsigned int j = 0; j < N; ++j)
      tmp1.vec[i].coeffs[j] = ROUND(tmp1.vec[i].coeffs[j]);

  chat = c;
  polyveck_shiftl(&t1, D);


  for(i = 0; i < K; ++i)
    pol_mul(tmp2.vec+i, &chat, t1.vec+i); //Newly added

  polyveck_freeze(&tmp2, Q);

  polyveck_sub(&tmp1, &tmp1, &tmp2);
  polyveck_freeze(&tmp1, P);

  /* Reconstruct w1 */
  polyveck_use_hint(&w1, &tmp1, &h);


  /* Call random oracle and verify challenge */
  challenge(&cp, mu, &w1);
  for(i = 0; i < N; ++i)
    if(c.coeffs[i] != cp.coeffs[i])
      goto badsig;

  /* All good, copy msg, return 0 */
  for(i = 0; i < m_len_bytes; ++i)
    m[i] = sn[CRYPTO_BYTES + i];

  free(buf);

  return 0;

  /* Signature verification failed */
badsig:

  if (buf != NULL)
    free(buf);

  return -1;
}
