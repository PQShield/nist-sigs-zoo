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
  iters     benchmark iteration override (default 0 = harness default), or `skip`: the
            instance is built, KAT-checked and listed by --sizes, but the timed benchmark
            leaves it out (shim INFO.iters = -1, see bench/harness.h). Comment why above the row.
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
SKIP = "skip"  # iters value: built and KAT-checked, not timed
DRIVER = re.compile(r"^(KAT_|PQCgenKAT|main|test|bench|speed|cpucycles)", re.IGNORECASE)
SOURCE_SUFFIXES = {".c", ".S", ".s"}
LABEL = re.compile(r"^[a-z0-9_]+$")
ID = re.compile(r"^(sign|kem)-\d\d$")
NAME = re.compile(r"^NGCC-[A-Za-z0-9._+-]+$")
INSTANCE = re.compile(r"^[A-Za-z0-9._+-]+$")
SAFE_PATH = re.compile(
    r"^[A-Za-z0-9._+/-]+$"
)  # anything that reaches a make target or recipe
FORBIDDEN = set(";&|`<>()\\$\n\r\t")
MACRO = re.compile(
    r"""^-[DU][A-Za-z_][A-Za-z0-9_]*(=([A-Za-z0-9_./+-]*|'"[A-Za-z0-9_./+-]*"'))?$"""
)
LIB = re.compile(r"^-l[A-Za-z0-9_+.-]+$")

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
\t@rm -f $@; echo "  FAIL  {name}: {why}" | tee $(BUILD)/{file}.log; exit 1

"""


class ParamsError(ValueError):
    pass


def unsafe_relpath(p: str) -> bool:
    return p.startswith("/") or ".." in p.replace("\\", "/").split("/") or "\\" in p


def check_row_values(row: dict, where: str) -> None:
    if unsafe_relpath(row["srcdir"]):
        raise ParamsError(f"{where}: srcdir must be relative without '..'")
    for item in row["srcs"].split():
        path = item.removeprefix("-")
        if path != "*" and (unsafe_relpath(path) or not SAFE_PATH.match(path)):
            raise ParamsError(f"{where}: unsafe srcs item {item!r}")
    for d in row["incs"].split():
        if unsafe_relpath(d) or not SAFE_PATH.match(d):
            raise ParamsError(f"{where}: unsafe incs item {d!r}")
    if row["kat"] != "-" and unsafe_relpath(row["kat"]):
        raise ParamsError(f"{where}: kat must be relative without '..'")
    for col in ("defs", "shimdefs"):
        for tok in row[col].split():
            if not MACRO.match(tok):
                raise ParamsError(
                    f"{where}: {col} token {tok!r} is not a plain -D/-U macro"
                )
    for tok in row["ldlibs"].split():
        if not LIB.match(tok):
            raise ParamsError(f"{where}: ldlibs token {tok!r} is not -l<name>")


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
        if bad := sorted({c for v in row.values() for c in v if c in FORBIDDEN}):
            raise ParamsError(f"{where}: forbidden character(s) {bad!r}")
        if not LABEL.match(row["file"]):
            raise ParamsError(f"{where}: file label must match [a-z0-9_]+")
        if not NAME.match(row["name"]):
            raise ParamsError(
                f"{where}: name must be NGCC-<instance> with [A-Za-z0-9._+-]"
            )
        if not INSTANCE.match(row["instance"]):
            raise ParamsError(f"{where}: instance must match [A-Za-z0-9._+-]+")
        check_row_values(row, where)
        if not ID.match(row["id"]):
            raise ParamsError(f"{where}: bad candidate id {row['id']!r}")
        if row["impl"] not in ("ref", "opt"):
            raise ParamsError(f"{where}: impl must be ref or opt")
        if (
            not re.fullmatch(r"\d+|skip", row["iters"])
            or not 1 <= int(row["katn"]) <= 10
        ):
            raise ParamsError(
                f"{where}: katn must be 1-10 and iters a number or 'skip'"
            )
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


def under(p: Path, root: Path) -> bool:
    return p.resolve().is_relative_to(root.resolve())


def problem(
    row: dict, sources: list[Path], paths: list[str], src_root: Path, suite_dir: Path
) -> str | None:
    cand = src_root / row["id"]
    patches = suite_dir / "patches" / row["id"]
    if not under(instance_dir(row, src_root), cand):
        return "srcdir escapes the candidate directory"
    if not sources:
        return "no sources (is src/ extracted and srcdir right?)"
    for p in sources:
        if not (under(p, cand) or under(p, patches)):
            return "source outside the candidate or its patches"
    for d in row["incs"].split():
        base = (
            suite_dir / d
            if d.startswith("patches/")
            else instance_dir(row, src_root) / d
        )
        if not (under(base, cand) or under(base, patches)):
            return "include dir escapes the candidate or its patches"
    if any(
        not SAFE_PATH.match(
            rp.replace("$(BUILD)", "BUILD").replace("(", "").replace(")", "")
        )
        for rp in paths
    ):
        return "source path has characters outside [A-Za-z0-9._+/-]"
    if missing := [p for p in sources if not p.is_file()]:
        return "missing source " + re.sub(r"[^A-Za-z0-9._+-]", "?", missing[0].name)
    if not any("drng" in p.name.lower() for p in sources):
        return "no drng source in srcs (randomness must come from drng_algorithm)"
    return None


def render_rules(rows: list[dict], src_root: Path, suite_dir: Path) -> str:
    out = [
        "# Generated by bench-common/ngcc/gen_rules.py from params.tsv. Do not edit.\n",
        "NGCC_SOS :=\n\n",
    ]
    for row in rows:
        sources = resolve_sources(row, src_root, suite_dir)
        paths = [rule_path(p, row, src_root, suite_dir) for p in sources]
        why = problem(row, sources, paths, src_root, suite_dir)
        if why:
            out.append(
                BAD.format(
                    file=row["file"],
                    name=row["name"],
                    why=re.sub(r"[^A-Za-z0-9 ._()/?,'-]", "?", why),
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
            if key == "iters" and value == SKIP:
                value = "-1"  # bench_scheme_info_t.iters < 0: build and size only, never timed
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
