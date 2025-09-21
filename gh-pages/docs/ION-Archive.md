# ION Archive 

Updated: September 21, 2025

This document describes removed ION Software and provide references to archived versions.

## Bundle Protocol v6 Implementation

`bpv6` - The last version of ION release that contains a tested version of the Bundle Protocol v6 implementation can be found in the [ION 4.1.3s Release](https://github.com/nasa-jpl/ION-DTN/releases/tag/ion-open-source-4.1.3s). Functionally, the BPv6 version in ION 4.1.3s is equivalent to ION 3.7.4.

## Deprecated Ports

`arch-android`, `arch-rtems` and `arch-uClibc` - are three demonstration ports of ION based on ION 3.x (bpv6) and were not actively maintained. 

Please see [ION 4.1.3s Release](https://github.com/nasa-jpl/ION-DTN/releases/tag/ion-open-source-4.1.3s) or earlier versions and consult the README documentations under each folder for dependencies and build instructions. There is no expectation that they will work with modern toolchains or operating systems, and they are provided "as is" for historical reference only.

## Depreciated Platform-specific Development Makefiles

The following platform-specific development Makefiles have been removed from the ION source tree:

- `i86-darwin` - Intel Darwin (Mac OS X)
- `i86-freebsd` - Intel FreeBSD
- `i86-mingw` - Intel MinGW (Windows)
- `i86-redhat` - Intel Red Hat Linux
- `spart-sol9` - SPARC Solaris 9

They were not actively maintained and are provided "as is" for historical reference only. Please see [ION 4.1.3s Release](https://github.com/nasa-jpl/ION-DTN/releases/tag/ion-open-source-4.1.3s) or earlier versions and consult the README documentation for dependencies and build instructions. 

The only development Makefile for each submodule that will be actively maintained is `i86_64-fedora` for Intel-based Linux systems. Although named for Fedora (historical reasons), it is designed to work on most modern Linux distributions with GNU Make and GCC installed.