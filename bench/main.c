#include <stdio.h>
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

int main(void) {
    bench_print_header();

    for (int i = 0; SO_PATHS[i]; i++) {
        bench_scheme_t *s = bench_load(SO_PATHS[i]);
        if (!s) continue;
        bench_run(s);
        bench_unload(s);
    }
    return 0;
}
