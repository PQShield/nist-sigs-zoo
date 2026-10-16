"""Every NGCC instance that builds must carry an explicit, measured iteration count.

iters 0 means the harness default (1000 iterations), whose cost is unknown for slow
instances; cloud runs have a hard TTL, so params.tsv sets iters for every buildable row (`skip` = built, never timed).
"""

import csv
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


def rows(path):
    lines = [ln for ln in path.read_text().splitlines() if not ln.startswith("#")]
    return list(csv.DictReader(lines, delimiter="\t"))


@pytest.mark.parametrize("suite", ["bench", "bench-kem"])
def test_buildable_rows_have_explicit_iters(suite):
    d = ROOT / suite / "schemes" / "ngcc"
    iters = {
        r["name"]: -1 if r["iters"] == "skip" else int(r["iters"])
        for r in rows(d / "params.tsv")
    }
    built = [r["name"] for r in rows(d / "status.tsv") if r["build"] == "ok"]
    assert built
    assert [n for n in built if iters[n] == 0] == []
