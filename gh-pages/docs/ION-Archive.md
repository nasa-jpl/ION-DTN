# ION Archive

Updated: September 21, 2025

This document describes removed ION Software and provide references to archived versions.

## Bundle Protocol v6 Implementation

`bpv6` - The last version of ION release that contains a tested version of the Bundle Protocol v6 implementation can be found in the [ION 4.1.3s Release](https://github.com/nasa-jpl/ION-DTN/releases/tag/ion-open-source-4.1.3s). Functionally, the BPv6 version in ION 4.1.3s is equivalent to ION 3.7.4.

## Deprecated Ports

`arch-android` and `arch-uClibc` - are demonstration ports of ION based on ION 3.x (bpv6) and were not actively maintained.

Please see [ION 4.1.3s Release](https://github.com/nasa-jpl/ION-DTN/releases/tag/ion-open-source-4.1.3s) or earlier versions and consult the README documentations under each folder for dependencies and build instructions. There is no expectation that they will work with modern toolchains or operating systems, and they are provided "as is" for historical reference only.

**Note on RTEMS:** The legacy `arch-rtems` SPARC/RTEMS 5.1 port from ION 3.x is deprecated. However, a new **RTEMS 6.1 ARM64 port** (ION 4.1.4+) is now available and actively maintained. See the [RTEMS 6.1 Port Quick Start Guide](RTEMS6-Installation.md) for the modern BPv7 implementation.

## Depreciated Platform-specific Development Makefiles

The following platform-specific development Makefiles have been removed from the ION source tree:

- `i86-darwin` - Intel Darwin (Mac OS X)
- `i86-freebsd` - Intel FreeBSD
- `i86-mingw` - Intel MinGW (Windows)
- `i86-redhat` - Intel Red Hat Linux
- `spart-sol9` - SPARC Solaris 9

They were not actively maintained and are provided "as is" for historical reference only. Please see [ION 4.1.3s Release](https://github.com/nasa-jpl/ION-DTN/releases/tag/ion-open-source-4.1.3s) or earlier versions and consult the README documentation for dependencies and build instructions.

The only development Makefile for each submodule that will be actively maintained is `x86_64-linux` for 64-bit Linux systems. It is designed to work on most modern Linux distributions with GNU Make and GCC installed.
