# Throwaway, network-less build/run environment for untrusted NGCC candidate C
# code. Used only through scripts/ngcc-run. Base: ubuntu:26.04 (pullable;
# matches the AWS benchmark VM). The package list mirrors the user-data
# apt-get list in scripts/aws-bench.sh, plus libarchive-tools (bsdtar, RAR).
FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -q \
 && apt-get install -y -q --no-install-recommends \
    build-essential git python3 cmake meson ninja-build libgmp-dev libssl-dev \
    zlib1g-dev libzstd-dev openssl pkg-config curl ca-certificates tar unzip \
    xz-utils rsync util-linux libarchive-tools \
 && (apt-get install -y -q --no-install-recommends libjitterentropy3-dev || true) \
 && rm -rf /var/lib/apt/lists/*

# uv, pinned by version and by the sha256 of the release tarball (from the release's
# .sha256 files); the build fails on a mismatch. Bump all three together.
ARG UV_VERSION=0.12.8
ARG UV_SHA256_X86_64=2e2b37e9811e17675a9e70bed5e1a58fc8c0388be63d751d72cc735188c149ff
ARG UV_SHA256_AARCH64=ba8661f4fd207c8e94814191598e619b355ac10d5014e851e21eb800f9ef2b00
RUN set -eu; \
    arch="$(uname -m)"; \
    case "$arch" in \
      x86_64) sum="$UV_SHA256_X86_64" ;; \
      aarch64) sum="$UV_SHA256_AARCH64" ;; \
      *) echo "no pinned uv checksum for $arch" >&2; exit 1 ;; \
    esac; \
    tgz="uv-${arch}-unknown-linux-gnu.tar.gz"; \
    curl -fsSLo "/tmp/$tgz" "https://github.com/astral-sh/uv/releases/download/${UV_VERSION}/$tgz"; \
    echo "$sum  /tmp/$tgz" | sha256sum -c -; \
    tar -xzf "/tmp/$tgz" -C /usr/local/bin --strip-components=1 \
      "uv-${arch}-unknown-linux-gnu/uv" "uv-${arch}-unknown-linux-gnu/uvx"; \
    rm "/tmp/$tgz"; \
    uv --version

# Non-root user. Ubuntu images ship an "ubuntu" user at uid/gid 1000: drop it.
ARG UID=1000
ARG GID=1000
RUN (userdel -r ubuntu 2>/dev/null || true) \
 && (groupdel ubuntu 2>/dev/null || true) \
 && groupadd -g "${GID}" bench \
 && useradd -m -u "${UID}" -g "${GID}" -s /bin/bash bench

# Writable workspace (an anonymous volume at run time inherits this ownership).
RUN mkdir -p /work && chown bench:bench /work
ENV NGCC_BUILD=1
WORKDIR /work
USER bench
