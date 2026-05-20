#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -x ./bench ]; then
    echo "bench binary not found; building..." >&2
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

# slug for filename: lowercase model, spaces/special chars → underscore
CPU_SLUG=$(echo "$CPU_MODEL" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/_/g' | sed 's/__*/_/g' | sed 's/_$//')
TIMESTAMP=$(date -u +"%Y%m%dT%H%M%SZ")
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
OUTFILE="$RESULTS_DIR/${TIMESTAMP}_${CPU_SLUG}.txt"

# --- write header to output file, tee bench output ---

{
    echo "# pq-sigs benchmark results"
    echo "# date:         $DATE"
    echo "# host:         $HOSTNAME"
    echo "# os:           $OS"
    echo "# kernel:       $KERNEL"
    echo "# cpu:          $CPU_MODEL"
    echo "# cores:        $CPU_CORES (threads/core: ${CPU_THREADS_PER_CORE:-?})"
    echo "# max_freq_mhz: $(( CPU_MAXFREQ_KHZ == 0 ? 0 : CPU_MAXFREQ_KHZ / 1000 )) (cur: $(( CPU_CURFREQ_KHZ == 0 ? 0 : CPU_CURFREQ_KHZ / 1000 )) MHz)"
    echo "# governor:     $CPU_GOVERNOR"
    echo "# turbo:        $TURBO"
    echo "# flags:        $CPU_FLAGS"
    echo "#"
} | tee "$OUTFILE"

./bench 2>&1 | tee -a "$OUTFILE"

echo "" | tee -a "$OUTFILE"
echo "# saved to $OUTFILE" >&2
