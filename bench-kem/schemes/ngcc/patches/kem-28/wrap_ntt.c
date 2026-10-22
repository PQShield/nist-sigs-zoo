/* PATCH (nist-sigs-zoo): the AVX2 assembly in asm/ reads the zetas table of ntt.c with PC-relative addressing,
 * which the linker refuses against a default-visibility symbol in a shared object (R_X86_64_PC32 ... recompile
 * with -fPIC). Give the symbols hidden visibility and compile the unmodified submitted file. */
#pragma GCC visibility push(hidden)
#include "ntt.c"
#pragma GCC visibility pop
