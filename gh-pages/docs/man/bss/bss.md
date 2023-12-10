# NAME

bss - Bundle Streaming Service library

# SYNOPSIS

    #include "bss.h"

    typedef int (*RTBHandler)(time_t time, unsigned long count, char *buffer, int bufLength);

    [see description for available functions]

# DESCRIPTION

The BSS library supports the streaming of data over delay-tolerant
networking (DTN) bundles.  The intent of the library is to enable applications
that pass streaming data received in transmission time order (i.e., without
time regressions) to an application-specific "display" function -- notionally
for immediate real-time display -- but to store **all** received data (including
out-of-order data) in a private database for playback under user control.  The
reception and real-time display of in-order data is performed by a background
thread, leaving the application's main (foreground) thread free to respond to
user commands controlling playback or other application-specific functions.

The application-specific "display" function invoked by the background thread
must conform to the RTBHandler type definition.  It must return 0 on success,
\-1 on any error that should terminate the background thread.  Only on return
from this function will the background thread proceed to acquire the next BSS
payload.

All data acquired by the BSS background thread is written to a BSS database
comprising three files: table, list, and data.  The name of the database
is the root name that is common to the three files, e.g., _db3_.tbl,
_db3_.lst, _db3_.dat would be the three files making up the _db3_ BSS
database.  All three files of the selected BSS database must reside in the
same directory of the file system.

Several replay navigation functions in the BSS library require that the
application provide a navigation state structure of type bssNav as defined
in the bss.h header file.  The application is not reponsible for populating
this structure; it's strictly for the private use of the BSS library.

- int bssOpen(char \*bssName, char \*path, char \*eid)

    Opens access to a BSS database, to enable data playback.  _bssName_
    identifies the specific BSS database that is to be opened.  _path_ identifies
    the directory in which the database resides.  _eid_ is ignored.  On any
    failure, returns -1.  On success, returns zero.

- int bssStart(char \*bssName, char \*path, char \*eid, char \*buffer, int bufLen, RTBHandler handler)

    Starts a BSS data acquisition background thread.  _bssName_ identifies the
    BSS database into which data will be acquired.  _path_ identifies the
    directory in which that database resides.  _eid_ is used to open the BP
    endpoint at which the delivered BSS bundle payload contents will be
    acquired.  _buffer_ identifies a data acquisition buffer, which must be
    provided by the application, and _bufLen_ indicates the length of that
    buffer; received bundle payloads in excess of this length will be discarded.

    _handler_ identifies the display function to which each in-order bundle
    payload will be passed.  The _time_ and _count_ parameters passed to this
    function identify the received bundle, indicating the bundle's creation
    timestamp time (in seconds) and counter value.  The _buffer_ and _bufLength_
    parameters indicate the location into which the bundle's payload was
    acquired and the length of the acquired payload.  _handler_ must return -1 on
    any unrecoverable system error, 0 otherwise.  A return value of -1 from
    _handler_ will terminate the BSS data acquisition background thread.

    On any failure, returns -1.  On success, returns zero.

- int bssRun(char \*bssName, char \*path, char \*eid, char \*buffer, int bufLen, RTBHandler handler)

    A convenience function that performs both bssOpen() and bssStart().  On any
    failure, returns -1.  On success, returns zero.

- void bssClose()

    Terminates data playback access to the most recently opened BSS database.

- void bssStop()

    Terminates the most recently initiated BSS data acquisition background thread.

- void bssExit()

    A convenience function that performs both bssClose() and bssStop().

- long bssRead(bssNav nav, char \*data, int dataLen)

    Copies the data at the current playback position in the database, as indicated
    by _nav_, into _data_; if the length of the data is in excess of _dataLen_
    then an error condition is asserted (i.e., -1 is returned).  Note that bssRead()
    cannot be successfully called until _nav_ has been populated, nominally by
    a preceding call to bssSeek(), bssNext(), or bssPrev().  Returns the length
    of data read, or -1 on any error.

- long bssSeek(bssNav \*nav, time\_t time, time\_t \*curTime, unsigned long \*count)

    Sets the current playback position in the database, in _nav_, to the data
    received in the bundle with the earliest creation time that was greater than
    or equal to _time_.  Populates _nav_ and also returns the creation time and
    bundle ID count of that bundle in _curTime_ and _count_.  Returns the length
    of data at this location, or -1 on any error.

- long bssSeek\_read(bssNav \*nav, time\_t time, time\_t \*curTime, unsigned long \*count, char \*data, int dataLen)

    A convenience function that performs bssSeek() followed by an immediate
    bssRead() to return the data at the new playback position.  Returns the length
    of data read, or -1 on any error.

- long bssNext(bssNav \*nav, time\_t \*curTime, unsigned long \*count)

    Sets the playback position in the database, in _nav_, to the data received
    in the bundle with the earliest creation time and ID count greater than that
    of the bundle at the current playback position.  Populates _nav_ and also
    returns the creation time and bundle ID count of that bundle in _curTime_
    and _count_.  Returns the length of data at this location (if any),
    \-2 on reaching end of list, or -1 on any error.

- long bssNext\_read(bssNav \*nav, time\_t \*curTime, unsigned long \*count, char \*data, int dataLen)

    A convenience function that performs bssNext() followed by an immediate
    bssRead() to return the data at the new playback position.  Returns the
    length of data read, -2 on reaching end of list, or -1 on any error.

- long bssPrev(bssNav \*nav, time\_t \*curTime, unsigned long \*count)

    Sets the playback position in the database, in _nav_, to the data received
    in the bundle with the latest creation time and ID count earlier than that
    of the bundle at the current playback position.  Populates _nav_ and also
    returns the creation time and bundle ID count of that bundle in _curTime_
    and _count_.  Returns the length of data at this location (if any), -2 on
    reaching end of list, or -1 on any error.

- long bssPrev\_read(bssNav \*nav, time\_t \*curTime, unsigned long \*count, char \*data, int dataLen)

    A convenience function that performs bssPrev() followed by an immediate
    bssRead() to return the data at the new playback position.  Returns the
    length of data read, -2 on reaching end of list, or -1 on any error.

# SEE ALSO

bp(3)
