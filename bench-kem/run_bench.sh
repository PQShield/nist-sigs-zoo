#!/usr/bin/env bash
# Usage: run_bench.sh [filter...]
#   Filters are case-insensitive substrings matched against scheme names.
#   Multiple filters are OR-ed: run_bench.sh rsa ecdsa  →  RSA + ECDSA only.
#   No filters → run all schemes.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -x ./bench-kem ]; then
    echo "bench-kem binary not found; building..." >&2
    make
fi

# --- collect system info ---

CPU_MODEL=$(grep -m1 "^model name" /proc/cpuinfo | cut -d: -f2 | sed 's/^ *//')
CPU_CORES=$(grep -c "^processor" /proc/cpuinfo)
CPU_THREADS_PER_CORE=$(lscpu 2>/dev/null | awk -F: '/Thread\(s\) per core/{gsub(/ /,"",$2); print $2}')
CPU_MAXFREQ_KHZ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq 2>/dev/null || echo "unknown")
CPU_CURFREQ_KHZ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null || echo "unknown")
CPU_GOVERNOR=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "unknown")

# Intel pstate turbo (0 = turbo enabled, 1 = disabled)
if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
    NO_TURBO=$(cat /sys/devices/system/cpu/intel_pstate/no_turbo)
    TURBO=$( [ "$NO_TURBO" = "0" ] && echo "enabled" || echo "disabled" )
elif [ -f /sys/devices/system/cpu/cpufreq/boost ]; then
    BOOST=$(cat /sys/devices/system/cpu/cpufreq/boost)
    TURBO=$( [ "$BOOST" = "1" ] && echo "enabled" || echo "disabled" )
else
    TURBO="unknown"
fi

CPU_FLAGS=$(grep -m1 "^flags" /proc/cpuinfo | cut -d: -f2 | tr -s ' ' | sed 's/^ //')
KERNEL=$(uname -r)
OS=$(. /etc/os-release 2>/dev/null && echo "$PRETTY_NAME" || uname -s)
HOSTNAME=$(hostname)
DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

# --- collect compiler + library versions ---

CC_VERSION=$(${CC:-cc} --version 2>&1 | head -1)
OPENSSL_VERSION=$(openssl version 2>/dev/null || echo "not found")

# Submodule sources: for each schemes/<name>/ref/, record url@commit
# Non-submodule sources: for each schemes/<name>/.source, record url from file
SUBMODULE_INFO=""
for ref_dir in "$SCRIPT_DIR"/schemes/*/ref; do
    [ -e "$ref_dir/.git" ] || continue
    scheme_path="${ref_dir#"$SCRIPT_DIR/"}"   # e.g. schemes/mldsa/ref
    url=$(git -C "$ref_dir" remote get-url origin 2>/dev/null || echo "unknown")
    commit=$(git -C "$ref_dir" rev-parse HEAD 2>/dev/null || echo "unknown")
    SUBMODULE_INFO="${SUBMODULE_INFO}#   ${scheme_path}: ${url}@${commit}"$'\n'
done
for src_file in "$SCRIPT_DIR"/schemes/*/.source; do
    [ -f "$src_file" ] || continue
    scheme_dir="${src_file%/.source}"
    scheme_path="${scheme_dir#"$SCRIPT_DIR/"}"   # e.g. schemes/sdith2
    url=$(head -1 "$src_file" | tr -d '[:space:]')
    SUBMODULE_INFO="${SUBMODULE_INFO}#   ${scheme_path}: ${url}"$'\n'
done

# slug for filename: lowercase model, spaces/special chars → underscore
CPU_SLUG=$(echo "$CPU_MODEL" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/_/g' | sed 's/__*/_/g' | sed 's/_$//')
TIMESTAMP=$(date -u +"%Y%m%dT%H%M%SZ")
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Append filter slug to filename when filters are specified
FILTER_SLUG=""
if [ $# -gt 0 ]; then
    FILTER_SLUG="_$(echo "$*" | tr ' ' '_' | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9_]//g')"
fi
OUTFILE="$RESULTS_DIR/${TIMESTAMP}_${CPU_SLUG}${FILTER_SLUG}.txt"

# --- write header to output file, tee bench output ---

_khz_to_mhz() { [ "$1" = "unknown" ] && echo "unknown" || echo "$(( $1 / 1000 ))"; }

{
    echo "# pq-kems benchmark results"
    echo "# date:         $DATE"
    echo "# host:         $HOSTNAME"
    echo "# os:           $OS"
    echo "# kernel:       $KERNEL"
    echo "# cpu:          $CPU_MODEL"
    echo "# cores:        $CPU_CORES (threads/core: ${CPU_THREADS_PER_CORE:-?})"
    echo "# max_freq_mhz: $(_khz_to_mhz "$CPU_MAXFREQ_KHZ") (cur: $(_khz_to_mhz "$CPU_CURFREQ_KHZ") MHz)"
    echo "# governor:     $CPU_GOVERNOR"
    echo "# turbo:        $TURBO"
    echo "# flags:        $CPU_FLAGS"
    echo "#"
    echo "# compiler:     $CC_VERSION"
    echo "# openssl:      $OPENSSL_VERSION"
    if [ -n "$SUBMODULE_INFO" ]; then
        echo "# sources:"
        printf "%s" "$SUBMODULE_INFO"
    fi
    if [ -n "${BENCH_ITER:-}" ]; then
        echo "# bench_iter:   $BENCH_ITER (env override)"
    fi
    if [ $# -gt 0 ]; then
        echo "# filter:       $*"
    fi
    echo "#"
} | tee "$OUTFILE"

./bench-kem "$@" 2>&1 | tee -a "$OUTFILE"

echo "" | tee -a "$OUTFILE"
echo "# saved to $OUTFILE" >&2
