Release Notes            {#releases}
==========

This page describes significant changes in NM in recent releases of ION.

## ION 4.0.0
NM has been upgraded to support the latest revision of the AMP specification.  Prior releases were based on [AMPv6](https://tools.ietf.org/html/draft-birrane-dtn-amp-06).  This version has been updated to support [AMPv8](https://tools.ietf.org/html/draft-birrane-dtn-amp-08).

These updates change the format of the CBOR-encoded messages and, as such, break backwards compatibility when enabled.  The updated specification optimizes the size of the CBOR encodings by eliminating selected CBOR container delimiters when not required for processing by NM through the newly introduced concept of 'OCTETS'.  

An OCTETS sequence is functionally equivalent to a CBOR indefinite-length array for which the header and size bytes are omitted from the encoded CBOR data.  The NM application explicitly instructs the QCBOR encoder as to the start and end of an octets sequence.  This enables the entirety of the OCTETS sequence to be counted as a single element in any encapsulating CBOR array and for error detection (ie: of unmatched array opens & closes) to proceed as normal.

Two new (experimental) UI features have been added to aide automation.  The 'Automator UI mode' provides a simple shell-style interface to the Manager optimized for scripted usage.  The second is a preliminary REST API. This API is subject change in future releases.  See (Manager UI)(@ref nmui) for details.

Command line options have been added to nm_mgr for configuration of expanded file logging capabilities.

This release includes also includes several bugfixes, updates to the CAMP tool for generating ADM source files, and the inclusion of this Doxygen documentation.

## ION 3.7.0
This release upgraded the CBOR library used by NM from tinycbor to QCBOR.  The primary motivation behind this change was to ensure all CBOR functionality is implemented in an endian neutral manner.

This release includes numerous bugfixes.
