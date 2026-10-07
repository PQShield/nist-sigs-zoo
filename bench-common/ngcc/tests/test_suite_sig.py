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
    assert built == ["toy_sig_1.so", "toy_sig_long.so"]


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
