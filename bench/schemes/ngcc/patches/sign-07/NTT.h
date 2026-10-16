/* PATCH (nist-sigs-zoo): ntt.c includes "NTT.h", but the file is named ntt.h (case-insensitive file system upstream).
 * ntt.h also declares MultiplyNTT non-static while ntt.c defines it static (an error in every GCC); the declaration is
 * renamed away while ntt.h is read (nothing else calls MultiplyNTT). */
#define MultiplyNTT ngcc_unused_MultiplyNTT_decl
#include "ntt.h"
#undef MultiplyNTT
