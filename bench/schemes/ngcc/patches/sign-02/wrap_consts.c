/* PATCH (nist-sigs-zoo): the AVX2 assembly (intt.S, pointwise.S, ...) reads the tables in consts.c with
 * PC-relative addressing, which the linker refuses against a default-visibility symbol in a shared object
 * (R_X86_64_PC32 ... recompile with -fPIC). Give the tables hidden visibility and compile the unmodified
 * submitted file. */
#pragma GCC visibility push(hidden)
#include "consts.c"
#pragma GCC visibility pop
