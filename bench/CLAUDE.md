# bench/

Cycle-count + wall-time benchmark for signature schemes.
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

Dependencies: C compiler (cc), make, git submodules initialized.
- OpenSSL 3.x for classic schemes (detected via `brew --prefix openssl`, `pkg-config`, or `/usr` fallback).
- CMake ≥ 3.13 + libgmp-dev for SQIsign (`sudo apt-get install cmake libgmp-dev`).

## Architecture

```
bench/
├── scheme.h              # bench_scheme_info_t — metadata type used by shims
├── loader.h / loader.c   # dlopen loader: bench_scheme_t, bench_load/unload
├── harness.h             # cycle counter + wall-clock timer + stats + bench_run()
├── main.c                # includes build/so_paths.h + filter logic + main loop
├── gen_shims.py          # shim generator: substitutes @COLNAME@ tokens from params.tsv
├── Makefile              # builds ./bench; generates build/so_paths.h from ALL_SOS
├── run_bench.sh          # wrapper: collects sysinfo, tees output to results/
├── results/              # saved benchmark runs (committed)
└── schemes/
    ├── classic/          # RSA/ECDSA/EdDSA via system OpenSSL; no submodule
    ├── mldsa/            # pq-crystals/dilithium
    ├── slhdsa/           # pq-code-package/slhdsa-c
    ├── fndsa/            # pornin/c-fn-dsa
    ├── hawk/             # hawk-sign/dev
    ├── mayo/             # PQCMayo/MAYO-C
    ├── faest/            # faest-sign/faest-ref
    ├── sdith/            # sdith/sdith
    ├── snova/            # PQCLAB-SNOVA/SNOVA
    ├── uov/              # pqov/pqov
    ├── qruov/            # qruov/round2
    ├── mqom/             # mqom/mqom-v2
    ├── sqisign/          # SQISign/the-sqisign (CMake build)
    └── <name>/
        ├── params.tsv       # one row per parameter set; columns substituted into template
        ├── shim_template.c  # C template with @COLNAME@ tokens
        ├── Makefile         # builds build/<param>.so; invokes gen_shims.py
        └── ref/             # git submodule: upstream implementation
```

`SO_PATHS[]` in `main.c` is **auto-generated** from `ALL_SOS` in `bench/Makefile` into
`build/so_paths.h`. Never edit `main.c` to add schemes; only add to `bench/Makefile`.

## Isolation model

Each parameter set compiles to a self-contained `.so`, loaded with `dlopen(RTLD_LOCAL)`.
Symbols in one `.so` are invisible to others — non-namespaced implementations
(e.g. bare `crypto_sign_keypair`) coexist without conflict.

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

## Harness (`harness.h`)

- **Cycle counter**: `rdtsc` + `lfence` on x86/x86-64; `cntvct_el0` on aarch64.
- **Wall clock**: `clock_gettime(CLOCK_MONOTONIC)` bracketing each operation.
- **Stats**: median over `BENCH_ITER` (default 1000) iterations.
  Override at build time: `make BENCH_ITER=200`.
- **Per-scheme override**: set `iters` field in `bench_scheme_info_t` to a non-zero value.
- **bench_run()**: one warm-up, then `n` timed iterations; prints one line with six columns.

## Shim pattern

The shim adapts upstream API conventions to the bench contract. Common issues:

- **Argument order**: some APIs put sk before pk, or carry ctx/addrnd parameters (pass `NULL, 0`).
- **Return value**: some return 1 on success or a `size_t` byte count; convert at the boundary.
- **`unsigned long long` vs `size_t`**: raw NIST APIs use `unsigned long long` for lengths.
  Cast at the shim boundary; never include the upstream's `crypto_sign.c` if it defines
  `crypto_sign_signature`/`crypto_sign_verify` with mismatched types — call lower-level
  functions directly instead (e.g. MQOM's `Sign`/`Verify_default`).
- **Old NIST combined API** (`crypto_sign`/`crypto_sign_open` where sm = sig ‖ m): wrap as:
  ```c
  // sign: call crypto_sign, copy first CRYPTO_BYTES from sm → sig
  // verify: reconstruct sm = sig ‖ m, call crypto_sign_open
  ```
  Use `malloc` for the temporary sm buffer (size = CRYPTO_BYTES + msglen).
- **Symbol namespacing**: some upstreams (e.g. SQIsign) `#define` all public names to
  namespaced forms via a namespace header. The compiled `.a` will have e.g.
  `sqisign_lvl1_ref_crypto_sign_keypair` rather than `crypto_sign_keypair`.
  Check with `nm lib.a | grep crypto_sign`.  Fix: declare the namespaced symbols as
  `extern` in the shim (using a `@NS_PREFIX@` token from params.tsv) and wrap them
  with plain-name functions.
- **`randombytes`**: some upstreams provide it (e.g. SQIsign common lib, SDitH);
  don't redefine it in the shim or you'll get a duplicate-symbol error.
  Others expect the shim to provide it (e.g. MQOM); implement with `getrandom`.
- **Mode selection**: compile-time `-D` flags (e.g. MQOM's `MQOM2_PARAM_*`, Dilithium's
  `DILITHIUM_MODE`) or runtime parameter structs (e.g. slhdsa-c's `slh_param_t *`).
- **`.DEFAULT_GOAL := all`**: required at the top of any sub-Makefile that uses
  `$(eval $(call RULE,...))` before the `all:` target, otherwise the first
  eval'd rule becomes make's default goal.
- **Platform flags in static libs**: if a static lib is compiled without `-march=native`
  but the shim is compiled with it, platform-detection macros (`__AES__`, `__AVX2__`)
  will disagree and you'll get undefined-symbol errors at link time.  Compile all
  sources with consistent flags, or include platform-dependent sources directly in
  the per-variant `.so` (MQOM approach: compiles everything including rijndael with
  per-variant `-march=native`).
- **OpenSSL / stateful keys**: classic shims store `EVP_PKEY *g_key` at module level;
  `crypto_sign_keypair` generates and stores it, sign/verify use it directly.
- **CMake-based upstreams**: run `cmake ref/ -B build/cmake/ -DCMAKE_POSITION_INDEPENDENT_CODE=ON`
  then `make -C build/cmake/` to produce static libs, then link shims against those `.a` files.
  See `schemes/sqisign/Makefile` for the stamp-based pattern.
- **Large stack**: some variants need >8 MB stack (e.g. SNOVA-97-33-16-2).
  `main.c` runs each scheme in a pthread with a 256 MB stack
  (`pthread_attr_setstacksize`) — this is independent of `RLIMIT_STACK` and
  works even on systems (e.g. AWS Linux) where the hard stack limit is capped.

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
   - `Makefile` — **`all:` must be the first explicit target** (add `.DEFAULT_GOAL := all`
     if `$(eval $(call ...))` blocks appear before it). Add shim generation rule:
     `$(SHIM_FILES): shim_template.c params.tsv` → `python3 ../../gen_shims.py …`.
     Compile upstream sources to `build/lib<name>.a` with `-fPIC`; compile+link
     each shim into `build/<file>.so`.

3. `bench/Makefile`:
   - Add `<NAME>_SOS` listing each `.so` path.
   - Add stamp-file prerequisites: `schemes/<name>/.stamp: Makefile shim_template.c params.tsv`.
   - Add shared recipe `$(STAMPS): schemes/%/.stamp:` already handles build; just add the
     prereq line.
   - Add `$(<NAME>_SOS): schemes/<name>/.stamp`.
   - Add `$(NAME_SOS)` to `ALL_SOS` and `<name>` to `SCHEME_DIRS`.
   - `SO_PATHS[]` in `main.c` is auto-generated — do **not** edit `main.c`.

## Filtering

`./bench [filter...]` runs only schemes whose names match at least one filter.
Matching is case-insensitive and ignores non-alphanumeric characters, so `mldsa`,
`ml-dsa`, and `ML_DSA` all match `ML-DSA-44`.

## Notes

- For stable numbers: disable turbo boost, pin to one core, performance governor.
- `rdtsc` measures reference cycles (constant TSC rate on modern Intel/AMD), not execution cycles.
- Message is a 32-byte zero buffer. Content does not affect cycle counts for lattice/hash schemes.
