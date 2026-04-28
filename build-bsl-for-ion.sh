#!/bin/bash
# Build or uninstall BSL for ION integration.
#
# Usage:
#   ./build-bsl-for-ion.sh           # Build and install BSL
#   ./build-bsl-for-ion.sh clean     # Remove BSL build artifacts and installed files
set -e

BSL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/external/BSL" && pwd)"
MARKER="${BSL_DIR}/testroot/usr/lib/libbsl_front.so"

case "${1:-build}" in
    clean)
        echo "Uninstalling BSL from ${BSL_DIR}/testroot ..."
        cd "$BSL_DIR"
        ./build.sh clean
        echo "BSL uninstalled."
        ;;
    build)
        if [ -f "$MARKER" ]; then
            echo "BSL already installed at ${BSL_DIR}/testroot — skipping build."
            echo "Run '$0 clean' first to force a rebuild."
            exit 0
        fi
        echo "Building BSL in ${BSL_DIR} ..."
        cd "$BSL_DIR"
        ./build.sh clean
        ./build.sh deps
        if [ $(uname) == "SunOS" ]; then
            export CFLAGS="$CFLAGS -D__EXTENSIONS__"
            ./build.sh prep -DBUILD_TESTING=OFF -DTEST_MEMCHECK=OFF
        else
            ./build.sh prep -DBUILD_TESTING=OFF
        fi
        ./build.sh
        ./build.sh install
        echo "BSL built and installed to ${BSL_DIR}/testroot"
        ;;
    *)
        echo "Usage: $0 [build|clean]" >&2
        exit 1
        ;;
esac
