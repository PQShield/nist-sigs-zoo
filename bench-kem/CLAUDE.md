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
    ├── mlkem/            # pq-code-package/mlkem-native (avx2); ref/ is a git submodule
    ├── hqc/              # gitlab.com/pqc-hqc/hqc (x86_64/avx256); ref/ is a git submodule
    ├── frodo/            # Microsoft/PQCrypto-LWEKE (FAST AVX2); ref/ is a git submodule
    └── mceliece/         # lib.mceliece.org (libmceliece, tarball); build_libs.sh fetches+builds
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
  mlkem-native matches it directly (no ctx args).
- **ML-KEM build** (pq-code-package/mlkem-native): monolithic — compile `mlkem_native.c`
  + `mlkem_native_asm.S` once per level with `-DMLK_CONFIG_PARAMETER_SET={512,768,1024}`.
  The native AVX2 backends are off by default; enable with
  `-DMLK_CONFIG_USE_NATIVE_BACKEND_ARITH -DMLK_CONFIG_USE_NATIVE_BACKEND_FIPS202` and AVX2
  `-m` flags. Set `-DMLK_CONFIG_NAMESPACE_PREFIX=mlkem` so symbols are `mlkem_keypair/enc/dec`
  (no level suffix — level is fixed per `.so`); the shim wraps those as `crypto_kem_*`.
- **`randombytes`**: mlkem-native declares `randombytes()` but ships only a test RNG, so the
  scheme dir provides `randombytes.c` (getrandom-backed) compiled into each `.so`.
- **ECDH as a KEM** (OpenSSL): recipient static keypair stored module-level (`g_key`);
  encaps generates an ephemeral keypair, `ct` = ephemeral public key, `ss` = ECDH
  shared secret; decaps imports the peer public key from `ct` and derives. NIST-curve
  public keys are imported via `EVP_PKEY_fromdata` (no throwaway keygen — keeps decaps
  timing clean). See `schemes/classic/classic_common.h`.
- **FrodoKEM** (Microsoft/PQCrypto-LWEKE) renames its API per parameter set via macros
  in `frodo<level>.c` (`crypto_kem_keypair_Frodo640` …), so the shim re-exports the bare
  names (`@SUFFIX@` token = `Frodo640`/`976`/`1344`). Each `frodo<level>.c` `#include`s
  `kem.c`/`noise.c`/`frodo_macrify.c` — compile **only** the `frodo<level>.c` files
  (plus `util.c` and the generic `common/` sources), never the `#include`d `.c`s, or you
  get duplicate symbols. The standard (ISO, salted) variant is built, `_FAST_` AVX2, no
  OpenSSL (upstream's own `aes_ni.c`). Matrix-A generation is benchmarked **both ways**
  (`_AES128_FOR_A_` / `_SHAKE128_FOR_A_`) → six `.so`s; sizes are identical per level, only
  timings differ. `config.h` demands an A-gen define even in files that never use one
  (`util.c`), so the shared generic objects carry a placeholder `-D_SHAKE128_FOR_A_`.
  Upstream's `random.c` (`/dev/urandom`) self-seeds — no constructor needed.
- **Classic McEliece** (lib.mceliece.org / libmceliece) is **not** a git submodule —
  it's a signed tarball with its own `./configure && make` and djb deps (librandombytes,
  libcpucycles). `schemes/mceliece/build_libs.sh` fetches all three tarballs (pinned
  version + SHA-256), builds them with `--no-valgrind` (the default build hard-includes
  `<valgrind/memcheck.h>` for its constant-time tests), and harvests `libmceliece.a` +
  `librandombytes-kernel.a` into `build/local/`. Each shim statically links those archives
  → a self-contained per-set `.so` with no runtime library deps, matching the isolation
  model of the other schemes. libmceliece exports IFUNC-dispatched, namespaced
  `mceliece_kem_<set>_{keypair,enc,dec}` (`@SET@` = `348864`/`460896`/`6688128`/`6960119`/
  `8192128`); enc/dec already match the NIST arg order, keypair returns void → shim returns 0.
  Provenance is recorded via `schemes/mceliece/.source` (the `.source` mechanism, not a
  submodule). Keygen is variable-time and slow (tens to ~135 ms), so `iters` is low (30).
- **HQC** (gitlab.com/pqc-hqc/hqc) exports the *bare* NIST API (`crypto_kem_*`) from
  its own objects, so the shim does **not** wrap them — the loader resolves HQC's
  directly, and `RTLD_LOCAL` isolates the three variants' identical symbols. The shim
  only adds `bench_info()` and a `__attribute__((constructor))` that seeds HQC's
  SHAKE PRNG (`prng_init`) — the upstream expects the NIST KAT harness to do this.
  Variant (HQC-1/3/5) is selected by include path; the x86_64/avx256 implementation
  is built (needs `-mavx2 -mpclmul`), with `gf2x.c`/`reed_solomon.c` per-variant.

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

## Feeding results into the site

`update_scheme_data.py` (KEM analogue of `bench/update_scheme_data.py`) parses a
`results/*.txt` run and writes `keygen/encaps/decaps` cycles + µs into the matching
`data/kems/*.yaml` parameter sets (flat schema, mapped via `BENCH_TO_YAML`), and writes
`data/kem_benchmark_env.yaml`. Run after a benchmark:

```bash
./run_bench.sh                       # writes results/<ts>_<cpu>.txt
uv run update_scheme_data.py         # uses the most recent results file
```

The `/kems/` page renders the timings (table columns) and the environment panel.

## Notes

- For stable numbers: disable turbo boost, pin to one core, performance governor.
- `rdtsc` measures reference cycles (constant TSC rate on modern Intel/AMD).
