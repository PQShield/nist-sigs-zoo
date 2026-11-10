/* PATCH (nist-sigs-zoo): unmodified copy of the submitted sysrng.h. The submission keeps it one directory above the
 * yuanyang-<n> instance directories, but the build copies only the instance directory. */
#ifndef YUANYANG_SYSRNG_H
#define YUANYANG_SYSRNG_H

#include <stddef.h>

/* Fill dst with operating-system randomness. Return 0 on success. */
int yuanyang_sysrng(void *dst, size_t len);

#endif
