# bench-kem

Microbenchmark for post-quantum and classical **KEMs**. Measures median keygen,
encapsulation, and decapsulation cost (CPU cycles + wall-clock µs) per parameter
set, and checks that encaps/decaps agree on the shared secret.

Companion to [`../bench`](../bench) (the signature benchmark); same isolation and
codegen design, KEM op contract.

## Quick start

```bash
git submodule update --init --recursive bench-kem/schemes/
cd bench-kem
make
./bench-kem               # all schemes
./bench-kem mlkem         # filter by name (case-insensitive, OR-ed)
BENCH_ITER=200 ./bench-kem
./run_bench.sh            # saves a timestamped run with system info to results/
```

Output columns: `keygen`, `encaps`, `decaps` in cycles and µs, plus an `ok` column
(`yes` if every encaps/decaps round-trip agreed on the shared secret, else `FAIL`).

## Schemes

| Scheme  | Source                                      | Parameter sets            |
|---------|---------------------------------------------|---------------------------|
| ML-KEM  | `pq-crystals/kyber` (avx2), FIPS 203        | ML-KEM-512 / 768 / 1024   |
| HQC     | `gitlab.com/pqc-hqc/hqc` (x86_64/avx256)    | HQC-128 / 192 / 256       |
| ECDH    | OpenSSL 3 (EVP)                             | X25519, P-256             |

Each KEM lives under `schemes/<name>/`; see `CLAUDE.md` for the contract and how
to add one.

## How it works

Every parameter set is compiled into a standalone `.so` exporting the standard NIST
KEM API (`crypto_kem_keypair` / `crypto_kem_enc` / `crypto_kem_dec`) plus a metadata
function. The runner `dlopen`s each with `RTLD_LOCAL`, so implementations that use
identical, non-namespaced symbol names do not collide. A small generated shim adapts
each upstream's calling convention to the standard names.

## Requirements

- C compiler, `make`, `python3`, git submodules.
- OpenSSL 3.x for ECDH.
- An x86-64 (AVX2) or aarch64 host for the cycle counter.
