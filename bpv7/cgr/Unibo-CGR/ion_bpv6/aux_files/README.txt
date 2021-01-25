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

After having copyed the auxiliary files and the unibo-CGR directory, you can compile (and install) with this sequence of commands: 
autoreconf -fi && ./configure --enable-bpv6 && make && make install
Note that "autoreconf -fi" is necessary only the first time.
