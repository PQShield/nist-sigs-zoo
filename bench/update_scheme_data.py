#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Parse a bench results file and update data/schemes/*.yaml with measured cycles.

Also writes data/benchmark_env.yaml describing the benchmark environment.

Usage:
    uv run bench/update_scheme_data.py [results_file]

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
    "MAYO-two (broken)": ("MAYO.yaml", "two"),
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
    # SNOVA — l=4 variants retained in v2.3
    "SNOVA-24-5-16-4": ("SNOVA.yaml", "SNOVA-24-5-16-4"),
    "SNOVA-37-8-16-4": ("SNOVA.yaml", "SNOVA-37-8-16-4"),
    "SNOVA-60-10-16-4": ("SNOVA.yaml", "SNOVA-60-10-16-4"),
    # SNOVA — updated l=2 variants, q=19 variants, and 4x5 structure
    "SNOVA-28-5-19-4":   ("SNOVA.yaml", "SNOVA-28-5-19-4"),
    "SNOVA-48-17-16-2":  ("SNOVA.yaml", "SNOVA-48-17-16-2"),
    "SNOVA-48-16-19-2":  ("SNOVA.yaml", "SNOVA-48-16-19-2"),
    "SNOVA-28-4-16-4x5": ("SNOVA.yaml", "SNOVA-28-4-16-4x5"),
    "SNOVA-28-4-19-4x5": ("SNOVA.yaml", "SNOVA-28-4-19-4x5"),
    "SNOVA-40-7-19-4":   ("SNOVA.yaml", "SNOVA-40-7-19-4"),
    "SNOVA-72-25-16-2":  ("SNOVA.yaml", "SNOVA-72-25-16-2"),
    "SNOVA-38-5-16-4x5": ("SNOVA.yaml", "SNOVA-38-5-16-4x5"),
    "SNOVA-38-5-19-4x5": ("SNOVA.yaml", "SNOVA-38-5-19-4x5"),
    "SNOVA-50-9-19-4":   ("SNOVA.yaml", "SNOVA-50-9-19-4"),
    "SNOVA-97-33-16-2":  ("SNOVA.yaml", "SNOVA-97-33-16-2"),
    # UOV
    "UOV-Ip-pkc": ("UOV.yaml", "Ip-pkc"),
    "UOV-Ip-classic": ("UOV.yaml", "Ip-classic"),
    "UOV-Is-pkc": ("UOV.yaml", "Is-pkc"),
    "UOV-Is-classic": ("UOV.yaml", "Is-classic"),
    "UOV-III-pkc": ("UOV.yaml", "III-pkc"),
    "UOV-III-classic": ("UOV.yaml", "III-classic"),
    "UOV-V-pkc": ("UOV.yaml", "V-pkc"),
    "UOV-V-classic": ("UOV.yaml", "V-classic"),
    # QR-UOV — bench uses commas, YAML uses spaces inside parens
    "QR-UOV-I-(127,156,54,3)": ("QR-UOV.yaml", "I-(127 156 54 3)"),
    "QR-UOV-I-(31,165,60,3)": ("QR-UOV.yaml", "I-(31 165 60 3)"),
    "QR-UOV-I-(31,600,70,10)": ("QR-UOV.yaml", "I-(31 600 70 10)"),
    "QR-UOV-I-(7,740,100,10)": ("QR-UOV.yaml", "I-(7 740 100 10)"),
    "QR-UOV-III-(127,228,78,3)": ("QR-UOV.yaml", "III-(127 228 78 3)"),
    "QR-UOV-III-(31,246,87,3)": ("QR-UOV.yaml", "III-(31 246 87 3)"),
    "QR-UOV-III-(31,890,100,10)": ("QR-UOV.yaml", "III-(31 890 100 10)"),
    "QR-UOV-III-(7,1100,140,10)": ("QR-UOV.yaml", "III-(7 1100 140 10)"),
    "QR-UOV-V-(127,306,105,3)": ("QR-UOV.yaml", "V-(127 306 105 3)"),
    "QR-UOV-V-(31,324,114,3)": ("QR-UOV.yaml", "V-(31 324 114 3)"),
    "QR-UOV-V-(31,1120,120,10)": ("QR-UOV.yaml", "V-(31 1120 120 10)"),
    "QR-UOV-V-(7,1490,190,10)": ("QR-UOV.yaml", "V-(7 1490 190 10)"),
    # MQOM — bench r3/r5 suffix → YAML 3r/5r
    "MQOM2-L1-gf2-fast-r3": ("MQOM.yaml", "L1-gf2-fast-3r"),
    "MQOM2-L1-gf2-fast-r5": ("MQOM.yaml", "L1-gf2-fast-5r"),
    "MQOM2-L1-gf2-short-r3": ("MQOM.yaml", "L1-gf2-short-3r"),
    "MQOM2-L1-gf2-short-r5": ("MQOM.yaml", "L1-gf2-short-5r"),
    "MQOM2-L1-gf16-fast-r3": ("MQOM.yaml", "L1-gf16-fast-3r"),
    "MQOM2-L1-gf16-fast-r5": ("MQOM.yaml", "L1-gf16-fast-5r"),
    "MQOM2-L1-gf16-short-r3": ("MQOM.yaml", "L1-gf16-short-3r"),
    "MQOM2-L1-gf16-short-r5": ("MQOM.yaml", "L1-gf16-short-5r"),
    "MQOM2-L1-gf256-fast-r3": ("MQOM.yaml", "L1-gf256-fast-3r"),
    "MQOM2-L1-gf256-fast-r5": ("MQOM.yaml", "L1-gf256-fast-5r"),
    "MQOM2-L1-gf256-short-r3": ("MQOM.yaml", "L1-gf256-short-3r"),
    "MQOM2-L1-gf256-short-r5": ("MQOM.yaml", "L1-gf256-short-5r"),
    "MQOM2-L3-gf2-fast-r3": ("MQOM.yaml", "L3-gf2-fast-3r"),
    "MQOM2-L3-gf2-fast-r5": ("MQOM.yaml", "L3-gf2-fast-5r"),
    "MQOM2-L3-gf2-short-r3": ("MQOM.yaml", "L3-gf2-short-3r"),
    "MQOM2-L3-gf2-short-r5": ("MQOM.yaml", "L3-gf2-short-5r"),
    "MQOM2-L3-gf16-fast-r3": ("MQOM.yaml", "L3-gf16-fast-3r"),
    "MQOM2-L3-gf16-fast-r5": ("MQOM.yaml", "L3-gf16-fast-5r"),
    "MQOM2-L3-gf16-short-r3": ("MQOM.yaml", "L3-gf16-short-3r"),
    "MQOM2-L3-gf16-short-r5": ("MQOM.yaml", "L3-gf16-short-5r"),
    "MQOM2-L3-gf256-fast-r3": ("MQOM.yaml", "L3-gf256-fast-3r"),
    "MQOM2-L3-gf256-fast-r5": ("MQOM.yaml", "L3-gf256-fast-5r"),
    "MQOM2-L3-gf256-short-r3": ("MQOM.yaml", "L3-gf256-short-3r"),
    "MQOM2-L3-gf256-short-r5": ("MQOM.yaml", "L3-gf256-short-5r"),
    "MQOM2-L5-gf2-fast-r3": ("MQOM.yaml", "L5-gf2-fast-3r"),
    "MQOM2-L5-gf2-fast-r5": ("MQOM.yaml", "L5-gf2-fast-5r"),
    "MQOM2-L5-gf2-short-r3": ("MQOM.yaml", "L5-gf2-short-3r"),
    "MQOM2-L5-gf2-short-r5": ("MQOM.yaml", "L5-gf2-short-5r"),
    "MQOM2-L5-gf16-fast-r3": ("MQOM.yaml", "L5-gf16-fast-3r"),
    "MQOM2-L5-gf16-fast-r5": ("MQOM.yaml", "L5-gf16-fast-5r"),
    "MQOM2-L5-gf16-short-r3": ("MQOM.yaml", "L5-gf16-short-3r"),
    "MQOM2-L5-gf16-short-r5": ("MQOM.yaml", "L5-gf16-short-5r"),
    "MQOM2-L5-gf256-fast-r3": ("MQOM.yaml", "L5-gf256-fast-3r"),
    "MQOM2-L5-gf256-fast-r5": ("MQOM.yaml", "L5-gf256-fast-5r"),
    "MQOM2-L5-gf256-short-r3": ("MQOM.yaml", "L5-gf256-short-3r"),
    "MQOM2-L5-gf256-short-r5": ("MQOM.yaml", "L5-gf256-short-5r"),
    # SDitH2 — v2.0 gf2 variants (bench output name matches shim's @NAME@)
    "SDitH2-L1-gf2-short": ("SDitH.yaml", "SDitH2-L1-gf2-short"),
    "SDitH2-L1-gf2-fast":  ("SDitH.yaml", "SDitH2-L1-gf2-fast"),
    "SDitH2-L3-gf2-short": ("SDitH.yaml", "SDitH2-L3-gf2-short"),
    "SDitH2-L3-gf2-fast":  ("SDitH.yaml", "SDitH2-L3-gf2-fast"),
    "SDitH2-L5-gf2-short": ("SDitH.yaml", "SDitH2-L5-gf2-short"),
    "SDitH2-L5-gf2-fast":  ("SDitH.yaml", "SDitH2-L5-gf2-fast"),
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


def parse_results_file(path: Path) -> tuple[dict, list[dict]]:
    """Return (env_meta, rows).  rows: list of dicts with keys name/keygen_cyc/sign_cyc/verify_cyc/sign_us/verify_us."""
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
            elif in_data and line.strip():
                parts = line.split()
                if len(parts) >= 8:
                    # scheme name may contain spaces; columns are last 6 fields
                    name = " ".join(parts[:-7])
                    iters, keygen_cyc, _keygen_us, sign_cyc, sign_us, verify_cyc, verify_us = parts[-7:]
                    rows.append({
                        "name": name,
                        "iters": int(iters),
                        "keygen_cycles": int(keygen_cyc),
                        "signing_cycles": int(sign_cyc),
                        "verification_cycles": int(verify_cyc),
                        "signing_us": float(sign_us),
                        "verification_us": float(verify_us),
                    })
    return meta, rows


def find_and_update_parameterset(
    yaml_data,
    ps_name: str,
    signing_cycles: int,
    verification_cycles: int,
    signing_us: float,
    verification_us: float,
) -> bool:
    """Find first version containing ps_name and update its cycles and µs timings. Return True if found."""
    for version in yaml_data.get("versions", []):
        for ps in version.get("parametersets", []):
            if str(ps.get("name", "")) == ps_name:
                ps["signing_cycles"] = signing_cycles
                ps["verification_cycles"] = verification_cycles
                ps["signing_us"] = signing_us
                ps["verification_us"] = verification_us
                return True
    return False


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
    counter = meta.get("cyclecounter", "rdtsc + lfence (constant TSC rate, not execution cycles)")
    env["notes"] = (
        "Median over 1000 iterations (fewer for slow schemes). "
        f"Cycle counter: {counter}. "
        "Wall clock: clock_gettime(CLOCK_MONOTONIC). "
        "Benchmark thread pinned to one core."
    )

    sources = meta.get("_sources", {})
    if sources:
        src_map = CommentedMap()
        for k, v in sorted(sources.items()):
            src_map[k] = v
        env["sources"] = src_map

    with open(path, "w") as f:
        yaml.dump(env, f)

    print(f"Wrote {path.relative_to(REPO_ROOT)}")


def main() -> None:
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
    file_updates: dict[str, list[tuple[str, int, int, float, float]]] = {}
    for row in rows:
        name = row["name"]
        if name not in BENCH_TO_YAML:
            not_mapped.append(name)
            continue
        yaml_file, ps_name = BENCH_TO_YAML[name]
        file_updates.setdefault(yaml_file, []).append(
            (ps_name, row["signing_cycles"], row["verification_cycles"],
             row["signing_us"], row["verification_us"])
        )

    for yaml_filename, updates in sorted(file_updates.items()):
        yaml_path = SCHEMES_DIR / yaml_filename
        if not yaml_path.exists():
            print(f"  MISSING: {yaml_filename}")
            skipped += len(updates)
            continue

        with open(yaml_path) as f:
            data = yaml.load(f)

        changed = False
        for ps_name, sign_cyc, verify_cyc, sign_us, verify_us in updates:
            if find_and_update_parameterset(data, ps_name, sign_cyc, verify_cyc, sign_us, verify_us):
                print(f"  {yaml_filename}  {ps_name}: sign={sign_cyc:>12,} ({sign_us} µs)  verify={verify_cyc:>12,} ({verify_us} µs)")
                updated += 1
                changed = True
            else:
                print(f"  NOT FOUND: {yaml_filename}  '{ps_name}'")
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
    yaml_to_bench: dict[tuple[str, str], str] = {(v[0], v[1]): k for k, v in BENCH_TO_YAML.items()}

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
        for version in (data.get("versions") or []):
            tags = version.get("tags") or []
            if "round-2" in tags or "round-3" in tags:
                round_key = "round-2"
            elif "round-1" in tags:
                round_key = "round-1"
            else:
                round_key = "untagged"
            for ps in (version.get("parametersets") or []):
                key = (yaml_path.name, str(ps.get("name", "")))
                if key not in yaml_to_bench:
                    no_bench[round_key].setdefault(yaml_path.name, []).append(str(ps.get("name", "")))

    if not_in_run:
        total = sum(len(v) for v in not_in_run.values())
        print(f"\nBench-mapped but not in this run ({total}):")
        for yaml_file, names in sorted(not_in_run.items()):
            print(f"  {yaml_file}: {', '.join(names)}")

    total_no_bench = sum(len(names) for r in ROUND_ORDER for names in no_bench[r].values())
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
