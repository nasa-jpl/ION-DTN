# NAME

sdrstring - Simple Data Recorder string functions

# SYNOPSIS

    #include "sdr.h"

    Object sdr_string_create (Sdr sdr, char *from);
    Object sdr_string_dup    (Sdr sdr, Object from);
    int    sdr_string_length (Sdr sdr, Object string);
    int    sdr_string_read   (Sdr sdr, char *into, Object string);

# DESCRIPTION

SDR strings are used to record strings of up to 255 ASCII characters
in the heap space of an SDR.  Unlike standard C strings, which are terminated
by a zero byte, SDR strings record the length of the string as
part of the string object.

To store strings longer than 255 characters, use sdr\_malloc() and sdr\_write()
instead of these functions.

- Object sdr\_string\_create(Sdr sdr, char \*from)

    Creates a "self-delimited string" in the heap of the
    indicated SDR, allocating the required space and copying the
    indicated content.  _from_ must be a standard C
    string for which strlen() must not exceed 255; if
    it does, or if insufficient SDR space is available, 0
    is returned.  Otherwise the address of the newly created SDR
    string object is returned.  To destroy, just use sdr\_free().

- Object sdr\_string\_dup(Sdr sdr, Object from)

    Creates a duplicate of the SDR string whose address is
    _from_, allocating the required space and copying the
    original string's content.  If insufficient SDR space is
    available, 0 is returned.  Otherwise the address of the newly
    created copy of the original SDR string object is returned.  To
    destroy, use sdr\_free().

- int sdr\_string\_length(Sdr sdr, Object string)

    Returns the length of the indicated self-delimited string (as would
    be returned by strlen()), or -1 on any error.

- int sdr\_string\_read(Sdr sdr, char \*into, Object string)

    Retrieves the content of the indicated self-delimited string into
    memory as a standard C string (NULL terminated).  Length of _into_
    should normally be SDRSTRING\_BUFSZ (i.e., 256) to allow for the largest
    possible SDR string (255 characters) plus the terminating NULL.  Returns
    length of string (as would be returned by strlen()), or -1 on any error.

# SEE ALSO

sdr(3), sdrlist(3), sdrtable(3), string(3)
