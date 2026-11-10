/* PATCH (nist-sigs-zoo): Aigis-Sig+: UpdateSM3() (the SM3 block function) lives in ippcpsm3/pcphashsm3px.h,
 * which the submission never #includes from a .c file, so the shared object has an
 * undefined UpdateSM3 and cannot be loaded. This wrapper compiles it once, with the
 * SM3 feature switch that pcpvariant.inc would otherwise provide. Our own file;
 * the extracted sources are untouched. */
#define _ENABLE_ALG_SM3_ 1
#include "pcphashsm3px.h"
