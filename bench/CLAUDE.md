# bench/

Cycle-count + wall-time benchmarking framework for signature schemes.
Measures median keygen / sign / verify cycles and microseconds per parameter set.

## Build and run

```bash
git submodule update --init --recursive bench/schemes/
make              # builds ./bench and all scheme .so files
./bench           # run all registered schemes
./bench mldsa     # run only schemes whose name matches "mldsa" (case-insensitive)
./bench rsa ecdsa # multiple filters are OR-ed
make clean        # remove build artifacts
```

`run_bench.sh` wraps `./bench`, prepends a system-info header, and saves
output to `results/<timestamp>_<cpu>.txt`:

```bash
./run_bench.sh              # all schemes
./run_bench.sh classic      # filtered; slug appended to filename
```

Requires: C compiler (cc), make, git submodules initialized,
OpenSSL 3.x (for classic schemes; detected via `brew --prefix openssl`,
`pkg-config`, or `/usr` fallback).

## Architecture

```
bench/
├── scheme.h              # bench_scheme_info_t — metadata type used by shims
├── loader.h / loader.c   # dlopen loader: bench_scheme_t, bench_load/unload
├── harness.h             # cycle counter + wall-clock timer + stats + bench_run()
├── main.c                # SO_PATHS[] list + filter logic + main loop
├── Makefile
├── run_bench.sh          # wrapper: collects sysinfo, tees output to results/
├── results/              # saved benchmark runs (committed)
└── schemes/
    ├── classic/          # RSA/ECDSA/EdDSA via system OpenSSL; no submodule
    ├── mldsa/            # pq-crystals/dilithium (AVX2)
    ├── slhdsa/           # pq-code-package/slhdsa-c
    ├── fndsa/            # pornin/c-fn-dsa
    └── <name>/
        ├── <param>_shim.c   # one shim per parameter set
        ├── Makefile          # builds build/<param>.so
        └── ref/              # git submodule: upstream implementation
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

- **Cycle counter**: `rdtsc` + `lfence` on x86/x86-64; `cntvct_el0` on aarch64.
- **Wall clock**: `clock_gettime(CLOCK_MONOTONIC)` bracketing each operation.
- **Stats**: median over `BENCH_ITER` (default 1000) iterations.
  Override at build time: `make BENCH_ITER=200`.
- **Per-scheme override**: set `iters` field in `bench_scheme_info_t` to
  a non-zero value (e.g. SLH-DSA `-s` variants use 5, RSA-4096 uses 20).
- **bench_run()**: one warm-up, then `n` timed iterations; prints one line
  with six columns (keygen/sign/verify × cycles/µs) and flushes stdout.

## Shim pattern

Each upstream implementation has its own API conventions.  The shim's job
is to absorb those differences and re-export the standard contract above.
Common adaptations needed:

- **Argument order**: some APIs put sk before pk, or carry ctx/addrnd
  parameters absent from the standard contract (pass `NULL, 0`).
- **Return value**: some return 1 on success or a `size_t` byte count
  instead of 0-on-success `int`; convert at the shim boundary.
- **Key/sig sizes**: hardcode in `bench_scheme_info_t` using the
  upstream's size macros or constants where available.
- **Mode selection**: compile-time `-D` flags (e.g. Dilithium) or a
  runtime parameter struct (e.g. slhdsa-c's `slh_param_t *`) — the
  shim selects the correct one for its parameter set.
- **OpenSSL / stateful keys**: classic shims store a module-level
  `EVP_PKEY *g_key`; `crypto_sign_keypair` generates and stores it,
  sign/verify use it directly.  The pk/sk harness buffers are unused
  (sizes in INFO are set to real DER values for documentation only).

The upstream library is compiled once as a static `.a` (with `-fPIC`)
and linked into each per-parameter-set `.so`.

## Adding a new scheme

**All schemes use the shim template system. Hand-written shim files are banned.**
`gen_shims.py` at `bench/` level generates one `<file>_shim.c` per row of `params.tsv`.
Generated shim files are not committed — each scheme dir has a `.gitignore` with `*_shim.c`.

1. Add upstream as a git submodule under `schemes/<name>/ref/`.
   (Classic/system-library schemes skip this step.)

2. Create `schemes/<name>/`:
   - `params.tsv` — TSV with columns: `file  name  pk  sk  sig  iters` plus any
     scheme-specific columns referenced by `@TOKEN@` in the template.
   - `shim_template.c` — C template; `@COLNAME@` tokens are substituted per row.
   - `.gitignore` containing `*_shim.c`.
   - `Makefile` — **`all:` must be the first explicit target** (so `$(MAKE) -C`
     without a goal builds everything). Add shim generation rule
     `$(SHIM_FILES): shim_template.c params.tsv` → `python3 ../../gen_shims.py …`.
     Compile upstream sources to `build/lib<name>.a` with `-fPIC`; compile+link
     each shim into `build/<file>.so`.

3. `bench/Makefile`:
   - Add `<NAME>_SOS` listing each `.so` path.
   - Add a stamp-file rule: `schemes/<name>/.stamp: … → $(MAKE) -C … && touch $@`.
     (Stamp files prevent parallel `-j` from racing multiple sub-make instances.)
   - Add `$(NAME_SOS): schemes/<name>/.stamp` order dependency.
   - Add `.so` paths to `bench` prerequisites.
   - Add `.stamp` removal and `$(MAKE) -C … clean` to `clean:`.

4. `bench/main.c`: append `.so` paths to `SO_PATHS[]`.

## Filtering

`./bench [filter...]` runs only schemes whose names match at least one
filter.  Matching is case-insensitive and ignores non-alphanumeric
characters, so `mldsa`, `ml-dsa`, and `ML_DSA` all match `ML-DSA-44`.
`run_bench.sh` passes filters through and appends them to the output filename.

## Notes

- For stable numbers: disable turbo boost, pin to one core, performance
  governor.  The `bench` binary does not pin itself.
- `rdtsc` measures reference cycles (constant TSC rate on modern
  Intel/AMD), not execution cycles.  Numbers are comparable across runs
  on the same machine.
- Message is a 32-byte zero buffer.  Content does not affect cycle counts
  for lattice or hash-based schemes.
