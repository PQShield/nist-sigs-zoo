#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["ruamel.yaml>=0.18"]
# ///
"""Parse a bench-kem results file and update data/kems/*.yaml with measured cycles.

Also writes data/kem_benchmark_env.yaml describing the benchmark environment.

Usage:
    uv run bench-kem/update_scheme_data.py [results_file]
    uv run bench-kem/update_scheme_data.py --ngcc-sizes bench-kem/results/ngcc_sizes.tsv

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
    "FrodoKEM-640-AES": ("FrodoKEM.yaml", "FrodoKEM-640-AES"),
    "FrodoKEM-640-SHAKE": ("FrodoKEM.yaml", "FrodoKEM-640-SHAKE"),
    "FrodoKEM-976-AES": ("FrodoKEM.yaml", "FrodoKEM-976-AES"),
    "FrodoKEM-976-SHAKE": ("FrodoKEM.yaml", "FrodoKEM-976-SHAKE"),
    "FrodoKEM-1344-AES": ("FrodoKEM.yaml", "FrodoKEM-1344-AES"),
    "FrodoKEM-1344-SHAKE": ("FrodoKEM.yaml", "FrodoKEM-1344-SHAKE"),
    "mceliece348864": ("ClassicMcEliece.yaml", "mceliece348864"),
    "mceliece460896": ("ClassicMcEliece.yaml", "mceliece460896"),
    "mceliece6688128": ("ClassicMcEliece.yaml", "mceliece6688128"),
    "mceliece6960119": ("ClassicMcEliece.yaml", "mceliece6960119"),
    "mceliece8192128": ("ClassicMcEliece.yaml", "mceliece8192128"),
    "BAT-257-512": ("BAT.yaml", "BAT-257-512"),
    "BAT-769-1024": ("BAT.yaml", "BAT-769-1024"),
    "ntruhps2048509": ("NTRU.yaml", "ntruhps2048509"),
    "ntruhps2048677": ("NTRU.yaml", "ntruhps2048677"),
    "ntruhps4096821": ("NTRU.yaml", "ntruhps4096821"),
    "ntruhrss701": ("NTRU.yaml", "ntruhrss701"),
    "sntrup653": ("NTRUPrime.yaml", "sntrup653"),
    "sntrup761": ("NTRUPrime.yaml", "sntrup761"),
    "sntrup857": ("NTRUPrime.yaml", "sntrup857"),
    "sntrup953": ("NTRUPrime.yaml", "sntrup953"),
    "sntrup1013": ("NTRUPrime.yaml", "sntrup1013"),
    "sntrup1277": ("NTRUPrime.yaml", "sntrup1277"),
    "NTRU+768": ("NTRUPlus.yaml", "NTRU+768"),
    "NTRU+864": ("NTRUPlus.yaml", "NTRU+864"),
    "NTRU+1152": ("NTRUPlus.yaml", "NTRU+1152"),
    "MIKE-fast-I": ("MIKE.yaml", "MIKE-fast-I"),
    "MIKE-fast-III": ("MIKE.yaml", "MIKE-fast-III"),
    "MIKE-fast-V": ("MIKE.yaml", "MIKE-fast-V"),
    "MIKE-rigorous-I": ("MIKE.yaml", "MIKE-rigorous-I"),
    "MIKE-rigorous-III": ("MIKE.yaml", "MIKE-rigorous-III"),
    "MIKE-rigorous-V": ("MIKE.yaml", "MIKE-rigorous-V"),
    "SMAUG-T128": ("SMAUGT.yaml", "SMAUG-T128"),
    "SMAUG-T192": ("SMAUGT.yaml", "SMAUG-T192"),
    "SMAUG-T256": ("SMAUGT.yaml", "SMAUG-T256"),
    "LightSaber": ("SABER.yaml", "LightSaber"),
    "Saber": ("SABER.yaml", "Saber"),
    "FireSaber": ("SABER.yaml", "FireSaber"),
    "ECDH-X25519": ("ECDH.yaml", "X25519"),
    "ECDH-X448": ("ECDH.yaml", "X448"),
    "ECDH-P256": ("ECDH.yaml", "P-256"),
    "ECDH-P384": ("ECDH.yaml", "P-384"),
}

# China NGCC round-1 instances: bench names are NGCC-<instance>. The mapping comes
# from data/ngcc/candidates.yaml (see bench-common/ngcc/ngcc_data.py).
sys.path.insert(0, str(REPO_ROOT / "bench-common" / "ngcc"))
import ngcc_data

NGCC_CANDIDATES = REPO_ROOT / "data" / "ngcc" / "candidates.yaml"
NGCC_STATUS = REPO_ROOT / "bench-kem" / "schemes" / "ngcc" / "status.tsv"
if NGCC_CANDIDATES.exists():
    BENCH_TO_YAML.update(
        ngcc_data.bench_to_yaml(ngcc_data.load_candidates(NGCC_CANDIDATES), "kem")
    )


def import_ngcc_sizes(sizes_file: Path) -> None:
    """Create or refresh the NGCC KEM YAML files from `--sizes NGCC` output."""
    cands = ngcc_data.load_candidates(NGCC_CANDIDATES)
    try:
        written = ngcc_data.write_files(
            cands,
            "kem",
            ngcc_data.load_sizes(sizes_file),
            ngcc_data.load_status(NGCC_STATUS),
            KEMS_DIR,
            ngcc_data.load_untimed(NGCC_STATUS.with_name("params.tsv")),
        )
    except ngcc_data.NgccDataError as e:
        sys.exit(f"NGCC import failed:\n{e}")
    print(f"Wrote {len(written)} NGCC KEM files in {KEMS_DIR.relative_to(REPO_ROOT)}")


# Results rows: "<name> <iters>" + cycles/µs for keygen, encaps, decaps + ok, or a
# DNF row (crash, timeout, failed warm-up check) whose six timing columns read DNF
# and whose last column is a one-word reason. Other lines (stderr merged in by
# run_bench.sh) are skipped.
_NUM = r"(\d+)\s+(\d+\.\d+)"  # cycles, then µs printed as %.1f
ROW_RE = re.compile(
    rf"^(?P<name>\S.*?)\s+(?P<iters>\d+)\s+{_NUM}\s+{_NUM}\s+{_NUM}\s+(?P<ok>\S+)\s*$"
)
DNF_RE = re.compile(
    r"^(?P<name>\S.*?)\s+(?P<iters>\d+|-)(?:\s+DNF){6}\s+(?P<reason>\S+)\s*$"
)


def parse_results_file(path: Path) -> tuple[dict, list[dict]]:
    """Return (env_meta, rows). Columns: keygen/encaps/decaps cycles + us, ok; or dnf."""
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
                kg_c, kg_u, en_c, en_u, de_c, de_u = m.groups()[2:8]
                rows.append(
                    {
                        "name": m["name"],
                        "iters": int(m["iters"]),
                        "dnf": None,
                        "keygen_cycles": int(kg_c),
                        "encaps_cycles": int(en_c),
                        "decaps_cycles": int(de_c),
                        "keygen_us": float(kg_u),
                        "encaps_us": float(en_u),
                        "decaps_us": float(de_u),
                        "ok": m["ok"],
                    }
                )
    return meta, rows


def apply_row(yaml_data, ps_name: str, row: dict) -> tuple[bool, str | None]:
    """Apply one results row to a flat-schema KEM parameter set.

    Returns (changed, message); message is None if ps_name was not found.
    A DNF row on an NGCC parameter set removes its timings and notes why; on any
    other scheme it only warns, so a crash never overwrites measured timings.
    """
    ps = next(
        (
            p
            for p in yaml_data.get("parametersets", [])
            if str(p.get("name", "")) == ps_name
        ),
        None,
    )
    if ps is None:
        return False, None
    ngcc = row["name"].startswith(ngcc_data.PREFIX)
    if row["dnf"]:
        if ngcc:
            ngcc_data.mark_dnf(ps, "kem", row["dnf"])
            return True, f"DNF ({row['dnf']}): timings removed, noted"
        return False, f"WARNING: DNF ({row['dnf']}); existing timings kept"
    for field in ngcc_data.TIMING["kem"]:
        ps[field] = row[field]
    if ngcc:
        ngcc_data.clear_dnf(ps)
    flag = "" if row["ok"] == "yes" else f"  [{row['ok']}]"
    return True, (
        f"keygen={row['keygen_us']}us encaps={row['encaps_us']}us "
        f"decaps={row['decaps_us']}us{flag}"
    )


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
    counter = meta.get(
        "cyclecounter", "rdtsc + lfence (constant TSC rate, not execution cycles)"
    )
    env["notes"] = (
        "KEM keygen/encapsulation/decapsulation. "
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

    write_benchmark_env(meta, BENCH_ENV_FILE)


if __name__ == "__main__":
    main()
