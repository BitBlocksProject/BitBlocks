#!/usr/bin/env bash
# Build a PIVX-style binary release (Linux tarball or Windows zip + installer)
# from pinned depends sources. Normally run through build-docker.sh.

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
HOST=${HOST:-$("$ROOT_DIR/depends/config.guess")}
JOBS=${JOBS:-2}
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build/release-$HOST"}
OUTPUT_DIR=${OUTPUT_DIR:-"$ROOT_DIR/release"}
DEPENDS_PREFIX="$ROOT_DIR/depends/$HOST"
SOURCE_DIR="$BUILD_DIR/source"
COMPILE_DIR="$BUILD_DIR/build"

case "$HOST" in
    *linux*)
        TARGET=linux
        EXEEXT=
        TOOL_PREFIX=
        # Like PIVX's gitian-linux descriptor.
        CONFIG_FLAGS="--enable-glibc-back-compat --enable-reduce-exports"
        CONFIG_LDFLAGS="-static-libstdc++ -static-libgcc"
        ;;
    x86_64-*mingw32)
        TARGET=win64
        EXEEXT=.exe
        TOOL_PREFIX="$HOST-"
        # Like PIVX's gitian-win descriptor; configure links MinGW statically.
        CONFIG_FLAGS="--enable-reduce-exports"
        CONFIG_LDFLAGS=
        ;;
    *)
        echo "error: unsupported release host $HOST" >&2
        exit 1
        ;;
esac

# Build from a clean source snapshot so ignored objects and config.status files
# from a developer build can never leak into the release. BUILD_DIR is kept
# below the repository's build directory to make its cleanup narrowly scoped.
case "$BUILD_DIR" in
    "$ROOT_DIR"/build/*) ;;
    *)
        echo "error: BUILD_DIR must be below $ROOT_DIR/build" >&2
        exit 1
        ;;
esac
rm -rf "$BUILD_DIR"
mkdir -p "$SOURCE_DIR" "$COMPILE_DIR/src/obj"

if git -C "$ROOT_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    (
        cd "$ROOT_DIR"
        git ls-files -co --exclude-standard -z |
            tar --null --no-recursion --files-from=- -cf -
    ) | tar --no-same-owner -C "$SOURCE_DIR" -xf -
    # The snapshot has no .git, so generate the version suffix/tag here.
    "$ROOT_DIR/share/genbuild.sh" "$COMPILE_DIR/src/obj/build.h" "$ROOT_DIR"
else
    # Already a clean export, e.g. inside the release container.
    tar -C "$ROOT_DIR" \
        --exclude=./build --exclude=./release \
        --exclude=./depends/work --exclude=./depends/built \
        --exclude=./depends/sources --exclude="./depends/$HOST" \
        -cf - . | tar --no-same-owner -C "$SOURCE_DIR" -xf -
    if [ -n "${BUILD_INFO_FILE:-}" ]; then
        cp "$BUILD_INFO_FILE" "$COMPILE_DIR/src/obj/build.h"
    fi
fi
# As in PIVX's gitian build, keep the pregenerated build.h instead of letting
# the git-less snapshot overwrite it with "No build information available".
if [ -s "$COMPILE_DIR/src/obj/build.h" ]; then
    printf '#!/bin/true\n' > "$SOURCE_DIR/share/genbuild.sh"
fi

# In the legacy depends makefiles any non-empty DEBUG value selects a debug
# build. Some CI environments export DEBUG=release, so explicitly clear it.
env -u DEBUG make -C "$ROOT_DIR/depends" HOST="$HOST" DEBUG= -j"$JOBS"

"$SOURCE_DIR/autogen.sh"
cd "$COMPILE_DIR"

# shellcheck disable=SC2086
CONFIG_SITE="$DEPENDS_PREFIX/share/config.site" \
    "$SOURCE_DIR/configure" \
    --prefix="$DEPENDS_PREFIX" \
    --disable-tests \
    --disable-ccache \
    --disable-bip70 \
    --without-libs \
    $CONFIG_FLAGS \
    LDFLAGS="$CONFIG_LDFLAGS"

# The legacy Automake target invokes LevelDB with `make -C $(builddir)/leveldb`
# instead of supporting VPATH. Copy only its clean vendored sources into the
# isolated build tree so generated objects still stay out of the source tree.
cp -a "$SOURCE_DIR/src/leveldb" "$COMPILE_DIR/src/leveldb"

make -j"$JOBS"

VERSION=$(awk '
    function val(line) {
        sub(/^.*,[[:space:]]*/, "", line)
        sub(/\).*$/, "", line)
        return line
    }
    /^define\(_CLIENT_VERSION_MAJOR,/ { major = val($0) }
    /^define\(_CLIENT_VERSION_MINOR,/ { minor = val($0) }
    /^define\(_CLIENT_VERSION_REVISION,/ { patch = val($0) }
    END { printf "%s.%s.%s", major, minor, patch }
' "$SOURCE_DIR/configure.ac")

DIST_NAME="bitblocks-$VERSION"
STAGE_DIR="$BUILD_DIR/package/$DIST_NAME"

rm -rf "$BUILD_DIR/package"
mkdir -p "$STAGE_DIR/bin" "$OUTPUT_DIR"

for binary in bitblocksd bitblocks-cli bitblocks-tx qt/bitblocks-qt; do
    install -m 0755 "$COMPILE_DIR/src/$binary$EXEEXT" "$STAGE_DIR/bin/"
done
install -m 0644 "$SOURCE_DIR/README.md" "$STAGE_DIR/README.md"
install -m 0644 "$SOURCE_DIR/COPYING" "$STAGE_DIR/COPYING"

for binary in "$STAGE_DIR"/bin/*; do
    "${TOOL_PREFIX}strip" --strip-all "$binary"
done

# Release binaries must not require project/build dependencies at runtime.
FORBIDDEN_LIBS='libdb(_cxx)?[-.]|lib(boost|ssl|crypto|event|zmq|protobuf|qrencode|stdc\+\+|gcc_s|winpthread)|qt5'

if [ "$TARGET" = linux ]; then
    # glibc and desktop system libraries (X11, fontconfig, etc.) remain normal
    # distribution dependencies.
    for binary in "$STAGE_DIR"/bin/*; do
        if ldd "$binary" | grep -iE "$FORBIDDEN_LIBS"; then
            echo "error: binary has a forbidden runtime dependency: $binary" >&2
            exit 1
        fi
    done

    # Like PIVX's symbol-check, refuse binaries that need a newer glibc than
    # the oldest distribution we support. Building on a new host (for example
    # Ubuntu 24.04) silently raises this floor, so check it explicitly.
    MAX_GLIBC=${MAX_GLIBC:-2.31}
    for binary in "$STAGE_DIR"/bin/*; do
        required=$(objdump -T "$binary" | grep -oE 'GLIBC_[0-9]+(\.[0-9]+)+' |
            sed 's/^GLIBC_//' | sort -Vu | tail -n 1)
        if [ "$(printf '%s\n%s\n' "$required" "$MAX_GLIBC" | sort -V | tail -n 1)" != "$MAX_GLIBC" ]; then
            echo "error: $binary requires glibc $required (maximum allowed $MAX_GLIBC)" >&2
            echo "       build inside an older distribution or raise MAX_GLIBC" >&2
            exit 1
        fi
    done

    # The legacy Bitcoin 0.10 based tools exit with status 1 after printing
    # their version/help text, so check the output rather than the status.
    for check in "bitblocksd --version" "bitblocks-cli --version" "bitblocks-tx -help"; do
        set -- $check
        output=$("$STAGE_DIR/bin/$1" "$2" 2>&1 || true)
        if ! grep -q 'version' <<<"$output"; then
            echo "error: $1 smoke test failed:" >&2
            echo "$output" >&2
            exit 1
        fi
    done
else
    # Windows binaries must only import DLLs shipped with Windows itself.
    for binary in "$STAGE_DIR"/bin/*; do
        if "${TOOL_PREFIX}objdump" -p "$binary" | awk '/DLL Name:/ { print $3 }' |
            grep -iE "$FORBIDDEN_LIBS"; then
            echo "error: binary imports a non-system DLL: $binary" >&2
            exit 1
        fi
    done
fi

SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$ROOT_DIR" log -1 --format=%ct)}
export SOURCE_DATE_EPOCH
find "$BUILD_DIR/package" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +

if [ "$TARGET" = linux ]; then
    ARTIFACTS=("$DIST_NAME-$HOST.tar.gz")
    tar \
        --sort=name \
        --mtime="@$SOURCE_DATE_EPOCH" \
        --owner=0 \
        --group=0 \
        --numeric-owner \
        -C "$BUILD_DIR/package" \
        -cf - "$DIST_NAME" | gzip -9n > "$OUTPUT_DIR/${ARTIFACTS[0]}"
else
    ARTIFACTS=("$DIST_NAME-win64.zip" "$DIST_NAME-win64-setup.exe")
    rm -f "$OUTPUT_DIR/${ARTIFACTS[0]}"
    (
        cd "$BUILD_DIR/package"
        find "$DIST_NAME" | sort | zip -X -q -@ "$OUTPUT_DIR/${ARTIFACTS[0]}"
    )

    # `make deploy` strips the binaries into release/ and runs makensis on
    # share/setup.nsi, like PIVX's gitian-win descriptor.
    make deploy
    install -m 0644 "$COMPILE_DIR/${ARTIFACTS[1]}" "$OUTPUT_DIR/${ARTIFACTS[1]}"
fi

(
    cd "$OUTPUT_DIR"
    for artifact in "${ARTIFACTS[@]}"; do
        sha256sum "$artifact" > "$artifact.sha256"
        echo "Release artifact: $OUTPUT_DIR/$artifact"
    done
)
