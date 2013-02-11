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
Copyright 2002-2013, by the California Institute of Technology. ALL
RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.

This software and/or related materials may be subject to U.S. export
control laws.  By accepting this software and related materials, the
user agrees to comply with all applicable U.S. export laws and
regulations.  User has the responsibility to obtain export licenses
or other export authority as may be required before exporting the
software or related materials to foreign countries or providing
access to foreign persons.
*******************************************************************

To build the entire ION system on a Linux, OS/X, or Solaris platform,
just cd into ion-open-source and enter two commands:

	./configure
	./make

To build ION for Android, cd into ion-open-source/arch-android and
see the instructions in the README.bionic text file.

To build ION for RTEMS, cd into ion-open-source/arch-rtems and
see the instructions in the README text file.

To build ION for Windows, see the instructions in the winion.pdf document.

It's also possible to build the individual packages of ION, using
platform-specific Makefiles in the package subdirectories.  If you choose
this option, be aware of the dependencies among the packages:

	The "ici" package must be built ("make" and "make install")
	before any other package.

	The "bp" package is dependent on "dgr" and "ltp" as well as
	"ici".

	The "cfdp", "ams", and "bss" packages are dependent on "bp".

	The "restart" package is dependent on "cfdp", "bp", "ltp",
	and "ici".

Additional details are provided in the README.txt files in the root
directories of some of the subsystems.

Note that the default individual build of the ams package requires that
the "expat" library be installed; this can be overridden at compile time.

Also note that all Makefiles are for gmake; on a freebsd platform, be sure
to install gmake before trying to build ION.

Scott Burleigh, JPL
scott.c.burleigh@jpl.nasa.gov 
