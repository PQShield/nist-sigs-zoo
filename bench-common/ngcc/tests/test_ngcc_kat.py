import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
NGCC = HERE.parent
TOY = {
    "sig": HERE / "fixtures/src/sign-99/Reference_Implementation/Toy-Sig-1",
    "kem": HERE / "fixtures/src/kem-99/Reference_Implementation/Toy-KEM-1",
}
API = {"sig": "SIG_AlgorithmInstance.c", "kem": "KEM_AlgorithmInstance.c"}


def build(tmp_path, kind, with_api=True):
    d = TOY[kind]
    so = tmp_path / f"toy_{kind}.so"
    srcs = [str(d / "drng.c"), str(HERE / "drng_def.c")] + (
        [str(d / API[kind])] if with_api else []
    )
    subprocess.run(
        ["cc", "-shared", "-fPIC", f"-I{d}", "-o", str(so), *srcs], check=True
    )
    exe = tmp_path / "ngcc_kat"
    subprocess.run(
        ["cc", "-O2", "-std=gnu17", "-o", str(exe), str(NGCC / "ngcc_kat.c"), "-ldl"],
        check=True,
    )
    return exe, so


def run(exe, kind, so, out, *extra):
    return subprocess.run(
        [str(exe), kind, str(so), str(out), *extra],
        capture_output=True,
        text=True,
        check=False,
    )


def keys(text):
    return [line.split(" = ")[0].rstrip(" =") for line in text.splitlines() if line]


def test_signature_records_follow_the_iccs_layout(tmp_path):
    exe, so = build(tmp_path, "sig")
    out = tmp_path / "kat.txt"
    assert run(exe, "sig", so, out).returncode == 0
    text = out.read_text()
    order = [
        "Count",
        "Seed_Len",
        "Seed",
        "PK_Len",
        "PK",
        "SK_Len",
        "SK",
        "M_Len",
        "M",
        "Sn_Len",
        "Sn",
    ]
    assert keys(text) == order * 10
    assert "Seed_Len = 64\n" in text and "PK_Len = 32\n" in text
    m_lens = [
        int(line.split(" = ")[1])
        for line in text.splitlines()
        if line.startswith("M_Len")
    ]
    assert m_lens == list(range(56, 136, 8))
    again = tmp_path / "again.txt"
    run(exe, "sig", so, again)
    assert again.read_text() == text


def test_kem_records_follow_the_iccs_layout(tmp_path):
    exe, so = build(tmp_path, "kem")
    out = tmp_path / "kat.txt"
    assert run(exe, "kem", so, out, "3").returncode == 0
    order = [
        "Count",
        "Seed_Len",
        "Seed",
        "PK_Len",
        "PK",
        "SK_Len",
        "SK",
        "CT_Len",
        "CT",
        "SS_Len",
        "SS",
    ]
    assert keys(out.read_text()) == order * 3


def test_missing_api_symbol_is_a_load_failure(tmp_path):
    exe, so = build(tmp_path, "sig", with_api=False)
    assert run(exe, "sig", so, tmp_path / "kat.txt").returncode == 2
