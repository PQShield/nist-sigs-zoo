#include <stdio.h>
#include <string.h>
#include "harness.h"
#include "loader.h"

/*
 * List of scheme .so paths to benchmark, relative to the bench/ directory.
 * To add a new scheme: build its .so and add the path here.
 */
static const char *SO_PATHS[] = {
    "schemes/mldsa/build/mldsa44.so",
    "schemes/mldsa/build/mldsa65.so",
    "schemes/mldsa/build/mldsa87.so",
    "schemes/slhdsa/build/slhdsa_sha2_128s.so",
    "schemes/slhdsa/build/slhdsa_shake_128s.so",
    "schemes/slhdsa/build/slhdsa_sha2_128f.so",
    "schemes/slhdsa/build/slhdsa_shake_128f.so",
    "schemes/slhdsa/build/slhdsa_sha2_192s.so",
    "schemes/slhdsa/build/slhdsa_shake_192s.so",
    "schemes/slhdsa/build/slhdsa_sha2_192f.so",
    "schemes/slhdsa/build/slhdsa_shake_192f.so",
    "schemes/slhdsa/build/slhdsa_sha2_256s.so",
    "schemes/slhdsa/build/slhdsa_shake_256s.so",
    "schemes/slhdsa/build/slhdsa_sha2_256f.so",
    "schemes/slhdsa/build/slhdsa_shake_256f.so",
    "schemes/fndsa/build/fndsa512.so",
    "schemes/fndsa/build/fndsa1024.so",
    "schemes/classic/build/rsa2048.so",
    "schemes/classic/build/rsa3072.so",
    "schemes/classic/build/rsa4096.so",
    "schemes/classic/build/ecdsa_p256.so",
    "schemes/classic/build/ecdsa_p384.so",
    "schemes/classic/build/ecdsa_p521.so",
    "schemes/classic/build/ed25519.so",
    "schemes/classic/build/ed448.so",
    NULL,
};

/* Lowercase-and-strip-non-alnum copy of src into dst (max dst_sz bytes). */
static void normalize(char *dst, size_t dst_sz, const char *src) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 1 < dst_sz; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            dst[j++] = c;
    }
    dst[j] = '\0';
}

/*
 * Return 1 if scheme name matches any of the filter strings, or if no filters
 * were given.  Matching is case-insensitive and ignores non-alphanumeric chars
 * in both the name and the filter (so "mldsa", "ml-dsa", "ML_DSA" all match
 * "ML-DSA-44").
 */
static int scheme_matches(const char *name, int nfilters, char **filters) {
    if (nfilters == 0) return 1;
    char norm_name[128];
    normalize(norm_name, sizeof(norm_name), name);
    for (int f = 0; f < nfilters; f++) {
        char norm_pat[128];
        normalize(norm_pat, sizeof(norm_pat), filters[f]);
        if (strstr(norm_name, norm_pat)) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    int nfilters = argc - 1;
    char **filters = argv + 1;

    bench_print_header();

    for (int i = 0; SO_PATHS[i]; i++) {
        bench_scheme_t *s = bench_load(SO_PATHS[i]);
        if (!s) continue;
        if (scheme_matches(s->name, nfilters, filters))
            bench_run(s);
        bench_unload(s);
    }
    return 0;
}
