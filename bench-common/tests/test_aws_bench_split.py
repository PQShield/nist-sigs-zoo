"""scripts/aws-bench-split: planning, retry detection, merging and the launcher.

Nothing here touches AWS: the launcher tests run a fake aws-bench.sh and a fake `aws`.
"""

import importlib.machinery
import importlib.util
import os
import signal
import subprocess
import sys
import textwrap
import time
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parents[2]
SCRIPT = REPO / "scripts" / "aws-bench-split"


def load():
    loader = importlib.machinery.SourceFileLoader("aws_bench_split", str(SCRIPT))
    spec = importlib.util.spec_from_loader("aws_bench_split", loader)
    mod = importlib.util.module_from_spec(spec)
    sys.modules["aws_bench_split"] = mod  # dataclasses look the module up
    loader.exec_module(mod)
    return mod


S = load()

COLUMNS = (
    "scheme                       iters    keygen (cyc)  keygen (us)      sign (cyc)   sign (us)    verify (cyc)  verify (us)\n"
    "------                       -----    ------------  -----------      ----------   ---------    ------------  ----------\n"
)


def row(name, iters, kg, sg, vf):
    return f"{name:<28}  {iters:>5}  {1:>14}  {kg:>11}  {1:>14}  {sg:>11}  {1:>14}  {vf:>11}\n"


def dnf(name, iters, why="SIGSEGV"):
    return f"{name:<28}  {iters:>5}" + "  DNF" * 6 + f"  {why}\n"


def results(rows, *, cpu="Test CPU", date="2026-10-13T10:00:00Z", flt=None):
    head = "# pq-sigs benchmark results\n"
    head += f"# date:         {date}\n# host:         h\n# cpu:          {cpu}\n"
    if flt is not None:
        head += f"# filter:       {flt}\n"
    return head + "#\n# cyclecounter: rdpmc\n" + COLUMNS + "".join(rows)


# --- rows and estimates ---------------------------------------------------------


def make_suite(tmp_path):
    suite = tmp_path / "bench"
    (suite / "schemes" / "a").mkdir(parents=True)
    (suite / "schemes" / "ngcc").mkdir(parents=True)
    (suite / "results").mkdir()
    (suite / "schemes" / "a" / "params.tsv").write_text(
        "# comment\nfile\tname\tpk\titers\na1\tAlpha-1\t1\t0\na2\tAlpha-2\t1\t10\n"
    )
    (suite / "schemes" / "a" / "other_params.tsv").write_text(
        "file\tname\titers\nrsa\tRSA-2048 (PSS)\t50\n"
    )
    (suite / "schemes" / "ngcc" / "params.tsv").write_text(
        "file\tname\tid\titers\n"
        "n1\tNGCC-X-I\tx\t1\n"
        "n2\tNGCC-X-II\tx\t5\n"
        "n3\tNGCC-Skip\tx\tskip\n"
        "n4\tNGCC-Broken\tx\t1000\n"
    )
    (suite / "schemes" / "ngcc" / "status.tsv").write_text(
        "file\tname\tid\tinstance\timpl\tbuild\n"
        "n1\tNGCC-X-I\tx\ti\topt\tok\n"
        "n2\tNGCC-X-II\tx\ti\topt\tok\n"
        "n3\tNGCC-Skip\tx\ti\topt\tok\n"
        "n4\tNGCC-Broken\tx\ti\topt\tFAIL\n"
    )
    return suite


def test_registry_skips_unbuilt_and_untimed_rows(tmp_path):
    suite = make_suite(tmp_path)
    assert S.load_registry(suite, include_ngcc=False) == [
        ("RSA-2048 (PSS)", "50"),
        ("Alpha-1", "0"),
        ("Alpha-2", "10"),
    ]
    with_ngcc = S.load_registry(suite, include_ngcc=True)
    assert ("NGCC-X-I", "1") in with_ngcc and ("NGCC-X-II", "5") in with_ngcc
    assert not {"NGCC-Skip", "NGCC-Broken"} & {n for n, _ in with_ngcc}


def test_filters_match_like_bench_main():
    assert S.matches("ML-DSA-44", ["mldsa"])
    assert S.matches("RSA-2048 (PSS)", ["rsa2048pss"])
    assert not S.matches("ML-DSA-44", ["mlkem"])
    assert S.matches("anything", [])


def test_iters_heuristic():
    assert [S.iters_estimate_s(n) for n in (1, 5, 10, 50, 999, 1000, 0)] == [
        1500,
        600,
        180,
        45,
        45,
        20,
        20,
    ]


def test_estimates_from_newest_merged_results(tmp_path):
    rd = tmp_path / "results"
    rd.mkdir()
    # Newest merged file wins over an unmerged one; a DNF row falls through to an
    # older merged file; a row in no merged file comes from an unmerged one.
    (rd / "20261013T000000Z_cpu_merged.txt").write_text(
        results([row("A", 10, "100.0", "200.0", "300.0"), row("B", 10, "1", "1", "1")])
    )
    (rd / "20261014T000000Z_cpu_split_merged.txt").write_text(
        results([row("A", 10, "1000.0", "2000.0", "3000.0"), dnf("B", 10)])
    )
    (rd / "20261015T000000Z_cpu_a.txt").write_text(
        results([row("A", 10, "9", "9", "9"), row("E", 10, "1", "2", "3")])
    )
    m = S.measured_us(rd)
    assert m == {"A": 6000.0, "B": 3.0, "E": 6.0}

    rows = S.estimate_rows([("A", "10"), ("C", "10"), ("D", "0")], m, None, 3600)
    a, c, d = rows
    assert (a.source, a.iters) == ("results", 10)
    assert a.est_s == pytest.approx(S.ROW_OVERHEAD_S + 11 * 6000 / 1e6)
    assert (c.source, c.est_s) == ("iters", 180)
    assert (d.iters, d.est_s) == (1000, 20)


def test_bench_iter_and_timeout_scale_estimates():
    rows = S.estimate_rows([("A", "1"), ("B", "1000")], {}, 5, 10**6)
    assert rows[0].iters == 5 and rows[0].est_s == pytest.approx(1500 * 6 / 2)
    assert rows[1].est_s == S.ROW_OVERHEAD_S  # 20 s * 6/1001, floored
    capped = S.estimate_rows([("A", "1")], {}, None, 100)
    assert capped[0].est_s == 130


# --- partitioning -----------------------------------------------------------------


def mk(name, est):
    return S.Row(name, 10, 10, est, "iters")


def test_partition_long_rows_alone_rest_first_fit_decreasing():
    rows = [mk("long1", 1500), mk("long2", 600), mk("a", 500), mk("b", 400)]
    rows += [mk("c", 300), mk("d", 200), mk("e", 100), mk("f", 599)]
    groups = S.partition(rows, 960)
    names = [sorted(g.filters) for g in groups]
    assert ["long1"] in names and ["long2"] in names
    # FFD into 960 s bins: f(599)+e? no: f, a(500) don't fit together (1099);
    # bins: [f 599, c 300] -> 899; [a 500, b 400] -> 900; [d 200, e 100] -> 300
    assert sorted(["c", "f"]) in names
    assert sorted(["a", "b"]) in names
    assert sorted(["d", "e"]) in names
    assert sorted(n for g in groups for n in g.filters) == sorted(r.name for r in rows)
    assert all(g.est_s <= 960 for g in groups if len(g.rows) > 1)
    # Longest first, numbered from 1.
    assert [g.index for g in groups] == list(range(1, len(groups) + 1))
    assert [g.est_s for g in groups] == sorted((g.est_s for g in groups), reverse=True)


def test_partition_oversized_short_row_gets_own_bin():
    groups = S.partition([mk("a", 500), mk("b", 500)], 300)
    assert len(groups) == 2


def test_partition_keeps_substring_names_together():
    rows = [mk("NGCC-Aigis-Enc+-I", 300), mk("X", 500), mk("NGCC-Aigis-Enc+-III", 300)]
    rows.append(mk("NGCC-Aigis-Enc+-II", 300))
    groups = S.partition(rows, 600)
    aigis = [g for g in groups if "NGCC-Aigis-Enc+-I" in g.filters][0]
    assert sorted(aigis.filters) == sorted(r.name for r in rows if "Aigis" in r.name)
    assert S.collisions(aigis.filters, [r.name for r in rows]) == {}


def test_collisions_only_outside_the_group():
    names = ["NGCC-Aigis-Enc+-I", "NGCC-Aigis-Enc+-II", "NGCC-Aigis-Enc+-III", "X"]
    assert S.collisions(["NGCC-Aigis-Enc+-I"], names) == {
        "NGCC-Aigis-Enc+-I": ["NGCC-Aigis-Enc+-II", "NGCC-Aigis-Enc+-III"]
    }
    assert S.collisions(names[:3], names) == {}


def test_ttl_and_command():
    assert S.ttl_minutes(16 * 60) == 41
    assert S.ttl_minutes(10) == 26
    cmd = S.build_command("aws.sh", "kems", "c8i.4xlarge", 41, "/t.tgz", ["A B", "C"])
    assert cmd == [
        "aws.sh",
        "--suite",
        "kems",
        "--instance-type",
        "c8i.4xlarge",
        "--ttl",
        "41",
        "--ngcc-src",
        "/t.tgz",
        "A B",
        "C",
    ]
    assert S.env_prefix({"BENCH_ITER": "5", "AWS_REGION": "eu", "HOME": "/h"}) == (
        "AWS_REGION=eu BENCH_ITER=5 "
    )


def test_default_instance_type_is_aws_bench_default():
    text = (REPO / "scripts" / "aws-bench.sh").read_text()
    assert f"INSTANCE_TYPE={S.default_instance_type()}\n" in text


# --- time-limit exemptions ---------------------------------------------------------


def test_read_exemptions_skips_comments_and_blanks(tmp_path):
    f = tmp_path / "ex.txt"
    f.write_text(
        "# why\n\nAlpha-1\t30\n  RSA-2048 (PSS)\t 12.5 \n  # indented comment\n"
    )
    assert S.read_exemptions(f) == {"Alpha-1": 30, "RSA-2048 (PSS)": 12.5}
    assert S.read_exemptions(S.DEFAULT_EXEMPT_FILE) == {
        "SLH-DSA-SHAKE-128-24": 70,
        "SLH-DSA-SHAKE-192-24": 85,
        "SLH-DSA-SHAKE-256-24": 95,
    }
    f.write_text("Alpha-1\n")
    with pytest.raises(SystemExit, match="NAME<TAB>minutes"):
        S.read_exemptions(f)


def test_exempt_rows_get_own_group_with_relaxed_env_and_ttl():
    rows = S.estimate_rows(
        [("Slow-24", "1"), ("A", "1000"), ("B", "1000")],
        {},
        None,
        3600,
        {"Slow-24": 70},
    )
    slow = rows[0]
    assert slow.exempt and slow.source == "exempt"
    assert slow.est_s == 70 * 60  # the file's estimate, not capped at BENCH_TIMEOUT
    assert not rows[1].exempt
    groups = S.partition(rows, 960)
    by = {tuple(sorted(g.filters)): g for g in groups}
    assert set(by) == {("Slow-24",), ("A", "B")}
    g = by[("Slow-24",)]
    assert g.exempt and g.env == {"BENCH_OP_LIMIT": "0", "BENCH_TIMEOUT": "0"}
    assert all(not x.exempt and x.env == {} for x in groups if x is not g)
    # the 70 min estimate + the standard 25 min margin
    assert S.group_ttl(g) == 70 + S.TTL_MARGIN_MIN == 95
    other = next(x for x in groups if x is not g)
    assert S.group_ttl(other) == S.ttl_minutes(other.est_s)


def test_exempt_row_with_results_uses_them_uncapped():
    rows = S.estimate_rows(
        [("Slow-24", "1")], {"Slow-24": 3e9}, None, 100, {"Slow-24": 70}
    )
    assert rows[0].source == "results" and rows[0].est_s == pytest.approx(2 + 2 * 3000)


def test_dry_run_shows_exemption_and_sets_env_only_for_exempt_rows(fakes):
    tmp, suite, state, env, args = fakes
    ex = tmp / "ex.txt"
    ex.write_text("# reason\nAlpha-1\t30\n")
    env.pop("BENCH_OP_LIMIT", None)
    r = subprocess.run(
        [*args, "--exempt-file", str(ex), "--dry-run"],
        env=env,
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0, r.stderr
    assert "1 rows are exempt from the time limits (ex.txt)" in r.stdout
    assert "EXEMPT from time limits: BENCH_OP_LIMIT=0 BENCH_TIMEOUT=0" in r.stdout
    cmds = r.stdout.split("commands:")[1].strip().splitlines()
    exempt = [c for c in cmds if "Alpha-1" in c]
    assert len(exempt) == 1
    assert exempt[0].startswith("BENCH_OP_LIMIT=0 BENCH_TIMEOUT=0 ")
    assert all("BENCH_OP_LIMIT" not in c for c in cmds if "Alpha-1" not in c)


def test_exempt_env_reaches_only_the_exempt_group(fakes):
    tmp, suite, state, env, args = fakes
    ex = tmp / "ex.txt"
    ex.write_text("Alpha-1\t30\n")
    env["BENCH_OP_LIMIT"] = "77"  # user's value: kept for others, overridden for exempt
    bench = tmp / "fake-aws-bench.sh"
    bench.write_text(
        bench.read_text().replace(
            'echo "[aws-bench] launching spot"',
            'echo "[aws-bench] launching spot"\n'
            'echo "$BENCH_OP_LIMIT $BENCH_TIMEOUT ${filters[*]}" >> "$FAKE_STATE/envs"',
        )
    )
    env["BENCH_TIMEOUT"] = "99"
    r = subprocess.run(
        [*args, "--exempt-file", str(ex)], env=env, capture_output=True, text=True
    )
    seen = {
        ln.split(" ", 2)[2]: ln.split(" ")[:2]
        for ln in (state / "envs").read_text().splitlines()
    }
    assert seen["Alpha-1"] == ["0", "0"]
    assert all(v == ["77", "99"] for k, v in seen.items() if k != "Alpha-1")
    assert r.returncode in (0, 1)


# --- logs -----------------------------------------------------------------------

REFUSED = """\
\x1b[1m[aws-bench 17:14:20]\x1b[0m launching spot c8i.4xlarge in eu-north-1
\x1b[1m[aws-bench 17:14:28]\x1b[0m launch in subnet-1 failed: aws: [ERROR]: An error occurred (MaxSpotInstanceCountExceeded) when calling the RunInstances operation: Max spot instance count exceeded
\x1b[1m[aws-bench 17:14:29]\x1b[0m ERROR: could not launch spot c8i.4xlarge in any AZ (capacity or quota?)
"""

LAUNCHED_AFTER_CAPACITY = """\
[aws-bench 17:14:28] launch in subnet-1 failed: aws: [ERROR]: An error occurred (InsufficientInstanceCapacity) when calling the RunInstances operation
[aws-bench 17:14:31] instance i-0f launched; waiting for it to run
[aws-bench 17:20:00] ERROR: instance is 'terminated' (spot interruption?); no results fetched
"""


def test_retry_detection():
    assert S.is_retryable(REFUSED)
    assert S.is_retryable(
        REFUSED.replace("MaxSpotInstanceCount", "InsufficientInstanceCapacity")
    )
    # An instance was launched (and later lost): rerunning would repeat work; not retried.
    assert not S.is_retryable(LAUNCHED_AFTER_CAPACITY)
    assert not S.is_retryable("[aws-bench] ERROR: working tree has uncommitted changes")
    assert S.last_error(REFUSED).startswith("could not launch spot")


def test_log_stage():
    assert S.log_stage(REFUSED) == "launching"
    assert S.log_stage("instance i-0 launched; waiting for it to run") == "booting"
    assert (
        S.log_stage(f"{S.LAUNCHED}\nbenchmark running; streaming bench.log")
        == "building"
    )
    assert (
        S.log_stage("=== building bench ===\n=== running bench ===") == "benchmarking"
    )


def test_recover_from_log():
    log = (
        "[aws-bench] benchmark running\n=== running bench ===\n"
        + results([row("A", 10, "1.0", "2.0", "3.0"), dnf("B", 10)])
        + "C                               10    12345"  # cut off mid-row
    )
    text = S.recover_from_log(log, "recovered")
    assert text is not None
    assert "# note:         recovered" in text
    lines = text.splitlines()
    assert lines[0] == "# pq-sigs benchmark results"
    assert [ln.split()[0] for ln in lines if ln[:1] in "ABC"] == ["A", "B"]
    assert S.recover_from_log("no header here", "x") is None


# --- results and merge ----------------------------------------------------------


def test_cpu_slug_matches_run_bench():
    assert (
        S.cpu_slug("Intel(R) Xeon(R) Platinum 8488C") == "intel_r_xeon_r_platinum_8488c"
    )


def test_merge_order_reruns_first_then_parts_oldest_first(tmp_path):
    r = tmp_path / "rerun.txt"
    p1 = tmp_path / "20261013T120000Z_x.txt"
    p2 = tmp_path / "20261013T110000Z_y.txt"
    assert S.merge_order([r], [p1, p2]) == [r, p2, p1]
    assert S.merge_order([p1], [p1, p2]) == [p1, p2]  # nothing twice


def test_merged_name_and_find_parts(tmp_path):
    rd = tmp_path
    (rd / "old.txt").write_text(results([], flt="A"))
    p2 = rd / "20261013T120000Z_intel_x_b.txt"
    p1 = rd / "20261013T110000Z_intel_x_a.txt"
    p1.write_text(results([], cpu="Intel X", flt="A B"))
    p2.write_text(results([], cpu="Intel X", flt="C"))
    (rd / "20261013T130000Z_intel_x_unrelated.txt").write_text(results([], flt="Z"))
    assert S.merged_name([p2, p1]) == "20261013T110000Z_intel_x_split_merged.txt"
    jobs = [
        S.Job(S.Group(1, [mk("A", 1), mk("B", 1)]), [], rd / "g1.log"),
        S.Job(S.Group(2, [mk("C", 1)]), [], rd / "g2.log"),
        S.Job(S.Group(3, [mk("D", 1)]), [], rd / "g3.log"),
    ]
    assert S.find_parts(rd, {"old.txt"}, jobs) == {1: p1, 2: p2}


# --- launcher, end to end with fakes ---------------------------------------------

FAKE_AWS_BENCH = """\
#!/usr/bin/env bash
# Fake scripts/aws-bench.sh. Refuses the first $REFUSE launches of each group (spot
# quota), then "runs": writes a results file for its filters, or with $FAKE_HANG
# waits in a foreground child (like ssh) until interrupted.
set -u
filters=()
while [ $# -gt 0 ]; do
    case "$1" in
        --suite|--instance-type|--ttl|--ngcc-src) shift 2 ;;
        *) filters+=("$1"); shift ;;
    esac
done
key=$(printf '%s' "${filters[*]}" | md5sum | cut -c1-8)
n=$(cat "$FAKE_STATE/$key" 2>/dev/null || echo 0); n=$((n + 1)); echo $n > "$FAKE_STATE/$key"
echo "[aws-bench] launching spot"
if [ "$n" -le "${REFUSE:-0}" ]; then
    echo "[aws-bench] launch in subnet-1 failed: An error occurred (MaxSpotInstanceCountExceeded) when calling the RunInstances operation"
    echo "[aws-bench] ERROR: could not launch spot in any AZ (capacity or quota?)"
    exit 1
fi
echo "[aws-bench] instance i-$key launched; waiting for it to run"
if [ -n "${FAKE_HANG:-}" ]; then
    trap 'echo cleaned > "$FAKE_STATE/cleaned-$key"; exit 130' INT TERM
    # Foreground child, like aws-bench.sh's ssh: bash runs the trap only after it
    # exits, so signalling bash alone would hang here.
    bash -c 'echo $$ > "$1"; exec sleep 300' _ "$FAKE_STATE/child-$key"
fi
echo "[aws-bench] benchmark running; streaming bench.log"
echo "=== running bench ==="
out="$FAKE_RESULTS/20261013T1200${n}0Z_test_cpu_$key.txt"
{
    echo "# pq-sigs benchmark results"
    echo "# date:         2026-10-13T12:00:00Z"
    echo "# host:         fake"
    echo "# cpu:          Test CPU"
    echo "# filter:       ${filters[*]}"
    echo "# cyclecounter: rdpmc"
    printf '%s\\n' "scheme                       iters    keygen (cyc)  keygen (us)      sign (cyc)   sign (us)    verify (cyc)  verify (us)"
    printf '%s\\n' "------                       -----    ------------  -----------      ----------   ---------    ------------  ----------"
    for f in "${filters[@]}"; do
        if [ "$f" = "Alpha-2" ]; then
            printf '%-28s  %5s  %s\\n' "$f" 10 "DNF  DNF  DNF  DNF  DNF  DNF  SIGSEGV"
        elif [ "$f" != "RSA-2048 (PSS)" ]; then
            printf '%-28s  %5s  %14s  %11s  %14s  %11s  %14s  %11s\\n' "$f" 10 1 1.0 1 2.0 1 3.0
        fi
    done
} > "$out"
"""


@pytest.fixture
def fakes(tmp_path):
    suite = make_suite(tmp_path)
    state = tmp_path / "state"
    state.mkdir()
    bench = tmp_path / "fake-aws-bench.sh"
    bench.write_text(FAKE_AWS_BENCH)
    bench.chmod(0o755)
    aws = tmp_path / "fake-aws"
    aws.write_text('#!/bin/sh\necho "$2" >> "$FAKE_STATE/aws-calls"\n')
    aws.chmod(0o755)
    env = dict(os.environ, FAKE_STATE=str(state), FAKE_RESULTS=str(suite / "results"))
    args = [
        sys.executable,
        str(SCRIPT),
        "--suite",
        "sigs",
        "--repo",
        str(tmp_path),
        "--aws-bench",
        str(bench),
        "--aws",
        str(aws),
        "--run-dir",
        str(tmp_path / "run"),
        "--stagger",
        "0",
        "--poll",
        "0.05",
        "--retry-delay",
        "0",
        "--bin-minutes",
        "0.1",  # one row per group
    ]
    return tmp_path, suite, state, env, args


def test_launcher_retries_refused_groups_and_merges(fakes):
    tmp, suite, state, env, args = fakes
    rerun = tmp / "rerun.txt"
    rerun.write_text(results([row("Alpha-1", 10, "9.0", "9.0", "9.0")]))
    env["REFUSE"] = "1"
    r = subprocess.run(
        [*args, "--max-instances", "2", "--rerun-first", str(rerun)],
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    out = r.stdout
    # RSA-2048 (PSS) never produces a row in the fake: missing, so exit 1.
    assert r.returncode == 1, out + r.stderr
    assert "3 done" in out
    logs = sorted((tmp / "run").glob("group-*.log"))
    assert len(logs) == 3
    for log in logs:
        text = log.read_text()
        assert "=== attempt 1:" in text and "=== attempt 2:" in text
        assert "=== attempt 3:" not in text
    merged = suite / "results" / "20261013T120020Z_test_cpu_split_merged.txt"
    text = merged.read_text()
    # The rerun came first: its Alpha-1 row wins.
    assert "9.0" in [ln for ln in text.splitlines() if ln.startswith("Alpha-1")][0]
    assert "DNF rows (1):\n  Alpha-2  SIGSEGV" in out
    assert "planned rows missing from the merged file (1):\n  RSA-2048 (PSS)" in out
    assert (state / "aws-calls").read_text().split() == [
        "describe-instances",
        "describe-security-groups",
        "describe-key-pairs",
    ]


def test_launcher_gives_up_after_max_retries(fakes):
    tmp, suite, state, env, args = fakes
    env["REFUSE"] = "99"
    r = subprocess.run(
        [*args, "--max-retries", "2", "Alpha-1"],
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert r.returncode == 1
    assert "gave up after 3 attempts" in r.stdout
    assert "nothing to merge" in r.stdout


def test_dry_run_launches_nothing(fakes):
    tmp, suite, state, env, args = fakes
    r = subprocess.run([*args, "--dry-run"], env=env, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    assert "commands:" in r.stdout and "'RSA-2048 (PSS)'" in r.stdout
    assert not (tmp / "run").exists()
    assert list(state.iterdir()) == []


def test_ctrl_c_reaches_every_aws_bench_and_its_children(fakes):
    tmp, suite, state, env, args = fakes
    env["FAKE_HANG"] = "1"
    p = subprocess.Popen(args, env=env, stdout=subprocess.PIPE, text=True)
    try:
        deadline = time.time() + 30
        while len(list(state.glob("child-*"))) < 3:
            assert time.time() < deadline, "fake groups did not start"
            time.sleep(0.05)
        p.send_signal(signal.SIGINT)
        out, _ = p.communicate(timeout=30)
    finally:
        p.kill()
    assert p.returncode == 130, out
    assert len(list(state.glob("cleaned-*"))) == 3
    for child in state.glob("child-*"):
        pid = int(child.read_text())
        with pytest.raises(ProcessLookupError):
            os.kill(pid, 0)
    assert "forwarding to every aws-bench.sh" in out
    assert "leftover AWS resources" in out


def test_script_is_executable_and_documents_usage():
    assert os.access(SCRIPT, os.X_OK)
    r = subprocess.run(
        [sys.executable, str(SCRIPT), "--help"], capture_output=True, text=True
    )
    assert r.returncode == 0 and "--dry-run" in r.stdout
    assert textwrap.dedent(S.__doc__).strip().startswith("Split one benchmark suite")


# --- staleness -------------------------------------------------------------------------


def record_fresh(suite, names, instance_type="c8i.4xlarge"):
    """fingerprints.tsv saying `names` were measured with today's inputs."""
    FP = S.FP
    FP._cache.clear()
    fps = FP.suite_fingerprints(suite.parent, "bench")
    pv = FP.Provenance("abc1234", instance_type, "CPU", "2026-10-13T00:00:00Z", "r.txt")
    FP.write_fingerprints(
        FP.fingerprints_path(suite.parent, "bench"),
        {n: FP.entry(n, fps[n], "ok", pv, "r.txt") for n in names},
    )


def test_only_stale_rows_are_planned_unless_all(fakes):
    tmp, suite, state, env, args = fakes
    record_fresh(suite, ["Alpha-1", "RSA-2048 (PSS)"])
    (suite / "results" / "20261013T000000Z_cpu.txt").write_text(
        results([row("Alpha-2", 10, "1.0", "1.0", "1.0")])
    )
    r = subprocess.run([*args, "--dry-run"], env=env, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    assert "1 of 3 selected rows need a run on c8i.4xlarge" in r.stdout
    assert "2 NGCC rows also need a run (they need --ngcc-src)" in r.stdout
    cmds = r.stdout.split("commands:")[1].strip().splitlines()
    assert len(cmds) == 1 and cmds[0].endswith(" Alpha-2")
    assert "[no timing]" in r.stdout

    every = subprocess.run(
        [*args, "--dry-run", "--all"], env=env, capture_output=True, text=True
    )
    assert len(every.stdout.split("commands:")[1].strip().splitlines()) == 3

    # A filter only narrows the stale set.
    fresh = subprocess.run(
        [*args, "--dry-run", "Alpha-1"], env=env, capture_output=True, text=True
    )
    assert fresh.returncode == 0 and "nothing to run" in fresh.stdout
    assert "commands:" not in fresh.stdout


def test_stale_report_gives_reasons(fakes):
    tmp, suite, state, env, args = fakes
    record_fresh(suite, ["Alpha-1", "Alpha-2"], instance_type="c7i.4xlarge")
    record = S.FP.read_fingerprints(S.FP.fingerprints_path(tmp, "bench"))
    record["RSA-2048 (PSS)"] = dict(record["Alpha-1"], name="RSA-2048 (PSS)")
    S.FP.write_fingerprints(S.FP.fingerprints_path(tmp, "bench"), record)
    params = suite / "schemes" / "a" / "params.tsv"
    params.write_text(
        params.read_text().replace("a2\tAlpha-2\t1\t10", "a2\tAlpha-2\t2\t10")
    )
    r = subprocess.run(
        [*args, "--stale-report", "--instance-type", "c7i.4xlarge"],
        env=env,
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0, r.stderr
    out = r.stdout
    assert "sigs: 4 of 5 rows need a run on c7i.4xlarge; 1 up to date" in out
    lines = {
        ln.split("  ")[1].strip(): ln.rsplit("  ", 1)[1] for ln in out.splitlines()[2:]
    }
    assert lines["Alpha-2"] == "params row"
    assert lines["RSA-2048 (PSS)"] == "params row"  # Alpha-1's record: another row
    assert lines["NGCC-X-I"] == "new row"  # NGCC rows are reported without --ngcc-src
    assert "Alpha-1" not in lines
    c8i = subprocess.run(
        [*args, "--stale-report", "Alpha-1"], env=env, capture_output=True, text=True
    )
    assert "Alpha-1  hardware: c7i.4xlarge → c8i.4xlarge" in c8i.stdout
    assert not list(state.iterdir())  # nothing launched


def test_no_code_sets_are_never_planned(tmp_path):
    (tmp_path / "data/schemes").mkdir(parents=True)
    (tmp_path / "bench").mkdir()
    (tmp_path / "bench/update_scheme_data.py").write_text(
        "BENCH_TO_YAML: dict[str, tuple[str, str]] = {\n"
        '    "Foo-1": ("Foo.yaml", "1"),\n    "Foo-2": ("Foo.yaml", "2"),\n}\n'
    )
    (tmp_path / "data/schemes/Foo.yaml").write_text(
        "name: Foo\nversions:\n- version: v1\n  parametersets:\n"
        "  - name: '1'\n    pk: 1\n    sig: 2\n    no_code: No implementation\n"
        "  - name: '2'\n    pk: 1\n    sig: 2\n    notes: no_code: in a note\n"
    )
    (tmp_path / "data/schemes/Bar.yaml").write_text(
        "name: Bar\nversions:\n- version: Round 1\n  programmes:\n    china-ngcc: round-1\n"
        "  parametersets:\n  - name: Bar-384\n    pk: 1\n    sig: 2\n"
        "    no_code: No implementation in the submission package\n"
    )
    assert S.no_code_rows(tmp_path, "sigs") == {"Foo-1", "NGCC-Bar-384"}


def test_real_data_marks_bit_384_no_code():
    assert "NGCC-BiT-384" in S.no_code_rows(REPO, "sigs")
    assert {"NGCC-ZEN-256-backup", "NGCC-BW_KEM_C384"} <= S.no_code_rows(REPO, "kems")
