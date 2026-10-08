---
name: integrating-ngcc-candidates
description: Use when adding or changing rows in bench/schemes/ngcc/params.tsv or bench-kem/schemes/ngcc/params.tsv, when a China NGCC candidate fails to build or shows MISMATCH, NOKAT, CRYPTOFAIL, CRASH or TIMEOUT in status.tsv, when choosing impl=ref or opt, when NGCC sizes or timings look wrong, or before regenerating NGCC site data.
---

# Integrating NGCC candidates

**Core principle:** a non-PASS row or an odd timing is *our build* until the submitter's own build files show otherwise. Rows must reproduce what the submitter's default build compiles. `bench/CLAUDE.md` ("NGCC candidates", "Running NGCC candidates") still applies.

## 1. Read the submitter's build first

Before writing or changing a row, read the tree's `Makefile`, `*.mk`, `CMakeLists.txt`, `*.cmake` and `README*` under `bench-common/ngcc/src/<id>/`. If they are missing, re-run `uv run bench-common/ngcc/extract.py <id>`. Read them as text and never run them. Copy the default target's defines into `defs`:
- `-DNDEBUG`, table switches (`-DSBOX_TABLE`)
- SIMD and ISA switches (`-D__AVX2`, `-DUSE_AVX2_*`, `-DPRG_ACCE=1`, `*_AESNI`, `*_FIELD_PCLMUL`)
- parameter selectors (`-DSECURITY_LEVEL=`, `-DPARAMS=`, `-D<X>_MODE=`)
- randomness switches (`-DNIST_RAND`, `-DSUBMISSION_DRNG`, `-DAPI_PKC_KAT_DRNG_BRIDGE`, `-DXOF_PSEUDO`)

All randomness must come from `drng_algorithm`. Drop `/dev/urandom` and `rand()` sources.

If several instances share one directory or have identical directories, put the parameter define on **every** row. Amoeba, for example, builds every size as level 128 without it. Then confirm that `ngcc_sizes.tsv` gives different sizes per instance.

## 2. Build, replay KATs, extract sizes (container only)

Use one invocation per candidate group (a full status run is OOM-killed), with its own `NGCC_OUT`, under the shared lock:

```bash
flock /tmp/ngcc-run.lock env NGCC_OUT=$PWD/.ngcc-out-sign05 scripts/ngcc-run sh -c '
  cd bench/schemes/ngcc &&
  { grep -v "^sign_" params.tsv; grep "^sign_05_" params.tsv; } > /tmp/p.tsv &&
  export PARAMS=/tmp/p.tsv && make -j4 all &&
  NGCC_KAT_TIMEOUT=600 make STATUS=/tmp/status.tsv status &&
  cd ../.. && make ALL_SOS= STAMPS= bench && ./bench --sizes NGCC > /tmp/sizes.tsv'
```

For KEMs use `bench-kem/`, `^kem_` and `./bench-kem`. Outputs arrive in `.ngcc-out-sign05/tmp/`, and build logs in `.ngcc-out-sign05/**/build/<file>.log`. Review them, then merge the changed rows into the committed `status.tsv` and `results/ngcc_sizes.tsv` by hand.

Before you call a non-PASS row a submission defect, work through `build-triage.md`.

## 3. ref or opt

Set `impl=opt` only when all three hold:
1. The opt tree builds (C/asm only; C++ trees stay ref).
2. The KAT is **PASS** against the opt tree's **own** KAT file if it ships one (`*_OPT.txt`, `*_AVX2.txt`, `*-Optimized.txt`, `Test_Vectors/Optimized_Implementation/`).
3. pk/sk/sig|ct sizes are identical to the ref build's.

Otherwise keep ref and put the reason in the `params.tsv` comment above the candidate.

## 4. Timing sanity (cross-check only)

Smoke-bench the changed rows (`BENCH_ITER=3` via `ngcc-run`). Compare per-op cycles with the spec's performance table or ngcc.dev. Container rdtsc reads about 0.55–0.8× real cycles. Anything off by more than 5× usually means a define is missing. Never copy third-party numbers into data.

## 5. Candidate bugs

Put a patched copy in `<suite>/schemes/ngcc/patches/<id>/` with a `/* PATCH (nist-sigs-zoo): why */` header, list it in `srcs`, mention it in the row comment, and link the item in `docs/ngcc-submission-bugs.md`. Never edit extracted sources, the shim template, `ngcc_kat`, `status.py` or `kat_compare` for one candidate.

## 6. Regenerate and test

```bash
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv
uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests
npm run test
```

Never hand-edit generated `data/schemes|kems/*.yaml`. Add a `data/history.yaml` entry when site data changes.
