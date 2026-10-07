#!/usr/bin/env python3
"""Trusted host-side exporter for scripts/ngcc-run.

Reads a tar stream of the container's /work (as produced by `docker cp ctr:/work -`)
on stdin and writes ONLY allowlisted, plain-text regular files under --out.
Everything else (symlinks, hardlinks, devices, `..`/absolute paths, binaries,
NUL bytes, non-UTF-8, oversize, non-allowlisted names) is listed on stderr as
rejected. Files are written with mode 0644. Files identical to the same path
in --repo are skipped. Nothing here executes any content.
"""

import argparse
import fnmatch
import os
import sys
import tarfile
from pathlib import PurePosixPath

MAX_FILE = 64 * 1024 * 1024
MAX_TOTAL = 512 * 1024 * 1024
TMP_PREFIX = ".ngcc-tmp/"  # entry.sh stages text files from /tmp here
TMP_EXT = (".tsv", ".txt", ".log", ".rsp")
ALLOW = (
    "*/schemes/ngcc/*.tsv",
    "*/ngcc_sizes.tsv",
    "ngcc_sizes.tsv",
    "*/results/*.txt",
    "*.log",
    "*/build/*.rsp",
    "*/build/KAT*.txt",
)
EXPECTED_LINKS = ("bench-common/ngcc/src", "bench-common/ngcc/specs")
DENY_PREFIX = ("bench-common/ngcc/src/", "bench-common/ngcc/specs/")


def safe_rel(name: str) -> str | None:
    """Return a normalised relative path (first tar component stripped), or None."""
    if "\0" in name or name.startswith("/") or "\\" in name:
        return None
    parts = PurePosixPath(name).parts
    if not parts or ".." in parts:
        return None
    rel = parts[1:]  # drop the leading "work"
    if not rel or any(p in ("", ".") for p in rel):
        return None
    return "/".join(rel)


def allowed(rel: str) -> str | None:
    """Return the destination path under --out, or None if not allowlisted."""
    if rel.startswith(DENY_PREFIX):
        return None
    if rel.startswith(TMP_PREFIX):
        return "tmp/" + rel[len(TMP_PREFIX) :] if rel.endswith(TMP_EXT) else None
    if any(fnmatch.fnmatchcase(rel, g) for g in ALLOW):
        return rel
    return None


def export_tar(fileobj, out: str, repo: str | None = None, log=sys.stderr):
    written, rejected, total = [], [], 0
    out = os.path.realpath(out)
    os.makedirs(out, exist_ok=True)

    def reject(name, why):
        rejected.append((name, why))
        print(f"rejected {name!r}: {why}", file=log)

    with tarfile.open(fileobj=fileobj, mode="r|") as tf:
        for m in tf:
            if m.isdir():
                continue
            if not m.isreg():
                # The extracted-source symlinks entry.sh creates are expected.
                if safe_rel(m.name) not in EXPECTED_LINKS:
                    reject(m.name, "not a regular file")
                continue
            rel = safe_rel(m.name)
            if rel is None:
                reject(m.name, "unsafe path")
                continue
            dest = allowed(rel)
            if dest is None:
                continue  # not allowlisted: silently ignored (binaries, sources, ...)
            if m.size > MAX_FILE:
                reject(m.name, "too large")
                continue
            if total + m.size > MAX_TOTAL:
                reject(m.name, "total size cap")
                continue
            data = tf.extractfile(m).read()
            if b"\0" in data:
                reject(m.name, "contains NUL bytes")
                continue
            try:
                data.decode("utf-8")
            except UnicodeDecodeError:
                reject(m.name, "not valid UTF-8")
                continue
            if repo is not None and not dest.startswith("tmp/"):
                try:
                    with open(os.path.join(repo, dest), "rb") as f:
                        if f.read() == data:
                            continue  # unchanged from the repo
                except OSError:
                    pass
            path = os.path.join(out, dest)
            parent = os.path.dirname(path)
            os.makedirs(parent, exist_ok=True)
            real_parent = os.path.realpath(parent)
            if real_parent != out and not real_parent.startswith(out + os.sep):
                reject(m.name, "destination escapes --out")
                continue
            tmp = path + ".part"
            fd = os.open(
                tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC | os.O_NOFOLLOW, 0o644
            )
            with os.fdopen(fd, "wb") as f:
                f.write(data)
                os.fchmod(f.fileno(), 0o644)
            os.replace(tmp, path)  # replaces a pre-existing symlink, never follows it
            total += len(data)
            written.append(dest)
            print(f"exported {dest}", file=log)
    return written, rejected


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", required=True)
    ap.add_argument("--repo", help="skip files identical to the same path here")
    a = ap.parse_args()
    export_tar(sys.stdin.buffer, a.out, a.repo)


if __name__ == "__main__":
    main()
