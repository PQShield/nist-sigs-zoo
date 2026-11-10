#!/usr/bin/env python3
"""Unpack nested RAR archives of extracted NGCC candidates. CONTAINER ONLY.

extract.py copies nested *.rar members unchanged to src/<id>/<path>.rar; it
never unpacks them on the host. This script runs inside scripts/ngcc-run (which
has bsdtar from libarchive-tools), unpacks each one into /tmp and copies the
files that extract.classify() allowlists as text (sources, headers, licences,
build and documentation files) to .ngcc-unpacked/<id>/<path>/<member>. The
host exporter only brings back plain UTF-8 text from there; files that are not
UTF-8 are transcoded from GBK (they are listed in .ngcc-unpacked/transcoded.txt).
Then `extract.py --import-unpacked .ngcc-out/unpacked` files them under src/.

Nothing unpacked is executed here either: bsdtar only extracts.

Usage (repo root, via ngcc-run): python3 -I bench-common/ngcc/unpack_nested.py
"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from extract import MAX_MEMBER, MAX_TEXT, classify, safe_relpath

OUT = Path(".ngcc-unpacked")


def text_bytes(data: bytes) -> tuple[bytes, bool] | None:
    """UTF-8 bytes of a text file and whether they were transcoded; None if binary."""
    if b"\0" in data:
        return None
    try:
        data.decode("utf-8")
        return data, False
    except UnicodeDecodeError:
        pass
    try:
        return data.decode("gbk").encode("utf-8"), True
    except UnicodeDecodeError:
        return None


def unpack(rar: Path, src: Path, out: Path, log: list[str]) -> int:
    rel = rar.relative_to(src)
    dest_root = out / rel.with_suffix("")
    kept = 0
    with tempfile.TemporaryDirectory() as tmp:
        # bsdtar refuses absolute and '..' member paths unless -P is given.
        proc = subprocess.run(
            ["bsdtar", "-xf", str(rar), "-C", tmp],
            capture_output=True,
            text=True,
            check=False,
        )
        if proc.returncode:
            print(f"bsdtar failed ({proc.returncode}): {rel}: {proc.stderr.strip()}")
        root = Path(tmp)
        for f in sorted(root.rglob("*")):
            if f.is_symlink() or not f.is_file():
                continue
            sub = safe_relpath(f.relative_to(root).as_posix())
            kind = classify(sub) if sub else None
            if kind not in ("src", "text"):
                continue
            if f.stat().st_size > (MAX_TEXT if kind == "text" else MAX_MEMBER):
                print(f"too large: {rel}/{sub}")
                continue
            res = text_bytes(f.read_bytes())
            if res is None:
                print(f"not text: {rel}/{sub}")
                continue
            data, transcoded = res
            dest = dest_root / sub
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
            dest.chmod(0o644)
            if transcoded:
                log.append(f"{rel.with_suffix('')}/{sub}")
            kept += 1
    return kept


def main() -> int:
    if shutil.which("bsdtar") is None:
        print("unpack_nested.py: bsdtar missing; run it through scripts/ngcc-run")
        return 1
    src = HERE / "src"
    out = OUT.resolve()
    out.mkdir(exist_ok=True)
    transcoded: list[str] = []
    for rar in sorted(src.glob("*/**/*.rar")):
        if rar.is_symlink() or not rar.is_file():
            continue
        n = unpack(rar, src, out, transcoded)
        print(f"{rar.relative_to(src)}: {n} text files")
    (out / "transcoded.txt").write_text("".join(f"{t}\n" for t in transcoded))
    return 0


if __name__ == "__main__":
    sys.exit(main())
