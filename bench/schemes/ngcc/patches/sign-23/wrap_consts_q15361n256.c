/* PATCH (nist-sigs-zoo): hidden visibility so the AVX2 NTT assembly can reach the tables PC-relative in a shared object. */
#pragma GCC visibility push(hidden)
#include "q15361n256/ntt_consts.c"
#pragma GCC visibility pop
