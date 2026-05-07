#!/usr/bin/env python3
# /// script
# dependencies = ["pyyaml"]
# ///
"""Add round-1 version entries from round-1/data/ CSVs into data/schemes/ YAML files.

For schemes already in data/schemes/ (round-2 survivors): prepend a round-1
version entry with round-1 paramsets and version info.
For round-1-only schemes: create new YAML files.
Skips reference/classical schemes (EdDSA, RSA, Falcon, ML-DSA, SLH-DSA).
"""

import csv
import re
import yaml
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).parent.parent
R1_DATA = ROOT / "round-1" / "data"
SCHEMES_DIR = ROOT / "data" / "schemes"

# ── round-1 version strings from spec PDFs ──────────────────────────────────
R1_VERSION = {
    # schemes that survived to round-2
    "CROSS":    ("Round 1", "2023-06-02"),
    "FAEST":    ("v1.0",    "2023-06-01"),
    "HAWK":     ("v1.0",    "2023-06-01"),
    "LESS":     ("Round 1", "2023-06-01"),
    "MAYO":     ("Round 1", "2023-06-01"),
    "MQOM":     ("v1.0",    "2023-05-31"),
    "PERK":     ("v1.0",    "2023-05-31"),
    "QR-UOV":   ("Round 1", "2023-06-01"),
    "RYDE":     ("Round 1", "2023-06-01"),
    "SDitH":    ("v1.0",    "2023-05-31"),
    "SNOVA":    ("Round 1", "2023-05-25"),
    "SQIsign":  ("v1.0",    "2023-06-01"),
    "UOV":      ("v1.0",    "2023-05-30"),
    # round-1-only (non-broken)
    "AIMer":         ("v1.0",    "2023-06-01"),
    "ALTEQ":         ("Round 1", "2023-06-01"),
    "Ascon-Sign":    ("Round 1", "2023-06-01"),
    "Biscuit":       ("Round 1", "2023-06-01"),
    "FuLeeca":       ("Round 1", "2023-05-31"),
    "HAETAE":        ("Round 1", "2023-05-31"),
    "HuFu":          ("Round 1", "2023-06-01"),
    "MEDS":          ("v1.0.1",  "2023-06-30"),
    "MIRA":          ("Round 1", "2023-06-01"),
    "MiRitH":        ("Round 1", "2023-06-01"),
    "PREON":         ("Round 1", "2023-05-31"),
    "PROV":          ("v1.0",    "2023-06-01"),
    "Raccoon":       ("Round 1", "2023-06-01"),
    "SPHINCS-alpha": ("Round 1", "2023-05-31"),
    "Squirrels":     ("Round 1", "2023-06-02"),
    "TUOV":          ("v1.0",    "2023-05-30"),
    "VOX":           ("v1.0",    "2023-06-01"),
    "Wave":          ("v1",      "2023-06-01"),
    # broken schemes
    "3WISE":            ("Round 1", "2023-06-01"),
    "DME-Sign":         ("Round 1", "2023-06-01"),
    "EagleSign":        ("Round 1", "2023-06-01"),
    "EHTv3 / EHTv4":    ("Round 1", "2023-06-01"),
    "eMLE-Sig 2.0":     ("Round 1", "2023-06-01"),
    "Enhanced pqsigRM": ("Round 1", "2023-06-01"),
    "HPPC":             ("Round 1", "2023-06-01"),
    "KAZ-Sign":         ("Round 1", "2023-06-01"),
    "Xifrat1-Sign.I":   ("Round 1", "2023-06-01"),
}

# Not additional-signature submissions — skip entirely
SKIP = {"EdDSA", "RSA", "Falcon", "ML-DSA (Dilithium)", "SLH-DSA (SPHINCS+)"}

# CSV name → YAML scheme name (for schemes that changed name round-1→round-2)
CSV_TO_YAML_NAME = {
    "QR-UOV": "QR-UOV",  # same, just confirming
}


def sanitize(name: str) -> str:
    return re.sub(r"_+", "_", name.replace("/", "_").replace(" ", "_").replace("(", "").replace(")", ""))


def parse_num(s: str):
    s = s.replace(",", "").strip()
    if not s:
        return None
    try:
        f = float(s)
        return int(f) if f == int(f) else f
    except ValueError:
        return None


# ── Load round-1 CSVs ────────────────────────────────────────────────────────
with open(R1_DATA / "schemes.csv") as f:
    r1_schemes = {r["Scheme"]: r for r in csv.DictReader(f)}

with open(R1_DATA / "parametersets.csv") as f:
    r1_paramsets: dict[str, list] = defaultdict(list)
    for r in csv.DictReader(f):
        r1_paramsets[r["Scheme"]].append(r)


def build_paramsets(csv_rows: list) -> list:
    result = []
    for ps in csv_rows:
        level_raw = ps["Security level"]
        level = level_raw if level_raw == "Pre-Quantum" else int(level_raw)

        pk = parse_num(ps["pk size"])
        sig = parse_num(ps["sig size"])
        sign_cyc = parse_num(ps["signing (cycles)"])
        verif_cyc = parse_num(ps["verification (cycles)"])
        sign_ms = parse_num(ps["signing (ms)"])
        verif_ms = parse_num(ps["verification (ms)"])

        entry: dict = {"name": ps["Parameterset"], "level": level, "pk": pk, "sig": sig}
        if sign_cyc:
            entry["signing_cycles"] = sign_cyc
            entry["verification_cycles"] = verif_cyc
        elif sign_ms is not None:
            entry["signing_ms"] = float(sign_ms)
            entry["verification_ms"] = float(verif_ms) if verif_ms is not None else None
        result.append(entry)
    return result


def build_version(csv_scheme: dict, paramsets: list, version: str, date: str) -> dict:
    v: dict = {
        "version": version,
        "date": date,
        "status": csv_scheme["NIST status"],
        "tags": ["round-1"],
        "parametersets": paramsets,
    }
    if csv_scheme.get("Broken"):
        v["broken"] = csv_scheme["Broken"]
    if csv_scheme.get("Warning"):
        v["warning"] = csv_scheme["Warning"]
    if csv_scheme.get("Info"):
        v["info"] = csv_scheme["Info"]
    return v


# ── Existing YAML files (round-2 survivors) ──────────────────────────────────
existing_yaml_names = {p.stem: p for p in SCHEMES_DIR.glob("*.yaml")}

for csv_name, csv_scheme in r1_schemes.items():
    if csv_name in SKIP:
        continue

    yaml_name = CSV_TO_YAML_NAME.get(csv_name, csv_name)
    yaml_file = SCHEMES_DIR / f"{yaml_name}.yaml"

    if not yaml_file.exists():
        continue  # handled below for new files

    version, date = R1_VERSION.get(csv_name, ("Round 1", "2023-06-01"))
    paramsets = build_paramsets(r1_paramsets[csv_name])
    if not paramsets:
        print(f"SKIP {csv_name}: no round-1 paramsets")
        continue

    r1_version = build_version(csv_scheme, paramsets, version, date)

    with open(yaml_file) as f:
        doc = yaml.safe_load(f)

    # Append round-1 version (date sorting handles ordering at runtime)
    doc["versions"].append(r1_version)

    with open(yaml_file, "w") as f:
        yaml.dump(doc, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
    print(f"Updated (added round-1): {yaml_file.name}")


# ── New YAML files (round-1-only schemes) ────────────────────────────────────
for csv_name, csv_scheme in r1_schemes.items():
    if csv_name in SKIP:
        continue

    yaml_name = CSV_TO_YAML_NAME.get(csv_name, csv_name)
    yaml_file = SCHEMES_DIR / f"{yaml_name}.yaml"

    if yaml_file.exists():
        continue  # already handled above

    version, date = R1_VERSION.get(csv_name, ("Round 1", "2023-06-01"))
    paramsets = build_paramsets(r1_paramsets[csv_name])

    r1_version = build_version(csv_scheme, paramsets, version, date)

    doc = {
        "name": csv_name,
        "website": csv_scheme["Website"],
        "category": csv_scheme["Category"],
        "assumption": csv_scheme["Assumption"],
        "versions": [r1_version],
    }

    filename = sanitize(csv_name) + ".yaml"
    out_path = SCHEMES_DIR / filename
    with open(out_path, "w") as f:
        yaml.dump(doc, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
    print(f"Created (round-1 only): {filename}")
