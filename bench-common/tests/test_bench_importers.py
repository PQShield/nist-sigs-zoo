"""Results-file parsing and DNF handling in bench*/update_scheme_data.py."""

import importlib.util
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parents[2]


def load(suite):
    spec = importlib.util.spec_from_file_location(
        f"{suite.replace('-', '_')}_update", REPO / suite / "update_scheme_data.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


@pytest.fixture(scope="module")
def sig():
    return load("bench")


@pytest.fixture(scope="module")
def kem():
    return load("bench-kem")


SIG_RESULTS = """\
# pq-sigs benchmark results
# cpu:          Test CPU
#   schemes/mldsa/ref: https://example.org/mldsa@abc
#
# cyclecounter: rdtsc (reference cycles, constant TSC rate)
scheme                       iters    keygen (cyc)  keygen (us)      sign (cyc)   sign (us)    verify (cyc)  verify (us)
------                       -----    ------------  -----------      ----------   ---------    ------------  ----------
ML-DSA-44                     1000           58407        29.3          153503        77.1           60647        30.5
toy chatter on stdout 1 2 3 4 5 6 7 8
RSA-2048 (PSS)                1000         1000000       500.0          200000       100.0           10000         5.0
NGCC-Tins128                    10             DNF         DNF             DNF         DNF             DNF         DNF  valid-signature-rejected
DNF NGCC-Tins128: valid-signature-rejected
FAIL Old-Style: valid signature rejected by the old harness here
ML-DSA-65                       10             DNF         DNF             DNF         DNF             DNF         DNF  SIGSEGV
3 scheme(s) did not finish (DNF), 2 finished
"""

KEM_RESULTS = """\
# cpu:          Test CPU
scheme                       iters    keygen (cyc)  keygen (us)    encaps (cyc)  encaps (us)    decaps (cyc)  decaps (us)      ok
------                       -----    ------------  -----------    ------------  -----------    ------------  -----------      --
ML-KEM-512                      20           31088        15.6           34950        17.6           45729        23.0     yes
  ! Toy: 1/20 shared-secret mismatches
NGCC-E1                         20             DNF         DNF             DNF         DNF             DNF         DNF  SIGABRT
NGCC-E2                         20             DNF         DNF             DNF         DNF             DNF         DNF  operation-over-600s
"""


def test_sig_parser_reads_rows_and_dnf_and_skips_noise(sig, tmp_path):
    f = tmp_path / "r.txt"
    f.write_text(SIG_RESULTS)
    meta, rows = sig.parse_results_file(f)
    assert meta["cpu"] == "Test CPU"
    assert meta["_sources"] == {"mldsa": "https://example.org/mldsa@abc"}
    assert [(r["name"], r["dnf"]) for r in rows] == [
        ("ML-DSA-44", None),
        ("RSA-2048 (PSS)", None),
        ("NGCC-Tins128", "valid-signature-rejected"),
        ("ML-DSA-65", "SIGSEGV"),
    ]
    assert rows[0]["signing_cycles"] == 153503
    assert rows[1]["verification_us"] == 5.0
    assert rows[2]["iters"] == 10


def test_kem_parser_reads_rows_and_dnf(kem, tmp_path):
    f = tmp_path / "r.txt"
    f.write_text(KEM_RESULTS)
    _, rows = kem.parse_results_file(f)
    assert [(r["name"], r["dnf"]) for r in rows] == [
        ("ML-KEM-512", None),
        ("NGCC-E1", "SIGABRT"),
        ("NGCC-E2", "operation-over-600s"),
    ]
    assert rows[0]["decaps_cycles"] == 45729 and rows[0]["ok"] == "yes"


def sig_doc(ps):
    return {"versions": [{"version": "Round 1", "parametersets": [ps]}]}


def test_sig_dnf_on_ngcc_drops_timings_and_notes_it(sig):
    ps = {"name": "Tins128", "signing_cycles": 1, "signing_us": 0.1, "notes": "Ref."}
    row = {"name": "NGCC-Tins128", "dnf": "valid-signature-rejected"}
    changed, msg = sig.apply_row(sig_doc(ps), "Tins128", row)
    assert changed and "DNF" in msg
    assert "signing_cycles" not in ps and "signing_us" not in ps
    assert ps["notes"] == "Ref. Benchmark did not finish (valid signature rejected)."


def test_op_limit_dnf_note_reads_naturally(sig):
    ps = {"name": "TRINE-512-Balanced", "signing_cycles": 1, "notes": "Ref."}
    row = {"name": "NGCC-TRINE-512-Balanced", "dnf": "operation-over-600s"}
    changed, _ = sig.apply_row(sig_doc(ps), "TRINE-512-Balanced", row)
    assert changed and "signing_cycles" not in ps
    assert ps["notes"] == "Ref. Benchmark did not finish (operation over 600s)."


def test_sig_dnf_elsewhere_keeps_timings(sig):
    ps = {"name": "ML-DSA-65", "signing_cycles": 1, "notes": None}
    changed, msg = sig.apply_row(
        sig_doc(ps), "ML-DSA-65", {"name": "ML-DSA-65", "dnf": "SIGSEGV"}
    )
    assert not changed and msg.startswith("WARNING")
    assert ps == {"name": "ML-DSA-65", "signing_cycles": 1, "notes": None}


def test_sig_timings_clear_an_old_ngcc_dnf_note(sig):
    ps = {"name": "Tins256", "notes": "Ref. Benchmark did not finish (SIGABRT)."}
    row = {
        "name": "NGCC-Tins256",
        "dnf": None,
        "signing_cycles": 5,
        "verification_cycles": 6,
        "signing_us": 0.5,
        "verification_us": 0.6,
    }
    changed, _ = sig.apply_row(sig_doc(ps), "Tins256", row)
    assert changed
    assert ps["notes"] == "Ref." and ps["signing_cycles"] == 5


def test_sig_missing_parameterset(sig):
    assert sig.apply_row(sig_doc({"name": "X"}), "Y", {"dnf": "timeout"}) == (
        False,
        None,
    )


def test_kem_dnf_on_ngcc_and_elsewhere(kem):
    ngcc = {"name": "E1", "keygen_cycles": 1, "decaps_us": 2.0}
    changed, _ = kem.apply_row(
        {"parametersets": [ngcc]}, "E1", {"name": "NGCC-E1", "dnf": "timeout"}
    )
    assert changed and ngcc == {
        "name": "E1",
        "notes": "Benchmark did not finish (timeout).",
    }
    other = {"name": "ML-KEM-512", "keygen_cycles": 1}
    changed, msg = kem.apply_row(
        {"parametersets": [other]},
        "ML-KEM-512",
        {"name": "ML-KEM-512", "dnf": "SIGSEGV"},
    )
    assert not changed and msg.startswith("WARNING") and other["keygen_cycles"] == 1
