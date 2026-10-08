import hashlib

from kat_compare import (
    canonical_records,
    check,
    compare,
    record_hash,
    record_hashes,
)

GEN = "".join(
    f"Count = {i}\nSeed_Len = 64\nSeed = {i:02X}\nPK_Len = 2\nPK = AB{i:02X}\n\n"
    for i in range(10)
)


def test_identical_passes():
    assert compare(GEN, GEN, 10) == "PASS"


def test_crlf_trailing_blanks_and_hex_case_are_ignored():
    ref = GEN.replace("\n", "  \r\n").replace("AB", "ab")
    assert compare(GEN, ref, 10) == "PASS"


def test_template_mode_reference_ignores_lengths():
    ref = GEN.replace("PK_Len = 2", "PK_Len =")
    assert compare(GEN, ref, 10) == "PASS"


def test_changed_byte_mismatches():
    assert compare(GEN, GEN.replace("PK = AB05", "PK = AC05"), 10) == "MISMATCH"


def test_compares_only_the_requested_prefix():
    assert compare(GEN, GEN.replace("PK = AB05", "PK = AC05"), 3) == "PASS"


def test_short_reference_mismatches():
    assert compare(GEN, GEN.split("Count = 5")[0], 10) == "MISMATCH"


def test_whitespace_and_field_name_case_are_ignored():
    ref = GEN.replace("PK = ", "pk=\t").replace("Count = ", "COUNT  =  ")
    assert compare(GEN, ref, 10) == "PASS"


def test_length_lines_are_never_compared():
    ref = GEN.replace("PK_Len = 2\n", "")
    assert compare(GEN, ref, 10) == "PASS"


def test_header_lines_before_first_record_are_ignored():
    assert compare(GEN, "# KAT_SIG_X\n\n" + GEN, 10) == "PASS"


def test_canonical_form_is_precise():
    rec = next(canonical_records(["junk\nCount = 0\r\nSeed_Len = 64\nSeed = A B\n\n"]))
    assert rec == "count=0\nseed=ab\n"
    assert record_hash(rec) == hashlib.sha256(b"count=0\nseed=ab\n").hexdigest()


def test_check_reports_first_mismatching_record():
    expected = record_hashes(GEN.replace("PK = AB05", "PK = AC05"))
    assert len(expected) == 10
    assert check(GEN, expected, 10) == ("MISMATCH", 5)
    assert check(GEN, expected, 5) == ("PASS", None)


def test_check_missing_records_mismatch_at_first_missing():
    expected = record_hashes(GEN)
    assert check(GEN, expected[:4], 10) == ("MISMATCH", 4)
    assert check(GEN.split("Count = 7")[0], expected, 10) == ("MISMATCH", 7)


def test_record_hashes_stop_at_limit():
    assert len(record_hashes(GEN, 3)) == 3
    assert len(record_hashes(GEN * 2)) == 10
