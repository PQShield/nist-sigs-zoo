# Site Reorganization: China NGCC Round-1 Import Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the 34 signature and 41 KEM candidates of China's NGCC round 1 to the site. Parameter sets and sizes come from our own builds of the official submission packages. Claimed targets come from our own reading of each specification. Timings come from our own `bench/` and `bench-kem/` harnesses.

**Architecture:** Every NGCC candidate implements one common ICCS C API (`SIG_AlgorithmInstance.h` / `KEM_AlgorithmInstance.h`, a global DRNG `drng_algorithm`, size getters). So one generic shim template per suite adapts *every* instance to the existing bench contract, and integrating ~390 instances is data: a `params.tsv` manifest per suite that `bench-common/ngcc/gen_rules.py` turns into make rules. Several pieces are shared by both suites under `bench-common/ngcc/`:
- archive fetch/extract;
- a best-effort build (`ngcc.mk`) in which one failing instance never breaks the suite;
- our own KAT replayer (`ngcc_kat.c`);
- a status recorder (build / KAT / hash usage).

The harnesses gain `--sizes`, which prints getter sizes as TSV. `update_scheme_data.py` (both suites) gains `--ngcc-sizes`, which joins those sizes, the status file and a hand-curated claims table (`data/ngcc/candidates.yaml`) into `data/schemes/*.yaml` / `data/kems/*.yaml`. Benchmarks then flow through the existing results → import path.

**Tech Stack:** C (gcc, GNU make, dlopen), Python 3 stdlib + `ruamel.yaml` via `uv` (pytest via `uv run --with pytest`), `ruff` via `uvx`, SvelteKit/Vitest/Playwright for the site, `scripts/aws-bench.sh` for cloud runs.

**Spec:** The controller brief for plan 3 (Thom's binding requirements, 2026-10-07), the plan-1 roadmap entry "China NGCC data import" in `docs/superpowers/plans/2026-10-07-site-reorganization-foundation.md`, and the staged research in `.superpowers/spec/ngcc/` (git-ignored: `downloads.csv` = official archive URLs; `data/{sign,kem}.csv` = official names/submitters; `doc/perf-x86.txt` = ICCS x86 guide; `harness-tree.txt`; the `ngcc_*` files and `data/parameters.csv` are **cross-check only**). The public ICCS API can be inspected with `gh api repos/ngcc-dev/ngcc-harness/contents/api/API_PKC/Implementations/Reference_Implementation/AlgorithmInstance/<file> --jq .content | base64 -d`. Read it for understanding only and never vendor it.

## Global Constraints

- Everything stays on branch `feat/programmes-foundation`. Never switch branches. Small focused commits, one or more per task. Stage specific files (never `git add -A` / `git add .`). End every commit message with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- **Never import third-party benchmark numbers.** ngcc.dev / ngcc-harness timings are never used, not even as iteration estimates. Timings come only from `bench/` and `bench-kem/`.
- **Sizes and parameter sets come only from our own extraction**, through the ICCS getters of our builds or, for instances we cannot build, from the official spec with a page/table reference. `.superpowers/spec/ngcc/data/parameters.csv`, `ngcc_*_instances.csv` and `ngcc_candidates.json` are cross-check only. A disagreement is investigated, never resolved by copying their value.
- **Never vendor ngcc-harness code or data.** Our adapter, KAT replayer and fixtures are our own code. Restating the ICCS API prototypes and the KAT seed schedule is necessary for interoperability and is allowed.
- **Findings are gated.** No `findings_url`, `broken` or `implementation` text for NGCC candidates is written until Thom confirms that attribution with Markku-Juhani Saarinen is agreed (Task 14). Summaries are in our own words with credit, and report text is never pasted.
- Archives are untrusted input. Extraction uses an allowlist (sources, headers, KAT files, licences, PDFs) and rejects traversal paths and links. Nothing from an archive is executed: candidates' Makefiles, CMake files, scripts and binaries are never run. Run our tools from the repo root, never with the cwd inside `bench-common/ngcc/src/` or `specs/`. Fixes to candidate code are patched copies under `<suite>/schemes/ngcc/patches/<id>/`, never edits to extracted files.
- The official archives (several GB) are kept outside the repo in `$NGCC_CACHE` (default `~/.cache/nist-sigs-zoo/ngcc`). The repo pins their URLs, sizes and sha256 (`bench-common/ngcc/archives.tsv`). Extracted sources and specs are git-ignored.
- NGCC parameter sets use `ngcc_level` (128/256/384/512 classical bits), never a NIST `level`, and are never converted.
- NGCC bench output names are `NGCC-<ALGORITHM_INSTANCE>`, and the site parameter-set name is the bare `ALGORITHM_INSTANCE`.
- Python: `uv` for running, `uvx ruff check --fix` + `uvx ruff format` on new Python files. On the two existing `update_scheme_data.py` scripts run only `uvx ruff check` (they are not ruff-formatted today, and formatting them would bury the diff).
- Python unit tests: `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests`.
- Old-URL compatibility is out of scope. Do not modify `round-1/`.

## Review Focus

1. **The server re-uploads an archive with different bytes:** `fetch.py` must refuse it, move it aside and keep it out of extraction. It must not silently re-pin. Pinned by Task 1 `test_changed_archive_is_refused_and_moved_aside`.
2. **An archive member named `../x.c`, an absolute path, a symlink, or a GBK-encoded Chinese name without the UTF-8 flag:** nothing may be written outside `src/<id>/`, and names must decode to real Chinese. Pinned by Task 2 `test_rejects_traversal_and_links` and `test_decodes_gbk_names`.
3. **One candidate that does not compile:** the rest of NGCC, and every existing NIST/KpqC scheme, must still build and benchmark, and the failure must be recorded. Pinned by Task 4 `test_failed_instance_does_not_fail_the_build`, and for KEMs by Task 5.
4. **An implementation that reports a longer key or signature than its getter claims:** the shim must return an error, so the harness prints `FAIL` rather than reading past the buffer. The KAT replay must report `CRYPTOFAIL`. Pinned by Task 4 `test_overlong_key_is_an_error` and `test_status_records_build_kat_and_hash`.
5. **Re-running the size import after timings exist** (for example after switching to an optimised implementation): timings must survive, and the importer must refuse to overwrite a non-NGCC file with the same name. Pinned by Task 9 `test_reimport_keeps_timings` and `test_refuses_to_overwrite_other_schemes`.

## Execution notes (network, long runtimes, Thom's decisions)

| Task | Needs | Notes |
|---|---|---|
| 1 | Network (www.niccs.org.cn), ~3.5 GB+, possibly hours | First fetch records sha256 (trust on first use). Thom may prefer to run it himself. |
| 2 | Local disk, minutes | Extracts sources + specs; DOVE (sign-09) needs `bsdtar` (`apt install libarchive-tools`) for its inner RARs |
| 6, 7 | Hours of curation, time-boxed per candidate | Builds ~200 + ~190 instances; KAT replay with 900 s timeout per instance |
| 8 | Hours of reading 75 spec PDFs | **Thom reviews** `data/ngcc/candidates.yaml` before Task 9 commits imported data |
| 11 | Thom's decision | Benchmark the optimised x86 implementations where they pass the KATs (bench policy), or reference only |
| 12–13 | Thom's AWS SSO login + spend approval; 2 × up to 6 h spot runs | Thom runs `aws sso login`; confirm before launching |
| 14 | **Blocked** until Thom confirms attribution with Saarinen | Separate commit for Thom's review |

Task 8 can start right after Task 2, in parallel with Tasks 6–7, and is finalised once their instance names are fixed. The site gets size data at Task 9, before any benchmark runs (Tasks 12–13).

## File Structure

| Path | Responsibility |
|---|---|
| `bench-common/ngcc/archives.tsv` (create) | Pinned official archive URL, page, bytes, sha256 per candidate (75 rows) |
| `bench-common/ngcc/fetch.py` (create) | Download to `$NGCC_CACHE`, record digests on first fetch, verify afterwards |
| `bench-common/ngcc/extract.py` (create) | Allowlisted, traversal-safe extraction (nested zip, GBK names, RAR via bsdtar) into `src/` + `specs/` |
| `bench-common/ngcc/gen_rules.py` (create) | `params.tsv` → `build/rules.mk`; source resolution rules |
| `bench-common/ngcc/inventory.py` (create) | Draft `params.tsv` rows for every ICCS instance found in an extracted candidate |
| `bench-common/ngcc/ngcc_kat.c` (create) | Our KAT replayer: ICCS seed schedule, dlopen of an instance `.so` |
| `bench-common/ngcc/kat_compare.py` (create) | Canonical comparison with submitted KAT files (CRLF, template-mode lengths) |
| `bench-common/ngcc/hash_usage.py` (create) | Classify an instance as `placeholder` / `sm3` / `own` hash user |
| `bench-common/ngcc/status.py` (create) | Per-instance build / KAT / hash / impl status → `status.tsv` |
| `bench-common/ngcc/ngcc.mk` (create) | Shared best-effort make logic for both suites |
| `bench-common/ngcc/ngcc_data.py` (create) | Sizes + status + claims → YAML documents (used by both `update_scheme_data.py`) |
| `bench-common/ngcc/.gitignore` (create) | Ignore `/src/`, `/specs/` |
| `bench-common/ngcc/tests/**` (create) | pytest suite + toy ICCS sig/KEM fixtures |
| `bench/schemes/ngcc/{Makefile,shim_template.c,params.tsv,status.tsv,.source,.gitignore,patches/}` (create) | Signature suite: generic ICCS SIG shim + manifest + recorded status |
| `bench-kem/schemes/ngcc/{…same…}` (create) | KEM suite equivalent |
| `bench/Makefile`, `bench-kem/Makefile` (modify) | NGCC stamp; append `build/built.list` to `so_paths.h`; clean |
| `bench/main.c`, `bench-kem/main.c` (modify) | `--sizes` TSV mode |
| `bench/results/ngcc_sizes.tsv`, `bench-kem/results/ngcc_sizes.tsv` (create) | Sizes emitted by our harness, committed |
| `bench/update_scheme_data.py`, `bench-kem/update_scheme_data.py` (modify) | `--ngcc-sizes`; NGCC bench-name mapping; env `sources` merge |
| `data/ngcc/candidates.yaml` (create) | Curated claims: family, assumption, official page, spec ref, per-instance `ngcc_level` / `recommended` / `omit` |
| `data/schemes/*.yaml` (34 new), `data/kems/*.yaml` (41 new) | Generated NGCC entries |
| `src/lib/__tests__/ngccCandidates.test.ts` (create), `src/lib/__tests__/dataFiles.test.ts` (modify) | Claims validation; NGCC counts |
| `e2e/{signatures,kems,home}.spec.ts` (modify) | NGCC rows visible, "128-bit", purple |
| `data/history.yaml`, `CLAUDE.md`, `bench/CLAUDE.md`, `bench-kem/CLAUDE.md` (modify) | Milestone + update entries; docs |
| `scripts/aws-bench.sh` (modify) | `--ngcc-src TARBALL` upload of extracted sources |

---

### Task 1: Pin and fetch the official round-1 archives

**Files:**
- Create: `bench-common/ngcc/archives.tsv`, `bench-common/ngcc/fetch.py`, `bench-common/ngcc/.gitignore`
- Test: `bench-common/ngcc/tests/conftest.py`, `bench-common/ngcc/tests/test_fetch.py`

**Interfaces:**
- Produces: `archives.tsv` columns `id kind name zip url page bytes sha256` (`kind` ∈ `sig`/`kem`). `fetch.py` exports `cache_dir() -> Path`, `read_archives(path=ARCHIVES) -> tuple[list[str], list[dict]]` (comment lines, rows), `write_archives(comments, rows, path=ARCHIVES)`, `sha256_file(path) -> str`, `fetch_one(row, cache, record) -> str` (`"recorded"|"downloaded"|"cached"`), `class ArchiveError(Exception)`.

**Why outside the repo:** the 75 archives total well over 3.5 GB (sign-32 alone is ~1.3 GB, mostly KAT text). Their redistribution terms are unclear, since they contain IP statements and papers. Only a few MB of sources are needed. Pinning URL + bytes + sha256 makes the fetch reproducible without redistributing anything.

- [ ] **Step 1: Write the failing tests**

`bench-common/ngcc/tests/conftest.py`:
```python
import sys
from pathlib import Path

# The tools live one directory up and import each other as plain modules.
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
```

`bench-common/ngcc/tests/test_fetch.py`:
```python
import hashlib

import pytest

from fetch import ArchiveError, fetch_one


def row_for(path):
    return {"id": "sign-01", "kind": "sig", "name": "X", "zip": "x.zip", "url": path.as_uri(),
            "page": "", "bytes": "", "sha256": ""}


def test_record_pins_size_and_digest(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(b"archive")
    row, cache = row_for(src), tmp_path / "cache"
    assert fetch_one(row, cache, record=True) == "recorded"
    assert row["bytes"] == "7"
    assert row["sha256"] == hashlib.sha256(b"archive").hexdigest()
    assert fetch_one(row, cache, record=False) == "cached"


def test_unpinned_archive_needs_record(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(b"archive")
    with pytest.raises(ArchiveError, match="--record"):
        fetch_one(row_for(src), tmp_path / "cache", record=False)


def test_changed_archive_is_refused_and_moved_aside(tmp_path):
    src = tmp_path / "x.zip"
    src.write_bytes(b"archive")
    row, cache = row_for(src), tmp_path / "cache"
    fetch_one(row, cache, record=True)
    (cache / "sign-01.zip").write_bytes(b"tampered")
    with pytest.raises(ArchiveError, match="differs"):
        fetch_one(row, cache, record=True)  # --record never re-pins a pinned archive
    assert not (cache / "sign-01.zip").exists()
    assert (cache / "sign-01.zip.mismatch").exists()
```

- [ ] **Step 2: Run them to verify they fail**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_fetch.py`
Expected: FAIL with `ModuleNotFoundError: No module named 'fetch'`.

- [ ] **Step 3: Write `fetch.py`**

```python
#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# ///
"""Download the official NGCC round-1 submission archives (signatures and KEMs).

The archives are not stored in the repository: together they are several GB,
mostly test vectors and documents, and their redistribution terms are unclear.
They go to $NGCC_CACHE (default ~/.cache/nist-sigs-zoo/ngcc/<id>.zip).

archives.tsv pins each URL. The first download records bytes and sha256
(--record); every later run verifies them and refuses a changed archive.

Usage:
    bench-common/ngcc/fetch.py --record          # first fetch: download + pin digests
    bench-common/ngcc/fetch.py                   # verify the cache, download what is missing
    bench-common/ngcc/fetch.py sign-01 kem-05    # only these ids
"""

import argparse
import csv
import hashlib
import os
import sys
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
ARCHIVES = HERE / "archives.tsv"
COLUMNS = ["id", "kind", "name", "zip", "url", "page", "bytes", "sha256"]
USER_AGENT = "nist-sigs-zoo-ngcc-fetch/1 (+https://github.com/PQShield/nist-sigs-zoo)"


class ArchiveError(Exception):
    pass


def cache_dir() -> Path:
    default = Path.home() / ".cache" / "nist-sigs-zoo" / "ngcc"
    return Path(os.environ.get("NGCC_CACHE", default))


def read_archives(path: Path = ARCHIVES) -> tuple[list[str], list[dict]]:
    with open(path, newline="", encoding="utf-8") as f:
        lines = f.readlines()
    comments = [ln for ln in lines if ln.startswith("#")]
    body = [ln for ln in lines if not ln.startswith("#")]
    return comments, list(csv.DictReader(body, delimiter="\t"))


def write_archives(comments: list[str], rows: list[dict], path: Path = ARCHIVES) -> None:
    with open(path, "w", newline="", encoding="utf-8") as f:
        f.writelines(comments)
        writer = csv.DictWriter(f, fieldnames=COLUMNS, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    part = dest.with_name(dest.name + ".part")
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=300) as resp, open(part, "wb") as out:
        while chunk := resp.read(1 << 20):
            out.write(chunk)
    part.replace(dest)


def fetch_one(row: dict, cache: Path, record: bool) -> str:
    """Make sure cache/<id>.zip exists and matches its pin; with record, pin an unpinned row."""
    dest = cache / f"{row['id']}.zip"
    fetched = not dest.exists()
    if fetched:
        download(row["url"], dest)
    digest, size = sha256_file(dest), str(dest.stat().st_size)
    if not row["sha256"]:
        if not record:
            raise ArchiveError("no pinned sha256; run fetch.py --record to pin it")
        row["sha256"], row["bytes"] = digest, size
        return "recorded"
    if digest != row["sha256"] or size != row["bytes"]:
        dest.replace(dest.with_name(dest.name + ".mismatch"))
        raise ArchiveError(f"archive differs from its pin (moved aside as {dest.name}.mismatch)")
    return "downloaded" if fetched else "cached"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Fetch and verify the NGCC round-1 archives.")
    ap.add_argument("ids", nargs="*", help="candidate ids (default: all)")
    ap.add_argument("--record", action="store_true", help="pin bytes/sha256 of archives that have none")
    args = ap.parse_args(argv)

    comments, rows = read_archives()
    known = {r["id"] for r in rows}
    wanted = set(args.ids) or known
    if unknown := sorted(wanted - known):
        print(f"unknown ids: {', '.join(unknown)}", file=sys.stderr)
        return 2

    cache, errors, total = cache_dir(), [], 0
    for row in rows:
        if row["id"] not in wanted:
            continue
        try:
            state = fetch_one(row, cache, args.record)
            total += int(row["bytes"])
            print(f"{row['id']:8} {state:10} {int(row['bytes']) / 1e6:9.1f} MB  {row['zip']}")
        except (ArchiveError, OSError) as e:
            errors.append(f"{row['id']}: {e}")
    if args.record:
        write_archives(comments, rows)
    print(f"cache {cache}: {total / 1e9:.2f} GB verified")
    for e in errors:
        print(f"ERROR {e}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
```

`bench-common/ngcc/.gitignore`:
```
# Extracted from the official archives (see extract.py); never committed.
/src/
/specs/
__pycache__/
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_fetch.py`
Expected: `3 passed`.

- [ ] **Step 5: Generate `archives.tsv` from the staged official URL list**

```bash
python3 - <<'EOF'
import csv
from pathlib import Path
rows = [r for r in csv.DictReader(open(".superpowers/spec/ngcc/downloads.csv", newline="", encoding="utf-8"), delimiter=";")
        if r["Category"] in ("sign", "kem")]
assert len(rows) == 75, len(rows)
with open("bench-common/ngcc/archives.tsv", "w", newline="", encoding="utf-8") as f:
    f.write("# Official China NGCC round-1 submission archives (signatures + KEMs), www.niccs.org.cn.\n")
    f.write("# bytes/sha256 are pinned by fetch.py --record on first download and verified afterwards.\n")
    w = csv.writer(f, delimiter="\t", lineterminator="\n")
    w.writerow(["id", "kind", "name", "zip", "url", "page", "bytes", "sha256"])
    for r in rows:
        w.writerow([r["ID"], "sig" if r["Category"] == "sign" else "kem", r["Algorithm"], r["ZipFile"],
                    r["DownloadURL"], r["PageURL"], "", ""])
EOF
grep -c $'^sign-' bench-common/ngcc/archives.tsv; grep -c $'^kem-' bench-common/ngcc/archives.tsv
```
Expected: `34` then `41`.

- [ ] **Step 6: Lint, then commit the tool (before the long fetch)**

```bash
chmod +x bench-common/ngcc/fetch.py
uvx ruff check --fix bench-common/ngcc && uvx ruff format bench-common/ngcc
git add bench-common/ngcc/fetch.py bench-common/ngcc/archives.tsv bench-common/ngcc/.gitignore bench-common/ngcc/tests/conftest.py bench-common/ngcc/tests/test_fetch.py
git commit -m "Add pinned fetch of the NGCC round-1 submission archives

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 7 (NETWORK, LONG; ask Thom whether he wants to run it): fetch and pin**

Run: `uv run bench-common/ngcc/fetch.py --record` (re-run until no `ERROR` lines; already-cached archives are verified, not re-downloaded).
Expected: 75 rows `recorded`/`cached` and a final line `cache …: ≥3.5 GB verified`. Optionally compare the pinned sha256 values with the "SOURCE_ARCHIVES.md" table of ngcc-harness, as a cross-check only: `gh api repos/ngcc-dev/ngcc-harness/contents/SOURCE_ARCHIVES.md --jq .content | base64 -d`. Report any mismatch to Thom and never copy their values.

- [ ] **Step 8: Commit the pins**

```bash
git add bench-common/ngcc/archives.tsv
git commit -m "Pin sizes and sha256 of the 75 NGCC round-1 archives

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 2: Safe extraction of sources, KAT files and specs

**Files:**
- Create: `bench-common/ngcc/extract.py`
- Test: `bench-common/ngcc/tests/test_extract.py`

**Interfaces:**
- Consumes: `fetch.cache_dir`, `fetch.read_archives`, `fetch.sha256_file`.
- Produces: `extract_archive(zip_path, cand_id, src_root=SRC_OUT, spec_root=SPEC_OUT) -> Extractor` (`.counts{"src","spec"}`, `.skipped: list[str]`), `decode_name(ZipInfo) -> str`, `safe_relpath(str) -> PurePosixPath | None`. The on-disk layout is `bench-common/ngcc/src/<id>/<archive-relative path>`, `bench-common/ngcc/specs/<id>/…pdf`, plus the marker `bench-common/ngcc/src/.extracted`.

- [ ] **Step 1: Write the failing tests**

`bench-common/ngcc/tests/test_extract.py`:
```python
import io
import zipfile

from extract import decode_name, extract_archive


def make_zip(path, members):
    """members: {name or ZipInfo: bytes}. zipfile stores names like '../x.c' verbatim."""
    with zipfile.ZipFile(path, "w") as zf:
        for name, data in members.items():
            zf.writestr(name, data)


def files_under(root):
    return sorted(p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file())


def test_keeps_sources_kats_licences_and_pdfs_only(tmp_path):
    z = tmp_path / "a.zip"
    make_zip(z, {
        "Cand/Ref/X/a.c": b"int a;", "Cand/Ref/X/a.h": b"", "Cand/Ref/X/b.S": b"",
        "Cand/KAT/KAT_SIG_X.txt": b"Count = 0\n", "Cand/doc/Algorithm specifications.pdf": b"%PDF",
        "Cand/Ref/X/a.o": b"\x7fELF", "Cand/Ref/X/Makefile": b"all:", "Cand/run.sh": b"rm -rf /", "Cand/LICENSE": b"MIT",
    })
    extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert files_under(tmp_path / "src") == [
        "sign-01/Cand/KAT/KAT_SIG_X.txt", "sign-01/Cand/LICENSE",
        "sign-01/Cand/Ref/X/a.c", "sign-01/Cand/Ref/X/a.h", "sign-01/Cand/Ref/X/b.S",
    ]
    assert files_under(tmp_path / "specs") == ["sign-01/Cand/doc/Algorithm specifications.pdf"]


def test_rejects_traversal_and_links(tmp_path):
    link = zipfile.ZipInfo("Cand/link.c")
    link.external_attr = 0o120777 << 16
    z = tmp_path / "a.zip"
    make_zip(z, {"../evil.c": b"x", "Cand/../../evil2.c": b"x", link: b"/etc/passwd"})
    ex = extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert not (tmp_path / "evil.c").exists() and not (tmp_path / "evil2.c").exists()
    assert files_under(tmp_path / "src") == []
    assert sum(s.startswith("unsafe path") for s in ex.skipped) == 2


def test_unpacks_nested_zip(tmp_path):
    inner = io.BytesIO()
    with zipfile.ZipFile(inner, "w") as zf:
        zf.writestr("Impl/b.c", b"int b;")
    z = tmp_path / "a.zip"
    make_zip(z, {"Cand/Implementations.zip": inner.getvalue()})
    extract_archive(z, "kem-01", tmp_path / "src", tmp_path / "specs")
    assert (tmp_path / "src/kem-01/Cand/Implementations/Impl/b.c").read_bytes() == b"int b;"


def test_decodes_gbk_names():
    name = "参考实现/a.c"
    info = zipfile.ZipInfo(name.encode("gbk").decode("cp437"))
    info.flag_bits = 0
    assert decode_name(info) == name


def test_replaces_a_previous_extraction(tmp_path):
    stale = tmp_path / "src/sign-01/old.c"
    stale.parent.mkdir(parents=True)
    stale.write_text("stale")
    z = tmp_path / "a.zip"
    make_zip(z, {"new.c": b""})
    extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert files_under(tmp_path / "src") == ["sign-01/new.c"]
```

- [ ] **Step 2: Run them to verify they fail**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_extract.py`
Expected: FAIL with `ModuleNotFoundError: No module named 'extract'`.

- [ ] **Step 3: Write `extract.py`**

```python
#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# ///
"""Extract sources, KAT files and specification PDFs from the cached NGCC archives.

The archives are untrusted input. Members are filtered by an allowlist, paths
are checked for traversal, links are skipped and sizes are capped. Nested zip
files are unpacked through a temporary file. RAR members (DOVE, sign-09) are
unpacked with bsdtar when it is installed (Debian: libarchive-tools) and are
otherwise listed as skipped. Nothing from an archive is ever executed.

Output (git-ignored, rebuilt from scratch per candidate):
    bench-common/ngcc/src/<id>/...     C/asm sources and headers, KAT_SIG_/KAT_KEM_ files, licences
    bench-common/ngcc/specs/<id>/...   every PDF
    bench-common/ngcc/src/.extracted   marker that ngcc.mk checks before building

Usage: bench-common/ngcc/extract.py [id ...]
"""

import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath

from fetch import cache_dir, read_archives, sha256_file

HERE = Path(__file__).resolve().parent
SRC_OUT = HERE / "src"
SPEC_OUT = HERE / "specs"
SOURCE_SUFFIXES = {".c", ".h", ".s", ".inc", ".macros", ".cpp", ".cc", ".hpp"}
KEEP_NAME = re.compile(
    r"^(KAT_(SIG|KEM)_[A-Za-z0-9_+.-]+\.txt|(LICEN[CS]E|COPYING|NOTICE)([._-].*)?)$", re.IGNORECASE
)
MAX_MEMBER = 256 << 20
S_IFMT, S_IFLNK = 0o170000, 0o120000


def decode_name(info: zipfile.ZipInfo) -> str:
    """zipfile decodes names without the UTF-8 flag as cp437; these archives often use GBK."""
    if info.flag_bits & 0x800:
        return info.filename
    raw = info.filename.encode("cp437")
    for encoding in ("utf-8", "gbk"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return info.filename


def safe_relpath(name: str) -> PurePosixPath | None:
    p = PurePosixPath(name.replace("\\", "/"))
    if not p.parts or p.is_absolute() or ".." in p.parts or ":" in p.parts[0]:
        return None
    if any(part == "__MACOSX" or part.startswith("._") for part in p.parts):
        return None
    return p


def classify(p: PurePosixPath) -> str | None:
    suffix = p.suffix.lower()
    if suffix == ".pdf":
        return "spec"
    if suffix in SOURCE_SUFFIXES or KEEP_NAME.match(p.name):
        return "src"
    if suffix in (".zip", ".rar"):
        return "archive"
    return None


class Extractor:
    def __init__(self, src_out: Path, spec_out: Path):
        self.out = {"src": src_out, "spec": spec_out}
        self.counts = {"src": 0, "spec": 0}
        self.skipped: list[str] = []

    def write(self, kind: str, rel: PurePosixPath, stream) -> None:
        dest = self.out[kind] / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        with open(dest, "wb") as out:
            shutil.copyfileobj(stream, out)
        self.counts[kind] += 1

    def zip(self, zf: zipfile.ZipFile, prefix: PurePosixPath) -> None:
        for info in zf.infolist():
            if info.is_dir() or (info.external_attr >> 16) & S_IFMT == S_IFLNK:
                continue
            rel = safe_relpath(decode_name(info))
            if rel is None:
                self.skipped.append(f"unsafe path: {info.filename!r}")
                continue
            rel = prefix / rel
            kind = classify(rel)
            if kind is None:
                continue
            if kind != "archive" and info.file_size > MAX_MEMBER:
                self.skipped.append(f"too large: {rel}")
                continue
            with zf.open(info) as stream:
                if kind == "archive":
                    self.nested(rel, stream)
                else:
                    self.write(kind, rel, stream)

    def nested(self, rel: PurePosixPath, stream) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "inner"
            with open(path, "wb") as out:
                shutil.copyfileobj(stream, out)
            prefix = rel.with_suffix("")
            if rel.suffix.lower() == ".zip":
                try:
                    with zipfile.ZipFile(path) as inner:
                        self.zip(inner, prefix)
                except zipfile.BadZipFile:
                    self.skipped.append(f"bad nested zip: {rel}")
            else:
                self.rar(path, prefix, rel)

    def rar(self, path: Path, prefix: PurePosixPath, rel: PurePosixPath) -> None:
        bsdtar = shutil.which("bsdtar")
        if bsdtar is None:
            self.skipped.append(f"RAR not unpacked (install libarchive-tools for bsdtar): {rel}")
            return
        with tempfile.TemporaryDirectory() as tmp:
            # bsdtar refuses absolute and '..' member paths unless -P is given.
            subprocess.run([bsdtar, "-xf", str(path), "-C", tmp], check=True)
            root = Path(tmp)
            for f in sorted(root.rglob("*")):
                if f.is_symlink() or not f.is_file():
                    continue
                sub = safe_relpath(f.relative_to(root).as_posix())
                kind = classify(sub) if sub else None
                if kind in ("src", "spec") and f.stat().st_size <= MAX_MEMBER:
                    with open(f, "rb") as stream:
                        self.write(kind, prefix / sub, stream)


def extract_archive(zip_path: Path, cand_id: str, src_root: Path = SRC_OUT, spec_root: Path = SPEC_OUT) -> Extractor:
    for root in (src_root, spec_root):
        shutil.rmtree(root / cand_id, ignore_errors=True)
    ex = Extractor(src_root / cand_id, spec_root / cand_id)
    with zipfile.ZipFile(zip_path) as zf:
        ex.zip(zf, PurePosixPath())
    return ex


def main(argv: list[str] | None = None) -> int:
    ids = set(sys.argv[1:] if argv is None else argv)
    _, rows = read_archives()
    cache, failed = cache_dir(), 0
    for row in rows:
        if ids and row["id"] not in ids:
            continue
        zip_path = cache / f"{row['id']}.zip"
        if not zip_path.exists() or not row["sha256"] or sha256_file(zip_path) != row["sha256"]:
            print(f"{row['id']:8} not fetched or not matching its pin; run fetch.py first", file=sys.stderr)
            failed += 1
            continue
        ex = extract_archive(zip_path, row["id"])
        print(f"{row['id']:8} {ex.counts['src']:5} source files {ex.counts['spec']:3} PDFs")
        for s in ex.skipped:
            print(f"         skipped {s}")
    SRC_OUT.mkdir(exist_ok=True)
    (SRC_OUT / ".extracted").write_text(datetime.now(timezone.utc).isoformat() + "\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_extract.py`
Expected: `5 passed`.

- [ ] **Step 5: Lint and commit**

```bash
chmod +x bench-common/ngcc/extract.py
uvx ruff check --fix bench-common/ngcc && uvx ruff format bench-common/ngcc
git add bench-common/ngcc/extract.py bench-common/ngcc/tests/test_extract.py
git commit -m "Extract NGCC sources, KATs and specs with an allowlist and traversal checks

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

- [ ] **Step 6 (needs Task 1 Step 7): Extract everything**

Run from the repo root: `uv run bench-common/ngcc/extract.py 2>&1 | tee /tmp/ngcc-extract.log; du -sh bench-common/ngcc/src bench-common/ngcc/specs; git status --short bench-common`
Expected: 75 lines with non-zero source counts; `git status` shows nothing under `src/`/`specs/` (ignored). If sign-09 reports `RAR not unpacked`, install `libarchive-tools` and re-run `uv run bench-common/ngcc/extract.py sign-09`. Report any other `skipped` lines to the controller.

---

### Task 3: Shared ICCS adapter tooling (rules generator, inventory, KAT replay and compare, hash usage)

**Files:**
- Create: `bench-common/ngcc/gen_rules.py`, `bench-common/ngcc/inventory.py`, `bench-common/ngcc/ngcc_kat.c`, `bench-common/ngcc/kat_compare.py`, `bench-common/ngcc/hash_usage.py`
- Create fixtures: `bench-common/ngcc/tests/fixtures/src/.extracted` (empty), `bench-common/ngcc/tests/fixtures/src/sign-99/Reference_Implementation/Toy-Sig-1/{SIG_AlgorithmInstance.h,SIG_AlgorithmInstance.c,drng.h,drng.c}`, `bench-common/ngcc/tests/fixtures/src/kem-99/Reference_Implementation/Toy-KEM-1/{KEM_AlgorithmInstance.h,KEM_AlgorithmInstance.c,drng.h,drng.c}`, `bench-common/ngcc/tests/drng_def.c`
- Test: `bench-common/ngcc/tests/{test_gen_rules.py,test_inventory.py,test_kat_compare.py,test_hash_usage.py,test_ngcc_kat.py}`

**Interfaces:**
- Produces (`gen_rules.py`): `read_params(path) -> list[dict]` (raises `ParamsError`), `instance_dir(row, src_root) -> Path`, `resolve_sources(row, src_root, suite_dir) -> list[Path]`, `render_rules(rows, src_root, suite_dir) -> str`, `write_shims(rows, template, out_dir)`, `make_links(rows, src_root, build_dir)`, `DRIVER` regex. CLI: `gen_rules.py --params P --src S --build B --suite-dir D --template shim_template.c --out B/rules.mk`. It writes the shims itself instead of using `gen_shims.py`, so optional columns get their defaults.
- Produces (`params.tsv` schema, used by Tasks 4–7, 11–12): required `file name id instance impl srcdir srcs kat`; optional `incs defs shimdefs ldlibs iters katn` (see the `gen_rules.py` docstring).
- Produces: `inventory.find_instances(cand_root: Path, cand_id: str, kind: str) -> list[dict]`. CLI: `inventory.py sig|kem [id …] > draft.tsv`.
- Produces: `kat_compare.compare(generated: str, reference: str, count: int) -> "PASS"|"MISMATCH"`.
- Produces: `hash_usage.hash_usage(paths) -> "placeholder"|"sm3"|"own"`.
- Produces: `ngcc_kat sig|kem LIB.so OUT.txt [records]`, which exits 0 ok, 1 usage, 2 load failure (missing symbol), 3 crypto failure (non-zero return, length over its claim, decaps mismatch).

- [ ] **Step 1: Add the toy fixtures** (insecure ICCS-API instances used only by tests)

`fixtures/src/sign-99/Reference_Implementation/Toy-Sig-1/drng.h` (and an identical copy under `kem-99/Reference_Implementation/Toy-KEM-1/drng.h`):
```c
/* Toy DRNG for the NGCC adapter tests: same interface and context layout as the
 * ICCS drng.h, but NOT the ICCS generator and not random. */
#ifndef DRNG_H
#define DRNG_H
#define SEEDLEN (55)
typedef struct {
    unsigned char V[SEEDLEN];
    unsigned char C[SEEDLEN];
    unsigned char reseed_counter[SEEDLEN];
} DRNG_ctx;
int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes);
int get_random_number(DRNG_ctx *drng, unsigned char *random_number, unsigned long long random_number_len_bits);
#endif
```

`…/drng.c` (identical copy in both toy dirs):
```c
#include <string.h>
#include "drng.h"

int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes) {
    memset(drng, 0, sizeof *drng);
    for (unsigned long long i = 0; i < seed_len_bytes; i++)
        drng->V[i % SEEDLEN] = (unsigned char)(drng->V[i % SEEDLEN] * 31u + seed[i]);
    return 0;
}

int get_random_number(DRNG_ctx *drng, unsigned char *out, unsigned long long bits) {
    unsigned long long n = (bits + 7) / 8;
    for (unsigned long long i = 0; i < n; i++) {
        unsigned char *v = &drng->V[drng->C[0] % SEEDLEN];
        *v = (unsigned char)(*v * 167u + 13u + drng->C[1]);
        out[i] = *v;
        if (++drng->C[0] == 0) drng->C[1]++;
    }
    return 0;
}
```

`…/Toy-Sig-1/SIG_AlgorithmInstance.h`:
```c
/* Toy NGCC signature instance for the adapter tests (insecure by design). */
#ifndef SIG_ALGORITHM_INSTANCE_H
#define SIG_ALGORITHM_INSTANCE_H
#define OUTPUT_BLANK_TEST_VECTORS 1
#define ALGORITHM_INSTANCE "Toy-Sig-1"
unsigned long long sig_get_pk_len_bytes(void);
unsigned long long sig_get_sk_len_bytes(void);
unsigned long long sig_get_sn_len_bytes(void);
int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes);
int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes);
int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes);
#endif
```

`…/Toy-Sig-1/SIG_AlgorithmInstance.c`:
```c
/* Toy signature: pk = sk, signature = keyed mix of the message. Tests only. */
#include <string.h>
#include "SIG_AlgorithmInstance.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

#define TOY 32
#ifdef TOY_LONG_PK
#define TOY_EXTRA 1   /* reports one byte more than it claims: the shim must refuse */
#else
#define TOY_EXTRA 0
#endif

unsigned long long sig_get_pk_len_bytes(void) { return TOY; }
unsigned long long sig_get_sk_len_bytes(void) { return TOY; }
unsigned long long sig_get_sn_len_bytes(void) { return TOY; }

static void toy_tag(const unsigned char *key, const unsigned char *m, unsigned long long mlen, unsigned char *tag) {
    memcpy(tag, key, TOY);
    for (unsigned long long j = 0; j < mlen; j++)
        tag[j % TOY] = (unsigned char)(tag[j % TOY] * 31u + m[j] + 1u);
}

int sig_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes) {
    if (get_random_number(&drng_algorithm, sk, TOY * 8)) return -1;
    memcpy(pk, sk, TOY);
    *pk_len_bytes = TOY + TOY_EXTRA;
    *sk_len_bytes = TOY;
    return 0;
}

int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes) {
    if (sk_len_bytes != TOY) return -1;
    toy_tag(sk, m, m_len_bytes, sn);
    *sn_len_bytes = TOY;
    return 0;
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes) {
    unsigned char t[TOY];
    if (pk_len_bytes < TOY || sn_len_bytes != TOY) return -1;
    toy_tag(pk, m, m_len_bytes, t);
    return memcmp(t, sn, TOY) ? -1 : 0;
}
```

`…/Toy-KEM-1/KEM_AlgorithmInstance.h`:
```c
/* Toy NGCC KEM instance for the adapter tests (insecure by design). */
#ifndef KEM_ALGORITHM_INSTANCE_H
#define KEM_ALGORITHM_INSTANCE_H
#define OUTPUT_BLANK_TEST_VECTORS 1
#define ALGORITHM_INSTANCE "Toy-KEM-1"
unsigned long long kem_get_pk_len_bytes(void);
unsigned long long kem_get_sk_len_bytes(void);
unsigned long long kem_get_ss_len_bytes(void);
unsigned long long kem_get_ct_len_bytes(void);
int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes);
int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes);
int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes);
#endif
```

`…/Toy-KEM-1/KEM_AlgorithmInstance.c`:
```c
/* Toy KEM: pk = sk, ct = ss XOR pk. Tests only. */
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

#define TOY 32

unsigned long long kem_get_pk_len_bytes(void) { return TOY; }
unsigned long long kem_get_sk_len_bytes(void) { return TOY; }
unsigned long long kem_get_ss_len_bytes(void) { return TOY; }
unsigned long long kem_get_ct_len_bytes(void) { return TOY; }

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes, unsigned char *sk, unsigned long long *sk_len_bytes) {
    if (get_random_number(&drng_algorithm, sk, TOY * 8)) return -1;
    memcpy(pk, sk, TOY);
    *pk_len_bytes = TOY;
    *sk_len_bytes = TOY;
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
    if (pk_len_bytes != TOY || get_random_number(&drng_algorithm, ss, TOY * 8)) return -1;
    for (int i = 0; i < TOY; i++) ct[i] = ss[i] ^ pk[i];
    *ss_len_bytes = TOY;
    *ct_len_bytes = TOY;
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
    if (sk_len_bytes != TOY || ct_len_bytes != TOY) return -1;
    for (int i = 0; i < TOY; i++) ss[i] = ct[i] ^ sk[i];
    *ss_len_bytes = TOY;
    return 0;
}
```

`bench-common/ngcc/tests/drng_def.c` (in real builds the shim defines it):
```c
#include "drng.h"
DRNG_ctx drng_algorithm;
```

Run: `touch bench-common/ngcc/tests/fixtures/src/.extracted`

- [ ] **Step 2: Write the failing tests**

`bench-common/ngcc/tests/test_gen_rules.py`:
```python
import pytest

from gen_rules import ParamsError, read_params, render_rules, resolve_sources, write_shims

HEADER = "file\tname\tid\tinstance\timpl\tsrcdir\tsrcs\tkat\tdefs\n"
ROW = ("x", "NGCC-X", "sign-01", "X", "ref", "Ref/X", "*", "-", "")


def write_params(tmp_path, *rows):
    p = tmp_path / "params.tsv"
    p.write_text("# comment\n" + HEADER + "".join("\t".join(r) + "\n" for r in rows))
    return p


def make_instance(src, files):
    d = src / "sign-01" / "Ref" / "X"
    for f in files:
        (d / f).parent.mkdir(parents=True, exist_ok=True)
        (d / f).write_text("int x;\n")
    return d


def test_star_skips_driver_files(tmp_path):
    make_instance(tmp_path / "src", ["a.c", "drng.c", "KAT_SIG.c", "main.c", "test_x.c", "speed.c", "cpucycles.c", "b.S", "c.h"])
    row = read_params(write_params(tmp_path, ROW))[0]
    assert [p.name for p in resolve_sources(row, tmp_path / "src", tmp_path)] == ["a.c", "b.S", "drng.c"]


def test_drop_subdir_and_patch(tmp_path):
    d = make_instance(tmp_path / "src", ["a.c", "drng.c", "kat_test/SIG_AlgorithmInstance.c"])
    srcs = "* -a.c kat_test/SIG_AlgorithmInstance.c patches/sign-01/a.c"
    row = read_params(write_params(tmp_path, ROW[:6] + (srcs, "-", "")))[0]
    assert resolve_sources(row, tmp_path / "src", tmp_path) == [
        d / "drng.c", d / "kat_test/SIG_AlgorithmInstance.c", tmp_path / "patches/sign-01/a.c",
    ]


def test_duplicate_labels_rejected(tmp_path):
    with pytest.raises(ParamsError, match="duplicate file"):
        read_params(write_params(tmp_path, ROW, ROW))


def test_name_needs_ngcc_prefix(tmp_path):
    with pytest.raises(ParamsError, match="NGCC-"):
        read_params(write_params(tmp_path, ("x", "X") + ROW[2:]))


def test_defs_reach_the_sources_but_not_the_shim(tmp_path):
    make_instance(tmp_path / "src", ["a.c", "drng.c"])
    rows = read_params(write_params(tmp_path, ROW[:8] + ("-Dcrypto_sign_keypair=cand_keypair",)))
    text = render_rules(rows, tmp_path / "src", tmp_path)
    assert "NGCC_SOS += $(BUILD)/x.so" in text
    shim_line = next(line for line in text.splitlines() if "-c x_shim.c" in line)
    link_line = next(line for line in text.splitlines() if "-shared" in line)
    assert "crypto_sign_keypair" not in shim_line
    assert "-Dcrypto_sign_keypair=cand_keypair" in link_line
    assert "$(BUILD)/src/x/a.c $(BUILD)/src/x/drng.c" in link_line


def test_missing_sources_become_a_failing_rule(tmp_path):
    rows = read_params(write_params(tmp_path, ROW))
    assert "FAIL  NGCC-X: no sources" in render_rules(rows, tmp_path / "src", tmp_path)


def test_missing_drng_is_reported(tmp_path):
    make_instance(tmp_path / "src", ["a.c"])
    rows = read_params(write_params(tmp_path, ROW))
    assert "no drng source" in render_rules(rows, tmp_path / "src", tmp_path)


def test_shims_get_defaults_for_absent_optional_columns(tmp_path):
    rows = read_params(write_params(tmp_path, ROW))
    (tmp_path / "t.c").write_text('"@NAME@" @ITERS@ @IMPL@ @KATN@\n')
    write_shims(rows, tmp_path / "t.c", tmp_path)
    assert (tmp_path / "x_shim.c").read_text() == '"NGCC-X" 0 ref 10\n'
```

`bench-common/ngcc/tests/test_inventory.py`:
```python
from inventory import find_instances

API = "unsigned long long sig_get_pk_len_bytes(void)\n{\n    return 1;\n}\n"
HDR = '#define ALGORITHM_INSTANCE "Aigis-sig1"\n'


def write(root, rel, text=""):
    p = root / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)


def test_finds_reference_and_optimised_instances(tmp_path):
    root = tmp_path / "sign-01"
    ref = "Aigis/Implementations/Reference_Implementation/Aigis-Sig-I"
    write(root, f"{ref}/sign.c")
    write(root, f"{ref}/drng.c")
    write(root, f"{ref}/kat_test/SIG_AlgorithmInstance.c", API)
    write(root, f"{ref}/kat_test/SIG_AlgorithmInstance.h", HDR)
    write(root, f"{ref}/kat_test/KAT_SIG.c")
    write(root, "Aigis/Implementations/Optimized_Implementation/avx2/SIG_AlgorithmInstance.c", API)
    write(root, "Aigis/Implementations/Optimized_Implementation/avx2/SIG_AlgorithmInstance.h", HDR)
    write(root, "Aigis/KAT/KAT_SIG_Aigis-sig1.txt", "Count = 0\n")

    rows = {r["impl"]: r for r in find_instances(root, "sign-01", "sig")}
    assert set(rows) == {"ref", "opt"}
    r = rows["ref"]
    assert (r["file"], r["name"], r["instance"], r["srcdir"]) == (
        "sign_01_aigis_sig1", "NGCC-Aigis-sig1", "Aigis-sig1", ref)
    assert r["srcs"] == "* kat_test/SIG_AlgorithmInstance.c"
    assert r["incs"] == "kat_test"
    assert r["kat"] == "Aigis/KAT/KAT_SIG_Aigis-sig1.txt"
    assert rows["opt"]["srcs"] == "*" and rows["opt"]["incs"] == ""
```

`bench-common/ngcc/tests/test_kat_compare.py`:
```python
from kat_compare import compare

GEN = "".join(
    f"Count = {i}\nSeed_Len = 64\nSeed = {i:02X}\nPK_Len = 2\nPK = AB{i:02X}\n\n" for i in range(10)
)


def test_identical_passes():
    assert compare(GEN, GEN, 10) == "PASS"


def test_crlf_trailing_blanks_and_hex_case_are_ignored():
    ref = GEN.replace("\n", "  \r\n").replace("AB", "ab")
    assert compare(GEN, ref, 10) == "PASS"


def test_template_mode_reference_ignores_lengths():
    ref = GEN.replace("PK_Len = 2", "PK_Len =")
    assert compare(GEN, ref, 10) == "PASS"


def test_changed_byte_mismatches():
    assert compare(GEN, GEN.replace("PK = AB05", "PK = AC05"), 10) == "MISMATCH"


def test_compares_only_the_requested_prefix():
    assert compare(GEN, GEN.replace("PK = AB05", "PK = AC05"), 3) == "PASS"


def test_short_reference_mismatches():
    assert compare(GEN, GEN.split("Count = 5")[0], 10) == "MISMATCH"
```

`bench-common/ngcc/tests/test_hash_usage.py`:
```python
from hash_usage import hash_usage


def test_placeholder_calls(tmp_path):
    (tmp_path / "a.c").write_text("void f(void) { pseudoXOF(256, m, 8, out); }\n")
    assert hash_usage([tmp_path / "a.c"]) == "placeholder"


def test_sm3_only(tmp_path):
    (tmp_path / "a.c").write_text("void f(void) { sm3hash(256, m, 8, out); }\n")
    assert hash_usage([tmp_path / "a.c"]) == "sm3"


def test_comments_auxfunc_and_missing_files_are_ignored(tmp_path):
    (tmp_path / "a.c").write_text("/* pseudohash(x) */\n// pseudoXOF(y)\nint shake(void);\n")
    (tmp_path / "auxfunc.c").write_text("int pseudohash(int a) { return a; }\n")
    assert hash_usage([tmp_path / "a.c", tmp_path / "auxfunc.c", tmp_path / "gone.c"]) == "own"
```

`bench-common/ngcc/tests/test_ngcc_kat.py`:
```python
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
NGCC = HERE.parent
TOY = {
    "sig": HERE / "fixtures/src/sign-99/Reference_Implementation/Toy-Sig-1",
    "kem": HERE / "fixtures/src/kem-99/Reference_Implementation/Toy-KEM-1",
}
API = {"sig": "SIG_AlgorithmInstance.c", "kem": "KEM_AlgorithmInstance.c"}


def build(tmp_path, kind, with_api=True):
    d = TOY[kind]
    so = tmp_path / f"toy_{kind}.so"
    srcs = [str(d / "drng.c"), str(HERE / "drng_def.c")] + ([str(d / API[kind])] if with_api else [])
    subprocess.run(["cc", "-shared", "-fPIC", f"-I{d}", "-o", str(so), *srcs], check=True)
    exe = tmp_path / "ngcc_kat"
    subprocess.run(["cc", "-O2", "-std=gnu17", "-o", str(exe), str(NGCC / "ngcc_kat.c"), "-ldl"], check=True)
    return exe, so


def run(exe, kind, so, out, *extra):
    return subprocess.run([str(exe), kind, str(so), str(out), *extra], capture_output=True, text=True, check=False)


def keys(text):
    return [line.split(" = ")[0].rstrip(" =") for line in text.splitlines() if line]


def test_signature_records_follow_the_iccs_layout(tmp_path):
    exe, so = build(tmp_path, "sig")
    out = tmp_path / "kat.txt"
    assert run(exe, "sig", so, out).returncode == 0
    text = out.read_text()
    order = ["Count", "Seed_Len", "Seed", "PK_Len", "PK", "SK_Len", "SK", "M_Len", "M", "Sn_Len", "Sn"]
    assert keys(text) == order * 10
    assert "Seed_Len = 64\n" in text and "PK_Len = 32\n" in text
    m_lens = [int(line.split(" = ")[1]) for line in text.splitlines() if line.startswith("M_Len")]
    assert m_lens == list(range(56, 136, 8))
    again = tmp_path / "again.txt"
    run(exe, "sig", so, again)
    assert again.read_text() == text


def test_kem_records_follow_the_iccs_layout(tmp_path):
    exe, so = build(tmp_path, "kem")
    out = tmp_path / "kat.txt"
    assert run(exe, "kem", so, out, "3").returncode == 0
    order = ["Count", "Seed_Len", "Seed", "PK_Len", "PK", "SK_Len", "SK", "CT_Len", "CT", "SS_Len", "SS"]
    assert keys(out.read_text()) == order * 3


def test_missing_api_symbol_is_a_load_failure(tmp_path):
    exe, so = build(tmp_path, "sig", with_api=False)
    assert run(exe, "sig", so, tmp_path / "kat.txt").returncode == 2
```

- [ ] **Step 3: Run the tests to verify they fail**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests`
Expected: the new modules fail with `ModuleNotFoundError` (`gen_rules`, `inventory`, `kat_compare`, `hash_usage`), and `test_ngcc_kat.py` fails because `ngcc_kat.c` is missing.

- [ ] **Step 4: Write `gen_rules.py`**

```python
#!/usr/bin/env python3
"""Generate make rules for China NGCC round-1 instances from a params.tsv manifest.

Every NGCC candidate implements the same ICCS public-key C API, so one shim
template per suite fits all of them. What differs per instance is data: where
its sources live, which files to compile, extra include dirs, defines and
libraries. This script turns that data into build/rules.mk for ngcc.mk.

params.tsv: tab-separated, '#' lines are comments. Required columns:
  file      label: <file>_shim.c and build/<file>.so ([a-z0-9_], unique)
  name      bench output name, NGCC-<instance> (unique)
  id        candidate id (sign-01 ... sign-34, kem-01 ... kem-41)
  instance  the submission's ALGORITHM_INSTANCE (joins data/ngcc/candidates.yaml)
  impl      ref | opt: which submitted implementation is built
  srcdir    the instance directory, relative to src/<id>/
  srcs      space-separated items:
              *                 every .c/.S/.s directly in srcdir except drivers
                                (KAT_*, PQCgenKAT*, main*, test*, bench*, speed*, cpucycles*)
              a.c / sub/b.c     a file relative to srcdir
              -a.c              drop that file (relative to srcdir) from the list
              patches/<id>/x.c  our patched copy, relative to the suite dir
  kat       the submitted KAT file relative to src/<id>/, or '-' if there is none
Optional columns (empty = default):
  incs      include dirs relative to srcdir, or patches/<id>[/...] (searched first)
  defs      -D/-U flags for the candidate's sources only (never the shim)
  shimdefs  flags for the shim only, e.g. -DNGCC_INSTANCE_HEADER='"kat_test/SIG_AlgorithmInstance.h"'
            or -DNGCC_NO_DRNG (the candidate defines drng_algorithm itself)
  ldlibs    extra libraries, e.g. -lgmp -lcrypto
  iters     benchmark iteration override (default 0 = harness default)
  katn      KAT records that status.py replays and compares, 1-10 (default 10)
No value may contain '$' or a tab.

Usage: gen_rules.py --params P --src SRC --build BUILD --suite-dir DIR --template shim_template.c --out BUILD/rules.mk
(also writes <file>_shim.c per row into the suite dir)
"""

import argparse
import csv
import re
import sys
from pathlib import Path

REQUIRED = ["file", "name", "id", "instance", "impl", "srcdir", "srcs", "kat"]
OPTIONAL = {"incs": "", "defs": "", "shimdefs": "", "ldlibs": "", "iters": "0", "katn": "10"}
DRIVER = re.compile(r"^(KAT_|PQCgenKAT|main|test|bench|speed|cpucycles)", re.IGNORECASE)
SOURCE_SUFFIXES = {".c", ".S", ".s"}
LABEL = re.compile(r"^[a-z0-9_]+$")
ID = re.compile(r"^(sign|kem)-\d\d$")

GOOD = """\
NGCC_SOS += $(BUILD)/{file}.so
$(BUILD)/{file}.so: {file}_shim.c {deps} $(BUILD)/rules.mk
\t@echo "  NGCC  {name}"
\t@( $(CC) $(NGCC_CFLAGS) {shimdefs} {incs} -c {file}_shim.c -o $(BUILD)/{file}_shim.o && \\
\t  $(CC) $(NGCC_CFLAGS) {defs} {incs} -shared -o $@ $(BUILD)/{file}_shim.o {deps} {ldlibs} -lm \\
\t) > $(BUILD)/{file}.log 2>&1 || {{ echo "  FAIL  {name} (see $(BUILD)/{file}.log)"; rm -f $@; exit 1; }}

"""
BAD = """\
NGCC_SOS += $(BUILD)/{file}.so
$(BUILD)/{file}.so:
\t@echo "  FAIL  {name}: {why}" | tee $(BUILD)/{file}.log; exit 1

"""


class ParamsError(ValueError):
    pass


def read_params(path: Path) -> list[dict]:
    with open(path, newline="", encoding="utf-8") as f:
        body = [ln for ln in f if not ln.startswith("#")]
    reader = csv.DictReader(body, delimiter="\t")
    missing = [c for c in REQUIRED if c not in (reader.fieldnames or [])]
    if missing:
        raise ParamsError(f"{path}: missing columns {missing}")
    rows = []
    for n, raw in enumerate(reader, start=2):
        row = {k: (v or "").strip() for k, v in raw.items() if k is not None}
        if not any(row.values()):
            continue
        for k, default in OPTIONAL.items():
            row[k] = row.get(k) or default
        where = f"{path}: row {n}"
        for k in REQUIRED:
            if not row[k]:
                raise ParamsError(f"{where}: empty '{k}'")
        if any("$" in v for v in row.values()):
            raise ParamsError(f"{where}: '$' is not allowed")
        if not LABEL.match(row["file"]):
            raise ParamsError(f"{where}: file label must match [a-z0-9_]+")
        if not row["name"].startswith("NGCC-"):
            raise ParamsError(f"{where}: name must be NGCC-<instance>")
        if not ID.match(row["id"]):
            raise ParamsError(f"{where}: bad candidate id {row['id']!r}")
        if row["impl"] not in ("ref", "opt"):
            raise ParamsError(f"{where}: impl must be ref or opt")
        if not 1 <= int(row["katn"]) <= 10 or int(row["iters"]) < 0:
            raise ParamsError(f"{where}: katn must be 1-10 and iters >= 0")
        rows.append(row)
    for key in ("file", "name"):
        values = [r[key] for r in rows]
        if dups := sorted({v for v in values if values.count(v) > 1}):
            raise ParamsError(f"{path}: duplicate {key}: {dups}")
    return rows


def instance_dir(row: dict, src_root: Path) -> Path:
    return src_root / row["id"] / row["srcdir"]


def resolve_sources(row: dict, src_root: Path, suite_dir: Path) -> list[Path]:
    srcdir = instance_dir(row, src_root)
    chosen: list[Path] = []
    dropped: set[str] = set()
    for item in row["srcs"].split():
        if item == "*":
            if srcdir.is_dir():
                chosen += sorted(
                    p for p in srcdir.iterdir()
                    if p.is_file() and p.suffix in SOURCE_SUFFIXES and not DRIVER.match(p.name)
                )
        elif item.startswith("-"):
            dropped.add(item[1:])
        elif item.startswith("patches/"):
            chosen.append(suite_dir / item)
        else:
            chosen.append(srcdir / item)
    result: list[Path] = []
    for p in chosen:
        rel = p.relative_to(srcdir).as_posix() if p.is_relative_to(srcdir) else None
        if rel in dropped or p in result:
            continue
        result.append(p)
    return result


def rule_path(p: Path, row: dict, src_root: Path, suite_dir: Path) -> str:
    srcdir = instance_dir(row, src_root)
    if p.is_relative_to(srcdir):
        return f"$(BUILD)/src/{row['file']}/{p.relative_to(srcdir).as_posix()}"
    return p.relative_to(suite_dir).as_posix()


def inc_flags(row: dict) -> str:
    dirs = row["incs"].split()
    patches = [f"-I{d}" for d in dirs if d.startswith("patches/")]
    others = [f"-I$(BUILD)/src/{row['file']}/{d}" for d in dirs if not d.startswith("patches/")]
    return " ".join([*patches, f"-I$(BUILD)/src/{row['file']}", *others])


def problem(sources: list[Path], paths: list[str]) -> str | None:
    if not sources:
        return "no sources (is src/ extracted and srcdir right?)"
    if missing := [p for p in sources if not p.is_file()]:
        return f"missing source {missing[0].name}"
    if not any("drng" in p.name.lower() for p in sources):
        return "no drng source in srcs (randomness must come from drng_algorithm)"
    if any(any(c.isspace() for c in rp) for rp in paths):
        return "whitespace in a source path below srcdir"
    return None


def render_rules(rows: list[dict], src_root: Path, suite_dir: Path) -> str:
    out = ["# Generated by bench-common/ngcc/gen_rules.py from params.tsv. Do not edit.\n", "NGCC_SOS :=\n\n"]
    for row in rows:
        sources = resolve_sources(row, src_root, suite_dir)
        paths = [rule_path(p, row, src_root, suite_dir) for p in sources]
        why = problem(sources, paths)
        if why:
            out.append(BAD.format(file=row["file"], name=row["name"], why=why.replace('"', "'")))
        else:
            out.append(GOOD.format(
                file=row["file"], name=row["name"], deps=" ".join(paths), incs=inc_flags(row),
                defs=row["defs"], shimdefs=row["shimdefs"], ldlibs=row["ldlibs"],
            ))
    return "".join(out)


def write_shims(rows: list[dict], template: Path, out_dir: Path) -> None:
    """Write <file>_shim.c per row, replacing @COLUMN@ tokens (upper-case column names).

    Same convention as bench/gen_shims.py, but it uses the normalised rows, so optional
    columns that are absent from params.tsv still get their defaults (e.g. @ITERS@ = 0).
    """
    text = template.read_text(encoding="utf-8")
    for row in rows:
        shim = text
        for key, value in row.items():
            shim = shim.replace(f"@{key.upper()}@", value)
        (out_dir / f"{row['file']}_shim.c").write_text(shim, encoding="utf-8")


def make_links(rows: list[dict], src_root: Path, build_dir: Path) -> None:
    """build/src/<file> -> the instance dir, so rule paths never contain spaces."""
    links = build_dir / "src"
    links.mkdir(parents=True, exist_ok=True)
    for row in rows:
        link, target = links / row["file"], instance_dir(row, src_root)
        if link.is_symlink() or link.exists():
            link.unlink()
        if target.is_dir():
            link.symlink_to(target.resolve(), target_is_directory=True)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="params.tsv -> rules.mk for NGCC instances")
    ap.add_argument("--params", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("--build", required=True)
    ap.add_argument("--suite-dir", default=".")
    ap.add_argument("--template", required=True, help="shim template with @COLUMN@ tokens")
    ap.add_argument("--out", required=True)
    args = ap.parse_args(argv)
    try:
        rows = read_params(Path(args.params))
    except ParamsError as e:
        print(f"gen_rules: {e}", file=sys.stderr)
        return 2
    src, build, suite = Path(args.src).resolve(), Path(args.build).resolve(), Path(args.suite_dir).resolve()
    write_shims(rows, Path(args.template), suite)
    make_links(rows, src, build)
    Path(args.out).write_text(render_rules(rows, src, suite), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 5: Write `inventory.py`**

```python
#!/usr/bin/env python3
"""Draft params.tsv rows for every ICCS API instance in extracted NGCC candidates.

Finds the .c files that define the API (sig_get_pk_len_bytes / kem_get_pk_len_bytes),
reads ALGORITHM_INSTANCE from the instance header next to them, classifies the
implementation tree (ref / opt / other) and looks up the matching submitted KAT file.
The output is a draft that a person curates into <suite>/schemes/ngcc/params.tsv.

Usage: inventory.py sig|kem [id ...] > draft.tsv     (reads bench-common/ngcc/src/)
"""

import re
import sys
from pathlib import Path

from gen_rules import DRIVER

HERE = Path(__file__).resolve().parent
API_DEF = {
    kind: re.compile(rf"unsigned\s+long\s+long\s+{kind}_get_pk_len_bytes\s*\([^;{{)]*\)\s*\{{")
    for kind in ("sig", "kem")
}
INSTANCE_DEF = re.compile(r'#\s*define\s+ALGORITHM_INSTANCE\s+"([^"]+)"')
HEADER = {"sig": "SIG_AlgorithmInstance.h", "kem": "KEM_AlgorithmInstance.h"}
WRAPPER_DIRS = {"kat_test", "algorithminstance", "api", "iccs", "niccs"}
COLUMNS = ["file", "name", "id", "instance", "impl", "srcdir", "srcs", "incs", "defs", "shimdefs", "ldlibs", "iters", "katn", "kat"]


def impl_of(rel: str) -> str:
    s = rel.lower()
    if "reference" in s or "/ref/" in f"/{s}/":
        return "ref"
    if "optimi" in s or "avx" in s or "x86" in s:
        return "opt"
    return "other"


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")


def instance_name(api_dir: Path, kind: str) -> str | None:
    for h in [api_dir / HEADER[kind], *sorted(api_dir.glob("*.h")), *sorted(api_dir.parent.glob("*.h"))]:
        if h.is_file() and (m := INSTANCE_DEF.search(h.read_text(errors="replace"))):
            return m.group(1)
    return None


def best_kat(kats: list[Path], srcdir: Path) -> Path | None:
    def shared(p: Path) -> int:
        n = 0
        for a, b in zip(p.parts, srcdir.parts):
            if a != b:
                break
            n += 1
        return n
    return min(kats, key=lambda p: (-shared(p), str(p))) if kats else None


def find_instances(cand_root: Path, cand_id: str, kind: str) -> list[dict]:
    kats: dict[str, list[Path]] = {}
    for p in sorted(cand_root.rglob(f"KAT_{kind.upper()}_*.txt")):
        kats.setdefault(p.name, []).append(p)
    rows = []
    for api in sorted(cand_root.rglob("*.c")):
        if not API_DEF[kind].search(api.read_text(errors="replace")):
            continue
        api_dir = api.parent
        instance = instance_name(api_dir, kind) or api_dir.name
        srcdir = api_dir.parent if api_dir.name.lower() in WRAPPER_DIRS else api_dir
        extra, incs = [], ""
        if api_dir != srcdir:
            extra = [f"{api_dir.name}/{p.name}" for p in sorted(api_dir.glob("*.c")) if not DRIVER.match(p.name)]
            incs = api_dir.name
        rel_src = srcdir.relative_to(cand_root).as_posix()
        kat = best_kat(kats.get(f"KAT_{kind.upper()}_{instance}.txt", []), srcdir)
        rows.append({
            "file": slug(f"{cand_id}_{instance}"), "name": f"NGCC-{instance}", "id": cand_id,
            "instance": instance, "impl": impl_of(rel_src), "srcdir": rel_src,
            "srcs": " ".join(["*", *extra]), "incs": incs, "defs": "", "shimdefs": "", "ldlibs": "",
            "iters": "0", "katn": "10", "kat": kat.relative_to(cand_root).as_posix() if kat else "-",
        })
    return rows


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if not args or args[0] not in ("sig", "kem"):
        print(__doc__, file=sys.stderr)
        return 2
    kind, ids = args[0], set(args[1:])
    prefix = "sign-" if kind == "sig" else "kem-"
    print("\t".join(COLUMNS))
    for cand in sorted((HERE / "src").glob(f"{prefix}*")):
        if ids and cand.name not in ids:
            continue
        for row in find_instances(cand, cand.name, kind):
            print("\t".join(row[c] for c in COLUMNS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 6: Write `kat_compare.py` and `hash_usage.py`**

`kat_compare.py`:
```python
"""Compare regenerated ICCS KAT records with a submitted KAT file.

Canonical form: CR and trailing blanks stripped, empty lines dropped, upper case
(hex case varies between submissions). When the submitted file was generated in
the ICCS template mode (blank "X_Len =" lines), every *_Len line is dropped from
both sides. Only the first `count` records are compared.
"""

import re

LEN_LINE = re.compile(r"^\w+_len\s*=", re.IGNORECASE)
BLANK_LEN = re.compile(r"^\w+_len\s*=$", re.IGNORECASE)


def canonical(text: str, nolen: bool) -> list[str]:
    lines = []
    for raw in text.splitlines():
        line = raw.rstrip().upper()
        if line and not (nolen and LEN_LINE.match(line)):
            lines.append(line)
    return lines


def records(lines: list[str]) -> list[list[str]]:
    out: list[list[str]] = []
    for line in lines:
        if line.startswith("COUNT ="):
            out.append([])
        if out:
            out[-1].append(line)
    return out


def compare(generated: str, reference: str, count: int) -> str:
    nolen = any(BLANK_LEN.match(raw.rstrip()) for raw in reference.splitlines())
    gen = records(canonical(generated, nolen))[:count]
    ref = records(canonical(reference, nolen))[:count]
    if len(gen) < count or len(ref) < count:
        return "MISMATCH"
    return "PASS" if gen == ref else "MISMATCH"
```

`hash_usage.py`:
```python
"""Classify how an NGCC instance hashes, by scanning the sources it is built from.

The ICCS package ships auxfunc.c with sm3hash (a real SM3) and two placeholders,
pseudohash and pseudoXOF, that ICCS documents as "only used for correctness
verification ... does not guarantee security". Timings of an instance that calls
the placeholders include them, which the site notes per parameter set.

  placeholder  some source calls pseudohash() or pseudoXOF()
  sm3          only sm3hash() from the ICCS package is called
  own          neither (the candidate brings its own hash/XOF)
"""

import re
from collections.abc import Iterable
from pathlib import Path

CALL = re.compile(r"\b(pseudohash|pseudoXOF|sm3hash)\s*\(")
COMMENTS = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
AUX_FILES = {"auxfunc.c", "auxfunc.h"}


def hash_usage(paths: Iterable[Path]) -> str:
    found: set[str] = set()
    for p in paths:
        if p.name in AUX_FILES or not p.is_file():
            continue
        text = COMMENTS.sub(" ", p.read_text(errors="replace"))
        found.update(m.group(1) for m in CALL.finditer(text))
    if found & {"pseudohash", "pseudoXOF"}:
        return "placeholder"
    return "sm3" if "sm3hash" in found else "own"
```

- [ ] **Step 7: Write `ngcc_kat.c`**

```c
/*
 * ngcc_kat: regenerate ICCS-format known-answer records from an NGCC instance .so.
 *
 * Our own driver. It replays the seed schedule of the ICCS KAT_SIG.c / KAT_KEM.c
 * generators shipped with every submission, so that its output can be compared
 * with the submitted KAT_SIG_<instance>.txt / KAT_KEM_<instance>.txt (kat_compare.py):
 *   - a seed DRNG initialised with "seed" x 16 yields one 64-byte seed per record;
 *   - drng_algorithm (the instance's DRNG) is re-initialised with that seed;
 *   - signatures sign a message from a DRNG initialised with "msg" x 21 + "m",
 *     56 bytes in record 0 and 8 bytes longer in each later record;
 *   - length variables start at the getter values once, then carry whatever the
 *     implementation wrote, exactly as in the ICCS drivers.
 * Lengths are always printed as numbers; kat_compare.py drops them when the
 * submitted file was generated in template mode.
 *
 * Usage: ngcc_kat sig|kem LIB.so OUT.txt [records]      (records 1-10, default 10)
 * Exit: 0 ok, 1 usage/IO, 2 load failure, 3 crypto failure (error return, a length
 * over its claim, or KEM shared secrets that differ).
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long long ull;
typedef int (*init_fn)(void *drng, const unsigned char *seed, ull seed_len);
typedef int (*rand_fn)(void *drng, unsigned char *out, ull bits);
typedef ull (*len_fn)(void);
typedef int (*keygen_fn)(unsigned char *, ull *, unsigned char *, ull *);
typedef int (*sig_sign_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *);
typedef int (*sig_verify_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull);
typedef int (*kem_enc_fn)(unsigned char *, ull, unsigned char *, ull *, unsigned char *, ull *);
typedef int (*kem_dec_fn)(unsigned char *, ull, unsigned char *, ull, unsigned char *, ull *);

#define SEED_LEN 64
/* Storage for the driver's own DRNG contexts (the ICCS DRNG_ctx is 165 bytes). */
typedef struct { _Alignas(64) unsigned char bytes[4096]; } drng_storage;

static void *lib;

static void *sym(const char *name) {
    void *p = dlsym(lib, name);
    if (!p) {
        fprintf(stderr, "ngcc_kat: missing symbol %s\n", name);
        exit(2);
    }
    return p;
}
#define SYM(type, name) ((type)(uintptr_t)sym(name))

static void pstr(FILE *f, const char *id, const unsigned char *b, ull n) {
    fputs(id, f);
    for (ull i = 0; i < n; i++) fprintf(f, "%02X", b[i]);
    fputc('\n', f);
}

static unsigned char *buf(ull n) {
    unsigned char *p = calloc(n ? n : 1, 1);
    if (!p) { perror("ngcc_kat: calloc"); exit(1); }
    return p;
}

static int fail(const char *what, int record) {
    fprintf(stderr, "ngcc_kat: %s in record %d\n", what, record);
    return 3;
}

static void seed_nonce(unsigned char *nonce) {
    for (int k = 0; k < SEED_LEN / 4; k++) memcpy(nonce + 4 * k, "seed", 4);
}

static int run_sig(FILE *out, int records) {
    init_fn init = SYM(init_fn, "init_random_number");
    rand_fn rnd = SYM(rand_fn, "get_random_number");
    void *drng_alg = sym("drng_algorithm");
    keygen_fn keygen = SYM(keygen_fn, "sig_keygen");
    sig_sign_fn sign = SYM(sig_sign_fn, "sig_sign");
    sig_verify_fn verify = SYM(sig_verify_fn, "sig_verify");
    const ull pk_cap = SYM(len_fn, "sig_get_pk_len_bytes")();
    const ull sk_cap = SYM(len_fn, "sig_get_sk_len_bytes")();
    const ull sn_cap = SYM(len_fn, "sig_get_sn_len_bytes")();

    drng_storage seed_drng, msg_drng;
    unsigned char nonce1[SEED_LEN], nonce2[SEED_LEN] = {0}, seed[SEED_LEN];
    seed_nonce(nonce1);
    for (int k = 0; k < SEED_LEN / 3; k++) memcpy(nonce2 + 3 * k, "msg", 3);
    nonce2[SEED_LEN - 1] = 'm';
    init(&seed_drng, nonce1, SEED_LEN);
    init(&msg_drng, nonce2, SEED_LEN);

    ull pk_len = pk_cap, sk_len = sk_cap, sn_len = sn_cap;
    unsigned char *pk = buf(pk_cap), *sk = buf(sk_cap), *sn = buf(sn_cap), *m = buf(128);
    int m_len = 56;
    for (int i = 0; i < records; i++) {
        fprintf(out, "Count = %d\n", i);
        rnd(&seed_drng, seed, SEED_LEN * 8);
        rnd(&msg_drng, m, (ull)m_len * 8);
        fprintf(out, "Seed_Len = %d\n", SEED_LEN);
        pstr(out, "Seed = ", seed, SEED_LEN);
        init(drng_alg, seed, SEED_LEN);
        if (keygen(pk, &pk_len, sk, &sk_len)) return fail("sig_keygen failed", i);
        if (pk_len > pk_cap || sk_len > sk_cap) return fail("key longer than claimed", i);
        fprintf(out, "PK_Len = %llu\n", pk_len);
        pstr(out, "PK = ", pk, pk_len);
        fprintf(out, "SK_Len = %llu\n", sk_len);
        pstr(out, "SK = ", sk, sk_len);
        fprintf(out, "M_Len = %d\n", m_len);
        pstr(out, "M = ", m, (ull)m_len);
        if (sign(sk, sk_len, m, (ull)m_len, sn, &sn_len)) return fail("sig_sign failed", i);
        if (sn_len > sn_cap) return fail("signature longer than claimed", i);
        fprintf(out, "Sn_Len = %llu\n", sn_len);
        pstr(out, "Sn = ", sn, sn_len);
        if (verify(pk, pk_len, sn, sn_len, m, (ull)m_len)) return fail("sig_verify rejected", i);
        fputc('\n', out);
        fflush(out);
        m_len += 8;
    }
    return 0;
}

static int run_kem(FILE *out, int records) {
    init_fn init = SYM(init_fn, "init_random_number");
    rand_fn rnd = SYM(rand_fn, "get_random_number");
    void *drng_alg = sym("drng_algorithm");
    keygen_fn keygen = SYM(keygen_fn, "kem_keygen");
    kem_enc_fn enc = SYM(kem_enc_fn, "kem_enc");
    kem_dec_fn dec = SYM(kem_dec_fn, "kem_dec");
    const ull pk_cap = SYM(len_fn, "kem_get_pk_len_bytes")();
    const ull sk_cap = SYM(len_fn, "kem_get_sk_len_bytes")();
    const ull ss_cap = SYM(len_fn, "kem_get_ss_len_bytes")();
    const ull ct_cap = SYM(len_fn, "kem_get_ct_len_bytes")();

    drng_storage seed_drng;
    unsigned char nonce[SEED_LEN], seed[SEED_LEN];
    seed_nonce(nonce);
    init(&seed_drng, nonce, SEED_LEN);

    ull pk_len = pk_cap, sk_len = sk_cap, ss_len = ss_cap, ct_len = ct_cap;
    unsigned char *pk = buf(pk_cap), *sk = buf(sk_cap), *ss = buf(ss_cap), *ss1 = buf(ss_cap), *ct = buf(ct_cap);
    for (int i = 0; i < records; i++) {
        fprintf(out, "Count = %d\n", i);
        rnd(&seed_drng, seed, SEED_LEN * 8);
        fprintf(out, "Seed_Len = %d\n", SEED_LEN);
        pstr(out, "Seed = ", seed, SEED_LEN);
        init(drng_alg, seed, SEED_LEN);
        if (keygen(pk, &pk_len, sk, &sk_len)) return fail("kem_keygen failed", i);
        if (pk_len > pk_cap || sk_len > sk_cap) return fail("key longer than claimed", i);
        fprintf(out, "PK_Len = %llu\n", pk_len);
        pstr(out, "PK = ", pk, pk_len);
        fprintf(out, "SK_Len = %llu\n", sk_len);
        pstr(out, "SK = ", sk, sk_len);
        if (enc(pk, pk_len, ss, &ss_len, ct, &ct_len)) return fail("kem_enc failed", i);
        if (ss_len > ss_cap || ct_len > ct_cap) return fail("output longer than claimed", i);
        fprintf(out, "CT_Len = %llu\n", ct_len);
        pstr(out, "CT = ", ct, ct_len);
        fprintf(out, "SS_Len = %llu\n", ss_len);
        pstr(out, "SS = ", ss, ss_len);
        if (dec(sk, sk_len, ct, ct_len, ss1, &ss_len)) return fail("kem_dec failed", i);
        if (ss_len > ss_cap || memcmp(ss, ss1, ss_len)) return fail("shared secrets differ", i);
        fputc('\n', out);
        fflush(out);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5 || (strcmp(argv[1], "sig") && strcmp(argv[1], "kem"))) {
        fprintf(stderr, "usage: ngcc_kat sig|kem LIB.so OUT.txt [records]\n");
        return 1;
    }
    int records = argc == 5 ? atoi(argv[4]) : 10;
    if (records < 1 || records > 10) {
        fprintf(stderr, "ngcc_kat: records must be 1-10\n");
        return 1;
    }
    lib = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        fprintf(stderr, "ngcc_kat: %s\n", dlerror());
        return 2;
    }
    FILE *out = fopen(argv[3], "w");
    if (!out) {
        perror(argv[3]);
        return 1;
    }
    int rc = strcmp(argv[1], "sig") == 0 ? run_sig(out, records) : run_kem(out, records);
    fclose(out);
    return rc;
}
```

- [ ] **Step 8: Run the tests to verify they pass**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests`
Expected: all tests pass (fetch 3, extract 5, gen_rules 8, inventory 1, kat_compare 6, hash_usage 3, ngcc_kat 3).

- [ ] **Step 9: Lint and commit**

```bash
chmod +x bench-common/ngcc/gen_rules.py bench-common/ngcc/inventory.py
uvx ruff check --fix bench-common/ngcc && uvx ruff format bench-common/ngcc
git add bench-common/ngcc/gen_rules.py bench-common/ngcc/inventory.py bench-common/ngcc/ngcc_kat.c bench-common/ngcc/kat_compare.py bench-common/ngcc/hash_usage.py bench-common/ngcc/tests/fixtures bench-common/ngcc/tests/drng_def.c bench-common/ngcc/tests/test_gen_rules.py bench-common/ngcc/tests/test_inventory.py bench-common/ngcc/tests/test_kat_compare.py bench-common/ngcc/tests/test_hash_usage.py bench-common/ngcc/tests/test_ngcc_kat.py
git commit -m "Add the shared NGCC ICCS-API tooling: rules generator, inventory, KAT replay

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 4: Signature suite: generic ICCS SIG shim in `bench/`, best-effort build, `--sizes`, status, sign-01 pilot

**Files:**
- Create: `bench-common/ngcc/ngcc.mk`, `bench-common/ngcc/status.py`
- Create: `bench/schemes/ngcc/{Makefile,shim_template.c,params.tsv,.gitignore,.source}`, `bench/schemes/ngcc/status.tsv` (generated)
- Create fixtures: `bench-common/ngcc/tests/fixtures/params_sig.tsv`, `bench-common/ngcc/tests/fixtures/src/sign-99/KAT/KAT_SIG_Toy-Sig-1.txt`
- Modify: `bench/Makefile` (NGCC stamp, `so_paths.h`, `clean`), `bench/main.c` (`--sizes`)
- Test: `bench-common/ngcc/tests/test_suite_sig.py`

**Interfaces:**
- Consumes: `gen_rules.read_params/resolve_sources/instance_dir`, `kat_compare.compare`, `hash_usage.hash_usage`, `ngcc_kat`.
- Produces: `make -C <suite>/schemes/ngcc [PARAMS=… NGCC_SRC=… BUILD=…]`, which always exits 0 and writes `BUILD/built.list`, plus `make … status [STATUS=path]`. `status.tsv` has the columns `file name id instance impl build kat hash`. `build` ∈ `ok|FAIL`. `kat` ∈ `PASS|MISMATCH|NOKAT|CRYPTOFAIL|LOADFAIL|CRASH|TIMEOUT|-`. `hash` ∈ `placeholder|sm3|own`. `./bench --sizes [filters]` prints the TSV header `scheme pk sk sig`.

- [ ] **Step 1: Write the failing test and fixtures**

`bench-common/ngcc/tests/fixtures/params_sig.tsv`. Create it with printf so the separators are real tabs:
```bash
printf '%s\n' \
 '# Toy signature instances for test_suite_sig.py' \
 "$(printf 'file\tname\tid\tinstance\timpl\tsrcdir\tsrcs\tkat\tdefs')" \
 "$(printf 'toy_sig_1\tNGCC-Toy-Sig-1\tsign-99\tToy-Sig-1\tref\tReference_Implementation/Toy-Sig-1\t*\tKAT/KAT_SIG_Toy-Sig-1.txt\t')" \
 "$(printf 'toy_sig_long\tNGCC-Toy-Sig-Long\tsign-99\tToy-Sig-1\tref\tReference_Implementation/Toy-Sig-1\t*\t-\t-DTOY_LONG_PK')" \
 "$(printf 'toy_missing\tNGCC-Toy-Missing\tsign-99\tToy-Missing\tref\tReference_Implementation/Missing\t*\t-\t')" \
 > bench-common/ngcc/tests/fixtures/params_sig.tsv
```

`bench-common/ngcc/tests/test_suite_sig.py`:
```python
"""Build the toy NGCC signature instances through bench/schemes/ngcc; check the shim and status.py."""

import csv
import ctypes
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
FIX = HERE / "fixtures"
SUITE = REPO / "bench" / "schemes" / "ngcc"


class SigInfo(ctypes.Structure):
    _fields_ = [("name", ctypes.c_char_p), ("pk_bytes", ctypes.c_size_t), ("sk_bytes", ctypes.c_size_t),
                ("sig_bytes", ctypes.c_size_t), ("iters", ctypes.c_int)]


def make(build, *goals):
    return subprocess.run(
        ["make", "-C", str(SUITE), f"PARAMS={FIX / 'params_sig.tsv'}", f"NGCC_SRC={FIX / 'src'}", f"BUILD={build}", *goals],
        capture_output=True, text=True, check=False,
    )


def test_failed_instance_does_not_fail_the_build(tmp_path):
    proc = make(tmp_path)
    assert proc.returncode == 0, proc.stdout + proc.stderr
    assert "FAIL  NGCC-Toy-Missing" in proc.stdout
    built = [Path(line).name for line in (tmp_path / "built.list").read_text().split()]
    assert built == ["toy_sig_1.so", "toy_sig_long.so"]


def test_shim_round_trips_and_rejects_tampering(tmp_path):
    make(tmp_path)
    lib = ctypes.CDLL(str(tmp_path / "toy_sig_1.so"))
    lib.bench_info.restype = ctypes.POINTER(SigInfo)
    info = lib.bench_info().contents
    assert (info.name, info.pk_bytes, info.sk_bytes, info.sig_bytes, info.iters) == (b"NGCC-Toy-Sig-1", 32, 32, 32, 0)
    pk, sk, sig = (ctypes.create_string_buffer(32) for _ in range(3))
    msg, siglen, n = bytes(32), ctypes.c_size_t(), ctypes.c_size_t(32)
    assert lib.crypto_sign_keypair(pk, sk) == 0
    assert lib.crypto_sign_signature(sig, ctypes.byref(siglen), msg, n, sk) == 0
    assert siglen.value == 32
    assert lib.crypto_sign_verify(sig, siglen, msg, n, pk) == 0
    assert lib.crypto_sign_verify(sig, siglen, b"\x01" + msg[1:], n, pk) != 0


def test_overlong_key_is_an_error(tmp_path):
    make(tmp_path)
    lib = ctypes.CDLL(str(tmp_path / "toy_sig_long.so"))
    pk, sk = ctypes.create_string_buffer(64), ctypes.create_string_buffer(64)
    assert lib.crypto_sign_keypair(pk, sk) != 0


def test_status_records_build_kat_and_hash(tmp_path):
    make(tmp_path)
    out = tmp_path / "status.tsv"
    proc = make(tmp_path, "status", f"STATUS={out}")
    assert proc.returncode == 0, proc.stdout + proc.stderr
    rows = {r["name"]: r for r in csv.DictReader(out.open(), delimiter="\t")}
    toy = rows["NGCC-Toy-Sig-1"]
    assert (toy["build"], toy["kat"], toy["hash"], toy["impl"]) == ("ok", "PASS", "own", "ref")
    assert rows["NGCC-Toy-Sig-Long"]["kat"] == "CRYPTOFAIL"
    assert (rows["NGCC-Toy-Missing"]["build"], rows["NGCC-Toy-Missing"]["kat"]) == ("FAIL", "-")
```

- [ ] **Step 2: Run it to verify it fails**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_suite_sig.py`
Expected: FAIL (`make: *** …/bench/schemes/ngcc: No such file or directory`).

- [ ] **Step 3: Write `ngcc.mk`**

```make
# Shared build for China NGCC round-1 candidates, which all implement the ICCS
# public-key C API. Included by bench/schemes/ngcc/Makefile (NGCC_KIND := sig)
# and bench-kem/schemes/ngcc/Makefile (NGCC_KIND := kem). See bench/CLAUDE.md.
#
# Best effort by design: an instance that fails to compile prints FAIL, leaves
# its log in $(BUILD)/<file>.log and is skipped; `make` still exits 0.
# $(BUILD)/built.list names the .so files that exist afterwards, and the suite
# Makefile appends it to SO_PATHS.
NGCC_ROOT   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
.DEFAULT_GOAL := all

NGCC_SRC    ?= $(NGCC_ROOT)/src
PARAMS      ?= params.tsv
BUILD       ?= build
STATUS      ?= status.tsv
# gnu17: ICCS targets GCC 8.5-era C; GCC 15 defaults to C23 (bool keywords, () prototypes).
# -fcommon: several submissions rely on tentative definitions shared between files.
NGCC_CFLAGS ?= -O3 -march=native -mtune=native -fPIC -std=gnu17 -fcommon
SUITE_PREFIX := schemes/ngcc

.PHONY: all sos built-list status clean

all:
	@mkdir -p $(BUILD)
	@if [ -f "$(NGCC_SRC)/.extracted" ]; then \
	    $(MAKE) --no-print-directory -k sos || echo "NGCC: some instances failed to build (FAIL lines above)"; \
	else \
	    echo "NGCC: no extracted sources in $(NGCC_SRC); skipping (bench-common/ngcc/fetch.py + extract.py)"; \
	fi
	@$(MAKE) --no-print-directory built-list

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(BUILD)/rules.mk
endif

$(BUILD)/rules.mk: $(PARAMS) shim_template.c $(NGCC_ROOT)/gen_rules.py $(wildcard $(NGCC_SRC)/.extracted)
	@mkdir -p $(BUILD)
	@python3 $(NGCC_ROOT)/gen_rules.py --params $(PARAMS) --src $(NGCC_SRC) --build $(BUILD) \
	    --suite-dir . --template shim_template.c --out $@

sos: $(NGCC_SOS)

built-list:
	@: > $(BUILD)/built.list
	@for so in $(NGCC_SOS); do \
	    if [ -f $$so ]; then case $$so in /*) echo $$so ;; *) echo "$(SUITE_PREFIX)/$$so" ;; esac; fi; \
	done >> $(BUILD)/built.list
	@echo "NGCC: $$(wc -l < $(BUILD)/built.list) of $(words $(NGCC_SOS)) instances built"

status: built-list
	python3 $(NGCC_ROOT)/status.py --kind $(NGCC_KIND) --params $(PARAMS) --src $(NGCC_SRC) \
	    --build $(BUILD) --suite-dir . --out $(STATUS)

clean:
	rm -rf $(BUILD)
	rm -f *_shim.c
```

- [ ] **Step 4: Write `status.py`**

```python
#!/usr/bin/env python3
"""Record build, KAT and hash status for every NGCC instance of a params.tsv.

  build  ok if BUILD/<file>.so exists, else FAIL (see BUILD/<file>.log)
  kat    our ngcc_kat replay compared with the submitted KAT file: PASS, MISMATCH,
         NOKAT (no submitted file), CRYPTOFAIL, LOADFAIL, CRASH, TIMEOUT; '-' if not built
  hash   placeholder | sm3 | own (hash_usage.py)
  impl   copied from params.tsv (ref | opt)

The output is committed as <suite>/schemes/ngcc/status.tsv and read by the YAML import.
Per-instance KAT timeout: NGCC_KAT_TIMEOUT seconds (default 900).

Usage: status.py --kind sig|kem --params P --src SRC --build BUILD --suite-dir DIR --out status.tsv
"""

import argparse
import csv
import os
import subprocess
import sys
import tempfile
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from gen_rules import instance_dir, read_params, resolve_sources
from hash_usage import hash_usage
from kat_compare import compare

HERE = Path(__file__).resolve().parent
COLUMNS = ["file", "name", "id", "instance", "impl", "build", "kat", "hash"]
EXIT = {2: "LOADFAIL", 3: "CRYPTOFAIL"}
TIMEOUT = int(os.environ.get("NGCC_KAT_TIMEOUT", "900"))


def build_kat_tool(build: Path) -> Path:
    exe, src = build / "ngcc_kat", HERE / "ngcc_kat.c"
    if not exe.exists() or exe.stat().st_mtime < src.stat().st_mtime:
        cc = os.environ.get("CC", "cc")
        subprocess.run([cc, "-O2", "-std=gnu17", "-o", str(exe), str(src), "-ldl"], check=True)
    return exe


def kat_status(exe: Path, kind: str, so: Path, reference: Path | None, records: int) -> str:
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "kat.txt"
        try:
            proc = subprocess.run([str(exe), kind, str(so.resolve()), str(out), str(records)],
                                  cwd=tmp, capture_output=True, timeout=TIMEOUT, check=False)
        except subprocess.TimeoutExpired:
            return "TIMEOUT"
        if proc.returncode < 0:
            return "CRASH"
        if proc.returncode:
            return EXIT.get(proc.returncode, "CRASH")
        if reference is None:
            return "NOKAT"
        return compare(out.read_text(errors="replace"), reference.read_text(errors="replace"), records)


def status_row(row: dict, kind: str, exe: Path, src_root: Path, build: Path, suite_dir: Path) -> dict:
    srcdir = instance_dir(row, src_root)
    headers = sorted(srcdir.glob("*.h")) if srcdir.is_dir() else []
    out = {k: row[k] for k in ("file", "name", "id", "instance", "impl")}
    out["hash"] = hash_usage([*resolve_sources(row, src_root, suite_dir), *headers])
    so = build / f"{row['file']}.so"
    if not so.exists():
        out.update(build="FAIL", kat="-")
        return out
    ref = None if row["kat"] == "-" else src_root / row["id"] / row["kat"]
    out.update(build="ok", kat=kat_status(exe, kind, so, ref if ref and ref.is_file() else None, int(row["katn"])))
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="NGCC build/KAT/hash status")
    ap.add_argument("--kind", choices=("sig", "kem"), required=True)
    for opt in ("--params", "--src", "--build", "--out"):
        ap.add_argument(opt, required=True)
    ap.add_argument("--suite-dir", default=".")
    args = ap.parse_args(argv)

    rows = read_params(Path(args.params))
    src, build, suite = Path(args.src).resolve(), Path(args.build).resolve(), Path(args.suite_dir).resolve()
    exe = build_kat_tool(build)
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        result = list(pool.map(lambda r: status_row(r, args.kind, exe, src, build, suite), rows))
    with open(args.out, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=COLUMNS, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(result)
    print("build:", dict(Counter(r["build"] for r in result)))
    print("kat:  ", dict(Counter(r["kat"] for r in result)))
    print("hash: ", dict(Counter(r["hash"] for r in result)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 5: Write the signature suite**

`bench/schemes/ngcc/Makefile`:
```make
# China NGCC round-1 signature candidates through one generic ICCS SIG shim.
# Instances are data (params.tsv); see bench/CLAUDE.md, "NGCC candidates".
NGCC_KIND := sig
include ../../../bench-common/ngcc/ngcc.mk
```

`bench/schemes/ngcc/shim_template.c`:
```c
/* Shim for @NAME@ (China NGCC @ID@, @IMPL@ implementation). Generated by gen_shims.py
 * from bench/schemes/ngcc/shim_template.c + params.tsv: edit those, not this file.
 *
 * Adapts the ICCS NGCC signature API (SIG_AlgorithmInstance.h) to the bench contract.
 * Sizes come from the instance's own getters. Randomness: the ICCS API draws from the
 * global DRNG drng_algorithm, which this shim defines (unless -DNGCC_NO_DRNG) and
 * seeds once from getrandom() at load time; ngcc_kat re-seeds it to replay the KATs. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/random.h>
#include <sys/types.h>
#ifndef NGCC_INSTANCE_HEADER
#define NGCC_INSTANCE_HEADER "SIG_AlgorithmInstance.h"
#endif
#include NGCC_INSTANCE_HEADER
#include "drng.h"
#include "../../scheme.h"

#ifdef NGCC_NO_DRNG
extern DRNG_ctx drng_algorithm;
#else
DRNG_ctx drng_algorithm;
#endif

typedef unsigned long long ull;
static bench_scheme_info_t INFO = { "@NAME@", 0, 0, 0, @ITERS@ };
static ull PK_LEN, SK_LEN, SN_LEN;

__attribute__((constructor)) static void ngcc_shim_init(void) {
    unsigned char seed[64];
    if (getrandom(seed, sizeof seed, 0) != (ssize_t)sizeof seed) abort();
    init_random_number(&drng_algorithm, seed, sizeof seed);
    PK_LEN = sig_get_pk_len_bytes();
    SK_LEN = sig_get_sk_len_bytes();
    SN_LEN = sig_get_sn_len_bytes();
    INFO.pk_bytes = (size_t)PK_LEN;
    INFO.sk_bytes = (size_t)SK_LEN;
    INFO.sig_bytes = (size_t)SN_LEN;
}

const bench_scheme_info_t *bench_info(void) { return &INFO; }

/* ICCS lengths go in and out. They start at the claimed sizes; an implementation
 * that reports more than it claimed has overrun our buffer, which is an error. */
int crypto_sign_keypair(uint8_t *pk, uint8_t *sk) {
    ull pk_len = PK_LEN, sk_len = SK_LEN;
    if (sig_keygen(pk, &pk_len, sk, &sk_len) != 0) return -1;
    return (pk_len <= PK_LEN && sk_len <= SK_LEN) ? 0 : -1;
}

int crypto_sign_signature(uint8_t *sig, size_t *siglen, const uint8_t *m, size_t mlen, const uint8_t *sk) {
    ull sn_len = SN_LEN;
    if (sig_sign((unsigned char *)sk, SK_LEN, (unsigned char *)m, mlen, sig, &sn_len) != 0) return -1;
    if (sn_len > SN_LEN) return -1;
    *siglen = (size_t)sn_len;
    return 0;
}

int crypto_sign_verify(const uint8_t *sig, size_t siglen, const uint8_t *m, size_t mlen, const uint8_t *pk) {
    return sig_verify((unsigned char *)pk, PK_LEN, (unsigned char *)sig, siglen, (unsigned char *)m, mlen) == 0 ? 0 : -1;
}
```

`bench/schemes/ngcc/.gitignore`:
```
*_shim.c
```

`bench/schemes/ngcc/.source` (one line; `run_bench.sh` records it under `# sources:`):
```
https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/
```

`bench/schemes/ngcc/params.tsv` (header only for now):
```bash
{ echo '# China NGCC round-1 signature instances. Columns: see bench-common/ngcc/gen_rules.py.'
  echo '# One row per ALGORITHM_INSTANCE. Explain non-obvious srcs/defs/patches in a comment line above the row.'
  printf 'file\tname\tid\tinstance\timpl\tsrcdir\tsrcs\tincs\tdefs\tshimdefs\tldlibs\titers\tkatn\tkat\n'
} > bench/schemes/ngcc/params.tsv
```

- [ ] **Step 6: Generate the toy reference KAT (template mode) and run the suite tests**

```bash
make -C bench/schemes/ngcc PARAMS=$PWD/bench-common/ngcc/tests/fixtures/params_sig.tsv NGCC_SRC=$PWD/bench-common/ngcc/tests/fixtures/src BUILD=/tmp/ngcc-toy-sig
cc -O2 -std=gnu17 -o /tmp/ngcc-toy-sig/ngcc_kat bench-common/ngcc/ngcc_kat.c -ldl
/tmp/ngcc-toy-sig/ngcc_kat sig /tmp/ngcc-toy-sig/toy_sig_1.so /tmp/ngcc-toy-sig/kat.txt
mkdir -p bench-common/ngcc/tests/fixtures/src/sign-99/KAT
sed -E 's/^(PK|SK|Sn)_Len = .*/\1_Len = /' /tmp/ngcc-toy-sig/kat.txt > bench-common/ngcc/tests/fixtures/src/sign-99/KAT/KAT_SIG_Toy-Sig-1.txt
uv run --with pytest pytest -q bench-common/ngcc/tests/test_suite_sig.py
```
Expected: the `make` prints `NGCC  NGCC-Toy-Sig-1`, `FAIL  NGCC-Toy-Missing: no sources …`, `NGCC: 2 of 3 instances built`; pytest `4 passed`.

- [ ] **Step 7: Wire the suite into `bench/Makefile`**

After the `SCHEME_DIRS = …` line, add:
```make
# China NGCC round-1 candidates: one generic ICCS-API shim, data-driven from
# schemes/ngcc/params.tsv. Best effort: instances that fail to build are skipped,
# and only the .so files in schemes/ngcc/build/built.list are benchmarked.
NGCC_DIR    = schemes/ngcc
NGCC_STAMP  = $(NGCC_DIR)/.stamp
```
After the `schemes/sqisign/.stamp: …` prerequisite line, add:
```make
$(NGCC_STAMP): $(NGCC_DIR)/Makefile $(NGCC_DIR)/shim_template.c $(NGCC_DIR)/params.tsv \
    ../bench-common/ngcc/ngcc.mk ../bench-common/ngcc/gen_rules.py \
    $(wildcard $(NGCC_DIR)/patches/*/*) $(wildcard ../bench-common/ngcc/src/.extracted)
	$(MAKE) -C $(NGCC_DIR) CC=$(CC)
	touch $@
```
Replace the `so_paths.h` rule with:
```make
$(BUILD)/so_paths.h: Makefile $(NGCC_STAMP) | $(BUILD)
	@printf '"%s",\n' $(ALL_SOS) > $@
	@sed 's/.*/"&",/' $(NGCC_DIR)/build/built.list >> $@
```
In `clean:` add the two lines:
```make
	$(MAKE) -C $(NGCC_DIR) clean
	rm -f $(NGCC_STAMP)
```

- [ ] **Step 8: Add `--sizes` to `bench/main.c`**

Replace the first two lines of `main` (`int nfilters = argc - 1;` / `char **filters = argv + 1;`) with:
```c
    /* --sizes: print the claimed sizes from bench_info() as TSV and exit without
     * running anything. Used for the NGCC import (results/ngcc_sizes.tsv). */
    int sizes_only = argc > 1 && strcmp(argv[1], "--sizes") == 0;
    int nfilters = argc - 1 - sizes_only;
    char **filters = argv + 1 + sizes_only;

    if (sizes_only) {
        printf("scheme\tpk\tsk\tsig\n");
        for (int i = 0; SO_PATHS[i]; i++) {
            bench_scheme_t *s = bench_load(SO_PATHS[i]);
            if (!s) continue;
            if (scheme_matches(s->name, nfilters, filters))
                printf("%s\t%zu\t%zu\t%zu\n", s->name, s->pk_bytes, s->sk_bytes, s->sig_bytes);
            bench_unload(s);
        }
        return 0;
    }
```

Run: `make -C bench -j"$(nproc)" && (cd bench && ./bench --sizes mldsa)`
Expected: the NGCC sub-make prints `NGCC: 0 of 0 instances built`. The output is the header plus three TSV lines (`ML-DSA-44`, `ML-DSA-65`, `ML-DSA-87`) whose pk/sk/sig values equal `bench/schemes/mldsa/params.tsv`. `./bench mldsa` still prints the usual timing table.

- [ ] **Step 9 (needs Task 2 Step 6): Pilot with sign-01 (Aigis-Sig+)**

```bash
python3 bench-common/ngcc/inventory.py sig sign-01 | tee /tmp/sign-01.tsv
```
Append to `bench/schemes/ngcc/params.tsv` the `impl=ref` rows only (skip the header line), one per instance. Then:
```bash
make -C bench/schemes/ngcc -j"$(nproc)"
```
For every `FAIL`, read `bench/schemes/ngcc/build/<file>.log` and fix the row using the triage table in Task 6 Step 3. Usually this means dropping the `/dev/urandom` `randombytes.c` in favour of the DRNG-backed `rng.c`, or adding a `kat_test` include. Then:
```bash
make -C bench/schemes/ngcc status && cat bench/schemes/ngcc/status.tsv
make -C bench -j"$(nproc)" && (cd bench && BENCH_ITER=20 ./bench NGCC-Aigis)
```
Expected: every sign-01 row `build=ok`, ideally `kat=PASS`, and the bench prints one timing row per Aigis instance without `FAIL` lines. If KATs mismatch after 30 minutes of triage, keep the recorded status and move on, and note it in the commit message.

- [ ] **Step 10: Lint and commit**

```bash
chmod +x bench-common/ngcc/status.py
uvx ruff check --fix bench-common/ngcc && uvx ruff format bench-common/ngcc
git add bench-common/ngcc/ngcc.mk bench-common/ngcc/status.py bench-common/ngcc/tests/test_suite_sig.py bench-common/ngcc/tests/fixtures/params_sig.tsv bench-common/ngcc/tests/fixtures/src/sign-99/KAT/KAT_SIG_Toy-Sig-1.txt bench/schemes/ngcc/Makefile bench/schemes/ngcc/shim_template.c bench/schemes/ngcc/params.tsv bench/schemes/ngcc/.gitignore bench/schemes/ngcc/.source bench/schemes/ngcc/status.tsv bench/Makefile bench/main.c
git commit -m "Benchmark NGCC signatures through one generic ICCS SIG shim (sign-01 pilot)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 5: KEM suite: generic ICCS KEM shim in `bench-kem/`, `--sizes`, kem-01 pilot

**Files:**
- Create: `bench-kem/schemes/ngcc/{Makefile,shim_template.c,params.tsv,.gitignore,.source,status.tsv}`
- Create fixtures: `bench-common/ngcc/tests/fixtures/params_kem.tsv`, `bench-common/ngcc/tests/fixtures/src/kem-99/KAT/KAT_KEM_Toy-KEM-1.txt`
- Modify: `bench-kem/Makefile`, `bench-kem/main.c`
- Test: `bench-common/ngcc/tests/test_suite_kem.py`

**Interfaces:**
- Consumes: `ngcc.mk`, `status.py` (Task 4).
- Produces: `./bench-kem --sizes [filters]`, which prints the TSV header `scheme pk sk ct ss`. Also `bench-kem/schemes/ngcc/status.tsv` with the same columns as the signature one.

- [ ] **Step 1: Write the failing test and fixture params**

```bash
printf '%s\n' \
 '# Toy KEM instances for test_suite_kem.py' \
 "$(printf 'file\tname\tid\tinstance\timpl\tsrcdir\tsrcs\tkat')" \
 "$(printf 'toy_kem_1\tNGCC-Toy-KEM-1\tkem-99\tToy-KEM-1\tref\tReference_Implementation/Toy-KEM-1\t*\tKAT/KAT_KEM_Toy-KEM-1.txt')" \
 "$(printf 'toy_kem_missing\tNGCC-Toy-KEM-Missing\tkem-99\tToy-KEM-Missing\tref\tReference_Implementation/Missing\t*\t-')" \
 > bench-common/ngcc/tests/fixtures/params_kem.tsv
```

`bench-common/ngcc/tests/test_suite_kem.py`:
```python
"""Build the toy NGCC KEM instance through bench-kem/schemes/ngcc; check the shim and status.py."""

import csv
import ctypes
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
FIX = HERE / "fixtures"
SUITE = REPO / "bench-kem" / "schemes" / "ngcc"


class KemInfo(ctypes.Structure):
    _fields_ = [("name", ctypes.c_char_p), ("pk_bytes", ctypes.c_size_t), ("sk_bytes", ctypes.c_size_t),
                ("ct_bytes", ctypes.c_size_t), ("ss_bytes", ctypes.c_size_t), ("iters", ctypes.c_int)]


def make(build, *goals):
    return subprocess.run(
        ["make", "-C", str(SUITE), f"PARAMS={FIX / 'params_kem.tsv'}", f"NGCC_SRC={FIX / 'src'}", f"BUILD={build}", *goals],
        capture_output=True, text=True, check=False,
    )


def test_failed_instance_does_not_fail_the_build(tmp_path):
    proc = make(tmp_path)
    assert proc.returncode == 0, proc.stdout + proc.stderr
    assert "FAIL  NGCC-Toy-KEM-Missing" in proc.stdout
    assert [Path(p).name for p in (tmp_path / "built.list").read_text().split()] == ["toy_kem_1.so"]


def test_shim_round_trips(tmp_path):
    make(tmp_path)
    lib = ctypes.CDLL(str(tmp_path / "toy_kem_1.so"))
    lib.bench_info.restype = ctypes.POINTER(KemInfo)
    info = lib.bench_info().contents
    assert (info.name, info.pk_bytes, info.sk_bytes, info.ct_bytes, info.ss_bytes) == (b"NGCC-Toy-KEM-1", 32, 32, 32, 32)
    pk, sk, ct, ss1, ss2 = (ctypes.create_string_buffer(32) for _ in range(5))
    assert lib.crypto_kem_keypair(pk, sk) == 0
    assert lib.crypto_kem_enc(ct, ss1, pk) == 0
    assert lib.crypto_kem_dec(ss2, ct, sk) == 0
    assert ss1.raw == ss2.raw


def test_status_records_build_and_kat(tmp_path):
    make(tmp_path)
    out = tmp_path / "status.tsv"
    assert make(tmp_path, "status", f"STATUS={out}").returncode == 0
    rows = {r["name"]: r for r in csv.DictReader(out.open(), delimiter="\t")}
    assert (rows["NGCC-Toy-KEM-1"]["build"], rows["NGCC-Toy-KEM-1"]["kat"]) == ("ok", "PASS")
    assert rows["NGCC-Toy-KEM-Missing"]["build"] == "FAIL"
```

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_suite_kem.py`
Expected: FAIL (no `bench-kem/schemes/ngcc`).

- [ ] **Step 2: Write the KEM suite**

`bench-kem/schemes/ngcc/Makefile`:
```make
# China NGCC round-1 KEM candidates through one generic ICCS KEM shim.
# Instances are data (params.tsv); see bench/CLAUDE.md, "NGCC candidates".
NGCC_KIND := kem
include ../../../bench-common/ngcc/ngcc.mk
```

`bench-kem/schemes/ngcc/shim_template.c`:
```c
/* Shim for @NAME@ (China NGCC @ID@, @IMPL@ implementation). Generated by gen_shims.py
 * from bench-kem/schemes/ngcc/shim_template.c + params.tsv: edit those, not this file.
 *
 * Adapts the ICCS NGCC KEM API (KEM_AlgorithmInstance.h) to the bench-kem contract.
 * Sizes come from the instance's getters; drng_algorithm is defined here (unless
 * -DNGCC_NO_DRNG) and seeded once from getrandom(). The ICCS API passes lengths
 * the NIST-style contract lacks: the actual lengths from the last keygen / encaps
 * are kept and passed on, which is sound because bench-kem always runs
 * keygen -> encaps -> decaps in that order on the same buffers. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/random.h>
#include <sys/types.h>
#ifndef NGCC_INSTANCE_HEADER
#define NGCC_INSTANCE_HEADER "KEM_AlgorithmInstance.h"
#endif
#include NGCC_INSTANCE_HEADER
#include "drng.h"
#include "../../scheme.h"

#ifdef NGCC_NO_DRNG
extern DRNG_ctx drng_algorithm;
#else
DRNG_ctx drng_algorithm;
#endif

typedef unsigned long long ull;
static bench_scheme_info_t INFO = { "@NAME@", 0, 0, 0, 0, @ITERS@ };
static ull PK_LEN, SK_LEN, CT_LEN, SS_LEN;
static ull g_pk_len, g_sk_len, g_ct_len;

__attribute__((constructor)) static void ngcc_shim_init(void) {
    unsigned char seed[64];
    if (getrandom(seed, sizeof seed, 0) != (ssize_t)sizeof seed) abort();
    init_random_number(&drng_algorithm, seed, sizeof seed);
    PK_LEN = kem_get_pk_len_bytes();
    SK_LEN = kem_get_sk_len_bytes();
    CT_LEN = kem_get_ct_len_bytes();
    SS_LEN = kem_get_ss_len_bytes();
    g_pk_len = PK_LEN;
    g_sk_len = SK_LEN;
    g_ct_len = CT_LEN;
    INFO.pk_bytes = (size_t)PK_LEN;
    INFO.sk_bytes = (size_t)SK_LEN;
    INFO.ct_bytes = (size_t)CT_LEN;
    INFO.ss_bytes = (size_t)SS_LEN;
}

const bench_scheme_info_t *bench_info(void) { return &INFO; }

int crypto_kem_keypair(uint8_t *pk, uint8_t *sk) {
    ull pk_len = PK_LEN, sk_len = SK_LEN;
    if (kem_keygen(pk, &pk_len, sk, &sk_len) != 0 || pk_len > PK_LEN || sk_len > SK_LEN) return -1;
    g_pk_len = pk_len;
    g_sk_len = sk_len;
    return 0;
}

int crypto_kem_enc(uint8_t *ct, uint8_t *ss, const uint8_t *pk) {
    ull ss_len = SS_LEN, ct_len = CT_LEN;
    if (kem_enc((unsigned char *)pk, g_pk_len, ss, &ss_len, ct, &ct_len) != 0) return -1;
    if (ss_len > SS_LEN || ct_len > CT_LEN) return -1;
    g_ct_len = ct_len;
    return 0;
}

int crypto_kem_dec(uint8_t *ss, const uint8_t *ct, const uint8_t *sk) {
    ull ss_len = SS_LEN;
    if (kem_dec((unsigned char *)sk, g_sk_len, (unsigned char *)ct, g_ct_len, ss, &ss_len) != 0) return -1;
    return ss_len <= SS_LEN ? 0 : -1;
}
```

`bench-kem/schemes/ngcc/.gitignore` and `.source`: same content as the signature suite. `params.tsv`: same header-creation command as Task 4 Step 5, writing to `bench-kem/schemes/ngcc/params.tsv` with the first comment saying "KEM instances".

- [ ] **Step 3: Toy reference KAT and tests**

```bash
make -C bench-kem/schemes/ngcc PARAMS=$PWD/bench-common/ngcc/tests/fixtures/params_kem.tsv NGCC_SRC=$PWD/bench-common/ngcc/tests/fixtures/src BUILD=/tmp/ngcc-toy-kem
cc -O2 -std=gnu17 -o /tmp/ngcc-toy-kem/ngcc_kat bench-common/ngcc/ngcc_kat.c -ldl
/tmp/ngcc-toy-kem/ngcc_kat kem /tmp/ngcc-toy-kem/toy_kem_1.so /tmp/ngcc-toy-kem/kat.txt
mkdir -p bench-common/ngcc/tests/fixtures/src/kem-99/KAT
sed -E 's/^(PK|SK|CT|SS)_Len = .*/\1_Len = /' /tmp/ngcc-toy-kem/kat.txt > bench-common/ngcc/tests/fixtures/src/kem-99/KAT/KAT_KEM_Toy-KEM-1.txt
uv run --with pytest pytest -q bench-common/ngcc/tests
```
Expected: every test passes (the KEM suite adds 3).

- [ ] **Step 4: Wire into `bench-kem/Makefile` and add `--sizes`**

Apply to `bench-kem/Makefile` exactly the Task 4 Step 7 edits. Put `NGCC_DIR`/`NGCC_STAMP` after `SCHEME_DIRS`, put the stamp rule after the `schemes/mike/.stamp:` prerequisite line, and make the same `so_paths.h` and `clean` changes.

In `bench-kem/main.c`, replace `int nfilters = argc - 1;` / `char **filters = argv + 1;` with:
```c
    /* --sizes: print the claimed sizes from bench_info() as TSV and exit without
     * running anything. Used for the NGCC import (results/ngcc_sizes.tsv). */
    int sizes_only = argc > 1 && strcmp(argv[1], "--sizes") == 0;
    int nfilters = argc - 1 - sizes_only;
    char **filters = argv + 1 + sizes_only;

    if (sizes_only) {
        printf("scheme\tpk\tsk\tct\tss\n");
        for (int i = 0; SO_PATHS[i]; i++) {
            bench_scheme_t *s = bench_load(SO_PATHS[i]);
            if (!s) continue;
            if (scheme_matches(s->name, nfilters, filters))
                printf("%s\t%zu\t%zu\t%zu\t%zu\n", s->name, s->pk_bytes, s->sk_bytes, s->ct_bytes, s->ss_bytes);
            bench_unload(s);
        }
        return 0;
    }
```
Run: `make -C bench-kem -j"$(nproc)" && (cd bench-kem && ./bench-kem --sizes mlkem)`
Expected: the header plus three ML-KEM rows whose sizes equal `bench-kem/schemes/mlkem/params.tsv` (ML-KEM-768: pk 1184, ct 1088, ss 32).

- [ ] **Step 5 (needs Task 2): Pilot with kem-01 (Aigis-Enc+)**

Same procedure as Task 4 Step 9, using `inventory.py kem kem-01`, `bench-kem/schemes/ngcc`, and `cd bench-kem && BENCH_ITER=20 ./bench-kem NGCC-Aigis`. Expected: `ok` column `yes` for every Aigis-Enc+ instance.

- [ ] **Step 6: Commit**

```bash
git add bench-common/ngcc/tests/test_suite_kem.py bench-common/ngcc/tests/fixtures/params_kem.tsv bench-common/ngcc/tests/fixtures/src/kem-99/KAT/KAT_KEM_Toy-KEM-1.txt bench-kem/schemes/ngcc/Makefile bench-kem/schemes/ngcc/shim_template.c bench-kem/schemes/ngcc/params.tsv bench-kem/schemes/ngcc/.gitignore bench-kem/schemes/ngcc/.source bench-kem/schemes/ngcc/status.tsv bench-kem/Makefile bench-kem/main.c
git commit -m "Benchmark NGCC KEMs through one generic ICCS KEM shim (kem-01 pilot)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 6: Signature manifest for all 34 candidates (LONG)

**Files:**
- Modify: `bench/schemes/ngcc/params.tsv`, `bench/schemes/ngcc/status.tsv`
- Create: `bench/schemes/ngcc/patches/<id>/…` (only where needed), `bench/results/ngcc_sizes.tsv`
- Test: `bench-common/ngcc/tests/test_manifests.py`

**Interfaces:**
- Produces: one `impl=ref` row per submitted reference instance. `bench/results/ngcc_sizes.tsv` is the output of `./bench --sizes NGCC` (header `scheme pk sk sig`, one line per built instance). `status.tsv` has one row per params row, in the same order.

- [ ] **Step 1: Write the failing manifest test**

`bench-common/ngcc/tests/test_manifests.py`:
```python
"""Consistency of the committed NGCC manifests, status files and size files."""

import csv
from pathlib import Path

import pytest

from gen_rules import read_params

REPO = Path(__file__).resolve().parents[3]
SUITES = {"sig": (REPO / "bench", 34, "sign"), "kem": (REPO / "bench-kem", 41, "kem")}
POPULATED = ["sig"]
KAT = {"PASS", "MISMATCH", "NOKAT", "CRYPTOFAIL", "LOADFAIL", "CRASH", "TIMEOUT", "-"}


def tsv(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader([ln for ln in f if not ln.startswith("#")], delimiter="\t"))


@pytest.mark.parametrize("kind", POPULATED)
def test_manifest_covers_every_candidate(kind):
    suite, n, prefix = SUITES[kind]
    rows = read_params(suite / "schemes/ngcc/params.tsv")
    assert sorted({r["id"] for r in rows}) == [f"{prefix}-{i:02d}" for i in range(1, n + 1)]
    instances = [(r["id"], r["instance"]) for r in rows]
    assert len(instances) == len(set(instances)), "one row per instance"


@pytest.mark.parametrize("kind", POPULATED)
def test_status_matches_manifest(kind):
    suite = SUITES[kind][0]
    rows = read_params(suite / "schemes/ngcc/params.tsv")
    status = tsv(suite / "schemes/ngcc/status.tsv")
    assert [s["name"] for s in status] == [r["name"] for r in rows]
    assert {s["build"] for s in status} <= {"ok", "FAIL"}
    assert {s["kat"] for s in status} <= KAT
    assert {s["hash"] for s in status} <= {"placeholder", "sm3", "own"}
    assert all((s["build"] == "FAIL") == (s["kat"] == "-") for s in status)


@pytest.mark.parametrize("kind", POPULATED)
def test_sizes_cover_exactly_the_built_instances(kind):
    suite = SUITES[kind][0]
    built = {s["name"] for s in tsv(suite / "schemes/ngcc/status.tsv") if s["build"] == "ok"}
    sizes = tsv(suite / "results/ngcc_sizes.tsv")
    assert {r["scheme"] for r in sizes} == built
    assert all(int(r["pk"]) > 0 for r in sizes)
```

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_manifests.py`
Expected: FAIL (only sign-01 is in the manifest, and there is no sizes file).

- [ ] **Step 2: Draft rows for every signature candidate**

```bash
python3 bench-common/ngcc/inventory.py sig > /tmp/ngcc-sig-draft.tsv
awk -F'\t' 'NR>1 {print $3, $5}' /tmp/ngcc-sig-draft.tsv | sort | uniq -c
```
Keep the `impl=ref` rows, one per `instance`, for `sign-02` … `sign-34`, and append them to `bench/schemes/ngcc/params.tsv` (sign-01 is already there). When a candidate has `other` rows and no `ref` rows (non-standard layout), read its tree and pick the reference implementation by hand. If one instance occurs twice inside the reference tree (e.g. a copy under a test dir), keep the one the submission's own build files reference. Read those build files; never run them. As a cross-check only, compare the instance count per candidate with `.superpowers/spec/ngcc/data/parameters.csv`:
```bash
python3 - <<'EOF'
import csv, collections
ours = collections.Counter(r["id"] for r in csv.DictReader([l for l in open("bench/schemes/ngcc/params.tsv") if not l.startswith("#")], delimiter="\t"))
theirs = collections.Counter(r["ID"] for r in csv.DictReader(open(".superpowers/spec/ngcc/data/parameters.csv"), delimiter=";") if r["ID"].startswith("sign-"))
for k in sorted(set(ours) | set(theirs)):
    if ours[k] != theirs[k]: print(k, "ours", ours[k], "cross-check", theirs[k])
EOF
```
Investigate each difference in the extracted tree. Never add or drop rows because the cross-check list says so.

- [ ] **Step 3: Build, triage, repeat (time-box: 30 minutes per candidate)**

Run: `make -C bench/schemes/ngcc -j"$(nproc)" 2>&1 | grep -E 'FAIL|instances built'`

For each `FAIL`, read `bench/schemes/ngcc/build/<file>.log` and apply the matching fix in that row:

| Symptom in the log | Fix |
|---|---|
| `multiple definition of 'randombytes'` (a `randombytes.c` on /dev/urandom next to a DRNG-backed `rng.c`) | `srcs`: `-randombytes.c` (randomness must come from `drng_algorithm`) |
| `multiple definition of 'main'` | the driver filter missed it: `-thatfile.c` |
| `multiple definition of 'crypto_sign_keypair'` (or `_signature` / `_verify`) | the candidate ships a NIST-API wrapper: drop it (`-api.c`), or if its ICCS code calls it, rename in `defs`: `-Dcrypto_sign_keypair=cand_crypto_sign_keypair` (defs never reach the shim) |
| `multiple definition of 'drng_algorithm'` | `shimdefs`: `-DNGCC_NO_DRNG` |
| `X.h: No such file or directory` | `incs` (relative to srcdir) |
| shim cannot find `SIG_AlgorithmInstance.h` | `incs` for its dir, or `shimdefs`: `-DNGCC_INSTANCE_HEADER='"sub/SIG_AlgorithmInstance.h"'` |
| parameter set chosen by a `-D` (e.g. `-DPARAMS=2`) | `defs`, and the same flag in `shimdefs` if the instance header needs it |
| `undefined reference to '__gmpz_…'` / `EVP_…` | `ldlibs`: `-lgmp` / `-lcrypto`, and make sure the `-dev` package is in the `scripts/aws-bench.sh` apt list |
| NEON/other-arch `.S` picked up by `*` | `-file.S` |
| a real bug needs a code change | copy the file to `bench/schemes/ngcc/patches/<id>/<file>`, start it with `/* PATCH (nist-sigs-zoo): <why> */`, use `* -file.c patches/<id>/file.c` (and `incs patches/<id>` for headers). Never edit `src/` |
| C++ sources, or no fix within the time-box | leave it failing; add a comment line above the row: `# <id> <instance>: not built: <reason>` |

Repeat until each remaining `FAIL` has a comment line.

- [ ] **Step 4: KAT status, then triage mismatches (same time-box)**

Run: `make -C bench/schemes/ngcc status`
The command prints counts. For each `MISMATCH`, check three things. Is randomness from `drng_algorithm` (a `randombytes` on /dev/urandom or `rand()` causes mismatches)? Is it the right KAT file for this instance (`kat` column)? Are the parameter `defs` right? `CRYPTOFAIL`/`CRASH` are recorded as they are: they are facts about the submitted code in our build. Slow instances: set `katn` to 1–3 (comment why). Re-run `make status` after fixes.

- [ ] **Step 5: Emit sizes with our harness and cross-check**

```bash
make -C bench -j"$(nproc)"
(cd bench && ./bench --sizes NGCC > results/ngcc_sizes.tsv)
wc -l bench/results/ngcc_sizes.tsv
python3 - <<'EOF'
import csv
ours = {r["scheme"].removeprefix("NGCC-"): r for r in csv.DictReader(open("bench/results/ngcc_sizes.tsv"), delimiter="\t")}
theirs = {r["Instance"]: r for r in csv.DictReader(open(".superpowers/spec/ngcc/data/parameters.csv"), delimiter=";") if r["ID"].startswith("sign-")}
for name, r in sorted(ours.items()):
    t = theirs.get(name)
    if t is None:
        print("not in cross-check list:", name); continue
    for k, tk in (("pk", "PublicKeyBytes"), ("sk", "SecretKeyBytes"), ("sig", "SignatureBytes")):
        if r[k] != t[tk]: print(f"{name}: {k} ours={r[k]} cross-check={t[tk]}")
EOF
```
Expected: the line count equals 1 + the number of `build=ok` rows. Investigate every cross-check difference in our build: is it the right instance, are the defs right? Our value stays unless our build was wrong. List remaining differences in the commit message.

- [ ] **Step 6: Run the manifest tests**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests`
Expected: all pass.

- [ ] **Step 7: Commit**

```bash
git add bench/schemes/ngcc/params.tsv bench/schemes/ngcc/status.tsv bench/results/ngcc_sizes.tsv bench-common/ngcc/tests/test_manifests.py
git add bench/schemes/ngcc/patches 2>/dev/null || true
git commit -m "Build all 34 NGCC signature candidates; record build/KAT status and sizes

<summary: N of M instances built, KAT PASS/MISMATCH/… counts, remaining cross-check differences>

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
(Replace the summary line with the real counts printed by `make status`.)

---

### Task 7: KEM manifest for all 41 candidates (LONG)

**Files:**
- Modify: `bench-kem/schemes/ngcc/params.tsv`, `bench-kem/schemes/ngcc/status.tsv`, `bench-common/ngcc/tests/test_manifests.py`
- Create: `bench-kem/schemes/ngcc/patches/<id>/…` (only where needed), `bench-kem/results/ngcc_sizes.tsv`

**Interfaces:**
- Produces: the same artefacts as Task 6 for `bench-kem` (sizes header `scheme pk sk ct ss`).

- [ ] **Step 1: Make the manifest test cover KEMs (it fails first)**

In `test_manifests.py` change `POPULATED = ["sig"]` to `POPULATED = ["sig", "kem"]`.
Run: `uv run --with pytest pytest -q bench-common/ngcc/tests/test_manifests.py`
Expected: the `kem` cases FAIL.

- [ ] **Step 2: Draft, build, triage, status**

Repeat Task 6 Steps 2–4 with these substitutions: `inventory.py kem`, `bench-kem/schemes/ngcc`, `kem-02 … kem-41` (kem-01 is in from the pilot), IDs starting `kem-` in the cross-check scripts, and the symbol names `crypto_kem_keypair/enc/dec` in the triage table. KEM-specific: a `CRYPTOFAIL` with "shared secrets differ" in a single record can be a legitimate decryption failure for schemes with a non-zero DFR. Re-run `make status`. If it does not recur, keep it and note it in the commit message.

- [ ] **Step 3: Emit sizes and cross-check**

```bash
make -C bench-kem -j"$(nproc)"
(cd bench-kem && ./bench-kem --sizes NGCC > results/ngcc_sizes.tsv)
python3 - <<'EOF'
import csv
ours = {r["scheme"].removeprefix("NGCC-"): r for r in csv.DictReader(open("bench-kem/results/ngcc_sizes.tsv"), delimiter="\t")}
theirs = {r["Instance"]: r for r in csv.DictReader(open(".superpowers/spec/ngcc/data/parameters.csv"), delimiter=";") if r["ID"].startswith("kem-")}
for name, r in sorted(ours.items()):
    t = theirs.get(name)
    if t is None:
        print("not in cross-check list:", name); continue
    for k, tk in (("pk", "PublicKeyBytes"), ("sk", "SecretKeyBytes"), ("ct", "CiphertextBytes"), ("ss", "SharedSecretBytes")):
        if r[k] != t[tk]: print(f"{name}: {k} ours={r[k]} cross-check={t[tk]}")
EOF
```

- [ ] **Step 4: Tests and commit**

Run: `uv run --with pytest pytest -q bench-common/ngcc/tests` (expected: all pass), then:
```bash
git add bench-kem/schemes/ngcc/params.tsv bench-kem/schemes/ngcc/status.tsv bench-kem/results/ngcc_sizes.tsv bench-common/ngcc/tests/test_manifests.py
git add bench-kem/schemes/ngcc/patches 2>/dev/null || true
git commit -m "Build all 41 NGCC KEM candidates; record build/KAT status and sizes

<summary with the real counts>

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 8: Claims table from the official specifications (THOM REVIEWS)

**Files:**
- Create: `data/ngcc/candidates.yaml`
- Test: `src/lib/__tests__/ngccCandidates.test.ts`

**Interfaces:**
- Produces: `data/ngcc/candidates.yaml`, a list of candidates. Each has `id` (`sign-NN`/`kem-NN`), `kind` (`signature`|`kem`), `name` (official name from `data/{sign,kem}.csv`), `file` (YAML basename without `.yaml`), `website` (the official NICCS page = `page` column of `archives.tsv`), `category` (site family), `assumption`, `spec` (archive path of the spec PDF) and `instances`. Each instance has `instance` (= `ALGORITHM_INSTANCE`, as in `params.tsv`) and `ref` (spec section/table/page), plus either `ngcc_level` (128/256/384/512) with optional `recommended: true`, or `omit: "<reason>"`. Optionally `pk`, `sig`|`ct` and `size_ref` are given for instances we cannot build. Consumed by Task 9 (`ngcc_data.py`) and Task 14.

**Rules for the curation (put the short form in the file header):**
- `ngcc_level` is the NGCC target the submitters say the instance is designed to meet: 128, 256 or 512 (required targets) or 384 (optional). It is read from the spec's parameter/security table. If the spec states a bit-security estimate other than these, use the target the submitters assign it to. If they assign none (e.g. an intermediate 192-bit set), set `omit: "Intermediate <n>-bit set; not an NGCC target (spec <ref>)"`. **(Thom to confirm this rule.)**
- `recommended: true` only where the spec designates a recommended/primary/default set for a target. If the spec designates none, no instance of that candidate gets it.
- `category` uses the site's families with these precedents: lattice (LWE/LWR/SIS/NTRU/LIP, any ring/module) → `Lattices`; codes (Hamming, rank metric, QC-MDPC, McEliece, Wave-type) → `Code-based`; multivariate (UOV family, MQ) → `Multivariate`; hash-based, or MPCitH/VOLEitH over a block cipher or hash (FAEST-like) → `Symmetric`; MPCitH/VOLEitH over a non-symmetric hard problem (syndrome decoding, MQ, MinRank) → `MPCitH` (as SDitH/MQOM/RYDE/PERK); isogenies → `Isogenies`; everything else (group actions, trilinear forms, factorisation-like) → `Other` (as ALTEQ).
- `assumption` is short, in the style of existing files (`MLWE/MSIS`, `Rank SD`, `QC-MDPC`, `Restricted Synd. Dec.`, `Multivariate Quadratic`, `Hash-based`, `Isogenies`).
- Sources: the official spec PDF only. The ngcc.dev family strings and claimed targets in `.superpowers/spec/ngcc/ngcc_*` are cross-check only (Step 4).

- [ ] **Step 1: Write the failing test**

`src/lib/__tests__/ngccCandidates.test.ts`:
```ts
import { describe, it, expect } from 'vitest';
import { existsSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import yaml from 'js-yaml';
import type { KemSchemeYaml, SchemeYaml } from '$lib/types';

interface NgccInstance {
	instance: string;
	ref: string;
	ngcc_level?: number;
	recommended?: boolean;
	omit?: string;
	pk?: number;
	sig?: number;
	ct?: number;
	size_ref?: string;
}
interface NgccCandidate {
	id: string;
	kind: 'signature' | 'kem';
	name: string;
	file: string;
	website: string;
	category: string;
	assumption: string;
	spec: string;
	instances: NgccInstance[];
}

const DATA = join(process.cwd(), 'data');
const candidates = yaml.load(readFileSync(join(DATA, 'ngcc', 'candidates.yaml'), 'utf8')) as NgccCandidate[];
const FAMILIES = ['Lattices', 'Code-based', 'Multivariate', 'Symmetric', 'MPCitH', 'Isogenies', 'Other'];
const LEVELS = [128, 256, 384, 512];
const ids = (prefix: string, n: number) => Array.from({ length: n }, (_, i) => `${prefix}-${String(i + 1).padStart(2, '0')}`);
const yamlPath = (c: NgccCandidate) => join(DATA, c.kind === 'signature' ? 'schemes' : 'kems', `${c.file}.yaml`);

describe('data/ngcc/candidates.yaml', () => {
	it('lists the 34 signature and 41 KEM round-1 candidates once each, in order', () => {
		expect(candidates.filter((c) => c.kind === 'signature').map((c) => c.id)).toEqual(ids('sign', 34));
		expect(candidates.filter((c) => c.kind === 'kem').map((c) => c.id)).toEqual(ids('kem', 41));
	});

	it('gives every candidate a site family, an assumption, the official page and a spec', () => {
		const errors = candidates.flatMap((c) => [
			...(FAMILIES.includes(c.category) ? [] : [`${c.id}: unknown family "${c.category}"`]),
			...(c.assumption?.trim() ? [] : [`${c.id}: empty assumption`]),
			...(/^https:\/\/www\.niccs\.org\.cn\//.test(c.website ?? '') ? [] : [`${c.id}: website must be the official NICCS page`]),
			...(c.spec?.trim() ? [] : [`${c.id}: empty spec`])
		]);
		expect(errors).toEqual([]);
	});

	it('gives every instance a spec reference and an NGCC target or a reason to leave it out', () => {
		const errors = candidates.flatMap((c) =>
			c.instances.flatMap((i) => {
				const e: string[] = [];
				if (!i.ref?.trim()) e.push(`${c.id} ${i.instance}: missing ref`);
				if (i.omit !== undefined) {
					if (!i.omit.trim()) e.push(`${c.id} ${i.instance}: empty omit reason`);
					if (i.ngcc_level !== undefined || i.recommended) e.push(`${c.id} ${i.instance}: omitted instances carry no level or recommendation`);
				} else if (!LEVELS.includes(i.ngcc_level as number)) {
					e.push(`${c.id} ${i.instance}: ngcc_level must be 128, 256, 384 or 512`);
				}
				const sizeKey = c.kind === 'signature' ? 'sig' : 'ct';
				if ((i.pk !== undefined || i[sizeKey] !== undefined) && !(i.pk && i[sizeKey] && i.size_ref?.trim()))
					e.push(`${c.id} ${i.instance}: spec sizes need pk, ${sizeKey} and size_ref together`);
				return e;
			})
		);
		expect(errors).toEqual([]);
	});

	it('lists at least one instance per candidate and each instance once per kind', () => {
		for (const kind of ['signature', 'kem'] as const) {
			const names = candidates.filter((c) => c.kind === kind).flatMap((c) => c.instances.map((i) => i.instance));
			expect(names.length).toBeGreaterThan(0);
			expect(names.filter((n, k) => names.indexOf(n) !== k)).toEqual([]);
		}
		expect(candidates.filter((c) => !c.instances.some((i) => i.omit === undefined)).map((c) => c.id)).toEqual([]);
	});

	it('uses safe, unique file names', () => {
		for (const kind of ['signature', 'kem'] as const) {
			const files = candidates.filter((c) => c.kind === kind).map((c) => c.file);
			expect(files.filter((f) => !/^[A-Za-z0-9][A-Za-z0-9._-]*$/.test(f))).toEqual([]);
			expect(new Set(files).size).toBe(files.length);
		}
	});

	it('agrees with the generated YAML files, which never replace another scheme', () => {
		const errors = candidates.flatMap((c) => {
			if (!existsSync(yamlPath(c))) return [];
			const doc = yaml.load(readFileSync(yamlPath(c), 'utf8'));
			const sets: { name: string; ngcc_level?: number; recommended?: boolean }[] | undefined =
				c.kind === 'signature'
					? (doc as SchemeYaml).versions.find((v) => v.programmes?.['china-ngcc'])?.parametersets
					: (doc as KemSchemeYaml).programmes?.['china-ngcc'] ? (doc as KemSchemeYaml).parametersets : undefined;
			if (!sets) return [`${c.file}.yaml exists but is not an NGCC entry`];
			const want = c.instances.filter((i) => i.omit === undefined).map((i) => `${i.instance}:${i.ngcc_level}:${!!i.recommended}`);
			const got = sets.map((p) => `${p.name}:${p.ngcc_level}:${!!p.recommended}`);
			return JSON.stringify(got) === JSON.stringify(want) ? [] : [`${c.file}.yaml parameter sets differ from candidates.yaml`];
		});
		expect(errors).toEqual([]);
	});
});
```

Run: `npx vitest run src/lib/__tests__/ngccCandidates.test.ts`
Expected: FAIL with `ENOENT … data/ngcc/candidates.yaml`.

- [ ] **Step 2: Convert the specs to text (local, minutes)**

```bash
python3 - <<'EOF'
import subprocess
from pathlib import Path
root, out = Path("bench-common/ngcc/specs"), Path("/tmp/ngcc-spec-text")
out.mkdir(exist_ok=True)
for cand in sorted(p for p in root.iterdir() if p.is_dir()):
    pdfs = sorted(cand.rglob("*.pdf"), key=lambda p: p.stat().st_size, reverse=True)
    specs = [p for p in pdfs if "spec" in p.name.lower()] or pdfs
    if not specs:
        print(cand.name, "NO PDF"); continue
    subprocess.run(["pdftotext", "-layout", str(specs[0]), str(out / f"{cand.name}.txt")], check=False)
    print(cand.name, specs[0].relative_to(root))
EOF
```
The printed path (relative to the candidate dir) goes into `spec:`. If the chosen PDF is not the algorithm specification, pick the right one by hand.

- [ ] **Step 3: Write `data/ngcc/candidates.yaml`**

Header (verbatim), then one entry per candidate in id order. Fill the fields from the spec text and PDF. Take `name` from `.superpowers/spec/ngcc/data/{sign,kem}.csv` (official list), `website` from `archives.tsv` `page`, and `instance` values exactly as in `params.tsv`. Instances absent from `params.tsv` (not built) still get entries; give their sizes from the spec with `size_ref` if they are to appear on the site.
```yaml
# China NGCC round-1 candidates: claims curated from each official specification.
# Source of truth for data/schemes/<file>.yaml and data/kems/<file>.yaml (generated by
# bench/update_scheme_data.py and bench-kem/update_scheme_data.py --ngcc-sizes).
#
# Per candidate: id, kind (signature|kem), name (official), file (YAML basename),
# website (official NICCS page), category (site family), assumption (short), spec
# (path of the specification PDF inside the submission archive), instances.
# Per instance: instance (= ALGORITHM_INSTANCE, as in <suite>/schemes/ngcc/params.tsv),
# ref (spec section/table/page for the claim), and either
#   ngcc_level: 128|256|384|512   the NGCC target the submitters assign the instance to
#   recommended: true             only where the spec designates it for its target
# or
#   omit: "<reason>"              not shown on the site (e.g. an intermediate 192-bit set)
# Optional for instances we cannot build: pk, sig|ct, size_ref (spec table/page).
#
# Sources: the official specifications only. ngcc.dev data is never copied here.
```
Shape of one entry (fill with what the spec says):
```yaml
- id: sign-01
  kind: signature
  name: Aigis-Sig+
  file: Aigis-Sig-plus
  website: https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101567126852161536.html
  category: Lattices
  assumption: MLWE/MSIS
  spec: Algorithm specifications.pdf
  instances:
    - instance: Aigis-sig1
      ngcc_level: 128
      recommended: true
      ref: "§4.2 Table 3 (p. 18)"
```
(The level, recommendation and ref above show the format. Use the values the spec states.)

- [ ] **Step 4: Cross-check against the harvest (report only)**

```bash
uv run --with pyyaml python - <<'EOF'
import csv
import yaml
ours = {(c["id"], i["instance"]): i.get("ngcc_level") for c in yaml.safe_load(open("data/ngcc/candidates.yaml")) for i in c["instances"]}
for f in (".superpowers/spec/ngcc/ngcc_sign_instances.csv", ".superpowers/spec/ngcc/ngcc_kem_instances.csv"):
    for r in csv.DictReader(open(f)):
        key = (r["id"], r["instance"])
        if key in ours and str(ours[key]) != r["claimed_target_bits"]:
            print(f"{key}: ours={ours[key]} cross-check={r['claimed_target_bits']}")
EOF
```
Re-read the spec for each difference. Keep our reading, and list the remaining differences for Thom.

- [ ] **Step 5: Run the test**

Run: `npx vitest run src/lib/__tests__/ngccCandidates.test.ts`
Expected: PASS (no generated YAML exists yet, so the last test checks nothing).

- [ ] **Step 6: Commit, then STOP for Thom's review**

```bash
git add data/ngcc/candidates.yaml src/lib/__tests__/ngccCandidates.test.ts
git commit -m "Add curated NGCC round-1 claims (targets, recommended sets, families) from the specs

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
Report to the controller: the omitted instances and why, the candidates without a recommended set, the family assignments that needed judgement, and the cross-check differences. **Thom must approve the file before Task 9 is committed.**

---

### Task 9: Import sizes into the site data (extends `update_scheme_data.py`)

**Files:**
- Create: `bench-common/ngcc/ngcc_data.py`
- Modify: `bench/update_scheme_data.py`, `bench-kem/update_scheme_data.py`, `src/lib/__tests__/dataFiles.test.ts`, `src/lib/__tests__/ngccCandidates.test.ts`
- Create (generated): 34 × `data/schemes/<file>.yaml`, 41 × `data/kems/<file>.yaml`
- Test: `bench-common/ngcc/tests/test_ngcc_data.py`

**Interfaces:**
- Consumes: `data/ngcc/candidates.yaml` (Task 8), `<suite>/results/ngcc_sizes.tsv` and `<suite>/schemes/ngcc/status.tsv` (Tasks 6–7).
- Produces (`ngcc_data.py`): `bench_name(instance) -> str`, `read_tsv(path) -> list[dict]`, `load_candidates(path)`, `load_sizes(path) -> dict[str, dict[str, int]]`, `load_status(path) -> dict[str, dict]`, `bench_to_yaml(cands, kind) -> dict[str, tuple[str, str]]`, `check_coverage(cands, kind, sizes)`, `ps_notes(status_row) -> str`, `scheme_doc(cand, kind, sizes, status, old=None) -> CommentedMap`, `write_files(cands, kind, sizes, status, out_dir) -> list[Path]`, `class NgccDataError(ValueError)`. `kind` ∈ `"signature"|"kem"`.
- CLI: `uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv` and the `bench-kem` equivalent. Timing imports (`update_scheme_data.py <results.txt>`) now map `NGCC-*` names automatically.

- [ ] **Step 1: Write the failing tests**

`bench-common/ngcc/tests/test_ngcc_data.py`:
```python
import pytest
from ruamel.yaml import YAML

import ngcc_data as nd

CANDS = [
    {"id": "sign-01", "kind": "signature", "name": "Aigis-Sig+", "file": "Aigis-Sig-plus",
     "website": "https://www.niccs.org.cn/x", "category": "Lattices", "assumption": "MLWE/MSIS", "spec": "s.pdf",
     "instances": [
         {"instance": "A1", "ngcc_level": 128, "recommended": True, "ref": "T1"},
         {"instance": "A2", "ngcc_level": 256, "ref": "T1"},
         {"instance": "A9", "omit": "Intermediate 192-bit set", "ref": "T1"},
         {"instance": "A3", "ngcc_level": 512, "ref": "T1", "pk": 10, "sig": 20, "size_ref": "Table 4"},
     ]},
    {"id": "kem-01", "kind": "kem", "name": "Aigis-Enc+", "file": "Aigis-Enc-plus",
     "website": "https://www.niccs.org.cn/y", "category": "Lattices", "assumption": "MLWE", "spec": "s.pdf",
     "instances": [{"instance": "E1", "ngcc_level": 128, "ref": "T2"}]},
]
SIG_SIZES = {"NGCC-A1": {"pk": 928, "sk": 1, "sig": 2015}, "NGCC-A2": {"pk": 1824, "sk": 1, "sig": 4533}}
KEM_SIZES = {"NGCC-E1": {"pk": 656, "sk": 1, "ct": 896, "ss": 32}}
STATUS = {
    "NGCC-A1": {"impl": "ref", "kat": "PASS", "hash": "placeholder", "build": "ok"},
    "NGCC-A2": {"impl": "opt", "kat": "MISMATCH", "hash": "own", "build": "ok"},
}


def load(path):
    return YAML(typ="safe").load(path.read_text())


def test_bench_names_map_to_listed_instances_only():
    assert nd.bench_to_yaml(CANDS, "signature") == {
        "NGCC-A1": ("Aigis-Sig-plus.yaml", "A1"), "NGCC-A2": ("Aigis-Sig-plus.yaml", "A2"),
        "NGCC-A3": ("Aigis-Sig-plus.yaml", "A3"),
    }


def test_unclaimed_built_instance_is_an_error():
    with pytest.raises(nd.NgccDataError, match="NGCC-Zz"):
        nd.check_coverage(CANDS, "signature", {**SIG_SIZES, "NGCC-Zz": {"pk": 1, "sk": 1, "sig": 1}})


def test_listed_instance_without_any_sizes_is_an_error():
    with pytest.raises(nd.NgccDataError, match="NGCC-A2"):
        nd.check_coverage(CANDS, "signature", {"NGCC-A1": SIG_SIZES["NGCC-A1"]})


def test_signature_document(tmp_path):
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    doc = load(tmp_path / "Aigis-Sig-plus.yaml")
    v = doc["versions"][0]
    assert (doc["name"], doc["category"], v["version"], v["date"], v["status"]) == (
        "Aigis-Sig+", "Lattices", "Round 1", "2026-09-20", "NGCC round 1")
    assert v["programmes"] == {"china-ngcc": "round-1"}
    a1, a2, a3 = v["parametersets"]
    assert a1 == {"name": "A1", "ngcc_level": 128, "recommended": True, "pk": 928, "sig": 2015,
                  "notes": nd.ps_notes(STATUS["NGCC-A1"])}
    assert "recommended" not in a2 and "level" not in a2
    assert "Optimised x86 implementation" in a2["notes"] and "test vectors" in a2["notes"]
    assert "placeholder hash" in a1["notes"]
    assert (a3["pk"], a3["sig"]) == (10, 20) and "Table 4" in a3["notes"]


def test_kem_document(tmp_path):
    nd.write_files(CANDS, "kem", KEM_SIZES, {}, tmp_path)
    doc = load(tmp_path / "Aigis-Enc-plus.yaml")
    assert (doc["status"], doc["programmes"]) == ("NGCC round 1", {"china-ngcc": "round-1"})
    assert doc["parametersets"] == [{"name": "E1", "ngcc_level": 128, "pk": 656, "ct": 896}]


def test_reimport_keeps_timings(tmp_path):
    nd.write_files(CANDS, "kem", KEM_SIZES, {}, tmp_path)
    path = tmp_path / "Aigis-Enc-plus.yaml"
    path.write_text(path.read_text().replace("ct: 896", "ct: 896\n  keygen_cycles: 1000\n  keygen_us: 0.4"))
    nd.write_files(CANDS, "kem", {"NGCC-E1": {"pk": 656, "sk": 1, "ct": 900, "ss": 32}}, {}, tmp_path)
    ps = load(path)["parametersets"][0]
    assert (ps["ct"], ps["keygen_cycles"], ps["keygen_us"]) == (900, 1000, 0.4)


def test_refuses_to_overwrite_other_schemes(tmp_path):
    (tmp_path / "Aigis-Enc-plus.yaml").write_text("name: Other\nprogrammes:\n  kpqc: selected\nparametersets: []\n")
    with pytest.raises(nd.NgccDataError, match="not an NGCC file"):
        nd.write_files(CANDS, "kem", KEM_SIZES, {}, tmp_path)
```

Add to `src/lib/__tests__/dataFiles.test.ts` inside `describe('programme references in scheme data', …)`:
```ts
	it('China NGCC round 1 has 34 signature and 41 KEM files', () => {
		const sigs = readdirSync(join(DATA, 'schemes'))
			.filter((f) => f.endsWith('.yaml'))
			.filter((f) => load<SchemeYaml>(join(DATA, 'schemes', f)).versions.some((v) => versionProgrammes(v)['china-ngcc']?.includes('round-1'))).length;
		const kems = readdirSync(join(DATA, 'kems'))
			.filter((f) => f.endsWith('.yaml'))
			.filter((f) => [load<KemSchemeYaml>(join(DATA, 'kems', f)).programmes?.['china-ngcc'] ?? []].flat().includes('round-1')).length;
		expect([sigs, kems]).toEqual([34, 41]);
	});
```
Add to `ngccCandidates.test.ts`:
```ts
	it('has a generated YAML file for every candidate', () => {
		expect(candidates.filter((c) => !existsSync(yamlPath(c))).map((c) => c.file)).toEqual([]);
	});
```

Run: `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests/test_ngcc_data.py; npx vitest run src/lib/__tests__/dataFiles.test.ts src/lib/__tests__/ngccCandidates.test.ts`
Expected: pytest FAILS (`No module named 'ngcc_data'`), and the two new Vitest cases FAIL (`[0, 0]` and 75 missing files).

- [ ] **Step 2: Write `ngcc_data.py`**

```python
"""Turn NGCC sizes + build status + curated claims into data/ YAML.

Shared by bench/update_scheme_data.py (signatures) and bench-kem/update_scheme_data.py
(KEMs). Inputs:
  data/ngcc/candidates.yaml         curated from the official specs (Task 8)
  <suite>/results/ngcc_sizes.tsv    `./bench --sizes NGCC` / `./bench-kem --sizes NGCC`
  <suite>/schemes/ngcc/status.tsv   status.py: build, KAT, hash usage, impl
Re-running keeps timing fields that a benchmark import already wrote.
"""

from __future__ import annotations

import csv
from pathlib import Path

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap

PREFIX = "NGCC-"
PROGRAMME, STAGE = "china-ngcc", "round-1"
ROUND1_DATE = "2026-09-20"
STATUS = "NGCC round 1"
SIZE_KEY = {"signature": "sig", "kem": "ct"}
TIMING = {
    "signature": ("signing_cycles", "verification_cycles", "signing_us", "verification_us"),
    "kem": ("keygen_cycles", "encaps_cycles", "decaps_cycles", "keygen_us", "encaps_us", "decaps_us"),
}
HASH_NOTE = {
    "placeholder": "uses the ICCS placeholder hash/XOF (pseudohash/pseudoXOF) from the submission package, so timings include it",
    "sm3": "uses the ICCS SM3 implementation from the submission package",
}
KAT_NOTE = {
    "MISMATCH": "our build does not reproduce the submitted test vectors",
    "NOKAT": "no submitted test vectors found for this instance",
    "CRYPTOFAIL": "the submitted code failed its own test-vector run in our build",
    "CRASH": "the test-vector run crashed in our build",
    "TIMEOUT": "the test-vector run timed out in our build",
    "LOADFAIL": "the built library is missing ICCS API symbols",
}


class NgccDataError(ValueError):
    pass


def bench_name(instance: str) -> str:
    return PREFIX + instance


def read_tsv(path: Path) -> list[dict]:
    with open(path, newline="", encoding="utf-8") as f:
        body = [ln for ln in f if not ln.startswith("#")]
    return [{k: (v or "").strip() for k, v in r.items()} for r in csv.DictReader(body, delimiter="\t")]


def load_candidates(path: Path) -> list[dict]:
    return YAML(typ="safe").load(path.read_text(encoding="utf-8"))


def load_sizes(path: Path) -> dict[str, dict[str, int]]:
    return {r["scheme"]: {k: int(v) for k, v in r.items() if k != "scheme"} for r in read_tsv(path)}


def load_status(path: Path) -> dict[str, dict]:
    return {r["name"]: r for r in read_tsv(path)} if path.exists() else {}


def of_kind(cands: list[dict], kind: str) -> list[dict]:
    return [c for c in cands if c["kind"] == kind]


def listed(cand: dict) -> list[dict]:
    return [i for i in cand["instances"] if "omit" not in i]


def bench_to_yaml(cands: list[dict], kind: str) -> dict[str, tuple[str, str]]:
    return {bench_name(i["instance"]): (f"{c['file']}.yaml", i["instance"]) for c in of_kind(cands, kind) for i in listed(c)}


def check_coverage(cands: list[dict], kind: str, sizes: dict) -> None:
    claimed = {bench_name(i["instance"]) for c in of_kind(cands, kind) for i in c["instances"]}
    problems = [f"built but not in candidates.yaml: {n}" for n in sorted(set(sizes) - claimed)]
    problems += [
        f"no sizes (not built and no spec sizes): {bench_name(i['instance'])}"
        for c in of_kind(cands, kind) for i in listed(c)
        if bench_name(i["instance"]) not in sizes and SIZE_KEY[kind] not in i
    ]
    if problems:
        raise NgccDataError("\n".join(problems))


def ps_notes(status: dict | None) -> str:
    if not status:
        return ""
    parts = ["Optimised x86 implementation" if status.get("impl") == "opt" else "Reference implementation"]
    if status.get("hash") in HASH_NOTE:
        parts.append(HASH_NOTE[status["hash"]])
    if status.get("kat") in KAT_NOTE:
        parts.append(KAT_NOTE[status["kat"]])
    return "; ".join(parts) + "."


def parameterset(inst: dict, kind: str, sizes: dict, status: dict, old: dict | None) -> CommentedMap:
    name, key = bench_name(inst["instance"]), SIZE_KEY[kind]
    ps = CommentedMap()
    ps["name"] = inst["instance"]
    ps["ngcc_level"] = inst["ngcc_level"]
    if inst.get("recommended"):
        ps["recommended"] = True
    if name in sizes:
        ps["pk"], ps[key] = sizes[name]["pk"], sizes[name][key]
        note = ps_notes(status.get(name))
    else:
        ps["pk"], ps[key] = inst["pk"], inst[key]
        note = f"Sizes from the specification ({inst['size_ref']}); our build of this instance fails."
    for field in TIMING[kind]:
        if old is not None and field in old:
            ps[field] = old[field]
    if note:
        ps["notes"] = note
    return ps


def _old_sets(old, kind: str) -> list:
    if old is None:
        return []
    if kind == "kem":
        return old.get("parametersets") or []
    version = next((v for v in old.get("versions") or [] if PROGRAMME in (v.get("programmes") or {})), None)
    return (version or {}).get("parametersets") or []


def _is_ngcc(old, kind: str) -> bool:
    if kind == "kem":
        return PROGRAMME in (old.get("programmes") or {})
    return any(PROGRAMME in (v.get("programmes") or {}) for v in old.get("versions") or [])


def scheme_doc(cand: dict, kind: str, sizes: dict, status: dict, old=None) -> CommentedMap:
    previous = {str(p["name"]): p for p in _old_sets(old, kind)}
    sets = [parameterset(i, kind, sizes, status, previous.get(i["instance"])) for i in listed(cand)]
    doc = CommentedMap()
    for field in ("name", "website", "category", "assumption"):
        doc[field] = cand[field]
    programmes = CommentedMap([(PROGRAMME, STAGE)])
    if kind == "signature":
        version = CommentedMap()
        version["version"] = "Round 1"
        version["date"] = ROUND1_DATE
        version["status"] = STATUS
        version["programmes"] = programmes
        version["parametersets"] = sets
        doc["versions"] = [version]
    else:
        doc["status"] = STATUS
        doc["programmes"] = programmes
        doc["parametersets"] = sets
    return doc


def write_files(cands: list[dict], kind: str, sizes: dict, status: dict, out_dir: Path) -> list[Path]:
    check_coverage(cands, kind, sizes)
    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096
    plans = []
    for cand in of_kind(cands, kind):
        path = out_dir / f"{cand['file']}.yaml"
        old = yaml.load(path.read_text(encoding="utf-8")) if path.exists() else None
        if old is not None and not _is_ngcc(old, kind):
            raise NgccDataError(f"{path.name} exists and is not an NGCC file")
        plans.append((path, scheme_doc(cand, kind, sizes, status, old)))
    for path, doc in plans:
        with open(path, "w", encoding="utf-8") as f:
            yaml.dump(doc, f)
    return [p for p, _ in plans]
```

- [ ] **Step 3: Extend `bench/update_scheme_data.py`**

Update the module docstring's usage block to:
```
Usage:
    uv run bench/update_scheme_data.py [results_file]
    uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
```
Directly after the closing `}` of `BENCH_TO_YAML`, insert:
```python
# China NGCC round-1 instances: bench names are NGCC-<instance>. The mapping comes
# from data/ngcc/candidates.yaml (see bench-common/ngcc/ngcc_data.py).
sys.path.insert(0, str(REPO_ROOT / "bench-common" / "ngcc"))
import ngcc_data  # noqa: E402

NGCC_CANDIDATES = REPO_ROOT / "data" / "ngcc" / "candidates.yaml"
NGCC_STATUS = REPO_ROOT / "bench" / "schemes" / "ngcc" / "status.tsv"
if NGCC_CANDIDATES.exists():
    BENCH_TO_YAML.update(ngcc_data.bench_to_yaml(ngcc_data.load_candidates(NGCC_CANDIDATES), "signature"))


def import_ngcc_sizes(sizes_file: Path) -> None:
    """Create or refresh the NGCC signature YAML files from `./bench --sizes NGCC` output."""
    cands = ngcc_data.load_candidates(NGCC_CANDIDATES)
    try:
        written = ngcc_data.write_files(
            cands, "signature", ngcc_data.load_sizes(sizes_file), ngcc_data.load_status(NGCC_STATUS), SCHEMES_DIR
        )
    except ngcc_data.NgccDataError as e:
        sys.exit(f"NGCC import failed:\n{e}")
    print(f"Wrote {len(written)} NGCC signature files in {SCHEMES_DIR.relative_to(REPO_ROOT)}")
```
In `write_benchmark_env`, replace `sources = meta.get("_sources", {})` with:
```python
    # Merge with the sources already recorded: a filtered run (e.g. only NGCC)
    # must not drop the provenance of the schemes it did not run.
    sources: dict = {}
    if path.exists():
        previous = YAML(typ="safe").load(path.read_text()) or {}
        sources.update(previous.get("sources") or {})
        prev_cpu = (previous.get("cpu") or {}).get("model")
        if prev_cpu and prev_cpu != meta.get("cpu", ""):
            print(f"WARNING: {path.name} was measured on {prev_cpu!r}, this run on {meta.get('cpu')!r}")
    sources.update(meta.get("_sources", {}))
```
At the start of `main()`, insert:
```python
    if len(sys.argv) == 3 and sys.argv[1] == "--ngcc-sizes":
        import_ngcc_sizes(Path(sys.argv[2]))
        return
```

- [ ] **Step 4: Extend `bench-kem/update_scheme_data.py` the same way**

Make the same three edits with these substitutions. Docstring: `uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv`. `NGCC_STATUS = REPO_ROOT / "bench-kem" / "schemes" / "ngcc" / "status.tsv"`. `bench_to_yaml(…, "kem")`. The function is:
```python
def import_ngcc_sizes(sizes_file: Path) -> None:
    """Create or refresh the NGCC KEM YAML files from `./bench-kem --sizes NGCC` output."""
    cands = ngcc_data.load_candidates(NGCC_CANDIDATES)
    try:
        written = ngcc_data.write_files(
            cands, "kem", ngcc_data.load_sizes(sizes_file), ngcc_data.load_status(NGCC_STATUS), KEMS_DIR
        )
    except ngcc_data.NgccDataError as e:
        sys.exit(f"NGCC import failed:\n{e}")
    print(f"Wrote {len(written)} NGCC KEM files in {KEMS_DIR.relative_to(REPO_ROOT)}")
```
Apply the identical `sources` merge in `write_benchmark_env` and the same `--ngcc-sizes` dispatch in `main()`.

- [ ] **Step 5: Run the Python tests, then import**

```bash
uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv
git status --short data/
```
Expected: pytest all pass. `Wrote 34 NGCC signature files…`, `Wrote 41 NGCC KEM files…`, and 75 new `??` YAML files. If the import exits with `NGCC import failed`, fix `candidates.yaml` (Task 8 data: a missing or renamed instance) or the manifest. Never edit generated YAML by hand.

- [ ] **Step 6: Site checks**

Run: `npm run test && npm run check && npm run build`
Expected: all Vitest tests pass, including the two new cases. `svelte-check` reports 0 errors. The build succeeds.

- [ ] **Step 7: Lint and commit (only after Thom approved Task 8)**

```bash
uvx ruff check --fix bench-common/ngcc && uvx ruff format bench-common/ngcc
uvx ruff check bench/update_scheme_data.py bench-kem/update_scheme_data.py
git add bench-common/ngcc/ngcc_data.py bench-common/ngcc/tests/test_ngcc_data.py bench/update_scheme_data.py bench-kem/update_scheme_data.py src/lib/__tests__/dataFiles.test.ts src/lib/__tests__/ngccCandidates.test.ts
git add $(git status --short data/schemes data/kems | awk '$1=="??"{print $2}')
git commit -m "Import NGCC round-1 parameter sets and sizes from our builds

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 10: Site: history, e2e coverage, documentation

**Files:**
- Modify: `data/history.yaml`, `e2e/signatures.spec.ts`, `e2e/kems.spec.ts`, `e2e/home.spec.ts`, `CLAUDE.md`, `bench/CLAUDE.md`, `bench-kem/CLAUDE.md`

**Interfaces:**
- Consumes: the generated YAML (Task 9). Aigis-Sig+ and Aigis-Enc+ are the e2e anchors because they are sign-01/kem-01. If Task 8 gave neither a 128-bit listed instance, pick another candidate that has one and use it in all three tests.

- [ ] **Step 1: Write the e2e tests**

Append to `e2e/signatures.spec.ts`:
```ts
test.describe('China NGCC candidates', () => {
	test.use({ colorScheme: 'light' });

	test('rows show NGCC targets in purple, never as NIST levels', async ({ page }) => {
		await page.goto('/signatures/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByRole('group', { name: 'Programme' }).getByLabel('China NGCC')).toBeChecked();
		const row = page.locator('tbody tr').filter({ hasText: 'Aigis-Sig+' }).filter({ hasText: '128-bit' }).first();
		await expect(row).toBeVisible();
		const level = row.getByTitle('China NGCC target in classical bits; not a NIST level');
		await expect(level).toHaveText('128-bit');
		await expect(level).toHaveCSS('color', 'rgb(107, 33, 168)');
	});
});
```
Append to `e2e/kems.spec.ts`:
```ts
test.describe('China NGCC candidates', () => {
	test.use({ colorScheme: 'light' });

	test('rows show NGCC targets in purple, never as NIST levels', async ({ page }) => {
		await page.goto('/kems/');
		await page.waitForLoadState('networkidle');
		await expect(page.getByRole('group', { name: 'Programme' }).getByLabel('China NGCC')).toBeChecked();
		const row = page.locator('tbody tr').filter({ hasText: 'Aigis-Enc+' }).filter({ hasText: '128-bit' }).first();
		await expect(row).toBeVisible();
		const level = row.getByTitle('China NGCC target in classical bits; not a NIST level');
		await expect(level).toHaveText('128-bit');
		await expect(level).toHaveCSS('color', 'rgb(107, 33, 168)');
	});
});
```
Add inside `test.describe('Home page', …)` in `e2e/home.spec.ts`:
```ts
	test('lists the NGCC round-1 candidates', async ({ page }) => {
		await page.goto('/');
		const ngcc = page
			.getByRole('region', { name: 'Programmes' })
			.getByRole('row')
			.filter({ hasText: 'Next-generation commercial cryptographic algorithms' });
		await expect(ngcc).toContainText('Round 1:');
		await expect(ngcc).toContainText('Aigis-Sig+');
		await expect(ngcc).toContainText('Aigis-Enc+');
	});
```

- [ ] **Step 2: Run them**

Run: `npm run test:e2e -- e2e/signatures.spec.ts e2e/kems.spec.ts e2e/home.spec.ts`
Expected: PASS (the data is already in from Task 9). If the colour assertion fails, check that the row is not in dark mode and that `SchemeTable`/`KemTable` still use `text-[#6b21a8]`. Do not change the test to fit the page. Then run the full `npm run test:e2e` to catch layout regressions from 75 more schemes (e.g. timeouts on the larger table). Fix them in the product code or with polling, never with sleeps.

- [ ] **Step 3: History entries**

At the top of `data/history.yaml`, insert (the date is today's, from `date -u +%F`):
```yaml
- date: 'YYYY-MM-DD'
  type: update
  description: "Added the 34 signature and 41 KEM candidates of round 1 of China's NGCC programme (ICCS). Parameter sets and sizes come from our own builds of the official submission packages through the ICCS API, and claimed security targets from each specification. NGCC targets are shown in classical bits and are not NIST levels. Timings follow once our benchmarks have run."
```
Before the `- date: '2026-09-08'` SDitH entry, insert:
```yaml
- date: '2026-09-20'
  type: milestone
  description: "ICCS published the round-1 candidates of China's Next-generation Commercial Cryptographic Algorithms programme (NGCC): 34 signatures, 41 KEMs, 9 key exchanges and 35 hash functions. Submissions had closed on 30 June 2026. <a href=\"https://www.niccs.org.cn/en/\">NICCS</a>."
```
Replace `YYYY-MM-DD` with the real date before saving. It must be a real ISO date, because `npm run test` parses this file through the home page loader.

- [ ] **Step 4: Documentation**

In `CLAUDE.md`, under `## Data` after the `### KEMs` subsection, add:
````markdown
### China NGCC

`data/ngcc/candidates.yaml` holds the curated claims for the 34 signature and 41 KEM
round-1 candidates: family, assumption, official NICCS page, spec reference, and per
instance (`ALGORITHM_INSTANCE`) the claimed `ngcc_level`, `recommended`, or `omit: reason`.
It is the source for the generated `data/schemes/<file>.yaml` / `data/kems/<file>.yaml`
(`programmes: {china-ngcc: round-1}`, status `NGCC round 1`). Never edit those by hand:

```bash
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv
```

Sizes come from our builds of the official archives (`./bench --sizes NGCC`); per-instance
notes record the implementation (reference/optimised), ICCS placeholder-hash use and KAT
status from `<suite>/schemes/ngcc/status.tsv`. `src/lib/__tests__/ngccCandidates.test.ts`
validates the claims and their agreement with the generated files. ngcc.dev data is
cross-check only; findings links/flags are added only with Thom's sign-off.
````
In the `data/` block of the Architecture tree, add `├── ngcc/candidates.yaml  # curated NGCC round-1 claims (source for generated NGCC YAML)`.

In `bench/CLAUDE.md`, add a section before `## Optimised builds`:
````markdown
## NGCC candidates (ICCS API)

China NGCC round-1 candidates all implement the ICCS public-key C API, so they share
one shim (`schemes/ngcc/shim_template.c`) and are integrated as **data**: one row per
`ALGORITHM_INSTANCE` in `schemes/ngcc/params.tsv` (columns documented in
`../bench-common/ngcc/gen_rules.py`). The shared machinery lives in `../bench-common/ngcc/`:

```bash
uv run ../bench-common/ngcc/fetch.py      # official archives -> $NGCC_CACHE, sha256-pinned in archives.tsv
uv run ../bench-common/ngcc/extract.py    # allowlisted sources/KATs/specs -> bench-common/ngcc/src, specs (git-ignored)
python3 ../bench-common/ngcc/inventory.py sig sign-07   # draft params.tsv rows
make -C schemes/ngcc                      # best effort: FAIL lines + build/<file>.log, never fatal
make -C schemes/ngcc status               # build/KAT/hash status -> schemes/ngcc/status.tsv (commit it)
./bench --sizes NGCC > results/ngcc_sizes.tsv   # getter sizes for the YAML import (commit it)
```

- Only `build/built.list` reaches `SO_PATHS`, so a failing candidate never breaks `./bench`.
  Without extracted sources (e.g. a fresh clone) the NGCC build is skipped with a message.
- Randomness comes from the ICCS DRNG `drng_algorithm` (defined and seeded by the shim);
  drop candidates' `/dev/urandom` `randombytes.c`, which also breaks KAT replay.
- `defs` reach only the candidate's sources and `shimdefs` only the shim, so NIST-API name
  clashes can be renamed away (`-Dcrypto_sign_keypair=cand_crypto_sign_keypair`).
- Never edit extracted sources or run candidates' build systems: fixes are copies in
  `schemes/ngcc/patches/<id>/` with a `/* PATCH (nist-sigs-zoo): why */` header.
- KATs are replayed by our `ngcc_kat` (ICCS seed schedule) and compared with the submitted
  files; mismatches are recorded, not hidden, and shown in the parameter set's notes.
- Cloud runs need the sources uploaded: `scripts/aws-bench.sh --ngcc-src <tar.gz>`.
````
In `bench-kem/CLAUDE.md`, add after its "Adding a new scheme" section:
```markdown
## NGCC candidates (ICCS API)

Same machinery as `bench/` (see `bench/CLAUDE.md`, "NGCC candidates"): `schemes/ngcc/`
with the ICCS KEM shim, `params.tsv`, `status.tsv`, and `./bench-kem --sizes NGCC >
results/ngcc_sizes.tsv`. The shim forwards the lengths reported by the last keygen/encaps,
which is sound because the harness always runs keygen → encaps → decaps in order.
```

- [ ] **Step 5: Full checks and commit**

Run: `npm run test && npm run check && npm run build && npm run test:e2e`
Expected: everything passes.
```bash
git add data/history.yaml e2e/signatures.spec.ts e2e/kems.spec.ts e2e/home.spec.ts CLAUDE.md bench/CLAUDE.md bench-kem/CLAUDE.md
git commit -m "Announce the NGCC round-1 import; cover NGCC rows in e2e; document the NGCC pipeline

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 11: Optimised x86 implementations where they pass the KATs (THOM DECIDES)

`bench/CLAUDE.md` says to benchmark each upstream's fastest x86-64 build. Many NGCC submissions ship an `Optimized_Implementation` (often AVX2). **Ask Thom first:** if he prefers reference-only timings for NGCC round 1, skip this task and record that in `bench/CLAUDE.md` under "NGCC candidates".

**Files:**
- Modify: `bench/schemes/ngcc/params.tsv`, `bench-kem/schemes/ngcc/params.tsv`, both `status.tsv`, both `results/ngcc_sizes.tsv` (must stay identical), regenerated NGCC YAML notes
- Test: `bench-common/ngcc/tests/test_manifests.py` (extended)

**Interfaces:**
- Produces: rows with `impl=opt` for instances whose optimised build passes the same KAT. The YAML notes read "Optimised x86 implementation".

- [ ] **Step 1: Add the failing guard test**

Append to `test_manifests.py`:
```python
@pytest.mark.parametrize("kind", POPULATED)
def test_optimised_rows_pass_their_kats(kind):
    suite = SUITES[kind][0]
    status = tsv(suite / "schemes/ngcc/status.tsv")
    assert [s["name"] for s in status if s["impl"] == "opt" and s["kat"] != "PASS"] == []
```
Run: `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests/test_manifests.py`. Expected: PASS (no `opt` rows yet). This guard pins the rule for the switch below.

- [ ] **Step 2: Build the optimised trees on the side (signatures, then KEMs)**

```bash
python3 bench-common/ngcc/inventory.py sig | awk -F'\t' 'NR==1 || $5=="opt"' > /tmp/ngcc-sig-opt.tsv
make -C bench/schemes/ngcc PARAMS=/tmp/ngcc-sig-opt.tsv BUILD=build/opt -j"$(nproc)"
make -C bench/schemes/ngcc status PARAMS=/tmp/ngcc-sig-opt.tsv BUILD=build/opt STATUS=/tmp/ngcc-sig-opt-status.tsv
```
Curate `/tmp/ngcc-sig-opt.tsv` with the Task 6 triage table, time-boxed to 20 minutes per candidate. Optimised trees often select the parameter set by `defs` from one shared dir, so expect `defs`/`shimdefs` work.

- [ ] **Step 3: Switch the rows that pass**

For each instance with `build=ok` and `kat=PASS` in `/tmp/ngcc-sig-opt-status.tsv`, replace its row in `bench/schemes/ngcc/params.tsv` with the curated opt row (same `file`, `name`, `instance`). Then:
```bash
make -C bench/schemes/ngcc -j"$(nproc)" && make -C bench/schemes/ngcc status
make -C bench -j"$(nproc)" && (cd bench && ./bench --sizes NGCC > /tmp/sizes-opt.tsv)
diff bench/results/ngcc_sizes.tsv /tmp/sizes-opt.tsv && echo "sizes unchanged"
```
Expected: `sizes unchanged`. If a size differs, the opt build is a different parameter set: revert that row. Repeat Steps 2–3 for `bench-kem` (`inventory.py kem`, `/tmp/ngcc-kem-opt*.tsv`, `./bench-kem --sizes NGCC`).

- [ ] **Step 4: Refresh notes, test, commit**

```bash
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv
uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests && npm run test
git add bench/schemes/ngcc/params.tsv bench/schemes/ngcc/status.tsv bench-kem/schemes/ngcc/params.tsv bench-kem/schemes/ngcc/status.tsv bench-common/ngcc/tests/test_manifests.py
git add bench/schemes/ngcc/patches bench-kem/schemes/ngcc/patches 2>/dev/null || true
git add $(git status --short data/schemes data/kems | awk '$1=="M"{print $2}')
git commit -m "Build NGCC optimised x86 implementations where they reproduce the KATs

<counts: N signature and M KEM instances switched to opt>

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 12: Cloud runs: upload NGCC sources, tune iterations, smoke test

**Files:**
- Modify: `scripts/aws-bench.sh`, `bench/schemes/ngcc/params.tsv`, `bench-kem/schemes/ngcc/params.tsv` (`iters`), `CLAUDE.md` (aws-bench options)

**Interfaces:**
- Produces: `scripts/aws-bench.sh --ngcc-src TARBALL`, which uploads a `tar.gz` of `bench-common/ngcc/src` and unpacks it into `repo/bench-common/ngcc/` on the VM before the build. The VM never downloads from niccs.org.cn, which avoids China-to-AWS reachability problems and load on their server.

- [ ] **Step 1: Add `--ngcc-src` to `scripts/aws-bench.sh`**

In the usage comment, after the `--ttl` lines, add:
```bash
#   --ngcc-src TARBALL       tar.gz of bench-common/ngcc/src (NGCC candidates' extracted
#                            sources); uploaded so the VM builds the NGCC instances too
```
After `FILTERS=()` add `NGCC_SRC_TAR=""`. In the option `case`, add before `-h|--help)`:
```bash
        --ngcc-src)      NGCC_SRC_TAR="$2"; shift 2 ;;
```
After the `--ttl` validation line, add:
```bash
if [ -n "$NGCC_SRC_TAR" ] && [ ! -f "$NGCC_SRC_TAR" ]; then
    echo "--ngcc-src: no such file: $NGCC_SRC_TAR" >&2; exit 2
fi
```
Right after the clone heredoc (the line `EOF` that ends `git submodule update …`), add:
```bash
if [ -n "$NGCC_SRC_TAR" ]; then
    log "uploading NGCC sources ($(du -h "$NGCC_SRC_TAR" | cut -f1))"
    rsync -a -e "$RSYNC_RSH" "$NGCC_SRC_TAR" "ubuntu@$IP:ngcc-src.tar.gz"
    rssh 'tar -xzf ngcc-src.tar.gz -C repo/bench-common/ngcc && test -f repo/bench-common/ngcc/src/.extracted' \
        || die "unpacking the NGCC sources failed"
fi
```
If Tasks 6–7 added libraries in `ldlibs`, make sure their `-dev` packages are in the user-data `apt-get install` list (e.g. `libgmp-dev`, `libssl-dev` are already there).

In `CLAUDE.md`, under "Cloud benchmark runs", add to the Options line: `` `--ngcc-src` tar.gz of `bench-common/ngcc/src` (NGCC candidates; create with `tar -C bench-common/ngcc -czf /tmp/ngcc-src.tar.gz src`) ``.

Run: `bash -n scripts/aws-bench.sh && scripts/aws-bench.sh --help | grep -- --ngcc-src && scripts/aws-bench.sh --ngcc-src /nonexistent; echo "exit=$?"`
Expected: syntax OK, the help line shows, and `--ngcc-src: no such file` with `exit=2`. The script exits before any AWS call, because validation happens right after option parsing.

- [ ] **Step 2: Local smoke run and iteration tuning**

```bash
(cd bench && BENCH_ITER=5 ./run_bench.sh NGCC) ; (cd bench-kem && BENCH_ITER=5 ./run_bench.sh NGCC)
ls -t bench/results/*ngcc*.txt bench-kem/results/*ngcc*.txt | head -2
```
These two results files are **junk**: never commit or import them. Delete them after this step. Use them to set `iters` so each instance runs about 30 s of timed work at most:
```bash
python3 - "$(ls -t bench/results/*_ngcc.txt | head -1)" bench/schemes/ngcc/params.tsv sig <<'EOF'
import csv, sys
from pathlib import Path
results, params, kind = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
budget_us = 30e6
per_iter = {}
for line in results.read_text().splitlines():
    p = line.split()
    if p and p[0].startswith("NGCC-") and len(p) >= 8:
        cols = (p[-5], p[-3], p[-1]) if kind == "sig" else (p[-6], p[-4], p[-2])
        per_iter[p[0]] = sum(float(c) for c in cols)
lines = params.read_text().splitlines(keepends=True)
comments = [ln for ln in lines if ln.startswith("#")]
rows = list(csv.DictReader([ln for ln in lines if not ln.startswith("#")], delimiter="\t"))
for r in rows:
    us = per_iter.get(r["name"])
    if us and us * 1000 > budget_us:
        r["iters"] = str(max(10, int(budget_us / us)))
with open(params, "w", newline="") as f:
    f.writelines(comments)
    w = csv.DictWriter(f, fieldnames=list(rows[0].keys()), delimiter="\t", lineterminator="\n")
    w.writeheader(); w.writerows(rows)
print({r["name"]: r["iters"] for r in rows if r["iters"] != "0"})
EOF
```
Run the same command for `bench-kem` with its results file, `bench-kem/schemes/ngcc/params.tsv` and `kem`. Comment lines that sit between rows move to the top of the file. Check `git diff` and move any row-specific comments back above their rows by hand. Then delete the two junk results files.

- [ ] **Step 3: Tests and commit**

Run: `uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests`, expected all pass.
```bash
git add scripts/aws-bench.sh CLAUDE.md bench/schemes/ngcc/params.tsv bench-kem/schemes/ngcc/params.tsv
git commit -m "Upload NGCC sources to cloud bench runs; cap NGCC iterations at ~30 s per instance

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Task 13: Benchmark NGCC on AWS and import timings (THOM: AWS login + spend, LONG)

**Files:**
- Create: `bench/results/<timestamp>_intel_r_xeon_r_platinum_8488c_ngcc.txt`, `bench-kem/results/<…>_ngcc.txt`
- Modify: the 75 NGCC YAML files (timings), `data/benchmark_env.yaml`, `data/kem_benchmark_env.yaml`, `data/history.yaml`

**Interfaces:**
- Consumes: `--ngcc-src` (Task 12), `update_scheme_data.py` NGCC mapping (Task 9).

- [ ] **Step 1: Confirm with Thom, then run**

Confirm with Thom before launching: it costs money and needs his `aws sso login --profile bench`. Use the same instance type as the existing data (`c7i.4xlarge`, Xeon 8488C) so the env file stays coherent.
```bash
tar -C bench-common/ngcc -czf /tmp/ngcc-src.tar.gz src
AWS_PROFILE=bench scripts/aws-bench.sh --suite sigs --ttl 360 --ngcc-src /tmp/ngcc-src.tar.gz NGCC
AWS_PROFILE=bench scripts/aws-bench.sh --suite kems --ttl 360 --ngcc-src /tmp/ngcc-src.tar.gz NGCC
```
Expected: each run ends with `done. Review the new results files…` and no `rdtsc` warning. If the TTL is reached, partial results are fetched. In that case split the run with narrower filters (e.g. `NGCCA NGCCB …` by first letter) and re-run the rest.

- [ ] **Step 2: Review the results**

For each new file, check three things: the `# cyclecounter:` line says rdpmc, every `FAIL <name>: …` line (signatures) and every `ok=FAIL` row (KEMs) is listed for the commit message, and the row count matches `built.list`. A signature instance that fails the harness's tamper checks gets no timing: list it for Thom. It is a fact about the submitted code. Whether it is shown as a finding is Task 14's decision.

- [ ] **Step 3: Import timings**

```bash
uv run bench/update_scheme_data.py bench/results/<new sigs file>.txt
uv run bench-kem/update_scheme_data.py bench-kem/results/<new kems file>.txt
```
Expected: `Updated N parametersets` with N = the number of NGCC rows in each file and no `No YAML mapping` lines. "Bench-mapped but not in this run" will list the NIST schemes, which is expected for a filtered run. The env files keep the previous `sources` (merged) and must not print the CPU warning.

- [ ] **Step 4: History, checks, commit**

Add at the top of `data/history.yaml` (date = today):
```yaml
- date: 'YYYY-MM-DD'
  type: update
  description: "Benchmarked the China NGCC round-1 candidates with our own harness (one core of an Intel Xeon Platinum 8488C, core cycles via rdpmc). Each instance is built from the official submission through the ICCS API; notes per parameter set say which implementation was measured and whether it uses the ICCS placeholder hash."
```
Run: `npm run test && npm run build && npm run test:e2e`
```bash
git add bench/results/*_ngcc.txt bench-kem/results/*_ngcc.txt data/benchmark_env.yaml data/kem_benchmark_env.yaml data/history.yaml
git add $(git status --short data/schemes data/kems | awk '$1=="M"{print $2}')
git commit -m "Benchmark NGCC round-1 signatures and KEMs on c7i.4xlarge

<instances without timings and why>

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
Do not commit the junk smoke files from Task 12.

---

### Task 14: Findings links and flags (BLOCKED until Thom confirms attribution)

**Gate:** do not start until Thom confirms in this session that attribution with Markku-Juhani Saarinen is agreed and gives the exact credit wording. Record his confirmation (date, wording) in the commit message. Until then, no NGCC file may contain `findings_url`, `broken` or `implementation`.

**Files:**
- Modify: `data/ngcc/candidates.yaml` (`findings` blocks), `bench-common/ngcc/ngcc_data.py`, `bench-common/ngcc/tests/test_ngcc_data.py`, `src/lib/__tests__/ngccCandidates.test.ts`, regenerated NGCC YAML, `data/history.yaml` (only if Thom wants `attack` entries for design breaks)

**Interfaces:**
- Consumes/extends `candidates.yaml`: a candidate-level `findings: {url, credit, implementation?, broken?}` and per-instance `broken?` / `implementation?`. `scheme_doc` writes `findings_url`, `implementation`, `broken` at version level (signatures) or scheme level (KEMs), and per-instance flags on parameter sets.

- [ ] **Step 1: Failing tests**

Append to `test_ngcc_data.py`:
```python
def test_findings_are_written_with_credit(tmp_path):
    cands = [dict(CANDS[1], findings={"url": "https://ngcc.dev/reports/kem-01.html", "credit": "Saarinen, ngcc.dev",
                                      "implementation": "Decapsulation does not reject invalid ciphertexts (Saarinen, ngcc.dev)."})]
    nd.write_files(cands, "kem", KEM_SIZES, {}, tmp_path)
    doc = load(tmp_path / "Aigis-Enc-plus.yaml")
    assert doc["findings_url"] == "https://ngcc.dev/reports/kem-01.html"
    assert doc["implementation"].endswith("(Saarinen, ngcc.dev).")
    assert "broken" not in doc
```
Append to `ngccCandidates.test.ts` (and add `findings?: { url: string; credit: string; implementation?: string; broken?: string }` to the `NgccCandidate` interface):
```ts
	it('keeps findings short, linked and credited', () => {
		const errors = candidates.flatMap((c) => {
			const f = c.findings;
			if (!f) return [];
			const e: string[] = [];
			if (f.url !== `https://ngcc.dev/reports/${c.id}.html`) e.push(`${c.id}: findings url must be its ngcc.dev report`);
			for (const text of [f.implementation, f.broken].filter((t): t is string => !!t)) {
				if (text.length > 280) e.push(`${c.id}: summary over 280 characters`);
				if (!text.includes(`(${f.credit})`)) e.push(`${c.id}: summary must credit (${f.credit})`);
			}
			return e;
		});
		expect(errors).toEqual([]);
	});
```
Run both. Expected: FAIL (`findings_url` missing in the document; the TS interface lacks `findings`).

- [ ] **Step 2: Implement in `ngcc_data.py`**

In `scheme_doc`, after the `programmes`/`parametersets` assignment for each kind, add:
```python
    findings = cand.get("findings")
    target = doc["versions"][0] if kind == "signature" else doc
    if findings:
        target["findings_url"] = findings["url"]
        for flag in ("implementation", "broken"):
            if findings.get(flag):
                target[flag] = findings[flag]
```
In `parameterset`, before `return ps`, add:
```python
    for flag in ("implementation", "broken"):
        if inst.get(flag):
            ps[flag] = inst[flag]
```
Run the tests. Expected: PASS.

- [ ] **Step 3: Write the summaries (our words, never pasted)**

For each candidate, read its ngcc.dev report and write at most one or two sentences in your own words. Each ends with the agreed credit in parentheses and goes in `findings.implementation` (bugs in submitted code) or `findings.broken` (design breaks only: attacks below the claimed target). Per-instance flags only where a finding affects specific parameter sets. Never copy sentences, titles or IDs from the reports. Add `findings.url` for every candidate that has a report, even without flags.

- [ ] **Step 4: Regenerate, check, and commit separately for Thom's review**

```bash
uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv
uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv
uv run --with pytest --with 'ruamel.yaml>=0.18' pytest -q bench-common/ngcc/tests && npm run test && npm run build
git add data/ngcc/candidates.yaml bench-common/ngcc/ngcc_data.py bench-common/ngcc/tests/test_ngcc_data.py src/lib/__tests__/ngccCandidates.test.ts
git add $(git status --short data/schemes data/kems | awk '$1=="M"{print $2}')
git commit -m "Summarise NGCC round-1 findings with links and credit (for review)

Attribution agreed with Markku-Juhani Saarinen per Thom on <date>; credit wording: <wording>.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
Stop and ask Thom to review this commit before anything else builds on it.

---

## Self-review notes

- Spec coverage: reproducible fetch (T1), safe extraction (T2), one generic adapter + manifest (T3–T7), failures and KAT mismatches recorded (status.tsv, T4–T7), placeholder-hash use explicit and shown in notes (`hash_usage.py`, `ps_notes`), sizes via ICCS getters emitted by our harness (`--sizes`, T4–T7) and imported by the extended `update_scheme_data.py` (T9), curated claims with spec refs and unit test (T8), unmapped instances (`omit` + `check_coverage` errors), family/assumption mapping (T8), local and AWS benchmarks with env merge (T12–T13), findings gated (T14), history/home/e2e/CLAUDE.md (T10), 75 files pass `dataFiles` (T9).
- Names used across tasks: `NGCC-<instance>`, `params.tsv` columns, `status.tsv` columns, `ngcc_sizes.tsv` headers, `ngcc_data.*` functions and `--ngcc-sizes`. All are defined once, in the task that produces them.

## Controller amendment (Task C)

NGCC candidates' C code is untrusted. The local build, KAT, `--sizes` and smoke
benchmark commands in Tasks 4, 5, 6, 7, 11 and 12 (every `make -C
<suite>/schemes/ngcc`, `./bench`/`./bench-kem` invocation, and `run_bench.sh`
smoke run) must be run as `scripts/ngcc-run <command>` (inside a network-less
container), not directly on the host. AWS benchmark runs (`scripts/aws-bench.sh`)
are unchanged.
