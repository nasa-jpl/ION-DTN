# NAME

psm - Personal Space Management

# SYNOPSIS

    #include "psm.h"

    typedef enum { Okay, Redundant, Refused } PsmMgtOutcome;
    typedef unsigned long PsmAddress;
    typedef struct psm_str
    {
            char            *space;
            int             freeNeeded;
            struct psm_str  *trace;
            int             traceArea[3];
    } PsmView, *PsmPartition;

    [see description for available functions]

# DESCRIPTION

PSM is a library of functions that support personal space management,  
that is, user management of an application-configured
memory partition.  PSM is designed to be faster and more efficient  
than malloc/free (for details, see the DETAILED DESCRIPTION below), but
more importantly it provides a memory management
abstraction that insulates applications from differences in the
management of private versus shared memory.  

PSM is often used to manage shared memory partitions.  On most
operating systems, separate tasks that connect to a common
shared memory partition are given the same base address with
which to access the partition.  On some systems (such as Solaris)
this is not necessarily the case; an absolute address within
such a shared partition will be mapped to different pointer values in
different tasks.  If a pointer value is stored within shared
memory and used without conversion by multiple tasks, segment violations
will occur.

PSM gets around this problem by providing functions for translating  
between local pointer values and relative addresses within the shared memory
partition.  For complete portability, applications which store
addresses in shared memory should store these addresses as PSM
relative addresses and convert them to local pointer values before
using them.  The PsmAddress data type is provided for this purpose, 
along with the conversion functions psa() and psp().

- int  psm\_manage(char \*start, unsigned int length, char \*name, PsmPartition \*partitionPointer, PsmMgtOutcome \*outcome)

    Puts the _length_ bytes of memory at _start_ under PSM management,
    associating this memory partition with the identifying string _name_
    (which is required and which can have a maximum string length of
    31).  PSM can manage any contiguous range of addresses to which the
    application has access, typically a block of heap memory
    returned by a malloc call.

    Every other PSM API function must be passed a pointer to a local "partition"
    state structure characterizing the PSM-managed memory to which the function
    is to be applied.  The partition state structure itself may be pre-allocated
    in static or local (or shared) memory by the application, in which
    case a pointer to that structure must be passed to psm\_manage() as
    the value of _\*partitionPointer_; if _\*partitionPointer_ is null,
    psm\_manage() will use malloc() to allocate this structure dynamically
    from local memory and will store a pointer to the structure in
    _\*partitionPointer_.

    psm\_manage() formats the managed memory as necessary and returns -1 on any
    error, 0 otherwise.  The outcome to the attempt to manage memory is placed
    in _outcome_.  An outcome of Redundant means that the memory at _start_
    is already under PSM management with the same name and size.  An outcome
    of Refused means that PSM was unable to put the memory at _start_ under
    PSM management as directed; a diagnostic message was posted to the message
    pool (see discussion of putErrmsg() in platform(3)).

- char \*psm\_name(PsmPartition partition)

    Returns the name associated with the partition at the time it was put
    under management.

- char \*psm\_space(PsmPartition partition)

    Returns the address of the space managed by PSM for _partition_.
    This function is provided to enable the application to do an
    operating-system release (such as free()) of this memory when
    the managed partition is no longer needed.  _NOTE_ that calling
    psm\_erase() or psm\_unmanage() \[or any other PSM function, for that
    matter\] after releasing that space is virtually guaranteed to result
    in a segmentation fault or other seriously bad behavior.

- void \*psp(PsmPartition partition, PsmAddress address)

    _address_ is an offset within the space managed for the partition.  Returns
    the conversion of that offset into a locally usable pointer.

- PsmAddress psa(PsmPartition partition, void \*pointer)

    Returns the conversion of _pointer_ into an offset within the space
    managed for the partition.

- PsmAddress psm\_malloc(PsmPartition partition,  unsigned int length)

    Allocates a block of memory from the "large pool" of
    the indicated partition.  (See the DETAILED DESCRIPTION below.)  _length_
    is the size of the block to allocate; the maximum size is 1/2 of the total
    address space (i.e., 2G for a 32-bit machine).  Returns
    NULL if no free block could be found.  The block returned 
    is aligned on a doubleword boundary.

- void psm\_panic(PsmPartition partition)

    Forces the "large pool" memory allocation algorithm to
    hunt laboriously for free blocks in buckets that may
    not contain any.  This setting remains in force for the
    indicated  partition until a subsequent psm\_relax() call reverses it.

- void psm\_relax(PsmPartition partition)

    Reverses psm\_panic().  Lets the "large pool" memory allocation  
    algorithm return NULL when no free block can be found easily.

- PsmAddress psm\_zalloc(PsmPartition partition,  unsigned int length)

    Allocates a block of memory from the "small pool" of
    the indicated partition, if possible; if the requested
    block size -- _length_ -- is too large for small pool
    allocation (which is limited to 64 words, i.e., 256 bytes for a
    32-bit machine), or if no small pool space is available
    and the size of the small pool cannot be increased,
    then allocates from the large pool instead.  Small pool
    allocation is performed by an especially speedy algorithm, and 
    minimum space is consumed in memory management  
    overhead for small-pool blocks.  Returns NULL if no free block could be
    found.  The block returned is aligned on a word boundary.

- void psm\_free(PsmPartition partition, PsmAddress block)

    Frees for subsequent re-allocation the indicated block
    of memory from the indicated partition.  _block_ may
    have been allocated by either psm\_malloc() or psm\_zalloc().

- int psm\_set\_root(PsmPartition partition, PsmAddress root)

    Sets the "root" word of the indicated partition (a word
    at a fixed, private location in the PSM bookkeeping
    data area) to the indicated value.  This function is
    typically useful in a shared-memory environment, such
    as a VxWorks address space, in which a task wants to
    retrieve from the indicated partition some data that was inserted 
    into the partition by some other task; the partition root 
    word enables multiple tasks to navigate the
    same data in the same PSM partition in shared memory.
    The argument is normally a pointer to something like a
    linked list of the linked lists that populate the partition;
    in particular, it is likely to be an object catalog
    (see psm\_add\_catlg()).  Returns 0 on success, -1 on any
    failure (e.g., the partition already has a root object, in which
    case psm\_erase\_root() must be called before psm\_set\_root()).

- PsmAddress psm\_get\_root(PsmPartition partition)

    Retrieves the current value of the root word of the indicated partition.

- void psm\_erase\_root(PsmPartition partition)

    Erases the current value of the root word of the indicated partition.

- PsmAddress psm\_add\_catlg(PsmPartition partition)

    Allocates space for an object catalog in the indicated partition
    and establishes the new catalog as the partition's root object.  Returns
    0 on success, -1 on any error (e.g., the partition already has
    some other root object).

- int psm\_catlg(PsmPartition partition, char \*objName, PsmAddress objLocation)

    Inserts an entry for the indicated object into the catalog that is the root
    object for this partition.  The length of _objName_ cannot exceed 32 bytes,
    and _objName_ must be unique in the catalog.  Returns 0 on success, -1 on
    any error.

- int psm\_uncatlg(PsmPartition partition, char \*objName)

    Removes the entry for the named object from the catalog that is the root
    object for this partition, if that object is found in the catalog.  Returns
    0 on success, -1 on any error.

- int psm\_locate(PsmPartition partition, char \*objName, PsmAddress \*objLocation, PsmAddress \*entryElt)

    Places in _\*objLocation_ the address associated with _objName_ in the
    catalog that is the root object for this partition and places in _\*entryElt_
    the address of the list element that points to this catalog entry.  If _name_
    is not found in catalog, set _\*entryElt_ to zero.  Returns 0 on success, -1
    on any error.

- void psm\_usage(PsmPartition partition, PsmUsageSummary \*summary)

    Loads the indicated PsmUsageSummary structure with a
    snapshot of the indicated partition's usage status.
    PsmUsageSummary is defined by:

        typedef struct {
            char            partitionName[32];
            unsigned int    partitionSize;
            unsigned int    smallPoolSize;
            unsigned int    smallPoolFreeBlockCount[SMALL_SIZES];
            unsigned int    smallPoolFree;
            unsigned int    smallPoolAllocated;
            unsigned int    largePoolSize;
            unsigned int    largePoolFreeBlockCount[LARGE_ORDERS];
            unsigned int    largePoolFree;
            unsigned int    largePoolAllocated;
            unsigned int    unusedSize;
        } PsmUsageSummary;

- void psm\_report(PsmUsageSummary \*summary)

    Sends to stdout the content of _summary_,
    a snapshot of a partition's usage status.

- void psm\_unmanage(PsmPartition partition)

    Terminates local PSM management of the memory in _partition_ and
    destroys the partition state structure _\*partition_,
    but doesn't erase anything in the managed memory; PSM
    management can be re-established by a subsequent call to psm\_manage().

- void psm\_erase(PsmPartition partition)

    Unmanages the indicated partition and additionally discards all information
    in the managed memory, preventing re-management of the partition.

# MEMORY USAGE TRACING

If PSM\_TRACE is defined at the time the PSM source code is compiled, the
system includes built-in support for simple tracing of memory usage: memory
allocations are logged, and memory deallocations are matched to logged
allocations, "closing" them.  This enables memory leaks and some other
kinds of memory access problems to be readily investigated.

- int psm\_start\_trace(PsmPartition partition, int traceLogSize, char \*traceLogAddress)

    Begins an episode of PSM memory usage tracing.  _traceLogSize_ is the
    number of bytes of shared memory to use for trace activity logging; the
    frequency with which "closed" trace log events must be deleted will vary
    inversely with the amount of memory allocated for the trace log.
    _traceLogAddress_ is normally NULL, causing the trace system to allocate
    _traceLogSize_ bytes of shared memory dynamically for trace logging; if
    non-NULL, it must point to _traceLogSize_ bytes of shared memory that
    have been pre-allocated by the application for this purpose.  Returns 0 on
    success, -1 on any failure.

- void psm\_print\_trace(PsmPartition partition, int verbose)

    Prints a cumulative trace report and current usage report for 
    _partition_.  If _verbose_ is zero, only exceptions (notably, trace
    log events that remain open -- potential memory leaks) are printed;
    otherwise all activity in the trace log is printed.

- void psm\_clear\_trace(PsmPartition partition)

    Deletes all closed trace log events from the log, freeing up memory for
    additional tracing.

- void psm\_stop\_trace(PsmPartition partition)

    Ends the current episode of PSM memory usage tracing.  If the shared
    memory used for the trace log was allocated by psm\_start\_trace(), releases
    that shared memory.

# EXAMPLE

For an example of the use of psm, see the file psmshell.c in
the PSM source directory.

# USER'S GUIDE

- Compiling a PSM application

    Just be sure to "#include "psm.h"" at the top of each source file 
    that includes any PSM function calls.

- Linking/loading a PSM application

    a. In a UNIX environment, link with libpsm.a.

    b. In a VxWorks environment, use

          ld 1, 0, "libpsm.o"

    to load PSM on the target before loading any PSM applications.

- Typical usage:

    a. Call psm\_manage() to initiate management of the partition.

    b. Call psm\_malloc() (and/or psm\_zalloc()) to allocate space in the 
    partition; call psm\_free() to release space for later re-allocation.

    c. When psm\_malloc() returns NULL and you're willing to wait
    a while for a more exhaustive free block search, call
    psm\_panic() before retrying psm\_malloc().  When you're no
    longer so desperate for space, call psm\_relax().

    d. To store a vital pointer in the single predefined location  
    in the partition that PSM reserves for this purpose, call 
    psm\_set\_root(); to retrieve that pointer, call psm\_get\_root().

    e. To get a snapshot of the current configuration of the partition,  
    call psm\_usage().  To print this snapshot to stdout, call psm\_report().

    f. When you're done with the partition but want to leave
    it in its current state for future re-management (e.g.,
    if the partition is in shared memory), call psm\_unmanage().
    If you're done with the partition forever, call psm\_erase().

# DETAILED DESCRIPTION

PSM supports user management of an application-configured memory
partition. The partition is functionally divided into two pools
of variable size: a "small pool" of low-overhead blocks aligned
on 4-byte boundaries that can each contain up to 256 bytes of
user data, and a "large pool" of high-overhead blocks aligned on
8-byte boundaries that can each contain up to 2GB of user data.

Space in the small pool is allocated in any one of 64 different
block sizes; each possible block size is (4i + n) where i is a
"block list index" from 1 through 64 and n is the length of the
PSM overhead information per block \[4 bytes on a 32-bit machine\].
Given a user request for a block of size q where q is in the
range 1 through 256 inclusive, we return the first block on the
j'th small-pool free list where j = (q - 1) / 4.  If there is no
such block, we increase the size of the small pool \[incrementing
its upper limit by (4 \* (j + 1)) + n\], initialize the increase as
a free block from list j, and return that block.  No attempt is
made to consolidate physically adjacent blocks when they are
freed or to bisect large blocks to satisfy requests for small
ones; if there is no free block of the requested size and the
size of the small pool cannot be increased without encroaching on
the large pool (or if the requested size exceeds 256), we attempt
to allocate a large-pool block as described below.  The differences 
between small-pool and large-pool blocks are transparent to
the user, and small-pool and large-pool blocks can be freely intermixed 
in an application.

Small-pool blocks are allocated and freed very rapidly, and space
overhead consumption is small, but capacity per block is limited
and space assigned to small-pool blocks of a given size is never
again available for any other purpose.  The small pool is
designed to satisfy requests for allocation of a stable overall
population of small, volatile objects such as List and ListElt
structures (see lyst(3)).

Space in the large pool is allocated from any one of 29 buckets,
one for each power of 2 in the range 8 through 2G.  The size of
each block can be expressed as (n + 8i + m) where i is any integer 
in the range 1 through 256M, n is the size of the block's
leading overhead area \[8 bytes on a 32-bit machine\], and m is the
size of the block's trailing overhead area \[also 8 bytes on a
32-bit machine\].  Given a user request for a block of size q
where q is in the range 1 through 2G inclusive, we first compute
r as the smallest multiple of 8 that is greater than or equal to
q.  We then allocate the first block in bucket t such that 2 \*\*
(t + 3) is the smallest power of 2 that is greater than r \[or, if
r is a power of 2, the first block in bucket t such that 2 \*\* (t
\+ 3) = r\].  That is, we try to allocate blocks of size 8 from
bucket 0 \[2\*\*3 = 8\], blocks of size 16 from bucket 1 \[2\*\*4 = 16\],
blocks of size 24 from bucket 2 \[2\*\*5 = 32, 32 > 24\], blocks of
size 32 from bucket 2 \[2\*\*5 = 32\], and so on.  t is the first
bucket whose free blocks are ALL guaranteed to be at least as
large as r; bucket t - 1 may also contain some blocks that are as
large as r (e.g., bucket 1 will contain blocks of size 24 as well
as blocks of size 16), but we would have to do a possibly time
consuming sequential search through the free blocks in that bucket 
to find a match, because free blocks within a bucket are
stored in no particular order.

If bucket t is empty, we allocate the first block from the first
non-empty bucket corresponding to a greater power of two; if all
eligible bucket are empty, we increase the size of the large pool
\[decrementing its lower limit by (r + 16)\], initialize the increase 
as a free block and "free" it, and try again.  If the size
of the large pool cannot be increased without encroaching on the
small pool, then if we are desperate we search sequentially
through all blocks in bucket t - 1 (some of which may be of size
r or greater) and allocate the first block that is big enough, if
any.  Otherwise, no block is returned.

Having selected a free block to allocate, we remove the allocated
block from the free list, split off as a new free block all bytes
in excess of (r + 16) bytes \[unless that excess is too small to
form a legal-size block\], and return the remainder to the user.
When a block is freed, it is automatically consolidated with the
physically preceding block (if that block is free) and the physically 
subsequent block (if that block is free).

Large-pool blocks are allocated and freed quite rapidly; capacity
is effectively unlimited; space overhead consumption is very high
for extremely small objects but becomes an insignificant fraction
of block size as block size increases.  The large pool is
designed to serve as a general-purpose heap with minimal fragmentation 
whose overhead is best justified when used to store relatively 
large, long-lived objects such as image packets.

The general goal of this memory allocation scheme is to satisfy
memory management requests rapidly and yet minimize the chance of
refusing a memory allocation request when adequate unused space
exists but is inaccessible (because it is fragmentary or is
buried as unused space in a block that is larger than necessary).
The size of a small-pool block delivered to satisfy a request for
q bytes will never exceed q + 3 (alignment), plus 4 bytes of
overhead.  The size of a large-pool block delivered to satisfy a
request for q bytes will never exceed q + 7 (alignment) + 20 (the
maximum excess that can't be split off as a separate free block),
plus 16 bytes of overhead.

Neither the small pool nor the large pool ever decrease in size,
but large-pool space previously allocated and freed is available
for small-pool allocation requests if no small-pool space is
available.  Small-pool space previously allocated and freed cannot 
easily be reassigned to the large pool, though, because
blocks in the large pool must be physically contiguous to support
defragmentation.  No such reassignment algorithm has yet been developed.

# SEE ALSO

lyst(3)
