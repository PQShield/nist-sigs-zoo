#!/usr/bin/env bash
# Fetch + build the libmceliece static library and its randombytes dependency
# into a local prefix, then harvest the .a archives the shims link against.
# Idempotent: re-running is a no-op once build/local/.done exists.
#
# libmceliece is distributed as a signed tarball (no git), built with its own
# ./configure + make. The KEM contract is satisfied by statically linking the
# resulting libmceliece.a (+ librandombytes-kernel.a for its RNG) into each
# per-parameter-set shim .so, so no runtime shared-library deps leak out — the
# same self-contained-.so isolation every other scheme in bench-kem uses.
#
# libcpucycles is NOT needed: it is referenced only by command/mceliece-speed.c
# (the upstream's benchmark CLI), never by the library itself. We build the
# package/lib/libmceliece.a target directly and never the CLI binaries, so the
# dependency is dropped entirely.
set -euo pipefail

# --- pinned upstream versions + SHA-256 (update together) ---
RB_VER=20240318;  RB_URL="https://randombytes.cr.yp.to/librandombytes-${RB_VER}.tar.gz"
RB_SHA=fae6fb839096e54ce8abb6dc8ae46ed67b02034474e83cbda088eddd2e584641
MC_VER=20260622;  MC_URL="https://lib.mceliece.org/libmceliece-${MC_VER}.tar.gz"
MC_SHA=a30b93fcc51281881a31007c0b82cdfaff532a9d2dba87c7f99760f1f83266b7

DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD="$DIR/build"
DL="$BUILD/dl"; SRC="$BUILD/src"; LOCAL="$BUILD/local"
JOBS="$(nproc 2>/dev/null || echo 4)"

[ -f "$LOCAL/.done" ] && exit 0
mkdir -p "$DL" "$SRC" "$LOCAL"

fetch() { # url sha outfile
    local url="$1" sha="$2" out="$DL/$3"
    if [ ! -f "$out" ]; then
        echo "  fetch $url"
        curl -fsSL --compressed "$url" -o "$out"
    fi
    echo "$sha  $out" | sha256sum -c - >/dev/null \
        || { echo "SHA-256 mismatch for $out" >&2; exit 1; }
}

fetch "$RB_URL" "$RB_SHA" rb.tar.gz
fetch "$MC_URL" "$MC_SHA" mc.tar.gz

rm -rf "$SRC"/librandombytes-* "$SRC"/libmceliece-*
tar xzf "$DL/rb.tar.gz" -C "$SRC"
tar xzf "$DL/mc.tar.gz" -C "$SRC"

# librandombytes: tiny djb lib, install into the local prefix.
echo "  build librandombytes"
( cd "$SRC/librandombytes-$RB_VER" && ./configure --prefix="$LOCAL" >/dev/null && make -j"$JOBS" install >/dev/null )

# libmceliece: --no-valgrind picks the no-op declassify (the valgrind variant
# hard-includes <valgrind/memcheck.h>, only needed for the constant-time tests).
# Build only the package/lib/libmceliece.a target — not the default target, which
# also builds the CLI binaries (command/mceliece-speed.c needs libcpucycles) and
# would write outside the prefix. configure creates build/0 (-> build/amd64).
echo "  build libmceliece (this takes a minute)"
( cd "$SRC/libmceliece-$MC_VER" \
    && CPATH="$LOCAL/include" LIBRARY_PATH="$LOCAL/lib" ./configure --no-valgrind >/dev/null \
    && CPATH="$LOCAL/include" LIBRARY_PATH="$LOCAL/lib" make -j"$JOBS" -C build/0 package/lib/libmceliece.a >/dev/null )

MC_A="$SRC/libmceliece-$MC_VER/build/0/package/lib/libmceliece.a"
MC_H="$SRC/libmceliece-$MC_VER/build/0/package/include/mceliece.h"
[ -f "$MC_A" ] || { echo "libmceliece.a not produced — build failed" >&2; exit 1; }
cp "$MC_A" "$LOCAL/lib/libmceliece.a"
cp "$MC_H" "$LOCAL/include/mceliece.h"

touch "$LOCAL/.done"
echo "  libmceliece ready: $LOCAL/lib/libmceliece.a"
