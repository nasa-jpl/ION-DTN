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

echo "Running autoreconf..."
autoreconf -fi

echo "Running configure..."
# EXTRA_CONFIGURE_FLAGS allows workflows to pass OS-specific flags (like CFLAGS for ARC)
./configure --enable-crypto-mbedtls --enable-bpsec-debugging "$EXTRA_CONFIGURE_FLAGS"

echo "Running ${MAKE_CMD} all..."
$MAKE_CMD -j"$(nproc)" all

echo "Running ${MAKE_CMD} buildcheck..."
$MAKE_CMD buildcheck

echo "Build and buildcheck completed successfully!"
