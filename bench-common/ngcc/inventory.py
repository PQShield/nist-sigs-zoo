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


KAT_SUFFIXES = (".txt", ".rsp", ".req", ".kat")
IMPL_WORDS = ("reference", "optimized", "optimised", "avx2", "ref", "opt", "x86")
ROMAN = {"i": "1", "ii": "2", "iii": "3", "iv": "4", "v": "5"}


def norm(text: str) -> str:
    """Lower-case, roman-numeral tail to digit, drop everything non-alphanumeric."""
    t = text.lower().replace("+", "")
    t = re.sub(r"(?<=[-_ ])(iii|ii|iv|i|v)$", lambda m: ROMAN[m.group(1)], t)
    return re.sub(r"[^a-z0-9]+", "", t)


def kat_stem(path: Path, kind: str) -> str | None:
    """Return the instance part of a KAT file name, or None if it is not a KAT file.

    Accepts KAT_SIG_<inst>.txt / KAT_KEM_<inst>.txt (any case) and the NIST-style
    PQCsignKAT_<inst>.rsp / PQCkemKAT_<inst>.rsp (+ .req).
    """
    name = path.name
    low = name.lower()
    if not low.endswith(KAT_SUFFIXES):
        return None
    for prefix in (f"kat_{kind}_", f"pqc{'sign' if kind == 'sig' else kind}kat_"):
        if low.startswith(prefix):
            stem = name[len(prefix) :]
            while stem.lower().endswith(KAT_SUFFIXES):  # also handles ".txt.txt"
                stem = stem.rsplit(".", 1)[0]
            return stem
    return None


def best_kat(kats: list[Path], srcdir: Path) -> Path | None:
    def shared(p: Path) -> int:
        n = 0
        for a, b in zip(p.parts, srcdir.parts, strict=False):
            if a != b:
                break
            n += 1
        return n

    def same_impl(p: Path) -> bool:
        return impl_of(p.parent.as_posix()) == impl_of(srcdir.as_posix())

    return (
        min(kats, key=lambda p: (-shared(p), not same_impl(p), str(p)))
        if kats
        else None
    )


def match_kat(
    kats: list[tuple[str, Path]], keys: list[str], srcdir: Path, impl_words: list[str]
) -> Path | None:
    """Map an instance to a KAT file if exactly one distinct KAT name fits.

    `keys` are candidate instance names in priority order (declared instance name,
    then source-dir names). Per key the tiers are: exact stem, normalised stem,
    normalised stem ignoring an `_iccs` marker, then key plus an implementation word
    (e.g. OPSsig-128 -> OPSsig-128-avx2). The first key/tier with a match decides;
    if it matches several distinct KAT names it is ambiguous and yields None.
    Same-named files in different directories are told apart by path proximity.
    """

    def no_iccs(stem: str) -> str:
        return norm(re.sub(r"[-_]?iccs$", "", stem, flags=re.IGNORECASE))

    for key in keys:
        nk = norm(key)
        tiers = [
            lambda stem, key=key, nk=nk: stem == key,
            lambda stem, key=key, nk=nk: norm(stem) == nk,
            lambda stem, key=key, nk=nk: no_iccs(stem) == nk,
            lambda stem, key=key, nk=nk: any(norm(stem) == nk + w for w in impl_words),
        ]
        for tier in tiers:
            hit = [p for stem, p in kats if tier(stem)]
            if hit:
                if len({p.name.lower() for p in hit}) == 1:
                    return best_kat(hit, srcdir)
                break  # ambiguous for this key: try the next key
    return None


def find_instances(cand_root: Path, cand_id: str, kind: str) -> list[dict]:
    kats = [
        (stem, p)
        for p in sorted(cand_root.rglob("*"))
        if p.is_file() and (stem := kat_stem(p, kind)) is not None
    ]
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
        keys = [
            instance,
            *[q.name for q in (srcdir, *srcdir.parents) if q != cand_root][:3],
        ]
        words = [
            w for w in IMPL_WORDS for part in srcdir.relative_to(cand_root).parts
            if w in part.lower()
        ]  # fmt: skip
        kat = match_kat(
            kats, list(dict.fromkeys(keys)), srcdir, list(dict.fromkeys(words))
        )
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
