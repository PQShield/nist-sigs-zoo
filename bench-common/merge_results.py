#!/usr/bin/env python3
"""Merge the results files of a split cloud run into one file for update_scheme_data.py.

Usage: merge_results.py OUT PART [PART ...]

A split run times disjoint groups of schemes on separate instances of the same type
(scripts/aws-bench.sh with filters, one invocation per group). The merged file keeps the
first part's header, records every part's file, host, date and CPU in "# part:" lines,
and lists each scheme once, in order of first appearance. A scheme timed by more than one
part keeps its first non-DNF row. Parts on different CPU models are refused.
"""

import re
import sys
from pathlib import Path


def read_part(path: Path) -> tuple[list[str], list[str], list[str], dict[str, str]]:
    """Return (header lines, column header lines, data rows, header metadata)."""
    header: list[str] = []
    columns: list[str] = []
    rows: list[str] = []
    meta: dict[str, str] = {}
    for line in path.read_text().splitlines():
        if line.startswith(("scheme", "------")):
            columns.append(line)
        elif columns:
            # Keep result rows (name, iters, ...); drop output a candidate printed to stdout.
            fields = re.split(r"\s{2,}", line.strip())
            if len(fields) > 2 and fields[1].isdigit():
                rows.append(line)
        else:
            header.append(line)
            if m := re.match(r"^#\s+([\w_]+):\s*(.*)", line):
                meta.setdefault(m.group(1), m.group(2).strip())
    return header, columns, rows, meta


def is_dnf(row: str) -> bool:
    return " DNF " in f" {row} "


def main() -> None:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    out = Path(sys.argv[1])
    parts = [Path(p) for p in sys.argv[2:]]

    merged: dict[str, str] = {}
    first_header: list[str] | None = None
    first_columns: list[str] = []
    part_lines: list[str] = []
    cpu = None
    for path in parts:
        header, columns, rows, meta = read_part(path)
        if cpu is None:
            cpu = meta.get("cpu")
        elif meta.get("cpu") != cpu:
            sys.exit(f"{path}: cpu {meta.get('cpu')!r} differs from {cpu!r}")
        if first_header is None:
            first_header, first_columns = header, columns
        part_lines.append(
            f"# part:         {path.name} (host {meta.get('host', '?')}, {meta.get('date', '?')}, {len(rows)} rows)"
        )
        for row in rows:
            # Names may contain single spaces ("RSA-2048 (PSS)"); columns are padded.
            name = re.split(r"\s{2,}", row.strip())[0]
            if name not in merged or (is_dnf(merged[name]) and not is_dnf(row)):
                merged[name] = row

    assert first_header is not None
    # Insert the part list just before the cycle-counter line (or at the end of the header).
    idx = next(
        (i for i, l in enumerate(first_header) if l.startswith("# cyclecounter:")),
        len(first_header),
    )
    note = [
        f"# merged:       {len(parts)} parts of a split run on separate instances of the same type"
    ]
    header = first_header[:idx] + note + part_lines + first_header[idx:]
    # Each part ran a subset; its filter is listed per part, not for the whole file.
    header = [h for h in header if not h.startswith("# filter:")]
    out.write_text("\n".join(header + first_columns + list(merged.values())) + "\n")
    print(f"wrote {out}: {len(merged)} rows from {len(parts)} parts")


if __name__ == "__main__":
    main()
