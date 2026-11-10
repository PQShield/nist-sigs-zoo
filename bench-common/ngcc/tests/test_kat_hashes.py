import io
import zipfile
from pathlib import PurePosixPath

import kat_hashes
from extract import Extractor
from kat_compare import record_hashes

KAT = "".join(
    f"Count = {i}\nSeed = {i:02X}\nPK_Len = 1\nPK = {i:02X}\n\n" for i in range(12)
)


def test_hash_stream_raw_and_first_ten_records():
    raw, recs = kat_hashes.hash_stream(io.BytesIO(KAT.encode()))
    import hashlib

    assert raw == hashlib.sha256(KAT.encode()).hexdigest()
    assert recs == record_hashes(KAT) and len(recs) == 10


def test_extractor_sinks_wanted_kats_from_nested_zips_and_writes_nothing(tmp_path):
    inner = io.BytesIO()
    with zipfile.ZipFile(inner, "w") as zf:
        zf.writestr("T/KAT_SIG_X.txt", KAT)
        zf.writestr("T/a.c", "int a;")
    outer = tmp_path / "a.zip"
    with zipfile.ZipFile(outer, "w") as zf:
        zf.writestr("Test_Vectors/y-512.txt", KAT)
        zf.writestr("Impl.zip", inner.getvalue())
        zf.writestr("Other/KAT_SIG_Z.txt", KAT)
    seen = {}
    ex = Extractor(
        tmp_path / "src",
        tmp_path / "specs",
        kat_wanted={
            PurePosixPath("Test_Vectors/y-512.txt"),
            PurePosixPath("Impl/T/KAT_SIG_X.txt"),
        },
        kat_sink=lambda rel, s: seen.__setitem__(
            rel.as_posix(), kat_hashes.hash_stream(s)
        ),
    )
    with zipfile.ZipFile(outer) as zf:
        ex.zip(zf, PurePosixPath())
    assert sorted(seen) == ["Impl/T/KAT_SIG_X.txt", "Test_Vectors/y-512.txt"]
    assert not (tmp_path / "src").exists()


def test_generate_from_src_and_load_round_trip(tmp_path):
    (tmp_path / "src/kem-07/TV").mkdir(parents=True)
    (tmp_path / "src/kem-07/TV/k.txt").write_text(KAT)
    params = tmp_path / "params.tsv"
    params.write_text(
        "file\tname\tid\tinstance\timpl\tsrcdir\tsrcs\tkat\n"
        "a\tNGCC-A\tkem-07\tA\tref\tR\t*\tTV/k.txt\n"
        "b\tNGCC-B\tkem-07\tB\tref\tR\t*\tTV/missing.txt\n"
    )
    out = tmp_path / "h.tsv"
    rc = kat_hashes.main(
        ["--src", str(tmp_path / "src"), "--params", str(params), "--out", str(out)]
    )
    assert rc == 1  # one KAT file is missing
    assert kat_hashes.load(out) == {("kem-07", "TV/k.txt"): record_hashes(KAT)}
    rows = [
        ln.split("\t") for ln in out.read_text().splitlines() if not ln.startswith("#")
    ]
    assert rows[0] == kat_hashes.COLUMNS
    assert rows[1][:4] == ["kem", "kem-07", "TV/k.txt", "raw"]
