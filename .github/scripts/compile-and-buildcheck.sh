#!/bin/sh
set -e

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

# config.status bootstraps automake's dependency fragments by invoking
# ${MAKE-make}.  On Solaris the default make is SysV make, which cannot run
# the generated am--depfiles rules and aborts with "Something went wrong
# bootstrapping makefile fragments".  Point MAKE at the same make used for
# the build so the bootstrap uses gmake there too; on Linux this stays make.
export MAKE="$MAKE_CMD"

echo "Running autoreconf..."
autoreconf -fi

echo "Running configure..."
# EXTRA_CONFIGURE_FLAGS allows workflows to pass OS-specific flags (like CFLAGS for ARC)
# shellcheck disable=SC2086
./configure --enable-crypto-mbedtls --enable-bpsec-debugging \
    --disable-silent-rules \
    --disable-legacy-aliases \
    $EXTRA_CONFIGURE_FLAGS

# ARC_CPU_LIMIT is a limit set in the infrastructure pod environment
# Fall back to nproc if not defined to maximize available vCPU utilization
CORES=${ARC_CPU_LIMIT:-$(nproc)}
echo "Running ${MAKE_CMD} all..."
$MAKE_CMD -j "${CORES}" all

echo "Running ${MAKE_CMD} buildcheck..."
$MAKE_CMD buildcheck

echo "Build and buildcheck completed successfully!"
