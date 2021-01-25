#!/bin/bash

## mv_unibo_cgr.sh
#
#  Utility script to add Unibo-CGR into ION/DTN2.
#  Launch this script without arguments to get help.
#
#
#  Written by Lorenzo Persampieri:  lorenzo.persampieri@studio.unibo.it
#  Supervisor Prof. Carlo Caini:    carlo.caini@unibo.it
#
#  Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
#
#  This file is part of Unibo-CGR.
#
#  Unibo-CGR is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#  Unibo-CGR is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
#
##

#set -euxo pipefail
set -euo pipefail

function help_fun() {
	echo "Usage: $0 <ion | dtn2> </path/to/Unibo-CGR/> </path/to/ION/ | /path/to/DTN2/>" 1>&2
	echo "This script includes Unibo-CGR in either ION or DTN2." 1>&2
	echo "Launch this script with the three parameters explicited in the Usage string." 1>&2
}

if test $# -ne 3
then
	help_fun
	exit 1
fi

BP_IMPL_NAME="$1"
UNIBO_CGR_DIR="$2"
BP_IMPL_DIR="$3"

if test "$BP_IMPL_NAME" != "ion" -a "$BP_IMPL_NAME" != "dtn2"
then
	help_fun
	exit 1
fi

if ! test -d "$UNIBO_CGR_DIR"
then
	help_fun
	echo "$UNIBO_CGR_DIR is not a directory" >&2
	exit 1
fi

if ! test -d "$BP_IMPL_DIR"
then
	help_fun
	echo "$BP_IMPL_DIR is not a directory" >&2
	exit 1
fi

function update_config_file() {
	PATH_TO_CONFIG_FILE="$1"
	MACRO="$2"
	VALUE="$3"

#   change macro values inside #ifndef/#endif block (with regex)
	sed -i '/^[[:space:]]*#ifndef[[:space:]]\+'"$MACRO"'[[:space:]]*$/,/^[[:space:]]*#endif[[:space:]]*$/s/^[[:space:]]*#define[[:space:]]\+'"$MACRO"'[[:space:]]\+.*$/#define '"$MACRO $VALUE"'/' "$PATH_TO_CONFIG_FILE"
}

function update_configure_ac_ion() {
	CONF_AC="$1/configure.ac"
	
	if ! test -f "$CONF_AC" ; then # check regular file
		return 1
	elif ! test -r "$CONF_AC" ; then # check read permission
		return 1
	fi

# first, check if the configure.ac file was previously modified by this script

	if egrep -q -i 'Unibo[-_]?CGR' "$CONF_AC" ; then
		# Unibo-CGR match, case insensitive, with or without dash or underscore
		# OK, nothing to do
		return 0
	fi
	
	if ! test -w "$CONF_AC" ; then # check write permission
		return 1
	fi
	
# store the following lines into "TO_ADD" variable...
TO_ADD='
#
# allow the user running configure to build Unibo-CGR
#
AC_ARG_ENABLE(
    unibo-cgr,
    [AC_HELP_STRING([--enable-unibo-cgr], [force build of Unibo-CGR])],
    [UNIBO_CGR=true
    AC_DEFINE([UNIBO_CGR],[1],[Define to 1 to build Unibo-CGR])],
    [])
AM_CONDITIONAL(UNIBO_CGR, test x$UNIBO_CGR = xtrue)'
	
	tmp_file="$(mktemp)" #create aux file

	if ! test -f "$tmp_file" ; then # check regular file
		return 1
	elif ! test -w "$tmp_file" ; then # check write permission
		return 1
	elif ! test -r "$tmp_file" ; then # check read permission
		return 1
	fi
	
	DONE="false"

	while read line ; do
		echo "$line" >> "$tmp_file"
		if test "$DONE" != "true" && echo "$line" | grep -q '##Process user flags' ; then
			# add here Unibo-CGR configure option
			echo "$TO_ADD" >> "$tmp_file"
			echo "" >> "$tmp_file"
			DONE="true"
		fi
	done < "$CONF_AC"
	
	cat "$tmp_file" > "$CONF_AC"
	rm -f "$tmp_file"
	
}

function mv_unibo_cgr_to_ion() {

	UNIBO_CGR="$1"
	ION="$2"
	ION_BPV7="$ION/bpv7"
	AUX_BPV7="$UNIBO_CGR/ion_bpv7/aux_files"
	EXT_BPV7="$UNIBO_CGR/ion_bpv7/extensions"
	CONFIG_FILE_BPV7="$ION_BPV7/cgr/Unibo-CGR/core/config.h"
	ION_BPV6="$ION/bpv6"
	AUX_BPV6="$UNIBO_CGR/ion_bpv6/aux_files"
	EXT_BPV6="$UNIBO_CGR/ion_bpv6/extensions"
	CONFIG_FILE_BPV6="$ION_BPV6/cgr/Unibo-CGR/core/config.h"

	echo ""
	echo "Moving Unibo-CGR into ION..."
	echo ""

	echo "Moving Unibo-CGR into bpv6..."
	rm -rf "$ION_BPV6/cgr/Unibo-CGR"
	cp -rpf "$UNIBO_CGR" "$ION_BPV6/cgr/Unibo-CGR"

	echo "Moving auxiliary files for bpv6..."
	cp -pf "$AUX_BPV6/bpextensions.c" "$ION_BPV6/library/ext/"
	cp -pf "$AUX_BPV6/cgr.h"          "$ION_BPV6/library/"
	cp -pf "$AUX_BPV6/cgrfetch.c"     "$ION_BPV6/utils/"
	cp -pf "$AUX_BPV6/ipnfw.c"        "$ION_BPV6/ipn/"
	cp -pf "$AUX_BPV6/libcgr.c"       "$ION_BPV6/cgr/"
	cp -pf "$AUX_BPV6/Makefile.am"    "$ION/"

	echo "Moving extensions for bpv6..."
	rm -rf "$ION_BPV6/library/ext/cgrr"
	cp -rpf "$EXT_BPV6/cgrr" "$ION_BPV6/library/ext/"
	rm -rf "$ION_BPV6/library/ext/rgr"
	cp -rpf "$EXT_BPV6/rgr" "$ION_BPV6/library/ext/"

	echo "Removing unnecessary files from Unibo-CGR for bpv6..."
	rm -rf "$ION_BPV6/cgr/Unibo-CGR/ion_bpv7"
	rm -rf "$ION_BPV6/cgr/Unibo-CGR/dtn2"
	rm -f "$ION_BPV6/cgr/Unibo-CGR/mv_unibo_cgr.sh"
	rm -f "$ION_BPV6/cgr/Unibo-CGR/.gitlab-ci.yml"
	rm -rf "$ION_BPV6/cgr/Unibo-CGR/docs/.doxygen"
#	rm -rf "$ION_BPV6/cgr/Unibo-CGR/ion_bpv6/aux_files"
#	rm -rf "$ION_BPV6/cgr/Unibo-CGR/ion_bpv6/extensions"

	echo "Moving Unibo-CGR into bpv7..."
	rm -rf "$ION_BPV7/cgr/Unibo-CGR"
	cp -rpf "$UNIBO_CGR" "$ION_BPV7/cgr/Unibo-CGR"

	echo "Moving auxiliary files for bpv7..."
	cp -pf "$AUX_BPV7/bp.h"           "$ION_BPV7/include/"
	cp -pf "$AUX_BPV7/bpextensions.c" "$ION_BPV7/library/ext/"
	cp -pf "$AUX_BPV7/libcgr.c"       "$ION_BPV7/cgr/"
	cp -pf "$AUX_BPV7/Makefile.am"    "$ION/"

	echo "Moving extensions for bpv7..."
	rm -rf "$ION_BPV7/library/ext/cgrr"
	cp -rpf "$EXT_BPV7/cgrr" "$ION_BPV7/library/ext/"
	rm -rf "$ION_BPV7/library/ext/rgr"
	cp -rpf "$EXT_BPV7/rgr" "$ION_BPV7/library/ext/"

	echo "Removing unnecessary files from Unibo-CGR for bpv7..."
	rm -rf "$ION_BPV7/cgr/Unibo-CGR/ion_bpv6"
	rm -rf "$ION_BPV7/cgr/Unibo-CGR/dtn2"
	rm -f "$ION_BPV7/cgr/Unibo-CGR/mv_unibo_cgr.sh"
	rm -f "$ION_BPV7/cgr/Unibo-CGR/.gitlab-ci.yml"
	rm -rf "$ION_BPV7/cgr/Unibo-CGR/docs/.doxygen"
#	rm -rf "$ION_BPV7/cgr/Unibo-CGR/ion_bpv7/aux_files"
#	rm -rf "$ION_BPV7/cgr/Unibo-CGR/ion_bpv7/extensions"

	echo "Updating Unibo-CGR's config.h file for ION..."
	update_config_file "$CONFIG_FILE_BPV6" CGR_BUILD_FOR_ION 1
	update_config_file "$CONFIG_FILE_BPV7" CGR_BUILD_FOR_ION 1
	
	echo ""
	echo "Updating configure.ac..."
	if ! update_configure_ac_ion "$ION" ; then
		echo "ERROR: Cannot update configure.ac." 1>&2
	fi

	echo ""

	AUTORECONF=""
	LIBTOOLIZE=""

	if ! command -v autoreconf > /dev/null 2>&1
	then
		echo "autoreconf: not found. Please install autoconf package." 1>&2
		AUTORECONF="missing"
	fi

	if ! command -v libtoolize > /dev/null 2>&1
	then
		echo "libtoolize: not found. Please install libtool package." 1>&2
		LIBTOOLIZE="missing"
	fi

	if test "$AUTORECONF" != "missing" -a "$LIBTOOLIZE" != "missing"
	then
		echo "Updating the configure script..."
		cd "$ION" && autoreconf -fi && echo -e "\nNow you can compile and install in the usual way with configure/make/make install\n" \
		&& echo "configure sintax from ION's root directory:" \
		&& echo "./configure --enable-unibo-cgr CPPFLAGS='-DRGREB=1 -DCGRREB=1'" \
		&& echo "If you don't want RGR or CGRR just don't specify them in CPPFLAGS."
	else
		echo -e "\nPlease install missing packages and launch autoreconf -fi in $ION to update the configure script.\n" 1>&2
	fi
	
	echo ""
}

function mv_unibo_cgr_to_dtn2() {

	UNIBO_CGR="$1"
	DTN2="$2"
	DTN2_ROUTING="$DTN2/servlib/routing"
	CONFIG_FILE="$DTN2_ROUTING/Unibo-CGR/core/config.h"

	echo ""
	echo "Moving Unibo-CGR into DTN2..."
	rm -rf "$DTN2_ROUTING/Unibo-CGR"
	cp -rpf "$UNIBO_CGR" "$DTN2_ROUTING/Unibo-CGR"

	echo "Moving auxiliary files into DTN2..."
	cp -pf "$UNIBO_CGR/dtn2/aux_files/BundleRouter.cc"         "$DTN2_ROUTING/"
	cp -pf "$UNIBO_CGR/dtn2/aux_files/UniboCGRBundleRouter.cc" "$DTN2_ROUTING/"
	cp -pf "$UNIBO_CGR/dtn2/aux_files/UniboCGRBundleRouter.h"  "$DTN2_ROUTING/"
	cp -pf "$UNIBO_CGR/dtn2/aux_files/Makefile"                "$DTN2/servlib/"

	echo "Removing unnecessary files from Unibo-CGR for DTN2..."
	rm -rf "$DTN2_ROUTING/Unibo-CGR/ion_bpv6"
	rm -rf "$DTN2_ROUTING/Unibo-CGR/ion_bpv7"
	rm -f "$DTN2_ROUTING/Unibo-CGR/mv_unibo_cgr.sh"
	rm -f "$DTN2_ROUTING/Unibo-CGR/.gitlab-ci.yml"
	rm -rf "$DTN2_ROUTING/Unibo-CGR/docs/.doxygen"
#	rm -rf "$DTN2_ROUTING/Unibo-CGR/dtn2/aux_files"

	echo "Updating Unibo-CGR's config.h file for DTN2..."
	update_config_file "$CONFIG_FILE" CGR_BUILD_FOR_ION 0
	update_config_file "$CONFIG_FILE" CGR_AVOID_LOOP 0
	update_config_file "$CONFIG_FILE" MSR 0
	update_config_file "$CONFIG_FILE" CGRR 0
	update_config_file "$CONFIG_FILE" RGR 0
	update_config_file "$CONFIG_FILE" MSR_PRECONF 0
	update_config_file "$CONFIG_FILE" UNIBO_CGR_SUGGESTED_SETTINGS 0

	echo

}

if test "$BP_IMPL_NAME" = "ion"
then
	mv_unibo_cgr_to_ion "$UNIBO_CGR_DIR" "$BP_IMPL_DIR"
elif test "$BP_IMPL_NAME" = "dtn2"
then
	mv_unibo_cgr_to_dtn2 "$UNIBO_CGR_DIR" "$BP_IMPL_DIR"
else
	help_fun
	exit 1
fi