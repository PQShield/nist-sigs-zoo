---
name: running-cloud-benchmarks
description: Use when signature (bench/) or KEM (bench-kem/) timings need real numbers, NGCC candidates need benchmarking, scripts/aws-bench.sh is involved, or results/*.txt files from a cloud run need importing into data/ YAML.
---

# Running cloud benchmarks

Real timings come only from `scripts/aws-bench.sh` (EC2 spot, `rdpmc`). Container and laptop numbers are smoke tests. Never import third-party numbers (ngcc.dev, papers, spec tables), not even to fill gaps.

## Before launching: ask Thom

Every launch costs money. Show Thom the exact command(s) and wait for a yes. He runs `aws sso login --profile bench` himself.

The VM builds local `HEAD`, and the script refuses to run with uncommitted tracked changes. If those changes are other agents' work, ask Thom. Never commit, stash or revert them to unblock a run.

## Recipes

| Purpose | Command (prefix `AWS_PROFILE=bench`) |
|---|---|
| Smoke test (junk) | `BENCH_ITER=5 scripts/aws-bench.sh --suite kems mlkem` (still builds the whole suite) |
| Full non-NGCC suite | `scripts/aws-bench.sh --suite sigs` |
| NGCC sigs | `scripts/aws-bench.sh --suite sigs --ttl 480 --ngcc-src /tmp/ngcc-src.tar.gz` |
| NGCC KEMs | same with `--suite kems --ttl 360` |

NGCC tarball without KATs (~0.2 GB instead of 5 GB):
```bash
tar -C bench-common/ngcc -czf /tmp/ngcc-src.tar.gz --exclude=Test_Vectors \
    --exclude='*KAT*.txt' --exclude='*.rsp' --exclude='*.req' src
```

- **Smoke-test first** after a harness, script or build change. Then delete those results files (`git status bench*/results`). Never commit or import them.
- **Real NGCC runs:** leave `BENCH_ITER` unset (it overrides the tuned `iters` in `*/schemes/ngcc/params.tsv`) and use no filter.
- **Sigs and KEMs are separate runs.** Never use `both` with NGCC.
- **The TTL counts from boot**: upload, cloud-init and build all come out of it. When the TTL runs out, only partial results come back. Leave margin.
- `BENCH_TIMEOUT` (per-scheme seconds, default 3600) is forwarded to the VM; a scheme that exceeds it becomes a `timeout` DNF.
- **Instance:** the default is `c8i.4xlarge` (check its first results header says `rdpmc`). The NGCC data of 2026-10-13 is from `c7i.4xlarge`: re-run single rows with `--instance-type c7i.4xlarge` so they stay comparable. `c7i.4xlarge` has a virtual PMU. A `*.metal*` type avoids noisy neighbours but costs more, so ask first. If the script warns `fell back to rdtsc`, the cycles are reference cycles: tell Thom before importing.

## Split runs (when Thom wants it fast)

A full NGCC suite takes 5+ hours on one instance and spot instances can be interrupted. Thom accepted splitting it over several instances of the same type (2026-10-13); say "same instance type", not "same machine", in the write-up.
- Positional filters are case-insensitive substrings, OR-ed. Use full row names and check that no name is a substring of another (`NGCC-Aigis-Enc+-I` also matches `-II`, `-III`); duplicates only cost time.
- Pack groups so each times about 15–20 min (each instance also spends ~10 min on setup and building the whole suite). Rows with `iters` 1 or 5 get their own instance. Long filter lists give a hashed results filename.
- The account allows **16 spot instances** (`MaxSpotInstanceCountExceeded`); a refused launch cleans up after itself, so retry it as slots free up. Region: `AWS_REGION=eu-north-1` (the profile sets none).
- Every instance builds and uploads independently; each has its own key pair and security group, so concurrent `aws-bench.sh` calls are safe. Launch them while the tree is clean and **don't commit tracked changes until every launch has passed its clean-tree check**.
- A spot interruption loses the results file, but the rows are in the script's streamed log: rebuild a results file from it (header lines plus rows, add a `# note:` line).
- Merge before importing; reruns go first, because the first non-DNF row per scheme wins:
  ```bash
  python3 bench-common/merge_results.py bench/results/<first-timestamp>_<cpu>_ngcc_sigs_merged.txt <rerun>.txt <parts>.txt...
  ```
  It refuses parts from different CPU models, lists every part in the header and drops stdout noise from candidates.
- A single sign or verify over 10 minutes is a DNF by policy (`BENCH_OP_LIMIT`, default 600 s; reason `sign-or-verify-over-600s`), so TRINE-512-Balanced (about 20 min per signature) is a DNF. Don't raise limits to time such schemes without asking Thom: compute isn't free.

## Review before importing

```bash
grep -h '^# cyclecounter:' <suite>/results/<new>.txt   # expect rdpmc
grep ' DNF ' <suite>/results/<new>.txt
```
- A crash, hang or failed check gives a **DNF row**: name, iters, six `DNF`, then a reason (`SIGABRT`, `timeout`, `valid-signature-rejected`, …). The run continues after it. List every DNF for Thom. Fix crashers in the candidate's wrapper, never in the harness.
- Check for missing schemes (build failure, TTL cut-off) against `params.tsv` and the previous run. On `rc!=0`, read the kept `bench.log`.

## Import

Always name the file. With no argument the importer picks the newest file, which may be junk.
```bash
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv   # NGCC, if sizes changed
uv run bench/update_scheme_data.py bench/results/<file>.txt          # + data/benchmark_env.yaml
uv run bench-kem/update_scheme_data.py bench-kem/results/<file>.txt  # + data/kem_benchmark_env.yaml
```
The importer never imports timings from a DNF row. On NGCC sets it clears the old timings and adds the note "Benchmark did not finish (…)". On other schemes it warns and keeps the old timings. Read its "not in this run" list.

Then:
1. Add an `update` entry to `data/history.yaml`.
2. Run `npm run test && npm run check` and `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/tests`.
3. On a feature branch, stage the results file, YAML, env yaml and history by name.

## After every run, especially an interrupted one

```bash
for r in instances security-groups key-pairs; do AWS_PROFILE=bench aws ec2 describe-$r \
  --filters 'Name=tag:Name,Values=nist-sigs-zoo-bench-*' --output text; done
```
Report leftovers to Thom. Terminated instances stay listed for about an hour.
