/* PATCH (nist-sigs-zoo, fix): compiles the submitted vector_com.c with patches/sign-13/prgs.h (aligned key buffer in the
 * SECURITY_PARAM == 512 PRG; see that file). The patched copy is reached through
 * the unmodified copy patches/sign-13/vole_params.h, included first; the submitted file is included unchanged. */
#include "vole_params.h"
#include "vector_com.c"
