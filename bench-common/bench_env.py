"""Benchmark environments: data/benchmark_env.yaml and data/kem_benchmark_env.yaml.

Each file holds a list of environments (`environments:`), one per imported results
file, keyed by `id` (`<run date>-<EC2 instance type>`, or `<run date>-local-<cpu slug>`
for a run outside EC2). Every parameter set with timings names the environment it was
measured in (`bench_env: <id>`), so rows from different runs or hardware can coexist,
e.g. while a split run is half imported. The importers add or replace the environment
of the file they import and drop environments no parameter set refers to any more.
Shared by bench/update_scheme_data.py and bench-kem/update_scheme_data.py.
"""

from __future__ import annotations

import re
from pathlib import Path

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap

import fingerprint

LICENSE_COMMENT = (
    "Licensed under CC BY 4.0: https://creativecommons.org/licenses/by/4.0/"
)
KEY = "bench_env"


def cpu_slug(cpu: str) -> str:
    """Same as run_bench.sh's CPU_SLUG."""
    return re.sub(r"_+", "_", re.sub(r"[^a-z0-9]", "_", cpu.lower())).rstrip("_")


def instance_type(meta: dict) -> str:
    return meta.get("instance_type") or fingerprint.legacy_instance_type(
        meta.get("cpu", "")
    )


def env_id(meta: dict) -> str:
    itype = instance_type(meta)
    if itype == fingerprint.LOCAL:
        itype = f"local-{cpu_slug(meta.get('cpu', 'unknown'))}"
    return f"{(meta.get('date') or 'unknown')[:10]}-{itype}"


def make_env(meta: dict, notes_prefix: str = "") -> CommentedMap:
    """One environment from a results header (parsed by the importers' parse_results_file)."""
    env = CommentedMap()
    env["id"] = env_id(meta)
    env["date"] = meta.get("date", "")
    env["instance_type"] = instance_type(meta)
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
        f"{notes_prefix}Median over 1000 iterations (fewer for slow schemes). "
        f"Cycle counter: {counter}. "
        "Wall clock: clock_gettime(CLOCK_MONOTONIC). "
        "Benchmark thread pinned to one core."
    )
    sources = meta.get("_sources") or {}
    if sources:
        env["sources"] = CommentedMap(sorted(sources.items()))
    return env


def load_envs(path: Path) -> list[dict]:
    if not path.exists():
        return []
    data = YAML().load(path.read_text()) or {}
    return list(data.get("environments") or [])


def used_ids(data_dir: Path) -> set[str]:
    """Every `bench_env` value in the YAML files of a data directory."""
    found: set[str] = set()

    def walk(node) -> None:
        if isinstance(node, dict):
            if isinstance(node.get(KEY), str):
                found.add(node[KEY])
            for v in node.values():
                walk(v)
        elif isinstance(node, list):
            for v in node:
                walk(v)

    yaml = YAML(typ="safe")
    for p in sorted(data_dir.glob("*.yaml")):
        walk(yaml.load(p.read_text()))
    return found


def write_envs(path: Path, envs: list[dict], keep: set[str]) -> list[str]:
    """Write the environments whose id is in `keep`, newest first; returns dropped ids."""
    kept = sorted(
        (e for e in envs if e["id"] in keep), key=lambda e: str(e["date"]), reverse=True
    )
    doc = CommentedMap()
    doc.yaml_set_start_comment(LICENSE_COMMENT)
    doc["license"] = "CC-BY-4.0"
    doc["attribution"] = "Thom Wiggers / PQShield"
    doc["environments"] = kept
    yaml = YAML()
    yaml.default_flow_style = False
    yaml.width = 4096
    with open(path, "w") as f:
        yaml.dump(doc, f)
    return [e["id"] for e in envs if e["id"] not in keep]


def update_env_file(path: Path, env: CommentedMap, data_dir: Path) -> None:
    """Add or replace `env`, then drop environments no parameter set uses."""
    envs = [e for e in load_envs(path) if e["id"] != env["id"]] + [env]
    dropped = write_envs(path, envs, used_ids(data_dir))
    print(f"Wrote {path.name}: environment {env['id']}")
    for d in dropped:
        print(f"  dropped environment {d}: no parameter set uses it any more")


def set_env(ps, eid: str, timing_fields) -> None:
    """Put `bench_env: eid` right after the parameter set's last timing field."""
    ps.pop(KEY, None)
    keys = list(ps.keys())
    after = [i for i, k in enumerate(keys) if k in timing_fields]
    pos = (after[-1] + 1) if after else len(keys)
    if hasattr(ps, "insert"):
        ps.insert(pos, KEY, eid)
    else:
        ps[KEY] = eid
