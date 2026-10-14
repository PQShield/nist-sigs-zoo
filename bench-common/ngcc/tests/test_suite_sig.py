"""Build the toy NGCC signature instances through bench/schemes/ngcc; check the shim and status.py."""

import csv
import ctypes
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
FIX = HERE / "fixtures"
SUITE = REPO / "bench" / "schemes" / "ngcc"


class SigInfo(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("pk_bytes", ctypes.c_size_t),
        ("sk_bytes", ctypes.c_size_t),
        ("sig_bytes", ctypes.c_size_t),
        ("iters", ctypes.c_int),
    ]


def make(build, *goals, params=None):
    return subprocess.run(
        [
            "make",
            "-C",
            str(SUITE),
            f"PARAMS={params or FIX / 'params_sig.tsv'}",
            f"NGCC_SRC={FIX / 'src'}",
            f"KAT_HASHES={FIX / 'kat_hashes.tsv'}",
            f"BUILD={build}",
            "NGCC_BUILD=1",
            *goals,
        ],
        capture_output=True,
        text=True,
        check=False,
    )


def test_failed_instance_does_not_fail_the_build(tmp_path):
    proc = make(tmp_path)
    assert proc.returncode == 0, proc.stdout + proc.stderr
    assert "FAIL  NGCC-Toy-Missing" in proc.stdout
    built = [Path(line).name for line in (tmp_path / "built.list").read_text().split()]
    assert built == ["toy_sig_1.so", "toy_sig_long.so", "toy_sig_cxx.so"]


def test_cxx_sources_link_the_cxx_runtime(tmp_path):
    """A row with a .cpp source compiles it with $(CXX) and links libstdc++ into the .so."""
    make(tmp_path)
    so = tmp_path / "toy_sig_cxx.so"
    dyn = subprocess.run(
        ["readelf", "-d", str(so)], capture_output=True, text=True, check=True
    ).stdout
    assert "libstdc++" in dyn
    assert (
        "libstdc++"
        not in subprocess.run(
            ["readelf", "-d", str(tmp_path / "toy_sig_1.so")],
            capture_output=True,
            text=True,
            check=True,
        ).stdout
    )
    lib = ctypes.CDLL(str(so))
    pk, sk, sig = (ctypes.create_string_buffer(32) for _ in range(3))
    msg, siglen, n = bytes(32), ctypes.c_size_t(), ctypes.c_size_t(32)
    assert lib.crypto_sign_keypair(pk, sk) == 0
    assert lib.crypto_sign_signature(sig, ctypes.byref(siglen), msg, n, sk) == 0
    assert lib.crypto_sign_verify(sig, siglen, msg, n, pk) == 0
    assert lib.crypto_sign_verify(sig, siglen, b"\x01" + msg[1:], n, pk) != 0


def test_shim_round_trips_and_rejects_tampering(tmp_path):
    make(tmp_path)
    lib = ctypes.CDLL(str(tmp_path / "toy_sig_1.so"))
    lib.bench_info.restype = ctypes.POINTER(SigInfo)
    info = lib.bench_info().contents
    assert (info.name, info.pk_bytes, info.sk_bytes, info.sig_bytes, info.iters) == (
        b"NGCC-Toy-Sig-1",
        32,
        32,
        32,
        0,
    )
    pk, sk, sig = (ctypes.create_string_buffer(32) for _ in range(3))
    msg, siglen, n = bytes(32), ctypes.c_size_t(), ctypes.c_size_t(32)
    assert lib.crypto_sign_keypair(pk, sk) == 0
    assert lib.crypto_sign_signature(sig, ctypes.byref(siglen), msg, n, sk) == 0
    assert siglen.value == 32
    assert lib.crypto_sign_verify(sig, siglen, msg, n, pk) == 0
    assert lib.crypto_sign_verify(sig, siglen, b"\x01" + msg[1:], n, pk) != 0


def test_overlong_key_is_an_error(tmp_path):
    make(tmp_path)
    lib = ctypes.CDLL(str(tmp_path / "toy_sig_long.so"))
    pk, sk = ctypes.create_string_buffer(64), ctypes.create_string_buffer(64)
    assert lib.crypto_sign_keypair(pk, sk) != 0


def test_status_records_build_kat_and_hash(tmp_path):
    make(tmp_path)
    out = tmp_path / "status.tsv"
    proc = make(tmp_path, "status", f"STATUS={out}")
    assert proc.returncode == 0, proc.stdout + proc.stderr
    rows = {r["name"]: r for r in csv.DictReader(out.open(), delimiter="\t")}
    toy = rows["NGCC-Toy-Sig-1"]
    assert (toy["build"], toy["kat"], toy["hash"], toy["impl"]) == (
        "ok",
        "PASS",
        "own",
        "ref",
    )
    assert rows["NGCC-Toy-Sig-Long"]["kat"] == "CRYPTOFAIL"
    assert (rows["NGCC-Toy-Sig-Cxx"]["build"], rows["NGCC-Toy-Sig-Cxx"]["kat"]) == (
        "ok",
        "PASS",
    )
    assert (rows["NGCC-Toy-Missing"]["build"], rows["NGCC-Toy-Missing"]["kat"]) == (
        "FAIL",
        "-",
    )


def test_status_nokat_when_no_submitted_file(tmp_path):
    header, toy, *_ = (FIX / "params_sig.tsv").read_text().splitlines()[1:]
    params = tmp_path / "params.tsv"
    cols = toy.split("\t")
    cols[0], cols[1], cols[7] = "toy_nokat", "NGCC-Toy-NoKat", "-"
    params.write_text(header + "\n" + "\t".join(cols) + "\n")
    build = tmp_path / "b"
    make(build, params=params)
    out = tmp_path / "status.tsv"
    make(build, "status", f"STATUS={out}", params=params)
    rows = list(csv.DictReader(out.open(), delimiter="\t"))
    assert [(r["build"], r["kat"]) for r in rows] == [("ok", "NOKAT")]


def test_status_needs_hashes_not_kat_files(tmp_path):
    """The submitted KAT file is not read: only kat_hashes.tsv is."""
    import shutil

    src = tmp_path / "src"
    shutil.copytree(FIX / "src", src)
    (src / "sign-99/KAT/KAT_SIG_Toy-Sig-1.txt").unlink()
    build = tmp_path / "b"
    common = [f"NGCC_SRC={src}"]
    make(build, *common)
    out = tmp_path / "status.tsv"
    make(build, "status", f"STATUS={out}", *common)
    rows = {r["name"]: r for r in csv.DictReader(out.open(), delimiter="\t")}
    assert (rows["NGCC-Toy-Sig-1"]["kat"], rows["NGCC-Toy-Sig-1"]["katrec"]) == (
        "PASS",
        "",
    )


def test_status_reports_first_mismatching_record(tmp_path):
    bad = []
    for ln in (FIX / "kat_hashes.tsv").read_text().splitlines(keepends=True):
        cols = ln.split("\t")
        if len(cols) == 5 and cols[1] == "sign-99" and cols[3] == "3":
            ln = "\t".join([*cols[:4], "0" * 64 + "\n"])
        bad.append(ln)
    hashes = tmp_path / "kat_hashes.tsv"
    hashes.write_text("".join(bad))
    build = tmp_path / "b"
    make(build)
    out = tmp_path / "status.tsv"
    make(build, "status", f"STATUS={out}", f"KAT_HASHES={hashes}")
    rows = {r["name"]: r for r in csv.DictReader(out.open(), delimiter="\t")}
    assert (rows["NGCC-Toy-Sig-1"]["kat"], rows["NGCC-Toy-Sig-1"]["katrec"]) == (
        "MISMATCH",
        "3",
    )
