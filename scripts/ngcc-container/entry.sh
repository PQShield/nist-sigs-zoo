#!/usr/bin/env bash
# Runs INSIDE the container (see scripts/ngcc-run). Copies the read-only repo
# at /repo into the writable /work and runs the command there. Nothing is
# exported from here: the host pulls /work afterwards via scripts/ngcc-container/export.py.
set -uo pipefail

mkdir -p /work
rsync -a \
    --exclude=/.git --exclude=/node_modules --exclude='/.ngcc-out*' --exclude=/dist \
    --exclude=/.svelte-kit --exclude=/round-1 --exclude=/static \
    --exclude=/bench-common/ngcc/src --exclude=/bench-common/ngcc/specs \
    /repo/ /work/ || { echo "ngcc-run: workspace copy failed" >&2; exit 125; }
# Extracted sources stay read-only mounts; link them in.
for d in src specs; do
    if [ -d "/repo/bench-common/ngcc/$d" ]; then
        ln -sfn "/repo/bench-common/ngcc/$d" "/work/bench-common/ngcc/$d"
    fi
done
cd /work || exit 125

"$@"
status=$?

# Stage text scratch files from /tmp (the plan writes some there) so the host
# can pull them with /work. The host exporter applies all the content checks.
mkdir -p /work/.ngcc-tmp
find /tmp -maxdepth 2 -type f \( -name '*.tsv' -o -name '*.txt' -o -name '*.log' -o -name '*.rsp' \) \
    -not -name '.ngcc-marker' -exec cp -t /work/.ngcc-tmp {} + 2>/dev/null
exit "$status"
