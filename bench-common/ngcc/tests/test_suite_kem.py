"""Build the toy NGCC KEM instance through bench-kem/schemes/ngcc; check the shim and status.py."""

import csv
import ctypes
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
FIX = HERE / "fixtures"
SUITE = REPO / "bench-kem" / "schemes" / "ngcc"


class KemInfo(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("pk_bytes", ctypes.c_size_t),
        ("sk_bytes", ctypes.c_size_t),
        ("ct_bytes", ctypes.c_size_t),
        ("ss_bytes", ctypes.c_size_t),
        ("iters", ctypes.c_int),
    ]


def make(build, *goals):
    return subprocess.run(
        [
            "make",
            "-C",
            str(SUITE),
            f"PARAMS={FIX / 'params_kem.tsv'}",
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
    assert "FAIL  NGCC-Toy-KEM-Missing" in proc.stdout
    built = [Path(p).name for p in (tmp_path / "built.list").read_text().split()]
    assert built == ["toy_kem_1.so"]


def test_shim_round_trips(tmp_path):
    make(tmp_path)
    lib = ctypes.CDLL(str(tmp_path / "toy_kem_1.so"))
    lib.bench_info.restype = ctypes.POINTER(KemInfo)
    info = lib.bench_info().contents
    assert (
        info.name,
        info.pk_bytes,
        info.sk_bytes,
        info.ct_bytes,
        info.ss_bytes,
    ) == (b"NGCC-Toy-KEM-1", 32, 32, 32, 32)
    pk, sk, ct, ss1, ss2 = (ctypes.create_string_buffer(32) for _ in range(5))
    assert lib.crypto_kem_keypair(pk, sk) == 0
    assert lib.crypto_kem_enc(ct, ss1, pk) == 0
    assert lib.crypto_kem_dec(ss2, ct, sk) == 0
    assert ss1.raw == ss2.raw


def test_status_records_build_and_kat(tmp_path):
    make(tmp_path)
    out = tmp_path / "status.tsv"
    assert make(tmp_path, "status", f"STATUS={out}").returncode == 0
    rows = {r["name"]: r for r in csv.DictReader(out.open(), delimiter="\t")}
    toy = rows["NGCC-Toy-KEM-1"]
    assert (toy["build"], toy["kat"]) == ("ok", "PASS")
    assert rows["NGCC-Toy-KEM-Missing"]["build"] == "FAIL"
