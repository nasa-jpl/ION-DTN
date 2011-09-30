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

To build the ION system, build the subsystems in the following order:

1.	ici
2.	dgr
3.	ltp
4.	bp
5.	cfdp
6.	ams

Additional details are provided in the README.txt files in the root
directories of some of the subsystems.

Note that the default build of the ams subsystem requires that the "expat"
library be installed; this can be overridden at compile time.  Also note
that all Makefiles are for gmake; on a freebsd platform, be sure to install
gmake before trying to build ION.

Scott Burleigh, JPL
scott.c.burleigh@jpl.nasa.gov 
