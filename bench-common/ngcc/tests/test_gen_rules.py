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
