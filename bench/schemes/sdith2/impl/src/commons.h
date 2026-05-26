#ifndef COMMONS_H
#define COMMONS_H

#ifdef __cplusplus
#define EXPORT extern "C"
#define EXPORT_DECL extern "C"
#include <cstdint>
#else
#define EXPORT
#define EXPORT_DECL extern
#include "stdint.h"
#endif

#ifdef __x86_64__
#define CPU_SUPPORTS(feature) __builtin_cpu_supports(feature)
#endif

typedef void bitvec_t;   // vector of bits row major (dimensions provided at runtime)
typedef void bitmat_t;   // matrix of bits row major (dimensions provided at runtime)
typedef void flambda_t;  // at runtime, the size of a big fielf elem  is lam (i.e. lambda/8)
typedef void fpoly_t;    // at runtime, the size of a degree d is (d+1).lam
typedef void seed_t;     // at runtime, the size of a commit is 1.lam
typedef void salt_t;     // at runtime, the size of a commit is 1.lam
typedef void hash_t;     // at runtime, the size of a hash is 2.lam
typedef void commit_t;   // at runtime, the size of a commit is 2.lam
typedef void hash_ctx_t;

// type large enough to hold one field element
typedef uint64_t flambda_max_t[4] __attribute((aligned(32)));  // usable as data type for temporary variables

#endif  // COMMONS_H
