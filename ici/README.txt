*******************************************************************
NO WARRANTY:

                           DISCLAIMER

THE SOFTWARE AND/OR RELATED MATERIALS ARE PROVIDED "AS-IS" WITHOUT
WARRANTY OF ANY KIND INCLUDING ANY WARRANTIES OF PERFORMANCE OR
MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE OR PURPOSE (AS SET
FORTH IN UCC 2312-2313) OR FOR ANY PURPOSE WHATSOEVER, FOR THE
LICENSED PRODUCT, HOWEVER USED.

IN NO EVENT SHALL CALTECH/JPL BE LIABLE FOR ANY DAMAGES AND/OR
COSTS, INCLUDING BUT NOT LIMITED TO INCIDENTAL OR CONSEQUENTIAL
DAMAGES OF ANY KIND, INCLUDING ECONOMIC DAMAGE OR INJURY TO
PROPERTY AND LOST PROFITS, REGARDLESS OF WHETHER CALTECH/JPL SHALL
BE ADVISED, HAVE REASON TO KNOW, OR IN FACT SHALL KNOW OF THE
POSSIBILITY.

USER BEARS ALL RISK RELATING TO QUALITY AND PERFORMANCE OF THE
SOFTWARE AND/OR RELATED MATERIALS.
*******************************************************************
Copyright 2002-2011, by the California Institute of Technology. ALL
RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.

This software and/or related materials may be subject to U.S. export
control laws.  By accepting this software and related materials, the
user agrees to comply with all applicable U.S. export laws and
regulations.  User has the responsibility to obtain export licenses
or other export authority as may be required before exporting the
software or related materials to foreign countries or providing
access to foreign persons.
*******************************************************************

Highly incomplete and preliminary notes on building the ICI library:

To build, type:

	make
	make install

The "install" target assumes that you want binaries copied into /opt/bin
(which is assumed to be in your execution path), libraries copied into 
/opt/lib (which is assumed to be in your library path), header files
copied into /opt/include, and man pages copied into /opt/man/man1 and
man3 and man5.  If you want everything in some other top-level directory,
such as /usr, just change "OPT = /opt" to "OPT = /usr" in the top-level
Makefile.  If you've got an altogether different directory structure,
you will need to hack the platform-specific Makefiles.

Scott Burleigh, JPL
scott.c.burleigh@jpl.nasa.gov
