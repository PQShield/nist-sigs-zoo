/* PATCH (nist-sigs-zoo): compiles the submitted vole_commit.c with patches/sign-13/prgs.h (aligned key buffer in the
 * SECURITY_PARAM == 512 PRG; see that file and docs/ngcc-submission-bugs.md U10). The patched copy is reached through
 * the unmodified copy patches/sign-13/vole_params.h, included first; the submitted file is included unchanged. */
#include "vole_params.h"
#include "vole_commit.c"
