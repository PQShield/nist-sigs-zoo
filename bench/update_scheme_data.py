#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Parse a bench results file and update data/schemes/*.yaml with measured cycles.

Also writes data/benchmark_env.yaml describing the benchmark environment.

Usage:
    uv run bench/update_scheme_data.py [results_file]
    uv run bench/update_scheme_data.py --ngcc-sizes bench/results/ngcc_sizes.tsv

If no results_file is given, uses the most recent file in bench/results/.
"""

import re
import sys
from pathlib import Path

from ruamel.yaml import YAML

REPO_ROOT = Path(__file__).parent.parent
SCHEMES_DIR = REPO_ROOT / "data" / "schemes"
BENCH_ENV_FILE = REPO_ROOT / "data" / "benchmark_env.yaml"

# Map bench output name → (yaml_filename, parameterset_name_in_yaml)
# Only entries where the bench measurement corresponds to the YAML parameterset.
BENCH_TO_YAML: dict[str, tuple[str, str]] = {
    # ML-DSA (FIPS 204)
    "ML-DSA-44": ("ML-DSA.yaml", "ML-DSA-44"),
    "ML-DSA-65": ("ML-DSA.yaml", "ML-DSA-65"),
    "ML-DSA-87": ("ML-DSA.yaml", "ML-DSA-87"),
    # SLH-DSA (FIPS 205) — only SHAKE variants present in YAML
    "SLH-DSA-SHAKE-128s": ("SLH-DSA.yaml", "SHAKE-128s"),
    "SLH-DSA-SHAKE-128f": ("SLH-DSA.yaml", "SHAKE-128f"),
    "SLH-DSA-SHAKE-192s": ("SLH-DSA.yaml", "SHAKE-192s"),
    "SLH-DSA-SHAKE-192f": ("SLH-DSA.yaml", "SHAKE-192f"),
    "SLH-DSA-SHAKE-256s": ("SLH-DSA.yaml", "SHAKE-256s"),
    "SLH-DSA-SHAKE-256f": ("SLH-DSA.yaml", "SHAKE-256f"),
    # FN-DSA / Falcon
    "FN-DSA-512": ("Falcon.yaml", "512"),
    "FN-DSA-1024": ("Falcon.yaml", "1024"),
    # HAWK
    "HAWK-512": ("HAWK.yaml", "512"),
    "HAWK-1024": ("HAWK.yaml", "1024"),
    # MAYO
    "MAYO-one": ("MAYO.yaml", "one"),
    "MAYO-two": ("MAYO.yaml", "two"),  # round 3: MAYO-two re-parameterised
    "MAYO-two (broken)": ("MAYO.yaml", "two"),  # round-2 results files
    "MAYO-three": ("MAYO.yaml", "three"),
    "MAYO-five": ("MAYO.yaml", "five"),
    # FAEST
    "FAEST-128s": ("FAEST.yaml", "128s"),
    "FAEST-128f": ("FAEST.yaml", "128f"),
    "FAEST-EM-128s": ("FAEST.yaml", "EM-128s"),
    "FAEST-EM-128f": ("FAEST.yaml", "EM-128f"),
    "FAEST-192s": ("FAEST.yaml", "192s"),
    "FAEST-192f": ("FAEST.yaml", "192f"),
    "FAEST-EM-192s": ("FAEST.yaml", "EM-192s"),
    "FAEST-EM-192f": ("FAEST.yaml", "EM-192f"),
    "FAEST-256s": ("FAEST.yaml", "256s"),
    "FAEST-256f": ("FAEST.yaml", "256f"),
    "FAEST-EM-256s": ("FAEST.yaml", "EM-256s"),
    "FAEST-EM-256f": ("FAEST.yaml", "EM-256f"),
    # SDitH — bench implements v1.0 hypercube/threshold parameters
    "SDitH-Hypercube-L1-GF256": ("SDitH.yaml", "gf256-L1-hyp"),
    "SDitH-Hypercube-L1-P251": ("SDitH.yaml", "gf251-L1-hyp"),
    "SDitH-Hypercube-L3-GF256": ("SDitH.yaml", "gf256-L3-hyp"),
    "SDitH-Hypercube-L3-P251": ("SDitH.yaml", "gf251-L3-hyp"),
    "SDitH-Hypercube-L5-GF256": ("SDitH.yaml", "gf256-L5-hyp"),
    "SDitH-Hypercube-L5-P251": ("SDitH.yaml", "gf251-L5-hyp"),
    "SDitH-Threshold-L1-GF256": ("SDitH.yaml", "gf256-L1-thr"),
    "SDitH-Threshold-L1-P251": ("SDitH.yaml", "gf251-L1-thr"),
    "SDitH-Threshold-L3-GF256": ("SDitH.yaml", "gf256-L3-thr"),
    "SDitH-Threshold-L3-P251": ("SDitH.yaml", "gf251-L3-thr"),
    "SDitH-Threshold-L5-GF256": ("SDitH.yaml", "gf256-L5-thr"),
    "SDitH-Threshold-L5-P251": ("SDitH.yaml", "gf251-L5-thr"),
    # SNOVA v3.0 (round 3) — bench output name matches YAML parameterset name
    "SNOVA_I_K": ("SNOVA.yaml", "SNOVA_I_K"),
    "SNOVA_I_B": ("SNOVA.yaml", "SNOVA_I_B"),
    "SNOVA_I_S": ("SNOVA.yaml", "SNOVA_I_S"),
    "SNOVA_III_K": ("SNOVA.yaml", "SNOVA_III_K"),
    "SNOVA_III_B": ("SNOVA.yaml", "SNOVA_III_B"),
    "SNOVA_III_S": ("SNOVA.yaml", "SNOVA_III_S"),
    "SNOVA_V_K": ("SNOVA.yaml", "SNOVA_V_K"),
    "SNOVA_V_B": ("SNOVA.yaml", "SNOVA_V_B"),
    "SNOVA_V_S": ("SNOVA.yaml", "SNOVA_V_S"),
    # SNOVA v3.0 alternative parameter sets (spec Table 6); odd-q sets use the reference implementation
    "SNOVA (27 3 16 4 7 5 84)": ("SNOVA.yaml", "SNOVA (27 3 16 4 7 5 84)"),
    "SNOVA (27 4 16 4 5 5 80)": ("SNOVA.yaml", "SNOVA (27 4 16 4 5 5 80)"),
    "SNOVA (27 4 19 4 6 5 96)": ("SNOVA.yaml", "SNOVA (27 4 19 4 6 5 96)"),
    "SNOVA (27 5 13 4 4 5 80)": ("SNOVA.yaml", "SNOVA (27 5 13 4 4 5 80)"),
    "SNOVA (50 17 16 2 2 17 68)": ("SNOVA.yaml", "SNOVA (50 17 16 2 2 17 68)"),
    "SNOVA (27 4 16 5 5 4 100)": ("SNOVA.yaml", "SNOVA (27 4 16 5 5 4 100)"),
    "SNOVA (33 5 16 4 5 6 100)": ("SNOVA.yaml", "SNOVA (33 5 16 4 5 6 100)"),
    "SNOVA (39 5 11 4 6 7 120)": ("SNOVA.yaml", "SNOVA (39 5 11 4 6 7 120)"),
    "SNOVA (38 5 19 4 6 7 120)": ("SNOVA.yaml", "SNOVA (38 5 19 4 6 7 120)"),
    "SNOVA (76 25 16 2 2 25 100)": ("SNOVA.yaml", "SNOVA (76 25 16 2 2 25 100)"),
    "SNOVA (40 5 19 5 6 6 150)": ("SNOVA.yaml", "SNOVA (40 5 19 5 6 6 150)"),
    "SNOVA (47 6 19 4 6 9 144)": ("SNOVA.yaml", "SNOVA (47 6 19 4 6 9 144)"),
    "SNOVA (104 33 16 2 2 33 132)": ("SNOVA.yaml", "SNOVA (104 33 16 2 2 33 132)"),
    # UOV
    "UOV-Ip-pkc": ("UOV.yaml", "Ip-pkc"),
    "UOV-Ip-classic": ("UOV.yaml", "Ip-classic"),
    "UOV-Is-pkc": ("UOV.yaml", "Is-pkc"),
    "UOV-Is-classic": ("UOV.yaml", "Is-classic"),
    "UOV-III-pkc": ("UOV.yaml", "III-pkc"),
    "UOV-III-classic": ("UOV.yaml", "III-classic"),
    "UOV-V-pkc": ("UOV.yaml", "V-pkc"),
    "UOV-V-classic": ("UOV.yaml", "V-classic"),
    # QR-UOV Round 3 — bench uses commas, YAML uses spaces inside parens.
    # Round 3 dropped the q=31/q=7 sets, so only the six q=127 sets are built;
    # the Round 2 version entry keeps its own (Round 2 implementation) numbers.
    "QR-UOV-I-(127,156,54,3)": ("QR-UOV.yaml", "I-(127 156 54 3)"),
    "QR-UOV-I-(127,540,60,10)": ("QR-UOV.yaml", "I-(127 540 60 10)"),
    "QR-UOV-III-(127,228,78,3)": ("QR-UOV.yaml", "III-(127 228 78 3)"),
    "QR-UOV-III-(127,820,90,10)": ("QR-UOV.yaml", "III-(127 820 90 10)"),
    "QR-UOV-V-(127,306,105,3)": ("QR-UOV.yaml", "V-(127 306 105 3)"),
    "QR-UOV-V-(127,1040,110,10)": ("QR-UOV.yaml", "V-(127 1040 110 10)"),
    # MQOM v3.0 (round 3) — bench MQOM3- prefix dropped in YAML
    "MQOM3-L1-gf2-shorter-ct": ("MQOM.yaml", "L1-gf2-shorter-ct"),
    "MQOM3-L1-gf2-shorter-ot": ("MQOM.yaml", "L1-gf2-shorter-ot"),
    "MQOM3-L1-gf16-short-ct": ("MQOM.yaml", "L1-gf16-short-ct"),
    "MQOM3-L1-gf16-short-ot": ("MQOM.yaml", "L1-gf16-short-ot"),
    "MQOM3-L1-gf16-fast-ct": ("MQOM.yaml", "L1-gf16-fast-ct"),
    "MQOM3-L1-gf16-fast-ot": ("MQOM.yaml", "L1-gf16-fast-ot"),
    "MQOM3-L3-gf2-shorter-ct": ("MQOM.yaml", "L3-gf2-shorter-ct"),
    "MQOM3-L3-gf2-shorter-ot": ("MQOM.yaml", "L3-gf2-shorter-ot"),
    "MQOM3-L3-gf16-short-ct": ("MQOM.yaml", "L3-gf16-short-ct"),
    "MQOM3-L3-gf16-short-ot": ("MQOM.yaml", "L3-gf16-short-ot"),
    "MQOM3-L3-gf16-fast-ct": ("MQOM.yaml", "L3-gf16-fast-ct"),
    "MQOM3-L3-gf16-fast-ot": ("MQOM.yaml", "L3-gf16-fast-ot"),
    "MQOM3-L5-gf2-shorter-ct": ("MQOM.yaml", "L5-gf2-shorter-ct"),
    "MQOM3-L5-gf2-shorter-ot": ("MQOM.yaml", "L5-gf2-shorter-ot"),
    "MQOM3-L5-gf16-short-ct": ("MQOM.yaml", "L5-gf16-short-ct"),
    "MQOM3-L5-gf16-short-ot": ("MQOM.yaml", "L5-gf16-short-ot"),
    "MQOM3-L5-gf16-fast-ct": ("MQOM.yaml", "L5-gf16-fast-ct"),
    "MQOM3-L5-gf16-fast-ot": ("MQOM.yaml", "L5-gf16-fast-ot"),
    # SDitH2 — gf2 variants (v3.0.0 first in YAML, so it gets the numbers) (bench output name matches shim's @NAME@)
    "SDitH2-L1-gf2-short": ("SDitH.yaml", "SDitH2-L1-gf2-short"),
    "SDitH2-L1-gf2-fast": ("SDitH.yaml", "SDitH2-L1-gf2-fast"),
    "SDitH2-L3-gf2-short": ("SDitH.yaml", "SDitH2-L3-gf2-short"),
    "SDitH2-L3-gf2-fast": ("SDitH.yaml", "SDitH2-L3-gf2-fast"),
    "SDitH2-L5-gf2-short": ("SDitH.yaml", "SDitH2-L5-gf2-short"),
    "SDitH2-L5-gf2-fast": ("SDitH.yaml", "SDitH2-L5-gf2-fast"),
    # SQIsign
    "SQIsign-I": ("SQIsign.yaml", "I"),
    "SQIsign-III": ("SQIsign.yaml", "III"),
    "SQIsign-V": ("SQIsign.yaml", "V"),
    # Classic
    "RSA-2048 (PSS)": ("RSA.yaml", "2048"),
    "RSA-3072 (PSS)": ("RSA.yaml", "3072"),
    "RSA-4096 (PSS)": ("RSA.yaml", "4096"),
    "ECDSA-P256 (SHA-256)": ("ECDSA.yaml", "P-256"),
    "ECDSA-P384 (SHA-384)": ("ECDSA.yaml", "P-384"),
    "ECDSA-P521 (SHA-512)": ("ECDSA.yaml", "P-521"),
    "Ed25519": ("EdDSA.yaml", "Ed25519"),
    "Ed448": ("EdDSA.yaml", "Ed448"),
}

# China NGCC round-1 instances: bench names are NGCC-<instance>. The mapping comes
# from data/ngcc/candidates.yaml (see bench-common/ngcc/ngcc_data.py).
sys.path.insert(0, str(REPO_ROOT / "bench-common" / "ngcc"))
import ngcc_data

NGCC_CANDIDATES = REPO_ROOT / "data" / "ngcc" / "candidates.yaml"
NGCC_STATUS = REPO_ROOT / "bench" / "schemes" / "ngcc" / "status.tsv"
if NGCC_CANDIDATES.exists():
    BENCH_TO_YAML.update(
        ngcc_data.bench_to_yaml(ngcc_data.load_candidates(NGCC_CANDIDATES), "signature")
    )


def import_ngcc_sizes(sizes_file: Path) -> None:
    """Create or refresh the NGCC signature YAML files from `--sizes NGCC` output."""
    cands = ngcc_data.load_candidates(NGCC_CANDIDATES)
    try:
        written = ngcc_data.write_files(
            cands,
            "signature",
            ngcc_data.load_sizes(sizes_file),
            ngcc_data.load_status(NGCC_STATUS),
            SCHEMES_DIR,
            ngcc_data.load_untimed(NGCC_STATUS.with_name("params.tsv")),
        )
    except ngcc_data.NgccDataError as e:
        sys.exit(f"NGCC import failed:\n{e}")
    print(
        f"Wrote {len(written)} NGCC signature files in {SCHEMES_DIR.relative_to(REPO_ROOT)}"
    )


# Results rows: "<name> <iters> kg_cyc kg_us sign_cyc sign_us verify_cyc verify_us",
# or a DNF row (scheme crashed, timed out or failed its warm-up check) whose six
# timing columns read DNF, followed by a one-word reason. The name may contain
# spaces. Anything else (stderr lines merged in by run_bench.sh) is skipped.
_NUM = r"(\d+)\s+(\d+\.\d+)"  # cycles, then µs printed as %.1f
ROW_RE = re.compile(
    rf"^(?P<name>\S.*?)\s+(?P<iters>\d+)\s+{_NUM}\s+{_NUM}\s+{_NUM}\s*$"
)
DNF_RE = re.compile(
    r"^(?P<name>\S.*?)\s+(?P<iters>\d+|-)(?:\s+DNF){6}\s+(?P<reason>\S+)\s*$"
)


def parse_results_file(path: Path) -> tuple[dict, list[dict]]:
    """Return (env_meta, rows). rows: dicts with name/iters/dnf and, unless dnf, the timings."""
    meta: dict[str, str] = {}
    rows: list[dict] = []
    in_data = False

    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# "):
                # header metadata lines: "# key: value"
                src_m = re.match(r"^#\s+schemes/(\w+)(?:/ref)?:\s+(https?://\S+)", line)
                if src_m:
                    meta.setdefault("_sources", {})[src_m.group(1)] = src_m.group(2)
                else:
                    m = re.match(r"^#\s+([\w_]+):\s*(.*)", line)
                    if m:
                        meta[m.group(1)] = m.group(2).strip()
            elif line.startswith("scheme") or line.startswith("------"):
                in_data = True
            elif in_data and (m := DNF_RE.match(line)):
                iters = m["iters"]
                rows.append(
                    {
                        "name": m["name"],
                        "iters": int(iters) if iters.isdigit() else None,
                        "dnf": m["reason"],
                    }
                )
            elif in_data and (m := ROW_RE.match(line)):
                _kg_cyc, _kg_us, sign_cyc, sign_us, verify_cyc, verify_us = m.groups()[
                    2:
                ]
                rows.append(
                    {
                        "name": m["name"],
                        "iters": int(m["iters"]),
                        "dnf": None,
                        "keygen_cycles": int(_kg_cyc),
                        "signing_cycles": int(sign_cyc),
                        "verification_cycles": int(verify_cyc),
                        "signing_us": float(sign_us),
                        "verification_us": float(verify_us),
                    }
                )
    return meta, rows


def apply_row(yaml_data, ps_name: str, row: dict) -> tuple[bool, str | None]:
    """Apply one results row to the first version containing ps_name.

    Returns (changed, message); message is None if ps_name was not found.
    A DNF row on an NGCC parameter set removes its timings and notes why; on any
    other scheme it only warns, so a crash never overwrites measured timings.
    """
    ps = next(
        (
            p
            for version in yaml_data.get("versions", [])
            for p in version.get("parametersets", [])
            if str(p.get("name", "")) == ps_name
        ),
        None,
    )
    if ps is None:
        return False, None
    ngcc = row["name"].startswith(ngcc_data.PREFIX)
    if row["dnf"]:
        if ngcc:
            ngcc_data.mark_dnf(ps, "signature", row["dnf"])
            return True, f"DNF ({row['dnf']}): timings removed, noted"
        return False, f"WARNING: DNF ({row['dnf']}); existing timings kept"
    for field in (
        "signing_cycles",
        "verification_cycles",
        "signing_us",
        "verification_us",
    ):
        ps[field] = row[field]
    if ngcc:
        ngcc_data.clear_dnf(ps)
    return True, (
        f"sign={row['signing_cycles']:>12,} ({row['signing_us']} µs)  "
        f"verify={row['verification_cycles']:>12,} ({row['verification_us']} µs)"
    )


def write_benchmark_env(meta: dict, path: Path) -> None:
    """Write data/benchmark_env.yaml from parsed header metadata."""
    yaml = YAML()
    yaml.default_flow_style = False
    yaml.width = 4096

    from ruamel.yaml.comments import CommentedMap

    env: CommentedMap = CommentedMap()
    env.yaml_set_start_comment(
        "Licensed under CC BY 4.0: https://creativecommons.org/licenses/by/4.0/"
    )
    env["license"] = "CC-BY-4.0"
    env["attribution"] = "Thom Wiggers / PQShield"
    env["date"] = meta.get("date", "")

    cpu = CommentedMap()
    cpu["model"] = meta.get("cpu", "")
    cores_str = meta.get("cores", "")
    cores_m = re.match(r"(\d+)\s+\(threads/core:\s+(\d+)\)", cores_str)
    if cores_m:
        cpu["cores"] = int(cores_m.group(1))
        cpu["threads_per_core"] = int(cores_m.group(2))
    freq_str = meta.get("max_freq_mhz", "")
    freq_m = re.match(r"(\d+)", freq_str)
    if freq_m:
        cpu["max_freq_mhz"] = int(freq_m.group(1))
    cpu["governor"] = meta.get("governor", "")
    cpu["turbo"] = meta.get("turbo", "")
    env["cpu"] = cpu

    env["os"] = meta.get("os", "")
    env["kernel"] = meta.get("kernel", "")
    env["compiler"] = meta.get("compiler", "")
    env["openssl"] = meta.get("openssl", "")
    counter = meta.get(
        "cyclecounter", "rdtsc + lfence (constant TSC rate, not execution cycles)"
    )
    env["notes"] = (
        "Median over 1000 iterations (fewer for slow schemes). "
        f"Cycle counter: {counter}. "
        "Wall clock: clock_gettime(CLOCK_MONOTONIC). "
        "Benchmark thread pinned to one core."
    )

    # Merge with the sources already recorded: a filtered run (e.g. only NGCC)
    # must not drop the provenance of the schemes it did not run.
    sources: dict = {}
    if path.exists():
        previous = YAML(typ="safe").load(path.read_text()) or {}
        sources.update(previous.get("sources") or {})
        prev_cpu = (previous.get("cpu") or {}).get("model")
        if prev_cpu and prev_cpu != meta.get("cpu", ""):
            print(
                f"WARNING: {path.name} was measured on {prev_cpu!r}, "
                f"this run on {meta.get('cpu')!r}"
            )
    sources.update(meta.get("_sources", {}))
    if sources:
        src_map = CommentedMap()
        for k, v in sorted(sources.items()):
            src_map[k] = v
        env["sources"] = src_map

    with open(path, "w") as f:
        yaml.dump(env, f)

    print(f"Wrote {path.relative_to(REPO_ROOT)}")


def main() -> None:
    if len(sys.argv) == 3 and sys.argv[1] == "--ngcc-sizes":
        import_ngcc_sizes(Path(sys.argv[2]))
        return
    results_dir = REPO_ROOT / "bench" / "results"

    if len(sys.argv) > 1:
        results_file = Path(sys.argv[1])
    else:
        files = sorted(results_dir.glob("*.txt"))
        if not files:
            sys.exit("No results files found in bench/results/")
        results_file = files[-1]
        print(f"Using {results_file.name}")

    meta, rows = parse_results_file(results_file)
    print(f"Parsed {len(rows)} benchmark rows")

    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096

    updated = 0
    skipped = 0
    not_mapped = []

    # Group updates by yaml file to minimise file reads/writes
    file_updates: dict[str, list[tuple[str, dict]]] = {}
    for row in rows:
        name = row["name"]
        if name not in BENCH_TO_YAML:
            not_mapped.append(name)
            continue
        yaml_file, ps_name = BENCH_TO_YAML[name]
        file_updates.setdefault(yaml_file, []).append((ps_name, row))

    for yaml_filename, updates in sorted(file_updates.items()):
        yaml_path = SCHEMES_DIR / yaml_filename
        if not yaml_path.exists():
            print(f"  MISSING: {yaml_filename}")
            skipped += len(updates)
            continue

        with open(yaml_path) as f:
            data = yaml.load(f)

        changed = False
        for ps_name, row in updates:
            ps_changed, msg = apply_row(data, ps_name, row)
            if msg is None:
                print(f"  NOT FOUND: {yaml_filename}  '{ps_name}'")
                skipped += 1
                continue
            print(f"  {yaml_filename}  {ps_name}: {msg}")
            if ps_changed:
                updated += 1
                changed = True
            else:
                skipped += 1

        if changed:
            with open(yaml_path, "w") as f:
                yaml.dump(data, f)

    print(f"\nUpdated {updated} parametersets, skipped {skipped}")
    if not_mapped:
        print(f"\nNo YAML mapping for {len(not_mapped)} bench names:")
        for n in not_mapped:
            print(f"  {n}")

    # Report what was NOT updated
    run_bench_names = {row["name"] for row in rows}
    yaml_to_bench: dict[tuple[str, str], str] = {
        (v[0], v[1]): k for k, v in BENCH_TO_YAML.items()
    }

    # Bench-mapped parametersets absent from this run's results
    not_in_run: dict[str, list[str]] = {}
    for bench_name, (yaml_file, ps_name) in sorted(BENCH_TO_YAML.items()):
        if bench_name not in run_bench_names:
            not_in_run.setdefault(yaml_file, []).append(ps_name)

    # Parametersets in YAML files with no bench mapping — grouped by round
    # round_order defines display order; keys match the labels used below
    ROUND_ORDER = ["round-2", "round-1", "untagged"]
    no_bench: dict[str, dict[str, list[str]]] = {r: {} for r in ROUND_ORDER}
    for yaml_path in sorted(SCHEMES_DIR.glob("*.yaml")):
        with open(yaml_path) as f:
            data = yaml.load(f)
        for version in data.get("versions") or []:
            tags = version.get("tags") or []
            if "round-2" in tags or "round-3" in tags:
                round_key = "round-2"
            elif "round-1" in tags:
                round_key = "round-1"
            else:
                round_key = "untagged"
            for ps in version.get("parametersets") or []:
                key = (yaml_path.name, str(ps.get("name", "")))
                if key not in yaml_to_bench:
                    no_bench[round_key].setdefault(yaml_path.name, []).append(
                        str(ps.get("name", ""))
                    )

    if not_in_run:
        total = sum(len(v) for v in not_in_run.values())
        print(f"\nBench-mapped but not in this run ({total}):")
        for yaml_file, names in sorted(not_in_run.items()):
            print(f"  {yaml_file}: {', '.join(names)}")

    total_no_bench = sum(
        len(names) for r in ROUND_ORDER for names in no_bench[r].values()
    )
    if total_no_bench:
        print(f"\nNo bench mapping ({total_no_bench}):")
        labels = {"round-2": "Round 2", "round-1": "Round 1", "untagged": "Untagged"}
        for round_key in ROUND_ORDER:
            by_file = no_bench[round_key]
            if not by_file:
                continue
            subtotal = sum(len(v) for v in by_file.values())
            print(f"\n  {labels[round_key]} ({subtotal}):")
            for yaml_file, names in sorted(by_file.items()):
                print(f"    {yaml_file}: {', '.join(names)}")

    write_benchmark_env(meta, BENCH_ENV_FILE)


if __name__ == "__main__":
    main()
