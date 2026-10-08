"""Crash isolation in ./bench and ./bench-kem: a scheme that crashes, hangs or fails
its warm-up check gives a DNF row, and the run carries on with the next scheme.

Builds both harnesses against toy schemes (fixtures/toy_*.c) in a temp dir; no
real scheme code runs.
"""

import os
import re
import signal
import subprocess
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
FIX = HERE / "fixtures"

# (name, mode, expected DNF reason or None for a normal row)
SIG_TOYS = [
    ("Toy-Abort", "TOY_ABORT", "SIGABRT"),
    ("Toy-Ok-1", "TOY_OK", None),
    ("Toy-Segv", "TOY_SEGV", "SIGSEGV"),
    ("Toy-Hang", "TOY_HANG", "timeout"),
    ("Toy-Badcheck", "TOY_BADCHECK", "valid-signature-rejected"),
    ("Toy-Fpe", "TOY_FPE", "SIGFPE"),
    ("Toy-Ok-2", "TOY_OK", None),
]
KEM_TOYS = [
    (name, mode, "shared-secret-mismatch" if mode == "TOY_BADCHECK" else reason)
    for name, mode, reason in SIG_TOYS
]
SUITES = {
    "sig": ("bench", "toy_sig.c", SIG_TOYS, 6),
    "kem": ("bench-kem", "toy_kem.c", KEM_TOYS, 6),
}

NUM_ROW = re.compile(r"^(?P<name>\S+)\s+5(\s+\d+\s+\d+\.\d){3}(\s+yes)?$")
DNF_ROW = re.compile(r"^(?P<name>\S+)\s+5(\s+DNF){6}\s+(?P<reason>\S+)$")


def cc(*args):
    subprocess.run(["cc", "-O1", "-Wall", *args], check=True)


@pytest.fixture(scope="module", params=sorted(SUITES))
def harness(request, tmp_path_factory):
    suite = request.param
    src_dir, toy_src, toys, _ = SUITES[suite]
    tmp = tmp_path_factory.mktemp(suite)
    sos = []
    for name, mode, _ in toys:
        so = tmp / f"{name}.so"
        cc(
            "-shared",
            "-fPIC",
            f"-I{REPO / src_dir}",
            f"-I{FIX}",
            f'-DTOY_NAME="{name}"',
            f"-DTOY_MODE={mode}",
            str(FIX / toy_src),
            "-o",
            str(so),
        )
        sos.append(so)
    (tmp / "so_paths.h").write_text("".join(f'"{so}",\n' for so in sos))
    exe = tmp / "harness"
    cc(
        f"-I{tmp}",
        str(REPO / src_dir / "main.c"),
        str(REPO / src_dir / "loader.c"),
        "-o",
        str(exe),
        "-ldl",
        "-lpthread",
    )
    return suite, exe, toys


def run(exe, *filters, timeout="2"):
    env = {
        **os.environ,
        "BENCH_ITER": "5",
        "BENCH_TIMEOUT": timeout,
        "BENCH_CYCLES": "tsc",
    }
    return subprocess.run(
        [str(exe), *filters],
        capture_output=True,
        text=True,
        env=env,
        timeout=60,
        check=False,
    )


def data_rows(stdout):
    lines = stdout.splitlines()
    start = next(i for i, ln in enumerate(lines) if ln.startswith("------")) + 1
    return [ln.rstrip() for ln in lines[start:] if ln.strip()]


def test_each_scheme_gets_a_row_in_order(harness):
    _suite, exe, toys = harness
    proc = run(exe)
    assert proc.returncode == 0, proc.stderr
    rows = data_rows(proc.stdout)
    assert len(rows) == len(toys), proc.stdout
    for row, (name, _, reason) in zip(rows, toys):
        if reason is None:
            m = NUM_ROW.match(row)
            assert m and m["name"] == name, row
        else:
            m = DNF_ROW.match(row)
            assert m and m["name"] == name and m["reason"] == reason, row
    # One stderr line per DNF, so it is visible without the results file.
    for name, _, reason in toys:
        if reason:
            assert f"DNF {name}: {reason}" in proc.stderr


def test_scheme_stdout_does_not_reach_results(harness):
    _, exe, _ = harness
    proc = run(exe, "Toy-Ok-1")
    assert "toy chatter" not in proc.stdout
    assert "toy chatter" in proc.stderr


def test_only_dnf_rows_exit_2(harness):
    _, exe, _ = harness
    proc = run(exe, "abort", "segv")
    assert proc.returncode == 2, proc.stdout + proc.stderr
    assert [DNF_ROW.match(r)["reason"] for r in data_rows(proc.stdout)] == [
        "SIGABRT",
        "SIGSEGV",
    ]


def test_no_match_exits_0(harness):
    _, exe, _ = harness
    proc = run(exe, "nosuchscheme")
    assert proc.returncode == 0
    assert data_rows(proc.stdout) == []


def test_timeout_0_disables_the_limit(harness):
    _, exe, _ = harness
    proc = subprocess.Popen(
        [str(exe), "hang"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        env={**os.environ, "BENCH_ITER": "5", "BENCH_TIMEOUT": "0"},
        start_new_session=True,  # so the hanging child can be killed with it
    )
    try:
        with pytest.raises(subprocess.TimeoutExpired):
            proc.wait(timeout=3)
    finally:
        os.killpg(proc.pid, signal.SIGKILL)
        proc.wait()


def test_sizes_unchanged(harness):
    _suite, exe, _toys = harness
    proc = run(exe, "--sizes", "ok")
    assert proc.returncode == 0
    lines = proc.stdout.splitlines()
    assert lines[0].startswith("scheme\tpk\tsk\t")
    assert [ln.split("\t")[0] for ln in lines[1:]] == ["Toy-Ok-1", "Toy-Ok-2"]
