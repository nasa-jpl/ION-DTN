This directory contains documentation resources for NM.

## Directories

### adms
This directory contains JSON files describing the currently implemented ADMs.  These files were used as input to automatically generate the associated ADM source files included in this distribution using the CAMP tool (available in the ../contrib directory).

The index.json file provides the full name and IETF references for each file.  The order of entires in the namespaces array matches the defined namespace identifier indexes.

### pod*
This file contains Perl POD formatted documentation used by ION to generate man page entries as a part of the standard build.

### doxygen
This directory contains Doxygen source and configuration files.  Doxygen can be used to generate detailed HTML documentation and API references.  See the README.md in this directory for instructions on running Doxygen.
