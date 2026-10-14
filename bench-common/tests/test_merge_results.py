"""bench-common/merge_results.py: merging the results files of a split cloud run."""

import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SCRIPT = REPO / "bench-common" / "merge_results.py"

COLUMNS = """\
scheme                       iters    keygen (cyc)  keygen (us)      sign (cyc)   sign (us)    verify (cyc)  verify (us)
------                       -----    ------------  -----------      ----------   ---------    ------------  ----------
"""


def part(host, date, rows, *, cpu="Test CPU", extra=""):
    return (
        "# pq-sigs benchmark results\n"
        f"{extra}"
        f"# date:         {date}\n"
        f"# host:         {host}\n"
        f"# cpu:          {cpu}\n"
        "# filter:       some filter\n"
        "# cyclecounter: rdpmc\n" + COLUMNS + rows
    )


ROW_A = "NGCC-A                          10             100         1.0             200         2.0             300         3.0\n"
ROW_B = "RSA-2048 (PSS)                  10             100         1.0             200         2.0             300         3.0\n"
DNF_C = "NGCC-C                          10             DNF         DNF             DNF         DNF             DNF         DNF  SIGSEGV\n"
ROW_C = "NGCC-C                          10             110         1.1             210         2.1             310         3.1\n"


def merge(tmp_path, *texts):
    paths = []
    for i, text in enumerate(texts):
        p = tmp_path / f"part{i}.txt"
        p.write_text(text)
        paths.append(str(p))
    out = tmp_path / "merged.txt"
    proc = subprocess.run(
        [sys.executable, "-I", str(SCRIPT), str(out), *paths],
        capture_output=True,
        text=True,
        check=False,
    )
    return proc, out


def test_merges_rows_once_keeping_the_first_good_row(tmp_path):
    first = part("h1", "2026-01-01", ROW_A + DNF_C + "toy chatter on stdout 1 2 3\n")
    second = part("h2", "2026-01-02", ROW_B + ROW_C + ROW_A.replace("100", "999"))
    proc, out = merge(tmp_path, first, second)
    assert proc.returncode == 0, proc.stderr
    text = out.read_text()
    lines = text.splitlines()
    rows = lines[next(i for i, ln in enumerate(lines) if ln.startswith("------")) + 1 :]
    assert rows == [ROW_A.rstrip("\n"), ROW_C.rstrip("\n"), ROW_B.rstrip("\n")]
    assert "toy chatter" not in text
    assert "# merged:       2 parts" in text


def test_part_notes_move_into_their_part_lines_and_filters_are_dropped(tmp_path):
    note = "# note:         recovered from a log; rows after X are missing\n"
    proc, out = merge(
        tmp_path,
        part("h1", "2026-01-01", ROW_A, extra=note),
        part("h2", "2026-01-02", ROW_B),
    )
    assert proc.returncode == 0, proc.stderr
    header = [ln for ln in out.read_text().splitlines() if ln.startswith("#")]
    assert not any(ln.startswith(("# note:", "# filter:")) for ln in header)
    parts = [ln for ln in header if ln.startswith("# part:")]
    assert parts == [
        "# part:         part0.txt (host h1, 2026-01-01, 1 rows; "
        "note: recovered from a log; rows after X are missing)",
        "# part:         part1.txt (host h2, 2026-01-02, 1 rows)",
    ]
    # The part list sits just before the cycle-counter line.
    assert header.index("# cyclecounter: rdpmc") == header.index(parts[-1]) + 1


def test_refuses_parts_from_different_cpus(tmp_path):
    proc, out = merge(
        tmp_path,
        part("h1", "d", ROW_A),
        part("h2", "d", ROW_B, cpu="Other CPU"),
    )
    assert proc.returncode != 0 and "differs" in proc.stderr
    assert not out.exists()
