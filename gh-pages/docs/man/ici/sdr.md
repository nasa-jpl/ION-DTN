# NAME

sdr - Simple Data Recorder library

# SYNOPSIS

    #include "sdr.h"

    [see below for available functions]

# DESCRIPTION

SDR is a library of functions that support the use of an abstract
data recording device called an "SDR" ("simple data recorder") for
persistent storage of data.  The SDR abstraction insulates
software not only from the specific characteristics of any single
data storage device but also from some kinds of persistent 
data storage and retrieval chores.  The underlying
principle is that an SDR provides standardized support for user
data organization at object granularity, with direct access to persistent 
user data objects, rather than supporting user data organization 
only at "file" granularity and requiring the user to
implement access to the data objects accreted within those files.

The SDR library is designed to provide some of the same kinds of
directory services as a file system together with support for
complex data structures that provide more operational flexibility
than files.  (As an example of this flexibility, consider how
much easier and faster it is to delete a given element from the middle 
of a linked list than it is to delete a range of bytes from
the middle of a text file.)  The intent is to enable the software
developer to take maximum advantage of the high speed and direct
byte addressability of a non-volatile flat address space
in the management of persistent data.  The SDR equivalent of a "record"
of data is simply a block of nominally persistent memory allocated from
this address space.  The SDR equivalent of a "file" is a _collection_
object.  Like files, collections can have names, can be located 
by name within persistent storage, and can impose structure
on the data items they encompass.  But, as discussed later, SDR
collection objects can impose structures other than the strict
FIFO accretion of records or bytes that characterizes a file.

The notional data recorder managed by the SDR library takes the
form of a single array of randomly accessible, contiguous,
nominally persistent memory locations called a _heap_.  Physically, the heap
may be implemented as a region of shared memory, as a single file of
predefined size, or both -- that is, the heap may be a region of shared
memory that is automatically mirrored in a file.

SDR services that manage SDR data are provided in several
layers, each of which relies on the services implemented at lower levels:

> At the highest level, a cataloguing service enables retrieval 
> of persistent objects by name.
>
> Services that manage three types of persistent data collections are 
> provided for use both by applications and by the cataloguing service:  
> linked lists, self-delimiting tables (which function as arrays that
> remember their own dimensions), and self-delimiting strings (short
> character arrays that remember their lengths, for speedier retrieval).
>
> Basic SDR heap space management services, analogous to malloc() and free(),
> enable the creation and destruction of objects of arbitrary type.
>
> Farther down the service stack are memcpy-like low-level 
> functions for reading from and writing to the heap.
>
> Protection of SDR data integrity across a series of reads and writes is 
> provided by a _transaction_ mechanism.

SDR persistent data are referenced in application code by Object
values and Address values, both of which are simply displacements
(offsets) within SDR address space.  The difference between the
two is that an Object is always the address of a block of heap
space returned by some call to sdr\_malloc(), while an Address can
refer to any byte in the address space.  That is, an Address is
the SDR functional equivalent of a C pointer in DRAM, and some
Addresses point to Objects.

Before using SDR services, the services must be loaded to the
target machine and initialized by invoking the sdr\_initialize()
function and the management profiles of one or more SDR's must be
loaded by invoking the sdr\_load\_profile() function.  These steps
are normally performed only once, at application load time.

An application gains access to an SDR by passing the name of the
SDR to the sdr\_start\_using() function, which returns an Sdr
pointer.  Most other SDR library functions take an Sdr pointer
as first argument.

All writing to an SDR heap must occur during a _transaction_ that
was initiated by the task issuing the write.  Transactions are
single-threaded; if task B wants to start
a transaction while a transaction begun by task A is still in progress,
it must wait until A's transaction is either ended or cancelled.  A
transaction is begun by calling sdr\_begin\_xn().  The current transaction
is normally ended by calling the sdr\_end\_xn() function, which returns an error
return code value in the event that any serious SDR-related processing error
was encountered in the course of the transaction.  Transactions may safely
be nested, provided that every level of transaction activity that is begun
is properly ended.

The current transaction may instead be cancelled by calling sdr\_cancel\_xn(),
which is normally used to indicate that some sort of serious SDR-related
processing error has been encountered.  Canceling a transaction reverses
all SDR update activity performed up to that point within the scope of the
transaction -- and, if the canceled transaction is an inner, nested
transaction, all SDR update activity performed within the scope of every
outer transaction encompassing that transaction _and_ every other transaction
nested within any of those outer transactions -- provided the SDR was
configured for transaction _reversibility_.  When an SDR is
configured for reversibility, all heap write operations
performed during a transaction are recorded in a log file that is
retained until the end of the transaction.  Each log file entry notes
the location at which the write operation was performed, the length
of data written, and the content of the overwritten heap bytes prior
to the write operation.  Canceling the transaction causes the log entries
to be read and processed in reverse order, restoring all overwritten data.
Ending the transaction, on the other hand, simply causes the log to be
discarded.

If a log file exists at the time that the profile for an SDR is loaded
(typically during application initialization), the transaction that was
being logged is automatically canceled and reversed.  This ensures that,
for example, a power failure that occurs in the middle of a
transaction will never wreck the SDR's data integrity: either all updates
issued during a given transaction are reflected in the current dataspace
content or none are.

As a further measure to protect SDR data integrity, an SDR may
additionally be configured for _object bounding_.  When an SDR is
configured to be "bounded", every heap write operation is restricted
to the extent of a single object allocated from heap space; that is,
it's impossible to overwrite part of one object by writing beyond
the end of another.  To enable the library to enforce this mechanism,
application code is prohibited from writing anywhere but within the
extent of an object that either (a) was allocated from managed heap
space during the same transaction (directly or indirectly via some
collection management function) or (b) was _staged_ -- identified
as an update target -- during the same transaction (again, either
directly or via some collection management function).

Note that both transaction reversibility and object bounding consume
processing cycles and inhibit performance to some degree.  Determining
the right balance between operational safety and processing speed is
left to the user.

Note also that, since SDR transactions are single-threaded, they can
additionally be used as a general mechanism for simply implementing "critical
sections" in software that is already using SDR for other purposes: the
beginning of a transaction marks the start of code that can't be executed
concurrently by multiple tasks.  To support this use of the SDR transaction
mechanism, the additional transaction termination function sdr\_exit\_xn() is
provided.  sdr\_exit\_xn() simply ends a transaction without either signaling
an error or checking for errors.  Like sdr\_cancel\_xn(), sdr\_exit\_xn()
has no return value; unlike sdr\_cancel\_xn(), it assures that ending an
inner, nested transaction does not cause the outer transaction to be
aborted and backed out.  But this capability must be used carefully: the
protection of SDR data integrity requires that transactions which are
ended by sdr\_exit\_xn() must not encompass any SDR update activity whatsoever.

The heap space management functions of the SDR library are adapted
directly from the Personal Space Management (_psm_)
function library.  The manual page for psm(3) explains
the algorithms used and the rationale behind them.  The principal
difference between PSM memory management and SDR heap management
is that, for performance reasons, SDR reserves the "small pool" for
its own use only; all user data space is allocated from the "large
pool", via the sdr\_malloc() function.

## RETURN VALUES AND ERROR HANDLING

Whenever an SDR function call fails, a diagnostic message explaining
the failure of the function is recorded in the error message pool
managed by the "platform" system (see the discussion
of putErrmsg() in platform(3)).

The failure of any function invoked in the course of an SDR
transaction causes all subsequent SDR activity in that
transaction to fail immediately.  This can streamline SDR application
code somewhat: it may not be necessary to check the return
value of every SDR function call executed during a transaction.
If the sdr\_end\_xn() call returns zero, all updates performed during
the transaction must have succeeded.

# SYSTEM ADMINISTRATION FUNCTIONS

- int sdr\_initialize(int wmSize, char \*wmPtr, int wmKey, char \*wmName)

    Initializes the SDR system.  sdr\_initialize() must be
    called once every time the computer on which the system
    runs is rebooted, before any call to any other SDR library function.

    This function attaches to a pool of shared memory, managed by PSM
    (see psm(3), that enables SDR library operations.  If the SDR system
    is to access a common pool of shared memory with one or more other
    systems, the key of that shared memory segment must be provided in
    _wmKey_ and the PSM partition name associated with that memory segment
    must be provided in _wmName_; otherwise _wmKey_ must be zero and
    _wmName_ must be NULL, causing sdr\_initialize() to assign default
    values.  If a shared memory segment identified by the effective
    value of _wmKey_ already exists, then _wmSize_ may be zero and the value of
    _wmPtr_ is ignored.  Otherwise the size of the shared memory pool must
    be provided in _wmSize_ and a new shared memory segment is created in
    a manner that is dependent on _wmPtr_: if _wmPtr_ is NULL then _wmSize_
    bytes of shared memory are dynamically acquired, allocated, and assigned
    to the newly created shared memory segment; otherwise the memory located
    at _wmPtr_ is assumed to have been pre-allocated and is merely assigned
    to the newly created shared memory segment.

    sdr\_initialize() also creates a semaphore to serialize access to the
    SDR system's private array of SDR profiles.  

    Returns 0 on success, -1 on any failure.

- void sdr\_wm\_usage(PsmUsageSummary \*summary)

    Loads _summary_ with a snapshot of the usage of the SDR system's private
    working memory.  To print the snapshot, use psm\_report().  (See psm(3).)

- void sdr\_shutdown( )

    Ends all access to all SDRs (see sdr\_stop\_using()), detaches from the
    SDR system's working memory (releasing the memory if it was dynamically
    allocated by sdr\_initialize()), and destroys the SDR system's private
    semaphore.  After sdr\_shutdown(), sdr\_initialize() must be called again
    before any call to any other SDR library function.

# DATABASE ADMINISTRATION FUNCTIONS

- int sdr\_load\_profile(char \*name, int configFlags, long heapWords, int heapKey, int logSize, int logKey, char \*pathName, char \*restartCmd, unsigned int restartLatency)

    Loads the profile for an SDR into the system's private list of SDR profiles.
    Although SDRs themselves are persistent, SDR profiles are not: in order
    for an application to access an SDR, sdr\_load\_profile() must have been called
    to load the profile of the SDR since the last invocation of sdr\_initialize(). 

    _name_ is the name of the SDR, required for any subsequent sdr\_start\_using()
    call.  

    _configFlags_ specifies the configuration of the
    SDR, the bitwise "or" of some combination of the following:

    - SDR\_IN\_DRAM

        SDR dataspace is implemented as a region of shared memory.

    - SDR\_IN\_FILE

        SDR dataspace is implemented as a file.

    - SDR\_REVERSIBLE

        SDR transactions are logged and are reversed if canceled.

    - SDR\_BOUNDED

        Heap updates are not allowed to cross object boundaries.

    _heapWords_ specifies the size of the heap in words; word size depends on
    machine architecture, i.e., a word is 4 bytes on a 32-bit machine, 8 bytes on
    a 64-bit machine.  Note that each SDR prepends to the heap a "map" of
    predefined, fixed size.  The total amount of space occupied by an SDR
    dataspace in memory and/or in a file is the sum of the size of the map
    plus the product of word size and _heapWords_.

    _heapKey_ is ignored if _configFlags_ does not include SDR\_IN\_DRAM.  It
    should normally be SM\_NO\_KEY, causing the shared memory region for the SDR
    dataspace to be allocated dynamically and shared using a dynamically selected
    shared memory key.  If specified, _heapKey_ must be a shared memory key
    identifying a pre-allocated region of shared memory whose length is equal
    to the total SDR dataspace size, shared via the indicated key.

    _logSize_ specifies the maximum size of the transaction log (in bytes) if
    and only if the log is to be written to memory rather than to a file; otherwise
    it must be zero.  _logKey_ is ignored if _logSize_ is zero.  It should
    normally be SM\_NO\_KEY, causing the shared memory region for the transaction
    log to be allocated dynamically and shared using a dynamically selected
    shared memory key.  If specified, _logKey_ must be a shared memory key
    identifying a pre-allocated region of shared memory whose length is equal
    to _logSize_, shared via the indicated key.

    _pathName_ is ignored if _configFlags_ includes neither SDR\_REVERSIBLE nor
    SDR\_IN\_FILE.  It is the fully qualified name of the directory into which the
    SDR's log file and/or dataspace file will be written.  The name of the log
    file (if any) will be "&lt;sdrname>.sdrlog".  The name of the dataspace file
    (if any) will be "&lt;sdrname>.sdr"; this file will be automatically created
    and filled with zeros if it does not exist at the time the SDR's profile
    is loaded.

    If a cleanup task must be run whenever a transaction is reversed, the command
    to execute this task must be provided in _restartCmd_ and the number of
    seconds to wait for this task to finish before resuming operations must be
    provided in _restartLatency_.  If _restartCmd_ is NULL or _restartLatency_
    is zero then no cleanup task will be run upon transaction reversal.

    Returns 0 on success, -1 on any error.

- int sdr\_reload\_profile(char \*name, int configFlags, long heapWords, int heapKey, int logSize, int logKey, char \*pathName, char \*restartCmd, unsigned int restartLatency)

    For use when the state of an SDR is thought to be inconsistent, perhaps
    due to crash of a program that had a transaction open.  Unloads the
    profile for the SDR, forcing the reversal of any transaction that is
    currently in progress when the SDR's profile is re-loaded.  Then
    calls sdr\_load\_profile() to re-load the profile for the SDR.  Same
    return values as sdr\_load\_profile.

- Sdr sdr\_start\_using(char \*name)

    Locates SDR profile by _name_ and returns a handle that can be used
    for all functions that operate on that SDR.  On any failure, returns NULL.

- char \*sdr\_name(Sdr sdr)

    Returns the name of the sdr.

- long sdr\_heap\_size(Sdr sdr)

    Returns the total size of the SDR heap, in bytes.

- void sdr\_stop\_using(Sdr sdr)

    Terminates access to the SDR via this handle.  Other users of the SDR are
    not affected.  Frees the Sdr object.

- void sdr\_abort(Sdr sdr)

    Terminates the task.  In flight configuration, also terminates all use
    of the SDR system by all tasks.

- void sdr\_destroy(Sdr sdr)

    Ends all access to this SDR, unloads the SDR's profile, and erases the SDR
    from memory and file system.

# DATABASE TRANSACTION FUNCTIONS

- int sdr\_begin\_xn(Sdr sdr)

    Initiates a transaction.  Returns 1 on success, 0 on any failure.  Note
    that transactions are single-threaded; any task that calls sdr\_begin\_xn()
    is suspended until all previously requested transactions have been ended
    or canceled.

- int sdr\_in\_xn(Sdr sdr)

    Returns 1 if called in the course of a transaction, 0 otherwise.

- void sdr\_exit\_xn(Sdr sdr)

    Simply abandons the current transaction, ceasing the calling task's lock on
    ION.  Must **not** be used if any dataspace modifications were performed
    during the transaction; sdr\_end\_xn() must be called instead, to commit
    those modifications.

- void sdr\_cancel\_xn(Sdr sdr)

    Cancels the current transaction.  If reversibility is enabled for
    the SDR, canceling a transaction reverses all heap modifications
    performed during that transaction.

- int sdr\_end\_xn(Sdr sdr)

    Ends the current transaction.  Returns 0 if the transaction completed
    without any error; returns -1 if any operation performed in the course
    of the transaction failed, in which case the transaction was automatically
    canceled.

# DATABASE I/O FUNCTIONS

- void sdr\_read(Sdr sdr, char \*into, Address from, int length)

    Copies _length_ characters at _from_ (a location in the
    indicated SDR) to the memory location given by _into_.  The data are
    copied from the shared memory region in which the SDR resides, if any;
    otherwise they are read from the file in which the SDR resides.

- void sdr\_peek(sdr, variable, from)

    sdr\_peek() is a macro that uses sdr\_read() to load _variable_ from
    the indicated address in the SDR dataspace; the size of _variable_ is
    used as the number of bytes to copy.

- void sdr\_write(Sdr sdr, Address into, char \*from, int length)

    Copies _length_ characters at _from_ (a location in memory) to the SDR
    heap location given by _into_.  Can only be performed during a transaction, 
    and if the SDR is configured for object bounding then heap
    locations _into_ through (_into_ + (_length_ - 1)) must be within
    the extent of some object that was either allocated or staged within the
    same transaction.  The data are copied both to the shared memory region
    in which the SDR resides, if any, and also to the file in which the SDR
    resides, if any.

- void sdr\_poke(sdr, into, variable)

    sdr\_poke() is a macro that uses sdr\_write() to store _variable_ at
    the indicated address in the SDR dataspace; the size of _variable_ is
    used as the number of bytes to copy.

- char \*sdr\_pointer(Sdr sdr, Address address)

    Returns a pointer to the indicated location in the heap - a "heap pointer" - or
    NULL if the indicated address is invalid.  NOTE that this
    function _cannot be used_ if the SDR does not reside in a shared memory region.

    Providing an alternative to using sdr\_read() to retrieve objects
    into local memory, sdr\_pointer() can help make SDR-based
    applications run very quickly, but it must be used WITH GREAT
    CAUTION!  Never use a direct pointer into the heap when not
    within a transaction, because you will have no assurance at
    any time that the object pointed to by that pointer has not changed
    (or is even still there).  And NEVER de-reference a heap 
    pointer in order to write directly into the heap: this makes
    transaction reversal impossible.  Whenever writing to the SDR, always use
    sdr\_write().

- Address sdr\_address(Sdr sdr, char \*pointer)

    Returns the address within the SDR heap of the indicated location,
    which must be (or be derived from) a heap pointer as returned
    by sdr\_pointer().  Returns zero if the indicated location is not
    greater than the start of the heap mirror.  NOTE that this
    function _cannot be used_ if the SDR does not reside in a shared memory region.

- void sdr\_get(sdr, variable, heap\_pointer)

    sdr\_get() is a macro that uses sdr\_read() to load _variable_ from
    the SDR address given by _heap\_pointer_; _heap\_pointer_ must be (or
    be derived from) a heap pointer as returned by sdr\_pointer().  The size
    of _variable_ is used as the number of bytes to copy.

- void sdr\_set(sdr, heap\_pointer, variable)

    sdr\_set() is a macro that uses sdr\_write() to store _variable_ at
    the SDR address given by _heap\_pointer_; _heap\_pointer_ must be (or
    be derived from) a heap pointer as returned by sdr\_pointer().  The size
    of _variable_ is used as the number of bytes to copy.

# HEAP SPACE MANAGEMENT FUNCTIONS

- Object sdr\_malloc(Sdr sdr, unsigned long size)

    Allocates a block of space from the of the indicated SDR's
    heap.  _size_ is the size of the
    block to allocate; the maximum size is 1/2 of the maximum
    address space size (i.e., 2G for a 32-bit machine).  Returns block address if
    successful, zero if block could not be allocated.

- Object sdr\_insert(Sdr sdr, char \*from, unsigned long size)

    Uses sdr\_malloc() to obtain a block of space of size _size_ and, if this
    allocation is successful, uses sdr\_write() to copy _size_ bytes of data
    from memory at _from_ into the newly allocated block.  Returns block address
    if successful, zero if block could not be allocated.

- Object sdr\_stow(sdr, variable)

    sdr\_stow() is a macro that uses sdr\_insert() to insert a copy of _variable_
    into the dataspace.  The size of _variable_ is used as the number of bytes
    to copy.

- int sdr\_object\_length(Sdr sdr, Object object)

    Returns the number of bytes of heap space allocated to the application
    data at _object_.

- void sdr\_free(Sdr sdr, Object object)

    Frees for subsequent re-allocation the heap space occupied by _object_.

- void sdr\_stage(Sdr sdr, char \*into, Object from, int length)

    Like sdr\_read(), this function will copy _length_ characters
    at _from_ (a location in the heap of the indicated SDR)
    to the memory location given by _into_.  Unlike
    sdr\_get(), sdr\_stage() requires that _from_ be the address of
    some allocated object, not just any location within the
    heap.  sdr\_stage(), when called from within a transaction, 
    notifies the SDR library that the indicated object may be 
    updated later in the transaction; this enables the library 
    to retrieve the object's size for
    later reference in validating attempts to write into
    some location within the object.  If _length_ is zero, the
    object's size is privately retrieved by SDR but none of the
    object's content is copied into memory.

- long sdr\_unused(Sdr sdr)

    Returns number of bytes of heap space not yet allocated to either the
    large or small objects pool.

- void sdr\_usage(Sdr sdr, SdrUsageSummary \*summary)

    Loads the indicated SdrUsageSummary structure with a snapshot of the SDR's
    usage status.  SdrUsageSummary is defined by:

        typedef struct
        {
                char            sdrName[MAX_SDR_NAME + 1];
                unsigned int    dsSize;
                unsigned int    smallPoolSize;
                unsigned int    smallPoolFreeBlockCount[SMALL_SIZES];
                unsigned int    smallPoolFree;
                unsigned int    smallPoolAllocated;
                unsigned int    largePoolSize;
                unsigned int    largePoolFreeBlockCount[LARGE_ORDERS];
                unsigned int    largePoolFree;
                unsigned int    largePoolAllocated;
                unsigned int    unusedSize;
        } SdrUsageSummary;

- void sdr\_report(SdrUsageSummary \*summary)

    Sends to stdout a printed summary of the SDR's usage status.

- int sdr\_heap\_depleted(Sdr sdr)

    A Boolean function: returns 1 if the total available space in the SDR's
    heap (small pool free, large pool free, and unused) is less than 1/16
    of the total size of the heap.  Otherwise returns zero.

# HEAP SPACE USAGE TRACING

If SDR\_TRACE is defined at the time the SDR source code is compiled, the
system includes built-in support for simple tracing of SDR heap space usage:
heap space allocations are logged, and heap space deallocations are matched
to logged allocations, "closing" them.  This enables heap space leaks and
some other kinds of SDR heap access problems to be readily investigated.

- int sdr\_start\_trace(Sdr sdr, int traceLogSize, char \*traceLogAddress)

    Begins an episode of SDR heap space usage tracing.  _traceLogSize_ is the
    number of bytes of shared memory to use for trace activity logging; the
    frequency with which "closed" trace log events must be deleted will vary
    inversely with the amount of memory allocated for the trace log.
    _traceLogAddress_ is normally NULL, causing the trace system to allocate
    _traceLogSize_ bytes of shared memory dynamically for trace logging; if
    non-NULL, it must point to _traceLogSize_ bytes of shared memory that
    have been pre-allocated by the application for this purpose.  Returns 0 on
    success, -1 on any failure.

- void sdr\_print\_trace(Sdr sdr, int verbose)

    Prints a cumulative trace report and current usage report for 
    _sdr_.  If _verbose_ is zero, only exceptions (notably, trace
    log events that remain open -- potential SDR heap space leaks) are printed;
    otherwise all activity in the trace log is printed.

- void sdr\_clear\_trace(Sdr sdr)

    Deletes all closed trace log events from the log, freeing up memory for
    additional tracing.

- void sdr\_stop\_trace(Sdr sdr)

    Ends the current episode of SDR heap space usage tracing.  If the shared
    memory used for the trace log was allocated by sdr\_start\_trace(), releases
    that shared memory.

# CATALOGUE FUNCTIONS

The SDR catalogue functions are used to maintain the catalogue of the
names, types, and addresses of objects within an SDR.  The catalogue
service includes functions for creating, deleting and finding catalogue
entries and a function for navigating through catalogue entries sequentially.

- void sdr\_catlg(Sdr sdr, char \*name, int type, Object object)

    Associates _object_ with _name_ in the indicated SDR's catalogue and notes 
    the _type_ that was declared for this object.  _type_ is optional and 
    has no significance other than that conferred on it by the application.

    The SDR catalogue is flat, not hierarchical like a directory tree, 
    and all names must be unique.  The length of _name_ is limited to
    15 characters.

- Object sdr\_find(Sdr sdr, char \*name, int \*type)

    Locates the Object associated with _name_ in the indicated SDR's catalogue 
    and returns its address; also reports the catalogued type of the object in 
    _\*type_ if _type_ is non-NULL.  Returns zero if no object is currently
    catalogued under this name.

- void sdr\_uncatlg(Sdr sdr, char \*name)

    Dissociates from _name_ whatever object in the indicated
    SDR's catalogue is currently catalogued under that name.

- Object sdr\_read\_catlg(Sdr sdr, char \*name, int \*type, Object \*object, Object previous\_entry)

    Used to navigate through catalogue entries sequentially.  If
    _previous\_entry_ is zero, reads the first entry in the
    indicated SDR's catalogue; otherwise, reads the next catalogue
    entry following the one located at _previous\_entry_.  In either case,
    returns zero if no such catalogue entry exists; otherwise, copies that
    entry's name, type, and catalogued object address into _name_,
    _\*type_, and _\*object_, and then returns the address of the catalogue
    entry (which may be used as _previous\_entry_ in a subsequent call
    to sdr\_read\_catlg()).

# USER'S GUIDE

- Compiling an SDR application

    Just be sure to "#include "sdr.h"" at the top of each source
    file that includes any SDR function calls.

    For UNIX applications, link with "-lsdr".

- Loading an SDR application (VxWorks)

        ld < "libsdr.o"

    After the library has been loaded, you can begin loading SDR applications.

# SEE ALSO

sdrlist(3), sdrstring(3), sdrtable(3)
