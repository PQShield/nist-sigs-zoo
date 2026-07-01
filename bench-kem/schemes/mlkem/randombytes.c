/* randombytes() for mlkem-native, backed by getrandom(2).
 * mlkem-native declares this in mlkem/src/randombytes.h but ships only a
 * test RNG; benchmarking needs a real one. RTLD_LOCAL keeps this bare symbol
 * isolated to each .so. */
#include <stddef.h>
#include <stdint.h>
#include <sys/random.h>

int randombytes(uint8_t *out, size_t outlen) {
    size_t off = 0;
    while (off < outlen) {
        ssize_t r = getrandom(out + off, outlen - off, 0);
        if (r < 0) return -1;
        off += (size_t)r;
    }
    return 0;
}
