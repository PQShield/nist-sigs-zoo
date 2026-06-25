# kembench/

Cycle-count + wall-time benchmark for **KEMs** (key encapsulation mechanisms).
Measures median keygen / encapsulation / decapsulation cycles and microseconds
per parameter set.

This is the KEM sibling of `../bench/` (signatures). The framework is identical
except the per-`.so` contract is the NIST KEM API. Read `../bench/CLAUDE.md`
first; this file only documents the differences.

## Build and run

```bash
git submodule update --init --recursive kembench/schemes/
make              # builds ./bench and all scheme .so files
./bench           # run all registered schemes
./bench mlkem     # filter by name (case-insensitive)
./bench ecdh hqc  # multiple filters OR-ed
make clean
```

`run_bench.sh` wraps `./bench`, prepends a system-info header, and saves output
to `results/<timestamp>_<cpu>.txt`. `update_kem_data.py` parses the newest (or a
given) results file and writes `keygen_*`/`encaps_*`/`decaps_*` fields into
`data/kems/*.yaml`, and writes `data/kem_benchmark_env.yaml`.

## Contract every `.so` must satisfy

```c
const bench_scheme_info_t *bench_info(void);  /* name, pk, sk, ct, ss, iters */

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
```

All return 0 on success. `bench_scheme_info_t` sizes map to the NIST API
constants: `pk_bytes`=CRYPTO_PUBLICKEYBYTES, `sk_bytes`=CRYPTO_SECRETKEYBYTES,
`ct_bytes`=CRYPTO_CIPHERTEXTBYTES, `ss_bytes`=CRYPTO_BYTES.

## Isolation model (handles badly-behaved implementations)

Each parameter set compiles to a self-contained `.so`, loaded with
`dlopen(RTLD_LOCAL)`. Symbols in one `.so` are invisible to others, so
**non-namespaced implementations coexist without conflict**. This is the whole
point of the design: many KEM references (notably HQC) export the bare,
non-namespaced symbols `crypto_kem_keypair` / `crypto_kem_enc` /
`crypto_kem_dec` for *every* variant. Because each variant lives in its own
`.so` and is never linked against another, the identical bare symbols never
collide.

If a future upstream *does* namespace its symbols (e.g. PQClean's
`PQCLEAN_HQC128_CLEAN_*`), declare the namespaced names `extern` in the shim —
optionally via a `@NS_PREFIX@` token from `params.tsv` — and wrap them with the
bare `crypto_kem_*` names.

## Harness specifics

- Same `rdtsc`+`lfence` (x86) / `cntvct_el0` (aarch64) counter as `bench/`.
- Warm-up also runs a **correctness check**: encapsulate, decapsulate, and
  `memcmp` the two shared secrets. A mismatch warns on stderr but does not abort
  (a broken implementation is still timed).
- Each scheme runs in a pthread with a 256 MB stack (code-based KEMs can use
  large on-stack key buffers).

## Shim patterns (KEM-specific)

- **ECDH as a KEM** (`schemes/ecdh/`): no submodule; uses OpenSSL `EVP_PKEY`.
  Modelled as DHKEM: `keypair` makes the static recipient key; `enc` makes an
  ephemeral key, does raw ECDH against the recipient public key, and returns the
  ephemeral public key as the ciphertext; `dec` does ECDH(recipient_sk,
  ephemeral_pk). The raw DH output is the shared secret (no KDF) — that is the
  operation we want to time. The recipient key is stored at module level.
- **ML-KEM** (`schemes/mlkem/`): pq-code-package/mlkem-native, **AVX2** backend,
  so it is **x86-64 only** (the upstream `sys.h` hard-errors when
  `MLK_FORCE_X86_64` is set off-x86). Built one `.so` per security level with
  `-DMLK_CONFIG_PARAMETER_SET=512|768|1024`. Flags mirror the upstream
  `test/mk/auto.mk` x86_64 path: `-DMLK_FORCE_X86_64 -mavx2 -mbmi2`, plus
  `-DMLK_CONFIG_USE_NATIVE_BACKEND_ARITH -DMLK_CONFIG_USE_NATIVE_BACKEND_FIPS202`
  to force the AVX2 + AVX2-Keccak backends (the portable C backend is the
  default otherwise), and `-DMLK_CONFIG_NAMESPACE_PREFIX=mlkem` with
  `-I ref/mlkem` for the config header. The Makefile globs all C/asm under
  `mlkem/src` (C backend + `native/x86_64` + `fips202` + `fips202/native/x86_64`);
  backend sources self-guard on `MLK_FORCE_X86_64`, so unselected ones compile to
  empty objects. mlkem-native's SUPERCOP layer already exports bare
  `crypto_kem_keypair/enc/dec` for a single-level build, so the shim only adds
  `bench_info()` plus a real `randombytes()` (via `getentropy()`); the upstream
  `test_only_rng` stub is deliberately unsafe and must not be linked. To build on
  a non-x86 host, drop the native-backend defines and `-mavx2/-mbmi2` to fall
  back to the portable C code.
- **HQC** (`schemes/hqc/`): canonical reference at
  https://gitlab.com/pqc-hqc/hqc, with bare `crypto_kem_*` symbols. We build the
  **AVX2** implementation (`HQC_ARCH=x86_64`, `HQC_X86_IMPL=avx256`), so it is
  **x86-64 only** — the upstream `immintrin.h` includes hard-error on other
  architectures. Flags + defines mirror the upstream CMakeLists for x86_64:
  `-mavx -mavx2 -mbmi -mpclmul -std=c99` plus `-DHQC_ARCH_X86_64=1
  -DHQC_ARCH_X86_64_AVX256=1`. Variants `hqc-1` / `hqc-3` / `hqc-5` are selected
  by **include path** (not a `-D`): the per-variant `src/common/<var>/api.h` and
  `src/x86_64/common/<var>/parameters.h` define the sizes/parameters. The
  Makefile globs the avx256 source set (`src/common/*.c`,
  `src/x86_64/common/*.c` + `<var>/`, `src/x86_64/avx256/*.c` + `<var>/`, and
  the bundled `lib/fips202/*.c`) per variant and links one `.so` each.
  SHA3/SHAKE comes from the bundled `lib/fips202` — **no OpenSSL**. HQC keeps a
  global PRNG that must be seeded with `prng_init()` before keygen; the shim
  does this once at `.so` load via a `constructor` calling `getentropy()`, which
  also avoids clashing with the bare upstream `crypto_kem_keypair`. To benchmark
  on a non-x86 host instead, switch the Makefile's globs/includes/flags to the
  portable `ref` source set (`src/ref/...`, no `-mavx2`).

## Adding a new KEM scheme

Mirror `../bench/CLAUDE.md`'s "Adding a new scheme":

1. Add upstream as `schemes/<name>/ref/` submodule (skip for system-library).
2. Create `schemes/<name>/`: `params.tsv` (columns `file name pk sk ct ss iters`
   plus any `@TOKEN@` columns), `shim_template.c`, `.gitignore` (`*_shim.c`),
   and a `Makefile` whose **first** target is `all:`.
3. In `kembench/Makefile`: add `<NAME>_SOS`, the `schemes/<name>/.stamp`
   prerequisite line, `$(<NAME>_SOS): schemes/<name>/.stamp`, and append to
   `ALL_SOS` + `SCHEME_DIRS`. `so_paths.h` is auto-generated; never edit `main.c`.
4. Add a `BENCH_TO_YAML` entry in `update_kem_data.py`.

## Notes

- ECDH "encaps" includes ephemeral key generation, so its keygen and encaps
  costs are similar — this is the realistic cost of using ECDH as a KEM.
- Message/zero buffers are unused; the harness passes real pk/sk/ct/ss buffers.
