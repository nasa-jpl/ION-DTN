# NAME

tc - the Trusted Collective system for Delay-Tolerant Networking

# SYNOPSIS

    #include "tcc.h"

    [see description for available functions]

# DESCRIPTION

The TC system provides a trustworthy framework for delay-tolerant
distribution of information that is of critical importance - that is,
information that must be made available as needed and must not be
corrupt - but is not confidential.  As such it accomplishes some of
the same objectives as are accomplished by "servers" in the Internet.

A central principle of TC is that items of critical information may
have **effective times** which condition their applicability.  For example,
a change in rules restricting air travel will typically be scheduled to
take effect on some stated date in the near future.  Effective times
enable critical information to be distributed far in advance of the
time at which it will be needed, which is what makes TC delay-tolerant:
when the time arrives at which a node needs a given item of critical
information, the information is already in place.  No query/response
exchange is necessary.

The TC framework for a given TC **application** comprises (a) a collective
authority, which should include several geographically distributed
"authority" nodes, and (b) "client" nodes which utilize the services
of the collective authority, possibly including the key authority nodes
themselves.  The framework is designed to convey to every participating
client node the critical information submitted to the collective
authority by all other participating client nodes, in a trustworthy
manner, prior to the times at which those items of information become
effective.  It operates as follows.

The user function of a given TC application generates **records** containing
critical information and issues those records as the application data units
forming the payloads of BP bundles.  The destination of each such bundle
is the multicast group designated for receivers of the records of that
application.  The members of that multicast group are the authority nodes
of the application's collective authority.

The records are delivered to the **tcarecv** daemons of the authority
nodes.  Each such daemon validates the received records and inserts all
valid records in its authority node's private database of pending records.

Periodically, the **tcacompile** daemons of all authority nodes in the
application's collective authority simultaneously compile **bulletins** of
all records recently received from user nodes.  (A TC bulletin is simply
an array of contiguous TC records.)  These daemons then all issue their
bulletins as the application data units forming the payloads of BP
bundles.  The destination of each such bundle is the multicast group
designated for receivers of the bulletins of that application.  The members
of that multicast group are, again, the authority nodes of the application's
collective authority.  In addition, each **tcacompile** daemon spawns one
**tcapublish** process that is assigned the task of processing the bulletins
compiled by all authority nodes during this iteration of the compilation cycle.

The bulletins are delivered to the **tcapublish** processes of all authority
nodes in the application's collective authority.  The **tcapublish** processes
then compute a common consensus bulletin, which includes all recently asserted
records that all of the authority nodes have received and found valid.

Each **tcapublish** process then computes a hash for the consensus bulletin and
erasure-codes the bulletin, producing a list of **code blocks**; the hashes and
lists of blocks will be identical for all key authority nodes.  It then
issues a small subset of those code blocks as the application data units
forming the payloads of BP bundles.  The destination of each such bundle
is the multicast group designated for receivers of the code blocks of the
application.  The members of that multicast group are the **tcc** (that is,
TC **client**) daemons serving the application's user nodes.  The subsets
of the block list issued by all **tcapublish** daemons are different, but
each code block is tagged with the common bulletin hash.

The code blocks are delivered to the **tcc** daemons of all of the
application's user nodes, each of which uses the received code blocks
to reassemble the consensus bulletin.  Code blocks with differing
bulletin hashes are not used to reassemble the same bulletin, and
the erasure coding of the bulletin prevents failure to receive all
code blocks from preventing reassembly of the complete bulletin.  When
a consensus bulletin has been successfully reassembled, the records
in the bulletin are delivered to the user function.

The **tcaboot** and **tcaadmin** utilities are used to configure the
collective authority of a given TC application; the **tccadmin** utility is
used to configure TC client functionality for a given TC application on
a given user node.

The TC library functions provided to TC application user software are
described below.

- int tc\_serialize(char \*buffer, unsigned int buflen, uvast nodeNbr, time\_t effectiveTime, time\_t assertionTime, unsigned short datLength, unsigned char \*datValue)

    Forms in _buffer_ a serialized TC record, ready for transmission as a BP
    application data unit, that contains the indicated node number, effective time,
    assertion time, application data length, and application data.  Returns the
    length of the record, or -1 on any missing arguments.

- int tcc\_getBulletin(int blocksGroupNbr, char \*\*bulletinContent, int \*length)

    Places in _\*bulletinContent_ a pointer to an ION private working memory
    buffer containing the content of the oldest previously unhandled received
    TC bulletin for the application identified by _blocksGroupNbr_.  Returns
    0 on success, -1 on any system failure.  A returned buffer length of 0
    indicates that the function was interrupted and must be repeated.

    Note that the calling function MUST **MRELEASE** the bulletin content buffer
    when processing is complete.  Failure to do so will introduce a memory leak.

- int tc\_deserialize(char \*\*buffer, int \*buflen, unsigned short maxDatLength, uvast \*nodeNbr, time\_t \*effectiveTime, time\_t \*assertionTime, unsigned short \*datLength, unsigned char \*datValue)

    Parses out of the bulletin in _buffer_ the data elements of a single
    serialized TC record: node number, effective time, assertion time,
    application data length, and application data.  Returns -1 on any missing
    arguments, 0 on any record malformation, record length otherwise.

# SEE ALSO

tcaboot(1), tcaadmin(1), tcarecv(1), tcacompile(1), tcapublish(1), tccadmin(1), tcc(1)
