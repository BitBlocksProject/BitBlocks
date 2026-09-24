#!/usr/bin/env bash
# Build the Linux and Windows releases inside the pinned Ubuntu 20.04
# container, like PIVX's gitian builds.
#
# Usage: contrib/release/docker-release.sh [host...]
# Hosts default to x86_64-pc-linux-gnu and x86_64-w64-mingw32; the aliases
# "linux" and "win64" are accepted too.

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
IMAGE=${IMAGE:-bitblocks-release-builder:focal}
CACHE_VOLUME=${CACHE_VOLUME:-bitblocks-release-depends}
JOBS=${JOBS:-$(nproc)}
OUTPUT_DIR=${OUTPUT_DIR:-"$ROOT_DIR/release"}

HOSTS=()
[ $# -gt 0 ] || set -- linux win64
for host in "$@"; do
    case "$host" in
        linux) HOSTS+=(x86_64-pc-linux-gnu) ;;
        win64|windows) HOSTS+=(x86_64-w64-mingw32) ;;
        *) HOSTS+=("$host") ;;
    esac
done

docker build -t "$IMAGE" "$ROOT_DIR/contrib/release"

mkdir -p "$OUTPUT_DIR" "$ROOT_DIR/depends/sources"

# Version info and timestamps come from the real git checkout; the container
# only ever sees a clean export of the tracked (and new, unignored) files.
BUILD_INFO=$(mktemp)
trap 'rm -f "$BUILD_INFO"' EXIT
"$ROOT_DIR/share/genbuild.sh" "$BUILD_INFO" "$ROOT_DIR"
SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$ROOT_DIR" log -1 --format=%ct)}

for host in "${HOSTS[@]}"; do
    echo "=== Building release for $host"
    (
        cd "$ROOT_DIR"
        git ls-files -co --exclude-standard -z |
            tar --null --no-recursion --files-from=- -cf -
    ) | docker run --rm -i \
        -v "$CACHE_VOLUME:/cache" \
        -v "$ROOT_DIR/depends/sources:/sources" \
        -v "$OUTPUT_DIR:/out" \
        -v "$BUILD_INFO:/build.h:ro" \
        -e HOST="$host" \
        -e JOBS="$JOBS" \
        -e MAX_GLIBC="${MAX_GLIBC:-2.31}" \
        -e SOURCE_DATE_EPOCH="$SOURCE_DATE_EPOCH" \
        -e SOURCES_PATH=/sources \
        -e BASE_CACHE=/cache/built \
        -e OUTPUT_DIR=/out \
        -e BUILD_INFO_FILE=/build.h \
        -e OUT_UID="$(id -u)" \
        -e OUT_GID="$(id -g)" \
        "$IMAGE" \
        bash -euo pipefail -c '
            mkdir -p /work/BitBlocks
            tar --no-same-owner -C /work/BitBlocks -xf -
            /work/BitBlocks/contrib/release/package.sh
            chown "$OUT_UID:$OUT_GID" /out/* /sources/* 2>/dev/null || true
        '
done

echo "Release artifacts in $OUTPUT_DIR:"
ls -1 "$OUTPUT_DIR"
