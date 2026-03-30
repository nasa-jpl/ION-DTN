#!/usr/bin/env bash
#
# uninstall_bsl.sh - Remove BSL (Bundle Security Library) files from a given prefix.
#
# Usage:
#   ./uninstall_bsl.sh [PREFIX]
#
# PREFIX defaults to /usr/local. The script removes BSL libraries, headers,
# executables, and cmake config files installed under PREFIX.
#
# Examples:
#   ./uninstall_bsl.sh                  # Remove from /usr/local
#   ./uninstall_bsl.sh /usr/local       # Same as above
#   ./uninstall_bsl.sh /opt/bsl         # Remove from /opt/bsl
#   sudo ./uninstall_bsl.sh /usr/local  # Use sudo for system directories

set -euo pipefail

PREFIX="${1:-/usr/local}"

# Resolve to absolute path
PREFIX="$(cd "$PREFIX" 2>/dev/null && pwd)" || {
    echo "Error: prefix directory '$1' does not exist."
    exit 1
}

echo "Removing BSL files from prefix: $PREFIX"
echo

ERRORS=0

remove_file() {
    if [ -e "$1" ] || [ -L "$1" ]; then
        echo "  rm $1"
        rm -f "$1" || ERRORS=$((ERRORS + 1))
    fi
}

remove_dir() {
    if [ -d "$1" ]; then
        echo "  rmdir $1"
        rmdir "$1" 2>/dev/null || true
    fi
}

# --- Libraries ---
echo "Libraries:"
for lib in bsl_front bsl_dynamic bsl_default_sc bsl_sample_pp bsl_crypto bsl_mock_bpa bsl_test_utils; do
    for f in "$PREFIX"/lib/lib${lib}.so*; do
        remove_file "$f"
    done
done
for f in "$PREFIX"/lib/libqcbor.so*; do
    remove_file "$f"
done
remove_file "$PREFIX/lib/libunity.a"
echo

# --- Headers ---
echo "Headers:"
for dir in bsl m-lib qcbor unity; do
    if [ -d "$PREFIX/include/$dir" ]; then
        echo "  rm -rf $PREFIX/include/$dir"
        rm -rf "$PREFIX/include/$dir" || ERRORS=$((ERRORS + 1))
    fi
done
echo

# --- Executables ---
echo "Executables:"
remove_file "$PREFIX/bin/bsl-mock-bpa"
if [ -d "$PREFIX/libexec/bsl" ]; then
    echo "  rm -rf $PREFIX/libexec/bsl"
    rm -rf "$PREFIX/libexec/bsl" || ERRORS=$((ERRORS + 1))
fi
echo

# --- CMake config ---
echo "CMake config:"
if [ -d "$PREFIX/lib/cmake/unity" ]; then
    echo "  rm -rf $PREFIX/lib/cmake/unity"
    rm -rf "$PREFIX/lib/cmake/unity" || ERRORS=$((ERRORS + 1))
    remove_dir "$PREFIX/lib/cmake"
fi
echo

# --- Update linker cache if ldconfig is available ---
if command -v ldconfig >/dev/null 2>&1; then
    echo "Updating linker cache..."
    ldconfig 2>/dev/null || true
fi

if [ "$ERRORS" -gt 0 ]; then
    echo "Completed with $ERRORS error(s). You may need to run with sudo."
    exit 1
else
    echo "Done."
fi
