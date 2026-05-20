# bench/

Cycle-count benchmarking framework for post-quantum signature schemes.
Measures median keygen / sign / verify cycles per parameter set.

## Build

```bash
git submodule update --init --recursive bench/schemes/
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
    ├── <name>/
    │   ├── <param>_shim.c   # one shim per parameter set
    │   ├── Makefile          # builds build/<param>.so
    │   └── ref/              # git submodule: upstream implementation
    └── ...
```

Current schemes: `mldsa/` (pq-crystals/dilithium, AVX2),
`slhdsa/` (pq-code-package/slhdsa-c), `fndsa/` (pornin/c-fn-dsa).

## Isolation model

Each parameter set is compiled into a self-contained `.so` and loaded at
runtime with `dlopen(RTLD_LOCAL)`.  Symbols in one `.so` are invisible to
others — non-namespaced implementations (e.g. bare `crypto_sign_keypair`)
can coexist without conflict.

## Contract every `.so` must satisfy

```c
/* Metadata — name, sizes, per-scheme iteration override. */
const bench_scheme_info_t *bench_info(void);

/* Standard names; all return 0 on success. */
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk);
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk);
```

`bench_scheme_info_t` is defined in `scheme.h`; shim files include only
that header plus the upstream implementation's own header.

## Harness (`harness.h`)

- **Cycle counter**: `rdtsc` + `lfence` on x86/x86-64; `cntvct_el0` on
  aarch64.
- **Stats**: median over `BENCH_ITER` (default 1000) iterations.
  Override at build time: `make BENCH_ITER=200`.
- **Per-scheme override**: set `iters` field in `bench_scheme_info_t` to
  a non-zero value (e.g. slow hash-based `-s` variants use 5).
- **bench_run()**: one warm-up, then `n` timed iterations of keygen /
  sign / verify; prints one line and flushes stdout.

## Shim pattern

Each upstream implementation has its own API conventions.  The shim's job
is to absorb those differences and re-export the standard contract above.
Common adaptations needed:

- **Argument order**: some APIs put sk before pk, or pass ctx/addrnd
  parameters absent from the standard contract (pass `NULL, 0`).
- **Return value**: some return 1 on success or a `size_t` byte count
  instead of 0-on-success `int`; convert at the shim boundary.
- **Key/sig sizes**: hardcode in `bench_scheme_info_t` using the
  upstream's size macros or constants where available.
- **Mode selection**: compile-time `-D` flags (e.g. Dilithium) or a
  runtime parameter struct (e.g. slhdsa-c's `slh_param_t *`) — the
  shim selects the correct one for its parameter set.

The upstream library is compiled once as a static `.a` (with `-fPIC`)
and linked into each per-parameter-set `.so`.

## Adding a new scheme

1. Add upstream as a git submodule under `schemes/<name>/ref/`.

2. Create `schemes/<name>/`:
   - `<param>_shim.c` per parameter set — implement `bench_info()` +
     `crypto_sign_*` wrappers, adapting the upstream API as needed.
   - `Makefile` — compiles upstream sources to `build/lib<name>.a`,
     compiles each shim, links each `.so`; `all:` rule first, guard on
     `ref/` being populated.

3. `bench/Makefile`: add `<NAME>_SOS` variable, a rule invoking
   `$(MAKE) -C schemes/<name>`, add `.so` paths to `bench` prerequisites,
   add `$(MAKE) -C schemes/<name> clean` to the `clean` target.

4. `bench/main.c`: append `.so` paths to `SO_PATHS[]`.

## Notes

- For stable numbers: disable turbo boost, pin to one core, performance
  governor.  The `bench` binary does not pin itself.
- `rdtsc` measures reference cycles (constant TSC rate on modern
  Intel/AMD), not execution cycles.  Numbers are comparable across runs
  on the same machine.
- Message is a 32-byte zero buffer.  Content does not affect cycle counts
  for lattice or hash-based schemes.
