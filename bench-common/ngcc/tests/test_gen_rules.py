import pytest
from gen_rules import (
    ParamsError,
    read_params,
    render_rules,
    resolve_sources,
    write_shims,
)

HEADER = "file\tname\tid\tinstance\timpl\tsrcdir\tsrcs\tkat\tdefs\n"
ROW = ("x", "NGCC-X", "sign-01", "X", "ref", "Ref/X", "*", "-", "")


def write_params(tmp_path, *rows):
    p = tmp_path / "params.tsv"
    p.write_text("# comment\n" + HEADER + "".join("\t".join(r) + "\n" for r in rows))
    return p


def make_instance(src, files):
    d = src / "sign-01" / "Ref" / "X"
    for f in files:
        (d / f).parent.mkdir(parents=True, exist_ok=True)
        (d / f).write_text("int x;\n")
    return d


def test_star_skips_driver_files(tmp_path):
    make_instance(
        tmp_path / "src",
        [
            "a.c",
            "drng.c",
            "KAT_SIG.c",
            "main.c",
            "test_x.c",
            "speed.c",
            "cpucycles.c",
            "b.S",
            "c.h",
        ],
    )
    row = read_params(write_params(tmp_path, ROW))[0]
    assert [p.name for p in resolve_sources(row, tmp_path / "src", tmp_path)] == [
        "a.c",
        "b.S",
        "drng.c",
    ]


def test_drop_subdir_and_patch(tmp_path):
    d = make_instance(
        tmp_path / "src", ["a.c", "drng.c", "kat_test/SIG_AlgorithmInstance.c"]
    )
    srcs = "* -a.c kat_test/SIG_AlgorithmInstance.c patches/sign-01/a.c"
    row = read_params(write_params(tmp_path, ROW[:6] + (srcs, "-", "")))[0]
    assert resolve_sources(row, tmp_path / "src", tmp_path) == [
        d / "drng.c",
        d / "kat_test/SIG_AlgorithmInstance.c",
        tmp_path / "patches/sign-01/a.c",
    ]


def test_duplicate_labels_rejected(tmp_path):
    with pytest.raises(ParamsError, match="duplicate file"):
        read_params(write_params(tmp_path, ROW, ROW))


def test_name_needs_ngcc_prefix(tmp_path):
    with pytest.raises(ParamsError, match="NGCC-"):
        read_params(write_params(tmp_path, ("x", "X") + ROW[2:]))


def test_defs_reach_the_sources_but_not_the_shim(tmp_path):
    make_instance(tmp_path / "src", ["a.c", "drng.c"])
    rows = read_params(
        write_params(tmp_path, ROW[:8] + ("-Dcrypto_sign_keypair=cand_keypair",))
    )
    text = render_rules(rows, tmp_path / "src", tmp_path)
    assert "NGCC_SOS += $(BUILD)/x.so" in text
    shim_line = next(line for line in text.splitlines() if "-c x_shim.c" in line)
    link_line = next(line for line in text.splitlines() if "-shared" in line)
    assert "crypto_sign_keypair" not in shim_line
    assert "-Dcrypto_sign_keypair=cand_keypair" in link_line
    assert "$(BUILD)/src/x/a.c $(BUILD)/src/x/drng.c" in link_line


def test_missing_sources_become_a_failing_rule(tmp_path):
    rows = read_params(write_params(tmp_path, ROW))
    assert "FAIL  NGCC-X: no sources" in render_rules(rows, tmp_path / "src", tmp_path)


def test_missing_drng_is_reported(tmp_path):
    make_instance(tmp_path / "src", ["a.c"])
    rows = read_params(write_params(tmp_path, ROW))
    assert "no drng source" in render_rules(rows, tmp_path / "src", tmp_path)


def test_shims_get_defaults_for_absent_optional_columns(tmp_path):
    rows = read_params(write_params(tmp_path, ROW))
    (tmp_path / "t.c").write_text('"@NAME@" @ITERS@ @IMPL@ @KATN@\n')
    write_shims(rows, tmp_path / "t.c", tmp_path)
    assert (tmp_path / "x_shim.c").read_text() == '"NGCC-X" 0 ref 10\n'


def rules_for(tmp_path, row, files=("a.c", "drng.c")):
    make_instance(tmp_path / "src", list(files))
    return render_rules(
        read_params(write_params(tmp_path, row)), tmp_path / "src", tmp_path
    )


@pytest.mark.parametrize(
    "name", ["NGCC-X`id`", 'NGCC-X"; rm', "NGCC-X$(y)", "NGCC-", "NGCC-a b"]
)
def test_malicious_name_rejected(tmp_path, name):
    with pytest.raises(ParamsError):
        read_params(write_params(tmp_path, ("x", name) + ROW[2:]))


def test_malicious_instance_rejected(tmp_path):
    with pytest.raises(ParamsError, match="instance"):
        read_params(write_params(tmp_path, ROW[:3] + ("X`id`",) + ROW[4:]))


@pytest.mark.parametrize("srcdir", ["../x", "R/ok/../../..", "/etc"])
def test_traversal_srcdir_rejected(tmp_path, srcdir):
    with pytest.raises(ParamsError, match="srcdir"):
        read_params(write_params(tmp_path, ROW[:5] + (srcdir,) + ROW[6:]))


@pytest.mark.parametrize("srcs", ["* ../evil.c", "/etc/passwd", "patches/../../x.c"])
def test_traversal_srcs_rejected(tmp_path, srcs):
    with pytest.raises(ParamsError, match="srcs"):
        read_params(write_params(tmp_path, ROW[:6] + (srcs, "-", "")))


def test_symlinked_srcdir_escape_is_a_failing_rule(tmp_path):
    (tmp_path / "outside").mkdir()
    (tmp_path / "outside" / "drng.c").write_text("int x;\n")
    (tmp_path / "src" / "sign-01").mkdir(parents=True)
    (tmp_path / "src" / "sign-01" / "Ref").symlink_to(tmp_path / "outside")
    rows = read_params(write_params(tmp_path, ROW[:5] + ("Ref",) + ROW[6:]))
    assert "escapes" in render_rules(rows, tmp_path / "src", tmp_path)


def test_bad_source_file_name_is_a_failing_rule(tmp_path):
    text = rules_for(tmp_path, ROW, files=("a.c", "drng.c", "e$(x).c"))
    assert "FAIL  NGCC-X: source path has characters" in text
    assert "e$(x)" not in text


@pytest.mark.parametrize(
    "col,value",
    [
        ("defs", "-Dx=$(shell id)"),
        ("defs", "-Dx;id"),
        ("defs", "-Dx`id`"),
        ("defs", "-Iother"),
        ("defs", "-Dx=a b|c"),
        ("shimdefs", '-Dx="y"'),
        ("ldlibs", "-lm;id"),
        ("ldlibs", "-L/tmp"),
        ("incs", "../x"),
        ("incs", "/abs"),
    ],
)
def test_injected_flags_rejected(tmp_path, col, value):
    row = dict(zip(HEADER.split(), ROW, strict=False))
    cols = ["file", "name", "id", "instance", "impl", "srcdir", "srcs", "kat", col]
    vals = [row.get(c, "") for c in cols[:-1]] + [value]
    p = tmp_path / "params.tsv"
    p.write_text("\t".join(cols) + "\n" + "\t".join(vals) + "\n")
    with pytest.raises(ParamsError):
        read_params(p)


def test_documented_shimdefs_example_is_accepted(tmp_path):
    cols = [
        "file",
        "name",
        "id",
        "instance",
        "impl",
        "srcdir",
        "srcs",
        "kat",
        "shimdefs",
    ]
    vals = list(ROW[:8]) + [
        "-DNGCC_INSTANCE_HEADER='\"kat_test/SIG_AlgorithmInstance.h\"' -DNGCC_NO_DRNG"
    ]
    p = tmp_path / "params.tsv"
    p.write_text("\t".join(cols) + "\n" + "\t".join(vals) + "\n")
    assert read_params(p)[0]["shimdefs"].startswith("-DNGCC_INSTANCE_HEADER=")


def test_newline_in_a_field_rejected(tmp_path):
    p = tmp_path / "params.tsv"
    p.write_text(HEADER + 'x\tNGCC-X\tsign-01\tX\tref\tRef/X\t*\t-\t"-Da\n-Db"\n')
    with pytest.raises(ParamsError, match="forbidden"):
        read_params(p)


def test_skip_iters_builds_but_marks_the_shim_untimed(tmp_path):
    header = HEADER.rstrip("\n") + "\titers\n"
    p = tmp_path / "params.tsv"
    p.write_text(header + "\t".join(ROW + ("skip",)) + "\n")
    rows = read_params(p)
    assert rows[0]["iters"] == "skip"
    assert "NGCC_SOS += $(BUILD)/x.so" in rules_for(tmp_path, ROW + ("skip",))
    (tmp_path / "t.c").write_text("@ITERS@\n")
    write_shims(rows, tmp_path / "t.c", tmp_path)
    assert (tmp_path / "x_shim.c").read_text() == "-1\n"


@pytest.mark.parametrize("bad", ["-1", "fast", "1.5"])
def test_bad_iters_rejected(tmp_path, bad):
    p = tmp_path / "params.tsv"
    p.write_text(HEADER.rstrip("\n") + "\titers\n" + "\t".join(ROW + (bad,)) + "\n")
    with pytest.raises(ParamsError, match="iters"):
        read_params(p)
