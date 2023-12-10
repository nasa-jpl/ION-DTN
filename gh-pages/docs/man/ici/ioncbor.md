# NAME

cbor - ION library for encoding and decoding CBOR data representations

# SYNOPSIS

    #include "cbor.h"

# DESCRIPTION

ION's "cbor" library implements a subset of the Concise Binary Object
Representation (CBOR) standard, RFC 7049; only those data types used in
ION code are implemented.  Unlike other CBOR implementations, ION CBOR
is specifically intended for compatibility with zero-copy objects, i.e.,
the data being decoded need not all be in a memory buffer.

For all functions, _\*cursor_ is a pointer to the location in the CBOR
coding buffer at which bytes are to be encoded or decoded.  This pointer
is automatically advanced as the encoding or decoding operation is
performed.

Most of the ION CBOR decoding functions entail the decoding of unsigned
integers.  The invoking code may require that an integer representation
have a specific size by indicating the integer size "class" that is
required.  Class -1 indicates that an integer of any size is acceptable;
the other classes (0, 1, 2, 4, 8) indicate the number of bytes of integer
data that MUST follow the integers initial byte.

- int cbor\_encode\_integer(uvast value, unsigned char \*\*cursor)

    Represent this value in an integer of the smallest possible integer class.
    Cursor is automatically advanced.  Returns number of bytes written.

- int cbor\_encode\_fixed\_int(uvast value, int class, unsigned char \*\*cursor)

    Represent this value in an integer of the indicated class.  Cursor is
    automatically advanced.  Returns number of bytes written, 0 on encoding error.

- int cbor\_encode\_byte\_string(unsigned char \*value, uvast size, unsigned char \*\*cursor)

    _size_ is the number of bytes to write.  If value is NULL, only the size of
    the byte string is written; otherwise the byte string itself is written as
    well.  Cursor is advanced by the number of bytes written in either case.
    Returns number of bytes written.

- int cbor\_encode\_text\_string(char \*value, uvast size, unsigned char \*\*cursor)

    _size_ is the number of bytes to write.  If value is NULL, only the size of
    the text string is written; otherwise the text string itself is written
    as well.  Cursor is advanced by the number of bytes written in either case.
    Returns number of bytes written.	

- int cbor\_encode\_array\_open(uvast size, unsigned char \*\*cursor)

    If _size_ is ((uvast) -1), the array is of indefinite size; otherwise _size_
    indicates the number of items in the array.  Cursor is automatically advanced.
    Returns number of bytes written.

- int cbor\_encode\_break(unsigned char \*\*cursor)

    Break code is written at the indicated location.  Cursor is automatically
    advanced.  Returns number of bytes written (always 1).

- int cbor\_decode\_initial\_byte(unsigned char \*\*cursor, unsigned int \*bytesBuffered, int \*majorType, int \*additionalInfo)

    This function just extracts major type and additional info from the byte
    identified by _cursor_.  Cursor is automatically advanced.  Returns number of
    bytes decoded (always 1) or 0 on decoding error (e.g., no byte to decode).

- int cbor\_decode\_integer(	uvast \*value, int class, unsigned char \*\*cursor, unsigned int \*bytesBuffered)

    If _class_ is CborAny, any class of data item is accepted; otherwise only an
    integer data item of the indicated class is accepted.  Cursor is automatically
    advanced.  Returns number of bytes read, 0 on decoding error (e.g., integer
    is of the wrong class).

- int cbor\_decode\_byte\_string(unsigned char \*value, uvast \*size, unsigned char \*\*cursor, unsigned int \*bytesBuffered)

    Initial value of _size_ is the maximum allowable size of the decoded byte
    string; the actual number of bytes in the byte string (which, **NOTE**, is
    less than the number of bytes read) is returned in _size_.  If _value_ is
    non-NULL, the decoded byte string is copied into _value_ and cursor is
    automatically advanced to the end of the byte string; otherwise, cursor is
    advanced only to the beginning of the byte string.  Returns number of bytes
    read, 0 on decoding error (e.g., byte string exceeds maximum size).

- int cbor\_decode\_text\_string(char \*value, uvast \*size, unsigned char \*\*cursor, unsigned int \*bytesBuffered)

    Initial value of _size_ is the maximum allowable size of the decoded text
    string; the actual number of bytes in the text string (which, **NOTE**, is
    less than the number of bytes read) is returned in size.  If _value_ is
    non-NULL, the decoded text string is copied into _value_ and cursor is
    automatically advanced to the end of the text string; otherwise, cursor
    is advanced only to the beginning of the text string.  Returns number of
    bytes read, 0 on decoding error (e.g., text string exceeds maximum size).

- int cbor\_decode\_array\_open(uvast \*size, unsigned char \*\*cursor, unsigned int \*bytesBuffered)

    If _size_ is zero, any array is accepted and the actual size of the decoded
    array is returned in _size_; ((uvast) -1) is returned in _size_ if the array
    is of indefinite size.  If _size_ is ((uvast) -1), **only** an array of
    indefinite length is accepted.  Otherwise, _size_ indicates the required
    number of items in the array.  Cursor is automatically advanced.  Returns
    number of bytes read, 0 on decoding error (such as wrong number of items).

- int cbor\_decode\_break(unsigned char \*\*cursor, unsigned int \*bytesBuffered)

    Break code is read from the indicated location.  Cursor is automatically
    advanced.  Returns number of bytes read, 0 on decoding error (e.g., no
    break character at this location).
