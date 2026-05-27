# NASA/JPL Interplanetary Overlay Network (ION)

[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://GitHub.com/lasuzuki/StrapDown.js/graphs/commit-activity)
[![Open Source Love svg1](https://badges.frapsoft.com/os/v1/open-source.svg?v=103)](https://github.com/ellerbrock/open-source-badges/)

## What is ION

NASA’s Interplanetary Overlay Network (ION) is an implementation of Delay/Disruption Tolerant Networking (DTN).

DTN is a digital communication networking technology that enables data to be conveyed reliably among communicating entities when round-trip times may be highly variable and/or very long. Data transmission is performed automatically and reliably even if one or more of the network links in the end-to-end path between those entities is subject to very long signal propagation latency and/or prolonged intervals of unavailability.

ION DTN is suitable for both planetary and interplanetary environments succeptible to communication delay and disruption.

## Supported Platform

ION Open Source supports Linux and Solaris. Please consult the [Quick Start](./gh-pages/docs/ION-Quick-Start-Guide.md) for detailed instruction.

Support for `FreeBSD`, `macOS`, `Raspberry Pi OS` are provided on a best-effort, test and report (BETR) level. We will try, with reasonable effort, to test, document and fix issues related to these platforms but they are not officially supported.

Prototype demonstrations on how to build ION on `Android`, `RTEMS`, and `ARM-based AT91SAM9G20 board` are provided as is. Available from ION 4.1.3s or earlier; deprecated as of ION 4.1.4-a.2.

## ION-related Software Packages

### ION Open Source (IOS) Software Suite (this repository)

IOS is a full featured collection of latest ION software, containing operational modules such as BP and LTP, experimental prototypes for upcoming features, and utilities/add-ons provided by external contributors. This package facilitates open-source development of ION, DTN technology research, and protocol standardization-related testing.

<https://github.com/nasa-jpl/ion-dtn>

### ION Core Software Package

ION-core is a streamlined package containing only a subset of core Bundle Protocol features, convergence layer adaptor (link) and applications that has been flight demonstrated and hardened via extensive end-to-end testing and stress testing. It provides the ability to selective compile a subset of modules.

This package is designed for infusion/prototyping for embedded flight and ground systems.

<https://github.com/nasa-jpl/ion-core>

### PyION

A python-based interface for rapid prototyping of DTN applications and DTN testing.

<https://github.com/nasa-jpl/pyion>

### ION Configuration Tool & ION Network Model

Both Web-based and Command Line Interfaces for automated generation of ION configuration files.

<https://github.com/nasa-jpl/ion-config-tool>

<https://github.com/nasa-jpl/ion-network-model>

## ION Versioning Scheme

ION uses a versioning scheme of the form `X.Y.Z[-suffix].N`, where:

- `X` is the major version number, incremented for significant changes that may include backward-incompatible changes.
- `Y` is the minor version number, incremented for backward-compatible feature additions and improvements.
- `Z` is the patch version number, incremented for backward-compatible bug fixes.
- `[-suffix]` is an optional suffix that may include additional information such as pre-release identifiers ('a' for alpha, 'b' for beta) or build metadata.
- `N` is the build number, incremented for each build of a given `X.Y.Z[-suffix]` version.

- Each stable release is tagged in the Git repository with a tag of the form `ion-open-source-X.Y.Z`.
- Each alpha and beta release is tagged in the Git repository with a tag of the form `ion-open-source-X.Y.Z-suffix.1`. A release package is created for the first build of each alpha and beta release (i.e., build number `1`).
- Online documentation is versioned according to the stable release and build 1 of alpha/beta releases.
- Starting with ION 4.1.4-b.1, we will be providing tags, but not release packages, for subsequent builds of alpha/beta releases (i.e., build numbers greater than `1`) to facilitate testing and development and wider, quicker access to the latest code.

## ION Documentation

Beginning with version 4.1.3, ION documentation is hosted here [online](https://ion-dtn.readthedocs.io).

Under the `doc` subfolder, there are older versions of ION documents kept for user's reference if needed, but they will not be updated going forward and may be removed without prior notice. Users are encouraged to use the online documentation in order to receive the most up-to-date information on ION features and issues.

A template [`AGENT.md`](gh-pages/docs/AGENTS.md) file is provided with the documention.

The `release-notes.txt` file will continue to be updated for each stable release.

## Other Online Sources

- Videos and documents and links to videos of the Interplanetary Overlay Network course can be found on the [NASA](https://www.nasa.gov/directorates/heo/scan/engineering/technology/disruption_tolerant_networking_software_options_ion) website.

- For details about changes regarding each ION release, please see the [Release Notes](./release-notes.txt)

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
    *Redistributions of source code must retain the above copyright
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
