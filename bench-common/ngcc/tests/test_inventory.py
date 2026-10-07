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
