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
    assert ps["dnf"] == "valid-signature-rejected"


def test_op_limit_dnf_note_reads_naturally(sig):
    ps = {"name": "TRINE-512-Balanced", "signing_cycles": 1, "notes": "Ref."}
    row = {"name": "NGCC-TRINE-512-Balanced", "dnf": "operation-over-600s"}
    changed, _ = sig.apply_row(sig_doc(ps), "TRINE-512-Balanced", row)
    assert changed and "signing_cycles" not in ps
    assert ps["notes"] == "Ref. Benchmark did not finish (operation over 600s)."
    assert ps["dnf"] == "operation-over-600s"


def test_sig_dnf_elsewhere_keeps_timings(sig):
    ps = {"name": "ML-DSA-65", "signing_cycles": 1, "notes": None}
    changed, msg = sig.apply_row(
        sig_doc(ps), "ML-DSA-65", {"name": "ML-DSA-65", "dnf": "SIGSEGV"}
    )
    assert not changed and msg.startswith("WARNING")
    assert ps == {"name": "ML-DSA-65", "signing_cycles": 1, "notes": None}


def test_sig_timings_clear_an_old_ngcc_dnf_note(sig):
    ps = {
        "name": "Tins256",
        "notes": "Ref. Benchmark did not finish (SIGABRT).",
        "dnf": "SIGABRT",
    }
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
    assert "dnf" not in ps


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
        "dnf": "timeout",
    }
    other = {"name": "ML-KEM-512", "keygen_cycles": 1}
    changed, msg = kem.apply_row(
        {"parametersets": [other]},
        "ML-KEM-512",
        {"name": "ML-KEM-512", "dnf": "SIGSEGV"},
    )
    assert not changed and msg.startswith("WARNING") and other["keygen_cycles"] == 1


def test_no_bench_mapping_report_groups_by_programme(sig):
    assert sig.membership_label(None) == "no programme"
    assert sig.membership_label({"nist-onramp": "round-2"}) == "nist-onramp round-2"
    assert (
        sig.membership_label(
            {"nist-onramp": ["round-2", "round-3"], "kpqc": "selected"}
        )
        == "kpqc selected, nist-onramp round-2/round-3"
    )


# --- benchmark environments ------------------------------------------------------


def test_timings_name_their_environment_after_the_timing_fields(sig):
    from ruamel.yaml.comments import CommentedMap

    ps = CommentedMap(
        [("name", "Tins256"), ("pk", 1), ("notes", "Ref."), ("bench_env", "old")]
    )
    row = {
        "name": "NGCC-Tins256",
        "dnf": None,
        "signing_cycles": 5,
        "verification_cycles": 6,
        "signing_us": 0.5,
        "verification_us": 0.6,
    }
    changed, _ = sig.apply_row(sig_doc(ps), "Tins256", row, "2026-10-16-c8i.4xlarge")
    assert changed
    keys = list(ps)
    assert ps["bench_env"] == "2026-10-16-c8i.4xlarge"
    assert keys.index("bench_env") == keys.index("verification_us") + 1
    sig.apply_row(sig_doc(ps), "Tins256", {"name": "NGCC-Tins256", "dnf": "SIGABRT"})
    assert "bench_env" not in ps  # a DNF has no timings and so no environment


def test_env_ids_and_file(sig, tmp_path):
    import bench_env

    meta = {
        "date": "2026-10-16T09:00:00Z",
        "cpu": "Intel(R) Xeon(R) 6975P-C",
        "instance_type": "c8i.4xlarge",
        "cores": "16 (threads/core: 2)",
        "_sources": {"mldsa": "https://example.org@abc"},
    }
    assert bench_env.env_id(meta) == "2026-10-16-c8i.4xlarge"
    legacy = {"date": "2026-10-13T10:23:10Z", "cpu": "Intel(R) Xeon(R) Platinum 8488C"}
    assert bench_env.env_id(legacy) == "2026-10-13-c7i.4xlarge"
    laptop = {"date": "2026-06-25T16:01:24Z", "cpu": "12th Gen Intel(R) Core(TM) i7"}
    assert bench_env.env_id(laptop) == "2026-06-25-local-12th_gen_intel_r_core_tm_i7"

    data = tmp_path / "schemes"
    data.mkdir()
    (data / "A.yaml").write_text(
        "name: A\nversions:\n- parametersets:\n  - name: a\n    bench_env: 2026-10-16-c8i.4xlarge\n"
    )
    path = tmp_path / "env.yaml"
    bench_env.update_env_file(path, bench_env.make_env(legacy), data)
    assert bench_env.load_envs(path) == []  # nothing uses it
    bench_env.update_env_file(path, bench_env.make_env(meta), data)
    envs = bench_env.load_envs(path)
    assert [e["id"] for e in envs] == ["2026-10-16-c8i.4xlarge"]
    assert envs[0]["cpu"]["cores"] == 16 and envs[0]["sources"] == meta["_sources"]
    assert path.read_text().startswith("# Licensed under CC BY 4.0")


# --- version-aware targets -------------------------------------------------------


def timed_row(name, sign):
    return {
        "name": name,
        "dnf": None,
        "signing_cycles": sign,
        "verification_cycles": 2,
        "signing_us": 0.5,
        "verification_us": 0.1,
    }


def two_rounds():
    return {
        "versions": [
            {
                "version": "Round 2",
                "date": "2025-03-26",
                "parametersets": [{"name": "one", "signing_cycles": 9}],
            },
            {
                "version": "Round 2",
                "date": "2025-02-05",
                "perf_source": "submission document",
                "parametersets": [
                    {"name": "one", "signing_cycles": 7},
                    {"name": "two", "signing_cycles": 8},
                ],
            },
            {
                "version": "Round 1",
                "date": "2023-06-01",
                "perf_source": "submission document",
                "parametersets": [{"name": "one", "signing_cycles": 5}],
            },
        ]
    }


def test_targets_normalise_all_value_forms(sig):
    assert sig.targets(("A.yaml", "1")) == [("A.yaml", "1", None)]
    assert sig.targets(("A.yaml", "1", "v1")) == [("A.yaml", "1", "v1")]
    assert sig.targets([("A.yaml", "1", "v2"), ("A.yaml", "1", "v1")]) == [
        ("A.yaml", "1", "v2"),
        ("A.yaml", "1", "v1"),
    ]


def test_row_without_version_lands_on_first_version(sig):
    doc = two_rounds()
    changed, _ = sig.apply_row(doc, "one", timed_row("X-one", 100))
    assert changed and doc["versions"][0]["parametersets"][0]["signing_cycles"] == 100
    assert doc["versions"][2]["parametersets"][0]["signing_cycles"] == 5


def test_row_with_version_lands_on_exactly_that_version(sig):
    doc = two_rounds()
    changed, _ = sig.apply_row(doc, "one", timed_row("X-R1-one", 100), "env", "Round 1")
    assert changed
    assert [v["parametersets"][0]["signing_cycles"] for v in doc["versions"]] == [
        9,
        7,
        100,
    ]
    # Every timed set of Round 1 is now ours, so its label follows.
    assert doc["versions"][2]["perf_source"] == "our benchmark"


def test_version_date_disambiguates_a_repeated_label(sig):
    doc = two_rounds()
    with pytest.raises(ValueError, match="matches 2 versions"):
        sig.apply_row(doc, "one", timed_row("X-R2-one", 1), None, "Round 2")
    sig.apply_row(doc, "one", timed_row("X-R2-one", 100), "env", "Round 2@2025-02-05")
    r2 = doc["versions"][1]
    assert r2["parametersets"][0]["signing_cycles"] == 100
    assert doc["versions"][0]["parametersets"][0]["signing_cycles"] == 9
    # "two" still has submission numbers: the label stays until all timed sets are ours.
    assert r2["perf_source"] == "submission document"
    sig.apply_row(doc, "two", timed_row("X-R2-two", 200), "env", "Round 2@2025-02-05")
    assert r2["perf_source"] == "our benchmark"


def test_unknown_version_or_set_in_version(sig):
    with pytest.raises(ValueError, match="matches 0 versions"):
        sig.apply_row(two_rounds(), "one", timed_row("X", 1), None, "Round 9")
    assert sig.apply_row(two_rounds(), "two", timed_row("X", 1), None, "Round 1") == (
        False,
        None,
    )


def test_mapped_sets_are_per_version(sig, monkeypatch):
    monkeypatch.setattr(
        sig,
        "BENCH_TO_YAML",
        {
            "X-one": ("X.yaml", "one"),
            "X-R1-one": ("X.yaml", "one", "Round 1"),
            "X-R2-both": [("X.yaml", "two", "Round 2@2025-02-05"), ("Y.yaml", "two")],
        },
    )
    assert sig.mapped_sets(two_rounds(), "X.yaml") == {
        (0, "one"),
        (1, "two"),
        (2, "one"),
    }


def test_every_real_target_resolves(sig):
    assert sig.target_errors() == []


def test_bench_row_names_are_unique_across_scheme_dirs():
    seen: dict[str, str] = {}
    dupes = []
    for tsv in sorted((REPO / "bench" / "schemes").glob("*/*params*.tsv")):
        lines = [
            ln for ln in tsv.read_text().splitlines() if ln and not ln.startswith("#")
        ]
        cols = lines[0].split("\t")
        for ln in lines[1:]:
            name = dict(zip(cols, ln.split("\t"), strict=False))["name"]
            if name in seen:
                dupes.append(f"{name}: {seen[name]} and {tsv}")
            seen[name] = str(tsv)
    assert dupes == []


def test_older_round_rows_target_their_own_version(sig):
    """Rows of bench/schemes/<x>_r1, <x>_r2 must name their version: without one they
    would land on the newest version of the set."""
    for tsv in sorted((REPO / "bench" / "schemes").glob("*_r[12]/*params*.tsv")):
        lines = [
            ln for ln in tsv.read_text().splitlines() if ln and not ln.startswith("#")
        ]
        cols = lines[0].split("\t")
        for ln in lines[1:]:
            name = dict(zip(cols, ln.split("\t"), strict=False))["name"]
            assert name in sig.BENCH_TO_YAML, f"{tsv}: {name} has no mapping"
            assert all(v for _, _, v in sig.targets(sig.BENCH_TO_YAML[name])), name
