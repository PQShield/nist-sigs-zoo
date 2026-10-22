#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# ///
"""Extract sources, KAT files and specification PDFs from the cached NGCC archives.

The archives are untrusted input. Members are filtered by an allowlist, paths
are checked for traversal, links are skipped and sizes are capped. Nested zip
files are unpacked through a temporary file. Nothing from an archive is ever
executed, and every file is written with mode 0644.

Nested RAR archives (DOVE, sign-09) are never unpacked on the host: they are
copied unchanged to src/<id>/<path>.rar. unpack_nested.py unpacks them inside
the container (scripts/ngcc-run), the export allowlist brings back their text
files, and `extract.py --import-unpacked DIR` files them under src/ with the
same allowlist and limits:

    flock /tmp/ngcc-run.lock scripts/ngcc-run python3 -I bench-common/ngcc/unpack_nested.py
    python3 -I bench-common/ngcc/extract.py --import-unpacked .ngcc-out/unpacked

Allowlist: C/asm sources and headers, licences, and build and documentation
files as plain text for reading (Makefile, *.mk, CMake, meson.build, *.sh, *.pro,
*.vcxproj, README*, *.md and *.txt that are not KATs). Build files are for
reading only: never run them.

KAT files (KAT_SIG_*/KAT_KEM_*, *.rsp/*.req, *.txt under a KAT/test-vector name)
are skipped: status.py needs only their record hashes, committed in
kat_hashes.tsv (kat_hashes.py reads them straight from the archives). `--kats`
extracts them too, for diagnosis (kat_compare.py) and for inventory.py's `kat`
column; oversize ones keep their first 10 records.

Output (git-ignored, rebuilt from scratch per candidate):
    bench-common/ngcc/src/<id>/...     allowlisted text files (above), nested *.rar
    bench-common/ngcc/specs/<id>/...   every PDF
    bench-common/ngcc/src/.extracted   marker that ngcc.mk checks before building

Usage: bench-common/ngcc/extract.py [--kats] [id ...]
       bench-common/ngcc/extract.py --import-unpacked DIR
"""

import os
import re
import shutil
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
    r"^(LICEN[CS]E|COPYING|NOTICE)([._-].*)?$",
    re.IGNORECASE,
)
# Build and documentation files, extracted as plain text for reading only.
TEXT_NAMES = {"makefile", "gnumakefile", "cmakelists.txt", "meson.build", "build.sh"}
TEXT_SUFFIXES = {".mk", ".cmake", ".sh", ".pro", ".vcxproj", ".md", ".txt"}
# A .txt file is a KAT/test-vector file, not documentation, when its name or a
# directory above it says so.
KATISH = re.compile(r"kat|test[-_ ]?vector|testvec|intermediate", re.IGNORECASE)
MAX_TEXT = 2 << 20
MAX_MEMBER = 256 << 20
KAT_PREFIX_CAP = 512 << 20
MAX_NESTED = 1 << 30
MAX_DEPTH = 3
KAT_NAME = re.compile(r"^KAT_(SIG|KEM)_[A-Za-z0-9_+.-]+\.txt$", re.IGNORECASE)
S_IFMT, S_IFLNK = 0o170000, 0o120000
ID = re.compile(r"^(sign|kem)-\d\d$")


class _Skip(Exception):
    pass


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


def is_kat_text(p: PurePosixPath) -> bool:
    """A .txt/.rsp/.req file that holds test vectors rather than prose."""
    suffix = p.suffix.lower()
    if suffix in (".rsp", ".req"):
        return True
    return suffix == ".txt" and any(KATISH.search(part) for part in p.parts)


def classify(p: PurePosixPath) -> str | None:
    suffix, name = p.suffix.lower(), p.name.lower()
    if suffix == ".pdf":
        return "spec"
    if suffix in SOURCE_SUFFIXES or KEEP_NAME.match(p.name):
        return "src"
    if suffix == ".zip":
        return "archive"
    if suffix == ".rar":
        return "rar"
    if name in TEXT_NAMES or name.startswith("readme"):
        return "text"
    if KAT_NAME.match(p.name) or is_kat_text(p):
        return "kat"
    if suffix in TEXT_SUFFIXES:
        return "text"
    return None


class Extractor:
    def __init__(
        self,
        src_out: Path,
        spec_out: Path,
        max_member: int = MAX_MEMBER,
        prefix_cap: int = KAT_PREFIX_CAP,
        keep_kats: bool = False,
        kat_wanted: set[PurePosixPath] | None = None,
        kat_sink=None,
    ):
        """keep_kats: also write KAT files. kat_sink(rel, stream): hash-only mode,
        called for each member in kat_wanted; nothing is written then."""
        self.out = {
            "src": src_out,
            "text": src_out,
            "rar": src_out,
            "kat": src_out,
            "spec": spec_out,
        }
        self.counts = {"src": 0, "text": 0, "rar": 0, "kat": 0, "spec": 0}
        self.keep_kats = keep_kats
        self.kat_wanted = kat_wanted or set()
        self.kat_sink = kat_sink
        self.skipped: list[str] = []
        self.truncated: list[str] = []
        self.max_member = max_member
        self.prefix_cap = prefix_cap

    def write(
        self, kind: str, rel: PurePosixPath, stream, size: int | None = None
    ) -> None:
        """Write one member (mode 0644). Oversize KAT files keep their first 10 records."""
        cap = {"text": MAX_TEXT, "rar": MAX_NESTED}.get(kind, self.max_member)
        truncate = size is not None and size > cap and kind == "kat"
        if size is not None and size > cap and not truncate:
            self.skipped.append(f"too large: {rel}")
            return
        dest = self.out[kind] / rel
        kept = 0
        try:
            dest.parent.mkdir(parents=True, exist_ok=True)
            with open(dest, "wb") as out:
                if truncate:
                    kept = self.copy_kat_prefix(stream, out)
                    if kept is None:
                        raise _Skip(f"KAT prefix exceeds cap: {rel}")
                else:
                    shutil.copyfileobj(stream, out)
            os.chmod(dest, 0o644)
        except _Skip as e:
            dest.unlink(missing_ok=True)
            self.skipped.append(str(e))
            return
        except (OSError, RuntimeError, zipfile.BadZipFile, EOFError) as e:
            # directory/file collisions, encrypted members, CRC errors
            if dest.is_file():
                dest.unlink(missing_ok=True)
            self.skipped.append(f"cannot extract {rel}: {type(e).__name__}: {e}")
            return
        if truncate:
            self.truncated.append(f"truncated KAT (first {kept} records): {rel}")
        self.counts[kind] += 1

    def copy_kat_prefix(self, stream, out) -> int | None:
        """Copy up to 10 whole records, fewer if the byte cap is hit.

        Returns the number of records kept, or None if not even one fits.
        """
        written = seen = rec_start = 0
        for line in stream:
            if line.startswith(b"Count = "):
                if seen == 10:
                    break
                seen += 1
                rec_start = written
            written += len(line)
            if written > self.prefix_cap:
                if seen <= 1:
                    return None
                out.seek(rec_start)
                out.truncate()
                return seen - 1
            out.write(line)
        return seen

    def zip(self, zf: zipfile.ZipFile, prefix: PurePosixPath, depth: int = 0) -> None:
        for info in zf.infolist():
            if info.is_dir() or (info.external_attr >> 16) & S_IFMT == S_IFLNK:
                continue
            rel = safe_relpath(decode_name(info))
            if rel is None:
                self.skipped.append(f"unsafe path: {info.filename!r}")
                continue
            rel = prefix / rel
            kind = classify(rel)
            if self.kat_sink is not None:  # hash-only mode
                if rel in self.kat_wanted:
                    try:
                        with zf.open(info) as stream:
                            self.kat_sink(rel, stream)
                    except (RuntimeError, zipfile.BadZipFile, OSError, EOFError) as e:
                        self.skipped.append(
                            f"cannot read {rel}: {type(e).__name__}: {e}"
                        )
                    continue
                if kind != "archive":
                    continue
            if kind is None or (kind == "kat" and not self.keep_kats):
                continue
            if kind in ("archive", "rar") and (
                depth >= MAX_DEPTH or info.file_size > MAX_NESTED
            ):
                self.skipped.append(f"nested archive too deep or large: {rel}")
                continue
            try:
                with zf.open(info) as stream:
                    if kind == "archive":
                        self.nested(rel, stream, depth + 1)
                    else:
                        self.write(kind, rel, stream, info.file_size)
            except (RuntimeError, zipfile.BadZipFile, OSError, EOFError) as e:
                self.skipped.append(f"cannot read {rel}: {type(e).__name__}: {e}")

    def nested(self, rel: PurePosixPath, stream, depth: int = 1) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "inner"
            with open(path, "wb") as out:
                shutil.copyfileobj(stream, out)
            try:
                with zipfile.ZipFile(path) as inner:
                    self.zip(inner, rel.with_suffix(""), depth)
            except zipfile.BadZipFile:
                self.skipped.append(f"bad nested zip: {rel}")

    def import_tree(self, root: Path) -> None:
        """File the text files unpack_nested.py brought back (DIR/<path>) under src/."""
        for f in sorted(root.rglob("*")):
            if f.is_symlink() or not f.is_file():
                continue
            rel = safe_relpath(f.relative_to(root).as_posix())
            kind = classify(rel) if rel else None
            if kind not in ("src", "text"):
                self.skipped.append(f"not allowlisted: {f.relative_to(root)}")
                continue
            with open(f, "rb") as stream:
                self.write(kind, rel, stream, f.stat().st_size)


def extract_archive(
    zip_path: Path,
    cand_id: str,
    src_root: Path = SRC_OUT,
    spec_root: Path = SPEC_OUT,
    **limits,
) -> Extractor:
    for root in (src_root, spec_root):
        shutil.rmtree(root / cand_id, ignore_errors=True)
    ex = Extractor(src_root / cand_id, spec_root / cand_id, **limits)
    with zipfile.ZipFile(zip_path) as zf:
        ex.zip(zf, PurePosixPath())
    return ex


def import_unpacked(root: Path, src_root: Path = SRC_OUT) -> int:
    """Import DIR/<id>/... (unpack_nested.py output) into src/<id>/... (no reset)."""
    for cand in sorted(root.iterdir()):
        if not cand.is_dir() or cand.is_symlink() or not ID.match(cand.name):
            continue
        ex = Extractor(src_root / cand.name, SPEC_OUT / cand.name)
        ex.import_tree(cand)
        print(
            f"{cand.name:8} {ex.counts['src']:5} source files "
            f"{ex.counts['text']:4} text files (imported)"
        )
        for s in ex.skipped:
            print(f"         skipped {s}")
    return 0


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if args[:1] == ["--import-unpacked"]:
        if len(args) != 2:
            print(__doc__, file=sys.stderr)
            return 2
        return import_unpacked(Path(args[1]))
    keep_kats = "--kats" in args
    ids = {a for a in args if a != "--kats"}
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
        ex = extract_archive(zip_path, row["id"], keep_kats=keep_kats)
        print(
            f"{row['id']:8} {ex.counts['src']:5} source files {ex.counts['text']:4} text files "
            f"{ex.counts['kat']:3} KATs {ex.counts['spec']:3} PDFs "
            f"{ex.counts['rar']:2} RAR (unpack_nested.py)"
        )
        for s in ex.truncated:
            print(f"         {s}")
        for s in ex.skipped:
            print(f"         skipped {s}")
    if not ids and not failed:
        SRC_OUT.mkdir(exist_ok=True)
        (SRC_OUT / ".extracted").write_text(
            datetime.now(timezone.utc).isoformat() + "\n"
        )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
