# NAME

memmgr - memory manager abstraction functions

# SYNOPSIS

    #include "memmgr.h"

    typedef void *(* MemAllocator)
        (char *fileName, int lineNbr, size_t size);
    typedef void (* MemDeallocator)
        (char *fileName, int lineNbr, void * blk);
    typedef void *(* MemAtoPConverter) (unsigned int address);
    typedef unsigned int (* MemPtoAConverter) (void * pointer);

    unsigned int memmgr_add       (char *name,
                                   MemAllocator take, 
                                   MemDeallocator release, 
                                   MemAtoPConverter AtoP, 
                                   MemPtoAConverter PtoA);
    int memmgr_find               (char *name);
    char *memmgr_name             (int mgrId);
    MemAllocator memmgr_take      (int mgrId);
    MemDeallocator memmgr_release (int mgrId);
    MemAtoPConverter memmgr_AtoP  (int mgrId);
    MemPtoAConverter memmgr_PtoA  (int mgrId;

    int memmgr_open               (int memKey,
                                   unsigned long memSize,
                                   char **memPtr,
                                   int *smId,
                                   char *partitionName,
                                   PsmPartition *partition,
                                   int *memMgr,
                                   MemAllocator afn,
                                   MemDeallocator ffn,
                                   MemAtoPConverter apfn,
                                   MemPtoAConverter pafn);
    void memmgr_destroy           (int smId,
                                   PsmPartition *partition);

# DESCRIPTION

"memmgr" is an abstraction layer for administration of memory
management.  It enables multiple memory managers to coexist
in a single application.  Each memory manager specification is required to
include pointers to a memory allocation function, a memory deallocation
function, and functions for translating between local memory pointers
and "addresses", which are abstract memory locations that have private
meaning to the manager.  The allocation function
is expected to return a block of memory of size "size" (in
bytes), initialized to all binary zeroes.  The _fileName_ and _lineNbr_
arguments to the allocation and deallocation functions are expected to
be the values of \_\_FILE\_\_ and \_\_LINE\_\_ at the point at which the functions
are called; this supports any memory usage tracing via sptrace(3) that
may be implemented by the underlying memory management system.

Memory managers are identified by number and by name.  The identifying
number for a memory manager is an index into a private, fixed-length
array of up to 8 memory manager configuration structures; that is,
memory manager number must be in the range 0-7.  However, memory
manager numbers are assigned dynamically and not always predictably.
To enable multiple applications to use the same memory manager for
a given segment of shared memory, a memory manager may be located by
a predefined name of up to 15 characters that is known to all the applications.

The memory manager with manager number 0 is always available; its
name is "std".  Its memory allocation function is calloc(), its
deallocation function is free(), and its pointer/address translation
functions are merely casts.

- unsigned int memmgr\_add(char \*name,
                              MemAllocator take, 
                              MemDeallocator release, 
                              MemAtoPConverter AtoP, 
                              MemPtoAConverter PtoA)

    Add a memory manager to the memory manager array, if not already defined;
    attempting to add a previously added memory manager is not considered an
    error.  _name_ is the name of the memory manager.
    _take_ is a pointer to the manager's memory allocation
    function; _release_ is a pointer to the manager's
    memory deallocation function.  _AtoP_ is a pointer to
    the manager's function for converting an address 
    to a local memory pointer; _PtoA_ is a pointer to
    the manager's pointer-to-address converter function.
    Returns the memory manager ID number assigned to the named manager,
    or -1 on any error.

    _NOTE_: memmgr\_add() is NOT thread-safe.  In a multithreaded execution
    image (e.g., VxWorks), all memory managers should be loaded _before_
    any subordinate threads or tasks are spawned.

- int memmgr\_find(char \*name)

    Return the memmgr ID of the named manager, or -1 if not found.

- char \*memmgr\_name(int mgrId)

    Return the name of the manager given by _mgrId_.

- MemAllocator memmgr\_take(int mgrId)

    Return the allocator function pointer for the manager given by _mgrId_.

- memDeallocator memmgr\_release(int mgrId)

    Return the deallocator function pointer for the manager given by _mgrId_.

- MemAtoPConverter memmgr\_AtoP(int mgrId)

    Return the address-to-pointer converter function
    pointer for the manager given by _mgrId_.

- MemPtoAConverter memmgr\_PtoA(int mgrId)

    Return the pointer-to-address converter function
    pointer for the manager given by _mgrId_.

- int memmgr\_open(int memKey,
                      unsigned long memSize,
                      char \*\*memPtr,
                      int \*smId,
                      char \*partitionName,
                      PsmPartition \*partition,
                      int \*memMgr,
                      MemAllocator afn,
                      MemDeallocator ffn,
                      MemAtoPConverter apfn,
                      MemPtoAConverter pafn);

    memmgr\_open() opens one avenue of access to a PSM managed region of shared
    memory, initializing as necessary.

    In order for multiple tasks to share access to this memory region, all must
    cite the same _memkey_ and _partitionName_ when they call memmgr\_open().  If
    shared access is not necessary, then _memKey_ can be SM\_NO\_KEY and
    _partitionName_ can be any valid partition name.

    If it is known that a prior invocation of memmgr\_open() has already
    initialized the region, then _memSize_ can be zero and _memPtr_
    must be NULL.  Otherwise _memSize_ is required and the required value
    of _memPtr_ depends on whether or not the memory that is to be shared
    and managed has already been allocated (e.g., it's a fixed region of bus
    memory).  If so, then the memory pointer variable that _memPtr_ points
    to must contain the address of that memory region.  Otherwise, _\*memPtr_
    must contain NULL.

    memmgr\_open() will allocate system memory as necessary and will in
    any case return the address of the shared memory region in _\*memPtr_.

    If the shared memory is newly allocated or otherwise not yet under
    PSM management, then memmgr\_open() will invoke psm\_manage() to manage
    the shared memory region.  It will also add a catalogue for the managed
    shared memory region as necessary.

    If _memMgr_ is non-NULL, then memmgr\_open() will additionally call
    memmgr\_add() to establish a new memory manager for this managed shared
    memory region, as necessary.  The index of the applicable memory manager
    will be returned in _memMgr_.  If that memory manager is newly created,
    then the supplied _afn_, _ffn_, _apfn_, and _pafn_ functions (which
    can be written with reference to the memory manager index value returned
    in _memMgr_) have been established as the memory management functions
    for local private access to this managed shared memory region.

    Returns 0 on success, -1 on any error.

- void memmgr\_destroy(int smId, PsmPartition \*partition);

    memmgr\_destroy() terminates all access to a PSM managed region of shared
    memory, invoking psm\_erase() to destroy the partition and sm\_ShmDestroy()
    to destroy the shared memory object.

# EXAMPLE

    /* this example uses the calloc/free memory manager, which is
     * called "std", and is always defined in memmgr. */

     #include "memmgr.h"

     main() 
     {
         int mgrId;
         MemAllocator myalloc;
         MemDeallocator myfree;
         char *newBlock;

         mgrId = memmgr_find("std");
         myalloc = memmgr_take(mgrId);
         myfree = memmgr_release(mgrId);
         ...

         newBlock = myalloc(5000);
         ...
         myfree(newBlock);
     }

# SEE ALSO

psm(3)
