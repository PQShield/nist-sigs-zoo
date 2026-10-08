"""Turn NGCC sizes + build status + curated claims into data/ YAML.

Shared by bench/update_scheme_data.py (signatures) and bench-kem/update_scheme_data.py
(KEMs). Inputs:
  data/ngcc/candidates.yaml         curated from the official specs (Task 8)
  <suite>/results/ngcc_sizes.tsv    `./bench --sizes NGCC` / `./bench-kem --sizes NGCC`
  <suite>/schemes/ngcc/status.tsv   status.py: build, KAT, hash usage, impl
Re-running keeps timing fields that a benchmark import already wrote.
"""

from __future__ import annotations

import csv
from pathlib import Path

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap

PREFIX = "NGCC-"
PROGRAMME, STAGE = "china-ngcc", "round-1"
ROUND1_DATE = "2026-09-20"
STATUS = "NGCC round 1"
SIZE_KEY = {"signature": "sig", "kem": "ct"}
TIMING = {
    "signature": (
        "signing_cycles",
        "verification_cycles",
        "signing_us",
        "verification_us",
    ),
    "kem": (
        "keygen_cycles",
        "encaps_cycles",
        "decaps_cycles",
        "keygen_us",
        "encaps_us",
        "decaps_us",
    ),
}
HASH_NOTE = {
    "placeholder": "uses the ICCS placeholder hash/XOF (pseudohash/pseudoXOF) from the submission package; any timings include it",
    "sm3": "uses the ICCS SM3 implementation from the submission package",
}
KAT_NOTE = {
    "MISMATCH": "our build does not reproduce the submitted test vectors (cause unknown)",
    "NOKAT": "no submitted test vectors found for this instance",
    "CRYPTOFAIL": "the submitted code failed its own test-vector run in our build",
    "CRASH": "the test-vector run crashed in our build",
    "TIMEOUT": "the test-vector run timed out in our build",
    "LOADFAIL": "the built library is missing ICCS API symbols",
}


class NgccDataError(ValueError):
    pass


def bench_name(instance: str) -> str:
    return PREFIX + instance


def read_tsv(path: Path) -> list[dict]:
    with open(path, newline="", encoding="utf-8") as f:
        body = [ln for ln in f if not ln.startswith("#")]
    return [
        {k: (v or "").strip() for k, v in r.items()}
        for r in csv.DictReader(body, delimiter="\t")
    ]


def load_candidates(path: Path) -> list[dict]:
    return YAML(typ="safe").load(path.read_text(encoding="utf-8"))


def load_sizes(path: Path) -> dict[str, dict[str, int]]:
    return {
        r["scheme"]: {k: int(v) for k, v in r.items() if k != "scheme"}
        for r in read_tsv(path)
    }


def load_status(path: Path) -> dict[str, dict]:
    return {r["name"]: r for r in read_tsv(path)} if path.exists() else {}


def of_kind(cands: list[dict], kind: str) -> list[dict]:
    return [c for c in cands if c["kind"] == kind]


def listed(cand: dict) -> list[dict]:
    return [i for i in cand["instances"] if "omit" not in i]


def bench_to_yaml(cands: list[dict], kind: str) -> dict[str, tuple[str, str]]:
    return {
        bench_name(i["instance"]): (f"{c['file']}.yaml", i["instance"])
        for c in of_kind(cands, kind)
        for i in listed(c)
    }


def check_coverage(cands: list[dict], kind: str, sizes: dict) -> None:
    claimed = {
        bench_name(i["instance"]) for c in of_kind(cands, kind) for i in c["instances"]
    }
    problems = [
        f"built but not in candidates.yaml: {n}" for n in sorted(set(sizes) - claimed)
    ]
    problems += [
        f"no sizes (not built and no spec sizes): {bench_name(i['instance'])}"
        for c in of_kind(cands, kind)
        for i in listed(c)
        if bench_name(i["instance"]) not in sizes and SIZE_KEY[kind] not in i
    ]
    if problems:
        raise NgccDataError("\n".join(problems))


def ps_notes(status: dict | None) -> str:
    if not status:
        return ""
    parts = [
        "Optimised x86 implementation"
        if status.get("impl") == "opt"
        else "Reference implementation"
    ]
    if status.get("hash") in HASH_NOTE:
        parts.append(HASH_NOTE[status["hash"]])
    if status.get("kat") in KAT_NOTE:
        parts.append(KAT_NOTE[status["kat"]])
    return "; ".join(parts) + "."


def spec_differences(inst: dict, measured: dict, key: str) -> str:
    """Spec values (pk/sk/size) that disagree with our measured ones, e.g. 'pk 405 B, sk 488 B'."""
    return ", ".join(
        f"{field} {inst[field]} B"
        for field in ("pk", "sk", key)
        if field in inst and inst[field] != measured[field]
    )


def parameterset(
    inst: dict, kind: str, sizes: dict, status: dict, old: dict | None
) -> CommentedMap:
    name, key = bench_name(inst["instance"]), SIZE_KEY[kind]
    ps = CommentedMap()
    ps["name"] = inst["instance"]
    ps["ngcc_level"] = inst["ngcc_level"]
    if inst.get("recommended"):
        ps["recommended"] = True
    if name in sizes:
        ps["pk"], ps[key] = sizes[name]["pk"], sizes[name][key]
        note = ps_notes(status.get(name))
        diff = spec_differences(inst, sizes[name], key)
        if diff:
            note = f"{note} Measured sizes differ from the specification, which gives {diff} ({inst['size_ref']}).".strip()
    else:
        ps["pk"], ps[key] = inst["pk"], inst[key]
        failed = (status.get(name) or {}).get("build") == "FAIL"
        why = (
            "our build of this instance fails"
            if failed
            else "we did not build this instance"
        )
        note = (
            f"Sizes from the specification ({inst['size_ref']}); not measured, {why}."
        )
    for field in TIMING[kind]:
        if old is not None and field in old:
            ps[field] = old[field]
    if note:
        ps["notes"] = note
    return ps


def _old_sets(old, kind: str) -> list:
    if old is None:
        return []
    if kind == "kem":
        return old.get("parametersets") or []
    version = next(
        (
            v
            for v in old.get("versions") or []
            if PROGRAMME in (v.get("programmes") or {})
        ),
        None,
    )
    return (version or {}).get("parametersets") or []


def _is_ngcc(old, kind: str) -> bool:
    if kind == "kem":
        return PROGRAMME in (old.get("programmes") or {})
    return any(
        PROGRAMME in (v.get("programmes") or {}) for v in old.get("versions") or []
    )


def scheme_doc(
    cand: dict, kind: str, sizes: dict, status: dict, old=None
) -> CommentedMap:
    previous = {str(p["name"]): p for p in _old_sets(old, kind)}
    sets = [
        parameterset(i, kind, sizes, status, previous.get(i["instance"]))
        for i in listed(cand)
    ]
    doc = CommentedMap()
    for field in ("name", "website", "category", "assumption"):
        doc[field] = cand[field]
    programmes = CommentedMap([(PROGRAMME, STAGE)])
    if kind == "signature":
        version = CommentedMap()
        version["version"] = "Round 1"
        version["date"] = ROUND1_DATE
        version["status"] = STATUS
        version["programmes"] = programmes
        version["parametersets"] = sets
        doc["versions"] = [version]
    else:
        doc["status"] = STATUS
        doc["programmes"] = programmes
        doc["parametersets"] = sets
    return doc


def write_files(
    cands: list[dict], kind: str, sizes: dict, status: dict, out_dir: Path
) -> list[Path]:
    check_coverage(cands, kind, sizes)
    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096
    plans = []
    for cand in of_kind(cands, kind):
        path = out_dir / f"{cand['file']}.yaml"
        old = yaml.load(path.read_text(encoding="utf-8")) if path.exists() else None
        if old is not None and not _is_ngcc(old, kind):
            raise NgccDataError(f"{path.name} exists and is not an NGCC file")
        plans.append((path, scheme_doc(cand, kind, sizes, status, old)))
    for path, doc in plans:
        with open(path, "w", encoding="utf-8") as f:
            yaml.dump(doc, f)
    return [p for p, _ in plans]
