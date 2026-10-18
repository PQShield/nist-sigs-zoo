#!/usr/bin/env python3
"""Report whether the host CPU has all the given instruction-set features.

Usage (from a Makefile):
    HAVE_AVX512 := $(shell python3 ../../cpu_features.py avx512f avx512bw)

Prints "1" if every named feature is present and nothing otherwise, so the
result can be tested with ifeq/ifneq. Feature names follow /proc/cpuinfo
(avx2, gfni, avx512f, avx512bw, avx512_vnni, ...); underscores and case are
ignored, so avx512vnni and AVX512_VNNI mean the same thing.

Set BENCH_DISABLE_FEATURES (space-separated names) to pretend features are
absent, e.g. to benchmark the AVX2 fallback on an AVX-512 machine.
"""

import os
import subprocess
import sys


def norm(name: str) -> str:
    return name.replace("_", "").replace(".", "").lower()


def host_features() -> set[str]:
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("flags"):
                    return {norm(x) for x in line.split(":", 1)[1].split()}
    except OSError:
        pass
    # macOS on x86-64
    try:
        out = subprocess.run(
            ["sysctl", "-n", "machdep.cpu.features", "machdep.cpu.leaf7_features"],
            capture_output=True,
            text=True,
            check=False,
        ).stdout
        return {norm(x) for x in out.split()}
    except OSError:
        return set()


def main() -> None:
    wanted = {norm(x) for x in sys.argv[1:]}
    have = host_features() - {
        norm(x) for x in os.environ.get("BENCH_DISABLE_FEATURES", "").split()
    }
    if wanted and wanted <= have:
        print("1")


if __name__ == "__main__":
    main()
