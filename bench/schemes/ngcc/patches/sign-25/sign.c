/* PATCH (nist-sigs-zoo, fix): protocols_verif_internal did not check whether the final splitting
 * step of the (2,2)-isogeny chain succeeded (theta_isogenies.c only asserts it, and the release
 * build's -DNDEBUG removes the assert), and then derived the codomain curves from an uninitialised
 * stack struct (theta_chain_t isog). A tampered signature could therefore be accepted when the
 * stack still held the state of an earlier valid verification in the same process (reported on
 * https://ngcc.dev/ as sign-25-1). This copy zero-initialises isog and calls the *_checked
 * variants of the chain functions (same computation, also return whether the chain splits;
 * patched theta_isogenies.c), rejecting the signature when it does not split. Nothing else is
 * changed. */
#include <sqisigndim2.h>
#include <curve_extras.h>
#include <tools.h>
#include <stdio.h>
#include <string.h>
#include <klptx.h>
#include <config.h>
#include <auxfunc.h>

/* PATCH (nist-sigs-zoo, fix): defined in the patched theta_isogenies.c */
int theta_chain_comput_strategy_checked(theta_chain_t *out,
	int n,
	theta_couple_curve_t *E12,
	const theta_couple_point_t *T1,
	const theta_couple_point_t *T2,
	const theta_couple_point_t *T1m2,
	int *strategy,
	int eight_above);
int theta_chain_comput_strategy_faster_no_eval_checked(theta_chain_t *out,
	int n,
	theta_couple_curve_t *E12,
	const theta_couple_point_t *T1,
	const theta_couple_point_t *T2,
	const theta_couple_point_t *T1m2,
	int *strategy,
	int eight_above);

int check_eideal_norm(ibz_t* norm_mu,
    int* flag,
    const ibz_t* eideal_norm,
    const ibz_t* N);

void
secret_sig_init(signature_t *sig)
{
	memset(sig, 0, sizeof(*sig));
#if COMPRESSED == 0
    ibz_mat_2x2_init(&(sig->mat_Bchall_can_to_B_chall));
#elif COMPRESSED == 1
	ibz_mat_2x2_init(&(sig->mat_Bpk_can_to_B_pk));
#endif
}

void
secret_sig_finalize(signature_t *sig)
{
#if COMPRESSED == 0
    ibz_mat_2x2_finalize(&(sig->mat_Bchall_can_to_B_chall));
#elif COMPRESSED == 1
	ibz_mat_2x2_finalize(&(sig->mat_Bpk_can_to_B_pk));
#endif
}

// compute the commitment with ideal to isogeny clapotis
// and apply it to the basis of E0 (together with the multiplication by some scalar u)
// the scalar adjusting_factor is a scalar through which the points of the basis are multiplied
void
commit(ec_curve_t* E_com,
    ec_basis_t* basis_even_com,
    ec_basis_t* basis_three_com,
    quat_left_ideal_t* lideal_com)
{
    int found;
    ibz_t n, pow_two;
    ec_basis_t basis_two, basis_three;

    ibz_init(&n);
    ibz_init(&pow_two);

    // Generate N_com below p^(1/2).
	// TODO: discuss whether this generation method is appropriate.
    //mpz_sqrt(&pow_two, &QUATALG_PINFTY.p);
	ibz_pow(&pow_two, &ibz_const_two, TORSION_PLUS_EVEN_POWER - 2);
    generate_random_prime_kygen(&n, &pow_two);


    theta_chain_t isog;

    found = fixed_degree_isogeny(&isog, lideal_com, &n, 1);
    (void)found;
    assert(found);
    (void)found;

    copy_point(&(basis_two.P), &BASIS_EVEN.P);
    copy_point(&(basis_two.Q), &BASIS_EVEN.Q);
    copy_point(&(basis_two.PmQ), &BASIS_EVEN.PmQ);
    copy_point(&(basis_three.P), &(BASIS_THREE.P));
    copy_point(&(basis_three.Q), &(BASIS_THREE.Q));
    copy_point(&(basis_three.PmQ), &(BASIS_THREE.PmQ));

#if _DEBUG
	ec_curve_t E0 = CURVE_E0;
	assert(test_point_order_twof(&(basis_two.P), &E0, TORSION_PLUS_EVEN_POWER));
	assert(test_point_order_twof(&(basis_two.Q), &E0, TORSION_PLUS_EVEN_POWER));
	assert(test_point_order_twof(&(basis_two.PmQ), &E0, TORSION_PLUS_EVEN_POWER));
	assert(test_point_order_threef(&(basis_three.P), &E0, TORSION_PLUS_THREE_POWER));
	assert(test_point_order_threef(&(basis_three.Q), &E0, TORSION_PLUS_THREE_POWER));
	assert(test_point_order_threef(&(basis_three.PmQ), &E0, TORSION_PLUS_THREE_POWER));
#endif

    theta_couple_point_t V1, V2, V1m2;
    theta_couple_point_t P, Q, PmQ;
    theta_couple_curve_t E01;

    E01.E1 = isog.domain.E1;
    E01.E2 = isog.domain.E2;

    copy_point(&(P.P1), &(basis_two.P));
    copy_point(&(Q.P1), &(basis_two.Q));
    copy_point(&(PmQ.P1), &(basis_two.PmQ));
    ec_set_zero(&P.P2);
    ec_set_zero(&Q.P2);
    ec_set_zero(&PmQ.P2);

    theta_chain_eval_special_case(&V1, &isog, &P, &E01);
    theta_chain_eval_special_case(&V2, &isog, &Q, &E01);
    theta_chain_eval_special_case(&V1m2, &isog, &PmQ, &E01);

#if _DEBUG
	{
		fp2_t w0, w1, test_pow;
		ec_point_t AC, A24;
		ibz_t temp;
		ibz_init(&temp);
		fp2_copy(&AC.x, &(E01.E1.A));
		fp2_copy(&AC.z, &(E01.E1.C));
		A24_from_AC(&A24, &AC);
		weil(&w0, TORSION_PLUS_EVEN_POWER, &(basis_two.P), &(basis_two.Q), &(basis_two.PmQ), &A24);
		copy_point(&(basis_two.P), &V1.P1);
		copy_point(&(basis_two.Q), &V2.P1);
		copy_point(&(basis_two.PmQ), &V1m2.P1);
		fp2_copy(&AC.x, &(isog.codomain.E1.A));
		fp2_copy(&AC.z, &(isog.codomain.E1.C));
		A24_from_AC(&A24, &AC);
		weil(&w1, TORSION_PLUS_EVEN_POWER, &(basis_two.P), &(basis_two.Q), &(basis_two.PmQ), &A24);
		digit_t digit[NWORDS_ORDER_2] = { 0 };
		ibz_sub(&temp, &pow_two, &n);
		ibz_to_digit_array(digit, &temp);
		fp2_pow_vartime(&test_pow, &w0, digit, NWORDS_ORDER_2);
		assert(fp2_is_equal(&test_pow, &w1));
		ibz_finalize(&temp);
	}
#endif

	copy_curve(E_com, &isog.codomain.E2);
	copy_point(&(basis_even_com->P), &V1.P2);
	copy_point(&(basis_even_com->PmQ), &V1m2.P2);
	copy_point(&(basis_even_com->Q), &V2.P2);
	copy_point(&(P.P1), &(basis_three.P));
	copy_point(&(Q.P1), &(basis_three.Q));
	copy_point(&(PmQ.P1), &(basis_three.PmQ));
	ec_set_zero(&P.P2);
	ec_set_zero(&Q.P2);
	ec_set_zero(&PmQ.P2);
	theta_chain_eval_special_case(&V1, &isog, &P, &E01);
	theta_chain_eval_special_case(&V2, &isog, &Q, &E01);
	theta_chain_eval_special_case(&V1m2, &isog, &PmQ, &E01);
	copy_point(&(basis_three_com->P), &V1.P2);
	copy_point(&(basis_three_com->PmQ), &V1m2.P2);
	copy_point(&(basis_three_com->Q), &V2.P2);

    ibz_finalize(&n);
    ibz_finalize(&pow_two);
}


// Conjugate a left ideal as a lattice.
void 
quat_lideal_conjugate_lattice(quat_lattice_t *lat, const quat_left_ideal_t *lideal)
{
    ibz_mat_4x4_copy(&(lat->basis), &(lideal->lattice.basis));
    ibz_copy(&(lat->denom), &(lideal->lattice.denom));

    for (int row = 1; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            ibz_neg(&(lat->basis[row][col]), &(lat->basis[row][col]));
        }
    }

    return;
}


void
norm_from_2_times_gram(ibz_t *norm, ibz_mat_4x4_t *gram, ibz_vec_4_t *vec)
{
    quat_qf_eval(norm, gram, vec);
    assert(ibz_is_even(norm));
    ibz_div_2exp(norm, norm, 1);
}

// TODECIDE : is the current sampling method satisfactory ?
// it seems uniformish but not sure the distribution is exactly as we might want it
int
sample_response(ibz_t* norm_mu,
    int* flag,
    ibz_t* eideal_norm,
    const ibz_t* N,
    quat_alg_elem_t* x,
    const quat_lattice_t* lattice,
    ibz_t const* lattice_content)
{
    ibz_mat_4x4_t lll;
    ibq_t temp_norm;
    ibz_t denom_gram, norm, temp, content;
    ibz_t bound;
    ibz_vec_4_t vec;
	quat_alg_coord_t coord;

    ibz_mat_4x4_init(&lll);
    ibq_init(&temp_norm);
    ibz_init(&denom_gram);
    ibz_init(&norm);
    ibz_init(&temp);
    ibz_init(&bound);
	ibz_init(&content);
    ibz_vec_4_init(&vec);
	quat_alg_coord_init(&coord);

    int err = quat_lattice_lll(&lll, lattice, &(QUATALG_PINFTY.p));
    assert(!err);

    // The shortest vector found by lll is our response
    ibz_mat_4x4_t prod, gram;
    ibz_mat_4x4_init(&prod);
    ibz_mat_4x4_init(&gram);

    ibz_mat_4x4_transpose(&prod, &lll);
    ibz_mat_4x4_mul(&prod, &prod, &(QUATALG_PINFTY.gram));
    ibz_mat_4x4_mul(&gram, &prod, &lll);

    ibz_copy(&denom_gram, &(lattice->denom));
    ibz_mul(&denom_gram, &denom_gram, &(lattice->denom));
    ibz_mul(&denom_gram, &denom_gram, lattice_content);
    assert(ibz_is_even(&denom_gram));
    ibz_div_2exp(&denom_gram, &denom_gram, 1);

    int divides = ibz_mat_4x4_scalar_div(&gram, &denom_gram, &gram);
    assert(divides);

    ibz_copy(&(x->denom), &(lattice->denom));

    int found = 0;
    int count = 0;

    ibz_vec_4_t b_bound;
    ibz_vec_4_init(&b_bound);

    ibz_pow(&bound, &ibz_const_two, SQIsign2D_response_length);

    // computing the upperbounds for the coefficients of the scalar decomposition
    int first_zero_index = -1;
    for (int j = 0; j < 4; j++) {
        ibz_copy(&b_bound[j], &gram[j][j]);
        ibz_div_2exp(&b_bound[j], &b_bound[j], 1);
        ibz_div(&b_bound[j], &norm, &bound, &b_bound[j]);
        ibz_sqrt_floor(&b_bound[j], &b_bound[j]);
        if (first_zero_index == -1 && ibz_cmp(&b_bound[j], &ibz_const_zero) == 0) {
            first_zero_index = j;
        }
    }
    if (first_zero_index == -1) {
        first_zero_index = 4;
    }

    quat_alg_elem_t quat_temp;
    quat_alg_elem_init(&quat_temp);
    ibz_copy(&(quat_temp.denom), &(lattice->denom));

    // Parameter-set-specific trial limit for finding a valid response.
    while (!found && count < FINDq_num_trial) {

        for (int i = 0; i < first_zero_index; i++) {
            ibz_rand_interval_minm_m(&vec[i], ibz_get(&b_bound[i]));
        }
        for (int i = first_zero_index; i < 4; i++) {
            ibz_set(&vec[i], 0);
        }

        norm_from_2_times_gram(&norm, &gram, &vec);

        // checking that we got something small enough
        found = ibz_cmp(&norm, &bound) < 0 &&
            (ibz_cmp(&vec[0], &ibz_const_zero) != 0 || ibz_cmp(&vec[1], &ibz_const_zero) != 0 ||
                ibz_cmp(&vec[2], &ibz_const_zero) != 0 || ibz_cmp(&vec[3], &ibz_const_zero) != 0);
        if (found) {
            // computing the absolute coordinates of the result
            ibz_mat_4x4_eval(&(x->coord), &lll, &vec);
#if _DEBUG
            assert(quat_lattice_contains(NULL, lattice, x, &QUATALG_PINFTY));
#endif
			// Ensure that x is not divisible by 3.
			quat_lattice_contains(&coord, &MAXORD_O0, x, &QUATALG_PINFTY);
			ibz_content(&content, &coord);
			ibz_mod(&temp, &content, &ibz_const_three);
			if (ibz_is_zero(&temp)) {
				found = 0;
				continue;
			}

            quat_alg_norm(&temp_norm, x, &QUATALG_PINFTY);
            ibq_to_ibz(&temp, &temp_norm);
            ibz_div(eideal_norm, &temp, &temp, lattice_content);
            if (!check_eideal_norm(norm_mu, flag, eideal_norm, N)) {
                found = 0;
            }
        }
        count++;
    }

    ibz_vec_4_finalize(&b_bound);
    ibq_finalize(&temp_norm);
    ibz_finalize(&temp);
    ibz_finalize(&denom_gram);
    ibz_finalize(&norm);
	ibz_finalize(&content);
    ibz_mat_4x4_finalize(&prod);
    ibz_mat_4x4_finalize(&gram);
    ibz_mat_4x4_finalize(&lll);
    ibz_vec_4_finalize(&vec);
	quat_alg_coord_finalize(&coord);
    ibz_finalize(&bound);
    return found;
}



// compute the challenge as the hash of the message and the commitment curve and public key
void
hash_to_challenge(digit_t *out,
                  const ec_curve_t *com_curve,
                  const unsigned char *message,
                  const public_key_t *pk,
                  size_t length)
{
    unsigned char *buf = malloc(FP2_ENCODED_BYTES + FP2_ENCODED_BYTES + length + 1);
    {
        fp2_t j1, j2;
        ec_j_inv(&j1, com_curve);
        ec_j_inv(&j2, &pk->curve);
        fp2_encode(buf, &j1);
        fp2_encode(buf + FP2_ENCODED_BYTES, &j2);
        memcpy(buf + FP2_ENCODED_BYTES + FP2_ENCODED_BYTES,
               message,
               length);
    }

    {
		size_t counter = 0;
		int hash_bits = (2 * SECURITY_BITS <= 512) ? 512 : (2 * SECURITY_BITS <= 768 ? 768 : 1024);
        digit_t res = 1;
        res = (res << LOCATION) - 1;
		unsigned char *temp = malloc(hash_bits / 8);
        while (true)
        {
            memcpy(buf + FP2_ENCODED_BYTES + FP2_ENCODED_BYTES + length,
                &counter, 1);
			pseudohash(hash_bits, buf, 8 * (FP2_ENCODED_BYTES + FP2_ENCODED_BYTES + length + 1), temp);
			for (int i = 0; i < HASH_num_iter - 1; i++)
				pseudohash(hash_bits, temp, hash_bits, temp);
            
			memcpy((void*)out, temp, NWORDS_ORDER_3*sizeof(digit_t));
            out[NWORDS_ORDER_3 - 1] = out[NWORDS_ORDER_3 - 1] & res;
            if (mp_compare(out, THREEpF, NWORDS_ORDER_3) < 0)
                break;
            counter++;
        }
		free(temp);
    }

    free(buf);
}

// Check whether eideal_norm satisfies the required conditions.
int check_eideal_norm(ibz_t* norm_mu,
    int* flag,
    const ibz_t* eideal_norm,
    const ibz_t* N)
{
    int found;
    ibz_t temp, pow3, r;
    ibz_init(&temp);
    ibz_init(&pow3);
	ibz_init(&r);

    found = (ibz_cmp(eideal_norm, &TORSION_PLUS_2POWER) < 0)
		&& (!ibz_is_even(eideal_norm));

    /*ibz_mod(&temp, eideal_norm, &ibz_const_three);
    found = found && (!ibz_is_zero(&temp));*/
    if (found) {
        ibz_pow(&pow3, &ibz_const_three, 2 * TORSION_PLUS_THREE_POWER - 1);
        ibz_sub(&temp, &TORSION_PLUS_2POWER, eideal_norm);
        ibz_mul(&temp, &temp, eideal_norm);
        ibz_mul(norm_mu, &pow3, &temp);
        ibz_neg(&temp, norm_mu);
        // If norm_mu = q(2^a-q)3^(2b-1), then flag = 0; otherwise flag = 1.
        if (mpz_legendre(temp, *N) == -1) {
            *flag = 1;
            ibz_mul(norm_mu, norm_mu, &ibz_const_three);
        }
        else {
            *flag = 0;
        }
    }

    ibz_finalize(&temp);
    ibz_finalize(&pow3);
	ibz_finalize(&r);
    return found;
}


void
image_of_sigma(ec_basis_t* Bchall0,
	const ec_curve_t* E_chall,
	const quat_alg_elem_t* alpha,
	const ibz_t* norm_psiphi)
{
	ibz_t inv_of_norm;
	ec_curve_t E0 = CURVE_E0;
	ec_basis_t bas, bas_alpha;
	quat_alg_elem_t resp_quat;
	ibz_mat_2x2_t mat_temp;
	
	ibz_init(&inv_of_norm);
	ec_curve_init(&E0);
	quat_alg_elem_init(&resp_quat);
	ibz_mat_2x2_init(&mat_temp);

	quat_alg_elem_copy(&resp_quat, alpha);
	quat_alg_conj(&resp_quat, &resp_quat);
	copy_point(&bas.P, &BASIS_EVEN.P);
	copy_point(&bas.Q, &BASIS_EVEN.Q);
	copy_point(&bas.PmQ, &BASIS_EVEN.PmQ);
	copy_point(&bas_alpha.P, &BASIS_EVEN.P);
	copy_point(&bas_alpha.Q, &BASIS_EVEN.Q);
	copy_point(&bas_alpha.PmQ, &BASIS_EVEN.PmQ);

	// applying \bar{\alpha}
	endomorphism_application_even_basis(&bas_alpha, &E0, &resp_quat, TORSION_PLUS_EVEN_POWER);
	change_of_basis_matrix_two(&mat_temp, &bas_alpha, &bas, &E0, TORSION_PLUS_EVEN_POWER);
	matrix_application_even_basis(Bchall0, E_chall, &mat_temp, TORSION_PLUS_EVEN_POWER);

	ibz_invmod(&inv_of_norm, norm_psiphi, &TORSION_PLUS_2POWER);

	ec_mul_ibz(&(Bchall0->P), E_chall, &inv_of_norm, &(Bchall0->P));
	ec_mul_ibz(&(Bchall0->PmQ), E_chall, &inv_of_norm, &(Bchall0->PmQ));
	ec_mul_ibz(&(Bchall0->Q), E_chall, &inv_of_norm, &(Bchall0->Q));

	ibz_finalize(&inv_of_norm);
	quat_alg_elem_finalize(&resp_quat);
	ibz_mat_2x2_finalize(&mat_temp);
}

int
computing_mu(quat_alg_elem_t* mu,
    const ibz_t* norm_mu,
    const quat_left_ideal_t* lideal)
{
    int found;
    ibz_t adjust_norm_mu, temp, norm_CD, lambda;
    ibz_vec_2_t CD;
    quat_alg_elem_t gamma, delta;

    ibz_init(&adjust_norm_mu);
    ibz_init(&temp);
    ibz_init(&norm_CD);
    ibz_init(&lambda);
    ibz_vec_2_init(&CD);
    quat_alg_elem_init(&gamma);
    quat_alg_elem_init(&delta);

    quat_alg_scalar(&gamma, &ibz_const_one, &ibz_const_one);
    quat_alg_scalar(&delta, &ibz_const_one, &ibz_const_one);
    found = solve_combi_eichler(&CD, &STANDARD_EXTREMAL_ORDER, &gamma, &delta, lideal, &QUATALG_PINFTY, 0);
    assert(found);
    (void)found;

    ibz_add(&adjust_norm_mu, norm_mu, norm_mu);
    ibz_add(&adjust_norm_mu, &adjust_norm_mu, &adjust_norm_mu);

    ibz_mul(&norm_CD, &CD[0], &CD[0]);
    ibz_mul(&temp, &CD[1], &CD[1]);
    ibz_add(&norm_CD, &norm_CD, &temp);
    ibz_mul(&norm_CD, &norm_CD, &QUATALG_PINFTY.p);
    ibz_invmod(&temp, &norm_CD, &(lideal->norm));
    ibz_mul(&temp, &temp, &adjust_norm_mu);
    ibz_mod(&temp, &temp, &(lideal->norm));
    ibz_sqrt_mod_p(&lambda, &temp, &(lideal->norm));

    found = strong_approximation(mu, &adjust_norm_mu, &STANDARD_EXTREMAL_ORDER,
        &CD, &norm_CD, &lambda, &(lideal->norm), KLPT_strongappro_num_trial,
        &check_ab_and_set_mu, &QUATALG_PINFTY);
#if _DEBUG
	ibq_t temp_norm;
	ibq_init(&temp_norm);
	ibq_to_ibz(&temp, &temp_norm);
	assert(ibz_cmp(&temp, norm_mu));
	ibq_finalize(&temp_norm);
#endif

    ibz_finalize(&adjust_norm_mu);
    ibz_finalize(&temp);
    ibz_finalize(&norm_CD);
    ibz_finalize(&lambda);
    ibz_vec_2_finalize(&CD);
    quat_alg_elem_finalize(&gamma);
    quat_alg_elem_finalize(&delta);

    return found;
}

int
auxiliary_path(ec_basis_t* Baux0,
    ec_curve_t* E_aux,
    const ibz_t* norm_mu,
    const ibz_t* eideal_norm,
    int* flag,
    secret_key_t* sk)
{
    int found;
    fp2_t w0, w1, test_pow;
    digit_t digit[NWORDS_ORDER_2] = { 0 };
    ec_curve_t E0 = CURVE_E0, E1, E2;
    ec_curve_init(&E0);
    ec_curve_t E_aux1;
    ec_point_t AC, A24;
    ec_point_t ker_phi1, ker_phi2, ker_phi1_dual;
    ec_basis_t bas, bas_temp, B_0_two, B_temp, B_0_three, B_two_1, B_two_2;
    ibz_t temp, pow3;
    ibz_vec_2_t vec;
    ibz_mat_2x2_t mat_temp;
    quat_alg_elem_t mu, mu_bar;
    ec_isog_three_t phi1, phi2;
    theta_chain_t isog;
    theta_couple_curve_t E0A;
    theta_couple_point_t T1, T2, T1m2;
    theta_couple_point_t P, Q, PmQ;
    theta_couple_point_t V1, V2, V1m2;
    theta_couple_point_t kernel_three;

    ibz_init(&temp);
    ibz_init(&pow3);
    ibz_vec_2_init(&vec);
    ibz_mat_2x2_init(&mat_temp);
    quat_alg_elem_init(&mu);
    quat_alg_elem_init(&mu_bar);

    found = computing_mu(&mu, norm_mu, &(sk->secret_ideal));
    if (!found)
        return 0;

#if COMPRESSED == 0
    copy_point(&B_0_two.P, &sk->basis_two.P);
    copy_point(&B_0_two.Q, &sk->basis_two.Q);
    copy_point(&B_0_two.PmQ, &sk->basis_two.PmQ);
        // B_0_two = (tau(P0),tau(Q0))
    copy_point(&B_0_three.P, &sk->basis_three.P);
    copy_point(&B_0_three.Q, &sk->basis_three.Q);
    copy_point(&B_0_three.PmQ, &sk->basis_three.PmQ);
        // B_0_three = (tau(R0),tau(S0))
#elif COMPRESSED == 1
	ec_curve_to_basis_2f_from_hint(&B_0_two, &sk->curve, TORSION_PLUS_EVEN_POWER, sk->hint_sk[0]);
	matrix_application_even_basis(&B_0_two, &sk->curve, &sk->mat_B0_to_Bcan_two, TORSION_PLUS_EVEN_POWER);

	ec_curve_to_basis_3f_from_hint(&B_0_three, &sk->curve, sk->hint_sk + 1, TORSION_PLUS_THREE_POWER);
	matrix_application_three_basis(&B_0_three, &sk->curve, &sk->mat_B0_to_Bcan_three);

#endif

         
    // Compute phi1 and phi2 separately; this can later be merged, mainly around vec.
    quat_alg_conj(&mu_bar, &mu);
    quat_to_isogeny_dlog_three(&vec, TORSION_PLUS_THREE_POWER, &mu_bar);

    digit_t scalars[2][NWORDS_ORDER_3];
    ibz_to_digit_array(scalars[0], &(vec[0]));
    ibz_to_digit_array(scalars[1], &(vec[1]));
    ec_biscalar_mul_3(&ker_phi1, &(sk->curve), scalars[0], scalars[1], &B_0_three);

#if _DEBUG
    assert(test_point_order_threef(&ker_phi1, &(sk->curve), TORSION_PLUS_THREE_POWER));
#endif

    ec_point_t points[4];
    ibz_mod(&temp, &(vec[0]), &ibz_const_three);
    if (ibz_is_zero(&temp)) {
        copy_point(&(points[3]), &(B_0_three.P));
    }
    else {
        copy_point(&(points[3]), &(B_0_three.Q));
    }
    copy_point(&(points[0]), &(B_0_two.P));
    copy_point(&(points[1]), &(B_0_two.Q));
    copy_point(&(points[2]), &(B_0_two.PmQ));
    phi1.curve = sk->curve;
    phi1.kernel = ker_phi1;
    phi1.length = TORSION_PLUS_THREE_POWER;
    ec_eval_three(&E1, &phi1, points, 4, NULL);
    copy_point(&(B_two_1.P), &(points[0]));
    copy_point(&(B_two_1.Q), &(points[1]));
    copy_point(&(B_two_1.PmQ), &(points[2]));
    copy_point(&ker_phi1_dual, &(points[3]));

    uint64_t b;
    if (*flag) {
        b = TORSION_PLUS_THREE_POWER;
        ibz_copy(&pow3, &TORSION_PLUS_3POWER);
    }
    else {
        b = TORSION_PLUS_THREE_POWER - 1;
        ibz_pow(&pow3, &ibz_const_three, b);
    }

    quat_to_isogeny_dlog_three(&vec, b, &mu);
    ibz_to_digit_array(scalars[0], &(vec[0]));
    ibz_to_digit_array(scalars[1], &(vec[1]));
    ec_biscalar_mul_3(&ker_phi2, &(sk->curve), scalars[0], scalars[1], &B_0_three);

    copy_point(&(B_two_2.P), &(B_0_two.P));
    copy_point(&(B_two_2.Q), &(B_0_two.Q));
    copy_point(&(B_two_2.PmQ), &(B_0_two.PmQ));
    // Compute B_two_2 = (3^b*q)^(-1)*mu*(tau(P0),tau(Q0)).
    ibz_mul(&temp, &pow3, eideal_norm);
    ibz_invmod(&temp, &temp, &TORSION_PLUS_2POWER);
    ibz_mul(&mu.coord[0], &mu.coord[0], &temp);
    ibz_mul(&mu.coord[1], &mu.coord[1], &temp);
    ibz_mul(&mu.coord[2], &mu.coord[2], &temp);
    ibz_mul(&mu.coord[3], &mu.coord[3], &temp);
    endomorphism_application_even_basis(&B_two_2, &(sk->curve), &mu, TORSION_PLUS_EVEN_POWER);
    phi2.curve = sk->curve;
    phi2.kernel = ker_phi2;
    phi2.length = b;
    ec_point_t B_two_2_points[3];
    copy_point(&B_two_2_points[0], &B_two_2.P);
    copy_point(&B_two_2_points[1], &B_two_2.Q);
    copy_point(&B_two_2_points[2], &B_two_2.PmQ);
    ec_eval_three(&E2, &phi2, B_two_2_points, 3, NULL);
    copy_point(&B_two_2.P, &B_two_2_points[0]);
    copy_point(&B_two_2.Q, &B_two_2_points[1]);
    copy_point(&B_two_2.PmQ, &B_two_2_points[2]);

    // Compute the dimension-2 isogeny.
    E0A.E1 = E1;
    E0A.E2 = E2;
    copy_point(&(T1.P1), &(B_two_1.P));
    copy_point(&(T2.P1), &(B_two_1.Q));
    copy_point(&(T1m2.P1), &(B_two_1.PmQ));
    copy_point(&(T1.P2), &(B_two_2.P));
    copy_point(&(T2.P2), &(B_two_2.Q));
    copy_point(&(T1m2.P2), &(B_two_2.PmQ));


    theta_chain_comput_strategy(&isog,
        TORSION_PLUS_EVEN_POWER,
        &E0A,
        &T1,
        &T2,
        &T1m2,
        strategies[0],
        0);

    copy_point(&(P.P1), &(B_two_1.P));
    copy_point(&(Q.P1), &(B_two_1.Q));
    copy_point(&(PmQ.P1), &(B_two_1.PmQ));
    copy_point(&(kernel_three.P1), &ker_phi1_dual);
    ec_set_zero(&P.P2);
    ec_set_zero(&Q.P2);
    ec_set_zero(&PmQ.P2);
    ec_set_zero(&kernel_three.P2);

    theta_chain_eval_special_case(&V1, &isog, &P, &E0A);
    theta_chain_eval_special_case(&V2, &isog, &Q, &E0A);
    theta_chain_eval_special_case(&V1m2, &isog, &PmQ, &E0A);
    theta_chain_eval_special_case(&kernel_three, &isog, &kernel_three, &E0A);

    // Select the target curve.
    fp2_copy(&AC.x, &(E0A.E1.A));
    fp2_copy(&AC.z, &(E0A.E1.C));
    A24_from_AC(&A24, &AC);
    copy_point(&(bas_temp.P), &(B_two_1.P));
    copy_point(&(bas_temp.Q), &(B_two_1.Q));
    copy_point(&(bas_temp.PmQ), &(B_two_1.PmQ));
    weil(&w0, TORSION_PLUS_EVEN_POWER, &(bas_temp.P), &(bas_temp.Q), &(bas_temp.PmQ), &A24);
    fp2_copy(&AC.x, &(isog.codomain.E1.A));
    fp2_copy(&AC.z, &(isog.codomain.E1.C));
    A24_from_AC(&A24, &AC);
    copy_point(&(bas_temp.P), &(V1.P1));
    copy_point(&(bas_temp.Q), &(V2.P1));
    copy_point(&(bas_temp.PmQ), &(V1m2.P1));
	assert(test_point_order_twof(&(bas_temp.P), &isog.codomain.E1, TORSION_PLUS_EVEN_POWER));
    weil(&w1, TORSION_PLUS_EVEN_POWER, &(bas_temp.P), &(bas_temp.Q), &(bas_temp.PmQ), &A24);
    ibz_to_digit_array(digit, eideal_norm);
    fp2_pow_vartime(&test_pow, &w0, digit, NWORDS_ORDER_2);
    if (fp2_is_equal(&test_pow, &w1)) {
        copy_curve(E_aux, &(isog.codomain.E2));
        copy_point(&(Baux0->P), &(V1.P2));
        copy_point(&(Baux0->Q), &(V2.P2));
        copy_point(&(Baux0->PmQ), &(V1m2.P2));
        copy_point(&ker_phi1, &(kernel_three.P2));
        /*printf("Auxliary Path Founding: It's the SECOND curve!\n");*/
    }
    else {
        copy_curve(E_aux, &(isog.codomain.E1));
        copy_point(&(Baux0->P), &(V1.P1));
        copy_point(&(Baux0->Q), &(V2.P1));
        copy_point(&(Baux0->PmQ), &(V1m2.P1));
        copy_point(&ker_phi1, &(kernel_three.P1));
        /*printf("Auxliary Path Founding: It's the FIRST curve!\n");*/
    }

    copy_curve(&(phi1.curve), E_aux);
    phi1.kernel = ker_phi1;
    phi1.length = TORSION_PLUS_THREE_POWER;
    ec_point_t Baux0_points[3];
    copy_point(&Baux0_points[0], &Baux0->P);
    copy_point(&Baux0_points[1], &Baux0->Q);
    copy_point(&Baux0_points[2], &Baux0->PmQ);
    ec_eval_three(E_aux, &phi1, Baux0_points, 3, NULL);
    copy_point(&Baux0->P, &Baux0_points[0]);
    copy_point(&Baux0->Q, &Baux0_points[1]);
    copy_point(&Baux0->PmQ, &Baux0_points[2]);

    ibz_finalize(&temp);
    ibz_finalize(&pow3);
    ibz_vec_2_finalize(&vec);
    ibz_mat_2x2_finalize(&mat_temp);
    quat_alg_elem_finalize(&mu);
    quat_alg_elem_finalize(&mu_bar);

    return 1;
}



int protocols_sign_internal(signature_t* sig,
    const public_key_t* pk,
    secret_key_t* sk,
    const unsigned char* m,
    size_t l)
{
    int found = 0, count = 0, flag;
	digit_t scalar0[NWORDS_ORDER_3], scalar1[NWORDS_ORDER_3], out[NWORDS_ORDER_3];
    ibz_t lattice_content, eideal_norm, temp, norm_mu;
    ec_point_t kernel_chall;
    ec_basis_t Bcom0, Bcom_can, Bcom_three_0, Bchall0, Bchall_can, Baux0, Baux_can;
    ec_curve_t E_com, E_chall, E_aux;
    ibz_vec_2_t vec, vec_chall;
    ibz_mat_2x2_t Bcom_can_to_Bcom_0, mat_temp;
    quat_alg_elem_t alpha;
    quat_lattice_t lattice_hom_pk_to_chall, lat_sec;
    quat_left_ideal_t lideal_commit, lideal_chall_two, lideal_chall_commit;
	ec_isog_three_t phi;

    ibz_init(&lattice_content);
    ibz_init(&eideal_norm);
    ibz_init(&temp);
    ibz_init(&norm_mu);
    ibz_vec_2_init(&vec);
    ibz_vec_2_init(&vec_chall);
    ibz_mat_2x2_init(&Bcom_can_to_Bcom_0);
    ibz_mat_2x2_init(&mat_temp);
    quat_alg_elem_init(&alpha);
    quat_lattice_init(&lattice_hom_pk_to_chall);
    quat_lattice_init(&lat_sec);
    quat_left_ideal_init(&lideal_commit);
    quat_left_ideal_init(&lideal_chall_two);
    quat_left_ideal_init(&lideal_chall_commit);

    while (!found) {
        count = count + 1;

        // computing the commitment
        commit(&E_com, &Bcom0, &Bcom_three_0, &lideal_commit);
		
        // computing the challenge
		hash_to_challenge(out, &E_com, m, pk, l);
#if COMPRESSED == 1
		memcpy(sig->chall, out, sizeof(sig->chall));
#endif
		ibz_set(&vec_chall[0], 1);
		ibz_copy_digit_array(&vec_chall[1], out);
#if COMPRESSED == 0
        found = ec_curve_to_basis_3f_to_hint(&Bcom_can, &E_com, sig->hint_com, TORSION_PLUS_THREE_POWER);
        if (!found)
            continue;
        
        ibz_to_digit_array(scalar0, &(vec_chall[0]));
        ibz_to_digit_array(scalar1, &(vec_chall[1]));
        ec_biscalar_mul_3(&kernel_chall, &E_com, scalar0, scalar1, &Bcom_can);
#if _DEBUG
		assert(test_point_order_threef(&Bcom_three_0.P, &E_com, TORSION_PLUS_THREE_POWER));
		assert(test_point_order_threef(&Bcom_three_0.Q, &E_com, TORSION_PLUS_THREE_POWER));
		assert(test_point_order_threef(&Bcom_three_0.PmQ, &E_com, TORSION_PLUS_THREE_POWER));
		assert(test_point_order_threef(&Bcom_can.P, &E_com, TORSION_PLUS_THREE_POWER));
		assert(test_point_order_threef(&Bcom_can.Q, &E_com, TORSION_PLUS_THREE_POWER));
		assert(test_point_order_threef(&Bcom_can.PmQ, &E_com, TORSION_PLUS_THREE_POWER));
#endif
        change_of_basis_matrix_three(
            &Bcom_can_to_Bcom_0, &Bcom_can, &Bcom_three_0, &E_com);
        ibz_mat_2x2_eval(&vec, &Bcom_can_to_Bcom_0, &vec_chall);
#elif COMPRESSED == 1
		ibz_to_digit_array(scalar0, &(vec_chall[0]));
		ibz_to_digit_array(scalar1, &(vec_chall[1]));
		ec_biscalar_mul_3(&kernel_chall, &E_com, scalar0, scalar1, &Bcom_three_0);

		ibz_copy(&vec[0], &vec_chall[0]);
		ibz_copy(&vec[1], &vec_chall[1]);
#endif

		// now we compute the response

        // computing the ideal corresonding to phi*psi*dual(tau), associated with its norm
        id2iso_kernel_dlogs_to_ideal_three_general(&lideal_chall_two, &vec, TORSION_PLUS_THREE_POWER);
            // lideal_chall_two is the pullback of the ideal challenge through the secret key ideal
        quat_lideal_inter(&lideal_chall_commit, &lideal_chall_two, &lideal_commit, &QUATALG_PINFTY);
            // lideal_chall_commit = I_{phi*psi}
        quat_lideal_conjugate_lattice(&lat_sec, &(sk->secret_ideal));
            // lat_sec is the conjugate of the secret ideal, stored as a lattice;
        quat_lattice_intersect(&lattice_hom_pk_to_chall, &lideal_chall_commit.lattice, &lat_sec);
            // lattice_hom_pk_to_chall is the ideal corresonding to phi*psi*dual(tau), stored as a lattce
        ibz_mul(&lattice_content, &(lideal_chall_commit.norm), &((sk->secret_ideal).norm));
            // lattice_content is the norm
		
        // computing the ideal I_sigma equivalent to I_{phi*psi*dual(tau)}
        found = sample_response(&norm_mu, &flag, &eideal_norm,
            &(sk->secret_ideal).norm, &alpha, &lattice_hom_pk_to_chall, &lattice_content);
			// alpha is the element that transforms I_{phi*psi*dual(tau)} to I_sigma
			// the norm of I_sigma = eideal_norm, eideal_norm is denoted by q below
			// norm_mu  = q*(2^a-q)*3^(2b-1) or q*(2^a-q)*3^(2b) according to flag = 0 or 1
		if (!found) {
			//printf("Sample_Response FAILED!\n");
			continue;
		}
        


        // computing the auxiliary isogeny w from EA of norm 2^a-q
        // computing 3^b*w*tau(P0,Q0), stored in Baux0
        found = auxiliary_path(&Baux0, &E_aux, &norm_mu, &eideal_norm, &flag, sk);
        if (!found) {
            //printf("Strong_Approxiamation FAILED!\n");
            continue;
        }

        phi.curve = E_com;
        phi.kernel = kernel_chall;
        phi.length = TORSION_PLUS_THREE_POWER;
#if COMPRESSED == 0
		Bchall0 = Bcom0;
		ec_point_t Bchall0_points[3];
		copy_point(&Bchall0_points[0], &Bchall0.P);
		copy_point(&Bchall0_points[1], &Bchall0.Q);
		copy_point(&Bchall0_points[2], &Bchall0.PmQ);
        ec_eval_three(&E_chall, &phi, Bchall0_points, 3, NULL); // Bchall0 stores phi*psi(P0,Q0).
		copy_point(&Bchall0.P, &Bchall0_points[0]);
		copy_point(&Bchall0.Q, &Bchall0_points[1]);
		copy_point(&Bchall0.PmQ, &Bchall0_points[2]);
#elif COMPRESSED == 1
		ec_point_t points[4], ker_chall_dual;
		ec_basis_t Bchall_com_three;
		ec_isom_t isom;
		copy_point(&points[0], &Bcom0.P);
		copy_point(&points[1], &Bcom0.Q);
		copy_point(&points[2], &Bcom0.PmQ);
		if (ibz_is_one(&vec_chall[0]))
			copy_point(&points[3], &Bcom_three_0.Q);
		else
			copy_point(&points[3], &Bcom_three_0.P);

		ec_eval_three(&E_chall, &phi, points, 4, NULL);
		copy_point(&Bchall0.P, &points[0]);
		copy_point(&Bchall0.Q, &points[1]);
		copy_point(&Bchall0.PmQ, &points[2]);
		copy_point(&ker_chall_dual, &points[3]);		

#if _DEBUG
		assert(test_point_order_threef(&ker_chall_dual, &E_chall, TORSION_PLUS_THREE_POWER));
#endif
		// Choose the canonical representative of E_chall.
		ec_curve_standardized(&isom, &E_chall, &E_chall);
		ec_iso_eval(&Bchall0.P, &isom);
		ec_iso_eval(&Bchall0.Q, &isom);
		ec_iso_eval(&Bchall0.PmQ, &isom);
		ec_iso_eval(&ker_chall_dual, &isom);

		// encode the dual of phi_chall into the signature
		ec_curve_to_basis_3f_to_hint(&Bchall_com_three, &E_chall, sig->hint_chall, TORSION_PLUS_THREE_POWER);
		ec_dlog_3_tate(scalar0, scalar1, &ker_chall_dual, &Bchall_com_three, &E_chall);
		ibz_copy_digit_array(&vec[0], scalar0);
		ibz_copy_digit_array(&vec[1], scalar1);
		ibz_mod(&temp, &vec[0], &ibz_const_three);
		if (ibz_is_zero(&temp)) {
			ibz_invmod(&temp, &vec[1], &TORSION_PLUS_3POWER);
			ibz_mul(&vec[0], &vec[0], &temp);
			ibz_mod(&vec[0], &vec[0], &TORSION_PLUS_3POWER);
			ibz_to_digit_array(sig->scalar, &vec[0]);
			sig->ind = 0;
		}
		else {
			ibz_invmod(&temp, &vec[0], &TORSION_PLUS_3POWER);
			ibz_mul(&vec[1], &vec[1], &temp);
			ibz_mod(&vec[1], &vec[1], &TORSION_PLUS_3POWER);
			ibz_to_digit_array(sig->scalar, &vec[1]);
			sig->ind = 1;
		}
#endif

        // computing 3^b*sigma*tau(P0,Q0), stored in Bchall0
        image_of_sigma(&Bchall0, &E_chall, &alpha, &(lideal_commit.norm));
        
#if COMPRESSED == 0
		sig->hint_aux = ec_curve_to_basis_2f_to_hint(&Baux_can, &E_aux, TORSION_PLUS_EVEN_POWER);
        change_of_basis_matrix_two(&mat_temp, &Baux_can, &Baux0, &E_aux, TORSION_PLUS_EVEN_POWER);
		sig->hint_chall = ec_curve_to_basis_2f_to_hint(&Bchall_can, &E_chall, TORSION_PLUS_EVEN_POWER);
        change_of_basis_matrix_two(&Bcom_can_to_Bcom_0, &Bchall0, &Bchall_can, &E_chall, TORSION_PLUS_EVEN_POWER);
        ibz_2x2_mul_mod(&sig->mat_Bchall_can_to_B_chall,
            &Bcom_can_to_Bcom_0, &mat_temp, &TORSION_PLUS_2POWER);
              
        // filling the signature
        fp2_t temp_fp2;
        fp2_copy(&temp_fp2, &E_aux.C);
        fp2_inv(&temp_fp2);
        fp2_mul(&(sig->E_aux).A, &temp_fp2, &E_aux.A);
        fp2_set_one(&(sig->E_aux).C);
        ec_point_init(&sig->E_aux.A24);
        sig->E_aux.is_A24_computed_and_normalized = 0;
		fp2_copy(&temp_fp2, &E_com.C);
		fp2_inv(&temp_fp2);
		fp2_mul(&(sig->E_com).A, &temp_fp2, &E_com.A);
		fp2_set_one(&(sig->E_com).C);
		ec_point_init(&sig->E_com.A24);
		sig->E_com.is_A24_computed_and_normalized = 0;
		sig->E_chall = E_chall;
#elif COMPRESSED == 1
		ec_basis_t B_diag_0, B_diag_can;
		theta_couple_curve_t Echall_x_aux;
		theta_couple_point_t T1, T2, T1m2, V1, V2, V1m2;
		theta_chain_t isog;

		// Compute the diagonal curve E_diag.
		Echall_x_aux.E1 = E_chall;
		Echall_x_aux.E2 = E_aux;
		copy_point(&T1.P1, &Bchall0.P);
		copy_point(&T2.P1, &Bchall0.Q);
		copy_point(&T1m2.P1, &Bchall0.PmQ);
		copy_point(&T1.P2, &Baux0.P);
		copy_point(&T2.P2, &Baux0.Q);
		copy_point(&T1m2.P2, &Baux0.PmQ);
		
		theta_chain_comput_strategy(&isog,
			TORSION_PLUS_EVEN_POWER,
			&Echall_x_aux,
			&T1,
			&T2,
			&T1m2,
			strategies[0],
			0);

		ec_set_zero(&T1.P2);
		ec_set_zero(&T2.P2);
		ec_set_zero(&T1m2.P2);
		theta_chain_eval_special_case(&V1, &isog, &T1, &Echall_x_aux);
		theta_chain_eval_special_case(&V2, &isog, &T2, &Echall_x_aux);
		theta_chain_eval_special_case(&V1m2, &isog, &T1m2, &Echall_x_aux);

		if (ec_is_isomorphic(&sk->curve, &isog.codomain.E1)) {
			sig->E_diag = isog.codomain.E2;
			copy_point(&B_diag_0.P, &V1.P2);
			copy_point(&B_diag_0.Q, &V2.P2);
			copy_point(&B_diag_0.PmQ, &V1m2.P2);
		}
		else {
			sig->E_diag = isog.codomain.E1;
			copy_point(&B_diag_0.P, &V1.P1);
			copy_point(&B_diag_0.Q, &V2.P1);
			copy_point(&B_diag_0.PmQ, &V1m2.P1);
		}
		
		sig->hint_diag = ec_curve_to_basis_2f_to_hint(&B_diag_can, &sig->E_diag, TORSION_PLUS_EVEN_POWER);
		change_of_basis_matrix_two(&mat_temp, &B_diag_can, &B_diag_0, &sig->E_diag, TORSION_PLUS_EVEN_POWER);
		ibz_2x2_mul_mod(&sig->mat_Bpk_can_to_B_pk, &sk->mat_B0_to_Bcan_two, &mat_temp, &TORSION_PLUS_2POWER);
		ibz_mul(&temp, &eideal_norm, &TORSION_PLUS_3POWER);
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 2; j++) {
				ibz_mul(&sig->mat_Bpk_can_to_B_pk[i][j], &sig->mat_Bpk_can_to_B_pk[i][j], &temp);
				ibz_mod(&sig->mat_Bpk_can_to_B_pk[i][j], &sig->mat_Bpk_can_to_B_pk[i][j], &TORSION_PLUS_2POWER);
			}

		fp2_t temp_fp2;
		fp2_copy(&temp_fp2, &sig->E_diag.C);
		fp2_inv(&temp_fp2);
		fp2_mul(&(sig->E_diag).A, &temp_fp2, &(sig->E_diag).A);
		fp2_set_one(&(sig->E_diag).C);
		ec_point_init(&sig->E_diag.A24);
		sig->E_diag.is_A24_computed_and_normalized = 0;

		// Determine the hint sig->hint_curve.
		fp2_t j1, j2;
		ec_j_inv(&j1, &E_aux);
		ec_j_inv(&j2, &E_chall);
		if (fp2_cmp(&j1, &j2) >= 0)
			sig->hint_curve = 0;
		else
			sig->hint_curve = 1;
		//sig->E_aux = E_chall;
		sig->E_com = E_com;
		sig->E_chall = E_chall;
#endif
    }
    
    /*printf("The main loop number is: %d\n", count);*/

    ibz_finalize(&lattice_content);
    ibz_finalize(&eideal_norm);
    ibz_finalize(&temp);
    ibz_finalize(&norm_mu);
    ibz_vec_2_finalize(&vec);
    ibz_vec_2_finalize(&vec_chall);
    quat_alg_elem_finalize(&alpha);
    ibz_mat_2x2_finalize(&Bcom_can_to_Bcom_0);
    ibz_mat_2x2_finalize(&mat_temp);
    quat_lattice_finalize(&lattice_hom_pk_to_chall);
    quat_lattice_finalize(&lat_sec);
    quat_left_ideal_finalize(&lideal_commit);
    quat_left_ideal_finalize(&lideal_chall_two);
    quat_left_ideal_finalize(&lideal_chall_commit);

    return count;
}

int
protocols_verif_internal(signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l)
{
	#if COMPRESSED == 0
		int verif = 1;
		digit_t scalar0[NWORDS_ORDER_3], scalar1[NWORDS_ORDER_3], out[NWORDS_ORDER_3];
	ec_point_t kernel_chall, dual;
	ec_basis_t Bcom_can, Bchall_can, Baux_can, B3;
	ec_curve_t E_com, E_chall, E_aux;
	ibz_vec_2_t vec_chall;
	theta_couple_curve_t Echall_x_aux;
	theta_couple_point_t T1, T2, T1m2, P, Q, PmQ;
	theta_chain_t isog;
	memset(&isog, 0, sizeof(isog)); /* PATCH (nist-sigs-zoo, fix) */
		ec_isog_three_t phi;
		ibz_vec_2_init(&vec_chall);
	
		// checking that we are given A coefficients and no precomputation
	assert(fp2_is_one(&pk->curve.C) && !pk->curve.is_A24_computed_and_normalized);
	assert(fp2_is_one(&sig->E_aux.C) && !sig->E_aux.is_A24_computed_and_normalized);
	assert(fp2_is_one(&sig->E_com.C) && !sig->E_com.is_A24_computed_and_normalized);

		copy_curve(&E_com, &(sig->E_com));
		copy_curve(&E_aux, &(sig->E_aux));
	
		// computing the challenge
    	ec_curve_to_basis_3f_from_hint(&Bcom_can, &E_com, sig->hint_com, TORSION_PLUS_THREE_POWER);
		hash_to_challenge(out, &E_com, m, pk, l);
		ibz_set(&vec_chall[0], 1);
		ibz_copy_digit_array(&vec_chall[1], out);
		ibz_to_digit_array(scalar0, &(vec_chall[0]));
		ibz_to_digit_array(scalar1, &(vec_chall[1]));
		ec_biscalar_mul_3(&kernel_chall, &E_com, scalar0, scalar1, &Bcom_can);
		
		phi.curve = E_com;
		phi.kernel = kernel_chall;
		phi.length = TORSION_PLUS_THREE_POWER;
		ec_eval_three(&E_chall, &phi, NULL, 0, &dual);
#if _DEBUG
	assert(test_point_order_threef(&dual, &E_chall, 1));
#endif

	    ec_curve_to_basis_2f_from_hint(&Bchall_can, &E_chall, TORSION_PLUS_EVEN_POWER, sig->hint_chall);
	    matrix_application_even_basis(&Bchall_can, &E_chall, &sig->mat_Bchall_can_to_B_chall, TORSION_PLUS_EVEN_POWER);
		ec_curve_to_basis_2f_from_hint(&Baux_can, &E_aux, TORSION_PLUS_EVEN_POWER, sig->hint_aux);


	// computing the dim2 isogeny
	Echall_x_aux.E1 = E_chall;
	Echall_x_aux.E2 = E_aux;

	copy_point(&T1.P1, &Bchall_can.P);
	copy_point(&T2.P1, &Bchall_can.Q);
	copy_point(&T1m2.P1, &Bchall_can.PmQ);
	copy_point(&T1.P2, &Baux_can.P);
	copy_point(&T2.P2, &Baux_can.Q);
	copy_point(&T1m2.P2, &Baux_can.PmQ);

	int is_split; /* PATCH (nist-sigs-zoo, fix) */
#if defined(SQISIGN_FP2_ASM_P255) || defined(SQISIGN_FP2_ASM_P511) || defined(SQISIGN_FP2_ASM_P1024)
		is_split = theta_chain_comput_strategy_faster_no_eval_checked(&isog,
#else
		is_split = theta_chain_comput_strategy_checked(&isog,
#endif
			TORSION_PLUS_EVEN_POWER,
			&Echall_x_aux,
			&T1,
			&T2,
			&T1m2,
			strategies[0],
			0);
	if (!is_split) { /* PATCH (nist-sigs-zoo, fix): reject when the chain does not split */
		ibz_vec_2_finalize(&vec_chall);
		return 0;
	}

    /*ec_curve_to_basis_3(&B3, &E_chall);
#if _DEBUG
    assert(test_point_order_threef(&B3.P, &E_chall, 1));
    assert(test_point_order_threef(&B3.Q, &E_chall, 1));
    assert(test_point_order_threef(&B3.PmQ, &E_chall, 1));
#endif*/
    copy_point(&(P.P1), &dual);
    /*copy_point(&(Q.P1), &B3.Q);
    copy_point(&(PmQ.P1), &B3.PmQ);*/
    ec_set_zero(&P.P2);
    /*ec_set_zero(&Q.P2);
    ec_set_zero(&PmQ.P2);*/

	    theta_chain_eval_special_case(&T1, &isog, &P, &Echall_x_aux);
    //theta_chain_eval_special_case(&T2, &isog, &Q, &Echall_x_aux);
    //theta_chain_eval_special_case(&T1m2, &isog, &PmQ, &Echall_x_aux);

		fp2_t j, j1, j2;
		ec_j_inv(&j, &(pk->curve));
		ec_j_inv(&j1, &(isog.codomain.E1));
		    
	    if (fp2_is_equal(&j, &j1)) {
	        if ((!ec_is_zero(&T1.P1)) /*&& (!ec_is_zero(&T2.P1)) &&
	            (!ec_is_equal(&T1.P1, &T2.P1))*/)
	            verif = 1;
	        else
	            verif = 0;
	    }
	    else {
	        ec_j_inv(&j2, &(isog.codomain.E2));
	        if (fp2_is_equal(&j, &j2)) {
	        if ((!ec_is_zero(&T1.P2))/* && (!ec_is_zero(&T2.P2)) &&
	            (!ec_is_equal(&T1.P2, &T2.P2))*/)
	            verif = 1;
	        else
	            verif = 0;
	        }
	        else
	            verif = 0;
	    }
	
    //verif = fp2_is_equal(&j, &j1) || fp2_is_equal(&j, &j2);

		ibz_vec_2_finalize(&vec_chall);
		return verif;
	#elif COMPRESSED == 1
	int verif = 1, flag;

	digit_t out[NWORDS_ORDER_3];
	ec_point_t ker_chall_dual;
	ec_basis_t B_pk_can, B_diag_can, B_chall_can, B3;
		ec_curve_t E_com, E_chall, E_pk_hint;
	ec_isom_t isom;
	ec_isog_three_t phi;
	theta_couple_curve_t Epk_x_diag;
		theta_couple_point_t T1, T2, T1m2, P, Q, PmQ;
		theta_chain_t isog;
		memset(&isog, 0, sizeof(isog)); /* PATCH (nist-sigs-zoo, fix) */
			// Compute the dimension-2 isogeny.
			Epk_x_diag.E1 = pk->curve;
			Epk_x_diag.E2 = sig->E_diag;
			E_pk_hint = pk->curve;
			
			ec_curve_to_basis_2f_from_hint(&B_diag_can, &sig->E_diag, TORSION_PLUS_EVEN_POWER, sig->hint_diag);
			ec_curve_to_basis_2f_from_hint(&B_pk_can, &E_pk_hint, TORSION_PLUS_EVEN_POWER, pk->hint_pk);
			matrix_application_even_basis(&B_pk_can, &pk->curve, &sig->mat_Bpk_can_to_B_pk, TORSION_PLUS_EVEN_POWER);

	copy_point(&T1.P1, &B_pk_can.P);
	copy_point(&T2.P1, &B_pk_can.Q);
	copy_point(&T1m2.P1, &B_pk_can.PmQ);
	copy_point(&T1.P2, &B_diag_can.P);
	copy_point(&T2.P2, &B_diag_can.Q);
	copy_point(&T1m2.P2, &B_diag_can.PmQ);

		int is_split = theta_chain_comput_strategy_checked(&isog, /* PATCH (nist-sigs-zoo, fix) */
			TORSION_PLUS_EVEN_POWER,
			&Epk_x_diag,
			&T1,
			&T2,
			&T1m2,
			strategies[0],
			0);
		if (!is_split) /* PATCH (nist-sigs-zoo, fix): reject when the chain does not split */
			return 0;
	
		// Select the curve E_chall according to the hint.
		fp2_t j1, j2;
		ec_j_inv(&j1, &(isog.codomain.E1));
		ec_j_inv(&j2, &(isog.codomain.E2));
		if (fp2_cmp(&j1, &j2) >= 0) {
			if (sig->hint_curve) {
				E_chall = isog.codomain.E1;
				flag = 1;
			}
			else {
				E_chall = isog.codomain.E2;
				flag = 0;
			}
		}
		else {
			if (sig->hint_curve) {
				E_chall = isog.codomain.E2;
				flag = 0;
			}
			else {
				E_chall = isog.codomain.E1;
				flag = 1;
			}
		}
		// Canonicalize E_chall.
		ec_curve_standardized(&isom, &E_chall, &E_chall);
	
		// Compute the dual of the challenge isogeny and E_com.
		ec_curve_to_basis_3f_from_hint(&B_chall_can, &E_chall, sig->hint_chall, TORSION_PLUS_THREE_POWER);
		digit_t scalar0[NWORDS_ORDER_3] = { 1 };
		if (sig->ind) {
			ec_biscalar_mul_3(&ker_chall_dual, &E_chall, scalar0, sig->scalar, &B_chall_can);
		}
		else {
			ec_biscalar_mul_3(&ker_chall_dual, &E_chall, sig->scalar, scalar0, &B_chall_can);
		}
		phi.curve = E_chall;
		phi.length = TORSION_PLUS_THREE_POWER;
		phi.kernel = ker_chall_dual;
	
		ec_eval_three(&E_com, &phi, NULL, 0, NULL);
	
		// Verify H(m,E_com,pk) == sig->chall.
		hash_to_challenge(out, &E_com, m, pk, l);
		verif = verif & (memcmp((void*)out, (void*)sig->chall, (NWORDS_ORDER_3)*sizeof(digit_t)) == 0);
		
		// Verify that phi_resp and phi_com have no common part.
		ec_curve_to_basis_3(&B3, &pk->curve);
		
		copy_point(&(P.P1), &B3.P);
		copy_point(&(Q.P1), &B3.Q);
		copy_point(&(PmQ.P1), &B3.PmQ);
		ec_set_zero(&P.P2);
		ec_set_zero(&Q.P2);
		ec_set_zero(&PmQ.P2);
		
		theta_chain_eval_special_case(&T1, &isog, &P, &Epk_x_diag);
		theta_chain_eval_special_case(&T2, &isog, &Q, &Epk_x_diag);
		
		if (flag) {
			if (((!ec_is_zero(&T1.P1)) && (!ec_is_zero(&T2.P1)) &&
				(!ec_is_equal(&T1.P1, &T2.P1))));
				// q is coprime to 3.
			else {
				ec_point_t X, Y, Z;
				copy_point(&X, &T1.P1);
				copy_point(&Y, &T2.P1);
				ec_iso_eval(&X, &isom);
				ec_iso_eval(&Y, &isom);
	#if _DEBUG
				assert(ec_is_on_curve(&E_chall, &X));
				assert(ec_is_on_curve(&E_chall, &Y));
	#endif
				ec_tpl_iter(&Z, TORSION_PLUS_THREE_POWER - 1, &E_chall, &ker_chall_dual);
				verif = verif && !ec_is_equal(&X, &Z) && !ec_is_equal(&Y, &Z);
				ec_neg(&Z, &Z);
				verif = verif && !ec_is_equal(&X, &Z) && !ec_is_equal(&Y, &Z);
			}
		}
		else {
			if (((!ec_is_zero(&T1.P2)) && (!ec_is_zero(&T2.P2)) &&
				(!ec_is_equal(&T1.P2, &T2.P2))));
				// q is coprime to 3.
			else {
				ec_point_t X, Y, Z;
				copy_point(&X, &T1.P2);
				copy_point(&Y, &T2.P2);
				ec_iso_eval(&X, &isom);
				ec_iso_eval(&Y, &isom);
	#if _DEBUG
				assert(ec_is_on_curve(&E_chall, &X));
				assert(ec_is_on_curve(&E_chall, &Y));
	#endif
				ec_tpl_iter(&Z, TORSION_PLUS_THREE_POWER - 1, &E_chall, &ker_chall_dual);
				verif = verif && !ec_is_equal(&X, &Z) && !ec_is_equal(&Y, &Z);
				ec_neg(&Z, &Z);
				verif = verif && !ec_is_equal(&X, &Z) && !ec_is_equal(&Y, &Z);
			}
		}

		return verif;
	#endif
	}

int protocols_sign(unsigned char* state_sig,
	const unsigned char* state_sk,
	const unsigned char* m,
	size_t l)
{
	public_key_t pk;
	secret_key_t sk;
	signature_t sig;

	public_key_init(&pk);
	secret_key_init(&sk);
	secret_sig_init(&(sig));

	secret_key_from_bytes(&sk, &pk, state_sk);
	protocols_sign_internal(&sig, &pk, &sk, m, l);
	signature_to_bytes(state_sig, &sig);

	public_key_finalize(&pk);
	secret_key_finalize(&sk);
	secret_sig_finalize(&sig);
	return 0;
}


int
protocols_verif(unsigned char* state_sig, const unsigned char* state_pk,
	const unsigned char *m, size_t l)
{
	public_key_t pk;
	signature_t sig;

	public_key_init(&pk);
	secret_sig_init(&(sig));

	public_key_from_bytes(&pk, state_pk);
	signature_from_bytes(&sig, state_sig);
	int res = protocols_verif_internal(&sig, &pk, m, l);

	public_key_finalize(&pk);
	secret_sig_finalize(&sig);

	return res;
}
