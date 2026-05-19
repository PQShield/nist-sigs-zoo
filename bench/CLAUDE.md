# bench/

Cycle-count benchmarking framework for post-quantum signature schemes.
Measures median keygen / sign / verify cycles over 1000 iterations.

## Build

```bash
make        # builds ./bench binary
./bench     # run all registered schemes
make clean  # remove build artifacts
```

Requires: C compiler (cc), make, git submodules initialized.

```bash
git submodule update --init --recursive
```

## Architecture

```
bench/
├── scheme.h              # Pluggable scheme interface (bench_scheme_t)
├── harness.h             # Cycle counter + stats + bench_run()
├── main.c                # Driver: scheme registry, table output
├── Makefile              # Top-level build
└── schemes/
    └── mldsa/            # ML-DSA / Dilithium reference implementation
        ├── mldsa.h       # Declares mldsa_schemes[] extern
        ├── mldsa.c       # Adapters for ML-DSA-44 / 65 / 87
        ├── Makefile      # Builds libmldsa.a from dilithium submodule
        └── ref/          # git submodule: pq-crystals/dilithium
```

## Scheme interface (`scheme.h`)

Each scheme exposes one or more `bench_scheme_t` instances — one per
parameter set:

```c
typedef struct {
    const char *name;
    size_t      pk_bytes;
    size_t      sk_bytes;
    size_t      sig_bytes;   /* maximum */

    int (*keygen_fn)(uint8_t *pk, uint8_t *sk);
    int (*sign_fn)(uint8_t *sig, size_t *siglen,
                   const uint8_t *msg, size_t msglen,
                   const uint8_t *sk);
    int (*verify_fn)(const uint8_t *sig, size_t siglen,
                     const uint8_t *msg, size_t msglen,
                     const uint8_t *pk);
} bench_scheme_t;
```

All functions return 0 on success. `sign_fn` writes the actual
signature length to `*siglen`.

## Harness (`harness.h`)

- **Cycle counter**: `rdtsc` with `lfence` serialisation on x86/x86-64;
  `cntvct_el0` on aarch64.
- **Stats**: median over `BENCH_ITER` (default 1000) samples. Override
  at build time: `make BENCH_ITER=500`.
- **bench_run()**: allocates key/sig buffers, runs one warm-up iteration,
  collects `BENCH_ITER` samples for each of keygen/sign/verify, prints
  one line.

Scheme adapters include only `scheme.h`, not `harness.h`, to avoid
unused-function warnings.

## ML-DSA scheme (`schemes/mldsa/`)

Uses the pq-crystals/dilithium reference implementation (git submodule
at `schemes/mldsa/ref/`). The dilithium source uses a compile-time
`-DDILITHIUM_MODE=N` flag to select the parameter set; symbols are
namespaced (`pqcrystals_dilithium2_ref_keypair`, etc.) so all three
modes can coexist in one binary.

Build strategy: compile each mode's source files into a separate object
directory (`build/mode{2,3,5}/`) then pack everything — including
`mldsa.o` — into a single `build/libmldsa.a`.

The adapter (`mldsa.c`) forward-declares the dilithium API functions
directly instead of including `api.h`. This avoids the include-guard
problem that would arise from trying to include `api.h` three times
with different `DILITHIUM_MODE` values. Sizes are taken from FIPS 204.

Context parameter (`ctx`, `ctxlen`) is passed as `NULL, 0` — the FIPS
204 default for pure message signing.

## Adding a new scheme

1. Create `schemes/<name>/` with:
   - `<name>.h` — declares `extern bench_scheme_t <name>_schemes[];` and
     `extern const size_t n_<name>_schemes;`
   - `<name>.c` — implements wrappers, defines the scheme array
   - `Makefile` — builds `build/lib<name>.a`; `all:` target must be
     listed **first** to be the default goal

2. In `bench/Makefile`:
   - Add `<NAME>_DIR` and `<NAME>_LIB` variables
   - Add the lib as a prerequisite of `bench`
   - Add a rule to build it via `$(MAKE) -C`

3. In `bench/main.c`:
   - `#include "schemes/<name>/<name>.h"`
   - Add `{ <name>_schemes, n_<name>_schemes }` to `scheme_groups[]`

## Notes

- Benchmarks run on the calling CPU with whatever governor / frequency
  scaling is active. For stable numbers: disable turbo boost, pin to one
  core, use a performance governor.
- `rdtsc` counts reference cycles (constant-rate TSC on modern Intel/AMD),
  not execution cycles. Results are still comparable across runs on the
  same machine; multiply by `TSC_freq / CPU_freq` for true cycles if needed.
- The 32-byte all-zero message is intentional: message content does not
  affect cycle counts for lattice-based schemes.
