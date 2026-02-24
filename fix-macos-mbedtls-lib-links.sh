#!/usr/bin/env bash

# Fixes bare mbedtls library references (e.g. "libmbedtls.dylib") in ION
# binaries so that dyld can find them at the correct absolute path.
#
# On macOS, mbedtls installs its dylibs with bare install names (no path
# prefix), causing dyld to fail at runtime. This script uses install_name_tool
# to rewrite those references to absolute paths.
#
# The script automatically discovers all Mach-O binaries in the given
# directories -- no hardcoded binary list to maintain.
#
# Usage: ./fix-macos-mbedtls-lib-links.sh <mbedtls-lib-dir> <ion-bin-dir> <ion-source-dir>
#
# Arguments:
#   mbedtls-lib-dir   Directory containing libmbedtls.dylib, libmbedx509.dylib,
#                     and libmbedcrypto.dylib (e.g. /usr/local/lib)
#   ion-bin-dir       Directory where ION binaries are installed (e.g. /usr/local/bin)
#   ion-source-dir    ION source tree root, used to find test binaries under tests/
#
# Note: Installed binaries (e.g. in /usr/local/bin) are typically owned by root,
#   so this script usually needs to be run with sudo.
#
# Example:
#   sudo ./fix-macos-mbedtls-lib-links.sh /usr/local/lib /usr/local/bin /path/to/ion

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <mbedtls-lib-dir> <ion-bin-dir> <ion-source-dir>"
    echo ""
    echo "  mbedtls-lib-dir   Directory containing libmbedtls.dylib (e.g. /usr/local/lib)"
    echo "  ion-bin-dir       Directory where ION binaries are installed (e.g. /usr/local/bin)"
    echo "  ion-source-dir    ION source tree root (for fixing test binaries under tests/)"
    exit 1
fi

NEW_LIB_PATH="$1"
INSTALL_FOLDER="$2"
ION_FOLDER="$3"

# Check write access to the install folder; installed binaries are typically
# owned by root and require sudo.
if [ ! -w "$INSTALL_FOLDER" ]; then
    echo "Error: No write access to $INSTALL_FOLDER. Try running with sudo."
    exit 1
fi

# Libraries to check and update
LIBRARIES=("libmbedtls.dylib" "libmbedx509.dylib" "libmbedcrypto.dylib")

# Fix mbedtls library paths in a single Mach-O binary.
# Skips libraries that are not linked or already have the correct path.
fix_binary() {
    local BINARY_PATH="$1"
    local FOUND_ANY=0

    for LIB in "${LIBRARIES[@]}"; do
        CURRENT_PATH=$(otool -L "$BINARY_PATH" 2>/dev/null | grep "$LIB" | awk '{print $1}')

        [ -z "$CURRENT_PATH" ] && continue

        NEW_PATH="$NEW_LIB_PATH/$LIB"

        if [ "$CURRENT_PATH" != "$NEW_PATH" ]; then
            if [ "$FOUND_ANY" -eq 0 ]; then
                echo "Fixing: $BINARY_PATH"
                FOUND_ANY=1
            fi
            echo "  $LIB: $CURRENT_PATH -> $NEW_PATH"
            install_name_tool -change "$CURRENT_PATH" "$NEW_PATH" "$BINARY_PATH"
            if [ $? -ne 0 ]; then
                echo "  Error updating $LIB."
            fi
        fi
    done
}

# Returns 0 if the file is a Mach-O binary, 1 otherwise.
is_macho() {
    file "$1" 2>/dev/null | grep -q "Mach-O"
}

echo "********** Scanning installed ION binaries in $INSTALL_FOLDER **********"
for BINARY_PATH in "$INSTALL_FOLDER"/*; do
    [ -f "$BINARY_PATH" ] || continue
    [ -x "$BINARY_PATH" ] || continue
    is_macho "$BINARY_PATH" || continue
    fix_binary "$BINARY_PATH"
done

echo "********** Scanning regression test binaries under $ION_FOLDER/tests **********"
# Autotools places real binaries in .libs/ subdirectories; also check top-level
# in case of non-libtool builds.
find "$ION_FOLDER/tests" -type f -perm +111 2>/dev/null | while read -r BINARY_PATH; do
    is_macho "$BINARY_PATH" || continue
    fix_binary "$BINARY_PATH"
done

echo "Finished checking and updating library paths."
