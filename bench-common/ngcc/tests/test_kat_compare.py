from kat_compare import compare

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
