#!/usr/bin/env python3
"""Write kat_hashes.tsv: per-record hashes of the submitted KAT files.

The NGCC KATs are deterministic (ICCS seed schedule), so status.py only needs a
hash per record of each submitted file, not the files themselves (several GB).
For every KAT file named in a params.tsv `kat` column this reads the file
straight from the pinned archive in $NGCC_CACHE (nested zips included; nothing
is extracted or executed) and records

  record = raw   sha256 of the submitted file's bytes (provenance)
  record = 0..9  sha256 of the canonical form of that record (kat_compare.py)

Only the first 10 records are hashed: ngcc_kat replays at most 10 (`katn`).

Usage (repo root; reads untrusted archives, so run isolated):
    python3 -I bench-common/ngcc/kat_hashes.py            # both suites, from the archives
    python3 -I bench-common/ngcc/kat_hashes.py --src DIR --params P --out F   # from extracted files
Re-run it whenever a `kat` column changes; commit kat_hashes.tsv.
"""

import argparse
import hashlib
import sys
import zipfile
from pathlib import Path, PurePosixPath

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from extract import Extractor
from fetch import cache_dir, read_archives, sha256_file
from gen_rules import read_params
from kat_compare import MAX_RECORDS, canonical_records, record_hash

REPO = HERE.parents[1]
PARAMS = [
    REPO / "bench/schemes/ngcc/params.tsv",
    REPO / "bench-kem/schemes/ngcc/params.tsv",
]
OUT = HERE / "kat_hashes.tsv"
COLUMNS = ["suite", "id", "kat", "record", "sha256"]
HEADER = """\
# Per-record sha256 of the submitted NGCC KAT files, written by kat_hashes.py; commit it.
# record=raw: sha256 of the submitted file's bytes. record=0..9: sha256 of the
# canonical form of that record (defined in kat_compare.py). status.py compares
# our ngcc_kat replay with these, so the KAT files themselves are not needed.
"""


def suite_of(cand_id: str) -> str:
    return "sig" if cand_id.startswith("sign-") else "kem"


def hash_stream(stream) -> tuple[str, list[str]]:
    """sha256 of all bytes, and the hashes of the first MAX_RECORDS canonical records."""
    raw = hashlib.sha256()

    def lines():
        for line in stream:
            raw.update(line)
            yield line.decode("utf-8", errors="replace")

    gen = lines()
    records = [record_hash(r) for r in canonical_records(gen, MAX_RECORDS)]
    for _ in gen:  # hash the rest of the file
        pass
    return raw.hexdigest(), records


def from_archive(cand_id: str, wanted: set[str], found: dict) -> None:
    rows = {r["id"]: r for r in read_archives()[1]}
    zip_path = cache_dir() / f"{cand_id}.zip"
    if not zip_path.exists() or sha256_file(zip_path) != rows[cand_id]["sha256"]:
        print(f"{cand_id}: not fetched or not matching its pin", file=sys.stderr)
        return

    def sink(rel: PurePosixPath, stream) -> None:
        found[(cand_id, rel.as_posix())] = hash_stream(stream)

    ex = Extractor(
        Path("/nonexistent"),
        Path("/nonexistent"),
        kat_wanted={PurePosixPath(k) for k in wanted},
        kat_sink=sink,
    )
    with zipfile.ZipFile(zip_path) as zf:
        ex.zip(zf, PurePosixPath())
    for s in ex.skipped:
        print(f"{cand_id}: skipped {s}", file=sys.stderr)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--params", action="append", type=Path)
    ap.add_argument("--src", type=Path, help="read extracted files from DIR/<id>/<kat>")
    ap.add_argument("--out", type=Path, default=OUT)
    args = ap.parse_args(argv)

    wanted: dict[str, set[str]] = {}
    for params in args.params or PARAMS:
        for row in read_params(params):
            if row["kat"] != "-":
                wanted.setdefault(row["id"], set()).add(row["kat"])

    found: dict[tuple[str, str], tuple[str, list[str]]] = {}
    for cand_id in sorted(wanted):
        if args.src is None:
            from_archive(cand_id, wanted[cand_id], found)
            continue
        for kat in sorted(wanted[cand_id]):
            path = args.src / cand_id / kat
            if path.is_file():
                with open(path, "rb") as f:
                    found[(cand_id, kat)] = hash_stream(f)

    missing = [
        f"{i}/{k}"
        for i in sorted(wanted)
        for k in sorted(wanted[i])
        if (i, k) not in found
    ]
    with open(args.out, "w", encoding="utf-8", newline="") as f:
        f.write(HEADER)
        f.write("\t".join(COLUMNS) + "\n")
        for (cand_id, kat), (raw, records) in sorted(found.items()):
            suite = suite_of(cand_id)
            f.write(f"{suite}\t{cand_id}\t{kat}\traw\t{raw}\n")
            f.writelines(
                f"{suite}\t{cand_id}\t{kat}\t{i}\t{h}\n" for i, h in enumerate(records)
            )
    print(f"{len(found)} KAT files hashed into {args.out}")
    for m in missing:
        print(f"missing KAT file: {m}", file=sys.stderr)
    return 1 if missing else 0


def load(path: Path = OUT) -> dict[tuple[str, str], list[str]]:
    """(id, kat) -> record hashes in order, from a kat_hashes.tsv."""
    out: dict[tuple[str, str], list[str]] = {}
    with open(path, encoding="utf-8") as f:
        lines = [ln.rstrip("\n") for ln in f if not ln.startswith("#")]
    for line in lines[1:]:
        _suite, cand_id, kat, record, digest = line.split("\t")
        recs = out.setdefault((cand_id, kat), [])
        if record != "raw":
            if int(record) != len(recs):
                raise ValueError(f"{path}: records out of order for {cand_id} {kat}")
            recs.append(digest)
    return out


if __name__ == "__main__":
    sys.exit(main())
