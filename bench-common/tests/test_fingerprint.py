"""bench-common/fingerprint.py: per-row input fingerprints, provenance, staleness."""

import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import fingerprint as F  # noqa: E402

HARNESS = """\
/* cycle counter */
static int counter(void) { return 1; }
#define BENCH_TIME_OP(c) do { c; } while (0)

/* ---------- per-operation limit ---------- */
static unsigned limit(void) { return 600; }
"""


def git(repo, *args):
    return subprocess.run(
        ["git", "-C", str(repo), *args], capture_output=True, text=True, check=True
    ).stdout.strip()


def write(repo, path, text):
    p = repo / path
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)


def commit(repo, msg="c", date="2026-10-13T10:00:00Z", add=True):
    if add:
        git(repo, "add", "-A")
    subprocess.run(
        ["git", "-C", str(repo), "commit", "-qm", msg],
        check=True,
        capture_output=True,
        env={
            "GIT_AUTHOR_DATE": date,
            "GIT_COMMITTER_DATE": date,
            "GIT_AUTHOR_NAME": "t",
            "GIT_AUTHOR_EMAIL": "t@t",
            "GIT_COMMITTER_NAME": "t",
            "GIT_COMMITTER_EMAIL": "t@t",
            "PATH": "/usr/bin:/bin",
        },
    )
    return git(repo, "rev-parse", "HEAD")


@pytest.fixture
def repo(tmp_path):
    r = tmp_path / "repo"
    r.mkdir()
    git(r, "init", "-q")
    write(r, "bench-common/harness_common.h", HARNESS)
    write(r, "bench/Makefile", "CC ?= cc\nCFLAGS := -O2\nALL_SOS = a\n")
    write(r, "bench/schemes/a/Makefile", "# build a\nall:\n\tcc -O3 a.c\n")
    write(r, "bench/schemes/a/shim_template.c", "/* shim */\nint f(void){return 1;}\n")
    write(
        r,
        "bench/schemes/a/params.tsv",
        "# rows\nfile\tname\titers\tmode\na1\tA-1\t10\tx\na2\tA-2\t10\ty\n",
    )
    write(r, "bench/schemes/a/.gitignore", "*_shim.c\n")
    write(r, "bench/schemes/b/Makefile", "all:\n")
    write(r, "bench/schemes/b/params.tsv", "file\tname\titers\nb1\tB-1\t0\n")
    write(
        r,
        "bench-common/ngcc/ngcc.mk",
        "NGCC_CFLAGS ?= -O3 \\\n  -fPIC\nNGCC_CXXFLAGS ?= -O3\n",
    )
    write(
        r,
        "bench-common/ngcc/archives.tsv",
        "# pinned\nid\tkind\tsha256\nsign-01\tsig\taaa\nsign-02\tsig\tbbb\n",
    )
    write(r, "bench/schemes/ngcc/Makefile", "include ngcc.mk\n")
    write(r, "bench/schemes/ngcc/shim_template.c", "int shim;\n")
    write(r, "bench/schemes/ngcc/status.tsv", "name\tbuild\nNGCC-X\tok\n")
    write(
        r,
        "bench/schemes/ngcc/patches/sign-01/fix.c",
        "/* PATCH (nist-sigs-zoo): x */\nint fix;\n",
    )
    write(r, "bench/schemes/ngcc/patches/sign-01/other.c", "int other;\n")
    write(r, "bench/schemes/ngcc/patches/sign-02/inc/h.h", "int h;\n")
    write(
        r,
        "bench/schemes/ngcc/params.tsv",
        "file\tname\tid\tsrcs\tincs\titers\tkat\n"
        "x\tNGCC-X\tsign-01\t* patches/sign-01/fix.c\t\t1\tk\n"
        "y\tNGCC-Y\tsign-02\t*\tpatches/sign-02/inc\t1\tk\n"
        "z\tNGCC-Z\tsign-02\t* a.cpp\t\t1\tk\n",
    )
    commit(r, "init", "2026-10-13T08:00:00Z")
    return r


def fps(repo, rev=None):
    F._cache.clear()
    return F.suite_fingerprints(repo, "bench", rev)


def changed(before, after):
    return {
        n: [c for c in F.COMPONENTS if before[n][c] != after[n][c]]
        for n in after
        if n in before and before[n] != after[n]
    }


def test_commit_and_working_tree_agree(repo):
    sha = git(repo, "rev-parse", "HEAD")
    at_commit, wt = fps(repo, sha), fps(repo)
    assert at_commit == wt
    assert set(wt) == {"A-1", "A-2", "B-1", "NGCC-X", "NGCC-Y", "NGCC-Z"}
    assert len({wt["A-1"]["fingerprint"], wt["A-2"]["fingerprint"]}) == 2
    assert wt["A-1"]["build"] == wt["A-2"]["build"] != wt["B-1"]["build"]


def test_comments_whitespace_and_untimed_columns_do_not_count(repo):
    before = fps(repo)
    write(repo, "bench/schemes/a/Makefile", "# new comment\nall:\n\tcc   -O3 a.c\n")
    write(repo, "bench/schemes/a/shim_template.c", "int f(void){return 1;} // x\n")
    write(
        repo,
        "bench/schemes/ngcc/patches/sign-01/fix.c",
        "﻿/* PATCH (nist-sigs-zoo, fix): reworded */\nint fix;\n",
    )
    write(
        repo,
        "bench/schemes/a/params.tsv",
        "# other comment\nfile\tname\titers\tmode\na1\tA-1\t99\tx\na2\tA-2\t10\ty\n",
    )
    write(repo, "bench/schemes/ngcc/status.tsv", "name\tbuild\nNGCC-X\tFAIL\n")
    write(repo, "bench-common/harness_common.h", HARNESS + "static int more_limits;\n")
    write(repo, "bench/Makefile", "CC ?= cc\nCFLAGS := -O2\nALL_SOS = a b c\n")
    assert fps(repo) == before


def test_each_input_changes_its_component(repo):
    before = fps(repo)
    write(repo, "bench/schemes/a/Makefile", "all:\n\tcc -O2 a.c\n")
    write(repo, "bench/schemes/ngcc/patches/sign-01/fix.c", "int fix2;\n")
    write(repo, "bench/schemes/ngcc/patches/sign-01/other.c", "int unused_by_rows;\n")
    write(repo, "bench/schemes/ngcc/patches/sign-02/inc/h.h", "int h2;\n")
    write(
        repo,
        "bench-common/ngcc/archives.tsv",
        "id\tkind\tsha256\nsign-01\tsig\taaa\nsign-02\tsig\tccc\n",
    )
    assert changed(before, fps(repo)) == {
        "A-1": ["build"],
        "A-2": ["build"],
        "NGCC-X": ["patch"],
        "NGCC-Y": ["source", "patch"],
        "NGCC-Z": ["source"],
    }


def test_harness_and_flags(repo):
    before = fps(repo)
    write(
        repo, "bench-common/harness_common.h", HARNESS.replace("return 1", "return 2")
    )
    after = fps(repo)
    assert all(changed(before, after)[n] == ["harness"] for n in after)
    write(repo, "bench-common/harness_common.h", HARNESS)
    write(
        repo,
        "bench-common/ngcc/ngcc.mk",
        "NGCC_CFLAGS ?= -O3 -fPIC\nNGCC_CXXFLAGS ?= -O2\n",
    )
    assert changed(before, fps(repo)) == {"NGCC-Z": ["build"]}  # the C++ row only


def test_gitlink_counts_as_source(repo):
    sha = git(repo, "rev-parse", "HEAD")
    git(
        repo,
        "update-index",
        "--add",
        "--cacheinfo",
        f"160000,{sha},bench/schemes/a/ref",
    )
    c2 = commit(repo, "sub", add=False)
    assert changed(fps(repo, sha), fps(repo, c2)) == {
        "A-1": ["source"],
        "A-2": ["source"],
    }


def test_outside_git_walks_the_tree(tmp_path, repo):
    in_git = fps(repo)
    plain = tmp_path / "plain"
    subprocess.run(["cp", "-r", str(repo), str(plain)], check=True)
    subprocess.run(["rm", "-rf", str(plain / ".git")], check=True)
    assert fps(plain) == in_git


# --- provenance ----------------------------------------------------------------------

COLUMNS = "scheme  iters  kg  kgus  sg  sgus  vf  vfus\n------  -----  --  ----  --  ----  --  ----\n"


def results(rows, **head):
    h = "# pq-sigs benchmark results\n" + "".join(
        f"# {k}:  {v}\n" for k, v in head.items()
    )
    return (
        h
        + "#\n"
        + COLUMNS
        + "".join(f"{n:<10}  10  {c}  1.0  {c}  2.0  {c}  3.0\n" for n, c in rows)
    )


def test_header_and_legacy_provenance(repo):
    (repo / "x").write_text("1")
    c1 = commit(repo, "one", "2026-10-13T09:00:00Z")
    (repo / "x").write_text("2")
    c2 = commit(repo, "two", "2026-10-13T12:00:00Z")
    meta = F.header(
        results([], date="2026-10-13T10:00:00Z", cpu="Intel(R) Xeon(R) Platinum 8488C")
    )
    pv = F.provenance(repo, meta, "f.txt")
    assert (pv.commit, pv.instance_type) == (c1, "c7i.4xlarge")
    meta = F.header(
        results(
            [],
            date="2026-10-13T10:00:00Z",
            cpu="Other",
            commit=c2,
            **{"instance_type": "c8i.4xlarge"},
        )
    )
    pv = F.provenance(repo, meta, "f.txt")
    assert (pv.commit, pv.instance_type) == (c2, "c8i.4xlarge")
    assert F.provenance(repo, {"cpu": "Laptop"}, "f").instance_type == F.LOCAL


def test_merged_rows_come_from_their_part(repo, tmp_path):
    rd = tmp_path / "results"
    rd.mkdir()
    (rd / "p1.txt").write_text(
        results([("A-1", 5), ("A-2", 6)], date="d1", commit="c1c1c1c")
    )
    (rd / "p2.txt").write_text(results([("A-2", 7)], date="d2", commit="c2c2c2c"))
    merged = results([("A-1", 5), ("A-2", 7), ("B-1", 8)], date="d1", commit="c1c1c1c")
    merged = merged.replace(
        "#\nscheme", "# part:  p1.txt (host h)\n# part:  p2.txt (host h)\n#\nscheme"
    )
    (rd / "m.txt").write_text(merged)
    prov = F.row_provenance(repo, rd / "m.txt")
    assert {n: (p.file, p.commit) for n, p in prov.items()} == {
        "A-1": ("p1.txt", "c1c1c1c"),
        "A-2": ("p2.txt", "c2c2c2c"),  # same row as p2, not p1's other value
        "B-1": ("m.txt", "c1c1c1c"),  # in no part: the merged header
    }


def test_record_and_staleness(repo, tmp_path):
    c1 = git(repo, "rev-parse", "HEAD")
    res = repo / "bench" / "results" / "r.txt"
    res.parent.mkdir(parents=True)
    res.write_text(
        results(
            [("A-1", 5), ("A-2", 6)],
            date="2026-10-13T10:00:00Z",
            cpu="CPU",
            commit=c1,
            instance_type="c8i.4xlarge",
        )
    )
    F._cache.clear()
    warn = F.record(
        repo, "bench", res, {"A-1": "ok", "A-2": "dnf:timeout", "Gone": "ok"}
    )
    assert warn == []
    path = F.fingerprints_path(repo, "bench")
    rec = F.read_fingerprints(path)
    assert set(rec) == {"A-1", "A-2"}
    assert rec["A-1"]["commit"] == c1[:12] and rec["A-2"]["result"] == "dnf:timeout"
    assert path.read_text().startswith("# Per-row benchmark provenance")

    write(
        repo,
        "bench/schemes/a/params.tsv",
        "file\tname\titers\tmode\na1\tA-1\t10\tz\na2\tA-2\t10\ty\n",
    )
    names = ["A-1", "A-2", "B-1", "NGCC-X"]
    why = F.stale_reasons(names, fps(repo), rec, "c8i.4xlarge", {"NGCC-X"})
    assert why == {
        "A-1": ["params row"],
        "A-2": [],
        "B-1": ["new row"],
        "NGCC-X": ["no timing"],
    }
    why = F.stale_reasons(["A-2"], fps(repo), rec, "c7i.4xlarge")
    assert why == {"A-2": ["hardware: c8i.4xlarge → c7i.4xlarge"]}
