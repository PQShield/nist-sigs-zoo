/* PATCH (nist-sigs-zoo, fix): protocols_verif did not check whether the final splitting step of
 * the (2,2)-isogeny chain succeeded, and derived the challenge from an uninitialised stack struct
 * (theta_chain_t isog) when it did not, so a tampered signature could be accepted when the stack
 * still held the state of an earlier valid verification (reported on https://ngcc.dev/ as
 * sign-26-3). This copy zero-initialises isog and calls theta_chain_comput_balanced_checked (same
 * computation, returns 0 when the chain does not split; patched theta_isogenies.c), rejecting the
 * signature in that case. Nothing else is changed. */
#include <sqisigndim2.h>
#include <curve_extras.h>
#include <tools.h>
#include <sqisign_xof.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <hd.h>
#include <time.h>
#include <tutil.h>

/* PATCH (nist-sigs-zoo, fix): defined in the patched theta_isogenies.c */
int theta_chain_comput_balanced_checked(theta_chain_t *out,
                                        int n,
                                        theta_couple_curve_t *E12,
                                        const theta_couple_point_t *T1,
                                        const theta_couple_point_t *T2,
                                        const theta_couple_point_t *T1m2);
#define RESPONSE_LENGTH TORSION_PLUS_EVEN_POWER+16
#define EXPONENT_TWO TORSION_PLUS_EVEN_POWER
#define EXPONENT_THREE TORSION_PLUS_ODD_POWERS[0]
#define POWER_OF_TWO TORSION_PLUS_2POWER
#define POWER_OF_THREE TORSION_PLUS_3POWER

#define COUNT 1000

const clock_t time_isogenies_odd = 0;
const clock_t time_sample_response = 0;
const clock_t time_change_of_basis_matrix = 0;


static void
jac_init(jac_point_t *P)
{ // Initialize Montgomery in Jacobian coordinates as identity element (0:1:0)
    fp2_set_zero(&(P->x));
    fp2_set_one(&(P->y));
    fp2_set_zero(&(P->z));
}


void secret_sig_init(signature_t *sig) {
    ec_curve_init(&(sig->E_aux));
    ibz_mat_2x2_init(&(sig->mat_sigma_phichall));
    ibz_init(&sig->chl);
    sig->nrsp = 0;
    sig->size_d = 0;
}

void secret_sig_finalize(signature_t *sig) {
    ibz_mat_2x2_finalize(&(sig->mat_sigma_phichall));
    ibz_finalize(&sig->chl);
}

// void
// secret_sig_init(signature_t *sig)
// {
//     ibz_mat_2x2_init(&(sig->mat_Bchall_can_to_B_chall));
//     ibz_init(&sig->chall_coeff);
//     sig->hint_aux = (int *)malloc(2 * sizeof(int));
//     sig->hint_chall = (int *)malloc(2 * sizeof(int));
// }

// void
// secret_sig_finalize(signature_t *sig)
// {
//     ibz_mat_2x2_finalize(&(sig->mat_Bchall_can_to_B_chall));
//     ibz_finalize(&sig->chall_coeff);
//     free(sig->hint_aux);
//     free(sig->hint_chall);
// }

static void ibz_vec_2_print2(char *name, const ibz_vec_2_t *vec){
    printf("%s", name);
    for(int i = 0; i < 2; i++){
        ibz_printf("%Zd ", &((*vec)[i]));
    }
    ibz_printf("\n");
}

static void ibz_vec_4_print2(char *name, const ibz_vec_4_t *vec){
    printf("%s", name);
    for(int i = 0; i < 4; i++){
        ibz_printf("%Zd ", &((*vec)[i]));
    }
    ibz_printf("\n");
}


void print_signature(const signature_t *sig) {
    fp2_t j;
    ec_j_inv(&j, &sig->E_aux);
    fp2_print("j_E1 = ", &j);
    // ibz_mat_2x2_print(&sig->mat_sigma_phichall);
    ibz_printf("M_sigma[00] = %Zd,\n", &((sig->mat_sigma_phichall)[0][0]));
    ibz_printf("M_sigma[01] = %Zd,\n", &((sig->mat_sigma_phichall)[0][1]));
    ibz_printf("M_sigma[10] = %Zd,\n", &((sig->mat_sigma_phichall)[1][0]));
    ibz_printf("M_sigma[11] = %Zd\n", &((sig->mat_sigma_phichall)[1][1]));
    printf("nrsp is %d\n", sig->nrsp);
}

void print_public_key(const public_key_t *pk) {
    fp2_t j;
    ec_j_inv(&j, &pk->curve);
    fp2_print("j_EA = ", &j);
}




static void commit(ibz_t *a3, ibz_t *b3, ec_point_t *k0,ec_point_t *km, ec_curve_t *E_mid, ec_curve_t *E_com, ec_basis_t *basis_even_mid, ec_basis_t *basis_even_com, quat_left_ideal_t *lideal_commit_three, int verbose) {    
    quat_alg_elem_t gamma;
    quat_left_ideal_t lideal_even;
    ec_isog_even_t two_isogeny_first_half, two_isogeny_second_half;
    ec_isog_odd_t phi_first_half, phi_second_half;
    ec_point_t list_points[3];

    quat_alg_elem_init(&gamma);
    quat_left_ideal_init(&lideal_even); 

    doublepath_com(&gamma, &lideal_even, lideal_commit_three, 
    NULL,  // not used ?
    basis_even_mid, basis_even_com, E_mid, k0, km,  E_com, a3, b3, verbose); // used only for image of BASIS_EVEN
    quat_alg_elem_finalize(&gamma);
    quat_left_ideal_finalize(&lideal_even); 
    return;
}

static void composed_rand_isog(ec_basis_t *image_two, ec_curve_t *Em_dd, ec_point_t *k0, ibz_t *d, int size_d)
{
        quat_left_ideal_t lideal_0;
        quat_alg_elem_t alpha, alpha_conj;
        ec_isog_odd_t iso_three;
        ibz_t tmp, remain, deg_odd;
        ibz_vec_2_t deg_three_ker_dlog;
        ec_point_t list_points[3];
        ec_curve_t E0_d, E0_dd;
        ec_basis_t basis_two, basis_three, basis_d;
        ibz_mat_2x2_t mat_alpha;
        digit_t scalarP[NWORDS_ORDER], scalarQ[NWORDS_ORDER];
        //ibz_vec_2_t vec_dd;

        ibz_init(&tmp);        
        ibz_init(&remain);
        ibz_init(&deg_odd);
        quat_alg_elem_init(&alpha);
        quat_alg_elem_init(&alpha_conj);
        quat_left_ideal_init(&lideal_0); 
        ibz_vec_2_init(&deg_three_ker_dlog);
        ibz_mat_2x2_init(&mat_alpha);
        ec_curve_init(&E0_d);
        ec_curve_init(&E0_dd);

       
        ibz_pow(&deg_odd, &ibz_const_two, size_d);
        ibz_sub(&deg_odd, &deg_odd, d);
        ibz_mul(&remain, d, &deg_odd);
        ibz_mul(&remain, &remain, &TORSION_PLUS_3POWER);
     

        int found = represent_integer(&alpha, &remain, &QUATALG_PINFTY);
    
        assert(quat_alg_is_primitive(&alpha, &MAXORD_O0, &QUATALG_PINFTY));
        quat_alg_conj(&alpha_conj, &alpha);
        quat_to_isog_power_of_three(&iso_three, &deg_three_ker_dlog,  &alpha_conj);

        copy_point(&basis_two.P, &BASIS_EVEN.P); 
        copy_point(&basis_two.Q, &BASIS_EVEN.Q); 
        copy_point(&basis_two.PmQ, &BASIS_EVEN.PmQ);
        matrix_of_endomorphism_even(&mat_alpha, &alpha); 
        matrix_application_even_basis_basic(&basis_two, &CURVE_E0, &mat_alpha);


        copy_point(list_points + 0, &basis_two.P); 
        copy_point(list_points + 1, &basis_two.Q); 
        copy_point(list_points + 2, &basis_two.PmQ);       
        ec_eval_three(&E0_dd, &iso_three, list_points, 3);

    // compute the isogeny Phi : E0 x E0_dd -> E0_d x F
    theta_chain_t isog;
    theta_couple_point_t T1, T2, T1m2;
    theta_couple_curve_t E0XE0dd;
    // preparing the domain
    copy_curve(&E0XE0dd.E1, &CURVE_E0);
    copy_curve(&E0XE0dd.E2, &E0_dd);

    // preparing the kernel
    copy_point(&T1.P1, &BASIS_EVEN.P);
    copy_point(&T2.P1, &BASIS_EVEN.Q);
    copy_point(&T1m2.P1, &BASIS_EVEN.PmQ);

    copy_point(&T1.P2, list_points + 0);
    copy_point(&T2.P2, list_points + 1);
    copy_point(&T1m2.P2, list_points + 2);

    ibz_invmod(&remain,  &TORSION_PLUS_3POWER, &TORSION_PLUS_2POWER);
    ec_mul_ibz(&T1.P2, &E0_dd, &remain, &T1.P2);
    ec_mul_ibz(&T2.P2, &E0_dd, &remain, &T2.P2);
    ec_mul_ibz(&T1m2.P2, &E0_dd, &remain, &T1m2.P2);



    ec_curve_t E0;
    E0 = CURVE_E0;
    ec_dbl_iter(&T1.P1, TORSION_PLUS_EVEN_POWER-size_d-2, &E0, &T1.P1);
    ec_dbl_iter(&T1.P2,TORSION_PLUS_EVEN_POWER-size_d-2, &E0_dd, &T1.P2);
    ec_dbl_iter(&T2.P1,TORSION_PLUS_EVEN_POWER-size_d-2, &E0, &T2.P1);
    ec_dbl_iter(&T2.P2, TORSION_PLUS_EVEN_POWER-size_d-2, &E0_dd, &T2.P2);
    ec_dbl_iter(&T1m2.P1,TORSION_PLUS_EVEN_POWER-size_d-2, &E0, &T1m2.P1);
    ec_dbl_iter(&T1m2.P2, TORSION_PLUS_EVEN_POWER-size_d-2, &E0_dd, &T1m2.P2);

    

   
    ibz_pow(&tmp, &ibz_const_two, size_d+2);
    //ibz_sub(&deg_odd, &tmp, d);
    ibz_invmod(&remain, &deg_odd, &tmp);
    ec_mul_ibz(&T1.P2, &E0_dd, &remain, &T1.P2);
    ec_mul_ibz(&T2.P2, &E0_dd, &remain, &T2.P2);
    ec_mul_ibz(&T1m2.P2, &E0_dd, &remain, &T1m2.P2);

    copy_point(list_points + 0,&T1.P2);
    copy_point(list_points + 1,&T2.P2);
    copy_point(list_points + 2,&T1m2.P2);



    int extra_info = 1;
    point_print("xP0:=", T1.P1);
    point_print("xQ0:=", T2.P1);
    point_print("xPmQ0:=", T1m2.P1);

    curve_print("add:=", E0_dd);
    point_print("xP_dd:=", T1.P2);
    point_print("xQ_dd:=", T2.P2);
    point_print("xPmQ_dd:=", T1m2.P2);
    assert(test_point_order_twof(&T1.P1, &CURVE_E0, size_d+2));
    assert(test_point_order_twof(&T2.P1, &CURVE_E0, size_d+2));
    assert(test_point_order_twof(&T1m2.P1, &CURVE_E0, size_d+2));
    assert(test_point_order_twof(&T1.P2, &E0_dd, size_d+2));
    assert(test_point_order_twof(&T2.P2, &E0_dd, size_d+2));
    assert(test_point_order_twof(&T1m2.P2, &E0_dd, size_d+2));

    //computation of the dim2 isogeny
    //TORSION_PLUS_EVEN_POWER

    theta_chain_comput_balanced(&isog,
                                size_d,
                                &E0XE0dd,
                                &T1,
                                &T2,
                                &T1m2);

    //printf("length is %d\n", isog.length);   
    theta_couple_point_t Tev1, Tev2, Tev1m2;

    theta_couple_point_t Teval1, Teval2, Teval3;

    ec_curve_to_basis_3(&basis_three, &E0_dd); 
    ec_set_zero(&Teval1.P1);
    ec_set_zero(&Teval2.P1);
    ec_set_zero(&Teval3.P1);
    copy_point(&Teval1.P2, &basis_three.P);
    copy_point(&Teval2.P2, &basis_three.Q);
    copy_point(&Teval3.P2, &basis_three.PmQ);


    theta_chain_eval_special_case(&Tev1, &isog, &Teval1, &E0XE0dd);
    theta_chain_eval_special_case(&Tev2, &isog, &Teval2, &E0XE0dd);
    theta_chain_eval_special_case(&Tev1m2, &isog, &Teval3, &E0XE0dd);
    //printf("length is %d\n", isog.length);   

    copy_point(&basis_d.P, &Tev1.P1);
    copy_point(&basis_d.Q, &Tev2.P1);
    copy_point(&basis_d.PmQ, &Tev1m2.P1);

    assert(test_point_order_threef(&basis_d.P, &isog.codomain.E1, EXPONENT_THREE));
    assert(test_point_order_threef(&basis_d.Q, &isog.codomain.E1, EXPONENT_THREE));
    assert(test_point_order_threef(&basis_d.PmQ, &isog.codomain.E1, EXPONENT_THREE));    
    copy_point(&Teval1.P1, k0);
    ec_set_zero(&Teval1.P2);
    theta_chain_eval_special_case(&Tev1, &isog, &Teval1, &E0XE0dd); 

    assert(test_point_order_threef(&Tev1.P1, &isog.codomain.E1, EXPONENT_THREE));

    ec_dlog_3(scalarP, scalarQ, &basis_d, &Tev1.P1, &isog.codomain.E1);
    ec_biscalar_mul(&Tev1.P1, &E0_dd, scalarP, scalarQ, &basis_three);
    isog_init_three(&iso_three, &E0_dd, &Tev1.P1, EXPONENT_THREE);
    ec_eval_three(Em_dd, &iso_three, list_points, 3);
    copy_point(&(image_two->P), list_points + 0);
    copy_point(&(image_two->Q), list_points + 1);
    copy_point(&(image_two->PmQ), list_points + 2);  
}


static void composed_rand_isog_faster(ec_basis_t *image_two, ec_curve_t *Em_dd, ec_point_t *k0, ibz_t *d, ibz_t *a3, ibz_t *b3, int size_d, int nrsp)
{
        quat_left_ideal_t lideal_0;
        quat_alg_elem_t alpha, alpha_conj;
        ec_isog_odd_t iso_odd;
        ibz_t tmp, remain, deg_odd;
        ibz_vec_2_t deg_minus_ker_dlog;
        ec_point_t list_points[4];
        ec_curve_t E0_dd;
        ec_basis_t basis_two, basis_three, basis_d;
        ibz_mat_2x2_t mat_alpha;
        ibz_vec_4_t dummy_coord;
        //ibz_vec_2_t vec_dd;

        ibz_init(&tmp);        
        ibz_init(&remain);
        ibz_init(&deg_odd);
        quat_alg_elem_init(&alpha);
        quat_alg_elem_init(&alpha_conj);
        quat_left_ideal_init(&lideal_0); 
        ibz_vec_2_init(&deg_minus_ker_dlog);
        ibz_mat_2x2_init(&mat_alpha);
        ec_curve_init(&E0_dd);
        ibz_vec_4_init(&dummy_coord);
        ibz_pow(&tmp, &ibz_const_two, size_d-nrsp);
        ibz_sub(&deg_odd, &tmp, d);
        ibz_mul(&remain, d, &deg_odd);
        ibz_mul(&remain, &remain, &TORSION_ODD_MINUS);
        int found = represent_integer(&alpha, &remain, &QUATALG_PINFTY);

        assert(quat_alg_is_primitive(&alpha, &MAXORD_O0, &QUATALG_PINFTY));
        quat_alg_conj(&alpha_conj, &alpha);
        matrix_of_endomorphism_odd(&mat_alpha, &alpha);
        ibz_mul(&deg_minus_ker_dlog[0], a3, &mat_alpha[0][0]);
        ibz_mul(&deg_minus_ker_dlog[1], b3, &mat_alpha[0][1]);
        ibz_add(&deg_minus_ker_dlog[0], &deg_minus_ker_dlog[0], &deg_minus_ker_dlog[1]);
        ibz_mod(&deg_minus_ker_dlog[0], &deg_minus_ker_dlog[0], &TORSION_PLUS_3POWER);
        ibz_mul(&tmp, a3, &mat_alpha[1][0]);
        ibz_mul(&deg_minus_ker_dlog[1], b3, &mat_alpha[1][1]);
        ibz_add(&deg_minus_ker_dlog[1], &tmp, &deg_minus_ker_dlog[1]);
        ibz_mod(&deg_minus_ker_dlog[1], &deg_minus_ker_dlog[1], &TORSION_PLUS_3POWER);
        ec_biscalar_mul_ibz(k0, &CURVE_E0, &(deg_minus_ker_dlog[0]), &(deg_minus_ker_dlog[1]), &BASIS_THREE, EXPONENT_THREE);


        copy_point(&basis_two.P, &BASIS_EVEN.P); 
        copy_point(&basis_two.Q, &BASIS_EVEN.Q); 
        copy_point(&basis_two.PmQ, &BASIS_EVEN.PmQ);
        matrix_of_endomorphism_even(&mat_alpha, &alpha); 
        matrix_application_even_basis_basic(&basis_two, &CURVE_E0, &mat_alpha);
        quat_to_isog_power_of_odd_minus(&iso_odd, &deg_minus_ker_dlog, &alpha_conj);

        copy_point(list_points + 0, &basis_two.P); 
        copy_point(list_points + 1, &basis_two.Q); 
        copy_point(list_points + 2, &basis_two.PmQ);  
        copy_point(list_points + 3, k0);  
        ec_eval_odd_minus(&E0_dd, &iso_odd, list_points, 4);
        ibz_pow(&tmp, &ibz_const_two, EXPONENT_TWO);

        ibz_invmod(&remain, &TORSION_ODD_MINUS, &tmp);
        ec_mul_ibz(list_points, &E0_dd, &remain, list_points);
        ec_mul_ibz(list_points + 1, &E0_dd, &remain, list_points + 1);
        ec_mul_ibz(list_points + 2, &E0_dd, &remain, list_points + 2);

        assert(test_point_order_twof(list_points + 0, &E0_dd, EXPONENT_TWO));
        assert(test_point_order_twof(list_points + 1, &E0_dd, EXPONENT_TWO));
        assert(test_point_order_twof(list_points + 2, &E0_dd, EXPONENT_TWO));



        assert(test_point_order_threef(list_points + 3, &E0_dd, EXPONENT_THREE));

        isog_init_three(&iso_odd, &E0_dd, list_points + 3, EXPONENT_THREE);
        ec_eval_three(Em_dd, &iso_odd, list_points, 4); 
        assert(test_point_order_twof(list_points + 0, Em_dd, EXPONENT_TWO));
        assert(test_point_order_twof(list_points + 1, Em_dd, EXPONENT_TWO));
        assert(test_point_order_twof(list_points + 2, Em_dd, EXPONENT_TWO));


        copy_point(&(image_two->P),  list_points + 0);
        copy_point(&(image_two->Q),  list_points + 1); 
        copy_point(&(image_two->PmQ),  list_points + 2);

        assert(test_point_order_twof(&(image_two->P), Em_dd, EXPONENT_TWO));
        assert(test_point_order_twof(&(image_two->Q), Em_dd, EXPONENT_TWO));
        assert(test_point_order_twof(&(image_two->PmQ), Em_dd, EXPONENT_TWO));        

        ec_dbl_iter(&(image_two->P), EXPONENT_TWO-size_d - 2 + nrsp, Em_dd, &(image_two->P));
        ec_dbl_iter(&(image_two->Q), EXPONENT_TWO-size_d - 2 + nrsp, Em_dd, &(image_two->Q));
        ec_dbl_iter(&(image_two->PmQ), EXPONENT_TWO-size_d - 2 + nrsp, Em_dd, &(image_two->PmQ));


        assert(test_point_order_twof(&(image_two->P), Em_dd, size_d + 2 - nrsp));
        assert(test_point_order_twof(&(image_two->Q), Em_dd, size_d + 2- nrsp));
        assert(test_point_order_twof(&(image_two->PmQ), Em_dd, size_d + 2- nrsp));

        ibz_pow(&tmp, &ibz_const_two, size_d + 2 - nrsp);
        ibz_invmod(&remain, &deg_odd, &tmp);
      

        ec_mul_ibz(&(image_two->P), Em_dd, &remain, &(image_two->P));
        ec_mul_ibz(&(image_two->Q), Em_dd, &remain, &(image_two->Q));
        ec_mul_ibz(&(image_two->PmQ), Em_dd, &remain, &(image_two->PmQ));
        assert(test_point_order_twof(&(image_two->P), Em_dd, size_d + 2 - nrsp));
        assert(test_point_order_twof(&(image_two->Q), Em_dd, size_d + 2 - nrsp));
        assert(test_point_order_twof(&(image_two->PmQ), Em_dd, size_d + 2 - nrsp));
}



static void push_rand_iso(ec_curve_t *E_aux,  ec_basis_t *basis_even_aux, ec_point_t *k0, ec_point_t *km, ec_curve_t *E_mid, 
    ec_curve_t *E_com, ec_basis_t *basis_even_mid,  ibz_t* d, ibz_t* a3, ibz_t* b3, int size_d, int nrsp) {    

    ibz_t tmp, remain, deg_three_power, deg_aux_d, deg_odd;
    ec_point_t list_points[3];
    ec_basis_t image_two_mdd, basis_d;
    ec_curve_t Em_dd;
    ec_isog_odd_t iso_three;


    ibz_init(&tmp);    
    ibz_init(&deg_odd);        
    ibz_init(&remain);
    ibz_init(&deg_three_power);
    ibz_init(&deg_aux_d);

    ec_point_init(list_points);
    ec_point_init(list_points+1);
    ec_point_init(list_points+2);
  
    ec_curve_init(&Em_dd);

    ibz_copy( &deg_aux_d, d); 

    composed_rand_isog_faster(&image_two_mdd, &Em_dd, k0, &deg_aux_d, a3, b3, size_d, nrsp);
   
    //composed_rand_isog(&image_two_mdd, &Em_dd, k0, &deg_aux_d, size_d);
    theta_chain_t isog;
    theta_couple_point_t T1, T2, T1m2;
    theta_couple_curve_t EmXEmdd;    



    // preparing the kernel
    copy_point(&T1.P1, &(basis_even_mid->P));
    copy_point(&T2.P1, &(basis_even_mid->Q));
    copy_point(&T1m2.P1, &(basis_even_mid->PmQ));

    copy_curve(&EmXEmdd.E1, E_mid);
    copy_curve(&EmXEmdd.E2, &Em_dd);


    copy_point(&T1.P2, &image_two_mdd.P);
    copy_point(&T2.P2, &image_two_mdd.Q);
    copy_point(&T1m2.P2, &image_two_mdd.PmQ);


    ec_dbl_iter(&T1.P1, TORSION_PLUS_EVEN_POWER-size_d-2, &EmXEmdd.E1, &T1.P1);
    ec_dbl_iter(&T2.P1,TORSION_PLUS_EVEN_POWER-size_d-2, &EmXEmdd.E1, &T2.P1);
    ec_dbl_iter(&T1m2.P1,TORSION_PLUS_EVEN_POWER-size_d-2, &EmXEmdd.E1, &T1m2.P1);

    theta_couple_point_t Tev1, Tev2, Tev1m2;
    theta_couple_point_t Teval1, Teval2, Teval3;

    copy_point(&Teval1.P1, &T1.P1);
    copy_point(&Teval2.P1, &T2.P1);
    copy_point(&Teval3.P1, &T1m2.P1);

    ec_set_zero(&Teval1.P2);
    ec_set_zero(&Teval2.P2);
    ec_set_zero(&Teval3.P2);

    ec_dbl_iter(&T1.P1, nrsp, &EmXEmdd.E1, &T1.P1);
    ec_dbl_iter(&T2.P1, nrsp, &EmXEmdd.E1, &T2.P1);
    ec_dbl_iter(&T1m2.P1,nrsp, &EmXEmdd.E1, &T1m2.P1);


    int extra_info = 1;

    assert(test_point_order_twof(&T1.P1, E_mid, size_d + 2 - nrsp));
    assert(test_point_order_twof(&T2.P1, E_mid, size_d + 2 - nrsp));
    assert(test_point_order_twof(&T1m2.P1, E_mid, size_d + 2 - nrsp));
    assert(test_point_order_twof(&T1.P2, &Em_dd, size_d + 2 - nrsp));
    assert(test_point_order_twof(&T2.P2, &Em_dd, size_d + 2 - nrsp));
    assert(test_point_order_twof(&T1m2.P2, &Em_dd, size_d + 2 - nrsp));
    assert(test_point_order_threef(km, E_mid, EXPONENT_THREE));

    theta_chain_comput_balanced(&isog,
                                size_d - nrsp,
                                &EmXEmdd,
                                &T1,
                                &T2,
                                &T1m2);





    theta_chain_eval_special_case(&Tev1, &isog, &Teval1, &EmXEmdd);
    theta_chain_eval_special_case(&Tev2, &isog, &Teval2, &EmXEmdd);
    theta_chain_eval_special_case(&Tev1m2, &isog, &Teval3, &EmXEmdd);
              
    copy_point(list_points, &Tev1.P1);
    copy_point(list_points+1, &Tev2.P1);
    copy_point(list_points+2, &Tev1m2.P1);

    assert(test_point_order_twof(list_points, &isog.codomain.E1, size_d+2));
    assert(test_point_order_twof(list_points+1, &isog.codomain.E1, size_d+2));
    assert(test_point_order_twof(list_points+2, &isog.codomain.E1, size_d+2));



    copy_point(&Teval1.P1, km);
    ec_set_zero(&Teval1.P2);
    assert(test_point_order_threef(km, E_mid, EXPONENT_THREE));
    theta_chain_eval_special_case(&Tev1, &isog, &Teval1, &EmXEmdd);
    assert(test_point_order_threef(&Tev1.P1, &isog.codomain.E1, EXPONENT_THREE));

    isog_init_three(&iso_three, &isog.codomain.E1, &Tev1.P1, EXPONENT_THREE);
    ec_eval_three(E_aux, &iso_three, list_points, 3);


    copy_point(&basis_even_aux->P, list_points);
    copy_point(&basis_even_aux->Q, list_points+1);
    copy_point(&basis_even_aux->PmQ, list_points+2);


    assert(test_point_order_twof(&basis_even_aux->P, E_aux, size_d+2));
    assert(test_point_order_twof(&basis_even_aux->Q, E_aux, size_d+2));
    assert(test_point_order_twof(&basis_even_aux->PmQ, E_aux, size_d+2));

    
}


void quat_lideal_conjugate_lattice(quat_lattice_t *lat, const quat_left_ideal_t *lideal) {
    ibz_mat_4x4_copy(&(lat->basis), &(lideal->lattice.basis));
    ibz_copy(&(lat->denom), &(lideal->lattice.denom));
    
    for (int row = 1; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            ibz_neg(&(lat->basis[row][col]),&(lat->basis[row][col]));
        }
    }

    return;
}

int is_good_norm(ibz_t *N) {
    if ((8 - (ibz_get(N) % 8)) != 5) return 0;

    ibz_t pow2, sum_of_squares_candidate;
    ibz_init(&pow2);
    ibz_init(&sum_of_squares_candidate);
    int res = 0;

    ibz_set(&pow2, 1);
    ibz_mul_2exp(&pow2, &pow2, RESPONSE_LENGTH);

    if(ibz_cmp(&pow2, N) < 0) {
        ibz_printf("WARNING: short vectors not short enough...\n2-pow = %Zd\nnorm = %Zd\n", &pow2, &N);
        // assert(0);
        ibz_finalize(&sum_of_squares_candidate);
        return 0;
    }

    ibz_sub(&sum_of_squares_candidate, &pow2, N);

    // unsigned int N_mod_four = ibz_mod_ui (&N, 4);
    assert(ibz_mod_ui (&sum_of_squares_candidate, 8) == 5);

    // if (N_mod_four == 1) {
    res = ibz_probab_prime(&sum_of_squares_candidate, 40);

    ibz_finalize(&pow2);
    ibz_finalize(&sum_of_squares_candidate);
    return res;
}


int is_good(quat_alg_elem_t *x, ibz_t const *lattice_content) {
    ibq_t N_q;
    ibz_t N, tmp, pow2;
    ibq_init(&N_q);
    ibz_init(&N);
    ibz_init(&tmp);
    int res = 0;


    quat_alg_norm(&N_q, x, &QUATALG_PINFTY);
    ibq_to_ibz(&N, &N_q);

    // ibz_printf(">>>> %Zd | %Zd\n", lattice_content, &N);

    assert(ibz_divides(&N, lattice_content));

    ibz_div(&N, &tmp, &N, lattice_content);

    res = is_good_norm(&N);


    #ifndef NDEBUG
        ibz_init(&pow2);
        ibz_set(&pow2, 1);
        ibz_mul_2exp(&pow2, &pow2, RESPONSE_LENGTH);
        int res2 = 0;
        if(ibz_cmp(&pow2, &N) < 0) {
            ibz_printf("WARNING: short vectors not short enough...\n2-pow = %Zd\nnorm = %Zd\n", &pow2, &N);
        }
        else {
            ibz_sub(&N, &pow2, &N);

            // unsigned int N_mod_four = ibz_mod_ui (&N, 4);
            unsigned int N_mod_eight = ibz_mod_ui (&N, 8);

            // if (N_mod_four == 1) {
            if (N_mod_eight == 5) {
                res2 = ibz_probab_prime(&N, 40);
            }
        }

        assert(res == res2);
        ibz_finalize(&pow2);
    #endif


    ibz_finalize(&N);
    ibq_finalize(&N_q);
    ibz_finalize(&tmp);
    return res;
}

void norm_from_2_times_gram(ibz_t *norm, ibz_mat_4x4_t *gram, ibz_vec_4_t *vec) {
    quat_qf_eval(norm, gram, vec);
    assert(ibz_is_even(norm));
    ibz_div_2exp(norm, norm, 1);
} 

void
sample_response(quat_alg_elem_t *x,
                const quat_lattice_t *lattice,
                ibz_t const *lattice_content,
                int verbose)
{
    ibz_mat_4x4_t lll;
    ibz_t denom_gram, norm;
    ibz_t bound;
    ibz_vec_4_t vec;

    ibz_mat_4x4_init(&lll);
    ibz_init(&denom_gram);
    ibz_init(&norm);
    ibz_init(&bound);
    ibz_vec_4_init(&vec);

    int err = quat_lattice_lll(&lll, lattice, &(QUATALG_PINFTY.p));
    assert(!err);

    // The shortest vector found by lll is our response
    ibz_mat_4x4_t prod, gram;
    ibz_mat_4x4_init(&prod);
    ibz_mat_4x4_init(&gram);
    //
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

    ibz_pow(&bound, &ibz_const_two, SQISIGN_response_length);

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

    // TODO make this a proper constant of the scheme
    // loop to find a correct answer
    while (!found && count < 50) {

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
            assert(quat_lattice_contains(NULL, lattice, x, &QUATALG_PINFTY));
        }

        count++;
    }

    // if not found then we just use the smallest vector of the lattice
    if (!found) {
        for (int i = 0; i < 4; i++) {
            ibz_copy(&x->coord[i], &lll[i][0]);
        }
        assert(quat_lattice_contains(NULL, lattice, x, &QUATALG_PINFTY));
        found = 1;
    }

    ibz_vec_4_finalize(&b_bound);

    ibz_finalize(&denom_gram);
    ibz_finalize(&norm);
    ibz_mat_4x4_finalize(&prod);
    ibz_mat_4x4_finalize(&gram);
    ibz_mat_4x4_finalize(&lll);
    ibz_vec_4_finalize(&vec);
    ibz_finalize(&bound);
    return;
}




void hash_to_challenge(ibz_vec_2_t *scalars, const ec_curve_t *curve, const unsigned char *message, const public_key_t *pk, size_t length)
{
    unsigned char *buf = malloc(sizeof(fp2_t) + sizeof(fp2_t) + length);
    {
        fp2_t j1, j2;
        ec_j_inv(&j1, curve);
        ec_j_inv(&j2, &pk->curve);
        memcpy(buf, &j1, sizeof(j1));
        memcpy(buf + sizeof(j1), &j2, sizeof(j2));
        memcpy(buf + sizeof(j1) + sizeof(j2), message, length);
    }

    {
        digit_t digits[NWORDS_FIELD];
        int xof_ret;

        xof_ret = sqisign_xof((unsigned char *)digits,
                              sizeof(digits),
                              buf,
                              sizeof(fp2_t) + sizeof(fp2_t) + length);
        if (xof_ret != 0) {
            free(buf);
            abort();
        }

        ibz_copy_digit_array(&(*scalars)[1], digits);
        ibz_mod(&(*scalars)[1], &(*scalars)[1], &TORSION_PLUS_3POWER);
    }

    ibz_set(&((*scalars)[0]), 1);
    free(buf);
}


int protocols_sign(signature_t *sig, const public_key_t *pk, const secret_key_t *sk, const unsigned char* m, size_t l, int verbose) {
    clock_t t = tic();
    
    int size_d, count;
    ec_curve_t E_com, E_mid, E_chl, E_aux;
    ec_basis_t Bcom0, Bcom_can, Bmid0, basis_even_aux, basis_even_aux_can;
    ec_basis_t pk_three, pk_two, resp_two;
    ibz_vec_2_t vec, vec_can;
    quat_left_ideal_t lideal_tmp; 
    quat_left_ideal_t lideal_commit_three, lideal_chall_three; 
    quat_left_ideal_t lideal_chall3_secret2, lideal_chall3_secret3;
    quat_lattice_t lattice_hom_chall_to_com, lat_commit;
    quat_alg_elem_t resp_quat;
    quat_alg_elem_t elem_tmp;
    ec_isog_odd_t  iso_chl_three;
    ibz_mat_2x2_t mat_alpha0, mat_tmp, mat_aux;
    ec_point_t k0, km, ker_chl, list_points[3];
    ibq_t temp_norm;
    ibz_t lattice_content, degree_full_resp, degree_odd_resp, remain, degree_aux, tmp, tmp2, a3, b3;
    ibz_vec_4_t dummy_coord;

    ibz_init(&tmp); ibz_init(&lattice_content); ibz_init(&tmp2);
    ibz_init(&degree_full_resp);ibz_init(&degree_odd_resp); ibz_init(&remain);
    ibz_init(&degree_aux); ibz_init(&a3); ibz_init(&b3);

    
    ibz_mat_2x2_init(&mat_alpha0);
    ibz_mat_2x2_init(&mat_tmp); 
    ibz_mat_2x2_init(&mat_aux);
    ibz_vec_4_init(&dummy_coord);

    quat_alg_elem_init(&resp_quat);
    quat_alg_elem_init(&elem_tmp);
    quat_lattice_init(&lattice_hom_chall_to_com); quat_lattice_init(&lat_commit);
    quat_left_ideal_init(&lideal_tmp);
    quat_left_ideal_init(&lideal_commit_three); quat_left_ideal_init(&lideal_chall_three);
    quat_left_ideal_init(&lideal_chall3_secret2); quat_left_ideal_init(&lideal_chall3_secret3);
    ibq_init(&temp_norm);
   
    ibz_vec_2_init(&vec); ibz_vec_2_init(&vec_can);

// fiat-shamir with aborts    
    retry:
        count =0;
        commit(&a3, &b3, &k0, &km, &E_mid, &E_com, &Bmid0, &Bcom0, &lideal_commit_three, verbose);
       
       
        hash_to_challenge(&vec_can, &E_com, m, pk, l);
        ibz_mat_2x2_eval(&vec, &(sk->mat_BAcan_to_BA0_three), &vec_can);
        id2iso_kernel_dlogs_to_ideal_three(&lideal_chall_three, &vec);
        assert(ibz_cmp(&lideal_chall_three.norm, &TORSION_PLUS_3POWER) == 0);
        quat_lideal_inter(&lideal_chall3_secret2, &lideal_chall_three, &(sk->secret_ideal_two), &QUATALG_PINFTY);
        quat_lideal_mul(&lideal_chall3_secret3, &lideal_chall3_secret2, &(sk->two_to_three_transporter), &QUATALG_PINFTY, 0); 
        quat_lideal_generator_coprime(&elem_tmp, &lideal_chall3_secret3, &ibz_const_one, &QUATALG_PINFTY, 0);
        quat_alg_conj(&elem_tmp, &elem_tmp);
        ibz_mul(&(elem_tmp.denom), &(elem_tmp.denom) , &(lideal_chall3_secret3.norm));
        quat_lideal_mul(&lideal_tmp, &lideal_chall3_secret3, &elem_tmp, &QUATALG_PINFTY, 0); 
        int test = quat_lideal_isom(&elem_tmp, &lideal_tmp, &lideal_chall3_secret3, &QUATALG_PINFTY);
        assert(test);
        quat_lideal_conjugate_lattice(&lat_commit, &lideal_commit_three);
        quat_lattice_intersect(&lattice_hom_chall_to_com, &lideal_tmp.lattice, &lat_commit);
        ibz_mul(&lattice_content, &(lideal_tmp.norm), &(lideal_commit_three.norm));
        if (verbose) TOC(t, "sample_response in");
        do{
            sample_response(&resp_quat, &lattice_hom_chall_to_com, &lattice_content, verbose);
            quat_alg_mul(&resp_quat, &resp_quat, &elem_tmp, &QUATALG_PINFTY); // bring it to intersection of lat_commit and lideal_chall3_secret3
            quat_alg_make_primitive(&dummy_coord, &tmp, &resp_quat, &MAXORD_O0, &QUATALG_PINFTY);
            int backtracking = two_adic_valuation(ibz_get(&tmp));
            ibz_pow(&tmp2, &ibz_const_two, backtracking);
            ibz_mul(&resp_quat.denom, &resp_quat.denom, &tmp2);  
            quat_alg_norm(&temp_norm, &resp_quat, &QUATALG_PINFTY);
            int is_int = ibq_to_ibz(&degree_odd_resp, &temp_norm); 
            assert(is_int);
            ibz_mul(&tmp, &(lideal_chall3_secret3.norm), &(lideal_commit_three.norm));
            ibz_div(&degree_full_resp, &remain, &degree_odd_resp, &tmp);
            ibz_mod(&tmp2, &degree_full_resp, &ibz_const_three);
            assert(ibz_cmp(&remain, &ibz_const_zero) == 0);
            count++;
            // if we can not find a suitable respone in 50 times, then we 
            //  gnerate a new commitment
            if(count >50)goto retry;
        }while(ibz_cmp(&tmp2, &ibz_const_zero)==0);


    sig->nrsp = two_adic_valuation(ibz_get(&degree_full_resp));
    ibz_pow(&tmp, &ibz_const_two, sig->nrsp);
    ibz_div(&degree_odd_resp, &remain, &degree_full_resp, &tmp);    
    assert(ibz_cmp(&remain, &ibz_const_zero) == 0);
   

    ibz_pow(&remain, &ibz_const_two, SQISIGN_response_length-sig->nrsp);
    ibz_sub(&degree_aux, &remain, &degree_odd_resp);
    ibz_mod(&remain, &degree_aux, &ibz_const_three);
    int verb=0;

    if(ibz_cmp(&remain, &ibz_const_zero)==0){
        ibz_pow(&remain, &ibz_const_two, SQISIGN_response_length + 1-sig->nrsp);
        ibz_sub(&degree_aux, &remain, &degree_odd_resp);
        verb=1;
    }
    size_d = SQISIGN_response_length+ verb;


    if (verbose) TOC(t, "sample_response out");



    quat_alg_conj(&resp_quat, &resp_quat);
    matrix_of_endomorphism_even(&mat_alpha0, &resp_quat); 

    ec_curve_to_basis_2(&(pk_two), &(pk->curve), EXPONENT_TWO); // canonical 
    ec_curve_to_basis_3(&(pk_three), &(pk->curve)); // canonical 
    ec_biscalar_mul_ibz(&ker_chl,
                        &pk->curve,
                        &vec_can[0],
                        &vec_can[1],
                        &pk_three, EXPONENT_THREE);
    ibz_2x2_inv_mod(&mat_tmp, &(sk->mat_BAcan_to_BA0_two), &TORSION_PLUS_2POWER);
    matrix_application_even_basis_basic(&(pk_two), &(pk->curve), &mat_tmp);
    copy_point(list_points + 0, &(pk_two.P)); 
    copy_point(list_points + 1, &(pk_two.Q)); 
    copy_point(list_points + 2, &(pk_two.PmQ));
    assert(test_point_order_twof(&(pk_two.P),  &(pk->curve), EXPONENT_TWO));
    assert(test_point_order_twof(&(pk_two.Q),  &(pk->curve), EXPONENT_TWO));
    assert(test_point_order_twof(&(pk_two.PmQ), &(pk->curve), EXPONENT_TWO));
    assert(test_point_order_threef(&(pk_three.P), &(pk->curve), EXPONENT_THREE));
    assert(test_point_order_threef(&(pk_three.Q), &(pk->curve), EXPONENT_THREE));
    assert(test_point_order_threef(&(pk_three.PmQ), &(pk->curve), EXPONENT_THREE));
    assert(test_point_order_threef(&ker_chl, &(pk->curve), EXPONENT_THREE));


    isog_init_three(&iso_chl_three, &pk->curve, &ker_chl, EXPONENT_THREE);

    ec_eval_three(&E_chl, &iso_chl_three, list_points, 3); 


    copy_point(&(resp_two.P), list_points+0); 
    copy_point(&(resp_two.Q), list_points+1); 
    copy_point(&(resp_two.PmQ), list_points+2);
    assert(test_point_order_twof(&(resp_two.P),  &E_chl, EXPONENT_TWO));
    assert(test_point_order_twof(&(resp_two.Q),  &E_chl, EXPONENT_TWO));
    assert(test_point_order_twof(&(resp_two.PmQ), &E_chl, EXPONENT_TWO));


    matrix_application_even_basis_basic(&resp_two, &E_chl, &mat_alpha0);
    ibz_mul(&tmp, &TORSION_PLUS_3POWER, &TORSION_PLUS_3POWER);
    ibz_mul(&tmp, &tmp, &TORSION_PLUS_3POWER);
    ibz_invmod(&lattice_content, &tmp, &TORSION_PLUS_2POWER);

    ec_mul_ibz(&(resp_two.P), &E_chl, &lattice_content, &(resp_two.P));
    ec_mul_ibz(&(resp_two.Q), &E_chl, &lattice_content, &(resp_two.Q));
    ec_mul_ibz(&(resp_two.PmQ), &E_chl, &lattice_content, &(resp_two.PmQ));

    // assert(test_point_order_twof(&(resp_two.P),  &E_chl, EXPONENT_TWO));
    // assert(test_point_order_twof(&(resp_two.Q),  &E_chl, EXPONENT_TWO));
    // assert(test_point_order_twof(&(resp_two.PmQ), &E_chl, EXPONENT_TWO));
    


    push_rand_iso(&E_aux, &basis_even_aux, &k0, &km, &E_mid, &E_com, &Bmid0,  &degree_aux, &a3, &b3, size_d, sig->nrsp);
    ec_normalize_curve(&E_aux);

    ec_dbl_iter(&(Bcom0.P), TORSION_PLUS_EVEN_POWER-size_d-2, &E_com, &(Bcom0.P));
    ec_dbl_iter(&(Bcom0.Q), TORSION_PLUS_EVEN_POWER-size_d-2, &E_com, &(Bcom0.Q));
    ec_dbl_iter(&(Bcom0.PmQ),TORSION_PLUS_EVEN_POWER-size_d-2, &E_com, &(Bcom0.PmQ));


    ec_curve_to_basis_2(&basis_even_aux_can, &E_aux, EXPONENT_TWO);
    ec_dbl_iter(&(basis_even_aux_can.P), TORSION_PLUS_EVEN_POWER-size_d - 2, &E_aux, &(basis_even_aux_can.P));
    ec_dbl_iter(&(basis_even_aux_can.Q), TORSION_PLUS_EVEN_POWER-size_d - 2, &E_aux, &(basis_even_aux_can.Q));
    ec_dbl_iter(&(basis_even_aux_can.PmQ),TORSION_PLUS_EVEN_POWER-size_d - 2, &E_aux, &(basis_even_aux_can.PmQ));



    change_of_basis_matrix_two(&mat_aux, &basis_even_aux_can, &basis_even_aux, &E_aux, size_d+2);
    

    ec_curve_to_basis_2(&pk_two, &E_chl, EXPONENT_TWO);
    ec_dbl_iter(&(pk_two.P), TORSION_PLUS_EVEN_POWER-size_d - 2, &E_chl, &(pk_two.P));
    ec_dbl_iter(&(pk_two.Q), TORSION_PLUS_EVEN_POWER-size_d - 2, &E_chl, &(pk_two.Q));
    ec_dbl_iter(&(pk_two.PmQ),TORSION_PLUS_EVEN_POWER-size_d - 2, &E_chl, &(pk_two.PmQ));

    ec_dbl_iter(&(resp_two.P), TORSION_PLUS_EVEN_POWER-size_d - 2, &E_chl, &(resp_two.P));
    ec_dbl_iter(&(resp_two.Q), TORSION_PLUS_EVEN_POWER-size_d - 2, &E_chl, &(resp_two.Q));
    ec_dbl_iter(&(resp_two.PmQ),TORSION_PLUS_EVEN_POWER-size_d - 2, &E_chl, &(resp_two.PmQ));
    change_of_basis_matrix_two(&mat_tmp,  &resp_two, &pk_two,  &E_chl, size_d + 2);



    //set the signature
    ibz_pow(&remain, &ibz_const_two, size_d + 2);
    ibz_2x2_mul_mod(&mat_tmp, &mat_tmp,  &mat_aux, &remain);
    ibz_invmod(&tmp, &degree_aux, &remain);
    ibz_mul(&tmp, &tmp, &degree_odd_resp);
    

    ibz_mod(&tmp, &tmp, &remain);
    ibz_mul(&mat_tmp[0][0], &mat_tmp[0][0], &tmp);
    ibz_mod(&mat_tmp[0][0], &mat_tmp[0][0], &remain);

    ibz_mul(&mat_tmp[0][1], &mat_tmp[0][1], &tmp);
    ibz_mod(&mat_tmp[0][1], &mat_tmp[0][1], &remain);

    ibz_mul(&mat_tmp[1][0], &mat_tmp[1][0], &tmp);
    ibz_mod(&mat_tmp[1][0], &mat_tmp[1][0], &remain);

    ibz_mul(&mat_tmp[1][1], &mat_tmp[1][1], &tmp);
    ibz_mod(&mat_tmp[1][1], &mat_tmp[1][1], &remain);

    ibz_mat_2x2_copy(&(sig->mat_sigma_phichall), &mat_tmp);
    copy_curve(&(sig->E_aux), &E_aux);
    sig->size_d=size_d;
    ibz_copy(&sig->chl, &vec_can[1]);





    ibz_finalize(&tmp); ibz_finalize(&lattice_content); ibz_finalize(&tmp2);
    ibz_finalize(&degree_full_resp);ibz_finalize(&degree_odd_resp); ibz_finalize(&remain);
    ibz_finalize(&degree_aux);

    ibz_mat_2x2_finalize(&mat_alpha0);
    ibz_mat_2x2_finalize(&mat_tmp); 
    ibz_mat_2x2_finalize(&mat_aux);

    quat_alg_elem_finalize(&resp_quat);
    quat_alg_elem_finalize(&elem_tmp);
    quat_lattice_finalize(&lattice_hom_chall_to_com); quat_lattice_finalize(&lat_commit);
    quat_left_ideal_finalize(&lideal_tmp);
    quat_left_ideal_finalize(&lideal_commit_three); quat_left_ideal_finalize(&lideal_chall_three);
    quat_left_ideal_finalize(&lideal_chall3_secret2); quat_left_ideal_finalize(&lideal_chall3_secret3);
    ibq_finalize(&temp_norm);
    return 0;
}


int
protocols_verif(signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l)
{
    int verif;
    ibz_t tmp;
    ibz_vec_2_t vec_chall, check_vec_chall;

    ec_point_t ker_chl;
    ec_basis_t bas2_EA, bas2_Eaux, bas3_EA;
    ec_curve_t E_chl,E_chld, Epk, E_aux;
    ec_isog_odd_t  iso_chl_three;
    ec_point_t list_points[3];

    ibz_init(&tmp);
    ibz_vec_2_init(&vec_chall);
    ibz_vec_2_init(&check_vec_chall);
    ec_point_init(&ker_chl);
    ec_curve_init(&E_chl);
    ec_curve_init(&E_chld);

    ec_curve_init(&Epk);
    ec_curve_init(&E_aux);


    clock_t t = tic();
    copy_curve(&Epk, &pk->curve);
    copy_curve(&E_aux, &sig->E_aux);

   
    // computing the challenge isogeny
    ec_curve_to_basis_3(&bas3_EA, &Epk);
    ibz_set(&vec_chall[0], 1);
    ibz_copy(&vec_chall[1], &sig->chl);
    ec_biscalar_mul_ibz(&ker_chl,
                        &Epk,
                        &vec_chall[0],
                        &vec_chall[1],
                        &bas3_EA, EXPONENT_THREE);    
    isog_init_three(&iso_chl_three, &Epk, &ker_chl, EXPONENT_THREE);
    ec_eval_three(&E_chl, &iso_chl_three, NULL, 0);


    //generating the kernel of the (2^n,2^n)-isogeny
    ec_curve_to_basis_2(&bas2_Eaux, &E_aux, EXPONENT_TWO);
    ec_curve_to_basis_2(&bas2_EA, &E_chl, EXPONENT_TWO);

    ec_dbl_iter(&(bas2_EA.P), TORSION_PLUS_EVEN_POWER-sig->size_d-2, &E_chl, &(bas2_EA.P));
    ec_dbl_iter(&(bas2_EA.Q), TORSION_PLUS_EVEN_POWER-sig->size_d-2, &E_chl, &(bas2_EA.Q));
    ec_dbl_iter(&(bas2_EA.PmQ),TORSION_PLUS_EVEN_POWER-sig->size_d-2, &E_chl, &(bas2_EA.PmQ));

    ec_dbl_iter(&(bas2_Eaux.P), TORSION_PLUS_EVEN_POWER-sig->size_d-2, &E_aux, &(bas2_Eaux.P));
    ec_dbl_iter(&(bas2_Eaux.Q), TORSION_PLUS_EVEN_POWER-sig->size_d-2, &E_aux, &(bas2_Eaux.Q));
    ec_dbl_iter(&(bas2_Eaux.PmQ),TORSION_PLUS_EVEN_POWER-sig->size_d-2, &E_aux, &(bas2_Eaux.PmQ));


    matrix_application_even_basis(&bas2_EA,
                                  &E_chl,
                                  &sig->mat_sigma_phichall,
                                  sig->size_d+2); 

    theta_couple_curve_t EauxXEchld;
    theta_chain_t isog;
    memset(&isog, 0, sizeof(isog)); /* PATCH (nist-sigs-zoo, fix) */
    theta_couple_point_t T1, T2, T1m2;
    copy_curve(&EauxXEchld.E1, &E_aux);

    ec_dbl_iter(&(bas2_Eaux.P), sig->nrsp, &E_aux, &(bas2_Eaux.P));
    ec_dbl_iter(&(bas2_Eaux.Q), sig->nrsp, &E_aux, &(bas2_Eaux.Q));
    ec_dbl_iter(&(bas2_Eaux.PmQ), sig->nrsp, &E_aux, &(bas2_Eaux.PmQ));
    copy_point(&T1.P1, &bas2_Eaux.P);
    copy_point(&T2.P1, &bas2_Eaux.Q);
    copy_point(&T1m2.P1, &bas2_Eaux.PmQ);
    
    if(sig->nrsp==0){
        copy_curve(&E_chld, &E_chl);
        copy_curve(&EauxXEchld.E2, &E_chld);
        copy_point(&T1.P2, &bas2_EA.P);
        copy_point(&T2.P2, &bas2_EA.Q);
        copy_point(&T1m2.P2, &bas2_EA.PmQ);
        assert(test_point_order_twof(&(T1.P2),  &E_chld, sig->size_d+2));
        assert(test_point_order_twof(&(T2.P2),  &E_chld, sig->size_d+2));
        assert(test_point_order_twof(&(T1m2.P2),  &E_chld, sig->size_d+2));

    }else{
        if(ibz_get(&sig->mat_sigma_phichall[0][0]) % 2 == 0 && ibz_get(&sig->mat_sigma_phichall[1][0]) % 2 == 0){
            assert(test_point_order_twof(&(bas2_EA.Q),  &E_chl, sig->size_d+2));
            copy_point(&ker_chl, &bas2_EA.Q);
        }else{
            copy_point(&ker_chl, &bas2_EA.P);
            assert(test_point_order_twof(&(bas2_EA.P),  &E_chl, sig->size_d+2));
        }

        ec_dbl_iter(&ker_chl, sig->size_d+2-sig->nrsp, &E_chl, &ker_chl);
        assert(test_point_order_twof(&ker_chl, &E_chl, sig->nrsp));
        copy_point(list_points + 0, &(bas2_EA.P)); 
        copy_point(list_points + 1, &(bas2_EA.Q)); 
        copy_point(list_points + 2, &(bas2_EA.PmQ));
        copy_curve(&E_chld, &E_chl);
        ec_eval_small_chain(&E_chld, &ker_chl, sig->nrsp, list_points, 3);

        copy_curve(&EauxXEchld.E2, &E_chld);

        copy_point(&T1.P2, list_points + 0);
        copy_point(&T2.P2, list_points + 1);
        copy_point(&T1m2.P2, list_points + 2);
    }

    assert(test_point_order_twof(&(T1.P1),  &E_aux, sig->size_d+2-sig->nrsp));
    assert(test_point_order_twof(&(T2.P1),  &E_aux, sig->size_d+2-sig->nrsp));
    assert(test_point_order_twof(&(T1m2.P1),  &E_aux, sig->size_d+2-sig->nrsp));

    assert(test_point_order_twof(&(T1.P2),  &E_chld, sig->size_d+2-sig->nrsp));
    assert(test_point_order_twof(&(T2.P2),  &E_chld, sig->size_d+2-sig->nrsp));
    assert(test_point_order_twof(&(T1m2.P2),  &E_chld, sig->size_d+2-sig->nrsp));


    /* PATCH (nist-sigs-zoo, fix): reject when the chain does not split */
    int is_split = theta_chain_comput_balanced_checked(&isog,
                                sig->size_d-sig->nrsp,
                                &EauxXEchld,
                                &T1,
                                &T2,
                                &T1m2);

    
    if (is_split) {
        hash_to_challenge(&check_vec_chall, &isog.codomain.E1, m, pk, l);
        verif = (ibz_cmp(&vec_chall[1], &check_vec_chall[1]) == 0);
    } else {
        verif = 0;
    }
    // if (verif==1){
    //     printf("The signature1 of SQIsign2D-push1/2 is sucessful!!!!\n");
    // }
    // else{
    //     printf("The signature1 of SQIsign2D-push1/2 is failed!!!!\n");
       
    // }


    ibz_finalize(&tmp);
    ibz_vec_2_finalize(&vec_chall);
    ibz_vec_2_finalize(&check_vec_chall);
    return verif;
}
