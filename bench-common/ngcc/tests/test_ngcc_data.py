import pytest
from ruamel.yaml import YAML

import ngcc_data as nd

CANDS = [
    {
        "id": "sign-01",
        "kind": "signature",
        "name": "Aigis-Sig+",
        "file": "Aigis-Sig-plus",
        "website": "https://www.niccs.org.cn/x",
        "category": "Lattices",
        "assumption": "MLWE/MSIS",
        "spec": "s.pdf",
        "instances": [
            {"instance": "A1", "ngcc_level": 128, "recommended": True, "ref": "T1"},
            {"instance": "A2", "ngcc_level": 256, "ref": "T1"},
            {"instance": "A9", "omit": "Intermediate 192-bit set", "ref": "T1"},
            {
                "instance": "A3",
                "ngcc_level": 512,
                "ref": "T1",
                "pk": 10,
                "sig": 20,
                "size_ref": "Table 4",
            },
        ],
    },
    {
        "id": "kem-01",
        "kind": "kem",
        "name": "Aigis-Enc+",
        "file": "Aigis-Enc-plus",
        "website": "https://www.niccs.org.cn/y",
        "category": "Lattices",
        "assumption": "MLWE",
        "spec": "s.pdf",
        "instances": [{"instance": "E1", "ngcc_level": 128, "ref": "T2"}],
    },
]
SIG_SIZES = {
    "NGCC-A1": {"pk": 928, "sk": 1, "sig": 2015},
    "NGCC-A2": {"pk": 1824, "sk": 1, "sig": 4533},
}
KEM_SIZES = {"NGCC-E1": {"pk": 656, "sk": 1, "ct": 896, "ss": 32}}
STATUS = {
    "NGCC-A1": {"impl": "ref", "kat": "PASS", "hash": "placeholder", "build": "ok"},
    "NGCC-A2": {"impl": "opt", "kat": "MISMATCH", "hash": "own", "build": "ok"},
}


def load(path):
    return YAML(typ="safe").load(path.read_text())


def test_bench_names_map_to_listed_instances_only():
    assert nd.bench_to_yaml(CANDS, "signature") == {
        "NGCC-A1": ("Aigis-Sig-plus.yaml", "A1"),
        "NGCC-A2": ("Aigis-Sig-plus.yaml", "A2"),
        "NGCC-A3": ("Aigis-Sig-plus.yaml", "A3"),
    }


def test_unclaimed_built_instance_is_an_error():
    with pytest.raises(nd.NgccDataError, match="NGCC-Zz"):
        nd.check_coverage(
            CANDS, "signature", {**SIG_SIZES, "NGCC-Zz": {"pk": 1, "sk": 1, "sig": 1}}
        )


def test_listed_instance_without_any_sizes_is_an_error():
    with pytest.raises(nd.NgccDataError, match="NGCC-A2"):
        nd.check_coverage(CANDS, "signature", {"NGCC-A1": SIG_SIZES["NGCC-A1"]})


def test_signature_document(tmp_path):
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    doc = load(tmp_path / "Aigis-Sig-plus.yaml")
    v = doc["versions"][0]
    assert (doc["name"], doc["category"], v["version"], v["date"], v["status"]) == (
        "Aigis-Sig+",
        "Lattices",
        "Round 1",
        "2026-09-20",
        "NGCC round 1",
    )
    assert v["programmes"] == {"china-ngcc": "round-1"}
    a1, a2, a3 = v["parametersets"]
    assert a1 == {
        "name": "A1",
        "ngcc_level": 128,
        "recommended": True,
        "pk": 928,
        "sig": 2015,
        "notes": nd.ps_notes(STATUS["NGCC-A1"]),
    }
    assert "recommended" not in a2 and "level" not in a2
    assert (
        "Optimised x86 implementation" in a2["notes"] and "test vectors" in a2["notes"]
    )
    assert "placeholder hash" in a1["notes"]
    assert (a3["pk"], a3["sig"]) == (10, 20) and "Table 4" in a3["notes"]


def test_kem_document(tmp_path):
    nd.write_files(CANDS, "kem", KEM_SIZES, {}, tmp_path)
    doc = load(tmp_path / "Aigis-Enc-plus.yaml")
    assert (doc["status"], doc["programmes"]) == (
        "NGCC round 1",
        {"china-ngcc": "round-1"},
    )
    assert doc["parametersets"] == [
        {"name": "E1", "ngcc_level": 128, "pk": 656, "ct": 896}
    ]


def test_reimport_keeps_timings(tmp_path):
    nd.write_files(CANDS, "kem", KEM_SIZES, {}, tmp_path)
    path = tmp_path / "Aigis-Enc-plus.yaml"
    path.write_text(
        path.read_text().replace(
            "ct: 896",
            "ct: 896\n  keygen_cycles: 1000\n  keygen_us: 0.4\n  bench_env: e1",
        )
    )
    nd.write_files(
        CANDS,
        "kem",
        {"NGCC-E1": {"pk": 656, "sk": 1, "ct": 900, "ss": 32}},
        {},
        tmp_path,
    )
    ps = load(path)["parametersets"][0]
    assert (ps["ct"], ps["keygen_cycles"], ps["keygen_us"]) == (900, 1000, 0.4)
    assert ps["bench_env"] == "e1"  # where the kept timings were measured


def test_refuses_to_overwrite_other_schemes(tmp_path):
    (tmp_path / "Aigis-Enc-plus.yaml").write_text(
        "name: Other\nprogrammes:\n  kpqc: selected\nparametersets: []\n"
    )
    with pytest.raises(nd.NgccDataError, match="not an NGCC file"):
        nd.write_files(CANDS, "kem", KEM_SIZES, {}, tmp_path)


def test_measured_sizes_that_differ_from_spec_get_a_note(tmp_path):
    cands = [
        {
            **CANDS[1],
            "instances": [
                {
                    "instance": "E1",
                    "ngcc_level": 128,
                    "ref": "T2",
                    "pk": 660,
                    "ct": 896,
                    "sk": 700,
                    "size_ref": "Table 7.1 (PDF p. 57)",
                }
            ],
        }
    ]
    nd.write_files(
        cands,
        "kem",
        {"NGCC-E1": {"pk": 656, "sk": 697, "ct": 896, "ss": 32}},
        {},
        tmp_path,
    )
    ps = load(tmp_path / "Aigis-Enc-plus.yaml")["parametersets"][0]
    assert (ps["pk"], ps["ct"]) == (656, 896)
    assert (
        "Table 7.1 (PDF p. 57)" in ps["notes"]
        and "pk 660 B" in ps["notes"]
        and "sk 700 B" in ps["notes"]
    )
    assert "ct" not in ps["notes"].split("specification")[1].split(".")[0]


def test_mismatch_cause_is_unknown_and_unbuilt_note_is_neutral():
    assert "cause unknown" in nd.ps_notes(
        {"impl": "ref", "kat": "MISMATCH", "hash": "own"}
    )
    assert (
        nd.ps_notes({"impl": "ref", "kat": "PASS", "hash": "own"})
        == "Reference implementation."
    )


def test_findings_are_written_with_credit(tmp_path):
    cands = [
        dict(
            CANDS[1],
            findings={
                "url": "https://ngcc.dev/reports/kem-01.html",
                "implementation": "Decapsulation does not reject invalid ciphertexts (Xiong and Wang).",
                "sources": ["https://example.org/post"],
            },
        )
    ]
    nd.write_files(cands, "kem", KEM_SIZES, {}, tmp_path)
    doc = load(tmp_path / "Aigis-Enc-plus.yaml")
    assert doc["findings_url"] == "https://ngcc.dev/reports/kem-01.html"
    assert doc["implementation"].endswith("(Xiong and Wang).")
    assert "broken" not in doc and "sources" not in doc


def test_signature_findings_go_on_the_version_and_instance_flags_on_sets(tmp_path):
    cand = dict(
        CANDS[0],
        findings={
            "url": "https://ngcc.dev/reports/sign-01.html",
        },
        instances=[
            dict(CANDS[0]["instances"][0], broken="Forgery (Feussner)."),
            *CANDS[0]["instances"][1:],
        ],
    )
    nd.write_files([cand], "signature", SIG_SIZES, {}, tmp_path)
    doc = load(tmp_path / "Aigis-Sig-plus.yaml")
    version = doc["versions"][0]
    assert version["findings_url"] == "https://ngcc.dev/reports/sign-01.html"
    assert "implementation" not in version and "broken" not in version
    sets = {p["name"]: p for p in version["parametersets"]}
    assert sets["A1"]["broken"] == "Forgery (Feussner)."
    assert "broken" not in sets["A2"]


def test_dnf_note_replaces_timings_and_is_cleared_again():
    ps = {
        "name": "E1",
        "keygen_cycles": 5,
        "decaps_us": 1.0,
        "notes": "Reference implementation.",
    }
    nd.mark_dnf(ps, "kem", "valid-signature-rejected")
    assert "keygen_cycles" not in ps and "decaps_us" not in ps
    assert ps["dnf"] == "valid-signature-rejected"
    assert ps["notes"] == (
        "Reference implementation. Benchmark did not finish (valid signature rejected)."
    )
    nd.mark_dnf(ps, "kem", "SIGABRT")  # a newer DNF replaces the old reason
    assert (
        ps["notes"] == "Reference implementation. Benchmark did not finish (SIGABRT)."
    )
    assert ps["dnf"] == "SIGABRT"
    nd.clear_dnf(ps)
    assert ps["notes"] == "Reference implementation." and "dnf" not in ps
    bare = {"name": "E1"}
    nd.mark_dnf(bare, "signature", "timeout")
    assert bare["notes"] == "Benchmark did not finish (timeout)."
    nd.clear_dnf(bare)
    assert "notes" not in bare


def test_reimport_keeps_dnf_note(tmp_path):
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    path = tmp_path / "Aigis-Sig-plus.yaml"
    yaml = YAML()
    doc = yaml.load(path.read_text())
    nd.mark_dnf(doc["versions"][0]["parametersets"][0], "signature", "SIGFPE")
    with open(path, "w") as f:
        yaml.dump(doc, f)
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    ps = load(path)["versions"][0]["parametersets"][0]
    assert (
        ps["notes"]
        == nd.ps_notes(STATUS["NGCC-A1"]) + " Benchmark did not finish (SIGFPE)."
    )
    assert ps["dnf"] == "SIGFPE"


def test_switching_ref_to_opt_drops_timings_measured_on_ref(tmp_path):
    """Timings (and a DNF) of the reference build never stay attached to an opt build."""
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    path = tmp_path / "Aigis-Sig-plus.yaml"
    yaml = YAML()
    doc = yaml.load(path.read_text())
    a1, a2 = doc["versions"][0]["parametersets"][:2]
    a1["signing_cycles"], a1["verification_us"] = 1000, 0.4
    a2["signing_cycles"] = 2000
    nd.mark_dnf(a1, "signature", "SIGFPE")
    with open(path, "w") as f:
        yaml.dump(doc, f)
    status = {**STATUS, "NGCC-A1": {**STATUS["NGCC-A1"], "impl": "opt"}}
    nd.write_files(CANDS, "signature", SIG_SIZES, status, tmp_path)
    a1, a2 = load(path)["versions"][0]["parametersets"][:2]
    assert not {"signing_cycles", "verification_us", "dnf"} & a1.keys()
    assert a1["notes"] == nd.ps_notes(status["NGCC-A1"])
    assert a1["notes"].startswith("Optimised x86 implementation")
    assert a2["signing_cycles"] == 2000  # impl unchanged: kept


def test_impl_changed_needs_a_known_old_and_new_impl():
    ref = {"notes": "Reference implementation."}
    assert nd.impl_changed(ref, {"impl": "opt"})
    assert not nd.impl_changed(ref, {"impl": "ref"})
    assert not nd.impl_changed(
        {"notes": "Sizes from the specification."}, {"impl": "opt"}
    )
    assert not nd.impl_changed(None, {"impl": "opt"})
    assert not nd.impl_changed(ref, None)


def test_patched_build_is_noted_neutrally():
    note = nd.ps_notes({"impl": "ref", "kat": "PASS", "hash": "own", "patch": "fix"})
    assert note == (
        "Reference implementation; our build applies a small patch to the submitted code."
    )
    assert "patch" not in nd.ps_notes(
        {"impl": "ref", "kat": "PASS", "hash": "own", "patch": ""}
    )


def test_untimed_instance_keeps_sizes_and_notes_it_is_not_benchmarked(tmp_path):
    timed = {"signing_cycles": 5, "verification_cycles": 6}
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    path = tmp_path / "Aigis-Sig-plus.yaml"
    doc = load(path)
    for ps in doc["versions"][0]["parametersets"][:2]:
        ps.update(timed)
    yaml = YAML()
    with open(path, "w") as f:
        yaml.dump(doc, f)
    untimed = nd.load_untimed(_params(tmp_path, "NGCC-A1", "skip", "NGCC-A2", "10"))
    assert untimed == {"NGCC-A1"}
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path, untimed)
    a1, a2, _ = load(path)["versions"][0]["parametersets"]
    assert a1["pk"] == 928 and "signing_cycles" not in a1
    assert (
        nd.UNTIMED_NOTE in a1["notes"] and nd.ps_notes(STATUS["NGCC-A1"]) in a1["notes"]
    )
    assert a2["signing_cycles"] == 5 and nd.UNTIMED_NOTE not in a2["notes"]
    assert a1["dnf"] == "timeout" and "dnf" not in a2


def _params(tmp_path, *pairs):
    rows = zip(pairs[::2], pairs[1::2], strict=True)
    p = tmp_path / "params.tsv"
    p.write_text(
        "# c\nfile\tname\titers\n" + "".join(f"f\t{n}\t{i}\n" for n, i in rows)
    )
    return p


@pytest.mark.parametrize(
    "kat", ["MISMATCH", "TIMEOUT", "NOKAT", "CRYPTOFAIL", "CRASH", "LOADFAIL", "ODD"]
)
def test_non_pass_kat_status_gets_a_visible_info_flag(kat):
    status = {"NGCC-A1": {"impl": "ref", "kat": kat, "hash": "own", "build": "ok"}}
    inst = CANDS[0]["instances"][0]
    ps = nd.parameterset(inst, "signature", SIG_SIZES, status, None)
    assert ps["info"] == nd.KAT_INFO.get(kat, nd.KAT_INFO_DEFAULT)
    assert ps["info"].endswith(".")


@pytest.mark.parametrize("kat", ["PASS", "-", ""])
def test_pass_or_unbuilt_kat_status_has_no_info_flag(kat):
    status = {"NGCC-A1": {"impl": "ref", "kat": kat, "hash": "own", "build": "ok"}}
    ps = nd.parameterset(CANDS[0]["instances"][0], "signature", SIG_SIZES, status, None)
    assert "info" not in ps


def test_kat_info_is_combined_with_a_curated_info_text():
    inst = dict(CANDS[0]["instances"][1], info="Curated note.")
    ps = nd.parameterset(inst, "signature", SIG_SIZES, STATUS, None)
    assert ps["info"] == "Curated note. " + nd.KAT_INFO["MISMATCH"]
    assert "info" not in nd.parameterset(
        CANDS[0]["instances"][0], "signature", SIG_SIZES, STATUS, None
    )


def test_written_files_flag_mismatched_sets(tmp_path):
    nd.write_files(CANDS, "signature", SIG_SIZES, STATUS, tmp_path)
    sets = {
        p["name"]: p
        for p in load(tmp_path / "Aigis-Sig-plus.yaml")["versions"][0]["parametersets"]
    }
    assert sets["A2"]["info"] == nd.KAT_INFO["MISMATCH"]
    assert "info" not in sets["A1"]


def test_perf_source_is_ours_only_when_every_timed_set_is():
    timing = nd.TIMING["signature"]
    ours = {
        "name": "a",
        "signing_cycles": 1,
        "verification_cycles": 1,
        "bench_env": "e",
    }
    theirs = {"name": "b", "signing_us": 2.0}
    sizes_only = {"name": "c", "pk": 1}

    v = {"perf_source": "submission document", "parametersets": [ours, sizes_only]}
    nd.set_perf_source(v, timing)
    assert v["perf_source"] == nd.OUR_BENCHMARK

    v = {"perf_source": "submission document", "parametersets": [ours, theirs]}
    nd.set_perf_source(v, timing)
    assert v["perf_source"] == "submission document"

    v = {"parametersets": [sizes_only]}
    nd.set_perf_source(v, timing)
    assert "perf_source" not in v


def test_listed_instance_without_params_row_is_marked_no_code(tmp_path):
    # A3 has spec sizes and no params.tsv row: no code in the package. A2 has a row whose
    # build failed (no sizes): code exists, so it is not no_code.
    sizes = {"NGCC-A1": SIG_SIZES["NGCC-A1"]}
    status = {**STATUS, "NGCC-A2": {"build": "FAIL", "kat": "-"}}
    cands = [dict(CANDS[0])]
    cands[0]["instances"] = [
        *CANDS[0]["instances"][:3],
        {**CANDS[0]["instances"][3]},
        {
            "instance": "A4",
            "ngcc_level": 384,
            "ref": "T1",
            "pk": 1,
            "sig": 2,
            "size_ref": "T5",
            "no_code": "Specified but not submitted",
        },
    ]
    cands[0]["instances"][1] = {
        **cands[0]["instances"][1],
        "pk": 3,
        "sig": 4,
        "size_ref": "T6",
    }
    rows = nd.load_rows(_params(tmp_path, "NGCC-A1", "10", "NGCC-A2", "10"))
    assert rows == {"NGCC-A1", "NGCC-A2"}
    nd.write_files(cands, "signature", sizes, status, tmp_path, rows=rows)
    a1, a2, a3, a4 = load(tmp_path / "Aigis-Sig-plus.yaml")["versions"][0][
        "parametersets"
    ]
    assert "no_code" not in a1 and "no_code" not in a2
    assert "our build of this instance fails" in a2["notes"]
    assert a3["no_code"] == nd.NO_CODE
    assert "has no implementation" in a3["notes"]
    assert a4["no_code"] == "Specified but not submitted"
    # Without a params.tsv (rows None) nothing is marked.
    nd.write_files(cands, "signature", sizes, status, tmp_path)
    sets = load(tmp_path / "Aigis-Sig-plus.yaml")["versions"][0]["parametersets"]
    assert not any("no_code" in p for p in sets)
    assert nd.load_rows(tmp_path / "missing.tsv") is None
