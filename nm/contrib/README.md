Utilities & Libraries
==============

This directory contains a collection of utilities and third party libraries.  Refer to the associated README files for details on ach component.

## Utiltiies
- CAMP - CAMpPython is a code generator utilizing ADM JSON definition files (see docs/adms) as input. Outputs include NM manager and agent C sources to process the associated ADMs, and SQL files for the associated atabase entries.
- amp-sql - MySQL Database Schema for usage with the manager
- amp.me - A collection of command-line and web UI utilities. Key features include parsing and conversion of AMP messages in URI, Hex, and visual representations, plus sample interfaces to the SQL schema. 


## Third-party libraries
- QCBOR - An open-source CBOR library.  https://github.com/laurencelundblade/QCBOR
- CivetWeb - An embedded HTTP server used by NM Manager (if built with --enable-nmrest) to provide a REST API interface. 
  - https://github.com/civetweb/civetweb
