#!/usr/bin/env bash
# Run bench/ (signatures) and/or bench-kem/ (KEMs) on a throwaway EC2 spot instance.
#
# Usage: scripts/aws-bench.sh [options] [filter...]
#
#   --suite sigs|kems|both   which benchmark(s) to run            (default: both)
#   --instance-type TYPE     EC2 instance type (x86-64 with AVX2) (default: c7i.4xlarge;
#                            use e.g. c7i.metal-24xl to avoid noisy neighbours)
#   --ubuntu VERSION         Ubuntu LTS release for the AMI       (default: 26.04)
#   --ttl MINUTES            hard lifetime; the VM powers off and self-terminates
#                            after this, even if this script dies (default: 60)
#   -h, --help               show this help
#
#   filter...                passed to run_bench.sh (case-insensitive, OR-ed)
#
# AWS permissions: scripts/aws-bench-iam-policy.json (least-privilege IAM policy).
#
# Environment:
#   AWS_PROFILE / AWS_REGION        select account and region (standard AWS CLI)
#   BENCH_ITER, BENCH_CYCLES,       forwarded to the benchmark on the VM
#   BENCH_CPU
#
# What it does:
#   1. Creates a per-run SSH key pair and a security group allowing SSH from
#      this machine's public IP only.
#   2. Launches a one-time spot instance (Ubuntu LTS, amd64) with
#      shutdown-behaviour=terminate and `shutdown -h +TTL` in user-data, so the
#      instance destroys itself after TTL minutes no matter what.
#   3. Clones the public repo + submodules on the VM at the local HEAD commit.
#      Local commits not yet on origin are shipped as an incremental git bundle.
#      Uncommitted changes are NOT benchmarked (the script refuses to run).
#   4. Builds and runs run_bench.sh in the background, polling for completion.
#   5. Copies bench/results/ and bench-kem/results/ back (also partial results on
#      failure/timeout), then terminates the instance and deletes the key pair
#      and security group.
#
# Cycle counts: bench/ and bench-kem/ read the real core-cycle PMC via rdpmc.
# Recent Nitro types expose a virtual PMU (verified on c7i.4xlarge); where it
# is missing they silently fall back to rdtsc reference cycles, and this
# script warns if the fetched results say so.
set -euo pipefail

SUITE=both
INSTANCE_TYPE=c7i.4xlarge
TTL_MIN=60
UBUNTU=26.04
FILTERS=()

usage() { sed -n '2,/^set -euo/{/^set -euo/d;s/^# \{0,1\}//;p}' "$0"; }

while [ $# -gt 0 ]; do
    case "$1" in
        --suite)         SUITE="$2"; shift 2 ;;
        --instance-type) INSTANCE_TYPE="$2"; shift 2 ;;
        --ttl)           TTL_MIN="$2"; shift 2 ;;
        --ubuntu)        UBUNTU="$2"; shift 2 ;;
        -h|--help)       usage; exit 0 ;;
        --)              shift; FILTERS+=("$@"); break ;;
        -*)              echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
        *)               FILTERS+=("$1"); shift ;;
    esac
done

case "$SUITE" in
    sigs|kems|both) ;;
    *) echo "--suite must be sigs, kems or both" >&2; exit 2 ;;
esac
[[ "$TTL_MIN" =~ ^[0-9]+$ ]] && [ "$TTL_MIN" -ge 10 ] \
    || { echo "--ttl must be an integer >= 10" >&2; exit 2; }

log() { printf '\033[1m[aws-bench %s]\033[0m %s\n' "$(date +%H:%M:%S)" "$*" >&2; }
die() { log "ERROR: $*"; exit 1; }

for cmd in aws ssh ssh-keygen rsync git curl; do
    command -v "$cmd" >/dev/null || die "'$cmd' not found in PATH (install the AWS CLI v2 for 'aws')"
done

REPO_ROOT="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
cd "$REPO_ROOT"

# --- decide what source the VM should build -----------------------------------

if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    die "working tree has uncommitted changes; commit them first (the VM builds HEAD)"
fi
if git submodule status --recursive | grep -q '^+'; then
    die "submodules are not at the commits recorded in HEAD; commit or 'git submodule update' first"
fi
HEAD_SHA="$(git rev-parse HEAD)"
# The VM clones anonymously over HTTPS, so origin must be a public repo.
REPO_URL="$(git remote get-url origin | sed -E 's#^git@([^:]+):#https://\1/#')"

aws sts get-caller-identity >/dev/null 2>&1 \
    || die "AWS CLI is not authenticated (check AWS_PROFILE / 'aws sso login')"
REGION="$(aws configure get region 2>/dev/null || true)"
REGION="${AWS_REGION:-${AWS_DEFAULT_REGION:-$REGION}}"
[ -n "$REGION" ] || die "no AWS region configured; set AWS_REGION"
export AWS_REGION="$REGION" AWS_PAGER=""

RUN_ID="nist-sigs-zoo-bench-$(date -u +%Y%m%dT%H%M%SZ)-$$"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/aws-bench.XXXXXX")"
KEY="$WORK/id_ed25519"
KNOWN_HOSTS="$WORK/known_hosts"

# --- cleanup (always runs) ------------------------------------------------------

INSTANCE_ID=""
SG_ID=""
KEY_CREATED=0
IP=""
FETCHED=0

# ControlPath lives in /tmp: $WORK may exceed the ~104-byte socket path limit.
ssh_opts=(-i "$KEY" -o UserKnownHostsFile="$KNOWN_HOSTS" -o StrictHostKeyChecking=accept-new
          -o ControlMaster=auto -o "ControlPath=/tmp/aws-bench-%C" -o ControlPersist=120
          -o ConnectTimeout=10 -o ServerAliveInterval=30 -o ServerAliveCountMax=4
          -o BatchMode=yes -o LogLevel=ERROR)
rssh() { ssh "${ssh_opts[@]}" "ubuntu@$IP" "$@"; }

fetch_results() {
    [ -n "$IP" ] || return 0
    FETCHED=1
    local suite dir
    for suite in bench bench-kem; do
        dir="$REPO_ROOT/$suite/results"
        mkdir -p "$dir"
        rsync -a --ignore-existing -e "ssh ${ssh_opts[*]}" \
            "ubuntu@$IP:repo/$suite/results/" "$dir/" 2>/dev/null || true
    done
    rsync -a -e "ssh ${ssh_opts[*]}" "ubuntu@$IP:bench.log" "$WORK/bench.log" 2>/dev/null || true
}

instance_state() {
    aws ec2 describe-instances --instance-ids "$INSTANCE_ID" \
        --query 'Reservations[0].Instances[0].State.Name' --output text 2>/dev/null || echo unknown
}

cleanup() {
    local rc=$?
    trap - EXIT INT TERM
    set +e
    if [ "$FETCHED" = 0 ] && [ -n "$IP" ]; then
        log "fetching whatever results exist before teardown"
        fetch_results
    fi
    if [ -n "$INSTANCE_ID" ]; then
        log "terminating $INSTANCE_ID"
        aws ec2 terminate-instances --instance-ids "$INSTANCE_ID" >/dev/null
        aws ec2 wait instance-terminated --instance-ids "$INSTANCE_ID"
    fi
    if [ -n "$SG_ID" ]; then
        log "deleting security group $SG_ID"
        # ENI detachment can lag the 'terminated' state by a few seconds
        for _ in 1 2 3 4 5 6 7 8 9 10 11 12; do
            aws ec2 delete-security-group --group-id "$SG_ID" 2>/dev/null && break
            sleep 10
        done
    fi
    if [ "$KEY_CREATED" = 1 ]; then
        aws ec2 delete-key-pair --key-name "$RUN_ID" >/dev/null
    fi
    [ -n "$IP" ] && ssh "${ssh_opts[@]}" -O exit "ubuntu@$IP" 2>/dev/null
    if [ -f "$WORK/bench.log" ]; then
        find "$WORK" -mindepth 1 ! -name bench.log -delete
        log "remote log kept at $WORK/bench.log"
    else
        rm -rf "$WORK"
    fi
    if [ -n "$INSTANCE_ID" ] || [ -n "$SG_ID" ]; then
        log "cleanup done (resources tagged '$RUN_ID')"
    fi
    exit "$rc"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

# --- local commits not on origin travel as a git bundle ------------------------

BUNDLE=""
git fetch --quiet origin || log "warning: 'git fetch origin' failed; using cached remote refs"
if [ -z "$(git branch -r --contains "$HEAD_SHA" 2>/dev/null)" ]; then
    BUNDLE="$WORK/local.bundle"
    log "HEAD $HEAD_SHA is not on origin; bundling local commits"
    git bundle create "$BUNDLE" HEAD --not --remotes=origin >/dev/null 2>&1 \
        || die "failed to create git bundle of local commits"
fi

# --- ephemeral key pair + security group ---------------------------------------

ssh-keygen -q -t ed25519 -N "" -C "$RUN_ID" -f "$KEY"
aws ec2 import-key-pair --key-name "$RUN_ID" \
    --public-key-material "fileb://$KEY.pub" \
    --tag-specifications "ResourceType=key-pair,Tags=[{Key=Name,Value=$RUN_ID}]" >/dev/null
KEY_CREATED=1

MY_IP="$(curl -fsS https://checkip.amazonaws.com | tr -d '[:space:]')"
[[ "$MY_IP" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "could not determine public IPv4 (got '$MY_IP')"

VPC_ID="$(aws ec2 describe-vpcs --filters Name=is-default,Values=true \
    --query 'Vpcs[0].VpcId' --output text)"
[ "$VPC_ID" != "None" ] || die "no default VPC in $REGION"

SG_ID="$(aws ec2 create-security-group --group-name "$RUN_ID" --vpc-id "$VPC_ID" \
    --description "ephemeral SSH for $RUN_ID" \
    --tag-specifications "ResourceType=security-group,Tags=[{Key=Name,Value=$RUN_ID}]" \
    --query GroupId --output text)"
aws ec2 authorize-security-group-ingress --group-id "$SG_ID" \
    --protocol tcp --port 22 --cidr "$MY_IP/32" >/dev/null

# --- AMI + candidate subnets ---------------------------------------------------

AMI_ID="$(aws ssm get-parameter \
    --name /aws/service/canonical/ubuntu/server/$UBUNTU/stable/current/amd64/hvm/ebs-gp3/ami-id \
    --query Parameter.Value --output text 2>/dev/null || true)"
if [ -z "$AMI_ID" ] || [ "$AMI_ID" = "None" ]; then
    AMI_ID="$(aws ec2 describe-images --owners 099720109477 \
        --filters "Name=name,Values=ubuntu/images/hvm-ssd-gp3/ubuntu-*-$UBUNTU-amd64-server-*" \
                  'Name=state,Values=available' \
        --query 'sort_by(Images,&CreationDate)[-1].ImageId' --output text)"
fi
[ -n "$AMI_ID" ] && [ "$AMI_ID" != "None" ] || die "could not resolve Ubuntu $UBUNTU amd64 AMI"

aws ec2 describe-instance-types --instance-types "$INSTANCE_TYPE" >/dev/null 2>&1 \
    || die "unknown instance type '$INSTANCE_TYPE' in $REGION"
mapfile -t AZS < <(aws ec2 describe-instance-type-offerings --location-type availability-zone \
    --filters "Name=instance-type,Values=$INSTANCE_TYPE" \
    --query 'InstanceTypeOfferings[].Location' --output text | tr '\t' '\n' | sed '/^$/d')
[ "${#AZS[@]}" -gt 0 ] || die "$INSTANCE_TYPE is not offered in any AZ of $REGION"

AZ_LIST="$(IFS=,; echo "${AZS[*]}")"
mapfile -t SUBNETS < <(aws ec2 describe-subnets \
    --filters "Name=vpc-id,Values=$VPC_ID" Name=default-for-az,Values=true \
              "Name=availability-zone,Values=$AZ_LIST" \
    --query 'Subnets[].SubnetId' --output text | tr '\t' '\n' | sed '/^$/d')
[ "${#SUBNETS[@]}" -gt 0 ] || die "no default subnet in an AZ offering $INSTANCE_TYPE ($AZ_LIST)"

# --- launch ----------------------------------------------------------------------

USER_DATA="$WORK/user-data.sh"
cat >"$USER_DATA" <<EOF
#!/bin/bash
# Dead-man switch: power off (=> terminate) after the TTL no matter what.
shutdown -h +$TTL_MIN "aws-bench TTL reached"
# Ubuntu defaults to 4, which blocks unprivileged perf_event_open (rdpmc).
sysctl -w kernel.perf_event_paranoid=1
export DEBIAN_FRONTEND=noninteractive
apt-get -o DPkg::Lock::Timeout=600 update -q
apt-get -o DPkg::Lock::Timeout=600 install -y -q --no-install-recommends \\
    build-essential git python3 cmake meson ninja-build libgmp-dev libssl-dev zlib1g-dev libzstd-dev openssl pkg-config \\
    curl ca-certificates tar unzip xz-utils rsync util-linux
# Ubuntu >= 26.04's static libcrypto.a links jitterentropy; absent on older releases.
apt-get -o DPkg::Lock::Timeout=600 install -y -q --no-install-recommends libjitterentropy3-dev || true
EOF

log "launching spot $INSTANCE_TYPE in $REGION (Ubuntu $UBUNTU, AMI $AMI_ID, TTL ${TTL_MIN}m)"
for subnet in "${SUBNETS[@]}"; do
    if out="$(aws ec2 run-instances \
        --image-id "$AMI_ID" --instance-type "$INSTANCE_TYPE" --count 1 \
        --key-name "$RUN_ID" --subnet-id "$subnet" --security-group-ids "$SG_ID" \
        --associate-public-ip-address \
        --instance-market-options 'MarketType=spot,SpotOptions={SpotInstanceType=one-time,InstanceInterruptionBehavior=terminate}' \
        --instance-initiated-shutdown-behavior terminate \
        --block-device-mappings 'DeviceName=/dev/sda1,Ebs={VolumeSize=40,VolumeType=gp3,DeleteOnTermination=true}' \
        --metadata-options HttpTokens=required \
        --user-data "file://$USER_DATA" \
        --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=$RUN_ID}]" \
                             "ResourceType=volume,Tags=[{Key=Name,Value=$RUN_ID}]" \
        --query 'Instances[0].InstanceId' --output text 2>&1)"; then
        INSTANCE_ID="$out"
        break
    fi
    log "launch in $subnet failed: $(echo "$out" | tail -1)"
done
[ -n "$INSTANCE_ID" ] || die "could not launch spot $INSTANCE_TYPE in any AZ (capacity or quota?)"
DEADLINE=$(( $(date +%s) + TTL_MIN * 60 - 180 ))
log "instance $INSTANCE_ID launched; waiting for it to run"

aws ec2 wait instance-running --instance-ids "$INSTANCE_ID"
IP="$(aws ec2 describe-instances --instance-ids "$INSTANCE_ID" \
    --query 'Reservations[0].Instances[0].PublicIpAddress' --output text)"
[ -n "$IP" ] && [ "$IP" != "None" ] || die "instance has no public IP"

log "waiting for SSH on $IP "
until rssh true 2>/dev/null; do
    [ "$(date +%s)" -lt "$DEADLINE" ] || die "SSH never came up"
    sleep 10
done

log "waiting for cloud-init (package install)"
if ! rssh 'cloud-init status --wait >/dev/null; cloud-init status | grep -q "status: done"'; then
    rssh 'sudo tail -n 40 /var/log/cloud-init-output.log' >&2 || true
    die "cloud-init failed"
fi

# --- fetch source on the VM -----------------------------------------------------

if [ -n "$BUNDLE" ]; then
    rsync -a -e "ssh ${ssh_opts[*]}" "$BUNDLE" "ubuntu@$IP:local.bundle"
fi

SUBMODULE_PATHS=()
case "$SUITE" in
    sigs) SUBMODULE_PATHS=(bench/schemes) ;;
    kems) SUBMODULE_PATHS=(bench-kem/schemes) ;;
    both) SUBMODULE_PATHS=(bench/schemes bench-kem/schemes) ;;
esac

log "cloning $REPO_URL @ ${HEAD_SHA:0:12} on the VM"
rssh bash -s -- "$REPO_URL" "$HEAD_SHA" "${SUBMODULE_PATHS[@]}" <<'EOF'
set -euo pipefail
url="$1"; sha="$2"; shift 2
git clone -q --no-checkout "$url" repo
cd repo
if [ -f ~/local.bundle ]; then
    git fetch -q ~/local.bundle HEAD
fi
git -c advice.detachedHead=false checkout -q "$sha"
# Anonymous HTTPS only: some upstreams record git@github.com: URLs.
git config --global url."https://github.com/".insteadOf git@github.com:
# Top-level scheme submodules only; nested ones (e.g. SABER's pqm4) are
# embedded-target code the x86 build never uses.
git submodule update -q --init --jobs 8 -- "$@"
EOF

# --- build + run in the background -----------------------------------------------

REMOTE_ENV=""
for v in BENCH_ITER BENCH_CYCLES BENCH_CPU; do
    if [ -n "${!v:-}" ]; then REMOTE_ENV+="export $v=$(printf '%q' "${!v}"); "; fi
done
FILTER_ARGS=""
if [ "${#FILTERS[@]}" -gt 0 ]; then FILTER_ARGS="$(printf '%q ' "${FILTERS[@]}")"; fi

RUNNER="$WORK/run.sh"
{
    echo '#!/usr/bin/env bash'
    echo 'set -uo pipefail'
    echo "$REMOTE_ENV"
    echo 'cd ~/repo'
    echo 'rc=0'
    echo 'nproc_=$(nproc)'
    for s in bench bench-kem; do
        case "$SUITE:$s" in sigs:bench-kem|kems:bench) continue ;; esac
        cat <<EOF
echo "=== building $s ==="
# parallel build first; fall back to serial if a Makefile is not -j safe.
# -k: build every scheme that can be built, report all failures at once
make -k -C $s -j"\$nproc_" || make -k -C $s || { echo "=== $s build FAILED ==="; rc=1; }
if [ -x $s/$s ]; then
    echo "=== running $s ==="
    $s/run_bench.sh $FILTER_ARGS || rc=1
fi
EOF
    done
    echo 'echo "$rc" > ~/bench.done'
} >"$RUNNER"
rsync -a -e "ssh ${ssh_opts[*]}" "$RUNNER" "ubuntu@$IP:run.sh"
rssh 'touch .bench-start; chmod +x run.sh; setsid -f ./run.sh > bench.log 2>&1 < /dev/null'

log "benchmark running; streaming bench.log"
offset=0
REMOTE_RC=""
ssh_failures=0
while :; do
    # stream new log output by byte offset; one ssh round-trip returns
    # "<done-rc or ->\t<log size>" and the chunk after the previous offset
    if status="$(rssh "printf '%s\t%s\n' \"\$(cat bench.done 2>/dev/null || echo -)\" \$(stat -c %s bench.log)" 2>/dev/null)"; then
        ssh_failures=0
        size="${status#*$'\t'}"
        if [ "$size" -gt "$offset" ]; then
            rssh "tail -c +$((offset + 1)) bench.log | head -c $((size - offset))" 2>/dev/null || true
            offset="$size"
        fi
        rc_field="${status%%$'\t'*}"
        if [ "$rc_field" != "-" ]; then REMOTE_RC="$rc_field"; break; fi
    else
        ssh_failures=$((ssh_failures + 1))
        if [ "$ssh_failures" -ge 3 ]; then
            state="$(instance_state)"
            [ "$state" = running ] || die "instance is '$state' (spot interruption?); no results fetched"
        fi
    fi
    if [ "$(date +%s)" -ge "$DEADLINE" ]; then
        log "TTL nearly reached; fetching partial results"
        break
    fi
    sleep 30
done

fetch_results
log "results copied into bench/results/ and bench-kem/results/"

# rdpmc vs rdtsc sanity check on what we just fetched
if rssh 'find repo/bench/results repo/bench-kem/results -name "*.txt" -newer .bench-start \
             -exec grep -h "^# cyclecounter:" {} +' 2>/dev/null | grep -qv rdpmc; then
    log "WARNING: some results fell back to rdtsc reference cycles (no PMU access on $INSTANCE_TYPE)"
fi

if [ "$REMOTE_RC" != "0" ]; then
    die "benchmark did not complete cleanly (rc=${REMOTE_RC:-timeout}); see $WORK/bench.log"
fi
rm -f "$WORK/bench.log"
log "done. Review the new results files, then run update_scheme_data.py to import them."
