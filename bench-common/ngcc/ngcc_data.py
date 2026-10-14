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
import re
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
# The visible `info` flag ("note" badge) for any instance whose KAT status is not PASS,
# so timings from such a build are never shown without a caveat. Neutral and factual.
KAT_INFO = {
    "MISMATCH": "Our build does not reproduce the submitted test vectors.",
    "TIMEOUT": "Our build could not check the submitted test vectors in time.",
    "NOKAT": "No submitted test vectors for this instance.",
    "CRYPTOFAIL": "The submitted code failed its own test-vector run in our build.",
    "CRASH": "The test-vector run crashed in our build.",
    "LOADFAIL": "Our build could not check the submitted test vectors.",
}
KAT_INFO_DEFAULT = "The submitted test vectors were not checked against our build."

# status.tsv `patch` column set (status.py): a patched copy of submitted code is built.
# params.tsv `iters` = skip: the instance is built and measured for sizes/KAT only.
# It is skipped because it exceeds our time limits, so its `dnf` reason is "timeout".
UNTIMED_NOTE = "Not benchmarked: a single run takes hours."
UNTIMED_DNF = "timeout"

PATCH_NOTE = "our build applies a small patch to the submitted code"


# A benchmark run that ended in a DNF row (crash, timeout, failed warm-up check)
# leaves this sentence at the end of the parameter set's notes and the one-word
# reason in its `dnf` field (shown in the timing cells); see mark_dnf().
DNF_NOTE = "Benchmark did not finish"
_DNF_RE = re.compile(r"\s*" + DNF_NOTE + r" \([^)]*\)\.")


def _dnf_sentence(notes) -> str:
    m = _DNF_RE.search(str(notes or ""))
    return m.group(0).strip() if m else ""


def clear_dnf(ps) -> None:
    """Remove a DNF sentence and `dnf` field from a parameter set (dropping empty notes)."""
    ps.pop("dnf", None)
    notes = _DNF_RE.sub("", str(ps.get("notes") or "")).strip()
    if notes:
        ps["notes"] = notes
    else:
        ps.pop("notes", None)


def mark_dnf(ps, kind: str, reason: str) -> None:
    """Record a DNF benchmark row: drop the timings, note why it did not finish."""
    for field in TIMING[kind]:
        ps.pop(field, None)
    clear_dnf(ps)
    sentence = f"{DNF_NOTE} ({reason.replace('-', ' ')})."
    ps["notes"] = f"{ps['notes']} {sentence}" if ps.get("notes") else sentence
    ps["dnf"] = reason


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


def load_untimed(path: Path) -> set[str]:
    """Bench names of params.tsv rows with iters `skip` (built, sizes and KAT only)."""
    if not path.exists():
        return set()
    return {r["name"] for r in read_tsv(path) if r.get("iters") == "skip"}


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
    parts = [IMPL_NOTE["opt" if status.get("impl") == "opt" else "ref"]]
    if status.get("hash") in HASH_NOTE:
        parts.append(HASH_NOTE[status["hash"]])
    if status.get("kat") in KAT_NOTE:
        parts.append(KAT_NOTE[status["kat"]])
    if status.get("patch"):
        parts.append(PATCH_NOTE)
    return "; ".join(parts) + "."


def kat_info(status: dict | None) -> str:
    """The `info` text for a non-PASS KAT status.

    "" for PASS, no status row, or a failed build (kat `-`: nothing measured or timed).
    """
    kat = (status or {}).get("kat")
    if kat in (None, "", "-", "PASS"):
        return ""
    return KAT_INFO.get(kat, KAT_INFO_DEFAULT)


def spec_differences(inst: dict, measured: dict, key: str) -> str:
    """Spec values (pk/sk/size) that disagree with our measured ones, e.g. 'pk 405 B, sk 488 B'."""
    return ", ".join(
        f"{field} {inst[field]} B"
        for field in ("pk", "sk", key)
        if field in inst and inst[field] != measured[field]
    )


IMPL_NOTE = {"opt": "Optimised x86 implementation", "ref": "Reference implementation"}


def _old_impl(old: dict | None) -> str | None:
    """The implementation (opt/ref) the existing YAML entry was built from, read from its notes."""
    notes = str((old or {}).get("notes") or "")
    return next((k for k, v in IMPL_NOTE.items() if notes.startswith(v)), None)


def impl_changed(old: dict | None, status: dict | None) -> bool:
    """True if the existing entry came from the other build (ref vs opt) than status.tsv now names.

    Its timings (and any DNF) were measured on that other build, so they are not carried over:
    the instance shows 'not yet benchmarked' until the next benchmark import.
    """
    was, now = _old_impl(old), (status or {}).get("impl")
    return was is not None and now in IMPL_NOTE and was != now


def parameterset(
    inst: dict,
    kind: str,
    sizes: dict,
    status: dict,
    old: dict | None,
    untimed: frozenset | set = frozenset(),
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
    if impl_changed(old, status.get(name)):
        old = None  # measured on the other build: drop its timings and DNF
    if name in untimed:
        note = f"{note} {UNTIMED_NOTE}".strip()
    else:
        for field in TIMING[kind]:
            if old is not None and field in old:
                ps[field] = old[field]
    dnf = _dnf_sentence(old.get("notes")) if old is not None else ""
    if dnf:
        note = f"{note} {dnf}".strip()
    if note:
        ps["notes"] = note
    info = " ".join(t for t in (inst.get("info"), kat_info(status.get(name))) if t)
    if info:
        ps["info"] = info
    for flag in ("implementation", "broken"):
        if inst.get(flag):
            ps[flag] = inst[flag]
    # Last, where mark_dnf() also puts it, so imports and --ngcc-sizes agree on key order.
    if name in untimed:
        ps["dnf"] = UNTIMED_DNF
    elif dnf and old.get("dnf"):
        ps["dnf"] = old["dnf"]
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
    cand: dict, kind: str, sizes: dict, status: dict, old=None, untimed=frozenset()
) -> CommentedMap:
    previous = {str(p["name"]): p for p in _old_sets(old, kind)}
    sets = [
        parameterset(i, kind, sizes, status, previous.get(i["instance"]), untimed)
        for i in listed(cand)
    ]
    doc = CommentedMap()
    doc["name"] = cand["name"]
    if cand.get("full_name"):  # the submission's full title; "name" is what tables show
        doc["full_name"] = cand["full_name"]
    for field in ("website", "category", "assumption"):
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
    findings = cand.get("findings")
    target = doc["versions"][0] if kind == "signature" else doc
    if findings:
        target["findings_url"] = findings["url"]
        for flag in ("implementation", "broken"):
            if findings.get(flag):
                target[flag] = findings[flag]
    return doc


def write_files(
    cands: list[dict],
    kind: str,
    sizes: dict,
    status: dict,
    out_dir: Path,
    untimed=frozenset(),
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
        plans.append((path, scheme_doc(cand, kind, sizes, status, old, untimed)))
    for path, doc in plans:
        with open(path, "w", encoding="utf-8") as f:
            yaml.dump(doc, f)
    return [p for p, _ in plans]
