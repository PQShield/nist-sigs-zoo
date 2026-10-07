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

# uv, pinned.
ARG UV_VERSION=0.12.8
RUN curl -LsSf "https://astral.sh/uv/${UV_VERSION}/install.sh" \
    | env UV_INSTALL_DIR=/usr/local/bin UV_NO_MODIFY_PATH=1 sh \
 && uv --version

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
