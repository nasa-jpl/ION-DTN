# NAME

llcv - library for manipulating linked-list condition variable objects

# SYNOPSIS

    #include "llcv.h"

    typedef struct llcv_str
    {
        Lyst            list;
        pthread_mutex_t mutex;
        pthread_cond_t  cv;
    } *Llcv;

    typedef int (*LlcvPredicate)(Llcv);

    [see description for available functions]

# DESCRIPTION

A "linked-list condition variable" object (LLCV) is an inter-thread
communication mechanism that pairs a process-private linked list in
memory with a condition variable as provided by the pthreads library.
LLCVs echo in thread programming the standard ION inter-process or
inter-task communication model that pairs shared-memory semaphores
with linked lists in shared memory or shared non-volatile storage.
As in the semaphore/list model, variable-length messages may be
transmitted; the resources allocated to the communication mechanism
grow and shrink to accommodate changes in data rate; the rate at
which messages are issued is completely decoupled from the rate at
which messages are received and processed.  That is, there is no flow
control, no blocking, and therefore no possibility of deadlock or
"deadly embrace".  Traffic spikes are handled without impact on
processing rate, provided sufficient memory is provided to
accommodate the peak backlog.

An LLCV comprises a Lyst, a mutex, and a condition variable.  The Lyst
may be in either private or shared memory, but the Lyst itself is not
shared with other processes.  The reader thread waits on the condition
variable until signaled by a writer that some condition is now true.  The
standard Lyst API functions are used to populate and drain the linked
list.  In order to protect linked list integrity, each thread must call
llcv\_lock() before operating on the Lyst and llcv\_unlock() afterwards.  The
other llcv functions merely effect flow signaling in a way that makes it
unnecessary for the reader to poll or busy-wait on the Lyst.

- Llcv llcv\_open(Lyst list, Llcv llcv)

    Opens an LLCV, initializing as necessary.  The _list_ argument must point
    to an existing Lyst, which may reside in either private or shared dynamic
    memory.  _llcv_ must point to an existing llcv\_str management object, which
    may reside in either static or dynamic (private or shared) memory -- but
    _NOT_ in stack space.  Returns _llcv_ on success, NULL on any error. 

- void llcv\_lock(Llcv llcv)

    Locks the LLCV's Lyst so that it may be updated or examined safely by the
    calling thread.  Fails silently on any error.

- void llcv\_unlock(Llcv llcv)

    Unlocks the LLCV's Lyst so that another thread may lock and update or examine
    it.  Fails silently on any error.

- int llcv\_wait(Llcv llcv, LlcvPredicate cond, int microseconds)

    Returns when the Lyst encapsulated within the LLCV meets the indicated
    condition.  If _microseconds_ is non-negative, will return -1 and set
    _errno_ to ETIMEDOUT when the indicated number of microseconds has
    passed, if and only if the indicated condition has not been met by that
    time.  Negative values of the microseconds argument other than LLCV\_BLOCKING
    (defined as -1) are illegal.  Returns -1 on any error.

- void llcv\_signal(Llcv llcv, LlcvPredicate cond)

    Locks the indicated LLCV's Lyst; tests (evaluates) the indicated condition
    with regard to that LLCV; if the condition is true, signals to the waiting
    reader on this LLCV (if any) that the Lyst encapsulated in the indicated
    LLCV now meets the indicated condition; and unlocks the Lyst.

- void llcv\_signal\_while\_locked(Llcv llcv, LlcvPredicate cond)

    Same as llcv\_signal() except does not lock the Llcv's mutex before
    signalling or unlock afterwards.  For use when the Llcv is already
    locked; prevents deadlock.

- void llcv\_close(Llcv llcv)

    Destroys the indicated LLCV's mutex and condition variable.  Fails silently
    (and has no effect) if a reader is currently waiting on the Llcv.

- int llcv\_lyst\_is\_empty(Llcv Llcv)

    A built-in "convenience" predicate, for use when calling llcv\_wait(),
    llcv\_signal(), or llcv\_signal\_while\_locked().  Returns true if the length
    of the indicated LLCV's encapsulated Lyst is zero, false otherwise.

- int llcv\_lyst\_not\_empty(Llcv Llcv)

    A built-in "convenience" predicate, for use when calling llcv\_wait(),
    llcv\_signal(), or llcv\_signal\_while\_locked().  Returns true if the length
    of the LLCV's encapsulated Lyst is non-zero, false otherwise.

# SEE ALSO

lyst(3)
