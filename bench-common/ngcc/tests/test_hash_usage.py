from hash_usage import hash_usage


def test_placeholder_calls(tmp_path):
    (tmp_path / "a.c").write_text("void f(void) { pseudoXOF(256, m, 8, out); }\n")
    assert hash_usage([tmp_path / "a.c"]) == "placeholder"


def test_sm3_only(tmp_path):
    (tmp_path / "a.c").write_text("void f(void) { sm3hash(256, m, 8, out); }\n")
    assert hash_usage([tmp_path / "a.c"]) == "sm3"


def test_comments_auxfunc_and_missing_files_are_ignored(tmp_path):
    (tmp_path / "a.c").write_text(
        "/* pseudohash(x) */\n// pseudoXOF(y)\nint shake(void);\n"
    )
    (tmp_path / "auxfunc.c").write_text("int pseudohash(int a) { return a; }\n")
    assert (
        hash_usage([tmp_path / "a.c", tmp_path / "auxfunc.c", tmp_path / "gone.c"])
        == "own"
    )


def test_cxx_headers_in_srcdir_and_incs_are_scanned(tmp_path):
    from status import scanned_headers

    src = tmp_path / "X"
    (src / "src").mkdir(parents=True)
    (src / "deep").mkdir()
    for f in ("a.h", "b.hpp", "src/c.hpp", "src/d.h", "deep/e.hpp"):
        (src / f).write_text("")
    row = {"incs": "src"}
    names = sorted(
        p.relative_to(src).as_posix() for p in scanned_headers(row, src, tmp_path)
    )
    assert names == ["a.h", "b.hpp", "src/c.hpp"]
    (src / "src/c.hpp").write_text("inline void f() { sm3hash(256, 0, 0, 0); }\n")
    assert hash_usage(scanned_headers(row, src, tmp_path)) == "sm3"
