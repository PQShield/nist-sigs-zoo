#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Parse a bench-kem results file and update data/kems/*.yaml with measured cycles.

Also writes data/kem_benchmark_env.yaml describing the benchmark environment.

Usage:
    uv run bench-kem/update_scheme_data.py [results_file]

If no results_file is given, uses the most recent file in bench-kem/results/.
"""
import re
import sys
from pathlib import Path
from ruamel.yaml import YAML

REPO_ROOT = Path(__file__).parent.parent
KEMS_DIR = REPO_ROOT / "data" / "kems"
BENCH_ENV_FILE = REPO_ROOT / "data" / "kem_benchmark_env.yaml"

# Map bench output name → (yaml_filename, parameterset_name_in_yaml).
BENCH_TO_YAML: dict[str, tuple[str, str]] = {
    "ML-KEM-512": ("ML-KEM.yaml", "ML-KEM-512"),
    "ML-KEM-768": ("ML-KEM.yaml", "ML-KEM-768"),
    "ML-KEM-1024": ("ML-KEM.yaml", "ML-KEM-1024"),
    "HQC-128": ("HQC.yaml", "HQC-128"),
    "HQC-192": ("HQC.yaml", "HQC-192"),
    "HQC-256": ("HQC.yaml", "HQC-256"),
    "ECDH-X25519": ("ECDH.yaml", "X25519"),
    "ECDH-X448": ("ECDH.yaml", "X448"),
    "ECDH-P256": ("ECDH.yaml", "P-256"),
    "ECDH-P384": ("ECDH.yaml", "P-384"),
}


def parse_results_file(path: Path) -> tuple[dict, list[dict]]:
    """Return (env_meta, rows). Columns: keygen/encaps/decaps cycles + us, ok."""
    meta: dict = {}
    rows: list[dict] = []
    in_data = False

    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# "):
                src_m = re.match(r"^#\s+schemes/([\w-]+)(?:/ref)?:\s+(\S+)", line)
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
                # Trailing columns: iters kg_cyc kg_us en_cyc en_us de_cyc de_us ok
                if len(parts) >= 9:
                    name = " ".join(parts[:-8])
                    iters, kg_c, kg_u, en_c, en_u, de_c, de_u, ok = parts[-8:]
                    rows.append({
                        "name": name,
                        "iters": int(iters),
                        "keygen_cycles": int(kg_c),
                        "encaps_cycles": int(en_c),
                        "decaps_cycles": int(de_c),
                        "keygen_us": float(kg_u),
                        "encaps_us": float(en_u),
                        "decaps_us": float(de_u),
                        "ok": ok,
                    })
    return meta, rows


def update_parameterset(yaml_data, ps_name: str, row: dict) -> bool:
    """Update a flat-schema KEM parameterset's perf fields. Return True if found."""
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

    env: CommentedMap = CommentedMap()
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
        "KEM keygen/encapsulation/decapsulation. "
        "Median over 1000 iterations (fewer for slow schemes). "
        "Cycle counter: rdtsc + lfence (constant TSC rate, not execution cycles). "
        "Wall clock: clock_gettime(CLOCK_MONOTONIC)."
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
    results_dir = REPO_ROOT / "bench-kem" / "results"
    if len(sys.argv) > 1:
        results_file = Path(sys.argv[1])
    else:
        files = sorted(results_dir.glob("*.txt"))
        if not files:
            sys.exit("No results files found in bench-kem/results/")
        results_file = files[-1]
        print(f"Using {results_file.name}")

    meta, rows = parse_results_file(results_file)
    print(f"Parsed {len(rows)} benchmark rows")

    yaml = YAML()
    yaml.preserve_quotes = True
    yaml.width = 4096

    file_updates: dict[str, list[dict]] = {}
    not_mapped: list[str] = []
    for row in rows:
        if row["name"] not in BENCH_TO_YAML:
            not_mapped.append(row["name"])
            continue
        yaml_file, ps_name = BENCH_TO_YAML[row["name"]]
        file_updates.setdefault(yaml_file, []).append({**row, "_ps": ps_name})

    updated = skipped = 0
    for yaml_filename, updates in sorted(file_updates.items()):
        yaml_path = KEMS_DIR / yaml_filename
        if not yaml_path.exists():
            print(f"  MISSING: {yaml_filename}")
            skipped += len(updates)
            continue
        with open(yaml_path) as f:
            data = yaml.load(f)
        changed = False
        for row in updates:
            ps_name = row["_ps"]
            flag = "" if row["ok"] == "yes" else f"  [{row['ok']}]"
            if update_parameterset(data, ps_name, row):
                print(f"  {yaml_filename}  {ps_name}: keygen={row['keygen_us']}us "
                      f"encaps={row['encaps_us']}us decaps={row['decaps_us']}us{flag}")
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

    write_benchmark_env(meta, BENCH_ENV_FILE)


if __name__ == "__main__":
    main()
