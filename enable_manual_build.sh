#!/usr/bin/env bash
#
# enable_manual_build
# Patricia Lindner
# November 18, 2024
#
# This script is intended to be used to switch from the autoconf build system (the default)
# to the manual developmental build system.

# Clean up old executables (skip if --skip-clean flag provided)
if [ "$1" != "--skip-clean" ]; then
    echo "Running clean and uninstall..."
    echo "  (Use --skip-clean flag to skip this step)"
    make uninstall
    make clean
else
    echo "Skipping clean/uninstall (--skip-clean flag provided)"
fi

# Rename manual Makefiles from Makefile.dev to Makefile
MAKEFILES=`find . -name 'Makefile.dev'`
for f in $MAKEFILES; do
    cp $f ${f%".dev"}
done

echo "Done. Makefile.dev files copied to Makefile."
