This directory contains a few auxiliary files taken from ION 4.0.1 and marginally modified to support Unibo-CGR.
These files maintain their original copyright and license.
They are intended to replace the homonimous original files contained in ION, thus they should be copyed in the following directories ("." refers to ION's root directory):

bpextensions.c:    ./bpv6/library/ext/
cgr.h:             ./bpv6/library/
cgrfetch.c:        ./bpv6/utils/
ipnfw.c            ./bpv6/ipn/
libcgr.c:          ./bpv6/cgr/
Makefile.am:       ./

The Unibo-CGR's root directory must be inserted under ./bpv6/cgr/ (next to libcgr.c).

Add the following lines to ION's configure.ac files (under ##Processing user flags section):
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


After having copied the auxiliary files and the Unibo-CGR directory, you can compile (and install) with this sequence of commands: 
autoreconf -fi && ./configure [see notes below] && make && make install
Note that "autoreconf -fi" is necessary only the first time.

Note on configure:
Launch the configure script with the following sintax (from ION's root directory):  
Only Unibo-CGR: ./configure --enable-unibo-cgr
Unibo-CGR + RGR Ext. Block: ./configure --enable-unibo-cgr CPPFLAGS='-DRGREB=1'
Unibo-CGR + CGRR Ext. Block: ./configure --enable-unibo-cgr CPPFLAGS='-DCGRREB=1'
Unibo-CGR + RGR and CGRR Ext. Block: ./configure --enable-unibo-cgr CPPFLAGS='-DRGREB=1 -DCGRREB=1'
Unibo-CGR disabled: ./configure
