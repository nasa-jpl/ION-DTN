#!/bin/bash
#
# enable_manual_build
# Patricia Lindner
# November 18, 2024
#
# This script is intended to be used to switch from the autoconf build system (the default)
# to the manual developmental build system.

# Clean up old executables
make uninstall
make clean

# Rename manual Makefiles from Makefile.dev to Makefile
MAKEFILES=`find . -name 'Makefile.dev'`
for f in $MAKEFILES; do 
		cp $f ${f%".dev"}
done