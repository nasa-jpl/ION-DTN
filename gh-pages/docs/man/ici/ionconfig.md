# NAME

ionconfig - ION node configuration parameters file

# DESCRIPTION

ION node configuration parameters are passed to **ionadmin** in a file of
parameter name/value pairs:

> _parameter\_name_ _parameter\_value_

Any line of the file that begins with a '#' character is considered a
comment and is ignored.

**ionadmin** supplies default values for any parameters for which no value
is provided in the node configuration parameters file.

The applicable parameters are as follows:

- sdrName

    This is the character string by which this ION node's SDR database will be
    identified.  (Note that the SDR database infrastructure enables multiple
    databases to be constructed on a single host computer.)  The default value is
    "ion".

- sdrWmSize

    This is the size of the block of dynamic memory that will be reserved as
    private working memory for the SDR system itself.  A block of system memory
    of this size will be allocated (e.g., by malloc()) at the time the SDR system
    is initialized on the host computer.  The default value is 1000000 (1 million
    bytes).

- configFlags

    This is the bitwise "OR" (i.e., the sum) of the flag values that characterize
    the SDR database to use for this ION node.  The default value is 13 (that is,
    SDR\_IN\_DRAM | SDR\_REVERSIBLE | SDR\_BOUNDED).  The SDR configuration flags are
    documented in detail in sdr(3).  To recap:

    - SDR\_IN\_DRAM (1)

        The SDR is implemented in a region of shared memory.  \[Possibly with
        write-through to a file, for fault tolerance.\]

    - SDR\_IN\_FILE (2)

        The SDR is implemented as a file.  \[Possibly cached in a region of shared
        memory, for faster data retrieval.\]

    - SDR\_REVERSIBLE (4)

        Transactions in the SDR are written ahead to a log, making them reversible.

    - SDR\_BOUNDED (8)

        SDR heap updates are not allowed to cross object boundaries.

- heapKey

    This is the shared-memory key by which the pre-allocated block of shared
    dynamic memory to be used as heap space for this SDR can be located, if
    applicable.  The default value is -1, i.e., not specified and not applicable.

- pathName

    This is the fully qualified path name of the directory in which are located
    (a) the file to be used as heap space for this SDR (which will be created, if
    it doesn't already exist), in the event that the SDR is to be implemented in
    a file, and (b) the file to be used to log the database updates of each
    SDR transaction, in the event that transactions in this SDR are to be
    reversible.  The default value is **/tmp**.

- heapWords

    This is the number of words (of 32 bits each on a 32-bit machine, 64 bits
    each on a 64-bit machine) of nominally non-volatile storage to use for ION's
    SDR database.  If the SDR is to be implemented in shared memory and no
    _heapKey_ is specified, a block of shared memory of this size will be
    allocated (e.g., by malloc()) at the time the node is created.  If the
    SDR is to be implemented in a file and no file named **ion.sdr** exists in
    the directory identified by _pathName_, then a file of this name and size
    will be created in this directory and initialized to all binary zeroes.  The
    default value is 250000 words (1 million bytes on a 32-bit computer).

- logSize

    This is the number of bytes of shared memory to use for ION's SDR transaction
    log.  If zero (the default), the transaction log is written to a file rather
    than to memory.  If the log is to be implemented in shared memory and no
    _logKey_ is specified, a block of shared memory of this size will be allocated
    (e.g., by malloc()) at the time the node is created.

- logKey

    This is the shared-memory key by which the pre-allocated block of shared
    dynamic memory to be used for the transaction log for this SDR can be located,
    if applicable.  The default value is -1, i.e., not specified and not applicable.

- wmKey

    This is the shared-memory key by which this ION node's working memory will
    be identified.  The default value is 65281.

- wmAddress

    This is the address of the block of dynamic memory -- volatile storage, which
    is not expected to persist across a system reboot -- to use for this ION
    node's working memory.  If zero, the working memory block will be allocated
    from system memory (e.g., by malloc()) at the time the local ION node is
    created.  The default value is zero.

- wmSize

    This is the size of the block of dynamic memory that will be used for this
    ION node's working memory.  If _wmAddress_ is zero, a block of system memory
    of this size will be allocated (e.g., by malloc()) at the time the node is
    created.  The default value is 5000000 (5 million bytes).

# EXAMPLE

configFlags 1

heapWords 2500000

heapKey -1

pathName /usr/ion

wmSize 5000000

wmAddress 0

# SEE ALSO

ionadmin(1)
