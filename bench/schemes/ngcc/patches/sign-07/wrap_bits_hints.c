/* PATCH (nist-sigs-zoo): GCC 14+ rejects implicit int (old K&R-style declarations) in bits-hints.c.
 * Downgrade that one diagnostic to a warning and compile the unmodified submitted file. */
#pragma GCC diagnostic warning "-Wimplicit-int"
#include "bits-hints.c"
