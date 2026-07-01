#!/usr/bin/env bash
# Fetch + build the libntruprime static library and its randombytes dependency
# into a local prefix, then harvest the .a archives the shims link against.
# Idempotent: re-running is a no-op once build/local/.done exists.
#
# libntruprime (Bernstein) packages the SUPERCOP-optimised Streamlined NTRU Prime
# code as a clean ./configure + make library — the same shape as libmceliece. The
# KEM contract is satisfied by statically linking libntruprime.a (+
# librandombytes-kernel.a for its RNG) into each per-parameter-set shim .so, so no
# runtime shared-library deps leak out. AVX2 is selected at runtime via the
# library's IFUNC dispatch (e.g. crypto_sort/int32/avx2).
#
# libcpucycles is not needed: like libmceliece it is referenced only by the
# benchmark CLI, never the library, so we build the package/lib/libntruprime.a
# target directly and never the CLI binaries.
set -euo pipefail

# --- pinned upstream versions + SHA-256 (update together) ---
RB_VER=20240318;  RB_URL="https://randombytes.cr.yp.to/librandombytes-${RB_VER}.tar.gz"
RB_SHA=fae6fb839096e54ce8abb6dc8ae46ed67b02034474e83cbda088eddd2e584641
NP_VER=20241021;  NP_URL="https://libntruprime.cr.yp.to/libntruprime-${NP_VER}.tar.gz"
NP_SHA=1b027c1420554ab8282db309815f3265196d24bad89cf3bf7ee67915fa47e3dc

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
fetch "$NP_URL" "$NP_SHA" np.tar.gz

rm -rf "$SRC"/librandombytes-* "$SRC"/libntruprime-*
tar xzf "$DL/rb.tar.gz" -C "$SRC"
tar xzf "$DL/np.tar.gz" -C "$SRC"

# librandombytes: tiny djb lib, install into the local prefix.
echo "  build librandombytes"
( cd "$SRC/librandombytes-$RB_VER" && ./configure --prefix="$LOCAL" >/dev/null && make -j"$JOBS" install >/dev/null )

# libntruprime: --no-valgrind picks the no-op declassify (the valgrind variant
# hard-includes <valgrind/memcheck.h>). Build only the static archive target.
echo "  build libntruprime (this takes a minute)"
( cd "$SRC/libntruprime-$NP_VER" \
    && CPATH="$LOCAL/include" LIBRARY_PATH="$LOCAL/lib" ./configure --no-valgrind >/dev/null \
    && CPATH="$LOCAL/include" LIBRARY_PATH="$LOCAL/lib" make -j"$JOBS" -C build/0 package/lib/libntruprime.a >/dev/null )

NP_A="$SRC/libntruprime-$NP_VER/build/0/package/lib/libntruprime.a"
NP_H="$SRC/libntruprime-$NP_VER/build/0/package/include/ntruprime.h"
[ -f "$NP_A" ] || { echo "libntruprime.a not produced — build failed" >&2; exit 1; }
cp "$NP_A" "$LOCAL/lib/libntruprime.a"
cp "$NP_H" "$LOCAL/include/ntruprime.h"

touch "$LOCAL/.done"
echo "  libntruprime ready: $LOCAL/lib/libntruprime.a"
