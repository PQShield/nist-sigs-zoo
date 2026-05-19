#include <stdio.h>
#include "harness.h"
#include "schemes/mldsa/mldsa.h"

/*
 * To add a new scheme:
 *   1. Create schemes/<name>/<name>.h  (declares bench_scheme_t[] + count)
 *   2. Add an #include here
 *   3. Add an entry in the scheme_groups table below
 */

typedef struct {
    bench_scheme_t *schemes;
    size_t          count;
} scheme_group_t;

static const scheme_group_t scheme_groups[] = {
    { mldsa_schemes, 3 /* n_mldsa_schemes */ },
};

int main(void) {
    printf("%-20s  %-16s  %-16s  %-16s\n",
           "scheme", "keygen (cycles)", "sign (cycles)", "verify (cycles)");
    printf("%-20s  %-16s  %-16s  %-16s\n",
           "------", "---------------", "-------------", "---------------");

    for (size_t g = 0; g < sizeof(scheme_groups) / sizeof(scheme_groups[0]); g++) {
        const scheme_group_t *grp = &scheme_groups[g];
        for (size_t i = 0; i < grp->count; i++) {
            bench_run(&grp->schemes[i]);
        }
    }

    return 0;
}
