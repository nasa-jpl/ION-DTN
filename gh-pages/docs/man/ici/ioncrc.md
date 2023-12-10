# NAME

crc - ION library for computing several types of checksums.

# SYNOPSIS

    #include "crc.h"

# DESCRIPTION

ION's "crc" library implements functions for computing four types of
checksums: X.25 (16-bit), bzip2 (32-bit), CRC32 (32-bit), and CRC32C
(32-bit).

All checksum computation functions were provided by Antara Teknik, LLC.

- uint16\_t ion\_CRC16\_1021\_X25(const char \*data, uint32\_t dLen, uint16\_t crc)

    Computes the CRC16 value for poly 0x1021.  _data_ points to the data block
    over which the checksum value is to be computed, _len_ must be the length of
    that data block, and _crc_ is the current value of the checksum that is
    being incrementally computed over a multi-block extent of data (zero for
    the first block of this extent, or if this block is the entire extent).

- uint32\_t ion\_CRC32\_04C11DB7\_bzip2(const char \*data, uint32\_t dLen, uint32\_t crc)

    Computes the bzip2 CRC32 checksum value for poly 0x04c11db7.  _data_ points
    to the data block over which the checksum value is to be computed, _len_ must
    be the length of that data block, and _crc_ is the current value of the
    checksum that is being incrementally computed over a multi-block extent of
    data (zero for the first block of this extent, or if this block is the entire
    extent).

- uint32\_t ion\_CRC32\_04C11DB7(const char \*data, uint32\_t dLen, uint32\_t crc)

    Computes the ISO-HDLC CRC32 value for poly 0x04c11db7.  _data_ points to the
    data block over which the checksum value is to be computed, _len_ must be the
    length of that data block, and _crc_ is the current value of the checksum
    that is being incrementally computed over a multi-block extent of data (zero
    for the first block of this extent, or if this block is the entire extent).

- uint32\_t ion\_CRC32\_1EDC6F41\_C(const char \*data, uint32\_t dLen, uint32\_t crc)

    Computes the CRC32C value for poly 0x1edc6f41.  _data_ points to the data
    block over which the checksum value is to be computed, _len_ must be the
    length of that data block, and _crc_ is the current value of the checksum
    that is being incrementally computed over a multi-block extent of data (zero
    for the first block of this extent, or if this block is the entire extent).
