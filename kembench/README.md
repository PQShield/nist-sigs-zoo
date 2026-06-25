# kembench

Cycle-count and wall-time benchmark for post-quantum and classical KEMs
(key encapsulation mechanisms). Measures median keygen / encapsulation /
decapsulation over multiple iterations.

This is the KEM counterpart of `../bench/` (which benchmarks signatures). It
uses the same isolation model and shim-template system, but the per-`.so`
contract is the NIST **KEM** API (`crypto_kem_keypair` / `crypto_kem_enc` /
`crypto_kem_dec`) instead of the signature API.

## Quick start

```bash
# Initialize submodules (first time)
git submodule update --init --recursive kembench/schemes/

cd kembench/
make
./bench

# Filter by scheme name (case-insensitive, non-alphanumeric ignored)
./bench mlkem
./bench ecdh hqc          # multiple filters are OR-ed

# Save results with system info header
./run_bench.sh            # all schemes → results/<timestamp>_<cpu>.txt
./run_bench.sh hqc        # filtered; slug appended to filename

# Feed measured timings back into data/kems/*.yaml
uv run update_kem_data.py
```

## Dependencies

| Scheme family | Extra requirement |
|---|---|
| ECDH (X25519, X448) | OpenSSL 3.x (`libssl-dev` / `openssl@3`) |
| HQC | x86-64 with AVX2/BMI/PCLMUL (bundled `lib/fips202` for SHA3) |
| ML-KEM | x86-64 with AVX2/BMI2 (pq-code-package/mlkem-native) |

> HQC and ML-KEM build their **AVX2** implementations and require an x86-64
> host. For a non-x86 host, switch the respective scheme Makefile to the
> portable reference source set (see `CLAUDE.md`).

## Schemes

| Family | Parameter sets | Category |
|---|---|---|
| ECDH | X25519, X448 | Pre-quantum (DHKEM-style) |
| ML-KEM | 512, 768, 1024 | Lattice (Module-LWE) |
| HQC | 128, 192, 256 | Code-based |

## Output format

```
scheme                          iters    keygen (cyc)  keygen (us)    encaps (cyc)  encaps (us)    decaps (cyc)  decaps (us)
ML-KEM-768                       1000           45123        18.0           52310        20.9           61204        24.5
```

Six measurement columns: keygen, encaps, decaps — each as reference cycles
(`rdtsc`) and microseconds. Values are medians over `BENCH_ITER` iterations
(default 1000; overridden per scheme via the `iters` field in
`bench_scheme_info_t`).

Each scheme is also checked for **correctness** during warm-up: the
decapsulated shared secret must equal the encapsulated one. A mismatch prints a
warning to stderr but still reports timings (a broken/badly-behaved
implementation is allowed — see the isolation model).

## Stable measurements

```bash
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo   # disable turbo
taskset -c 0 ./bench                                              # pin to one core
```

## Architecture

```
kembench/
├── scheme.h          # bench_scheme_info_t — metadata struct for shims
├── loader.h/c        # dlopen wrapper: resolves the 4 required symbols
├── harness.h         # rdtsc cycle counter, clock_gettime wall time, bench_run()
├── main.c            # SO_PATHS[] list, filter logic, main loop
├── Makefile          # builds ./bench + delegates to per-scheme sub-makes
├── run_bench.sh      # collects sysinfo, tees output to results/
├── gen_shims.py      # generates <param>_shim.c from shim_template.c + params.tsv
├── update_kem_data.py# writes measured timings into data/kems/*.yaml
├── results/          # saved benchmark runs (committed)
└── schemes/
    ├── ecdh/         # no submodule; uses system OpenSSL (DHKEM-style)
    ├── mlkem/        # pq-code-package/mlkem-native AVX2 (per-level .so)
    └── hqc/          # gitlab.com/pqc-hqc/hqc AVX2 (bare crypto_kem_*; variants hqc-1/3/5)
```

## Isolation model

Each parameter set compiles to a self-contained `.so`, loaded with
`dlopen(RTLD_LOCAL)`. Symbols in one `.so` are invisible to others, so
**non-namespaced implementations coexist without conflict** — HQC's reference
exports a bare `crypto_kem_keypair` for every variant, and mlkem-native's
single-level builds export identical `crypto_kem_*` names across all three
levels; none of them collide because each `.so` has its own symbol table.

## Contract every `.so` must satisfy

```c
const bench_scheme_info_t *bench_info(void);

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);
int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk);
int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk);
```

All return 0 on success.

## Adding a scheme

See `CLAUDE.md` for detailed instructions. Summary:

1. Add upstream as `schemes/<name>/ref/` submodule (skip for system-library schemes).
2. Create `params.tsv`, `shim_template.c`, `Makefile`, `.gitignore`.
3. Add `<NAME>_SOS`, stamp rule, and `.so` dependency in `kembench/Makefile`.
4. `make` rebuilds everything; `./bench <name>` tests the new scheme.
5. Add a `BENCH_TO_YAML` entry in `update_kem_data.py`.
