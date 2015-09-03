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
Copyright 2002-2015, by the California Institute of Technology. ALL
RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.

This software and/or related materials may be subject to U.S. export
control laws.  By accepting this software and related materials, the
user agrees to comply with all applicable U.S. export laws and
regulations.  User has the responsibility to obtain export licenses
or other export authority as may be required before exporting the
software or related materials to foreign countries or providing
access to foreign persons.
*******************************************************************

To build this prototype DTKA implementation:

1.	If it's not already installed, install a version of ION that
includes dtka support.  In your home directory, create a symbolic link
to ion-open-source/bp named "bp".

2.	Download PolarSSL and un-tar it.  polarssl-1.2.8 was used in
development of the DTKA code, but more recent releases might also work.

3.	Build PolarSSL: cd into the PolarSSL directory and enter
"make clean", "make lib", and "make install".  The resulting library
should end up in /usr/local/lib and the header files should be in
/usr/local/include/polarssl.

4.	Then cd into /usr/local/lib and

		ln -s libpolarssl.so libpolarssl.so.0
		
(The build should do this automatically, or make it unnecessary somehow,
but we haven't yet figured out how to make that happen.)

5.	Download zfec-1.4.24 into the top-level DTKA directory and untar it.

6.	Apply the fec.h.patch patch to zfec-1.4.24/zfec/fec.h.  (We needed
to do this to get zfec to build.  There is undoubtedly a better fix, but we
haven't found it yet.)

7.	Then in the top-level dtka directory enter "make clean", "make",
and "make install".

To test the build, cd into ion-open-source/tests/dtka and enter "./dotest".
