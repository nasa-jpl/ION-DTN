#!/usr/bin/env bash

# remove_unibo_cgr_from_ion.sh
#
# Utility script to remove Unibo-CGR files copied into ION.
# Usage: ./remove_unibo_cgr_from_ion.sh </path/to/ION/>
#
# This script reverses the file operations performed by mv_unibo_cgr.sh,
# mv_unibo_cgr_ion_bpv7_aux_files.sh, and mv_unibo_cgr_ion_bpv7_extensions.sh
# for the ION bpv7 implementation.
#

set -euo pipefail

function help_fun() {
    echo "Usage: $0 </path/to/ION/>" 1>&2
    echo "This script removes Unibo-CGR files from the specified ION directory." 1>&2
    echo "Provide the path to the ION directory as the only argument." 1>&2
}

# Check for correct number of arguments
if test $# -ne 1; then
    help_fun
    exit 1
fi

ION="$1"
ION_BPV7="$ION/bpv7"

# Verify ION directory exists
if ! test -d "$ION"; then
    echo "Error: $ION is not a directory" >&2
    help_fun
    exit 1
fi

echo "Removing Unibo-CGR files from ION..."

# Remove the entire Unibo-CGR directory copied into bpv7
echo "Removing $ION_BPV7/cgr/Unibo-CGR..."
rm -rf "$ION_BPV7/cgr/Unibo-CGR"

# Remove auxiliary file (Makefile.am) from ION root
echo "Removing $ION/Makefile.am..."
rm -f "$ION/Makefile.am"
echo "Restoring original Makefile.am..."
if [ -f "$ION/Makefile.am.bak" ]; then
    cp "$ION/Makefile.am.bak" "$ION/Makefile.am"
else
    echo "The original Makefile.am was not restored. You need to restore it manually."
fi

# Remove extensions (cgrr and rgr) from bpv7 library
echo "Removing $ION_BPV7/library/ext/cgrr..."
rm -rf "$ION_BPV7/library/ext/cgrr"
echo "Removing $ION_BPV7/library/ext/rgr..."
rm -rf "$ION_BPV7/library/ext/rgr"

echo "Unibo-CGR files have been removed from ION."
echo "Note: You may need to run 'autoreconf -fi' in $ION to update the build system if necessary."
