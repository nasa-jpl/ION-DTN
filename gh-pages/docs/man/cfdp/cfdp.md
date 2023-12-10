# NAME

cfdp - CCSDS File Delivery Protocol (CFDP) communications library

# SYNOPSIS

    #include "cfdp.h"

    typedef enum
    {
        CksumTypeUnknown = -1,
        ModularChecksum = 0,
        CRC32CChecksum = 2,
        NullChecksum = 15
    } CfdpCksumType;

    typedef int (*CfdpReaderFn)(int fd, unsigned int *checksum, CfdpCksumType ckType);

    typedef int (*CfdpMetadataFn)(uvast fileOffset, unsigned int recordOffset, unsigned int length, int sourceFileFD, char *buffer);

    typedef enum
    {
        CfdpCreateFile = 0,
        CfdpDeleteFile,
        CfdpRenameFile,
        CfdpAppendFile,
        CfdpReplaceFile,
        CfdpCreateDirectory,
        CfdpRemoveDirectory,
        CfdpDenyFile,
        CfdpDenyDirectory
    } CfdpAction;

    typedef enum
    {
        CfdpNoEvent = 0,
        CfdpTransactionInd,
        CfdpEofSentInd,
        CfdpTransactionFinishedInd,
        CfdpMetadataRecvInd,
        CfdpFileSegmentRecvInd,
        CfdpEofRecvInd,
        CfdpSuspendedInd,
        CfdpResumedInd,
        CfdpReportInd,
        CfdpFaultInd,
        CfdpAbandonedInd
    } CfdpEventType;

    typedef struct
    {
        char            *sourceFileName;
        char            *destFileName;
        MetadataList    messagesToUser;
        MetadataList    filestoreRequests;
        CfdpHandler     *faultHandlers;
        int             unacknowledged;
        unsigned int    flowLabelLength;
        unsigned char   *flowLabel;
        int             recordBoundsRespected;
        int             closureRequested;
    } CfdpProxyTask;

    typedef struct
    {
        char            *directoryName;
        char            *destFileName;
    } CfdpDirListTask;

    [see description for available functions]

# DESCRIPTION

The cfdp library provides functions enabling application software to use CFDP
to send and receive files.  It conforms to the Class 1 (Unacknowledged)
service class defined in the CFDP Blue Book and includes implementations of
several standard CFDP user operations.

In the ION implementation of CFDP, the CFDP notion of **entity ID** is taken
to be identical to the BP (CBHE) notion of DTN **node number**.

CFDP entity and transaction numbers may be up to 64 bits in length.  For
portability to 32-bit machines, these numbers are stored in the CFDP state
machine as structures of type CfdpNumber.

To simplify the interface between CFDP the user application without risking
storage leaks, the CFDP-ION API uses MetadataList objects.  A MetadataList is
a specially formatted SDR list of user messages, filestore requests, or
filestore responses.  During the time that a MetadataList is pending
processing via the CFDP API, but is not yet (or is no longer) reachable
from any FDU object, a pointer to the list is appended to one of the
lists of MetadataList objects in the CFDP non-volatile database.  This
assures that any unplanned termination of the CFDP daemons won't leave any
SDR lists unreachable -- and therefore un-recyclable -- due to the
absence of references to those lists.  Restarting CFDP automatically
purges any unused MetadataLists from the CFDP database.  The "user data"
variable of the MetadataList itself is used to implement this feature:
while the list is reachable only from the database root, its user data
variable points to the database root list from which it is referenced;
while the list is attached to a File Delivery Unit, its user data is null. 

By default, CFDP transmits the data in a source file in segments of fixed size.
The user application can override this behavior at the time transmission of
a file is requested, by supplying a file reader callback function that reads
the file -- one byte at a time -- until it detects the end of a "record" that
has application significance.  Each time CFDP calls the reader function, the
function must return the length of one such record (which must be no greater
than 65535).

When CFDP is used to transmit a file, a 32-bit checksum must be provided in
the "EOF" PDU to enable the receiver of the file to assure that it was not
corrupted in transit.  When an application-specific file reader function
is supplied, that function is responsible for updating the computed checksum
as it reads each byte of the file; a CFDP library function is provided for
this purpose.  Two types of file checksums are supported: a simple modular
checksum or a 32-bit CRC.  The checksum type must be passed through to the
CFDP checksum computation function, so it must be provided by (and thus to)
the file reader function.

Per-segment metadata may be provided by the user application.  To enable
this, upon formation of each file data segment, CFDP will invoke the
user-provided per-segment metadata composition callback function (if
any), a function conforming to the CfdpMetadataFn type definition.  The
callback will be passed the offset of the segment within the file, the
segment's offset within the current record (as applicable), the length
of the segment, an open file descriptor for the source file (in case
the data must be read in order to construct the metadata), and a 63-byte
buffer in which to place the new metadata.  The callback function must
return the length of metadata to attach to the file data segment PDU
(may be zero) or -1 in the event of a general system failure.

The return value for each CFDP "request" function (put, cancel, suspend,
resume, report) is a reference number that enables "events" obtained by
calling cfdp\_get\_event() to be matched to the requests that caused them.
Events with reference number set to zero are events that were caused by
autonomous CFDP activity, e.g., the reception of a file data segment.

- int cfdp\_attach()

    Attaches the application to CFDP functionality on the local computer.  Returns
    0 on success, -1 on any error.

- int cfdp\_entity\_is\_started()

    Returns 1 if the local CFDP entity has been started and not yet stopped,
    0 otherwise.

- void cfdp\_detach()

    Terminates all access to CFDP functionality on the local computer.

- void cfdp\_compress\_number(CfdpNumber \*toNbr, uvast from)

    Converts an unsigned **vast** number into a CfdpNumber structure, e.g., for
    use when invoking the cfdp\_put() function.

- void cfdp\_decompress\_number(uvast toNbr, CfdpNumber \*from)

    Converts a numeric value in a CfdpNumber structure to an unsigned **vast**
    integer.

- void cfdp\_update\_checksum(unsigned char octet, uvast \*offset, unsigned int \*checksum, CfdpCksumType ckType)

    For use by an application-specific file reader callback function, which must
    pass to cfdp\_update\_checksum() the value of each byte (octet) it reads.
    _offset_ must be _octet_'s displacement in bytes from the start of the
    file.  The _checksum_ pointer is provided to the reader function by CFDP.

- MetadataList cfdp\_create\_usrmsg\_list()

    Creates a non-volatile linked list, suitable for containing messages-to-user
    that are to be presented to cfdp\_put().

- int cfdp\_add\_usrmsg(MetadataList list, unsigned char \*text, int length)

    Appends the indicated message-to-user to _list_.

- int cfdp\_get\_usrmsg(MetadataList list, unsigned char \*textBuf, int \*length)

    Removes from _list_ the first of the remaining messages-to-user contained in
    the list and delivers its text and length.  When the last message in the
    list is delivered, destroys the list.

- void cfdp\_destroy\_usrmsg\_list(MetadataList \*list)

    Removes and destroys all messages-to-user in _list_ and destroys the list.

- MetadataList cfdp\_create\_fsreq\_list()

    Creates a non-volatile linked list, suitable for containing filestore requests
    that are to be presented to cfdp\_put().

- int cfdp\_add\_fsreq(MetadataList list, CfdpAction action, char \*firstFileName, char \*seconfdFIleName)

    Appends the indicated filestore request to _list_.

- int cfdp\_get\_fsreq(MetadataList list, CfdpAction \*action, char \*firstFileNameBuf, char \*secondFileNameBuf)

    Removes from _list_ the first of the remaining filestore requests contained in
    the list and delivers its action code and file names.  When the last request in
    the list is delivered, destroys the list.

- void cfdp\_destroy\_fsreq\_list(MetadataList \*list)

    Removes and destroys all filestore requests in _list_ and destroys the list.

- int cfdp\_get\_fsresp(MetadataList list, CfdpAction \*action, int \*status, char \*firstFileNameBuf, char \*secondFileNameBuf, char \*messageBuf)

    Removes from _list_ the first of the remaining filestore responses contained
    in the list and delivers its action code, status, file names, and message.
    When the last response in the list is delivered, destroys the list.

- void cfdp\_destroy\_fsresp\_list(MetadataList \*list)

    Removes and destroys all filestore responses in _list_ and destroys the list.

- int cfdp\_read\_space\_packets(int fd, unsigned int \*checksum)

    This is a standard "reader" function that segments the source file on CCSDS
    space packet boundaries.  Multiple small packets may be aggregated into a
    single file data segment.

- int cfdp\_read\_text\_lines(int fd, unsigned int \*checksum)

    This is a standard "reader" function that segments a source file of text lines
    on line boundaries.

- int cfdp\_put(CfdpNumber \*destinationEntityNbr, unsigned int utParmsLength, unsigned char \*utParms, char \*sourceFileName, char \*destFileName, CfdpReaderFn readerFn, CfdpMetadataFn metadataFn, CfdpHandler \*faultHandlers, unsigned int flowLabelLength, unsigned char \*flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpTransactionId \*transactionId)

    Sends the file identified by _sourceFileName_ to the CFDP entity identified by
    _destinationEntityNbr_.  _destinationFileName_ is used to indicate the name
    by which the file will be catalogued upon arrival at its final destination; if
    NULL, the destination file name defaults to _sourceFileName_.  If 
    _sourceFileName_ is NULL, it is assumed that the application is requesting
    transmission of metadata only (as discussed below) and _destinationFileName_
    is ignored.  Note that both _sourceFileName_ and _destinationFileName_ are
    interpreted as path names, i.e., directory paths may be indicated in either
    or both.  The syntax of path names is opaque to CFDP; the syntax of
    _sourceFileName_ must conform to the path naming syntax of the source
    entity's file system and the syntax of _destinationFileName_ must conform
    to the path naming syntax of the destination entity's file system.

    The byte array identified by _utParms_, if non-NULL, is interpreted as
    transmission control information that is to be passed on to the UT layer.  The
    nominal UT layer for ION's CFDP being Bundle Protocol, the _utParms_ array is
    normally a pointer to a structure of type BpUtParms; see the _bp_ man page
    for a discussion of the parameters in that structure. 

    _closureLatency_ is the length of time following transmission of the EOF PDU
    within which a responding Transaction Finish PDU is expected.  If no Finish
    PDU is requested, this parameter value should be zero.

    _messagesToUser_ and _filestoreRequests_, where non-zero, must be the
    addresses of non-volatile linked lists (that is, linked lists in ION's
    SDR database) of CfdpMsgToUser and CfdpFilestoreRequest objects identifying
    metadata that are intended to accompany the transmitted file.  Note that
    this metadata may accompany a file of zero length (as when _sourceFileName_
    is NULL as noted above) -- a transmission of metadata only.

    On success, the function populates _\*transactionID_ with the source entity
    ID and the transaction number assigned to this transmission and returns the
    request number identifying this "put" request.  The transaction ID may be
    used to suspend, resume, cancel, or request a report on the progress of
    this transmission.  cfdp\_put() returns -1 on any error.

- int cfdp\_cancel(CfdpTransactionId \*transactionId)

    Cancels transmission or reception of the indicated transaction.  Note that,
    since the ION implementation of CFDP is Unacknowledged, cancellation of a
    file transmission may have little effect.  Returns request number on success,
    \-1 on any error.

- int cfdp\_suspend(CfdpTransactionId \*transactionId)

    Suspends transmission of the indicated transaction.  Note that, since the ION
    implementation of CFDP is Unacknowledged, suspension of a file transmission
    may have little effect.  Returns request number on success, -1 on any error.

- int cfdp\_resume(CfdpTransactionId \*transactionId)

    Resumes transmission of the indicated transaction.  Note that, since the ION
    implementation of CFDP is Unacknowledged, resumption of a file transmission
    may have little effect.  Returns request number on success, -1 on any error.

- int cfdp\_report(CfdpTransactionId \*transactionId)

    Requests issuance of a report on the transmission or reception progress of
    the indicated transaction.  The report takes the form of a character string
    that is returned in a CfdpEvent structure; use cfdp\_get\_event() to receive
    the event (which may be matched to the request by request number).  Returns
    request number on success, 0 if transaction is unknown, -1 on any error.

- int cfdp\_get\_event(CfdpEventType \*type, time\_t \*time, int \*reqNbr, CfdpTransactionId \*transactionId, char \*sourceFileNameBuf, char \*destFileNameBuf, uvast \*fileSize, MetadataList \*messagesToUser, uvast \*offset, unsigned int \*length, CfdpCondition \*condition, uvast \*progress, CfdpFileStatus \*fileStatus, CfdpDeliveryCode \*deliveryCode, CfdpTransactionId \*originatingTransactionId, char \*statusReportBuf, MetadataList \*filestoreResponses);

    Populates return value fields with data from the oldest CFDP event not yet
    delivered to the application.  

    cfdp\_get\_event() always blocks indefinitely until an CFDP processing
    event is delivered or the function is interrupted by an invocation of
    cfdp\_interrupt().

    On application error, returns zero but sets errno to EINVAL.  Returns -1 on
    system failure, zero otherwise.

- void cfdp\_interrupt()

    Interrupts an cfdp\_get\_event() invocation.  This function is designed to be
    called from a signal handler.

- int cfdp\_rput(CfdpNumber \*respondentEntityNbr, unsigned int utParmsLength, unsigned char \*utParms, char \*sourceFileName, char \*destFileName, CfdpReaderFn readerFn, CfdpHandler \*faultHandlers, unsigned int flowLabelLength, unsigned char \*flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpNumber \*beneficiaryEntityNbr, CfdpProxyTask \*proxyTask, CfdpTransactionId \*transactionId)

    Sends to the indicated respondent entity a "proxy" request to perform a file
    transmission.  The transmission is to be subject to the configuration values
    in _proxyTask_ and the destination of the file is to be the entity identified
    by _beneficiaryEntityNbr_.

- int cfdp\_rput\_cancel(CfdpNumber \*respondentEntityNbr, unsigned int utParmsLength, unsigned char \*utParms, char \*sourceFileName, char \*destFileName, CfdpReaderFn readerFn, CfdpHandler \*faultHandlers, unsigned int flowLabelLength, unsigned char \*flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpTransactionId \*rputTransactionId, CfdpTransactionId \*transactionId)

    Sends to the indicated respondent entity a request to cancel a prior "proxy"
    file transmission request as identified by _rputTransactionId_, which is
    the value of _transactionId_ that was returned by that earlier proxy
    transmission request.

- int cfdp\_get(CfdpNumber \*respondentEntityNbr, unsigned int utParmsLength, unsigned char \*utParms, char \*sourceFileName, char \*destFileName, CfdpReaderFn readerFn, CfdpHandler \*faultHandlers, unsigned int flowLabelLength, unsigned char \*flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpProxyTask \*proxyTask, CfdpTransactionId \*transactionId)

    Same as **cfdp\_rput** except that _beneficiaryEntityNbr_ is omitted; the local
    entity is the implicit beneficiary of the request.

- int cfdp\_rls(CfdpNumber \*respondentEntityNbr, unsigned int utParmsLength, unsigned char \*utParms, char \*sourceFileName, char \*destFileName, CfdpReaderFn readerFn, CfdpHandler \*faultHandlers, unsigned int flowLabelLength, unsigned char \*flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpDirListTask \*dirListTask, CfdpTransactionId \*transactionId)

    Sends to the indicated respondent entity a request to prepare a directory
    listing, save that listing in a file, and send it to the local entity.  The
    request is subject to the configuration values in _dirListTask_. 

- int cfdp\_preview(CfdpTransactionId \*transactionId, uvast offset, unsigned int length, char \*buffer);

    This function is provided to enable the application to get an advance look
    at the content of a file that CFDP has not yet fully received.  Reads _length_
    bytes starting at _offset_ bytes from the start of the file that is the
    destination file of the transaction identified by _transactionID_, into
    _buffer_.  On user error (transaction is nonexistent or is outbound, or
    offset is beyond the end of file) returns 0.  On system failure, returns -1.
    Otherwise returns number of bytes read.

- int cfdp\_map(CfdpTransactionId \*transactionId, unsigned int \*extentCount, CfdpExtent \*extentsArray);

    This function is provided to enable the application to report on the portions
    of a partially-received file that have been received and written.  Lists the
    received continuous data extents in the destination file of the transaction
    identified by _transactionID_.  The extents (offset and length) are returned
    in the elements of _extentsArray_; the number of extents returned in the
    array is the total number of continuous extents received so far, or
    _extentCount_, whichever is less.  The total number of extents received
    so far is returned as the new value of _extentCount_.  On system failure,
    returns -1.  Otherwise returns 0.

# SEE ALSO

cfdpadmin(1), cfdprc(5)
