#!/usr/bin/env python3
"""Record build, KAT and hash status for every NGCC instance of a params.tsv.

  build  ok if BUILD/<file>.so exists, else FAIL (see BUILD/<file>.log)
  kat    our ngcc_kat replay of the row's first `katn` records compared with the
         submitted KAT file's record hashes in kat_hashes.tsv (kat_hashes.py; the
         KAT files themselves are not needed): PASS, MISMATCH, NOKAT (no submitted
         file, or no hashes for it), CRYPTOFAIL, LOADFAIL, CRASH, TIMEOUT; '-' if not built
  katrec first mismatching record index (0-based) for MISMATCH, else empty
  hash   placeholder | sm3 | own (hash_usage.py)
  impl   copied from params.tsv (ref | opt)

The output is committed as <suite>/schemes/ngcc/status.tsv and read by the YAML import.
Per-instance KAT timeout: NGCC_KAT_TIMEOUT seconds (default 900).

Usage: status.py --kind sig|kem --params P --src SRC --build BUILD --suite-dir DIR
                 [--hashes kat_hashes.tsv] --out status.tsv
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
from kat_compare import check
from kat_hashes import OUT as HASHES
from kat_hashes import load as load_hashes

HERE = Path(__file__).resolve().parent
COLUMNS = ["file", "name", "id", "instance", "impl", "build", "kat", "hash", "katrec"]
EXIT = {2: "LOADFAIL", 3: "CRYPTOFAIL"}
TIMEOUT = int(os.environ.get("NGCC_KAT_TIMEOUT", "900"))


def build_kat_tool(build: Path) -> Path:
    exe, src = build / "ngcc_kat", HERE / "ngcc_kat.c"
    if not exe.exists() or exe.stat().st_mtime < src.stat().st_mtime:
        cc = os.environ.get("CC", "cc")
        subprocess.run(
            [cc, "-O2", "-std=gnu17", "-o", str(exe), str(src), "-ldl"], check=True
        )
    return exe


def kat_status(
    exe: Path, kind: str, so: Path, expected: list[str] | None, records: int
) -> tuple[str, int | None]:
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "kat.txt"
        try:
            proc = subprocess.run(
                [str(exe), kind, str(so.resolve()), str(out), str(records)],
                cwd=tmp,
                capture_output=True,
                timeout=TIMEOUT,
                check=False,
            )
        except subprocess.TimeoutExpired:
            return "TIMEOUT", None
        if proc.returncode < 0:
            return "CRASH", None
        if proc.returncode:
            return EXIT.get(proc.returncode, "CRASH"), None
        if expected is None:
            return "NOKAT", None
        return check(out.read_text(errors="replace"), expected, records)


def status_row(
    row: dict,
    kind: str,
    exe: Path,
    src_root: Path,
    build: Path,
    suite_dir: Path,
    hashes: dict[tuple[str, str], list[str]],
) -> dict:
    srcdir = instance_dir(row, src_root)
    headers = sorted(srcdir.glob("*.h")) if srcdir.is_dir() else []
    out = {k: row[k] for k in ("file", "name", "id", "instance", "impl")}
    out["katrec"] = ""
    out["hash"] = hash_usage([*resolve_sources(row, src_root, suite_dir), *headers])
    so = build / f"{row['file']}.so"
    if not so.exists():
        out.update(build="FAIL", kat="-")
        return out
    expected = None
    if row["kat"] != "-":
        expected = hashes.get((row["id"], row["kat"]))
        if expected is None:
            print(
                f"{row['file']}: no hashes for {row['id']}/{row['kat']} (kat_hashes.py)"
            )
    kat, first = kat_status(exe, kind, so, expected, int(row["katn"]))
    out.update(build="ok", kat=kat, katrec="" if first is None else str(first))
    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="NGCC build/KAT/hash status")
    ap.add_argument("--kind", choices=("sig", "kem"), required=True)
    for opt in ("--params", "--src", "--build", "--out"):
        ap.add_argument(opt, required=True)
    ap.add_argument("--suite-dir", default=".")
    ap.add_argument("--hashes", default=str(HASHES))
    args = ap.parse_args(argv)
    hashes = load_hashes(Path(args.hashes))

    rows = read_params(Path(args.params))
    src, build, suite = (
        Path(args.src).resolve(),
        Path(args.build).resolve(),
        Path(args.suite_dir).resolve(),
    )
    exe = build_kat_tool(build)
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        result = list(
            pool.map(
                lambda r: status_row(r, args.kind, exe, src, build, suite, hashes), rows
            )
        )
    with open(args.out, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f, fieldnames=COLUMNS, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(result)
    print("build:", dict(Counter(r["build"] for r in result)))
    print("kat:  ", dict(Counter(r["kat"] for r in result)))
    print("hash: ", dict(Counter(r["hash"] for r in result)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
