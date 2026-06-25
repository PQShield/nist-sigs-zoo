# bench-kem/

Cycle-count + wall-time benchmark for **KEMs** (key encapsulation mechanisms).
Measures median keygen / encaps / decaps cycles and microseconds per parameter set,
and verifies each scheme's encaps/decaps round-trip agrees on the shared secret.

Sibling of `bench/` (the signature benchmark); same isolation model and shim/codegen
machinery, different op contract. Keep them separate — do not merge.

## Build and run

```bash
git submodule update --init --recursive bench-kem/schemes/
make              # builds ./bench-kem and all scheme .so files
./bench-kem       # run all registered schemes
./bench-kem mlkem # run only schemes whose name matches "mlkem" (case-insensitive)
./bench-kem ecdh  # multiple filters are OR-ed
make clean        # remove build artifacts
```

`run_bench.sh` wraps `./bench-kem`, prepends a system-info header, and saves
output to `results/<timestamp>_<cpu>.txt`.

Dependencies: C compiler (cc), make, git submodules. OpenSSL 3.x for ECDH
(detected via `brew --prefix openssl`, `pkg-config`, or `/usr` fallback).
ML-KEM builds with AVX2 (`-march=native`).

## Architecture

```
bench-kem/
├── scheme.h              # bench_scheme_info_t — name, pk/sk/ct/ss sizes, iters
├── loader.h / loader.c   # dlopen loader: resolves crypto_kem_keypair/enc/dec
├── harness.h             # cycle counter + timer + stats + bench_run() + correctness check
├── main.c                # includes build/so_paths.h + filter logic + large-stack thread
├── gen_shims.py          # shim generator: substitutes @COLNAME@ tokens from params.tsv
├── Makefile              # builds ./bench-kem; generates build/so_paths.h from ALL_SOS
├── run_bench.sh          # wrapper: collects sysinfo, tees output to results/
└── schemes/
    ├── classic/          # ECDH (X25519, P-256) via system OpenSSL; no submodule
    └── mlkem/            # pq-crystals/kyber (avx2); ref/ is a git submodule
```

`SO_PATHS[]` in `main.c` is **auto-generated** from `ALL_SOS` in `Makefile` into
`build/so_paths.h`. Never edit `main.c` to add schemes; only add to `Makefile`.

## Isolation model

Each parameter set compiles to a self-contained `.so`, loaded with `dlopen(RTLD_LOCAL)`.
Symbols in one `.so` are invisible to others — non-namespaced implementations
(e.g. bare `crypto_kem_keypair` from several upstreams at once) coexist without
conflict. This is the entire mechanism that lets badly-behaved, non-namespaced KEM
code be benchmarked side by side; the shim just re-exports the standard names.

## Contract every `.so` must satisfy

```c
/* Metadata — name, sizes, per-scheme iteration override. */
const bench_scheme_info_t *bench_info(void);

/* Standard NIST KEM API; all return 0 on success. */
int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
```

## Harness (`harness.h`)

- **Cycle counter**: `rdtsc` + `lfence` on x86/x86-64; `cntvct_el0` on aarch64.
- **Wall clock**: `clock_gettime(CLOCK_MONOTONIC)` bracketing each operation.
- **Stats**: median over `BENCH_ITER` (default 1000) iterations.
  Override at build time `make BENCH_ITER=200` or at run time `BENCH_ITER=200 ./bench-kem`.
- **Per-scheme override**: set `iters` in `bench_scheme_info_t` to a non-zero value.
- **Correctness**: every iteration compares the encaps shared secret against the
  decaps shared secret (outside the timed region). Mismatches are tallied and the
  `ok` column shows `FAIL`, but the scheme **still reports its timing numbers** — a
  broken KEM is informative, not fatal. (This is a deliberate divergence from
  `bench/`, which never checks the verify return value. It matters once schemes with
  a non-zero decryption-failure rate, e.g. HQC, are added.)
- Warm-up runs a full keygen → encaps → decaps before the timed loop.

## Shim pattern

The shim adapts upstream API conventions to the KEM contract. Notes:

- **Argument order**: the NIST KEM API is `enc(ct, ss, pk)` / `dec(ss, ct, sk)`.
  pq-crystals/kyber matches it directly (no ctx args, unlike dilithium's signature API).
- **Symbol namespacing**: kyber's avx2 symbols are `pqcrystals_kyber{512,768,1024}_avx2_*`,
  selected at compile time by `-DKYBER_K={2,3,4}`. Declare the namespaced symbols and
  wrap them with plain `crypto_kem_*` (token `@NS@`/`@K@` from `params.tsv`).
- **`randombytes`**: kyber's avx2 dir ships `randombytes.c`; include it, don't redefine.
- **Consistent `-march=native`**: compile all kyber sources (incl. the `keccak4x`
  SIMD object) with the same AVX2 flags as the shim, or platform-detection macros
  disagree and link fails.
- **ECDH as a KEM** (OpenSSL): recipient static keypair stored module-level (`g_key`);
  encaps generates an ephemeral keypair, `ct` = ephemeral public key, `ss` = ECDH
  shared secret; decaps imports the peer public key from `ct` and derives. NIST-curve
  public keys are imported via `EVP_PKEY_fromdata` (no throwaway keygen — keeps decaps
  timing clean). See `schemes/classic/classic_common.h`.

## Adding a new scheme

Same template system as `bench/` (**hand-written shim files are banned**; `gen_shims.py`
generates one `<file>_shim.c` per row of `params.tsv`, not committed — `.gitignore` has `*_shim.c`).

1. Add upstream as a git submodule under `schemes/<name>/ref/` (classic/system-library
   schemes skip this).
2. Create `schemes/<name>/`: `params.tsv` (columns `file name pk sk ct ss iters` + any
   `@TOKEN@` columns), `shim_template.c`, `.gitignore` with `*_shim.c`, and a `Makefile`
   (`all:` first; build upstream to objects/`.a` with `-fPIC`; compile+link each shim to
   `build/<file>.so`).
3. `Makefile`: add `<NAME>_SOS`, the `schemes/<name>/.stamp` prereq line, `$(<NAME>_SOS):
   schemes/<name>/.stamp`, and add `<NAME>_SOS` to `ALL_SOS` / `<name>` to `SCHEME_DIRS`.

## Status / TODO

- **HQC** not yet integrated — reference code is distributed as a zip on pqc-hqc.org
  (no clean git submodule); will use a `.source` non-submodule pattern.
- **Data wiring deferred**: there is no KEM equivalent of `bench/update_scheme_data.py`
  yet, and the `data/kems/*.yaml` schema + the `/kems/` page carry no performance fields.
  Feeding measured cycles into the site is a separate follow-up.

## Notes

- For stable numbers: disable turbo boost, pin to one core, performance governor.
- `rdtsc` measures reference cycles (constant TSC rate on modern Intel/AMD).
