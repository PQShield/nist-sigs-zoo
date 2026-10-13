/* PATCH (nist-sigs-zoo): copy of the submitted yuanyang-1024 keygen/ntrugen/src/ntrugen.c (Optimized_Implementation;
 * identical to the yuanyang-512 copy and, apart from two `#pragma omp simd` lines, to the Reference_Implementation
 * copies), CRLF converted to LF. One functional change: the debug `assert(maxe<0.1)` before the precision check of
 * the reconstruction step is removed, so a lack of floating-point precision takes the existing `return -2`
 * (NTRUGEN_ERR_VERIFY) path and keygen samples another (f, g) pair instead of aborting the process. This is what
 * the submitters' own yuanyang-2048 copy does. See docs/ngcc-submission-bugs.md item 18.
 * Build-only change: the fft.h include names keygen/ntrugen/src explicitly (this copy lives elsewhere, and the
 * instance directory, searched first, has a different fft.h). */
#include "complex.h"
#include "ntrugen/src/fft.h" /* PATCH: was "fft.h"; this copy lives outside keygen/ntrugen/src, and the instance
                                * directory (searched first) has its own fft.h. Resolved via the keygen include dir. */
#include "cfft.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>

#ifndef NTRUGEN_DEBUG
#define NTRUGEN_DEBUG 0
#endif

#if NTRUGEN_DEBUG
#define NTRUGEN_LOG(...) printf(__VA_ARGS__)
#else
#define NTRUGEN_LOG(...) ((void)0)
#endif

extern const int64_t precision[];
extern const uint64_t twiddle_abs[];

static double ntrugen_time_now(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (double)tv.tv_sec+1e-6*(double)tv.tv_usec;
}

static void ntrugen_time_log(const char *part,int logk,double seconds){
#if NTRUGEN_DEBUG
	printf("time %s logk=%d %.6f s\n",part,logk,seconds);
#else
	(void)part;
	(void)logk;
	(void)seconds;
#endif
}

static void ntrugen_time_log_ext_gcd(double seconds){
#if NTRUGEN_DEBUG
	printf("time ext_gcd %.6f s\n",seconds);
#else
	(void)seconds;
#endif
}


void normcomplex(Complex *r,Complex *a,uint64_t *work){
	r->re.n=2*a->re.n;
	r->im.n=2*a->im.n;
	bigint_mul_karatsuba(&a->re,&a->re,&r->re,work,2);
	bigint_mul_karatsuba(&a->im,&a->im,&r->im,work,2);
	bigint_add(&r->re,&r->re,&r->im);
}


void sizered(double *restrict F,int logn,const double *restrict f,const double *restrict g,double *restrict tmp,const double *restrict twid,int q){
	size_t n=(size_t)1<<logn,hn=n/2;
#pragma omp simd
	for(size_t i=0;i<n/2;i++){
		double normg=g[i]*g[i]+g[i+hn]*g[i+hn];
		double prodr=f[i]*F[i]-f[i+hn]*F[i+hn];
		double prodi=f[i]*F[i+hn]+f[i+hn]*F[i];
		double Gr=((q-prodr)*g[i]-prodi*g[i+hn])/normg;
		double Gi=(-(q-prodr)*g[i+hn]-prodi*g[i])/normg;
		double normf=f[i]*f[i]+f[i+hn]*f[i+hn];
		double lr=(g[i]*F[i]+g[i+hn]*F[i+hn]-f[i]*Gr-f[i+hn]*Gi)/(normf+normg);
		double li=(g[i]*F[i+hn]-g[i+hn]*F[i]-f[i]*Gi+f[i+hn]*Gr)/(normf+normg);
		tmp[i]=lr;
		tmp[i+hn]=li;
//		printf("%lf+I*%lf\n",lr,li);
	}
	double_negacyclic_iFFT(tmp,logn,twid);
//	puts("quo:");
	for(size_t i=0;i<n;i++){
		tmp[i]=llround(tmp[i]);
//		printf("%lld ",llround(tmp[i]));
	}
//	puts("");
	double_negacyclic_FFT(tmp,logn,twid);
	for(size_t i=0;i<hn;i++){
		double pr=tmp[i]*g[i]-tmp[i+hn]*g[i+hn];
		double pi=tmp[i+hn]*g[i]+tmp[i]*g[i+hn];
		F[i]-=pr;
		F[i+hn]-=pi;
	}
	memcpy(tmp,F,8*n);
	double_negacyclic_iFFT(F,logn,twid);
	for(size_t i=0;i<n;i++)
		F[i]=llround(F[i]);
//	puts("");
	if(q==1)
		return;

#pragma omp simd
	for(size_t i=0;i<hn;i++){
		double normg=g[i]*g[i]+g[i+hn]*g[i+hn];
		double prodr=f[i]*tmp[i]-f[i+hn]*tmp[i+hn];
		double prodi=f[i]*tmp[i+hn]+f[i+hn]*tmp[i];
		double Gr=((q-prodr)*g[i]-prodi*g[i+hn])/normg;
		double Gi=(-(q-prodr)*g[i+hn]-prodi*g[i])/normg;
		tmp[i]=Gr;
		tmp[i+hn]=Gi;
	}
	double_negacyclic_iFFT(tmp,logn,twid);
}

int ntrugen(const int8_t *f,const int8_t *g,int32_t *F,int32_t *G,int logn,int q,uint64_t *work){
	size_t n=(size_t)1<<logn,i;
	const uint64_t *initwork=work;
	(void)initwork;
	double *fd=(double*)work;work+=n;
	double *gd=(double*)work;work+=n;
	double *twid=(double*)work;work+=n*2;
	double *nf=(double*)work;work+=n/4;
	double *ng=(double*)work;work+=n/4;
	double *ntwid=(double*)work;work+=n/2;
	for(i=0;i<n;i++)
		fd[i]=f[i];
	for(i=0;i<n;i++)
		gd[i]=g[i];
	make_twiddles(logn,twid);
	make_twiddles(logn-2,ntwid);
/*	for(int i=0;i<n;i++)
		printf("%lf ",fd[i]);
	puts("");*/
	double_negacyclic_FFT(fd,logn,twid);
	double_negacyclic_FFT(gd,logn,twid);
	double_FFT_fold4(fd,logn,nf);
/*	for(int i=0;i<n/4;i++)
		printf("%lf ",nf[i]);
	puts("");*/
	double_FFT_fold4(gd,logn,ng);
	double_negacyclic_iFFT(nf,logn-2,ntwid);
	double_negacyclic_iFFT(ng,logn-2,ntwid);
/*	puts("NF");
	for(int i=0;i<n/4;i++)
		printf("%lf ",nf[i]);
	puts("");*/
	Complex *cf=(Complex*)work;work+=sizeof(Complex)*(n/4)/8;
	Complex *cg=(Complex*)work;work+=sizeof(Complex)*(n/4)/8;
	Complex *cnf=(Complex*)work;work+=sizeof(Complex)*(n/4)/8;
	Complex *cng=(Complex*)work;work+=sizeof(Complex)*(n/4)/8;
	Complex *ctwid=(Complex*)work;work+=sizeof(Complex)*(n/2)/8;
	const uint64_t *tw_abs=twiddle_abs;
	int maxi=1;
	int maxilogk[20];
	for(int logk=logn-2;logk>1;logk--){
		double ntrugen_t0=ntrugen_time_now();
		size_t k=(size_t)1<<logk,p=precision[logk-2];
		maxi++;
		int precfft=(maxi*1.2)*2+1;
		NTRUGEN_LOG("k=%zu p=%zu\n",k,p);
		for(size_t i=0;i<k/2;i++){
			cf[i].re.d=work;cf[i].re.al=2*p;work+=2*p;
			cf[i].im.d=work;cf[i].im.al=2*p;work+=2*p;
			cf[i].e=0;
		}
		for(size_t i=0;i<k/2;i++){
			cg[i].re.d=work;cg[i].re.al=2*p;work+=2*p;
			cg[i].im.d=work;cg[i].im.al=2*p;work+=2*p;
			cg[i].e=0;
		}
		complex_make_twiddles(ctwid,logk, tw_abs,-p);
		if(logk==logn-2)
			for(size_t i=0;i<k/2;i++){
				bigint_scalar(&cf[i].re,llround(nf[i]));
				bigint_scalar(&cf[i].im,llround(nf[i+n/8]));
				bigint_scalar(&cg[i].re,llround(ng[i]));
				bigint_scalar(&cg[i].im,llround(ng[i+n/8]));
			}
		else{
			for(size_t i=0;i<k/2;i++){
				bigint_copy(&cf[i].re,&cnf[2*i].re);
				bigint_copy(&cg[i].re,&cng[2*i].re);
				bigint_copy(&cf[i].im,&cnf[2*i].im);
				bigint_copy(&cg[i].im,&cng[2*i].im);
			}
			cnf+=k;
			cng+=k;
		}
		for(size_t i=0;i<k/2;i++){
			cnf[i].re.d=work;cnf[i].re.al=4*p;work+=4*p;
			cnf[i].im.d=work;cnf[i].im.al=4*p;work+=4*p;
			cng[i].re.d=work;cng[i].re.al=4*p;work+=4*p;
			cng[i].im.d=work;cng[i].im.al=4*p;work+=4*p;
//			printf("e=%ld p=%zu n=%zu al=%zu\n",cf[i].e,p,cf[i].re.n,cf[i].re.al);
			complex_precision_add(&cf[i],precfft/2);
//			printf("e=%ld p=%zu n=%zu al=%zu\n",cf[i].e,p,cf[i].re.n,cf[i].re.al);
			complex_precision_add(&cg[i],precfft/2);
		}

//		for(int i=0;i<k/2;i++)
//			bigint_printx(&cf[i].re);
		negacyclic_FFT(cf,logk,ctwid,0,precfft,work);
/*		puts("output:");
		for(int i=0;i<k/2;i++)
			complex_print(&cf[i]);*/
		complex_FFT_fold(cf, logk,cnf,work);
/*		puts("FOLD:");
		for(int i=0;i<k/4;i++)
			complex_print(&cnf[2*i]);*/
		negacyclic_iFFT(cnf,logk,ctwid,1,maxi*2,work);
//		puts("END IFFT F:");
		negacyclic_FFT(cg,logk,ctwid,0,precfft,work);
		complex_FFT_fold(cg, logk,cng,work);
		negacyclic_iFFT(cng,logk,ctwid,1,maxi*2,work);
//		puts("RESULT NORMF:");
		for(size_t i=0;i<k/2;i++){
			complex_round(&cnf[i]);
//			complex_print(&cnf[i]);
			complex_round(&cng[i]);
		}
		maxi=1;
		for(size_t i=0;i<k/2;i+=2){
			complex_round(cnf+i);
			complex_trim(cnf+i);
//			complex_print(cnf+i);
			maxi=fmax(maxi,fmax(cnf[i].re.n,cnf[i].im.n));
			maxi=fmax(maxi,fmax(cng[i].re.n,cng[i].im.n));
		}
		NTRUGEN_LOG("maxi=%d\n",maxi);
		maxilogk[logk]=maxi;
/*		puts("RESULT NORMg:");
		for(int i=0;i<k/2;i++)
			complex_print(&cng[i]);*/
		assert((size_t)maxi<=p);
		if(logk>2){
			ctwid+=k;
			cf+=k/2;
			cg+=k/2;
			tw_abs+=2*k*p;
		}
		ntrugen_time_log("begin",logk,ntrugen_time_now()-ntrugen_t0);
	}
	size_t p=maxi;
//	complex_print(&cnf[0]);
	normcomplex(&cnf[1],&cnf[0],work);
//	puts("g");complex_print(cng);
//	puts("resultants");
//	bigint_printx(&cnf[1].re);
	normcomplex(&cng[1],&cng[0],work);
//	bigint_printx(&cng[1].re);
	double ntrugen_ext_gcd_t0=ntrugen_time_now();
	int ntrugen_ext_gcd_ok=bigint_ext_gcd(&cnf[1].re,&cnf[1].re,&cng[1].re,work);
	ntrugen_time_log_ext_gcd(ntrugen_time_now()-ntrugen_ext_gcd_t0);
	if(!ntrugen_ext_gcd_ok)
		return -1;
	double ntrugen_res_t0=ntrugen_time_now();
	Complex quo;
	quo.re.d=work;quo.re.al=6*p;work+=6*p;
	quo.im.d=work;quo.im.al=6*p;work+=6*p;
	NTRUGEN_LOG("Arena size>=%zu\n",work-initwork);
//	puts("inv");
//	complex_print(cnf+1);
	complex_inv(&cng[1],&cng[0],p,work);
//	complex_print(cng+1);
	bigint_copy(&cnf[1].im,&cnf[1].re);
	int shift=cnf[1].re.n>p ? cnf[1].re.n-p : 0;
//	printf("shift=%d\n",shift);
	bigint_shr(&cnf[1].im,shift);
	bigint_mul_karatsuba(&cnf[1].im,&cng[1].re,&quo.re,work,4);
	bigint_mul_karatsuba(&cnf[1].im,&cng[1].im,&quo.im,work,4);
	quo.e=cng[1].e+shift;
//	complex_print(&quo);
	complex_round(&quo); // true quotient
//	puts("quotient:");
//	complex_print(&quo);
	complex_mul(&quo,&cng[0],&quo,work);
//	complex_print(&quo);
	cnf[1].im.n=0;
	complex_sub(cnf+1,cnf+1,&quo);
	complex_trim(cnf+1);
	complex_conj(cnf);
//	puts("new solution");
//	complex_print(cnf+1);
	complex_mul(cnf+1,cnf+1,cnf,work);
//	complex_print(cnf+1);
	complex_mul(&quo,cng+1,cnf+1,work);
//	complex_print(&quo);
	complex_round(&quo);
//	puts("quotient:");
//	complex_print(&quo);
	complex_mul(&quo,&quo,cng,work);
//	puts("quotient mul:");
//	complex_print(cnf+1);
//	complex_print(&quo);
	complex_sub(cnf,cnf+1,&quo);
//	puts("new solution");
//	complex_print(cnf);
	Complex *resf=cnf;
	ntrugen_time_log("middle",1,ntrugen_time_now()-ntrugen_res_t0);
	work-=8*p;
	for(int logk=2;logk<=logn-2;logk++){
		double ntrugen_t0=ntrugen_time_now();
		size_t k=(size_t)1<<logk,p=precision[logk-2];
		int maxires=maxi;
		maxi=fmax(maxi,maxilogk[logk])*1.3;
		maxi+=logk<logn-2;
		NTRUGEN_LOG("k=%zu p=%zu using maxi=%d\n",k,p,maxi);
		for(size_t i=0;i<k/4;i++){
			complex_copy(&cnf[2*i+(k==4)],&resf[i]); // odd elements are not used so we store an exact version when k==4 (as cnf==resf)
			complex_trim(cnf+2*i);
//			cnf[2*i+1].re.n=cnf[2*i+1].im.n=0;
//			cnf[2*i+1].e=0;
			complex_precision_add(&cnf[2*i],cnf[2*i].e-maxires/2);
		}
/*		puts("cng before/fft");
		for(int i=0;i<k/2;i++)
			complex_print(cnf+i);*/
		// We compute cnf/g
		negacyclic_FFT(cnf,logk,ctwid,1,maxi/2+1,work);
		for(size_t i=0;i<k/2;i++){
//			printf("g %d:",i);complex_print(cg+i);
			complex_inv(cng+i,cg+i,maxi/2+1,work);
//			printf("inv%d:",i);complex_print(cng+i);
//			printf("u%d:",i);complex_print(cnf+i);
//			complex_print(cnf+i);
//			puts("mul fft:");
//			complex_print(cnf+i-(i%2));
			complex_mul(cng+i,cng+i,cnf+i-(i%2),work);
//			puts("quo fft:");
//			complex_print(cng+i);
			complex_precision_add(cng+i,cng[i].e+1);
//			printf("result%d:",i);complex_print(cng+i);
		}
		negacyclic_iFFT(cng,logk,ctwid,0,maxi/2+1,work);
//		puts("quotient:");
		for(size_t i=0;i<k/2;i++){
			complex_round(cng+i);
//			complex_print(cng+i);
			complex_precision_add(cng+i,maxi);
		}
		// Now we compute cnf%g
		negacyclic_FFT(cng,logk,ctwid,0,maxi,work);
		for(size_t i=0;i<k/2;i++){
/*			puts("mul fft:");
			complex_print(cng+i);
			complex_print(cg+i);*/
			complex_mul(cng+i,cng+i,cg+i,work);
			complex_precision_add(cng+i,cng[i].e+1);
//			complex_print(cng+i);
		}
		negacyclic_iFFT(cng,logk,ctwid,0,maxi,work);
//		puts("prod:");
		for(size_t i=0;i<k/2;i++){
			complex_round(cng+i);
//			complex_trim(cng+i);complex_print(cng+i);
			if(i%2)
				complex_neg(cnf+i,cng+i);
			else{
				complex_round(resf+i/2+(k==4)); // When k==4 we use the exact value
				complex_sub(cnf+i,&resf[i/2+(k==4)],cng+i);
			}
			complex_trim(cnf+i);
			complex_precision_add(cnf+i,maxi/2+1);
		}
/*		puts("reduced:");
		for(int i=0;i<k/2;i++)
			complex_print(cnf+i);*/
		// cnf is reduced, we multiply by conj(f) and the quotient by g
		negacyclic_FFT(cnf,logk,ctwid,0,maxi+1,work);
		for(size_t i=0;i<k/2;i++){
//			printf("f %d:",i);complex_print(cg+i);
			complex_inv(cng+i,cg+i,maxi/2+1,work);
//			printf("inv%d:",i);complex_print(cng+i);
//			complex_precision_add(cng+i,cng[i].e+p/2+1);
//			puts("mul fft:");
//			complex_print(cnf+i);
			complex_mul(cnf+i,cf+i+1-(i%2)*2,cnf+i,work);
			complex_precision_add(cnf+i,cnf[i].e+maxi/2+1);
//			puts("quo fft:");
//			complex_print(cng+i);
			complex_mul(cng+i,cnf+i,cng+i,work);
			complex_precision_add(cng+i,cng[i].e+1);
			complex_precision_add(cnf+i,cnf[i].e+1);
//			printf("result%d:",i);complex_print(cnf+i);
		}
		negacyclic_iFFT(cng,logk,ctwid,0,maxi/2+1,work);
//		puts("quotient:");
		for(size_t i=0;i<k/2;i++){
			complex_round(cng+i);
//			complex_print(cng+i);
			complex_precision_add(cng+i,maxi/2+1);
		}
		// quotient is computed, so we compute the final result: cnf*conj(f) % g
		negacyclic_FFT(cng,logk,ctwid,0,maxi+1,work);
		for(size_t i=0;i<k/2;i++){
//			puts("mul fft:");
//			complex_print(cng+i);
			complex_mul(cng+i,cng+i,cg+i,work);
			complex_precision_add(cng+i,cng[i].e+1);
			complex_sub(cnf+i,cnf+i,cng+i);
			complex_trim(cnf+i);
		}
		negacyclic_iFFT(cnf,logk,ctwid,0,maxi/2+1,work);
//		puts("rereduced");
		for(size_t i=0;i<k/2;i++){
			complex_round(cnf+i);
			complex_trim(cnf+i);
//			complex_print(cnf+i);
		}
		if(1){
			for(size_t i=0;i<k/2;i++){
				complex_precision_add(cnf+i,cnf[i].e+p/2);
			}
			negacyclic_FFT(cnf,logk,ctwid,0,p,work);
			for(size_t i=0;i<k/2;i++){
				complex_inv(cng+i,cg+i,p/2+1,work);
				complex_copy(cg+i,cng+i);
				complex_mul(cng+i,cf+i,cnf+i,work);
				complex_precision_add(cng+i,cng[i].e+p/2+1);
				complex_neg1(cng+i);
				cng[i].re.d[-cng[i].e]+=cng[i].re.s;
				bigint_fast_normalize_signed(&cng[i].re);
				complex_mul(cng+i,cng+i,cg+i,work);
				complex_precision_add(cng+i,cng[i].e+1);
			}
			negacyclic_iFFT(cng,logk,ctwid,0,p,work);
			double maxe=0;
			for(size_t i=0;i<k/2;i++){
				double t;
				t=cng[i].re.d[0]/(1.*(1ull<<BIGINT_BASE));
				maxe=fmax(maxe,fabs(round(t)-t));
				t=cng[i].im.d[0]/(1.*(1ull<<BIGINT_BASE));
				maxe=fmax(maxe,fabs(round(t)-t));
			}
			NTRUGEN_LOG("max erreur=%lf\n",maxe);
			/* PATCH (nist-sigs-zoo): assert(maxe<0.1) removed; insufficient precision is a rejected candidate,
			 * keygen samples another pair (as in the submitted yuanyang-2048 copy). */
			if(maxe>0.1)
				return -2;
			negacyclic_iFFT(cnf,logk,ctwid,0,p,work);
			for(size_t i=0;i<k/2;i++){
				complex_round(cnf+i);
				complex_trim(cnf+i);
			}
		}

//		puts("prod reduced:");
		maxi=0;
		for(size_t i=0;i<k/2;i++){
//			complex_print(cnf+i);
			maxi=fmax(maxi,fmax(cnf[i].re.n,cnf[i].im.n));
		}
		NTRUGEN_LOG("maxi=%d\n",maxi);
		assert((size_t)maxi<=p/2+1);
		if(k<n/4){
			resf=cnf;
			ctwid-=k*2;
			cf-=k;
			cg-=k;
			cnf-=k;
			cng-=k;
			work-=k*p*2;
		}
		ntrugen_time_log("end",logk,ntrugen_time_now()-ntrugen_t0);
	}
	double *Fd=(double*)work;work+=n;
	double *Gd=(double*)work;work+=n;
	memset(Fd,0,8*n);
	for(size_t i=0;i<n/8;i++){
		Fd[4*i    ]=cnf[i].re.n ? (int64_t)cnf[i].re.d[0]*cnf[i].re.s : 0;
		Fd[4*i+n/2]=cnf[i].im.n ? (int64_t)cnf[i].im.d[0]*cnf[i].im.s : 0;
	}
	double_negacyclic_FFT(Fd,logn,twid);
	sizered(Fd,logn,fd,gd,Gd,twid,1);
/*	for(int i=0;i<n;i++)
		printf("%lld ",llround(Fd[i]));
	puts("");*/
	double_negacyclic_FFT(Fd,logn,twid);
	for(size_t i=0;i<n/2;i++){
		double fr=Fd[i],fi=Fd[i+n/2];
		for(size_t j=i-(i%4);j<4+i-(i%4);j++){
			if(i==j) continue;
			double t=fr*fd[j]-fi*fd[j+n/2];
			fi=fr*fd[j+n/2]+fi*fd[j];
			fr=t;
		}
		Fd[i]=fr;
		Fd[i+n/2]=fi;
	}
//	double_negacyclic_iFFT(Fd,logn,twid);
/*	for(int i=0;i<n;i++)
		printf("%lld ",llround(Fd[i]));
	puts(": u norm f/f");*/
//	double_negacyclic_FFT(Fd,logn,twid);
	sizered(Fd,logn,fd,gd,Gd,twid,1);
/*	for(int i=0;i<n;i++)
		printf("%lld,",llround(Fd[i]));
	puts(" det 1 sol");*/
	for(size_t i=0;i<n;i++)
		Fd[i]*=q;
	double_negacyclic_FFT(Fd,logn,twid);
	sizered(Fd,logn,fd,gd,Gd,twid,q);
/*	for(int i=0;i<n;i++)
		printf("%lld,",llround(Fd[i]));
	puts("");*/
	double maxerror=0.;
	for(size_t i=0;i<n;i++){
		F[i]=llround(Fd[i]);
		G[i]=llround(Gd[i]);
		maxerror=fmax(maxerror,fabs(Gd[i]-G[i]));
//		printf("%lf ",Gd[i]);
	}
	NTRUGEN_LOG("error=%lg\n",maxerror);
	return maxerror>1e-3 ? -2 : 0;
}
