/* PATCH (nist-sigs-zoo): the AVX2 assembly in asm/ reads the tables of ntt_asm_consts.c with PC-relative
 * addressing, which the linker refuses against default-visibility symbols in a shared object
 * (R_X86_64_PC32 ... recompile with -fPIC). Give the tables hidden visibility and compile the unmodified
 * submitted file. */
#pragma GCC visibility push(hidden)
#include "ntt_asm_consts.c"
#pragma GCC visibility pop
