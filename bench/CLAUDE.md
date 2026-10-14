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
- CMake ≥ 3.13 for SQIsign (`sudo apt-get install cmake`) — nist-v3 dropped the GMP dependency.

## Architecture

```
bench/
├── scheme.h              # bench_scheme_info_t — metadata type used by shims
├── loader.h / loader.c   # dlopen loader: bench_scheme_t, bench_load/unload
├── harness.h             # sig bench_run() + columns; shares cycle counter/stats/timing
│                         #   with bench-kem/ via ../bench-common/harness_common.h
├── main.c                # includes build/so_paths.h + filter logic + main loop
│                         #   (one forked child per scheme, ../bench-common/isolate.h)
├── gen_shims.py          # shim generator: substitutes @COLNAME@ tokens from params.tsv
├── cpu_features.py       # prints 1 if the host CPU has all named features (/proc/cpuinfo);
│                         #   scheme Makefiles use it to pick GFNI/AVX-512 backends
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
    ├── qruov/            # qruov/round3 (IMPL=avx2, PRG=aes; six q=127 parameter sets)
    ├── mqom/             # mqom/mqom-v3
    ├── sqisign/          # SQISign/the-sqisign (CMake build; SQISIGN_BUILD_TYPE=broadwell by default, x86-64 asm field arithmetic)
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

- **Cycle counter**: prefers the core's **real** `CPU_CYCLES` PMC via `rdpmc` (no
  root; `perf_event` mmap page + kernel-assigned index, avoiding the hybrid P/E-core
  rdpmc bug), falling back to `rdtsc`+`lfence` (x86, a constant-rate off-core clock,
  not real cycles) / `cntvct_el0` (aarch64) when perf/rdpmc is unavailable or
  `BENCH_CYCLES=tsc`. User-space cycles only (kernel excluded). The worker thread is
  pinned to one core (`BENCH_CPU`, default 0). The counter + stats + timing macro
  live in the shared `../bench-common/harness_common.h` (used by `bench-kem/` too);
  the active counter is printed as `# cyclecounter:` and recorded in the env notes.
- **Wall clock**: `clock_gettime(CLOCK_MONOTONIC)` bracketing each operation.
- **Stats**: median over `BENCH_ITER` (default 1000) iterations.
  Override at build time: `make BENCH_ITER=200`.
- **Per-scheme override**: set `iters` field in `bench_scheme_info_t` to a non-zero value.
  A negative `iters` means "do not time": `./bench` still loads the scheme for `--sizes` but
  skips it in the timed run (also under `BENCH_ITER`). NGCC rows get this from `iters` `skip` in
  `params.tsv`: built, KAT-checked, sizes imported, never benchmarked (the YAML notes say a
  single run takes hours). Use it only for rows too slow to time; explain why in a comment.
- **bench_run()**: one warm-up, then `n` timed iterations; writes one row with six columns.
- **Correctness check**: before timing, a fresh signature must verify and a tampered
  signature / message must be rejected; return codes are also checked in the timed loop.
  A failing scheme gets a **DNF row** (below) instead of timings, so a broken build can't
  be imported with numbers.

## Crash isolation and DNF rows (`../bench-common/isolate.h`)

Every scheme `.so` is loaded and benchmarked in its own **forked child** (fork before
`dlopen`, so the scheme's constructors run there too; the large-stack thread, CPU
pinning and the perf/rdpmc counter all live in the child). The child sends its row
to the parent over a pipe; the parent prints it only after the child exited cleanly.
Anything a scheme prints on stdout goes to stderr, never between rows. A scheme that
crashes, hangs or fails its correctness check costs only its own row, which becomes:

```
NGCC-Tins128                     2             DNF         DNF             DNF         DNF             DNF         DNF  valid-signature-rejected
```

i.e. name, iters, six `DNF` timing columns, and a one-word reason: the signal
(`SIGABRT`, `SIGSEGV`, `SIGFPE`, …, `signal-N`), `timeout`, `operation-over-<n>s`, `exit-N`, `no-result`, or
the failed check (`keygen-failed`, `sign-failed`, `bad-signature-length`,
`valid-signature-rejected`, `tampered-signature-accepted`, `tampered-message-accepted`,
`<op>-error-in-timed-loop`, `alloc-failed`). Each DNF also prints `DNF <name>: <reason>`
on stderr. The run always continues with the next scheme.

- `BENCH_TIMEOUT` (seconds, default 3600, `0` = none) bounds each scheme's wall-clock
  time, including all iterations; on expiry the child gets `SIGKILL` (`timeout`).
  Raise it for runs whose slowest scheme legitimately needs more than an hour.
- `BENCH_OP_LIMIT` (seconds, default 600, `0` = none; shared with `bench-kem/`, in
  `../bench-common/harness_common.h`): policy (Thom, 2026-10-13) is that a scheme needing more
  than 10 minutes for one signature or verification (KEMs: one encapsulation or decapsulation) is
  not benchmarked. Each such call of the warm-up check runs under `alarm()`; on expiry the child
  dies with SIGALRM and the DNF reason is `operation-over-600s` (runs before 2026-10-14 wrote
  `sign-or-verify-over-600s`; the importers accept any one-word reason). A value that is not a
  number of seconds is reported on stderr and the default is used. Key generation is not limited
  (UVW-512 uses `iters skip`). Exemption (Thom, 2026-10-14): rows listed in
  `../scripts/aws-bench-exemptions.txt` (the SLH-DSA SP 800-230 `*-24` sets) run with
  `BENCH_OP_LIMIT=0 BENCH_TIMEOUT=0` when launched through `scripts/aws-bench-split`, each on its own
  instance. Adding a row to that file needs Thom's decision. The harness itself is unchanged.
- Exit status: 0, or **2 if every scheme that ran was DNF** (usually a broken setup).
  Filters that match nothing exit 0.
- `--sizes` is unchanged (in-process, no isolation).
- The importer (`update_scheme_data.py`) never imports timings from a DNF row. On an
  NGCC parameter set it removes the old timings, appends
  `Benchmark did not finish (<reason>).` to its notes and writes `dnf: <reason>` (shown as
  "timeout" or "DNF" in the timing cells; `ngcc_data.mark_dnf`; a later good row removes
  both, `--ngcc-sizes` keeps them). `iters skip` rows get `dnf: timeout`. On any other scheme it prints a
  warning and keeps the existing timings.
- This is generic: never special-case a candidate here. A crasher is fixed in its own
  wrapper (see "NGCC candidates") or shows up as DNF.
- Tests: `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/tests`
  (CI runs it together with `bench-common/ngcc/tests`) builds both harnesses against toy schemes (`bench-common/tests/fixtures/`) that abort,
  segfault, divide by zero, hang, fail the warm-up check or succeed, and tests the
  importers' parsers. Host-safe: no real scheme code runs.

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
  `sqisign_p324_3_broadwell_crypto_sign_keypair` rather than `crypto_sign_keypair`
  (SQIsign: the build-type part is filled in by the Makefile via `-DSQISIGN_NS_TYPE`).
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
  `main.c` runs each scheme (in its child process) in a pthread with a 256 MB stack
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

## Optimised builds

Benchmark upstream's **fastest x86-64 build**, as its own build system produces it by
default. Check this against upstream's Makefile/CMake (e.g. `make -n` in `ref/`), not a
liboqs META profile: MQOM's portable "default" profile was 3-5x slower (see #58).
Backends that need ISA extensions are chosen with `cpu_features.py`, e.g.

```make
HAVE_AVX512 := $(shell python3 ../../cpu_features.py avx512f avx512bw avx512_vnni)
```

Also pass the matching `-m` flags explicitly so the fast path can be compile-tested on
another machine (`make HAVE_AVX512=1`). `BENCH_DISABLE_FEATURES="gfni avx512f"`
forces the fallback. Current choices: MAYO `gfni` (GFNI+AVX-512) else `avx2`; QR-UOV
`IMPL=avx2` (Round 3 dropped the Round 2 avx512 implementation); UOV `PROJ=gfni`
else `avx2`; SQIsign `broadwell`; SNOVA, FAEST,
MQOM, ML-DSA etc. rely on `-march=native`.

## Notes

- For stable numbers: disable turbo boost, performance governor (the harness
  already pins to one core and reads real core cycles via the PMC when available).
- The `rdpmc` PMC path reports real core cycles; the `rdtsc` fallback reports
  reference cycles (constant TSC rate on modern Intel/AMD), which diverge from real
  cycles under turbo/frequency scaling.
- Message is a 32-byte zero buffer. Content does not affect cycle counts for lattice/hash schemes.

## NGCC candidates (ICCS API)

China NGCC round-1 candidates all implement the ICCS public-key C API, so they share
one shim (`schemes/ngcc/shim_template.c`) and are integrated as **data**: one row per
`ALGORITHM_INSTANCE` in `schemes/ngcc/params.tsv` (columns documented in
`../bench-common/ngcc/gen_rules.py`). The shared machinery lives in `../bench-common/ngcc/`:

```bash
uv run bench-common/ngcc/fetch.py      # official archives -> $NGCC_CACHE, sha256-pinned in archives.tsv
uv run bench-common/ngcc/extract.py    # allowlisted sources/text/specs -> bench-common/ngcc/src, specs (git-ignored); --kats adds KAT files
scripts/ngcc-run python3 -I bench-common/ngcc/unpack_nested.py   # nested .rar (DOVE): unpacked in the container only
python3 -I bench-common/ngcc/extract.py --import-unpacked .ngcc-out/unpacked   # file their text under src/
python3 bench-common/ngcc/inventory.py sig sign-07   # draft params.tsv rows (kat column needs extract.py --kats)
python3 -I bench-common/ngcc/kat_hashes.py          # after a params.tsv kat change: rewrite kat_hashes.tsv, commit it
```

Building, `make status` (build/KAT/hash status -> `schemes/ngcc/status.tsv`, commit it) and
`./bench --sizes NGCC > results/ngcc_sizes.tsv` (input for the YAML import, commit it) run
through `scripts/ngcc-run`; see "Running NGCC candidates" below.

- Only `build/built.list` reaches `SO_PATHS`, so a failing candidate never breaks `./bench`.
  Without extracted sources (e.g. a fresh clone) the NGCC build is skipped with a message.
- Randomness comes from the ICCS DRNG `drng_algorithm` (defined and seeded by the shim);
  drop candidates' `/dev/urandom` `randombytes.c`, which also breaks KAT replay.
- `defs` reach only the candidate's sources and `shimdefs` only the shim, so NIST-API name
  clashes can be renamed away (`-Dcrypto_sign_keypair=cand_crypto_sign_keypair`).
- `src/` also holds the candidates' build and documentation files (Makefile, *.mk, CMake,
  meson, *.sh, *.pro, *.vcxproj, README*, *.md, non-KAT *.txt) as plain 0644 text, for reading only.
- Never edit extracted sources or run candidates' build systems: fixes are copies in
  `schemes/ngcc/patches/<id>/` with a `/* PATCH (nist-sigs-zoo): why */` header.
- Fix a candidate's bugs in its own wrapper (the submitted ICCS adapter, e.g.
  `SIG_AlgorithmInstance.c`, patched as above), not in the harness: the shim template,
  `ngcc_kat`, `status.py` and `kat_compare` stay generic and never special-case a candidate,
  so they keep catching real defects in others. Describe the defect and the workaround in
  the PATCH header and in the `params.tsv` comment; both must be self-contained (no pointers
  to documents outside the repo). A header that starts `/* PATCH (nist-sigs-zoo, fix):`
  marks the row as patched (`status.tsv` column `patch` = `fix`, set by `status.py`), and the
  site notes then say neutrally that our build applies a small patch. Build-only patches
  (visibility wrappers, missing headers, DRNG copies) keep the plain `PATCH (nist-sigs-zoo)`.
- Compiler flags are global: every candidate builds with `NGCC_CFLAGS` from `ngcc.mk`
  (`-O3 -march=native -mtune=native -fPIC -std=gnu17`). There are no per-row CFLAGS.
  C++ sources (`.cpp`/`.cc`/`.cxx`, picked up by `*` like `.c`) are compiled with `$(CXX)`
  and `NGCC_CXXFLAGS` (same flags, `-std=gnu++23`, which covers the submitters' c++20/c++23
  trees) into `build/obj/<file>/`; such a `.so` is linked with `-lstdc++`.
  Mirror the submitter's preprocessor defines and source list in `defs`/`srcs`, but not
  their optimisation or codegen flags: `-Ofast`, `-funroll*-loops`, `-flto`, `-fopenmp`,
  `-fno-stack-protector`, `-fcf-protection=none`, `-O2` and the like are ignored. A row
  that only reproduces its KAT or only runs correctly with such a flag stays on the
  reference build, with the reason in its comment (e.g. yuanyang-512 opt, built with `-Ofast`).
- Optimised x86 builds: a row is `impl=opt` only where the submitted `Optimized_Implementation`
  (avx2/asm) builds here, reproduces the submitted KAT (`status.tsv` `PASS`) and gives the same
  pk/sk/sig sizes as the reference build (`ngcc_sizes.tsv` stays identical). Everything else keeps its
  reference row; the reason is in the `params.tsv` comment above the candidate. C++ optimised trees
  are built like C ones (Lynxer, ReSolveD, sydo); Galas stays on the reference build because its
  optimised tree has no ICCS adapter.
  Multithreaded optimised trees (Sigurd) also stay on the reference build: benchmarks are
  single-threaded and the cycle counter only sees the calling thread.
- KATs are replayed by our `ngcc_kat` (ICCS seed schedule) and the first `katn` records are
  compared with the submitted files' per-record hashes in the committed
  `../bench-common/ngcc/kat_hashes.tsv` (canonical record form defined in `kat_compare.py`,
  plus each file's raw sha256), so the KAT files themselves need not be extracted.
  `status.tsv` `katrec` gives the first mismatching record. Mismatches are recorded, not
  hidden: any built instance whose `kat` is not `PASS` gets a neutral `info` flag (the "note"
  badge, `KAT_INFO` in `ngcc_data.py`) besides its notes, so its timings never appear without a
  caveat (`ngccCandidates.test.ts` checks this against `status.tsv`). To diagnose one, `extract.py --kats <id>`
  and `python3 -I bench-common/ngcc/kat_compare.py OURS.txt SUBMITTED.txt`.

## Running NGCC candidates

NGCC candidates' C code is untrusted. Every build, KAT replay, `--sizes`
extraction and local smoke benchmark of it goes through `scripts/ngcc-run`,
never directly on the host:

```bash
scripts/ngcc-run make -C bench/schemes/ngcc -j"$(nproc)"
scripts/ngcc-run sh -c 'cd bench && ./bench --sizes NGCC > results/ngcc_sizes.tsv'
scripts/ngcc-run --self-test     # verify the sandbox
```

It runs a throwaway container (`scripts/ngcc-container/Dockerfile`, Ubuntu 26.04):
`--network none`, capabilities dropped, non-root, read-only rootfs. The repo is
mounted read-only and copied into a writable `/work` volume; the command runs
there, and no host-writable path is mounted while it runs. Afterwards the host
pulls `/work` through the trusted `scripts/ngcc-container/export.py`, which writes
to the git-ignored `.ngcc-out/` only allowlisted regular files
(`*/schemes/ngcc/*.tsv`, `ngcc_sizes.tsv`, `*/results/*.txt`, `*.log`, KAT
`*.rsp`/`KAT*.txt`, text staged from `/tmp` as `.ngcc-out/tmp/*`, source/build text that
`unpack_nested.py` stages in `.ngcc-unpacked/` as `.ngcc-out/unpacked/*`) that are plain
UTF-8 text with no NUL bytes, mode 0644. Symlinks, binaries and oversize files
are rejected and listed on stderr. Review them, then copy them to their committed locations by hand.
For two runs at once, give each its own output dir (`NGCC_OUT=$PWD/.ngcc-out-<name> scripts/ngcc-run …`); they would otherwise overwrite each other's `.ngcc-out/`.
Binaries, `.so` and `.mk` files never leave the container, and the host never
runs or `make`s container output. Host `make -C bench` for non-NGCC work must
run with NGCC disabled, or after removing `bench*/schemes/ngcc/build`.
The NGCC suite is opt-in: `bench/schemes/ngcc` (and the stamp rule in
`bench/Makefile`) only compile or load candidate code when `NGCC_BUILD=1`,
which the container image sets (the AWS VM sets it too). On a host both print
"NGCC: skipped" and exit 0, and nothing from `schemes/ngcc` enters `so_paths.h`.
Build and `make status` must run in the same `ngcc-run` invocation (fresh
container each time). `NGCC_KEEP=1` keeps the stopped container for debugging. If your shell
lacks the docker group: `sg docker -c "scripts/ngcc-run …"`. `rdpmc` is usually
unavailable in the container, so local timings are smoke tests only; real numbers
come from `scripts/aws-bench.sh`.
