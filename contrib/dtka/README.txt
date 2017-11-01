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

1.	In the top-level DTKA directory, create a symbolic link to
ion-open-source/bp named "bp".

2.	Download zfec-1.4.24, untar it, and apply the fec.h.patch patch
to zfec-1.4.24/zfec/fec.h.  (We needed to do this to get zfec to build.
There is undoubtedly a better fix, but we haven't found it yet.)

3.	In the top-level DTKA directory, create a symbolic link to
zfec-1.4.24/zfec named "zfec".

4.	Then in the top-level dtka directory enter "make clean", "make",
and "make install".

To test the build, cd into ion-open-source/tests/dtka and enter "./dotest".
But note that the test will not succeed until you build with cryptography
software installed as discussed on the knclock(1) man page.

The prototype now includes key authority configuration assessment tools
developed by Michael Stadler.  The dtka_workbook.xlsm file is an Excel
spreadsheet that automatically computes a number of useful metrics based
on the user-supplied values of several configuration parameters.  The
dynamicDTKA script is a generalization of the ion-open-source/tests/dtka
test system, enabling the configuration of the test network to be declared
at runtime; the dynamicDTKA.test script tests that generalized test script.

In addition, Michael heavily annotated the dtka source code to help make
it easier for developers to work with.  The annotated source deviates somewhat
from the ION Style Guide, so the revised code will be included in the next
release of ION following some reformatting.
