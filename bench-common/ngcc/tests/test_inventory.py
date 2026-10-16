from inventory import find_instances

API = "unsigned long long sig_get_pk_len_bytes(void)\n{\n    return 1;\n}\n"
HDR = '#define ALGORITHM_INSTANCE "Aigis-sig1"\n'


def write(root, rel, text=""):
    p = root / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)


def test_finds_reference_and_optimised_instances(tmp_path):
    root = tmp_path / "sign-01"
    ref = "Aigis/Implementations/Reference_Implementation/Aigis-Sig-I"
    write(root, f"{ref}/sign.c")
    write(root, f"{ref}/drng.c")
    write(root, f"{ref}/kat_test/SIG_AlgorithmInstance.c", API)
    write(root, f"{ref}/kat_test/SIG_AlgorithmInstance.h", HDR)
    write(root, f"{ref}/kat_test/KAT_SIG.c")
    write(
        root,
        "Aigis/Implementations/Optimized_Implementation/avx2/SIG_AlgorithmInstance.c",
        API,
    )
    write(
        root,
        "Aigis/Implementations/Optimized_Implementation/avx2/SIG_AlgorithmInstance.h",
        HDR,
    )
    write(root, "Aigis/KAT/KAT_SIG_Aigis-sig1.txt", "Count = 0\n")

    rows = {r["impl"]: r for r in find_instances(root, "sign-01", "sig")}
    assert set(rows) == {"ref", "opt"}
    r = rows["ref"]
    assert (r["file"], r["name"], r["instance"], r["srcdir"]) == (
        "sign_01_aigis_sig1",
        "NGCC-Aigis-sig1",
        "Aigis-sig1",
        ref,
    )
    assert r["srcs"] == "* kat_test/SIG_AlgorithmInstance.c"
    assert r["incs"] == "kat_test"
    assert r["kat"] == "Aigis/KAT/KAT_SIG_Aigis-sig1.txt"
    assert rows["opt"]["srcs"] == "*" and rows["opt"]["incs"] == ""


def kat_for(tmp_path, kind, instance, srcrel, kat_files, header=True):
    """Build one candidate with a single API instance and return its `kat` column."""
    root = tmp_path / "cand"
    api = kind + "_get_pk_len_bytes"
    text = API.replace("sig_get_pk_len_bytes", api)
    write(root, f"{srcrel}/{kind.upper()}_AlgorithmInstance.c", text)
    if header:
        write(
            root,
            f"{srcrel}/{kind.upper()}_AlgorithmInstance.h",
            f'#define ALGORITHM_INSTANCE "{instance}"\n',
        )
    for f in kat_files:
        write(root, f, "Count = 0\n")
    (row,) = find_instances(root, "cand", kind)
    return row["kat"]


def test_roman_and_plus_instance_names_and_iccs_variant(tmp_path):
    kat = kat_for(
        tmp_path,
        "sig",
        "Aigis-Sig+-II",
        "Impl/Aigis-Sig+-II",
        [
            "Test_Vectors/KAT_SIG_Aigis-sig2.txt",
            "Test_Vectors/KAT_SIG_Aigis-sig2_iccs.txt",
            "Test_Vectors/KAT_SIG_Aigis-sig1.txt",
        ],
    )
    assert kat == "Test_Vectors/KAT_SIG_Aigis-sig2.txt"


def test_case_and_separator_insensitive_in_any_dir_case(tmp_path):
    kat = kat_for(
        tmp_path,
        "kem",
        "Mito-1-E-128",
        "Impl/Mito-1-E-128",
        ["Test_Vector/KAT_KEM_MITO_1_E_128.txt", "Test_Vector/KAT_KEM_MITO_1_128.txt"],
    )
    assert kat == "Test_Vector/KAT_KEM_MITO_1_E_128.txt"


def test_falls_back_to_source_dir_name_when_instance_is_generic(tmp_path):
    kat = kat_for(
        tmp_path,
        "sig",
        "ngcc",
        "Impl/Galas-160S/ngcc",
        ["KAT/KAT_SIG_Galas-160S.txt", "KAT/KAT_SIG_Galas-160F.txt"],
        header=False,
    )
    assert kat == "KAT/KAT_SIG_Galas-160S.txt"


def test_implementation_suffix_in_kat_name(tmp_path):
    files = [
        "TV/avx2/OPSsig-128/KAT_SIG_OPSsig-128-avx2.txt",
        "TV/reference/OPSsig-128/KAT_SIG_OPSsig-128-reference.txt",
    ]
    kat = kat_for(
        tmp_path, "sig", "OPSsig-128", "Impl/Reference_Implementation/OPSsig-128", files
    )
    assert kat == "TV/reference/OPSsig-128/KAT_SIG_OPSsig-128-reference.txt"


def test_same_name_in_several_dirs_prefers_closest(tmp_path):
    kat = kat_for(
        tmp_path,
        "kem",
        "BIKE",
        "Impl/reference/BIKE",
        ["TV/reference/KAT_KEM_BIKE.txt", "TV/optimization/KAT_KEM_BIKE.txt"],
    )
    assert kat == "TV/reference/KAT_KEM_BIKE.txt"


def test_double_extension_and_nist_rsp_names(tmp_path):
    assert (
        kat_for(tmp_path, "sig", "T-512", "Impl/T-512", ["TV/KAT_SIG_T-512.txt.txt"])
        == "TV/KAT_SIG_T-512.txt.txt"
    )
    other = tmp_path / "o"
    other.mkdir()
    assert (
        kat_for(other, "sig", "X-1", "Impl/X-1", ["kat/PQCsignKAT_X-1.rsp"])
        == "kat/PQCsignKAT_X-1.rsp"
    )


def test_ambiguous_or_missing_kat_gives_dash(tmp_path):
    amb = kat_for(
        tmp_path,
        "sig",
        "SQI-L1",
        "Impl/SQI-L1",
        ["TV/KAT_SIG_SQI-L1_compressed.txt", "TV/KAT_SIG_SQI-L1_uncompressed.txt"],
    )
    assert amb == "-"
    other = tmp_path / "o"
    other.mkdir()
    assert (
        kat_for(other, "kem", "Foo-512", "Impl/Foo-512", ["TV/KAT_KEM_Foo-128.txt"])
        == "-"
    )
    sigs = tmp_path / "s"
    sigs.mkdir()
    # a SIG KAT never matches a KEM instance
    assert kat_for(sigs, "kem", "Foo", "Impl/Foo", ["TV/KAT_SIG_Foo.txt"]) == "-"
