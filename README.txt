*********************************************************************
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
*********************************************************************
Copyright 2002-2013, by the California Institute of Technology. ALL
RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.

This software and/or related materials may be subject to U.S. export
control laws.  By accepting this software and related materials, the
user agrees to comply with all applicable U.S. export laws and
regulations.  User has the responsibility to obtain export licenses
or other export authority as may be required before exporting the
software or related materials to foreign countries or providing
access to foreign persons.
*********************************************************************
The QCBOR code included is distributed with the following condition

Copyright (c) 2016-2018, The Linux Foundation.
Copyright (c) 2018-2019, Laurence Lundblade.
All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors, nor the name "Laurence Lundblade" may be used to
      endorse or promote products derived from this software without
      specific prior written permission.
 
THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************

To build and install the entire ION system on a Linux, OS/X, or Solaris
platform, cd into ion-open-source and enter three commands:

	./configure (If configure is not present run: "autoreconf -fi" first)
	make
	sudo make install

NOTE: if you want to set overriding compile-time switches for a build, 
the place to do this is in the ./configure command.  For details,

	./configure -h

To build ION for Windows, please download the Windows installer.

It's also possible to build the individual packages of ION, using
platform-specific Makefiles in the package subdirectories.  Currently 
the only actively maintained platform-specific Makefile is for 64-bits 
Linux under the "i86_48-fedora" folder. If you choose
this option, be aware of the dependencies among the packages:

	The "ici" package must be built ("make" and "make install")
	before any other package.

	The "bp" package is dependent on "dgr" and "ltp" and "bssp"
	as well as "ici".

	The "cfdp", "ams", "bss", and "dtpc" packages are dependent on "bp".

	The "restart" package is dependent on "cfdp", "bp", "ltp",
	and "ici".

Also, be aware that these Makefiles install everything into subdirectories
of /usr/local.  To override this behavior, change the value of OPT in the
top-level Makefile of each package.

Additional details are provided in the README.txt files in the root
directories of some of the subsystems.

Note that all Makefiles are for gmake; on a freebsd platform, be sure
to install gmake before trying to build ION.

A quick start guide can be found on the ION-DTN SourceForge page at:
https://sourceforge.net/p/ion-dtn/wiki/Home/

Tutorials can be found on the ION-DTN Sourceforge page at
https://sourceforge.net/p/ion-dtn/wiki/NASA_ION_Course/

Jay L. Gao, JPL/Caltech
Jay.L.Gao@jpl.nasa.gov
