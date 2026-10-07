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
    r"^(KAT_(SIG|KEM)_[A-Za-z0-9_+.-]+\.txt|(LICEN[CS]E|COPYING|NOTICE)([._-].*)?)$",
    re.IGNORECASE,
)
MAX_MEMBER = 256 << 20
S_IFMT, S_IFLNK = 0o170000, 0o120000


def decode_name(info: zipfile.ZipInfo) -> str:
    """zipfile decodes names without the UTF-8 flag as cp437; these archives often use GBK."""
    if info.flag_bits & 0x800:
        return info.filename
    try:
        raw = info.filename.encode("cp437")
    except UnicodeEncodeError:  # already decoded from a Unicode path extra field
        return info.filename
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
            self.skipped.append(
                f"RAR not unpacked (install libarchive-tools for bsdtar): {rel}"
            )
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


def extract_archive(
    zip_path: Path, cand_id: str, src_root: Path = SRC_OUT, spec_root: Path = SPEC_OUT
) -> Extractor:
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
        if (
            not zip_path.exists()
            or not row["sha256"]
            or sha256_file(zip_path) != row["sha256"]
        ):
            print(
                f"{row['id']:8} not fetched or not matching its pin; run fetch.py first",
                file=sys.stderr,
            )
            failed += 1
            continue
        ex = extract_archive(zip_path, row["id"])
        print(
            f"{row['id']:8} {ex.counts['src']:5} source files {ex.counts['spec']:3} PDFs"
        )
        for s in ex.skipped:
            print(f"         skipped {s}")
    SRC_OUT.mkdir(exist_ok=True)
    (SRC_OUT / ".extracted").write_text(datetime.now(timezone.utc).isoformat() + "\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
