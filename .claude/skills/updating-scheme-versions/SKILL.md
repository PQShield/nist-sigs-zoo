---
name: updating-scheme-versions
description: Use when a signature or KEM scheme (NIST on-ramp, NIST PQC, KpqC, ISO, …) publishes a new specification, round submission, parameter change or new implementation release, or when a new attack or implementation bug affects a scheme already on the site.
---

# Updating scheme versions

A spec update touches `data/`, `bench*/schemes/<name>/`, `bench*/update_scheme_data.py` and `data/history.yaml`. A missed step fails silently. Schemas are in `CLAUDE.md`, `bench/CLAUDE.md` and `bench-kem/CLAUDE.md`. Precedent: `c2c537e`/`259f12c` (QR-UOV R3), `4ea3182` (SNOVA 3.0), `ff1d567`/`699226e` (MIKE).

## Data

- **Signatures:** add the new version at the **top** of `versions:`, because the importer writes timings into the *first* version that has a matching parameter-set name. Leave old versions alone, and keep their flags as a historical record.
- Quote `date: 'YYYY-MM-DD'` and set it to the spec's publication date.
- `programmes:` goes only on a stage submission (`nist-onramp: round-3`; stage ids are in `data/programmes.yaml`). A post-round update has **no** `nist-onramp` entry; adding one breaks the round views and the counts in `dataFiles.test.ts`. Never use `tags:`.
- **KEMs** have no versions: update in place and record the old→new values in the history entry.
- NGCC YAML is generated: edit `data/ngcc/candidates.yaml` instead.
- Spec numbers need `perf_source: "submission document (Table N, CPU, impl)"`. Delete it once our benchmark replaces them.
- **Never** use third-party timings (ngcc.dev, other papers, liboqs). Leave them empty until we have measured them.

## Flags and credit

- `broken`: attack below the level threshold. `warning`: partial or unquantified attack. `info`: context. `implementation`: bug in submitted code (never sets broken). `findings_url`: link to the source.
- Set flags per version or override them per parameter set. Bit counts go in the parameter set's `notes`.
- Credit the **original authors** and link the primary source (ePrint or paper). An aggregator is not an author. Ask Thom before adding NGCC findings.

## History

Add a new top entry to `data/history.yaml` with a quoted `date`, a `type` (`update`|`attack`|`milestone`|`standardization`) and `schemes: [<exact name:>]`. The HTML `description` holds the spec and repo links, what changed (with numbers), the authors of any new implementation, and how we build and benchmark it.

## Benchmark

1. Bump `schemes/<name>/ref` to the release tag (replace vendored code with a submodule). Update `params.tsv` (sizes, names) and the Makefile. Build upstream's fastest default x86-64 path (`make -n` in `ref/`).
2. Run upstream's KAT check if it has one (e.g. `make check-kat`). Then run `make -C bench && ./bench --sizes <f> && ./bench <f>`. The params.tsv sizes must match the spec and the YAML, and there must be no DNF.
3. If sets were renamed, update `BENCH_TO_YAML`. Otherwise the import skips them silently (`7184c66`).
4. Commit: `scripts/aws-bench.sh` builds `HEAD`. **Ask Thom before spending on AWS.** He runs `aws sso login --profile bench`. Then run `AWS_PROFILE=bench scripts/aws-bench.sh --suite sigs|kems <f>`.
5. Check the results file uses `rdpmc` and has no DNF. Run `uv run bench/update_scheme_data.py` (or the `bench-kem/` one) and read the unmapped list. Delete `perf_source` and commit "Import bench results from …" separately. Never commit `BENCH_ITER=5` smoke runs.

## Verify

```bash
npm run check && npm run test && npm run test:e2e
uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/tests  # importer/harness changed
```

## Common mistakes

| Mistake | Effect |
|---|---|
| New version appended last | Timings land on the old version |
| `nist-onramp` on a post-round update | Round views and counts break |
| Submodule bumped, `params.tsv` not | Overflow or wrong sizes |
| `perf_source` kept after import | Comparison mislabels our numbers |
| Attack credited to an aggregator | Wrong attribution |
