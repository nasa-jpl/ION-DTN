#!/bin/bash
# Build or uninstall BSL for ION integration.
#
# Usage:
#   ./build-bsl-for-ion.sh           # Build and install BSL
#   ./build-bsl-for-ion.sh clean     # Remove BSL build artifacts and installed files
set -e

BSL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/external/BSL" && pwd)"

# BSL installs its libraries into lib64/ on some platforms (RHEL family)
# and lib/ on others.  Checking only one of them makes this script treat an
# installed BSL as missing and rebuild it on every "make".
bsl_is_installed() {
    for libdir in lib64 lib; do
        if ls "${BSL_DIR}/testroot/usr/${libdir}"/libbsl_front.* \
                >/dev/null 2>&1; then
            return 0
        fi
    done
    return 1
}

case "${1:-build}" in
    clean)
        echo "Uninstalling BSL from ${BSL_DIR}/testroot ..."
        cd "$BSL_DIR"
        ./build.sh clean
        echo "BSL uninstalled."
        ;;
    build)
        if bsl_is_installed; then
            echo "BSL already installed at ${BSL_DIR}/testroot — skipping build."
            echo "Run '$0 clean' first to force a rebuild."
            exit 0
        fi
        echo "Building BSL in ${BSL_DIR} ..."
        cd "$BSL_DIR"
        ./build.sh clean
        ./build.sh deps
        # BSL gates its unit tests on BUILD_UNITTEST (default ON), not on
        # BUILD_TESTING.  Leaving it on pulls in the Unity test tooling, which
        # hard-requires ruby, so an ION build that wants no tests still fails
        # on hosts without it.
        NOTEST_OPTS="-DBUILD_TESTING=OFF -DBUILD_UNITTEST=OFF"

        # BSL's resources/prep.sh hardcodes "-G Ninja".  Fall back to Unix
        # Makefiles when ninja isn't installed; a later -G overrides the
        # earlier one.
        NEED_MAKE_GENERATOR=no
        if ! command -v ninja >/dev/null 2>&1; then
            NEED_MAKE_GENERATOR=yes
        fi

        if [ "$(uname)" = "SunOS" ]; then
            export CFLAGS="$CFLAGS -D__EXTENSIONS__"
            JANSSON_OPTS=""
            for dir in /usr/include/jansson /usr/local/include/jansson; do
                if [ -f "$dir/jansson.h" ]; then
                    JANSSON_OPTS="-DCMAKE_INCLUDE_PATH=$dir"
                    break
                fi
            done
            ./build.sh prep $NOTEST_OPTS -DTEST_MEMCHECK=OFF -G "Unix Makefiles" $JANSSON_OPTS
        elif [ "$NEED_MAKE_GENERATOR" = yes ]; then
            ./build.sh prep $NOTEST_OPTS -G "Unix Makefiles"
        else
            ./build.sh prep $NOTEST_OPTS
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
