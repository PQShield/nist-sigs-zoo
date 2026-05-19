# bench/

Cycle-count benchmarking framework for post-quantum signature schemes.
Measures median keygen / sign / verify cycles per parameter set.

## Build

```bash
git submodule update --init bench/schemes/mldsa/ref
make        # builds ./bench and all scheme .so files
./bench     # run all registered schemes
make clean  # remove build artifacts
```

Requires: C compiler (cc), make, git submodules initialized.

## Architecture

```
bench/
├── scheme.h              # bench_scheme_info_t — metadata type used by shims
├── loader.h / loader.c   # dlopen loader: bench_scheme_t, bench_load/unload
├── harness.h             # Cycle counter + stats + bench_run()
├── main.c                # SO_PATHS[] list + main loop
├── Makefile
└── schemes/
    ├── mldsa/
    │   ├── mldsa{44,65,87}_shim.c  # one shim per parameter set
    │   ├── Makefile                 # builds mldsa{44,65,87}.so
    │   └── ref/                     # git submodule: pq-crystals/dilithium
    └── slhdsa/
        ├── Makefile                 # scaffold; guards on ref/ being populated
        └── ref/                     # git submodule: pq-code-package/slhdsa-c
                                     # (init when network available)
```

## Isolation model

Each parameter set is compiled into a self-contained `.so` and loaded at
runtime with `dlopen(RTLD_LOCAL)`.  Symbols in one `.so` are invisible to
others — non-namespaced implementations (e.g. bare `crypto_sign_keypair`)
can coexist without conflict.

## Contract every `.so` must satisfy

```c
/* Metadata — name, sizes, per-scheme iteration override. */
const bench_scheme_info_t *bench_info(void);

/* Standard NIST API names. */
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk);
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk);
```

All return 0 on success.  `bench_scheme_info_t` is defined in `scheme.h`;
shim files include only that header.

## Harness (`harness.h`)

- **Cycle counter**: `rdtsc` + `lfence` on x86/x86-64; `cntvct_el0` on
  aarch64.
- **Stats**: median over `BENCH_ITER` (default 1000) iterations.
  Override at build time: `make BENCH_ITER=200`.
- **Per-scheme override**: set `iters` field in `bench_scheme_info_t` to
  a non-zero value. Used by slow schemes (e.g. SLH-DSA -s variants).
- **bench_run()**: one warm-up, then `n` timed iterations of keygen /
  sign / verify; prints one line and flushes stdout.

## ML-DSA (`schemes/mldsa/`)

Source: `pq-crystals/dilithium` reference implementation (submodule).

Each shim (`mldsa44/65/87_shim.c`) forward-declares dilithium's
namespaced functions (`pqcrystals_dilithiumN_ref_*`) and re-exports them
under the standard `crypto_sign_*` names.  Dilithium sources are compiled
three times with `-DDILITHIUM_MODE=2/3/5` into separate object directories
(`build/mode{2,3,5}/`), all with `-fPIC`.  Each `.so` is fully
self-contained.

Context parameter (`ctx`/`ctxlen`) passed as `NULL, 0` — FIPS 204 default
for pure message signing.

## SLH-DSA (`schemes/slhdsa/`)

Source: `pq-code-package/slhdsa-c` (submodule URL registered; not yet
cloned — network was unavailable at setup time).

```bash
git submodule update --init bench/schemes/slhdsa/ref
```

Once populated: inspect the API, write one shim per parameter set
following the ML-DSA shims as a template, update `schemes/slhdsa/Makefile`
to build the `.so` files, and add paths to `SO_PATHS[]` in `main.c`.

## Adding a new scheme

1. Create `schemes/<name>/`:
   - `<param>_shim.c` per parameter set — implement `bench_info()` +
     `crypto_sign_*` wrappers around the underlying implementation.
   - `Makefile` — builds `build/<param>.so` with `-fPIC`; `all:` must be
     listed first.

2. `bench/Makefile`: add `<NAME>_SOS` variable, a rule invoking
   `$(MAKE) -C schemes/<name>`, and add the `.so` paths to the `bench`
   prerequisite list.

3. `bench/main.c`: append `.so` paths to `SO_PATHS[]`.

## Notes

- For stable numbers: disable turbo boost, pin to one core, performance
  governor.  The `bench` binary does not pin itself.
- `rdtsc` measures reference cycles (constant TSC rate on modern
  Intel/AMD), not execution cycles.  Numbers are comparable across runs
  on the same machine.
- Message is a 32-byte zero buffer.  Content does not affect cycle counts
  for lattice or hash-based schemes.
