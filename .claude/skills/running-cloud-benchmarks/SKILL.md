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
| Real runs (only the rows that need one) | `scripts/aws-bench-split --suite sigs\|kems [--ngcc-src /tmp/ngcc-src.tar.gz]`, see "Normal flow" |

NGCC tarball without KATs (~0.2 GB instead of 5 GB):
```bash
tar -C bench-common/ngcc -czf /tmp/ngcc-src.tar.gz --exclude=Test_Vectors \
    --exclude='*KAT*.txt' --exclude='*.rsp' --exclude='*.req' src
```

- **Smoke-test first** after a harness, script or build change. Then delete those results files (`git status bench*/results`). Never commit or import them.
- **Real runs:** leave `BENCH_ITER` unset (it overrides the tuned `iters`, e.g. in `*/schemes/ngcc/params.tsv`).
- **Sigs and KEMs are separate runs.** Never use `both` with NGCC.
- **The TTL counts from boot**: upload, cloud-init and build all come out of it. When the TTL runs out, only partial results come back. Leave margin.
- `BENCH_TIMEOUT` (per-scheme seconds, default 3600) is forwarded to the VM; a scheme that exceeds it becomes a `timeout` DNF.
- **Instance:** one instance type per dataset, so rows stay comparable. Each results header records `# cpu:`, `# commit:` and `# instance_type:`; every imported timing names its run (`bench_env`) in `data/benchmark_env.yaml` / `data/kem_benchmark_env.yaml`, which list one environment per run. A different instance type than the recorded one makes every row stale ("hardware: …"), so the first run on a new type re-runs the whole suite: ask Thom. On the first run on a new type, check its header says `# cyclecounter: rdpmc`. A `*.metal*` type avoids noisy neighbours but costs more, so ask first. If the script warns `fell back to rdtsc`, the cycles are reference cycles: tell Thom before importing.

## Normal flow: run only what changed

Thom (2026-10-15): we don't re-benchmark rows whose results are current. `scripts/aws-bench-split` plans only rows that need a run: no imported result ("new row", "no timing"), measured on another instance type ("hardware: c7i.4xlarge → c8i.4xlarge"), or an input of their timing changed since that run (source, params row, patch, build, harness; see `bench/CLAUDE.md`, "Provenance and staleness"). Use it for every cloud run, also for a few rows, not hand-made filter lists. Spreading rows over several instances of the same type is accepted (2026-10-13); say "same instance type", not "same machine", in the write-up.

1. **Why (free):** `python3 scripts/aws-bench-split --suite sigs --stale-report [filter...]` lists the rows that need a run, NGCC included, with their reasons. Check that they make sense (a harness edit re-stales everything).
2. **Plan (free):** `python3 scripts/aws-bench-split --suite sigs [--ngcc-src /tmp/ngcc-src.tar.gz] --dry-run [filter...]`. Filters are intersected with the rows that need a run. It prints each group with per-row estimates and reasons, `--ttl`, warnings, and the exact `aws-bench.sh` commands. Without `--ngcc-src` it says how many NGCC rows it leaves out. Show Thom the report and the plan (rows, group count, longest group, total minutes) and **wait for his go**. `--all` plans every row: only when Thom asks.
3. **Run:** the same command without `--dry-run`, prefixed `AWS_PROFILE=bench AWS_REGION=eu-north-1` (the profile sets no region). Run it in the background and read its status table. Logs go to `$TMPDIR/aws-bench-split-<time>/group-NN.log`.
4. **Merge:** happens at the end, into `<suite>/results/<first-timestamp>_<cpu>_split_merged.txt`. Pass `--rerun-first FILE...` for re-runs that must win over the parts. Report the printed DNF rows and missing rows, and the leftover-resource check, to Thom. Review and import only the merged file (keep the part files: the importer reads each row's commit from its part).

What it does for you, so don't redo it by hand:
- **Rows and estimates:** rows come from `<suite>/schemes/*/*params*.tsv` (NGCC only with `--ngcc-src`). Each row's time comes from the newest results file that has it (merged files first), or failing that from its `iters`. Rows of 10+ minutes get their own instance; the rest are packed into groups of `--bin-minutes` (default 16). Each instance also spends about 10 minutes on setup and building. Rows whose names contain one another (`NGCC-Aigis-Enc+-I` / `-II`) are kept in one group, because filters are substring matches.
- **Quota:** at most `--max-instances` (default 16, the account's spot limit) run at once. Launches refused with `MaxSpotInstanceCountExceeded` or `InsufficientInstanceCapacity` are retried.
- **Clean tree:** each instance builds local `HEAD`. The script stops launching if the tree gets dirty or `HEAD` moves. Still, **don't commit tracked changes while groups are launching**.
- **Ctrl-C:** it forwards the signal to every `aws-bench.sh` and its ssh children, waits for them to terminate their instances, and still merges what came back. A second Ctrl-C does not abort that cleanup.
- **Spot interruption:** that group's rows are rebuilt from its streamed log into a `*_recovered.txt` part with a `# note:`. Re-run the missing rows (it prints them), then merge: `python3 bench-common/merge_results.py OUT <rerun>.txt <split_merged>.txt` (reruns first: the first non-DNF row per scheme wins).
- A single sign or verify (KEMs: encaps or decaps) over 10 minutes is a DNF by policy (`BENCH_OP_LIMIT`, default 600 s; reason `operation-over-600s`), so TRINE-512-Balanced (about 20 min per signature) is a DNF. Don't raise limits to time such schemes without asking Thom: compute isn't free.
- **Exemptions:** rows named in `scripts/aws-bench-exemptions.txt` (`--exempt-file`; now the SLH-DSA SP 800-230 `*-24` sets, Thom 2026-10-14) are exempt from `BENCH_OP_LIMIT` and `BENCH_TIMEOUT`: the split launcher runs each alone with both set to 0. Each line is `NAME<TAB>minutes`: the estimated instance run (boot and build included) that sets its TTL (estimate + the usual 25 min; measured results win); the dry-run marks them `EXEMPT`. Adding a row, or changing an estimate, needs Thom's decision.

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
uv run bench/update_scheme_data.py bench/results/<file>.txt          # + benchmark_env.yaml, fingerprints.tsv
uv run bench-kem/update_scheme_data.py bench-kem/results/<file>.txt  # + kem_benchmark_env.yaml, fingerprints.tsv
```
The importer sets each timed row's `bench_env`, adds the run's environment (dropping unused ones) and records each row's fingerprint in `<suite>/results/fingerprints.tsv` at the commit the run built. Read its WARNINGs (a commit not in the repo, a row not in params.tsv there).
The importer never imports timings from a DNF row. On NGCC sets it clears the old timings and adds the note "Benchmark did not finish (…)". On other schemes it warns and keeps the old timings. Read its "not in this run" list.
CI (`bench-common/tests/test_benchmark_coverage.py`) fails until every new or mapped row is benchmarked and imported, or its set is marked `dnf:` / `no_code:`.

Then:
1. Add an `update` entry to `data/history.yaml`.
2. Run `npm run test && npm run check` and `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/tests`.
3. On a feature branch, stage the results files (merged and parts), YAML, env yaml, fingerprints.tsv and history by name.

## After every run, especially an interrupted one

```bash
for r in instances security-groups key-pairs; do AWS_PROFILE=bench aws ec2 describe-$r \
  --filters 'Name=tag:Name,Values=nist-sigs-zoo-bench-*' --output text; done
```
Report leftovers to Thom. Terminated instances stay listed for about an hour.
