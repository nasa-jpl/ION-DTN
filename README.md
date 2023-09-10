# 🛰️ NASA/JPL Interplanetary Overlay Network (ION)

[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://GitHub.com/lasuzuki/StrapDown.js/graphs/commit-activity)
[![Open Source Love svg1](https://badges.frapsoft.com/os/v1/open-source.svg?v=103)](https://github.com/ellerbrock/open-source-badges/)

# What is ION? 
The NASA’s Interplanetary Overlay Network (ION) is an implementation of Delay/Disruption Tolerant Networking (DTN). 

DTN is a digital communication networking technology that enables data to be conveyed reliably among communicating entities when roundtrip times may be highly variable
and/or very long. Data transmission is done automatically and reliably even if one or more of the network links in the end-to-end path between those entities is subject to very long signal propagation latency and/or prolonged intervals of unavailability.

ION DTN is suitable to both planetary and interplanetary environments succeptible to communication delay and disruption.

## Quick Start

**1. Linux, MacOS or Solaris**
	To build and install the entire ION system on a `Linux, OS/X, or Solaris
platform`, cd into ion-open-source and enter three commands:

````
$ ./configure (If configure is not present run: "autoreconf -fi" first)
$ make
$ sudo make install
````
`NOTE`: if you want to set overriding compile-time switches for a build, 
the place to do this is in the ./configure command.  For details,

````
$ ./configure -h
````

**2. Android**

	To build ION for `Android`, cd into ion-open-source/arch-android and see the instructions in the `README.bionic` text file.

**3. RTEMS**

	To build ION for `RTEMS`, cd into ion-open-source/arch-rtems and see the instructions in the README text file.

**4. Windows** 

	To build ION for `Windows`, see the instructions in the "Building ION from source on Windows.pdf" document.

**5. ARM-based AT91SAM9G20 board**

	To build ION for the `ARM-based AT91SAM9G20 board`, cd into ion-open-source/arch-uClibc and see the instructions in the "ARM build.pdf" file. 

**6. Build individual ION packages**

	It's also possible to build the individual packages of ION, using platform-specific Makefiles in the package subdirectories.  If you choose this option, be aware of the dependencies among the packages:

* The `ici` package must be built (`make` and `make install`) before any other package.

* The `bp` package is dependent on `dgr` and "ltp" and `bssp` as well as `ici`.
  		
* The `cfdp`, `ams`, `bss`, and `dtpc` packages are dependent on `bp`.

* The `restart` package is dependent on `cfdp`, `bp`, `ltp`, and `ici`.

Also, be aware that these Makefiles install everything into subdirectories
of /usr/local.  To override this behavior, change the value of OPT in the
top-level Makefile of each package.

Additional details are provided in the README.txt files in the root
directories of some of the subsystems.

Note that all Makefiles are for gmake; on a freebsd platform, be sure
to install gmake before trying to build ION.

## Tutorials and More

* Tutorials, applications of Artificial Intelligence and IoT, and further details on ION capabilities can be found on the [Wiki](https://github.com/nasa-jpl/ION-DTN/wiki) pages.

* Videos and documents and links to videos of the Interplanetary Overlay Network course can be found on the [NASA](https://www.nasa.gov/directorates/heo/scan/engineering/technology/disruption_tolerant_networking_software_options_ion) website.

* For details about changes regarding each ION release, please see the [Release Notes](./release-notes.txt)

## Copyright and No Warranty Disclaimer

The software and/or related materials are provided "AS-IS" without
warranty of any kind including any warranties of performance or
merchantability or fitness for a particular use or purpose (as set
forth in UCC 2312-2313) or for any purpose whatsoever, for the
licensed product, however used.

In no event shall caltech/jpl be liable for any damages and/or
costs, including but not limited to incidental or consequential
damages of any kind, including economic damage or injury to
property and lost profits, regardless of whether Caltech/JPL shall
be advised, have reason to know, or in fact shall know of the
Possibility.

User bears all risk relating to quality and performance of the
software and/or related materials.

**Copyright 2002-2013, by the California Institute of Technology. ALL
RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.**

This software and/or related materials may be subject to U.S. export
control laws.  By accepting this software and related materials, the
user agrees to comply with all applicable U.S. export laws and
regulations.  User has the responsibility to obtain export licenses
or other export authority as may be required before exporting the
software or related materials to foreign countries or providing
access to foreign persons.

The QCBOR code included is distributed with the following condition

**Copyright (c) 2016-2018, The Linux Foundation.
Copyright (c) 2018-2019, Laurence Lundblade.
All rights reserved.**
 
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
 
This software is provided "AS IS" and any express or implied
warranties, including, but not limited to, the implied warranties of
merchantability, fitness for a particular purpose and non-infringement
are disclaimed.  In no event shall the copyright owner or contributors
be liable for any direct, indirect, incidental, special, exemplary, or
consequential damages (including, but not limited to, procurement of
substitute goods or services; loss of use, data, or profits; or
business interruption) however caused and on any theory of liability,
whether in contract, strict liability, or tort (including negligence
or otherwise) arising in any way out of the use of this software, even
ff advised of the possibility of such damage.