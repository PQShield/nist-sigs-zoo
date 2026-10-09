import ngcc_data as nd
import pytest
from ruamel.yaml import YAML

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
            "ct: 896", "ct: 896\n  keygen_cycles: 1000\n  keygen_us: 0.4"
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
    assert ps["notes"] == (
        "Reference implementation. Benchmark did not finish (valid signature rejected)."
    )
    nd.mark_dnf(ps, "kem", "SIGABRT")  # a newer DNF replaces the old reason
    assert (
        ps["notes"] == "Reference implementation. Benchmark did not finish (SIGABRT)."
    )
    nd.clear_dnf(ps)
    assert ps["notes"] == "Reference implementation."
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


def test_patched_build_is_noted_neutrally():
    note = nd.ps_notes({"impl": "ref", "kat": "PASS", "hash": "own", "patch": "fix"})
    assert note == (
        "Reference implementation; our build applies a small patch to the submitted code."
    )
    assert "patch" not in nd.ps_notes(
        {"impl": "ref", "kat": "PASS", "hash": "own", "patch": ""}
    )
