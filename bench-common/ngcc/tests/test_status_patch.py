from pathlib import Path

import status


def _row(srcs: str) -> dict:
    return {
        "file": "x",
        "id": "sign-99",
        "srcdir": "inst",
        "srcs": srcs,
        "incs": "",
    }


def _tree(tmp_path: Path) -> tuple[Path, Path]:
    src, suite = tmp_path / "src", tmp_path / "suite"
    (src / "sign-99" / "inst").mkdir(parents=True)
    (src / "sign-99" / "inst" / "a.c").write_text("int a;\n")
    (src / "sign-99" / "inst" / "drng.c").write_text("int d;\n")
    p = suite / "patches" / "sign-99"
    p.mkdir(parents=True)
    (p / "fix.c").write_text("/* PATCH (nist-sigs-zoo, fix): x. */\nint f;\n")
    (p / "wrap.c").write_text(
        "/* PATCH (nist-sigs-zoo): hidden visibility for the asm tables. */\nint w;\n"
    )
    return src, suite


def test_patch_column_marks_rows_with_a_defect_workaround(tmp_path):
    src, suite = _tree(tmp_path)
    assert status.patch_status(_row("* patches/sign-99/fix.c"), src, suite) == "fix"


def test_build_only_patches_and_plain_rows_are_not_marked(tmp_path):
    src, suite = _tree(tmp_path)
    assert status.patch_status(_row("* patches/sign-99/wrap.c"), src, suite) == ""
    assert status.patch_status(_row("*"), src, suite) == ""


def test_patch_is_a_status_column():
    assert status.COLUMNS[-1] == "patch"
