import io
import zipfile

from extract import decode_name, extract_archive


def make_zip(path, members):
    """members: {name or ZipInfo: bytes}. zipfile stores names like '../x.c' verbatim."""
    with zipfile.ZipFile(path, "w") as zf:
        for name, data in members.items():
            zf.writestr(name, data)


def files_under(root):
    return sorted(
        p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()
    )


def test_keeps_sources_kats_licences_and_pdfs_only(tmp_path):
    z = tmp_path / "a.zip"
    make_zip(
        z,
        {
            "Cand/Ref/X/a.c": b"int a;",
            "Cand/Ref/X/a.h": b"",
            "Cand/Ref/X/b.S": b"",
            "Cand/KAT/KAT_SIG_X.txt": b"Count = 0\n",
            "Cand/doc/Algorithm specifications.pdf": b"%PDF",
            "Cand/Ref/X/a.o": b"\x7fELF",
            "Cand/Ref/X/Makefile": b"all:",
            "Cand/run.sh": b"rm -rf /",
            "Cand/LICENSE": b"MIT",
        },
    )
    extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert files_under(tmp_path / "src") == [
        "sign-01/Cand/KAT/KAT_SIG_X.txt",
        "sign-01/Cand/LICENSE",
        "sign-01/Cand/Ref/X/a.c",
        "sign-01/Cand/Ref/X/a.h",
        "sign-01/Cand/Ref/X/b.S",
    ]
    assert files_under(tmp_path / "specs") == [
        "sign-01/Cand/doc/Algorithm specifications.pdf"
    ]


def test_rejects_traversal_and_links(tmp_path):
    link = zipfile.ZipInfo("Cand/link.c")
    link.external_attr = 0o120777 << 16
    z = tmp_path / "a.zip"
    make_zip(z, {"../evil.c": b"x", "Cand/../../evil2.c": b"x", link: b"/etc/passwd"})
    ex = extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert not (tmp_path / "evil.c").exists() and not (tmp_path / "evil2.c").exists()
    assert files_under(tmp_path / "src") == []
    assert sum(s.startswith("unsafe path") for s in ex.skipped) == 2


def test_unpacks_nested_zip(tmp_path):
    inner = io.BytesIO()
    with zipfile.ZipFile(inner, "w") as zf:
        zf.writestr("Impl/b.c", b"int b;")
    z = tmp_path / "a.zip"
    make_zip(z, {"Cand/Implementations.zip": inner.getvalue()})
    extract_archive(z, "kem-01", tmp_path / "src", tmp_path / "specs")
    assert (
        tmp_path / "src/kem-01/Cand/Implementations/Impl/b.c"
    ).read_bytes() == b"int b;"


def test_decodes_gbk_names():
    name = "参考实现/a.c"
    info = zipfile.ZipInfo(name.encode("gbk").decode("cp437"))
    info.flag_bits = 0
    assert decode_name(info) == name


def test_keeps_names_that_are_already_unicode():
    info = zipfile.ZipInfo("Cand/a\uff08b\uff09.c")
    info.flag_bits = 0
    assert decode_name(info) == "Cand/a\uff08b\uff09.c"


def test_replaces_a_previous_extraction(tmp_path):
    stale = tmp_path / "src/sign-01/old.c"
    stale.parent.mkdir(parents=True)
    stale.write_text("stale")
    z = tmp_path / "a.zip"
    make_zip(z, {"new.c": b""})
    extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert files_under(tmp_path / "src") == ["sign-01/new.c"]


def test_truncates_oversize_kat_to_ten_records_and_skips_other_oversize(tmp_path):
    kat = b"".join(b"Count = %d\nmsg = AA\n\n" % i for i in range(30))
    z = tmp_path / "a.zip"
    make_zip(z, {"C/KAT_SIG_X.txt": kat, "C/big.c": b"x" * len(kat), "C/ok.c": b"y"})
    ex = extract_archive(
        z, "sign-01", tmp_path / "src", tmp_path / "specs", max_member=100
    )
    out = (tmp_path / "src/sign-01/C/KAT_SIG_X.txt").read_bytes()
    assert out.count(b"Count = ") == 10 and b"Count = 9\n" in out
    assert b"Count = 10" not in out
    assert ex.truncated == ["truncated KAT (first 10 records): C/KAT_SIG_X.txt"]
    assert ex.skipped == ["too large: C/big.c"]
    assert not (tmp_path / "src/sign-01/C/big.c").exists()


def test_kat_keeps_whole_records_that_fit_under_the_cap(tmp_path):
    rec = b"Count = %d\n" + b"a" * 40 + b"\n\n"
    kat = b"".join(rec % i for i in range(30))
    z = tmp_path / "a.zip"
    make_zip(z, {"C/KAT_SIG_X.txt": kat})
    ex = extract_archive(
        z,
        "sign-01",
        tmp_path / "src",
        tmp_path / "specs",
        max_member=100,
        prefix_cap=len(rec % 0) * 2 + 5,
    )
    out = (tmp_path / "src/sign-01/C/KAT_SIG_X.txt").read_bytes()
    assert out == rec % 0 + rec % 1
    assert ex.truncated == ["truncated KAT (first 2 records): C/KAT_SIG_X.txt"]


def test_kat_with_first_record_over_cap_is_skipped(tmp_path):
    z = tmp_path / "a.zip"
    make_zip(z, {"C/KAT_SIG_X.txt": b"Count = 0\n" + b"a" * 500 + b"\nCount = 1\n"})
    ex = extract_archive(
        z,
        "sign-01",
        tmp_path / "src",
        tmp_path / "specs",
        max_member=100,
        prefix_cap=50,
    )
    assert files_under(tmp_path / "src") == []
    assert len(ex.skipped) == 1 and "prefix exceeds cap" in ex.skipped[0]


def test_file_collision_is_skipped_not_fatal(tmp_path):
    z = tmp_path / "a.zip"
    make_zip(z, {"C/a.c": b"x", "C/a.c/b.c": b"y", "C/z.c": b"z"})
    ex = extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert "sign-01/C/z.c" in files_under(tmp_path / "src")
    assert len(ex.skipped) == 1 and "C/a.c/b.c" in ex.skipped[0]


def test_nested_depth_limit(tmp_path):
    data = b"int a;"
    for i in range(5):
        buf = io.BytesIO()
        with zipfile.ZipFile(buf, "w") as zf:
            zf.writestr("a.c" if i == 0 else f"d{i - 1}.zip", data)
        data = buf.getvalue()
    z = tmp_path / "a.zip"
    make_zip(z, {"d4.zip": data})
    ex = extract_archive(z, "sign-01", tmp_path / "src", tmp_path / "specs")
    assert files_under(tmp_path / "src") == []
    assert any("too deep" in s for s in ex.skipped)
