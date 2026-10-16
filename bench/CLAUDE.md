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
    ├── aimer/            # samsungsds-opensource/AIMer v3.0 (ships AIMer.zip, unpacked into build/src; reference implementation only)
    ├── haetae/           # HAETAE 1.2.0 (official zip from Google Drive, vendored as HAETAE-1.2.0.zip, MIT; URL + sha256 in .source; reference implementation only)
    ├── mirath/ ryde/ perk/  # official round-2 zips (URL + sha256 in .source, fetched into build/dl; 30-36 MB each, so not vendored); AVX2 Optimized_Implementation
    ├── cross/            # CROSS-signature/CROSS-implementation v2.2 (Optimized_Implementation, AVX2)
    ├── less/             # less-sig/LESS 9ec6915 = NIST round-2 code (Optimized_Implementation/avx2)
    ├── <x>_r1/ <x>_r2/   # older NIST on-ramp rounds of the schemes still in the competition
    │                     #   (see "Older on-ramp rounds"): faest, hawk (r1 = r1+r2), mayo, mqom, qruov,
    │                     #   sdith (r2; sdith/ = r1), snova, sqisign, uov
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
- A listed NGCC instance with no `params.tsv` row at all (no code in the submission package,
  e.g. BiT-384) gets `no_code: <reason>` from `ngcc_data.py` (`NO_CODE`, or the instance's
  `no_code:` in candidates.yaml); the site shows "no code" and aws-bench-split never plans it.
- This is generic: never special-case a candidate here. A crasher is fixed in its own
  wrapper (see "NGCC candidates") or shows up as DNF.
- Tests: `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/tests`
  (CI runs it together with `bench-common/ngcc/tests`) builds both harnesses against toy schemes (`bench-common/tests/fixtures/`) that abort,
  segfault, divide by zero, hang, fail the warm-up check or succeed, and tests the
  importers' parsers. Host-safe: no real scheme code runs.

## Benchmark coverage check (CI)

`../bench-common/tests/test_benchmark_coverage.py` (CI's Python job) fails, one line per
parameter set (file, version, set, rule), unless: (1) every set a mapping lands on (both
importers' `BENCH_TO_YAML`, version-aware entries included; NGCC: instances with a
`params.tsv` row) has all the suite's timing fields plus `bench_env`, or a `dnf:` reason
(`iters skip` gives `dnf: timeout`); (2) every set of a scheme still in a competition has a
mapping or `no_code:`; (3) every mapping resolves (the importers also refuse otherwise).
"Still in a competition" is derived from `../data/programmes.yaml`: the scheme's newest version
naming a programme lists that programme's final stage (on-ramp round-3, NGCC round-1, KpqC
selected, NIST PQC standardised, ISO published). Rule 2 covers the newest version and, for the
on-ramp (`ALL_ROUNDS`, Thom's decision B), every on-ramp-tagged version; eliminated schemes
are exempt from it, not from rule 1. It reuses the importers' `target_errors()` and
`mapped_sets()`. A failure means benchmark and import the rows, or mark the set; never weaken
the rules.

## Provenance and staleness (`../bench-common/fingerprint.py`)

`run_bench.sh` writes `# commit:` (the built HEAD, `-dirty` with uncommitted tracked changes)
and `# instance_type:` (`BENCH_INSTANCE_TYPE`, set by `scripts/aws-bench.sh`; else `local`) into
each results header; `merge_results.py` copies both into every `# part:` line. On import,
`update_scheme_data.py` fingerprints each timed row (and NGCC DNF rows) at the commit its run
(its part, for merged files) built and writes `results/fingerprints.tsv`: name, fingerprint and
its components, result, commit, instance type, CPU, date, results file. It also sets the row's
`bench_env` and adds the run to `data/benchmark_env.yaml` (`../bench-common/bench_env.py`).

A fingerprint hashes what determines a row's timing, per component: `source` (submodule
commits / `.source`; NGCC: the archive sha256 in `archives.tsv`), `params` (the row's
params.tsv line without `iters`/`kat`/`katn`), `patch` (NGCC: the `patches/<id>/` files its
srcs name, headers in patch dirs its incs name), `build` (the scheme dir's other tracked files;
NGCC: shim template, Makefile, `NGCC_CFLAGS`, `NGCC_CXXFLAGS` for C++ rows) and `harness`
(`harness_common.h` up to its per-operation-limit section, the suite Makefile's `CC`/`CFLAGS`,
`../bench-common/flags.mk`, `HARNESS_EPOCH`). Comments and whitespace do not count (Python: its AST). Excluded on purpose:
`harness.h`, `main.c`, `isolate.h`, the limits and DNF plumbing, `gen_rules.py`. If a change to
an excluded file changes what is timed, bump `HARNESS_EPOCH`. No candidate code is read.

`scripts/aws-bench-split` plans a row only if it needs a run: not in fingerprints.tsv, measured
on another instance type than the planned one, or a component changed (`--stale-report`;
`--all` overrides). `python3 bench-common/fingerprint.py sigs [rev]` prints the fingerprints.
The 2026-10-15 backfill (rows without `# commit:`) used the newest ancestor of HEAD committed
before the run's `# date:` and c7i.4xlarge for the Xeon Platinum 8488C (module doc).

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
  See `schemes/sqisign/Makefile` for the stamp-based pattern, and "Compiler flags" for how the
  global flags reach a CMake (or Meson, or configure-script) build.
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

## Compiler flags

Policy (Thom, 2026-10-15): every benchmarked scheme in `bench/` and `bench-kem/`, NGCC
included, is compiled with **one global flag set**, `BENCH_CFLAGS` in
`../bench-common/flags.mk` (C++: `BENCH_CXXFLAGS`, the same flags with the build's own `-std`):

```
-O3 -march=native -mtune=native -fno-semantic-interposition -DNDEBUG
-fno-stack-protector -U_FORTIFY_SOURCE -fno-stack-clash-protection -fcf-protection=none
```

plus `-fPIC` (every scheme is a shared object). The harness (`main.c`, `loader.c`) uses the
same set without `-fPIC`. Why:

- Ubuntu's GCC enables a stack protector (`-fstack-protector-strong`), `_FORTIFY_SOURCE=3`,
  stack-clash protection and CET (`-fcf-protection`) by default. None of them is part of an
  algorithm, and the defaults differ between distributions, so we turn them off.
  `_FORTIFY_SOURCE` needs `-U_FORTIFY_SOURCE` (GCC's spec only skips its default when the
  command line has `-U_FORTIFY_SOURCE` or `-D_FORTIFY_SOURCE=…`); check the effective state
  with `gcc <flags> -dM -E - </dev/null | grep -E 'FORTIFY|SSP|CET'` on GCC 15 (the
  `scripts/ngcc-run` image, like the VM).
- `-fno-semantic-interposition`: with `-fPIC -shared` GCC otherwise may not inline or
  specialise calls between an upstream's global functions (they could be interposed); RYDE
  signed up to 4x slower without it.
- `-DNDEBUG`: release builds (no `assert`).
- Not used: `-funroll-loops`, LTO, PGO, `-fvisibility=hidden`, upstream `-O2`/`-Ofast` and other
  optimisation flags.

Rules for scheme Makefiles: `include ../../../bench-common/flags.mk`, put `$(BENCH_CFLAGS)
-fPIC` **after** the scheme's own flags in every compile command, and keep only flags a build
needs: `-std`, `-m` ISA flags (so the intrinsics compile; `-march=native` enables them on the
benchmark machine anyway), `-D`/`-I`, warnings. Upstream build systems: CMake (SQIsign, MIKE)
uses a build type upstream does not know (`Bench`), whose `CMAKE_C_FLAGS_BENCH` comes after
upstream's `CMAKE_C_FLAGS`, plus `no_lto.cmake` against upstream's forced LTO; Meson (FAEST) gets
them through `c_args`/`cpp_args`, which Meson puts after its own flags; the djb libraries
(libmceliece, libntruprime, librandombytes) get them appended to every line of their
`compilers/*` lists (`build_libs.sh`). A flag that a scheme needs to build or reproduce its
KATs stays, with the reason in a comment next to it. Exceptions (2026-10-15):

| scheme | kept | why |
|---|---|---|
| SQIsign, MIKE | `-fvisibility=hidden`, `-fno-strict-aliasing` (upstream's `CMAKE_C_FLAGS`) | hidden visibility is needed to link the broadwell `fp_asm.S` (PC-relative reference to a C constant) into a shared object; `-fno-strict-aliasing` is upstream's default (ENABLE_STRICT) build |
| MIKE | `-fno-tree-vectorize -fno-tree-slp-vectorize` on the gf/sat64 target | a target option of upstream's CMakeLists, placed after all configurable flags |
| PERK | `-fwrapv` for djbsort | as upstream builds it |
| libmceliece, libntruprime, librandombytes | `-fwrapv`; `-fvisibility=hidden` | `-fwrapv` is in every upstream compiler line; upstream's build scripts add `-fvisibility=hidden` after the compiler line (library-internal symbols) |
| NTRU+ | `-fvisibility=hidden` for `consts.c` | needed to link the asm's `%rip`-relative table references into a shared object |
| NGCC | `-std=gnu17 -fcommon -Wno-error=…` | see "NGCC candidates" |
| SQIsign, MIKE, and other hand-written `.S` files built by CMake | upstream's assembler flags | C codegen flags do not change hand-written assembly |
| classic, QR-UOV, UOV, SABER, NGCC `-lcrypto` rows | system OpenSSL `libcrypto` | the algorithm (classic) or its hash/DRBG calls run in the distribution's library |
| UOV R1/R2, QR-UOV R2, SNOVA R1 | system OpenSSL `libcrypto` | as above (SNOVA R1: SHAKE and AES, as upstream links them) |
| QR-UOV R1 | a pinned static OpenSSL 3.1.8 (`.source-openssl`), linked with `-Wl,--exclude-libs,ALL`; `-fwrapv` | round 1's `mgf.c` uses OpenSSL 3.1 internals (`KECCAK1600_CTX`, 4-argument `SHA3_squeeze`) and crashes with 3.5; `-fwrapv` as upstream (also QR-UOV R2) |
| SQIsign R1/R2 | system GMP; R2 `-Wno-error` (upstream's ENABLE_STRICT `-Werror` vs a GCC 15 false-positive `-Wmaybe-uninitialized`); R1 `-Wl,--exclude-libs,ALL` | round-1/2 SQIsign needs GMP; round 1's `fp_asm.S` addresses a default-visibility symbol PC-relatively (link-time only, no codegen change) |

`make` does not rebuild objects when only `flags.mk` changed: run `make clean` (the VM always
builds from scratch). A change to `flags.mk` makes every timed row stale (its fingerprint's
`harness` component, see "Provenance and staleness").

## Optimised builds

Benchmark upstream's **fastest x86-64 implementation**: the backend its own build system
selects by default, compiled with our global flags (above). Check the selection against
upstream's Makefile/CMake (e.g. `make -n` in `ref/`), not a liboqs META profile: MQOM's
portable "default" profile was 3-5x slower (see #58).
Backends that need ISA extensions are chosen with `cpu_features.py`, e.g.

```make
HAVE_AVX512 := $(shell python3 ../../cpu_features.py avx512f avx512bw avx512_vnni)
```

Also pass the matching `-m` flags explicitly so the fast path can be compile-tested on
another machine (`make HAVE_AVX512=1`). `BENCH_DISABLE_FEATURES="gfni avx512f"`
forces the fallback. Current choices: MAYO `gfni` (GFNI+AVX-512) else `avx2`; QR-UOV
`IMPL=avx2` (Round 3 dropped the Round 2 avx512 implementation); UOV `PROJ=gfni`
else `avx2`; SQIsign `broadwell`; SNOVA, FAEST,
MQOM, ML-DSA etc. rely on `-march=native` (in the global flags).

## Older on-ramp rounds

Policy (Thom, 2026-10-15): every round-1 and round-2 version of a scheme still in the NIST
on-ramp (one with a round-3 version) is benchmarked from that round's own official code, so
the round views and the scheme pages' version comparison show real round-to-round changes on
one machine. Eliminated schemes keep their reported numbers; no versioned row targets them.

- One dir per round, `schemes/<x>_r1/`, `schemes/<x>_r2/` (SDitH round 1 is `sdith/`; HAWK's
  round-2 package has the round-1 code, so `hawk_r1/` times v1.0 and v1.1, and `hawk/` the
  round-3 "v1.1 (keygen update)"). Each pins exactly the submitted code: an
  upstream tag/commit whose code equals the NIST round package (checked by diff), else the
  package zip itself via `.source` (URL + sha256, fetched into `build/dl`). The Makefile header
  says which, and why. Same flag policy, same "fastest backend the submitters shipped" rule.
- Row names insert the round after the scheme prefix (`MAYO-R2-one`, `QR-UOV-R1-I-(7,740,100,10)`);
  they must be unique across all dirs (`test_bench_importers.py`).
- `BENCH_TO_YAML` entries of these rows name their version: `(file, set, version)`, where
  `version` is the YAML `version:` label or `"label@date"` if the label repeats (MAYO has two
  "Round 2" versions). A list of targets maps one row to several versions with identical code.
  Without a version a row lands on the newest version with that set name. The importer refuses
  targets that do not resolve, and `set_perf_source` relabels a version "our benchmark" once all
  its timed sets have our timings.
- Recorded quirks (details in each Makefile/shim): FAEST R1 256s/256f build a patched copy of
  the submitted faest.c (`schemes/faest_r1/patches/`, `PATCH (nist-sigs-zoo, fix)` header: the
  round-1 AVX2 key-validity check reads w[k-4] instead of w[k-8] for AES-256); with it the
  submitted KATs reproduce. Non-NGCC dirs patch the same way: a copy with the header, never an
  edit of the fetched sources;
  SNOVA R1 is ssk, R2 ESK (each as its YAML version reports); MQOM R1 signatures are
  variable-length (`params.tsv` has the maximum); QR-UOV R1 III-(7,1100,140,10) is built as
  submitted with v=1150 (sig 508; the spec says v=1100, 489).

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
- Compiler flags are global: every candidate builds with `NGCC_CFLAGS` from `ngcc.mk`, the
  global set (`BENCH_CFLAGS`, see "Compiler flags") plus `-fPIC -std=gnu17 -fcommon` and some
  `-Wno-error=`. There are no per-row CFLAGS.
  C++ sources (`.cpp`/`.cc`/`.cxx`, picked up by `*` like `.c`) are compiled with `$(CXX)`
  and `NGCC_CXXFLAGS` (`BENCH_CXXFLAGS -fPIC -std=gnu++23`, which covers the submitters'
  c++20/c++23 trees) into `build/obj/<file>/`; such a `.so` is linked with `-lstdc++`.
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
