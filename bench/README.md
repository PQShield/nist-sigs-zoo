# bench

Cycle-count and wall-time benchmark for post-quantum and classical signature schemes.
Measures median keygen / sign / verify over multiple iterations.

## Quick start

```bash
# Initialize submodules (first time)
git submodule update --init --recursive bench/schemes/

# Build everything and run
cd bench/
make
./bench

# Filter by scheme name (case-insensitive, non-alphanumeric ignored)
./bench mldsa
./bench rsa ecdsa        # multiple filters are OR-ed

# Save results with system info header
./run_bench.sh           # all schemes → results/<timestamp>_<cpu>.txt
./run_bench.sh sqisign   # filtered; slug appended to filename
```

## Dependencies

| Scheme family | Extra requirement |
|---|---|
| Classic (RSA, ECDSA, EdDSA) | OpenSSL 3.x (`libssl-dev` / `openssl@3`) |
| SQIsign | CMake ≥ 3.13 + GMP (`cmake libgmp-dev`) |
| All others | C compiler, make, git |

## Schemes

| Family | Parameter sets | Category |
|---|---|---|
| ML-DSA | 44, 65, 87 | Lattice (Module-LWE) |
| SLH-DSA | 12 (sha2/shake × 128/192/256 × s/f) | Hash-based |
| FN-DSA | 512, 1024 | Lattice (NTRU) |
| HAWK | 512, 1024 | Lattice (LIP) |
| MAYO | 1, 2, 3, 5 | Multivariate |
| FAEST | 12 (128/192/256 × s/f × standard/EM) | Symmetric (VOLE-in-the-Head) |
| SDitH | 12 (hypercube/threshold × cat1/cat3/cat5 × gf256/p251) | MPC-in-the-Head |
| SNOVA | 15 | Multivariate |
| UOV | 8 (Ip/Is/III/V × classic/pkc) | Multivariate |
| QR-UOV | 12 | Multivariate |
| MQOM | 36 (L1/L3/L5 × gf2/gf16/gf256 × fast/short × r3/r5) | MPC-in-the-Head |
| SQIsign | I, III, V | Isogenies |
| Classic | RSA-2048/3072/4096, ECDSA-P256/384/521, Ed25519, Ed448 | Classical baseline |

## Output format

```
scheme                        keygen (cyc)  keygen (us)      sign (cyc)   sign (us)    verify (cyc)  verify (us)
ML-DSA-44                            39536        26.4           93340        62.4           40494        27.1
```

Six columns: keygen and sign and verify, each as reference cycles (`rdtsc`) and microseconds.
Values are medians over `BENCH_ITER` iterations (default 1000; overridden per scheme via `iters`
field in `bench_scheme_info_t`).

Cycle counter uses `rdtsc` + `lfence` (x86); constant-TSC rate, not execution cycles.

## Stable measurements

```bash
# Disable turbo boost (Intel)
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# Pin to one core, performance governor
taskset -c 0 ./bench
```

## Architecture

```
bench/
├── scheme.h          # bench_scheme_info_t — metadata struct for shims
├── loader.h/c        # dlopen wrapper: resolves the 4 required symbols
├── harness.h         # rdtsc cycle counter, clock_gettime wall time, bench_run()
├── main.c            # SO_PATHS[] list, filter logic, main loop
├── Makefile          # builds ./bench + delegates to per-scheme sub-makes
├── run_bench.sh      # collects sysinfo, tees output to results/
├── gen_shims.py      # generates <param>_shim.c from shim_template.c + params.tsv
└── schemes/
    ├── classic/      # no submodule; uses system OpenSSL
    ├── mldsa/        # pq-crystals/dilithium
    ├── slhdsa/       # pq-code-package/slhdsa-c
    ├── fndsa/        # pornin/c-fn-dsa
    ├── hawk/         # hawk-sign/dev
    ├── mayo/         # PQCMayo/MAYO-C
    ├── faest/        # faest-sign/faest-ref
    ├── sdith/        # sdith/sdith
    ├── snova/        # PQCLAB-SNOVA/SNOVA
    ├── uov/          # pqov/pqov
    ├── qruov/        # qruov/round2
    ├── mqom/         # mqom/mqom-v2
    └── sqisign/      # SQISign/the-sqisign (nist-v2 tag)
```

Each scheme directory contains:
- `ref/` — git submodule pointing to upstream implementation
- `params.tsv` — one row per parameter set; columns substituted into shim template
- `shim_template.c` — C template with `@COLNAME@` tokens
- `Makefile` — builds `build/<param>.so`; invokes `gen_shims.py`
- `.gitignore` — excludes generated `*_shim.c` and `build/`

## Isolation model

Each parameter set compiles to a self-contained `.so`, loaded with `dlopen(RTLD_LOCAL)`.
Symbols in one `.so` are invisible to others — non-namespaced implementations
(e.g. bare `crypto_sign_keypair`) coexist without conflict.

## Contract every `.so` must satisfy

```c
const bench_scheme_info_t *bench_info(void);

int crypto_sign_keypair(uint8_t *pk, uint8_t *sk);
int crypto_sign_signature(uint8_t *sig, size_t *siglen,
                          const uint8_t *m, size_t mlen,
                          const uint8_t *sk);
int crypto_sign_verify(const uint8_t *sig, size_t siglen,
                       const uint8_t *m, size_t mlen,
                       const uint8_t *pk);
```

All return 0 on success.

## Adding a scheme

See `CLAUDE.md` for detailed instructions. Summary:

1. Add upstream as `schemes/<name>/ref/` submodule.
2. Create `params.tsv`, `shim_template.c`, `Makefile`, `.gitignore`.
3. Add `<NAME>_SOS`, stamp rule, and `.so` dependency in `bench/Makefile`.
4. `make` rebuilds everything; `./bench <name>` tests the new scheme.
