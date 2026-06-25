#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Parse a kembench results file and update data/kems/*.yaml with measured cycles.

Also writes data/kem_benchmark_env.yaml describing the benchmark environment.

Usage:
    uv run kembench/update_kem_data.py [results_file]

If no results_file is given, uses the most recent file in kembench/results/.
"""
import re
import sys
from pathlib import Path
from ruamel.yaml import YAML

REPO_ROOT = Path(__file__).parent.parent
KEMS_DIR = REPO_ROOT / "data" / "kems"
BENCH_ENV_FILE = REPO_ROOT / "data" / "kem_benchmark_env.yaml"

# Map kembench output name → (yaml_filename, parameterset_name_in_yaml).
BENCH_TO_YAML: dict[str, tuple[str, str]] = {
    # ECDH (modelled as DHKEM)
    "X25519": ("ECDH.yaml", "X25519"),
    "X448": ("ECDH.yaml", "X448"),
    # ML-KEM (FIPS 203)
    "ML-KEM-512": ("ML-KEM.yaml", "ML-KEM-512"),
    "ML-KEM-768": ("ML-KEM.yaml", "ML-KEM-768"),
    "ML-KEM-1024": ("ML-KEM.yaml", "ML-KEM-1024"),
    # HQC
    "HQC-128": ("HQC.yaml", "HQC-128"),
    "HQC-192": ("HQC.yaml", "HQC-192"),
    "HQC-256": ("HQC.yaml", "HQC-256"),
}


def parse_results_file(path: Path) -> tuple[dict, list[dict]]:
    """Return (env_meta, rows). rows have keygen/encaps/decaps cycles + µs."""
    meta: dict = {}
    rows: list[dict] = []
    in_data = False

    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# "):
                src_m = re.match(r"^#\s+schemes/(\w+)(?:/ref)?:\s+(\S+)", line)
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
                # columns: name... iters kg_cyc kg_us en_cyc en_us de_cyc de_us
                if len(parts) >= 8:
                    name = " ".join(parts[:-7])
                    iters, kg_cyc, _kg_us, en_cyc, en_us, de_cyc, de_us = parts[-7:]
                    rows.append({
                        "name": name,
                        "iters": int(iters),
                        "keygen_cycles": int(kg_cyc),
                        "encaps_cycles": int(en_cyc),
                        "decaps_cycles": int(de_cyc),
                        "keygen_us": float(_kg_us),
                        "encaps_us": float(en_us),
                        "decaps_us": float(de_us),
                    })
    return meta, rows


def update_parameterset(yaml_data, ps_name: str, row: dict) -> bool:
    """Find ps_name in any version-less KEM yaml and set timing fields."""
    for ps in yaml_data.get("parametersets", []):
        if str(ps.get("name", "")) == ps_name:
            ps["keygen_cycles"] = row["keygen_cycles"]
            ps["encaps_cycles"] = row["encaps_cycles"]
            ps["decaps_cycles"] = row["decaps_cycles"]
            ps["keygen_us"] = row["keygen_us"]
            ps["encaps_us"] = row["encaps_us"]
            ps["decaps_us"] = row["decaps_us"]
            return True
    return False


def write_benchmark_env(meta: dict, path: Path) -> None:
    yaml = YAML()
    yaml.default_flow_style = False
    yaml.width = 4096
    from ruamel.yaml.comments import CommentedMap

    env = CommentedMap()
    env.yaml_set_start_comment(
        "Licensed under CC BY 4.0: https://creativecommons.org/licenses/by/4.0/"
    )
    env["license"] = "CC-BY-4.0"
    env["attribution"] = "Thom Wiggers / PQShield"
    env["date"] = meta.get("date", "")

    cpu = CommentedMap()
    cpu["model"] = meta.get("cpu", "")
    cores_m = re.match(r"(\d+)\s+\(threads/core:\s+(\d+)\)", meta.get("cores", ""))
    if cores_m:
        cpu["cores"] = int(cores_m.group(1))
        cpu["threads_per_core"] = int(cores_m.group(2))
    freq_m = re.match(r"(\d+)", meta.get("max_freq_mhz", ""))
    if freq_m:
        cpu["max_freq_mhz"] = int(freq_m.group(1))
    cpu["governor"] = meta.get("governor", "")
    cpu["turbo"] = meta.get("turbo", "")
    env["cpu"] = cpu

    env["os"] = meta.get("os", "")
    env["kernel"] = meta.get("kernel", "")
    env["compiler"] = meta.get("compiler", "")
    env["openssl"] = meta.get("openssl", "")
    env["notes"] = (
        "Median over 1000 iterations (fewer for slow schemes). "
        "KEM operations: keygen / encapsulation / decapsulation. "
        "Cycle counter: rdtsc + lfence (constant TSC rate, not execution cycles). "
        "Wall clock: clock_gettime(CLOCK_MONOTONIC). "
        "ECDH is modelled as a DHKEM (encaps = ephemeral keygen + raw ECDH)."
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
    results_dir = REPO_ROOT / "kembench" / "results"

    if len(sys.argv) > 1:
        results_file = Path(sys.argv[1])
    else:
        files = sorted(results_dir.glob("*.txt"))
        if not files:
            sys.exit("No results files found in kembench/results/")
        results_file = files[-1]
        print(f"Using {results_file.name}")

    meta, rows = parse_results_file(results_file)
    print(f"Parsed {len(rows)} benchmark rows")

    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096

    updated = 0
    not_mapped = []

    file_updates: dict[str, list[tuple[str, dict]]] = {}
    for row in rows:
        if row["name"] not in BENCH_TO_YAML:
            not_mapped.append(row["name"])
            continue
        yaml_file, ps_name = BENCH_TO_YAML[row["name"]]
        file_updates.setdefault(yaml_file, []).append((ps_name, row))

    for yaml_filename, updates in sorted(file_updates.items()):
        yaml_path = KEMS_DIR / yaml_filename
        if not yaml_path.exists():
            print(f"  MISSING: {yaml_filename}")
            continue
        with open(yaml_path) as f:
            data = yaml.load(f)
        changed = False
        for ps_name, row in updates:
            if update_parameterset(data, ps_name, row):
                print(f"  {yaml_filename}  {ps_name}: "
                      f"keygen={row['keygen_us']}µs encaps={row['encaps_us']}µs decaps={row['decaps_us']}µs")
                updated += 1
                changed = True
            else:
                print(f"  NOT FOUND: {yaml_filename}  '{ps_name}'")
        if changed:
            with open(yaml_path, "w") as f:
                yaml.dump(data, f)

    print(f"\nUpdated {updated} parametersets")
    if not_mapped:
        print(f"\nNo YAML mapping for {len(not_mapped)} bench names:")
        for n in not_mapped:
            print(f"  {n}")

    if meta:
        write_benchmark_env(meta, BENCH_ENV_FILE)


if __name__ == "__main__":
    main()
