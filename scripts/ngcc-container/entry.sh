#!/usr/bin/env bash
# Runs INSIDE the container (see scripts/ngcc-run). Copies the read-only repo
# at /repo into the writable /work, runs the command there, then exports only
# allowlisted text outputs to /out (the host's .ngcc-out/), exec bits stripped.
set -uo pipefail

mkdir -p /work
rsync -a \
    --exclude=/.git --exclude=/node_modules --exclude=/.ngcc-out --exclude=/dist \
    --exclude=/.svelte-kit --exclude=/round-1 --exclude=/static \
    --exclude=/bench-common/ngcc/src --exclude=/bench-common/ngcc/specs \
    /repo/ /work/ || { echo "ngcc-run: workspace copy failed" >&2; exit 125; }
# Extracted sources stay read-only mounts; link them in.
for d in src specs; do
    if [ -d "/repo/bench-common/ngcc/$d" ]; then
        ln -sfn "/repo/bench-common/ngcc/$d" "/work/bench-common/ngcc/$d"
    fi
done
touch /tmp/.ngcc-marker
sleep 1
cd /work || exit 125

"$@"
status=$?

export_files() { # root, relative-to-/out prefix, find expression
    local root="$1" prefix="$2"; shift 2
    [ -d "$root" ] || return 0
    (
        cd "$root" || exit 0
        find . -type f -newer /tmp/.ngcc-marker -size -64M \
            \( "$@" \) \
            -not -path './bench-common/ngcc/src/*' -not -path './bench-common/ngcc/specs/*' \
            -print0 |
            while IFS= read -r -d '' f; do
                dst="/out/${prefix}${f#./}"
                mkdir -p "$(dirname "$dst")" && cat -- "$f" >"$dst" && chmod 0644 "$dst"
            done
    )
}
export_files /work "" -path '*/schemes/ngcc/*.tsv' -o -name 'ngcc_sizes.tsv' \
    -o -path '*/results/*.txt' -o -name '*.log' \
    -o -path '*/build/*.rsp' -o -path '*/build/KAT*.txt'
# Scratch outputs the plan writes under /tmp: text only.
export_files /tmp "tmp/" -name '*.tsv' -o -name '*.txt' -o -name '*.log' -o -name '*.rsp'
exit "$status"
