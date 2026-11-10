"""Per-row input fingerprints, benchmark provenance and staleness.

A benchmark row (one parameter set in <suite>/schemes/*/*params*.tsv) needs re-running
when something that determines its timing changed since it was last measured, or when
it was measured on other hardware. This module computes, for every row, a short hash of
those inputs, split into components so a report can say which input changed:

  source   non-NGCC: the scheme dir's submodule commits (gitlinks), its .source
           (tarball URL) and vendored archives (*.zip, hashed as bytes); NGCC: the candidate's archive sha256 in
           bench-common/ngcc/archives.tsv
  params   the row's own params.tsv line (column=value), without the columns that do
           not change a timing: iters, kat, katn
  patch    non-NGCC: files under the scheme dir's patches/; NGCC: the patches/<id>/
           files the row's srcs name, and the headers in patches/ dirs its incs name
  build    non-NGCC: every other tracked file of the scheme dir (Makefile, shim
           templates, build_libs.sh, helpers), except *params*.tsv, status.tsv,
           .gitignore and .stamp; NGCC: shim_template.c, the suite's ngcc Makefile and
           the NGCC_CFLAGS definition in bench-common/ngcc/ngcc.mk (+ NGCC_CXXFLAGS
           when the row's srcs name a C++ file)
  harness  bench-common/harness_common.h up to its "per-operation limit" section (the
           cycle counter, stats and BENCH_TIME_OP), the CC/CFLAGS lines of the suite
           Makefile, bench-common/flags.mk (the compiler flags of every scheme, NGCC
           included; hashed only where it exists, so older commits keep their
           fingerprints) and HARNESS_EPOCH below

Deliberately left out: isolate.h, main.c, loader.c, the time limits and DNF plumbing,
<suite>/harness.h (its timed loop only calls BENCH_TIME_OP once per operation),
gen_rules.py (it turns the params row into make rules; the row and the flags are
hashed instead) and comments: C-family sources are hashed without comments, Python as
its AST, other text files without full-line `#` comments, all with whitespace runs
collapsed, so editing a
PATCH header or a Makefile comment does not re-stale rows. If a change to an excluded
file does change what is timed (e.g. a different timed loop in harness.h, or a
gen_rules.py change to the rules of existing rows), bump HARNESS_EPOCH (all rows) or
edit the affected rows.

Nothing here reads or runs candidate code: NGCC rows are fingerprinted from our own
files and the archives' recorded checksums.

Inputs are read from a git commit (`git ls-tree` / `git cat-file`, so a fingerprint
can be computed for the commit a run built) or from the working tree (tracked files
only, as `git ls-files` lists them; a plain directory walk outside git, for tests).

Provenance: run_bench.sh writes `# commit:` and aws-bench.sh `# instance_type:` into
each results header. Files from before that have neither; for those (the backfill of
fingerprints.tsv, 2026-10-15) the commit is the newest ancestor of HEAD committed
before the run's `# date:` (this matches the commits the aws-bench logs of 2026-10-11..13
recorded, c03458e and 35a714b; for the yuanyang-512/1024 rerun, which started after a
docs-only commit, fingerprints.tsv has the logged 8debfd3), and the instance type is c7i.4xlarge for the Xeon Platinum
8488C (the only EC2 type used before c8i became the default) and `local` otherwise
(a laptop run).

fingerprints.tsv (<suite>/results/, tracked) has one line per row with the
fingerprint of the commit its last imported run built, the result (ok or dnf:<reason>),
commit, instance type, CPU, date and results file. The importers write it.
"""

from __future__ import annotations

import ast
import hashlib
import re
import subprocess
from dataclasses import dataclass, field
from pathlib import Path

SUITE_DIRS = {"sigs": "bench", "kems": "bench-kem"}
COMPONENTS = ("source", "params", "patch", "build", "harness")
REASON = {
    "source": "source",
    "params": "params row",
    "patch": "patch",
    "build": "build",
    "harness": "harness",
}
# Bump when a change outside the hashed files changes what every row times.
HARNESS_EPOCH = 1
UNTIMED_COLS = {"iters", "kat", "katn"}
EXCLUDED_FILES = {".gitignore", ".stamp", "status.tsv", ".source"}
C_SUFFIXES = {".c", ".h", ".cpp", ".cc", ".cxx", ".hpp", ".hh", ".inc"}
CXX_SUFFIXES = (".cpp", ".cc", ".cxx")
HEADER_SUFFIXES = {".h", ".hpp", ".hh", ".inc"}
ARCHIVE_SUFFIXES = (".zip", ".tar.gz", ".tgz", ".tar.xz")
LIMIT_MARKER = "per-operation limit"
FLAGS_MK = "bench-common/flags.mk"
FLAG_VARS = re.compile(r"^\s*(CC|CXX|CFLAGS|CXXFLAGS|LDFLAGS|LDLIBS)\s*[:?+]?=")
LEGACY_EC2_CPUS = {"Intel(R) Xeon(R) Platinum 8488C": "c7i.4xlarge"}
LOCAL = "local"
TSV_COLS = (
    "name",
    "fingerprint",
    *COMPONENTS,
    "result",
    "commit",
    "instance_type",
    "cpu",
    "date",
    "results",
)


# --- reading a tree -------------------------------------------------------------


def _git(repo: Path, *args: str, input: bytes | None = None) -> bytes:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        input=input,
        capture_output=True,
        check=True,
    ).stdout


def is_git(repo: Path) -> bool:
    return subprocess.run(
        ["git", "-C", str(repo), "rev-parse", "--show-toplevel"],
        capture_output=True,
        text=True,
    ).stdout.strip() == str(repo.resolve())


def tree_paths(suite_dir: str) -> list[str]:
    return [
        f"{suite_dir}/schemes",
        f"{suite_dir}/Makefile",
        "bench-common/harness_common.h",
        FLAGS_MK,
        "bench-common/ngcc/ngcc.mk",
        "bench-common/ngcc/archives.tsv",
    ]


class Tree:
    """Tracked files under the paths fingerprints read, at a commit or in the working
    tree. entries: path -> ("blob" | "commit", object id or None)."""

    def __init__(self, repo: Path, rev: str | None, paths: list[str]):
        self.repo, self.rev = repo, rev
        self.entries: dict[str, tuple[str, str | None]] = {}
        self._blobs: dict[str, bytes] | None = None
        if rev is not None:
            out = _git(repo, "ls-tree", "-r", "-z", rev, "--", *paths)
            for rec in filter(None, out.decode().split("\0")):
                meta, path = rec.split("\t", 1)
                _mode, kind, sha = meta.split()
                self.entries[path] = (kind, sha)
        elif is_git(repo):
            out = _git(repo, "ls-files", "-s", "-z", "--", *paths)
            for rec in filter(None, out.decode().split("\0")):
                meta, path = rec.split("\t", 1)
                mode, sha, _stage = meta.split()
                gitlink = mode == "160000"
                if gitlink or (repo / path).is_file():
                    self.entries[path] = ("commit", sha) if gitlink else ("blob", None)
        else:
            for p in paths:
                root = repo / p
                files = [root] if root.is_file() else sorted(root.rglob("*"))
                for f in files:
                    if f.is_file() and "build" not in f.relative_to(repo).parts:
                        self.entries[f.relative_to(repo).as_posix()] = ("blob", None)

    def _load(self) -> dict[str, bytes]:
        if self._blobs is None:
            want = {p: s for p, (k, s) in self.entries.items() if k == "blob" and s}
            self._blobs = {}
            if want:
                order = list(want)
                req = "".join(want[p] + "\n" for p in order).encode()
                out = _git(self.repo, "cat-file", "--batch", input=req)
                pos = 0
                for p in order:
                    nl = out.index(b"\n", pos)
                    size = int(out[pos:nl].split()[2])
                    self._blobs[p] = out[nl + 1 : nl + 1 + size]
                    pos = nl + 1 + size + 1
        return self._blobs

    def read_bytes(self, path: str) -> bytes | None:
        e = self.entries.get(path)
        if e is None or e[0] != "blob":
            return None
        return self._load()[path] if self.rev else (self.repo / path).read_bytes()

    def read(self, path: str) -> str | None:
        data = self.read_bytes(path)
        return None if data is None else data.decode("utf-8", errors="replace")

    def under(self, prefix: str) -> list[str]:
        prefix = prefix.rstrip("/") + "/"
        return sorted(p for p in self.entries if p.startswith(prefix))


# --- normalisation ------------------------------------------------------------------

_C_TOKENS = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S
)


def _collapse(text: str) -> str:
    lines = (re.sub(r"[ \t]+", " ", ln).strip() for ln in text.splitlines())
    return "\n".join(ln for ln in lines if ln)


def normalize(path: str, text: str) -> str:
    """Text as hashed: comments and whitespace differences removed (see module doc)."""
    text = text.replace("\ufeff", "").replace("\r\n", "\n")
    if Path(path).suffix == ".py":
        try:
            return ast.dump(ast.parse(text))
        except SyntaxError:
            pass
    if Path(path).suffix in C_SUFFIXES:
        text = _C_TOKENS.sub(lambda m: m[0] if m[0][0] in "\"'" else " ", text)
    elif Path(path).suffix not in (".s", ".S"):
        text = "\n".join(
            ln for ln in text.splitlines() if not ln.lstrip().startswith("#")
        )
    return _collapse(text)


def make_vars(text: str) -> dict[str, str]:
    """name -> value of each make assignment, continuation lines joined."""
    out: dict[str, str] = {}
    joined = re.sub(r"\\\n", " ", text)
    for ln in joined.splitlines():
        m = re.match(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*[:?+]?=\s*(.*)$", ln)
        if m and not ln.lstrip().startswith("#"):
            out.setdefault(m[1], re.sub(r"\s+", " ", m[2]).strip())
    return out


def _h(*parts: str) -> str:
    d = hashlib.sha256()
    for p in parts:
        d.update(p.encode())
        d.update(b"\0")
    return d.hexdigest()[:12]


# --- rows -------------------------------------------------------------------------


@dataclass
class ParamsRow:
    name: str
    scheme_dir: str  # e.g. bench/schemes/ngcc
    params_file: str  # file name within scheme_dir
    fields: dict[str, str] = field(default_factory=dict)

    @property
    def ngcc(self) -> bool:
        return self.scheme_dir.endswith("/ngcc")


def parse_tsv(text: str) -> list[dict[str, str]]:
    lines = [ln for ln in text.splitlines() if ln.strip() and not ln.startswith("#")]
    if not lines:
        return []
    cols = lines[0].split("\t")
    return [
        {c: v.strip() for c, v in zip(cols, ln.split("\t"), strict=False)}
        for ln in lines[1:]
    ]


def params_rows(tree: Tree, suite_dir: str) -> dict[str, ParamsRow]:
    rows: dict[str, ParamsRow] = {}
    for path in tree.under(f"{suite_dir}/schemes"):
        parts = path.split("/")
        if len(parts) != 4 or "params" not in parts[3] or not path.endswith(".tsv"):
            continue
        for r in parse_tsv(tree.read(path) or ""):
            name = r.get("name", "")
            if name:
                rows.setdefault(name, ParamsRow(name, "/".join(parts[:3]), parts[3], r))
    return rows


def _files_hash(tree: Tree, paths: list[str], base: str) -> str:
    parts = []
    for p in paths:
        kind, sha = tree.entries[p]
        rel = p[len(base) + 1 :] if p.startswith(base + "/") else p
        if kind == "commit":
            parts.append(f"{rel}@{sha}")
        elif p.endswith(ARCHIVE_SUFFIXES):
            digest = hashlib.sha256(tree.read_bytes(p) or b"").hexdigest()
            parts.append(f"{rel}#{digest}")
        else:
            parts.append(f"{rel}:{normalize(p, tree.read(p) or '')}")
    return _h(*parts)


def harness_hash(tree: Tree, suite_dir: str) -> str:
    common = tree.read("bench-common/harness_common.h") or ""
    cut = common.find(LIMIT_MARKER)
    if cut >= 0:
        common = common[: common.rfind("\n", 0, cut) + 1]
    mk = tree.read(f"{suite_dir}/Makefile") or ""
    flags = [
        ln
        for ln in re.sub(r"\\\n", " ", mk).splitlines()
        if FLAG_VARS.match(ln) and not ln.lstrip().startswith("#")
    ]
    parts = [
        f"epoch={HARNESS_EPOCH}",
        normalize("harness_common.h", common),
        _collapse("\n".join(flags)),
    ]
    global_flags = tree.read(FLAGS_MK)
    if global_flags is not None:
        parts.append(normalize(FLAGS_MK, global_flags))
    return _h(*parts)


def _row_params(row: ParamsRow) -> str:
    kept = [f"{c}={v}" for c, v in row.fields.items() if c not in UNTIMED_COLS]
    return _h(row.scheme_dir, row.params_file, *kept)


def _scheme_files(
    tree: Tree, scheme_dir: str
) -> tuple[list[str], list[str], list[str]]:
    """(source, patch, build) paths of a non-NGCC scheme dir."""
    source, patch, build = [], [], []
    for p in tree.under(scheme_dir):
        rel = p[len(scheme_dir) + 1 :]
        name = rel.rsplit("/", 1)[-1]
        if (
            tree.entries[p][0] == "commit"
            or rel == ".source"
            or rel.endswith(ARCHIVE_SUFFIXES)
        ):
            source.append(p)
        elif rel.startswith("patches/"):
            patch.append(p)
        elif name in EXCLUDED_FILES or ("params" in name and name.endswith(".tsv")):
            continue
        else:
            build.append(p)
    return source, patch, build


def _archives(tree: Tree) -> dict[str, str]:
    return {
        r["id"]: r.get("sha256", "")
        for r in parse_tsv(tree.read("bench-common/ngcc/archives.tsv") or "")
        if r.get("id")
    }


def _ngcc_patches(tree: Tree, row: ParamsRow) -> list[str]:
    out: set[str] = set()
    for col in ("srcs", "incs"):
        for ref in row.fields.get(col, "").split():
            if not ref.startswith("patches/"):
                continue
            p = f"{row.scheme_dir}/{ref.rstrip('/')}"
            if p in tree.entries:
                out.add(p)
            elif col == "incs":  # an include dir: the headers in it
                out.update(
                    x for x in tree.under(p) if Path(x).suffix in HEADER_SUFFIXES
                )
    return sorted(out)


def fingerprints(tree: Tree, suite_dir: str) -> dict[str, dict[str, str]]:
    """name -> {component: hash, ..., "fingerprint": hash of all components}."""
    harness = harness_hash(tree, suite_dir)
    rows = params_rows(tree, suite_dir)
    per_dir: dict[str, tuple[str, str, str]] = {}
    archives: dict[str, str] | None = None
    ngcc_mk = make_vars(tree.read("bench-common/ngcc/ngcc.mk") or "")
    out: dict[str, dict[str, str]] = {}
    for name, row in rows.items():
        d = row.scheme_dir
        if row.ngcc:
            if archives is None:
                archives = _archives(tree)
            cand = row.fields.get("id", "")
            source = _h("archive", cand, archives.get(cand, ""))
            patch = _files_hash(tree, _ngcc_patches(tree, row), d)
            flags = [f"NGCC_CFLAGS={ngcc_mk.get('NGCC_CFLAGS', '')}"]
            if any(
                x.endswith(CXX_SUFFIXES) for x in row.fields.get("srcs", "").split()
            ):
                flags.append(f"NGCC_CXXFLAGS={ngcc_mk.get('NGCC_CXXFLAGS', '')}")
            files = [
                p
                for p in (f"{d}/shim_template.c", f"{d}/Makefile")
                if p in tree.entries
            ]
            build = _h(_files_hash(tree, files, d), *flags)
        else:
            if d not in per_dir:
                src, pat, bld = _scheme_files(tree, d)
                per_dir[d] = (
                    _files_hash(tree, src, d),
                    _files_hash(tree, pat, d),
                    _files_hash(tree, bld, d),
                )
            source, patch, build = per_dir[d]
        comps = {
            "source": source,
            "params": _row_params(row),
            "patch": patch,
            "build": build,
            "harness": harness,
        }
        comps["fingerprint"] = _h(*(f"{c}={comps[c]}" for c in COMPONENTS))
        out[name] = comps
    return out


_cache: dict[tuple[str, str | None, str], dict[str, dict[str, str]]] = {}


def suite_fingerprints(
    repo: Path, suite_dir: str, rev: str | None = None
) -> dict[str, dict[str, str]]:
    """Fingerprints of every row of a suite at `rev` (None: the working tree)."""
    key = (str(repo), rev, suite_dir)
    if key not in _cache:
        _cache[key] = fingerprints(Tree(repo, rev, tree_paths(suite_dir)), suite_dir)
    return _cache[key]


# --- provenance -----------------------------------------------------------------------


def header(text: str) -> dict[str, str]:
    """`# key: value` lines of a results header (first occurrence wins)."""
    meta: dict[str, str] = {}
    for line in text.splitlines():
        if line.startswith(("scheme ", "------")):
            break
        if m := re.match(r"^#\s+([\w-]+):\s*(.*)$", line):
            meta.setdefault(m[1].replace("-", "_"), m[2].strip())
    return meta


def part_files(text: str) -> list[str]:
    """File names of a merged results file's `# part:` lines, in merge order."""
    return re.findall(r"^#\s+part:\s+(\S+)", text, re.M)


DATA_SPLIT = re.compile(r"\s{2,}")


def result_rows(text: str) -> dict[str, list[str]]:
    """name -> fields of each results row (first one per name)."""
    rows: dict[str, list[str]] = {}
    started = False
    for line in text.splitlines():
        if line.startswith("------"):
            started = True
            continue
        if not started or line.startswith("#") or not line.strip():
            continue
        f = DATA_SPLIT.split(line.strip())
        if len(f) >= 8 and (f[1].isdigit() or f[1] == "-"):
            rows.setdefault(f[0], f)
    return rows


def commit_before(repo: Path, date: str) -> str | None:
    """Newest ancestor of HEAD committed before `date` (legacy results files)."""
    if not date:
        return None
    try:
        out = _git(repo, "rev-list", "-1", f"--before={date}", "HEAD").decode().strip()
    except subprocess.CalledProcessError:
        return None
    return out or None


def legacy_instance_type(cpu: str) -> str:
    return LEGACY_EC2_CPUS.get(cpu, LOCAL)


@dataclass
class Provenance:
    commit: str | None
    instance_type: str
    cpu: str
    date: str
    file: str  # the file the row was timed in (a part, for merged files)


def provenance(repo: Path, meta: dict[str, str], file: str) -> Provenance:
    # "-dirty": built with uncommitted changes (a local run); the commit is the best guess.
    commit = (meta.get("commit") or "").removesuffix("-dirty")
    if not re.fullmatch(r"[0-9a-f]{7,40}", commit):
        commit = commit_before(repo, meta.get("date", ""))
    cpu = meta.get("cpu", "")
    itype = meta.get("instance_type") or legacy_instance_type(cpu)
    return Provenance(commit, itype, cpu, meta.get("date", ""), file)


def row_provenance(repo: Path, results_file: Path) -> dict[str, Provenance]:
    """Row name -> where it was timed. For a merged file, each row comes from the first
    part (in merge order) with the same row, or else the first part that has it; rows
    of missing parts fall back to the merged file's own header."""
    text = results_file.read_text(errors="replace")
    own = provenance(repo, header(text), results_file.name)
    rows = result_rows(text)
    out = {name: own for name in rows}
    parts = []
    for p in part_files(text):
        path = results_file.parent / p
        if path.is_file():
            t = path.read_text(errors="replace")
            parts.append((provenance(repo, header(t), p), result_rows(t)))
    for name, fields in rows.items():
        same = [pv for pv, r in parts if r.get(name) == fields]
        any_ = [pv for pv, r in parts if name in r]
        if same or any_:
            out[name] = (same or any_)[0]
    return out


# --- fingerprints.tsv -------------------------------------------------------------------


def fingerprints_path(repo: Path, suite_dir: str) -> Path:
    return repo / suite_dir / "results" / "fingerprints.tsv"


TSV_HEADER = """\
# Per-row benchmark provenance: the input fingerprint (bench-common/fingerprint.py) of
# the commit each row's last imported run built, its result, and where it ran. Written
# by update_scheme_data.py; scripts/aws-bench-split plans rows whose current
# fingerprint or hardware differs. Rows before 2026-10-15: backfilled, see the module doc.
"""


def read_fingerprints(path: Path) -> dict[str, dict[str, str]]:
    if not path.exists():
        return {}
    return {r["name"]: r for r in parse_tsv(path.read_text())}


def write_fingerprints(path: Path, entries: dict[str, dict[str, str]]) -> None:
    lines = [TSV_HEADER + "\t".join(TSV_COLS)]
    for name in sorted(entries):
        e = entries[name]
        lines.append(
            "\t".join(str(e.get(c, "") or "-").replace("\t", " ") for c in TSV_COLS)
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n")


def entry(
    name: str, fp: dict[str, str] | None, result: str, pv: Provenance, results: str
) -> dict[str, str]:
    e = {c: (fp or {}).get(c, "-") for c in ("fingerprint", *COMPONENTS)}
    e.update(
        name=name,
        result=result,
        commit=(pv.commit or "-")[:12],
        instance_type=pv.instance_type,
        cpu=pv.cpu,
        date=pv.date,
        results=results,
    )
    return e


def record(
    repo: Path,
    suite_dir: str,
    results_file: Path,
    outcomes: dict[str, str],
    path: Path | None = None,
) -> list[str]:
    """Write fingerprints.tsv lines for the rows of results_file in `outcomes`
    (name -> "ok" or "dnf:<reason>"); returns warnings."""
    path = path or fingerprints_path(repo, suite_dir)
    entries = read_fingerprints(path)
    prov = row_provenance(repo, results_file)
    warnings = []
    for name, result in outcomes.items():
        pv = prov.get(name)
        if pv is None:
            continue
        fp = None
        if pv.commit:
            try:
                fp = suite_fingerprints(repo, suite_dir, pv.commit).get(name)
            except subprocess.CalledProcessError:
                warnings.append(f"{name}: commit {pv.commit} not in this repository")
        if fp is None:
            warnings.append(
                f"{name}: no fingerprint (not a params.tsv row at {pv.commit or 'unknown commit'}); it stays stale"
            )
        entries[name] = entry(name, fp, result, pv, results_file.name)
    write_fingerprints(path, entries)
    return warnings


# --- staleness ------------------------------------------------------------------------


def stale_reasons(
    names: list[str],
    current: dict[str, dict[str, str]],
    recorded: dict[str, dict[str, str]],
    instance_type: str,
    in_results: set[str] = frozenset(),
) -> dict[str, list[str]]:
    """name -> why it needs a run (empty list: up to date).

    "new row": never imported and in no results file; "no timing": in a results file
    but never imported (or only as a DNF the importer did not record); "hardware: a → b":
    last run on another instance type; otherwise each changed input component.
    """
    out: dict[str, list[str]] = {}
    for n in names:
        rec = recorded.get(n)
        if rec is None:
            out[n] = ["no timing" if n in in_results else "new row"]
            continue
        why = []
        if rec.get("instance_type") != instance_type:
            why.append(f"hardware: {rec.get('instance_type')} → {instance_type}")
        cur = current.get(n, {})
        if rec.get("fingerprint") != cur.get("fingerprint"):
            changed = [REASON[c] for c in COMPONENTS if rec.get(c) != cur.get(c)]
            why += changed or ["fingerprint"]
        out[n] = why
    return out


def main(argv: list[str] | None = None) -> int:
    """Print the current fingerprints of a suite: fingerprint.py sigs|kems [rev]."""
    import sys

    args = sys.argv[1:] if argv is None else argv
    if not args or args[0] not in SUITE_DIRS:
        print("usage: fingerprint.py sigs|kems [rev]")
        return 2
    repo = Path(__file__).resolve().parents[1]
    fps = suite_fingerprints(
        repo, SUITE_DIRS[args[0]], args[1] if len(args) > 1 else None
    )
    for name in sorted(fps):
        f = fps[name]
        print("\t".join([name, f["fingerprint"], *(f[c] for c in COMPONENTS)]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
