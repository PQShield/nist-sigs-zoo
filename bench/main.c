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
    /* SLH-DSA entries will be added once bench/schemes/slhdsa/ref/ is
     * populated (git submodule update --init bench/schemes/slhdsa/ref). */
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
