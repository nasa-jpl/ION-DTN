# NAME

zco - library for manipulating zero-copy objects

# SYNOPSIS

    #include "zco.h"

    typedef enum
    {
        ZcoInbound = 0,
        ZcoOutbound = 1,
        ZcoUnknown = 2
    } ZcoAcct;

    typedef enum
    {
        ZcoFileSource = 1,
        ZcoBulkSource = 2,
        ZcoObjSource = 3,
        ZcoSdrSource = 4,
        ZcoZcoSource = 5
    } ZcoMedium;

    typedef void (*ZcoCallback)(ZcoAcct);

    [see description for available functions]

# DESCRIPTION

"Zero-copy objects" (ZCOs) are abstract data access representations
designed to minimize I/O in the encapsulation of application source
data within one or more layers of communication protocol structure.  ZCOs
are constructed within the heap space of an SDR to which implementations
of all layers of the stack must have access.  Each ZCO contains information
enabling access to the source data objects, together with (a) a linked list
of zero or more "extents" that reference portions of these source data
objects and (b) linked lists of protocol header and trailer capsules that
have been explicitly attached to the ZCO since its creation.  The
concatenation of the headers (in ascending stack sequence), source data
object extents, and trailers (in descending stack sequence) is what is to
be transmitted or has been received.

Each source data object may be either a file (identified by pathname
stored in a "file reference" object in SDR heap) or an item in mass
storage (identified by item number, with implementation-specific
semantics, stored in a "bulk reference" object in SDR heap) or an
object in SDR heap space (identified by heap address stored in an
"object reference" object in SDR heap) or an array of bytes in SDR
heap space (identified by heap address).  Each protocol header or
trailer capsule indicates the length and the address (within SDR
heap space) of a single protocol header or trailer at some layer
of the stack.  Note that the source data object for each ZCO extent
is specified indirectly, by reference to a content lien reference
structure that refers to a heap space object, mass storage item, or file;
the reference structures contain the actual locations of the source data
together with reference counts, enabling any number of "clones" of a
given ZCO extent to be constructed without consuming additional resources.
These reference counts ensure that the reference structures and the
source data items they refer to are deleted automatically when (and
only when) all ZCO extents that reference them have been deleted.

Note that the safety of shared access to a ZCO is protected by the
fact that the ZCO resides in SDR heap space and therefore cannot be modified
other than in the course of an SDR transaction, which serializes
access.  Moreover, extraction of data from a ZCO may entail the reading
of file-based source data extents, which may cause file progress to
be updated in one or more file reference objects in the SDR heap.  For
this reason, all ZCO "transmit" and "receive" functions must be performed
within SDR transactions.

Note also that ZCO can more broadly be used as a general-purpose
reference counting system for non-volatile data objects, where a
need for such a system is identified.

The total volume of file system space, mass storage space, and SDR heap
space that may be occupied by inbound and (separately) outbound ZCO extents
are system configuration parameters that may be set by ZCO library
functions.  Those limits are enforced when extents are appended to ZCOs:
total inbound and outbound ZCO file space, mass storage, and SDR heap
occupancy are updated continuously as ZCOs are created and destroyed,
and the formation of a new extent is prohibited when the length of the
extent exceeds the difference between the applicable limit and the
corresponding current occupancy total.  Doing separate accounting for
inbound and outbound ZCOs enables inbound ZCOs to be formed (for data
reception purposes) even when the total current volume of outbound ZCOs
has reached its limit, and vice versa.

- void zco\_register\_callback(ZcoCallback notify)

    This function registers the "callback" function that the ZCO system will
    invoke every time a ZCO is destroyed, making ZCO file, bulk, and/or heap space
    available for the formation of new ZCO extents.  This mechanism can be
    used, for example, to notify tasks that are waiting for ZCO space to be
    made available so that they can resume some communication protocol
    procedure.

- void zco\_unregister\_callback( )

    This function simply unregisters the currently registered callback function
    for ZCO destruction.

- Object zco\_create\_file\_ref(Sdr sdr, char \*pathName, char \*cleanupScript, ZcoAcct acct)

    Creates and returns a new file reference object, which can be used as the
    source data extent location for creating a ZCO whose source data object is
    the file identified by _pathName_.  _cleanupScript_, if not NULL, is invoked
    at the moment the last ZCO that cites this file reference is destroyed
    \[normally upon delivery either down to the "ZCO transition layer" of the
    protocol stack or up to a ZCO-capable application\].  A zero-length string
    is interpreted as implicit direction to delete the referenced file when
    the file reference object is destroyed.  Maximum length of _cleanupScript_
    is 255.  _acct_ must be ZcoInbound or ZcoOutbound, depending on whether
    the first ZCO that will reference this object will be inbound or outbound.
    Returns SDR location of file reference object on success, 0 on any
    error.

- Object zco\_revise\_file\_ref(Sdr sdr, Object fileRef, char \*pathName, char \*cleanupScript)

    Changes the _pathName_ and _cleanupScript_ of the indicated file
    reference.  The new values of these fields are validated as for
    zco\_create\_file\_ref().  Returns 0 on success, -1 on any error.

- char \*zco\_file\_ref\_path(Sdr sdr, Object fileRef, char \*buffer, int buflen)

    Retrieves the pathName associated with _fileRef_ and stores it in _buffer_,
    truncating it to fit (as indicated by _buflen_) and NULL-terminating it.  On
    success, returns _buffer_; returns NULL on any error.

- int zco\_file\_ref\_xmit\_eof(Sdr sdr, Object fileRef)

    Returns 1 if the last octet of the referenced file (as determined at the
    time the file reference object was created) has been read by ZCO via a
    reader with file offset tracking turned on.  Otherwise returns zero.

- void zco\_destroy\_file\_ref(Sdr sdr, Object fileRef)

    If the file reference object residing at location _fileRef_ within
    the indicated Sdr is no longer in use (no longer referenced by any ZCO),
    destroys this file reference object immediately.  Otherwise, flags this
    file reference object for destruction as soon as the last reference to
    it is removed.

- Object zco\_create\_bulk\_ref(Sdr sdr, unsigned long item, vast length, ZcoAcct acct)

    Creates and returns a new bulk reference object, which can be used as the
    source data extent location for creating a ZCO whose source data object is
    the mass storage item of length _length_ identified by _item_ (the semantics
    of which are implementation-dependent).  Note that the referenced item is
    automatically destroyed at the time that the last ZCO that cites this bulk
    reference is destroyed (normally upon delivery either down to the "ZCO
    transition layer" of the protocol stack or up to a ZCO-capable application).
    _acct_ must be ZcoInbound or ZcoOutbound, depending on whether the first
    ZCO that will reference this object will be inbound or outbound.  Returns
    SDR location of bulk reference object on success, 0 on any error.

- void zco\_destroy\_bulk\_ref(Sdr sdr, Object bulkRef)

    If the bulk reference object residing at location _bulkRef_ within
    the indicated Sdr is no longer in use (no longer referenced by any ZCO),
    destroys this bulk reference object immediately.  Otherwise, flags this
    bulk reference object for destruction as soon as the last reference to
    it is removed.

- Object zco\_create\_obj\_ref(Sdr sdr, Object object, vast length, ZcoAcct acct)

    Creates and returns a new object reference object, which can be used as the
    source data extent location for creating a ZCO whose source data object is
    the SDR heap object of length _length_ identified by _object_.  Note that
    the referenced object is automatically freed at the time that the last ZCO
    that cites this object reference is destroyed (normally upon delivery either
    down to the "ZCO transition layer" of the protocol stack or up to a
    ZCO-capable application).  _acct_ must be ZcoInbound or ZcoOutbound,
    depending on whether the first ZCO that will reference this object will
    be inbound or outbound.  Returns SDR location of object reference object
    on success, 0 on any error.

- void zco\_destroy\_obj\_ref(Sdr sdr, Object objRef)

    If the object reference object residing at location _objRef_ within
    the indicated Sdr is no longer in use (no longer referenced by any ZCO),
    destroys this object reference object immediately.  Otherwise, flags this
    object reference object for destruction as soon as the last reference to
    it is removed.

- void zco\_status(Sdr sdr)

    Uses the ION logging function to write a report of the current contents of
    the ZCO space accounting database.

- vast zco\_get\_file\_occupancy(Sdr sdr, ZcoAcct acct)

    Returns the total number of file system space bytes occupied by ZCOs (inbound
    or outbound) created in this Sdr.

- void zco\_set\_max\_file\_occupancy(Sdr sdr, vast occupancy, ZcoAcct acct)

    Declares the total number of file system space bytes that may be occupied by
    ZCOs (inbound or outbound) created in this Sdr.

- vast zco\_get\_max\_file\_occupancy(Sdr sdr, ZcoAcct acct)

    Returns the total number of file system space bytes that may be occupied by
    ZCOs (inbound or outbound) created in this Sdr.

- int zco\_enough\_file\_space(Sdr sdr, vast length, ZcoAcct acct)

    Returns 1 if the total remaining file system space available for ZCOs (inbound
    or outbound) in this Sdr is greater than _length_.  Returns 0 otherwise.

- vast zco\_get\_bulk\_occupancy(Sdr sdr, ZcoAcct acct)

    Returns the total number of mass storage space bytes occupied by ZCOs (inbound
    or outbound) created in this Sdr.

- void zco\_set\_max\_bulk\_occupancy(Sdr sdr, vast occupancy, ZcoAcct acct)

    Declares the total number of mass storage space bytes that may be occupied by
    ZCOs (inbound or outbound) created in this Sdr.

- vast zco\_get\_max\_bulk\_occupancy(Sdr sdr, ZcoAcct acct)

    Returns the total number of mass storage space bytes that may be occupied by
    ZCOs (inbound or outbound) created in this Sdr.

- int zco\_enough\_bulk\_space(Sdr sdr, vast length, ZcoAcct acct)

    Returns 1 if the total remaining mass storage space available for ZCOs (inbound
    or outbound) in this Sdr is greater than _length_.  Returns 0 otherwise.

- vast zco\_get\_heap\_occupancy(Sdr sdr, ZcoAcct acct)

    Returns the total number of SDR heap space bytes occupied by ZCOs (inbound or
    outbound) created in this Sdr.

- void zco\_set\_max\_heap\_occupancy(Sdr sdr, vast occupancy, ZcoAcct acct)

    Declares the total number of SDR heap space bytes that may be occupied by
    ZCOs (inbound or outbound) created in this Sdr.

- vast zco\_get\_max\_heap\_occupancy(Sdr sdr, ZcoAcct acct)

    Returns the total number of SDR heap space bytes that may be occupied by
    ZCOs (inbound or outbound) created in this Sdr.

- int zco\_enough\_heap\_space(Sdr sdr, vast length, ZcoAcct acct)

    Returns 1 if the total remaining SDR heap space available for ZCOs (inbound or
    outbound) in this Sdr is greater than _length_.  Returns 0 otherwise.

- int zco\_extent\_too\_large(Sdr sdr, ZcoMedium source, vast length, ZcoAcct acct)

    Returns 1 if the total remaining space available for ZCOs (inbound or outbound)
    is NOT enough to contain a new extent of the indicated length in the indicated
    source medium.  Returns 0 otherwise.

- int zco\_get\_aggregate\_length(Sdr sdr, Object location, vast offset, vast length, vast \*fileSpaceOccupied, vast \*bulkSpaceOccupied, vast \*heapSpaceOccupied)

    Populates _\*fileSpaceOccupied_, _\*bulkSpaceOccupied_, and
    _\*heapSpaceOccupied_ with the total number of ZCO space bytes occupied by
    the extents of the zco at _location_, from _offset_ to _offset + length_.
    If _offset_ isn't the start of an extent or _offset + length_ isn't the
    end of an extent, returns -1 in all three fields.

- Object zco\_create(Sdr sdr, ZcoMedium firstExtentSourceMedium, Object firstExtentLocation, vast firstExtentOffset, vast firstExtentLength, ZcoAcct acct)

    Creates a new inbound or outbound ZCO.  _firstExtentLocation_ and
    _firstExtentLength_ must either both be zero (indicating that
    zco\_append\_extent() will be used to insert the first source data extent
    later) or else both be non-zero.  If _firstExtentLocation_ is non-zero,
    then (a) _firstExtentLocation_ must be the SDR location of a file
    reference object, bulk reference object, object reference object, SDR heap
    object, or ZCO, depending on the value of _firstExtentSourceMedium_, and
    (b) _firstExtentOffset_ indicates how many leading bytes of the source
    data object should be skipped over when adding the initial source data
    extent to the new ZCO.  A negative value for
    _firstExtentLength_ indicates that the extent is already known not to be
    too large for the available ZCO space, and the actual length of the extent
    is the additive inverse of this value.  On success, returns the SDR location
    of the new ZCO.  Returns 0 if there is insufficient ZCO space for creation
    of the new ZCO; returns ((Object) -1) on any error.

- int zco\_append\_extent(Sdr sdr, Object zco, ZcoMedium sourceMedium, Object location, vast offset, vast length)

    Appends the indicated source data extent to the indicated ZCO, as described
    for zco\_create().  Both the _location_ and _length_ of the source data
    must be non-zero.  A negative value for _length_ indicates that the extent
    is already known not to be too large for the available ZCO space, and the
    actual length of the extent is the additive inverse of this value.  For
    constraints on the value of _location_, see zco\_create().  Returns
    _length_ on success, 0 if there is insufficient ZCO space for creation of
    the new source data extent, -1 on any error.

- int zco\_prepend\_header(Sdr sdr, Object zco, char \*header, vast length)
- int zco\_append\_trailer(Sdr sdr, Object zco, char \*trailer, vast length)
- void zco\_discard\_first\_header(Sdr sdr, Object zco)
- void zco\_discard\_last\_trailer(Sdr sdr, Object zco)

    These functions attach and remove the ZCO's headers and trailers.  _header_
    and _trailer_ are assumed to be arrays of octets, not necessarily text.  
    Attaching a header or trailer causes it to be written to the SDR.  The
    prepend and append functions return 0 on success, -1 on any error.

- Object zco\_header\_text(Sdr sdr, Object zco, int skip, vast \*length)

    Skips over the first _skip_ headers of _zco_ and returns the address of
    the text of the next header, placing the length of the header's text in
    _\*length_.  Returns 0 on any error.

- Object zco\_trailer\_text(Sdr sdr, Object zco, int skip, vast \*length)

    Skips over the first _skip_ trailers of _zco_ and returns the address of
    the text of the next trailer, placing the length of the trailer's text in
    _\*length_.  Returns 0 on any error.

- void zco\_destroy(Sdr sdr, Object zco)

    Destroys the indicated Zco.  This reduces the reference counts for all
    files and SDR objects referenced in the ZCO's extents, resulting in the
    freeing of SDR objects and (optionally) the deletion of files as those
    reference count drop to zero.

- void zco\_bond(Sdr sdr, Object zco)

    Converts all headers and trailers of the indicated Zco to source data extents.
    Use this function to ensure that known header and trailer data are included
    when the ZCO is cloned.

- int zco\_revise(Sdr sdr, Object zco, vast offset, char \*buffer, vast length)

    Writes the contents of _buffer_, for length _length_, into _zco_ at offset
    _offset_.  Returns 0 on success, -1 on any error.

- Object zco\_clone(Sdr sdr, Object zco, vast offset, vast length)

    Creates a new ZCO whose source data is a copy of a subset of the source
    data of the referenced ZCO.  This procedure is required whenever it is
    necessary to process the ZCO's source data in multiple different ways, for
    different purposes, and therefore the ZCO must be in multiple states at the
    same time.  Portions of the source data extents of the original ZCO are
    copied as necessary, but no header or trailer capsules are copied.  Returns
    SDR location of the new ZCO on success, (Object) -1 on any error.

- vast zco\_clone\_source\_data(Sdr sdr, Object toZco, Object fromZco, vast offset, vast length)

    Appends to _toZco_ a copy of a subset of the source data of _fromZCO_.
    Portions of the source data extents of _fromZCO_ are copied as necessary.
    Returns total data length cloned, or -1 on any error.

- vast zco\_length(Sdr sdr, Object zco)

    Returns length of entire ZCO, including all headers and trailers and
    all source data extents.  This is the size of the object that would be
    formed by concatenating the text of all headers, trailers, and source
    data extents into a single serialized object.

- vast zco\_source\_data\_length(Sdr sdr, Object zco)

    Returns length of entire ZCO minus the lengths of all attached header and
    trailer capsules.  This is the size of the object that would be formed by
    concatenating the text of all source data extents (including those that
    are presumed to contain header or trailer text attached elsewhere) into
    a single serialized object.

- ZcoAcct zco\_acct(Sdr sdr, Object zco)

    Returns an indicator as to whether _zco_ is inbound or outbound.

- void zco\_start\_transmitting(Object zco, ZcoReader \*reader)

    Used by underlying protocol layer to start extraction of an outbound ZCO's
    bytes (both from header and trailer capsules and from source data extents) for
    "transmission" -- i.e., the copying of bytes into a memory buffer for
    delivery to some non-ZCO-aware protocol implementation.  Initializes
    reading at the first byte of the total concatenated ZCO object.  Populates
    _reader_, which is used to keep track of "transmission" progress via this
    ZCO reference.

    Note that this function can be called multiple times to restart reading at
    the start of the ZCO.  Note also that multiple ZcoReader objects may be used
    concurrently, by the same task or different tasks, to advance through the
    ZCO independently.

- void zco\_track\_file\_offset(ZcoReader \*reader)

    Turns on file offset tracking for this reader.

- vast zco\_transmit(Sdr sdr, ZcoReader \*reader, vast length, char \*buffer)

    Copies _length_ as-yet-uncopied bytes of the total concatenated ZCO
    (referenced by _reader_) into _buffer_.  If _buffer_ is NULL, skips
    over _length_ bytes without copying.  Returns the number of bytes copied
    (or skipped) on success, 0 on any file access error, -1 on any other error.

- void zco\_start\_receiving(Object zco, ZcoReader \*reader)

    Used by overlying protocol layer to start extraction of an inbound ZCO's
    bytes for "reception" -- i.e., the copying of bytes into a memory buffer
    for delivery to a protocol header parser, to a protocol trailer parser,
    or to the ultimate recipient (application).  Initializes reading of
    headers, source data, and trailers at the first byte of the concatenated
    ZCO objects.  Populates _reader_, which is used to keep track of "reception"
    progress via this ZCO reference and is required.

- vast zco\_receive\_headers(Sdr sdr, ZcoReader \*reader, vast length, char \*buffer)

    Copies _length_ as-yet-uncopied bytes of presumptive protocol header text
    from ZCO source data extents into _buffer_.  If _buffer_ is NULL, skips
    over _length_ bytes without copying.  Returns number of bytes copied (or
    skipped) on success, 0 on any file access error, -1 on any other error.

- void zco\_delimit\_source(Sdr sdr, Object zco, vast offset, vast length)

    Sets the computed offset and length of actual source data in the ZCO,
    thereby implicitly establishing the total length of the ZCO's concatenated
    protocol headers as _offset_ and the location of the ZCO's innermost
    protocol trailer as the sum of _offset_ and _length_.  Offset and length
    are typically determined from the information carried in received presumptive
    protocol header text.

- vast zco\_receive\_source(Sdr sdr, ZcoReader \*reader, vast length, char \*buffer)

    Copies _length_ as-yet-uncopied bytes of source data from ZCO extents into
    _buffer_.  If _buffer_ is NULL, skips over _length_ bytes without
    copying.  Returns number of bytes copied (or skipped) on success, 0 on any
    file access error, -1 on any other error.

- vast zco\_receive\_trailers(Sdr sdr, ZcoReader \*reader, vast length, char \*buffer)

    Copies _length_ as-yet-uncopied bytes of trailer data from ZCO extents into
    _buffer_.  If _buffer_ is NULL, skips over _length_ bytes without copying.
    Returns number of bytes copied (or skipped) on success, 0 on any file access
    error, -1 on any other error.

- void zco\_strip(Sdr sdr, Object zco)

    Deletes all source data extents that contain only header or trailer data and
    adjusts the offsets and/or lengths of all remaining extents to exclude any
    known header or trailer data.  This function is useful when handling a ZCO
    that was received from an underlying protocol layer rather than from an
    overlying application or protocol layer; use it before starting the
    transmission of the ZCO to another node or before enqueuing it for
    reception by an overlying application or protocol layer.

# SEE ALSO

sdr(3)
