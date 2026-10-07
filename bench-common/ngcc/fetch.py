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


def write_archives(
    comments: list[str], rows: list[dict], path: Path = ARCHIVES
) -> None:
    with open(path, "w", newline="", encoding="utf-8") as f:
        f.writelines(comments)
        writer = csv.DictWriter(
            f, fieldnames=COLUMNS, delimiter="\t", lineterminator="\n"
        )
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
        raise ArchiveError(
            f"archive differs from its pin (moved aside as {dest.name}.mismatch)"
        )
    return "downloaded" if fetched else "cached"


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Fetch and verify the NGCC round-1 archives."
    )
    ap.add_argument("ids", nargs="*", help="candidate ids (default: all)")
    ap.add_argument(
        "--record",
        action="store_true",
        help="pin bytes/sha256 of archives that have none",
    )
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
            print(
                f"{row['id']:8} {state:10} {int(row['bytes']) / 1e6:9.1f} MB  {row['zip']}"
            )
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
