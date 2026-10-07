"""Consistency of the committed NGCC manifests, status files and size files."""

import csv
from pathlib import Path

import pytest

from gen_rules import read_params

REPO = Path(__file__).resolve().parents[3]
SUITES = {"sig": (REPO / "bench", 34, "sign"), "kem": (REPO / "bench-kem", 41, "kem")}
POPULATED = ["sig", "kem"]
KAT = {"PASS", "MISMATCH", "NOKAT", "CRYPTOFAIL", "LOADFAIL", "CRASH", "TIMEOUT", "-"}


def tsv(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(
            csv.DictReader([ln for ln in f if not ln.startswith("#")], delimiter="\t")
        )


@pytest.mark.parametrize("kind", POPULATED)
def test_manifest_covers_every_candidate(kind):
    suite, n, prefix = SUITES[kind]
    rows = read_params(suite / "schemes/ngcc/params.tsv")
    assert sorted({r["id"] for r in rows}) == [
        f"{prefix}-{i:02d}" for i in range(1, n + 1)
    ]
    instances = [(r["id"], r["instance"]) for r in rows]
    assert len(instances) == len(set(instances)), "one row per instance"


@pytest.mark.parametrize("kind", POPULATED)
def test_status_matches_manifest(kind):
    suite = SUITES[kind][0]
    rows = read_params(suite / "schemes/ngcc/params.tsv")
    status = tsv(suite / "schemes/ngcc/status.tsv")
    assert [s["name"] for s in status] == [r["name"] for r in rows]
    assert {s["build"] for s in status} <= {"ok", "FAIL"}
    assert {s["kat"] for s in status} <= KAT
    assert {s["hash"] for s in status} <= {"placeholder", "sm3", "own"}
    assert all((s["build"] == "FAIL") == (s["kat"] == "-") for s in status)


@pytest.mark.parametrize("kind", POPULATED)
def test_sizes_cover_exactly_the_built_instances(kind):
    suite = SUITES[kind][0]
    built = {
        s["name"] for s in tsv(suite / "schemes/ngcc/status.tsv") if s["build"] == "ok"
    }
    sizes = tsv(suite / "results/ngcc_sizes.tsv")
    assert {r["scheme"] for r in sizes} == built
    assert all(int(r["pk"]) > 0 for r in sizes)
