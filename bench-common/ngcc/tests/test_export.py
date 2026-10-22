import importlib.util
import io
import os
import tarfile
from pathlib import Path

spec = importlib.util.spec_from_file_location(
    "ngcc_export", Path(__file__).parents[3] / "scripts/ngcc-container/export.py"
)
exp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exp)

OK = "bench/schemes/ngcc/status.tsv"


def make_tar(members):
    """members: (name, kind, payload) with kind file|symlink|hardlink."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w") as tf:
        for name, kind, payload in members:
            ti = tarfile.TarInfo(name)
            if kind == "file":
                ti.size = len(payload)
                ti.mode = 0o755
                tf.addfile(ti, io.BytesIO(payload))
            else:
                ti.type = tarfile.SYMTYPE if kind == "symlink" else tarfile.LNKTYPE
                ti.linkname = payload
                tf.addfile(ti)
    buf.seek(0)
    return buf


def run(tmp_path, members, repo=None):
    log = io.StringIO()
    w, r = exp.export_tar(make_tar(members), str(tmp_path / "out"), repo, log)
    return w, r


def test_allowlisted_text_ok_mode_0644(tmp_path):
    w, r = run(
        tmp_path,
        [
            ("work/" + OK, "file", b"a\tb\n"),
            ("work/bench/results/x.txt", "file", b"hi"),
        ],
    )
    assert sorted(w) == sorted([OK, "bench/results/x.txt"]) and not r
    p = tmp_path / "out" / OK
    assert p.read_bytes() == b"a\tb\n"
    assert (p.stat().st_mode & 0o777) == 0o644


def test_tmp_stage_maps_to_tmp(tmp_path):
    w, _ = run(
        tmp_path,
        [("work/.ngcc-tmp/s.tsv", "file", b"x"), ("work/.ngcc-tmp/s.so", "file", b"x")],
    )
    assert w == ["tmp/s.tsv"]


def test_rejects_symlink_and_hardlink(tmp_path):
    w, r = run(
        tmp_path,
        [
            ("work/bench/results/l.txt", "symlink", "/repo/x"),
            ("work/bench/results/h.txt", "hardlink", "work/a"),
        ],
    )
    assert not w and len(r) == 2
    assert not (tmp_path / "out/bench/results/l.txt").exists()


def test_rejects_dotdot_and_absolute(tmp_path):
    w, r = run(
        tmp_path, [("work/../x.log", "file", b"x"), ("/etc/x.log", "file", b"x")]
    )
    assert not w and len(r) == 2
    assert not (tmp_path / "x.log").exists()


def test_rejects_elf_named_log(tmp_path):
    w, r = run(tmp_path, [("work/build/a.log", "file", b"\x7fELF\x02\x01\x00\x00")])
    assert not w and r[0][1] == "contains NUL bytes"
    w, r = run(tmp_path, [("work/build/b.log", "file", b"\x7fELF\xff\xfe")])
    assert not w and r[0][1] == "not valid UTF-8"


def test_rejects_oversize(tmp_path, monkeypatch):
    monkeypatch.setattr(exp, "MAX_FILE", 10)
    w, r = run(tmp_path, [("work/build/a.log", "file", b"x" * 11)])
    assert not w and r[0][1] == "too large"


def test_non_allowlisted_ignored(tmp_path):
    w, r = run(
        tmp_path,
        [
            ("work/bench/x.so", "file", b"text"),
            ("work/bench-common/ngcc/src/a.log", "file", b"x"),
        ],
    )
    assert not w and not r


def test_does_not_follow_existing_symlink(tmp_path):
    out = tmp_path / "out/bench/results"
    out.mkdir(parents=True)
    victim = tmp_path / "victim"
    victim.write_text("keep")
    (out / "x.txt").symlink_to(victim)
    run(tmp_path, [("work/bench/results/x.txt", "file", b"new")])
    assert victim.read_text() == "keep"
    assert not (out / "x.txt").is_symlink()


def test_skips_identical_to_repo(tmp_path):
    repo = tmp_path / "repo"
    (repo / "bench/results").mkdir(parents=True)
    (repo / "bench/results/x.txt").write_text("same")
    w, _ = run(
        tmp_path,
        [
            ("work/bench/results/x.txt", "file", b"same"),
            ("work/bench/results/y.txt", "file", b"n"),
        ],
        str(repo),
    )
    assert w == ["bench/results/y.txt"]
    assert os.path.exists(tmp_path / "out/bench/results/y.txt")


def test_unpacked_text_maps_to_unpacked(tmp_path):
    w, _ = run(
        tmp_path,
        [
            ("work/.ngcc-unpacked/sign-09/I/r/a.c", "file", b"int a;"),
            ("work/.ngcc-unpacked/sign-09/I/r/Makefile", "file", b"all:"),
            ("work/.ngcc-unpacked/sign-09/I/r/README", "file", b"hi"),
            ("work/.ngcc-unpacked/sign-09/I/r/a.o", "file", b"x"),
            ("work/.ngcc-unpacked/sign-09/I/r/b.c", "file", b"\x7fELF\0"),
        ],
    )
    assert sorted(w) == [
        "unpacked/sign-09/I/r/Makefile",
        "unpacked/sign-09/I/r/README",
        "unpacked/sign-09/I/r/a.c",
    ]
    assert (tmp_path / "out/unpacked/sign-09/I/r/a.c").stat().st_mode & 0o777 == 0o644
