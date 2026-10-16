"""CI check: every parameter set that should have our benchmark has it.

Runs over the real data/ and the importers' mappings, resolved with the importers' own
code (bench*/update_scheme_data.py: BENCH_TO_YAML, target_errors, mapped_sets) and, for
NGCC, the params.tsv rows (ngcc_data). One line per offending parameter set:

  rule 1  A set that a benchmark row maps to has our timings (all of the suite's timing
          fields and `bench_env`) or a `dnf:` reason (NGCC `iters skip` rows: `dnf: timeout`).
          A mapping counts for NGCC only if the instance has a params.tsv row.
  rule 2  Every set of a scheme still in a competition has a mapping, unless it carries
          `no_code: <reason>`. "Still in a competition": the scheme's newest version that
          names a programme lists one of that programme's current stages: its final
          stage in data/programmes.yaml (the last of its `stages`) or, for programmes in
          ACTIVE_FROM, that stage or any later one in `stages` order. Today: NIST on-ramp
          round-3, China NGCC round-1, KpqC selected, NIST PQC selected or standardised,
          ISO published. It applies to the scheme's
          newest version and, for ALL_ROUNDS programmes, to every version that names the
          programme. Eliminated schemes (CROSS, LESS, round-1-only on-ramp entrants, NIST
          PQC round-3 KEMs) are exempt; rule 1 still applies to any set they have mapped.
  rule 3  Every mapping resolves to a YAML parameter set (the importers refuse otherwise).

A failure means: benchmark the new rows (scripts/aws-bench-split) and import them, or mark
the set `no_code:` / fix the mapping. Never weaken the rules here.
"""

from __future__ import annotations

import importlib.util
import shutil
from dataclasses import dataclass
from pathlib import Path

import pytest
from ruamel.yaml import YAML

REPO = Path(__file__).resolve().parents[2]

# Programmes whose every round we benchmark from that round's own code while the scheme
# is still in it (Thom, 2026-10-15, decision B; bench/CLAUDE.md "Older on-ramp rounds").
ALL_ROUNDS = {"nist-onramp"}

# Programmes still "in" from an earlier stage than their last: NIST PQC from `selected`
# (Falcon/FN-DSA and HQC: selected, standard pending; Thom, 2026-10-16). Every stage from
# this one onwards in programmes.yaml order counts.
ACTIVE_FROM = {"nist-pqc": "selected"}


def load(suite):
    spec = importlib.util.spec_from_file_location(
        f"{suite.replace('-', '_')}_coverage", REPO / suite / "update_scheme_data.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


SIG = load("bench")
KEM = load("bench-kem")
NGCC = SIG.ngcc_data
SAFE = YAML(typ="safe")


@dataclass
class Data:
    schemes_dir: Path
    kems_dir: Path
    programmes: Path
    sig_map: dict
    kem_map: dict
    ngcc_rows: dict[str, set[str]]  # kind -> NGCC bench names with a params.tsv row


def real_data() -> Data:
    return Data(
        schemes_dir=REPO / "data" / "schemes",
        kems_dir=REPO / "data" / "kems",
        programmes=REPO / "data" / "programmes.yaml",
        sig_map=dict(SIG.BENCH_TO_YAML),
        kem_map=dict(KEM.BENCH_TO_YAML),
        ngcc_rows={
            "signature": NGCC.load_rows(REPO / "bench/schemes/ngcc/params.tsv")
            or set(),
            "kem": NGCC.load_rows(REPO / "bench-kem/schemes/ngcc/params.tsv") or set(),
        },
    )


def current_stages(programmes: Path) -> dict[str, set[str]]:
    """Programme id -> the stages a scheme is still in it at: the last of its `stages`,
    or every stage from its ACTIVE_FROM stage onwards."""
    out = {}
    for p in SAFE.load(programmes.read_text()):
        ids = [s["id"] for s in p["stages"]]
        out[p["id"]] = set(ids[ids.index(ACTIVE_FROM.get(p["id"], ids[-1])) :])
    return out


def stages(memberships, programme: str) -> set[str]:
    s = (memberships or {}).get(programme)
    return set() if not s else {s} if isinstance(s, str) else set(s)


def active(memberships: list, current: dict[str, set[str]]) -> set[str]:
    """Programmes a scheme is still in; memberships newest first (one per version)."""
    out = set()
    for programme, wanted in current.items():
        newest = next((m for m in memberships if programme in (m or {})), None)
        if newest is not None and wanted & stages(newest, programme):
            out.add(programme)
    return out


def benchmarked(mapping: dict, rows: set[str]) -> dict:
    """The mapping without NGCC instances that have no params.tsv row (no code to time)."""
    return {
        n: v for n, v in mapping.items() if not n.startswith(NGCC.PREFIX) or n in rows
    }


def check_set(file, version, ps, mapped, required, timing) -> list[str]:
    where = f"{file}: {version}: {ps.get('name')}"
    if mapped:
        timed = all(ps.get(f) is not None for f in timing) and ps.get("bench_env")
        if not timed and not ps.get("dnf"):
            return [
                f"{where}: rule 1: mapped, but no timings with bench_env and no dnf"
            ]
    elif required and not ps.get("no_code"):
        return [
            f"{where}: rule 2: still in a competition, but no mapping and no no_code"
        ]
    return []


def coverage_errors(d: Data) -> list[str]:
    errors = [f"rule 3: {e}" for e in SIG.target_errors(d.sig_map, d.schemes_dir)]
    errors += [f"rule 3: {e}" for e in KEM.target_errors(d.kem_map, d.kems_dir)]
    current = current_stages(d.programmes)

    sig_map = benchmarked(d.sig_map, d.ngcc_rows["signature"])
    for path in sorted(d.schemes_dir.glob("*.yaml")):
        doc = SAFE.load(path.read_text())
        versions = doc.get("versions") or []
        mapped = SIG.mapped_sets(doc, path.name, sig_map)
        programmes = active([v.get("programmes") for v in versions], current)
        required = {0} if programmes else set()
        for programme in programmes & ALL_ROUNDS:
            required |= {
                i
                for i, v in enumerate(versions)
                if stages(v.get("programmes"), programme)
            }
        for i, v in enumerate(versions):
            for ps in v.get("parametersets") or []:
                errors += check_set(
                    path.name,
                    v.get("version"),
                    ps,
                    (i, str(ps.get("name"))) in mapped,
                    i in required,
                    NGCC.TIMING["signature"],
                )

    kem_map = benchmarked(d.kem_map, d.ngcc_rows["kem"])
    kem_targets = set(kem_map.values())
    for path in sorted(d.kems_dir.glob("*.yaml")):
        doc = SAFE.load(path.read_text())
        required = bool(active([doc.get("programmes")], current))
        for ps in doc.get("parametersets") or []:
            errors += check_set(
                path.name,
                "-",
                ps,
                (path.name, str(ps.get("name"))) in kem_targets,
                required,
                NGCC.TIMING["kem"],
            )
    return errors


def test_every_set_that_should_have_our_benchmark_has_it():
    errors = coverage_errors(real_data())
    assert not errors, "\n".join(errors)


def test_current_stages_come_from_programmes_yaml():
    assert current_stages(REPO / "data" / "programmes.yaml") == {
        "nist-pqc": {"selected", "standardised"},
        "nist-onramp": {"round-3"},
        "china-ngcc": {"round-1"},
        "kpqc": {"selected"},
        "iso": {"published"},
    }


@pytest.mark.parametrize(
    "file,expected",
    [
        ("ML-DSA.yaml", {"nist-pqc"}),
        ("Falcon.yaml", {"nist-pqc"}),  # selected, standard pending
        ("MAYO.yaml", {"nist-onramp"}),
        ("BiT.yaml", {"china-ngcc"}),
        ("HAETAE.yaml", {"kpqc"}),  # newest kpqc membership (v3.0) is "selected"
        ("AIMer.yaml", {"kpqc"}),  # its on-ramp entry ended in round 1
        ("CROSS.yaml", set()),  # eliminated after round 2
        ("3WISE.yaml", set()),
        ("RSA.yaml", set()),  # pre-quantum reference: rule 1 only
    ],
)
def test_which_schemes_are_still_in_a_competition(file, expected):
    doc = SAFE.load((REPO / "data" / "schemes" / file).read_text())
    current = current_stages(REPO / "data" / "programmes.yaml")
    assert active([v.get("programmes") for v in doc["versions"]], current) == expected


# --- each rule fires on a copy of a few data files ----------------------------------

FILES = {
    "schemes": ["ML-DSA.yaml", "Falcon.yaml", "MAYO.yaml", "CROSS.yaml", "BiT.yaml"],
    "kems": ["ML-KEM.yaml", "HQC.yaml", "SABER.yaml"],
}


@pytest.fixture
def data(tmp_path) -> Data:
    real = real_data()
    for sub, files in FILES.items():
        (tmp_path / sub).mkdir()
        for f in files:
            shutil.copy(REPO / "data" / sub / f, tmp_path / sub / f)
    shutil.copy(real.programmes, tmp_path / "programmes.yaml")
    return Data(
        schemes_dir=tmp_path / "schemes",
        kems_dir=tmp_path / "kems",
        programmes=tmp_path / "programmes.yaml",
        sig_map={
            n: v
            for n, v in real.sig_map.items()
            if all(t[0] in FILES["schemes"] for t in SIG.targets(v))
        },
        kem_map={n: v for n, v in real.kem_map.items() if v[0] in FILES["kems"]},
        ngcc_rows=real.ngcc_rows,
    )


def edit(path: Path, change) -> None:
    doc = SAFE.load(path.read_text())
    change(doc)
    with open(path, "w") as f:
        YAML().dump(doc, f)


def sig_set(doc, version: str, name: str):
    v = next(v for v in doc["versions"] if v["version"] == version)
    return next(p for p in v["parametersets"] if p["name"] == name)


def test_copied_data_passes(data):
    assert coverage_errors(data) == []


def test_rule_1_needs_timings_with_bench_env_or_dnf(data):
    edit(
        data.schemes_dir / "ML-DSA.yaml",
        lambda d: sig_set(d, "FIPS 204", "ML-DSA-44").pop("bench_env"),
    )
    edit(
        data.kems_dir / "ML-KEM.yaml",
        lambda d: d["parametersets"][0].pop("decaps_us"),
    )
    # CROSS is eliminated, but its mapped sets still need timings.
    edit(
        data.schemes_dir / "CROSS.yaml",
        lambda d: sig_set(d, "v2.2", "R-SDP 1 fast").pop("signing_cycles"),
    )
    assert coverage_errors(data) == [
        "CROSS.yaml: v2.2: R-SDP 1 fast: rule 1: mapped, but no timings with bench_env and no dnf",
        "ML-DSA.yaml: FIPS 204: ML-DSA-44: rule 1: mapped, but no timings with bench_env and no dnf",
        "ML-KEM.yaml: -: ML-KEM-512: rule 1: mapped, but no timings with bench_env and no dnf",
    ]

    def dnf(d):
        ps = sig_set(d, "FIPS 204", "ML-DSA-44")
        ps["dnf"] = "timeout"

    edit(data.schemes_dir / "ML-DSA.yaml", dnf)
    assert len(coverage_errors(data)) == 2


def test_rule_1_ngcc_rows_need_timings_but_no_code_sets_do_not(data):
    edit(
        data.schemes_dir / "BiT.yaml",
        lambda d: sig_set(d, "Round 1", "BiT-128").pop("signing_us"),
    )
    # BiT-384 has no params.tsv row (no code), so it is not mapped.
    assert "NGCC-BiT-384" not in data.ngcc_rows["signature"]
    assert coverage_errors(data) == [
        "BiT.yaml: Round 1: BiT-128: rule 1: mapped, but no timings with bench_env and no dnf"
    ]


def test_rule_2_sets_of_active_schemes_need_a_mapping_or_no_code(data):
    new = {"name": "new-set", "level": 1, "pk": 1, "sig": 1}

    def add(version):
        return lambda d: next(v for v in d["versions"] if v["version"] == version)[
            "parametersets"
        ].append(dict(new))

    edit(data.schemes_dir / "MAYO.yaml", add("Round 1"))  # on-ramp: every round
    edit(data.schemes_dir / "ML-DSA.yaml", add("FIPS 204"))  # standardised: newest
    edit(
        data.schemes_dir / "BiT.yaml",
        lambda d: sig_set(d, "Round 1", "BiT-384").pop("no_code"),
    )
    edit(data.schemes_dir / "Falcon.yaml", add("initial"))  # selected: still in
    edit(data.schemes_dir / "CROSS.yaml", add("v2.2"))  # eliminated: exempt
    edit(data.kems_dir / "HQC.yaml", lambda d: d["parametersets"].append(dict(new)))
    edit(data.kems_dir / "ML-KEM.yaml", lambda d: d["parametersets"].append(dict(new)))
    edit(data.kems_dir / "SABER.yaml", lambda d: d["parametersets"].append(dict(new)))
    msg = "rule 2: still in a competition, but no mapping and no no_code"
    assert coverage_errors(data) == [
        f"BiT.yaml: Round 1: BiT-384: {msg}",
        f"Falcon.yaml: initial: new-set: {msg}",
        f"MAYO.yaml: Round 1: new-set: {msg}",
        f"ML-DSA.yaml: FIPS 204: new-set: {msg}",
        f"HQC.yaml: -: new-set: {msg}",
        f"ML-KEM.yaml: -: new-set: {msg}",
    ]

    def no_code(d):
        for v in d["versions"]:
            for p in v["parametersets"]:
                if p["name"] == "new-set":
                    p["no_code"] = "No implementation"

    edit(data.schemes_dir / "MAYO.yaml", no_code)
    assert len(coverage_errors(data)) == 5


def test_rule_3_mappings_must_resolve(data):
    data.sig_map["MAYO-R1-six"] = ("MAYO.yaml", "six", "Round 1")
    data.sig_map["MAYO-R9-one"] = ("MAYO.yaml", "one", "Round 9")
    data.sig_map["Gone-1"] = ("Gone.yaml", "1")
    data.kem_map["ML-KEM-2048"] = ("ML-KEM.yaml", "ML-KEM-2048")
    assert coverage_errors(data) == [
        "rule 3: Gone-1: Gone.yaml does not exist",
        "rule 3: MAYO-R1-six: MAYO.yaml has no parameter set 'six' in version 'Round 1'",
        "rule 3: MAYO-R9-one: MAYO.yaml: version 'Round 9' matches 0 versions",
        "rule 3: ML-KEM-2048: ML-KEM.yaml has no parameter set 'ML-KEM-2048'",
    ]
