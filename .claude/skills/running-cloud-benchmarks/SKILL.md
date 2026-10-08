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
- **Instance:** `c7i.4xlarge` has a virtual PMU. A `*.metal*` type avoids noisy neighbours but costs more, so ask first. If the script warns `fell back to rdtsc`, the cycles are reference cycles: tell Thom before importing.

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
