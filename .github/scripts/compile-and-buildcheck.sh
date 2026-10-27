#!/bin/sh
set -e

# Parse command-line arguments
WITH_BSL=false
while [ $# -gt 0 ]; do
    case "$1" in
        --bsl)
            WITH_BSL=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--bsl]"
            echo "  --bsl    Build with Bundle Security Layer (BSL) support"
            exit 0
            ;;
        *)
            echo "Error: Unknown option '$1'" >&2
            echo "Run '$0 --help' for usage information" >&2
            exit 1
            ;;
    esac
done

echo "Starting build and buildcheck at $(date)"

# Set up common environment variables
USER=$(whoami)
export USER
echo "USER is set to: $USER"
export ION_RUN_EXPERT="yes"
export PATH=/usr/local/bin:"$PATH"
export LD_LIBRARY_PATH=/usr/local/lib:"$LD_LIBRARY_PATH"

# Allow overriding the make command (defaults to 'make' for Linux, can be 'gmake' for Solaris)
MAKE_CMD=${MAKE_CMD:-make}

echo "Running autoreconf..."
autoreconf -fi

echo "Running configure..."
# EXTRA_CONFIGURE_FLAGS allows workflows to pass OS-specific flags (like CFLAGS for ARC)
# shellcheck disable=SC2086
if [ "$WITH_BSL" = "true" ]; then
    echo "Building with BSL..."
    ./configure MAKE="${MAKE_CMD}" --with-bsl \
    --disable-silent-rules \
    --disable-legacy-aliases \
    $EXTRA_CONFIGURE_FLAGS
else
    echo "Building with native BPSec..."
    ./configure MAKE="${MAKE_CMD}" --enable-crypto-mbedtls --enable-bpsec-debugging \
    --disable-silent-rules \
    --disable-legacy-aliases \
    $EXTRA_CONFIGURE_FLAGS
fi

# ARC_CPU_LIMIT is a limit set in the infrastructure pod environment
# Fall back to nproc if not defined to maximize available vCPU utilization
CORES=${ARC_CPU_LIMIT:-$(nproc)}
echo "Running ${MAKE_CMD} all..."
$MAKE_CMD -j "${CORES}" all

echo "Running ${MAKE_CMD} buildcheck..."
$MAKE_CMD buildcheck

echo "Build and buildcheck completed successfully!"
