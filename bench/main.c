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
    NULL,
};

int main(void) {
    printf("%-24s  %16s  %16s  %16s\n",
           "scheme", "keygen (cycles)", "sign (cycles)", "verify (cycles)");
    printf("%-24s  %16s  %16s  %16s\n",
           "------", "---------------", "-------------", "---------------");

    for (int i = 0; SO_PATHS[i]; i++) {
        bench_scheme_t *s = bench_load(SO_PATHS[i]);
        if (!s) continue;
        bench_run(s);
        bench_unload(s);
    }
    return 0;
}
