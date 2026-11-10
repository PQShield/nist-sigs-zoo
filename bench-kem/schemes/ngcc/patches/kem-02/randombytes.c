/* PATCH (nist-sigs-zoo): the submitted backend/randombytes.c reads the operating
 * system RNG, so the ICCS adapter's deterministic DRNG never reaches the scheme and
 * the output cannot be replayed. Route CryptoRandomBytes() to drng_algorithm
 * like every other NGCC instance. */
#include <stddef.h>
#include <stdint.h>
#include "drng.h"

extern DRNG_ctx drng_algorithm;

void CryptoRandomBytes(uint8_t *out, size_t outlen) {
    get_random_number(&drng_algorithm, out, 8ULL * outlen);
}
