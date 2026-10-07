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
    kind: re.compile(
        rf"unsigned\s+long\s+long\s+{kind}_get_pk_len_bytes\s*\([^;{{)]*\)\s*\{{"
    )
    for kind in ("sig", "kem")
}
INSTANCE_DEF = re.compile(r'#\s*define\s+ALGORITHM_INSTANCE\s+"([^"]+)"')
HEADER = {"sig": "SIG_AlgorithmInstance.h", "kem": "KEM_AlgorithmInstance.h"}
WRAPPER_DIRS = {"kat_test", "algorithminstance", "api", "iccs", "niccs"}
COLUMNS = [
    "file", "name", "id", "instance", "impl", "srcdir", "srcs",
    "incs", "defs", "shimdefs", "ldlibs", "iters", "katn", "kat",
]  # fmt: skip


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
    for h in [
        api_dir / HEADER[kind],
        *sorted(api_dir.glob("*.h")),
        *sorted(api_dir.parent.glob("*.h")),
    ]:
        if h.is_file() and (m := INSTANCE_DEF.search(h.read_text(errors="replace"))):
            return m.group(1)
    return None


def best_kat(kats: list[Path], srcdir: Path) -> Path | None:
    def shared(p: Path) -> int:
        n = 0
        for a, b in zip(p.parts, srcdir.parts, strict=False):
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
            extra = [
                f"{api_dir.name}/{p.name}"
                for p in sorted(api_dir.glob("*.c"))
                if not DRIVER.match(p.name)
            ]
            incs = api_dir.name
        rel_src = srcdir.relative_to(cand_root).as_posix()
        kat = best_kat(kats.get(f"KAT_{kind.upper()}_{instance}.txt", []), srcdir)
        rows.append(
            {
                "file": slug(f"{cand_id}_{instance}"),
                "name": f"NGCC-{instance}",
                "id": cand_id,
                "instance": instance,
                "impl": impl_of(rel_src),
                "srcdir": rel_src,
                "srcs": " ".join(["*", *extra]),
                "incs": incs,
                "defs": "",
                "shimdefs": "",
                "ldlibs": "",
                "iters": "0",
                "katn": "10",
                "kat": kat.relative_to(cand_root).as_posix() if kat else "-",
            }
        )
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
