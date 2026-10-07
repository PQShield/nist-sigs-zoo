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
OPTIONAL = {
    "incs": "",
    "defs": "",
    "shimdefs": "",
    "ldlibs": "",
    "iters": "0",
    "katn": "10",
}
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
                    p
                    for p in srcdir.iterdir()
                    if p.is_file()
                    and p.suffix in SOURCE_SUFFIXES
                    and not DRIVER.match(p.name)
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
    others = [
        f"-I$(BUILD)/src/{row['file']}/{d}"
        for d in dirs
        if not d.startswith("patches/")
    ]
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
    out = [
        "# Generated by bench-common/ngcc/gen_rules.py from params.tsv. Do not edit.\n",
        "NGCC_SOS :=\n\n",
    ]
    for row in rows:
        sources = resolve_sources(row, src_root, suite_dir)
        paths = [rule_path(p, row, src_root, suite_dir) for p in sources]
        why = problem(sources, paths)
        if why:
            out.append(
                BAD.format(
                    file=row["file"], name=row["name"], why=why.replace('"', "'")
                )
            )
        else:
            out.append(
                GOOD.format(
                    file=row["file"],
                    name=row["name"],
                    deps=" ".join(paths),
                    incs=inc_flags(row),
                    defs=row["defs"],
                    shimdefs=row["shimdefs"],
                    ldlibs=row["ldlibs"],
                )
            )
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
    ap = argparse.ArgumentParser(
        description="params.tsv -> rules.mk for NGCC instances"
    )
    ap.add_argument("--params", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("--build", required=True)
    ap.add_argument("--suite-dir", default=".")
    ap.add_argument(
        "--template", required=True, help="shim template with @COLUMN@ tokens"
    )
    ap.add_argument("--out", required=True)
    args = ap.parse_args(argv)
    try:
        rows = read_params(Path(args.params))
    except ParamsError as e:
        print(f"gen_rules: {e}", file=sys.stderr)
        return 2
    src, build, suite = (
        Path(args.src).resolve(),
        Path(args.build).resolve(),
        Path(args.suite_dir).resolve(),
    )
    write_shims(rows, Path(args.template), suite)
    make_links(rows, src, build)
    Path(args.out).write_text(render_rules(rows, src, suite), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
