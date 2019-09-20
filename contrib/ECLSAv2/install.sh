#!/bin/bash

ECLSA_PATH=$(pwd)
MAKEFILE_PATCH_PATH="$ECLSA_PATH/install_files/eclsa_Makefile.patch"
CONFIGURE_PATCH_PATH="$ECLSA_PATH/install_files/configure_ECLSA.patch"
ION_PATH="../../"

echo "------------------------------------------------"
echo "Welcome to the ECLSAv2 installer."
echo "------------------------------------------------"
echo "Current variables:"
echo "ECLSA_PATH=$ECLSA_PATH"
echo "ION_PATH=$ION_PATH"
echo "MAKEFILE_PATCH_PATH=$MAKEFILE_PATCH_PATH"
echo "CONFIGURE_PATCH_PATH=$CONFIGURE_PATCH_PATH"
echo "------------------------------------------------"
echo "HUNKS will fail if the patch was already applied"
echo "------------------------------------------------"

echo -n "copying (ignore if already exists) "
cp -v install_files/configure_eclsa.sh "$ION_PATH"
echo -n "copying (ignore if already exists) "
cp -v install_files/kill_ECLSA.sh "$ION_PATH"
cd ../../
patch -F1 -r- -fp0 --no-backup-if-mismatch Makefile.am < "$MAKEFILE_PATCH_PATH"
patch -F1 -r- -fp0 --no-backup-if-mismatch configure.ac < "$CONFIGURE_PATCH_PATH"

# To generate patch file use:
# diff -u MakefileION.am MakefileECLSA.am