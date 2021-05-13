In this directory you find the source code for the RGR and CGRR Extension Blocks.
You have to insert the "rgr" and "cgrr" directories under ./bpv7/library/ext/ where "." is ION's root directory.
You need also the modified "bpextensions.c" and "bp.h" files but you find them under Unibo-CGR/ion_bpv7/aux_files/ directory.

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

Just a reminder: CGRR Ext. Block is required to enable Moderate Source Routing and RGR Ext. Block
is required to enable anti-loop mechanism of Unibo-CGR.