#!/usr/bin/env python3
# /// script
# dependencies = ["pyyaml"]
# ///
"""Migrate static/data/{schemes,parametersets}.csv → data/schemes/*.yaml"""

import csv
import yaml
from pathlib import Path
from collections import defaultdict

base = Path(__file__).parent.parent / "static" / "data"
out_dir = Path(__file__).parent.parent / "data" / "schemes"
out_dir.mkdir(parents=True, exist_ok=True)


def parse_int(s: str) -> int | None:
    try:
        v = float(s.replace(",", ""))
        return int(v) if v else None
    except (ValueError, AttributeError):
        return None


def parse_float(s: str) -> float | None:
    try:
        return float(s) if s else None
    except (ValueError, AttributeError):
        return None


with open(base / "schemes.csv") as f:
    schemes = {r["Scheme"]: r for r in csv.DictReader(f)}

with open(base / "parametersets.csv") as f:
    paramsets: dict[str, list] = defaultdict(list)
    for r in csv.DictReader(f):
        paramsets[r["Scheme"]].append(r)

for scheme_name, scheme in schemes.items():
    filename = scheme_name.replace("/", "_").replace(" ", "_") + ".yaml"

    ps_list = []
    for ps in paramsets[scheme_name]:
        level_raw = ps["Security level"]
        level: int | str = level_raw if level_raw == "Pre-Quantum" else int(level_raw)

        ps_data: dict = {
            "name": ps["Parameterset"],
            "level": level,
            "pk": parse_int(ps["pk size"]),
            "sig": parse_int(ps["sig size"]),
        }

        sign_cyc = parse_int(ps["signing (cycles)"])
        verif_cyc = parse_int(ps["verification (cycles)"])
        sign_ms = parse_float(ps["signing (ms)"])
        verif_ms = parse_float(ps["verification (ms)"])

        if sign_cyc:
            ps_data["signing_cycles"] = sign_cyc
            ps_data["verification_cycles"] = verif_cyc
        elif sign_ms is not None:
            ps_data["signing_ms"] = sign_ms
            ps_data["verification_ms"] = verif_ms

        ps_list.append(ps_data)

    version: dict = {
        "version": "initial",
        # TODO: replace with actual submission/update dates
        "date": "2024-10-01",
        "status": scheme["NIST status"],
        "parametersets": ps_list,
    }
    if scheme["Broken"]:
        version["broken"] = scheme["Broken"]
    if scheme["Warning"]:
        version["warning"] = scheme["Warning"]
    if scheme["Info"]:
        version["info"] = scheme["Info"]

    doc = {
        "name": scheme_name,
        "website": scheme["Website"],
        "category": scheme["Category"],
        "assumption": scheme["Assumption"],
        "versions": [version],
    }

    with open(out_dir / filename, "w") as f:
        yaml.dump(doc, f, default_flow_style=False, allow_unicode=True, sort_keys=False)

    print(f"Written: {filename}")

print(f"\nDone. {len(schemes)} scheme files written to {out_dir}")
