# ION Application Services

This section covers interfaces for users to access the following four DTN application-level services provided by ION:

* *CFDP* (CCSDS File Delivery Protocol) - CCSDS Bluebook CCSDS 727.0-B-4
* *BSS* (Bundle Streaming Service)
* *AMS* (Asynchronous Message Service) - CCSDS Bluebook CCSDS 735.1-B-1, Greenbook CCSDS 735.0-G-1
* *DTPC* (Delay-Tolerant Payload Conditioning)

## CCSDS File Delivery Protocol (CFDP) APIs

The CFDP library provides functions enabling application software to use CFDP to send and receive files. It conforms to the Class 1 (Unacknowledged) service class defined in the CFDP Blue Book and includes several standard CFDP user operations implementations.

In the ION implementation of CFDP, the CFDP notion of entity ID is identical to the BP (CBHE) notion of DTN node number used in ION.

CFDP entity and transaction numbers may be up to 64 bits in length. For portability to 32-bit machines, these numbers are stored in the CFDP state machine as structures of type `CfdpNumber`.

To simplify the interface between CFDP and the user application without risking storage leaks, the CFDP-ION API uses `MetadataList` objects. A MetadataList is a specially formatted SDR list of user messages, filestore requests, or filestore responses. During the time that a MetadataList is pending processing via the CFDP APIs, but is not yet (or is no longer) reachable from any FDU object, a pointer to the list is appended to one of the lists of MetadataList objects in the CFDP non-volatile database. This assures that any unplanned termination of the CFDP daemons won't leave any SDR lists unreachable - and therefore un-recyclable - due to the absence of references to those lists. Restarting CFDP will automatically purge any unused MetadataLists from the CFDP database. The "user data" variable of the MetadataList itself is used to implement this feature: while the list is reachable only from the database root, its user data variable points to the database root list from which it is referenced. In contrast, the list is attached to a File Delivery Unit; its user data is NULL.

CFDP transmits the data in a source file in fixed-sized segments by default. The user application can override this behavior at the time transmission of a file is requested by supplying a file reader callback function that reads the file - one byte at a time - until it detects the end of a "record" that has application significance. Each time CFDP calls the reader function, the function must return the length of one such record (not greater than 65535).

When CFDP is used to transmit a file, a 32-bit checksum must be provided in the "EOF" PDU to enable the receiver of the file to ensure that it was not corrupted in transit. When supplied with an application-specific file reader function, it updates the computed checksum as it reads each file byte; a CFDP library function is provided. Two types of file checksums are supported: a simple modular checksum or a 32-bit CRC. The checksum type must be passed through to the CFDP checksum computation function, so it must be provided by (and thus to) the file reader function.

The user application may provide per-segment metadata. To enable this, upon formation of each file data segment, CFDP will invoke the user-provided per-segment metadata composition callback function (if any), a function conforming to the CfdpMetadataFn type definition. The callback will be passed the offset of the segment within the file, the segment's offset within the current record (as applicable), the length of the segment, an open file descriptor for the source file (in case the data must be read to construct the metadata), and a 63-byte buffer in which to place the new metadata. The callback function must return the metadata length to attach to the file data segment PDU (may be zero) or -1 in case of a general system failure.

The return value for each CFDP "request" function (put, cancel, suspend, resume, report) is a reference number that enables "events" obtained by calling cfdp_get_event() to be matched to the requests that caused them. Events with a reference number set to zero were caused by autonomous CFDP activity, e.g., the reception of a file data segment.

```c
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
```

### cfdp_attach

```c
int cfdp_attach()
```

Attaches the application to CFDP functionality on the local computer. 

Return Value

* 0: on success
* -1: on any error

### cfdp_entity_is_started

```c
int cfdp_entity_is_started()
```

Return Value
* 1: if the local CFDP entity has been started and not yet stopped
* 0: otherwise

### cfdp_detach

```c
void cfdp_detach()
```

Terminates all access to CFDP functionality on the local computer.

### cfdp_compress_number

```c
void cfdp_compress_number(CfdpNumber *toNbr, uvast from)
```

Converts an unsigned vast number into a CfdpNumber structure, e.g., for use when invoking the cfdp_put() function.

### cfdp_decompress_number

```c
void cfdp_decompress_number(uvast toNbr, CfdpNumber *from)
```

Converts a numeric value in a CfdpNumber structure to an unsigned vast integer.

### cfdp_update_checksum

```c
void cfdp_update_checksum(unsigned char octet, uvast *offset, unsigned int *checksum, CfdpCksumType ckType)
```

For use by an application-specific file reader callback function, which must pass to cfdp_update_checksum() the value of each byte (octet) it reads. offset must be octet's displacement in bytes from the start of the file. The checksum pointer is provided to the reader function by CFDP.

### cfdp_create_usrmsg_list

```c
MetadataList cfdp_create_usrmsg_list()
```

Creates a non-volatile linked list, suitable for containing messages-to-user that are to be presented to cfdp_put().

### cfdp_add_usrmsg

```c
int cfdp_add_usrmsg(MetadataList list, unsigned char *text, int length)
```

Appends the indicated message-to-user to list.

### cfdp_get_usrmsg

```c
int cfdp_get_usrmsg(MetadataList list, unsigned char *textBuf, int *length)
```

Removes from list the first of the remaining messages-to-user contained in the list and delivers its text and length. When the last message in the list is delivered, destroys the list.

### cfdp_destroy_usrmsg_list

```c
void cfdp_destroy_usrmsg_list(MetadataList *list)
```

Removes and destroys all messages-to-user in list and destroys the list.

### cfdp_create_fsreq_list

```c
MetadataList cfdp_create_fsreq_list()
```

Creates a non-volatile linked list, suitable for containing filestore requests that are to be presented to cfdp_put().

### cfdp_add_fsreq

```c
int cfdp_add_fsreq(MetadataList list, CfdpAction action, char *firstFileName, char *seconfdFIleName)
```

Appends the indicated filestore request to list.

### cfdp_get_fsreq

```c
int cfdp_get_fsreq(MetadataList list, CfdpAction *action, char *firstFileNameBuf, char *secondFileNameBuf)
```

Removes from list the first of the remaining filestore requests contained in the list and delivers its action code and file names. When the last request in the list is delivered, destroys the list.

### cfdp_destroy_fsreq_list

```c
void cfdp_destroy_fsreq_list(MetadataList *list)
```

Removes and destroys all filestore requests in list and destroys the list.

### cfdp_get_fsresp

```c
int cfdp_get_fsresp(MetadataList list, CfdpAction *action, int *status, char *firstFileNameBuf, char *secondFileNameBuf, char *messageBuf)
```

Removes from list the first of the remaining filestore responses contained in the list and delivers its action code, status, file names, and message. When the last response in the list is delivered, it destroys the list.

### cfdp_destroy_fsresp_list

```c
void cfdp_destroy_fsresp_list(MetadataList *list)
```

Removes and destroys all filestore responses in list and destroys the list.

### cfdp_read_space_packets

```c
int cfdp_read_space_packets(int fd, unsigned int *checksum)
```

This is a standard "reader" function that segments the source file on CCSDS space packet boundaries. Multiple small packets may be aggregated into a single file data segment.

### cfdp_read_text_lines

```c
int cfdp_read_text_lines(int fd, unsigned int *checksum)
```

This is a standard "reader" function that segments a source file of text lines on line boundaries.

### cfdp_put

```c
int cfdp_put(CfdpNumber *destinationEntityNbr, unsigned int utParmsLength, unsigned char *utParms, char *sourceFileName, char *destFileName, CfdpReaderFn readerFn, CfdpMetadataFn metadataFn, CfdpHandler *faultHandlers, unsigned int flowLabelLength, unsigned char *flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpTransactionId *transactionId)
```

Sends the file identified by `sourceFileName` to the CFDP entity identified by `destinationEntityNbr`. destinationFileName is used to indicate the name by which the file will be catalogued upon arrival at its final destination; if NULL, the destination file name defaults to sourceFileName. If sourceFileName is NULL, it is assumed that the application is requesting transmission of metadata only (as discussed below) and destinationFileName is ignored. Note that both sourceFileName and destinationFileName are interpreted as path names, i.e., directory paths may be indicated in either or both. The syntax of path names is opaque to CFDP; the syntax of sourceFileName must conform to the path naming syntax of the source entity's file system and the syntax of destinationFileName must conform to the path naming syntax of the destination entity's file system.

The byte array identified by `utParms`, if non-NULL, is interpreted as transmission control information that is to be passed on to the UT layer. The nominal UT layer for ION's CFDP being Bundle Protocol, the utParms array is normally a pointer to a structure of type BpUtParms; see the bp man page for a discussion of the parameters in that structure.

`closureLatency` is the length of time following transmission of the EOF PDU within which a responding Transaction Finish PDU is expected. If no Finish PDU is requested, this parameter value should be zero.

`messagesToUser` and `filestoreRequests`, where non-zero, must be the addresses of non-volatile linked lists (that is, linked lists in ION's SDR database) of `CfdpMsgToUser` and `CfdpFilestoreRequest` objects identifying metadata that are intended to accompany the transmitted file. Note that this metadata may accompany a file of zero length (as when sourceFileName is NULL as noted above) -- a transmission of metadata only.

Return Value

* request number of this "put" request: On success, the function populates `*transactionID` with the source entity ID and the transaction number assigned to this transmission and returns the request number identifying this "put" request. The transaction ID may be used to suspend, resume, cancel, or request a report on the progress of this transmission. 
* -1: on any error

### cfdp_cancel

```c
int cfdp_cancel(CfdpTransactionId *transactionId)
```

Cancels transmission or reception of the indicated transaction. Note that, since the ION implementation of CFDP is Unacknowledged, cancellation of a file transmission may have little effect. 

Return Value
* request number: on success
* -1: on any error

### cfdp_suspend

```c
int cfdp_suspend(CfdpTransactionId *transactionId)
```

Suspends transmission of the indicated transaction. Note that, since the ION implementation of CFDP is Unacknowledged, suspension of a file transmission may have little effect. 

Return Value
* request number: on success
* -1: on any error

### cfdp_resume

```c
int cfdp_resume(CfdpTransactionId *transactionId)
```

Resumes transmission of the indicated transaction. Note that, since the ION implementation of CFDP is Unacknowledged, resumption of a file transmission may have little effect. 

Return Value
* request number: on success
* -1: on any error

### cfdp_report

```c
int cfdp_report(CfdpTransactionId *transactionId)
```

Requests issuance of a report on the transmission or reception progress of the indicated transaction. The report takes the form of a character string that is returned in a CfdpEvent structure; use cfdp_get_event() to receive the event (which may be matched to the request by request number). 

Return Value
* request number: on success
* 0: if the transaction ID is unknown
* -1: on any error

### cfdp_get_event

```c
int cfdp_get_event(CfdpEventType *type, time_t *time, int *reqNbr, CfdpTransactionId *transactionId, char *sourceFileNameBuf, char *destFileNameBuf, uvast *fileSize, MetadataList *messagesToUser, uvast *offset, unsigned int *length, CfdpCondition *condition, uvast *progress, CfdpFileStatus *fileStatus, CfdpDeliveryCode *deliveryCode, CfdpTransactionId *originatingTransactionId, char *statusReportBuf, MetadataList *filestoreResponses);
```

Populates return value fields with data from the oldest CFDP event not yet delivered to the application. cfdp_get_event() blocks indefinitely until a CFDP processing event is delivered or the function is interrupted by an invocation of cfdp_interrupt().

Return Value
* 0: on success -OR- on application error, returns zero but sets errno to EINVAL. 
* -1: on system failure

### cfdp_interrupt

```c
void cfdp_interrupt()
```

Interrupts an cfdp_get_event() invocation. This function is designed to be called from a signal handler.

### cfdp_rput 

```c
int cfdp_rput(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength, unsigned char *utParms, char *sourceFileName, char *destFileName, CfdpReaderFn readerFn, CfdpHandler *faultHandlers, unsigned int flowLabelLength, unsigned char *flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpNumber *beneficiaryEntityNbr, CfdpProxyTask *proxyTask, CfdpTransactionId *transactionId)
```

Sends to the indicated respondent entity a "proxy" request to perform a file transmission. The transmission is to be subject to the configuration values in proxyTask and the destination of the file is to be the entity identified by `beneficiaryEntityNbr`.

### cfdp_rput_cancel

```c
int cfdp_rput_cancel(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength, unsigned char *utParms, char *sourceFileName, char *destFileName, CfdpReaderFn readerFn, CfdpHandler *faultHandlers, unsigned int flowLabelLength, unsigned char *flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpTransactionId *rputTransactionId, CfdpTransactionId *transactionId)
```

Sends to the indicated respondent entity a request to cancel a prior "proxy" file transmission request as identified by rputTransactionId, which is the value of transactionId that was returned by that earlier proxy transmission request.

### cfdp_get

```c
int cfdp_get(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength, unsigned char *utParms, char *sourceFileName, char *destFileName, CfdpReaderFn readerFn, CfdpHandler *faultHandlers, unsigned int flowLabelLength, unsigned char *flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpProxyTask *proxyTask, CfdpTransactionId *transactionId)
```

Same as cfdp_rput except that beneficiaryEntityNbr is omitted; the local entity is the implicit beneficiary of the request.

### cfdp_rls

```c
int cfdp_rls(CfdpNumber *respondentEntityNbr, unsigned int utParmsLength, unsigned char *utParms, char *sourceFileName, char *destFileName, CfdpReaderFn readerFn, CfdpHandler *faultHandlers, unsigned int flowLabelLength, unsigned char *flowLabel, unsigned int closureLatency, MetadataList messagesToUser, MetadataList filestoreRequests, CfdpDirListTask *dirListTask, CfdpTransactionId *transactionId)
```

Sends to the indicated respondent entity a request to prepare a directory listing, save that listing in a file, and send it to the local entity. The request is subject to the configuration values in `dirListTask`.

### cfdp_preview

```c
int cfdp_preview(CfdpTransactionId *transactionId, uvast offset, unsigned int length, char *buffer);
```

This function enables the application to get an advanced look at the content of a file that CFDP has not yet fully received. Reads length bytes starting at offset bytes from the start of the file that is the destination file of the transaction identified by `transactionID`, into `buffer`. 

Return Value
* number of bytes read: on success
* 0: on user error (transaction is nonexistent or is outbound, or offset is beyond the end of file) 
* -1: on system failure 

### cfdp_map

```c
int cfdp_map(CfdpTransactionId *transactionId, unsigned int *extentCount, CfdpExtent *extentsArray);
```

This function enables the application to report on the portions of a partially-received file that have been received and written. Lists the received continuous data extents in the destination file of the transaction identified by `transactionID`. The extents (offset and length) are returned in the elements of `extentsArray`; the number of extents returned in the array is the total number of continuous extents received so far, or `extentCount`, whichever is less. 

Return Value
* 0: on success, the total number of extents received so far is reported through `extentCount`
* -1: on system failure, returns -1

### CFDP Shell Test Program: `cfdptest`

ION provides application CFDP test program called `cfdptest`, which installed as part of regular ION build/install process and can be invoked from terminal this way:

```bash
cfdptest
```

The shell program present a ':' prompt for interactive mode commanding. You can type 'h' to see a list of available commands.

One can also feed a sequence of commands to `cfdptest` non-interactively such that you will not see the stdout of the program. This is useful for running automated tests.

```bash
cfdptest [file_containing_cfdptest_commands]
```

A third way to use cfdptest is to feed command scripts but allows the interactive responses to be displayed in stdout:

```bash
cfdptest < [file_containing_cfdptest_commands]
```

The `cfdptest.c` source code is also provided as a code examples on how write applications using the CFDP APIs. The cfdptest command set can be found in the manual pages [here.](https://nasa-jpl.github.io/ION-DTN/man/cfdp/cfdptest/?h=cfdptest)

### CFDP Application Code Example

_this section is work-in-progress_

------------------------

## Bundle Streaming Service (BSS)

The BSS library supports the streaming of data over delay-tolerant networking (DTN) bundles. The intent of the library is to enable applications that pass streaming data received in transmission time order (i.e., without time regressions) to an application-specific "display" function -- notionally for immediate real-time display -- but to store all received data (including out-of-order data) in a private database for playback under user control. The reception and real-time display of in-order data is performed by a background thread, leaving the application's main (foreground) thread free to respond to user commands controlling playback or other application-specific functions.

The application-specific "display" function invoked by the background thread must conform to the RTBHandler type definition. It must return 0 on success, -1 on any error that should terminate the background thread. Only on return from this function will the background thread proceed to acquire the next BSS payload.

All data acquired by the BSS background thread is written to a BSS database comprising three files: table, list, and data. The name of the database is the root name that is common to the three files, e.g., db3.tbl, db3.lst, db3.dat would be the three files making up the db3 BSS database. All three files of the selected BSS database must reside in the same directory of the file system.

Several replay navigation functions in the BSS library require that the application provide a navigation state structure of type bssNav as defined in the bss.h header file. The application is not reponsible for populating this structure; it's strictly for the private use of the BSS library.

### Bundle Streaming Service (BSS) & Bundle Streaming Service Protocol (BSSP CLA)

The Bundle Streaming Service (BSS) and the Bundle Streaming Service Protocol (BSSP) CLA are independent modules.

The BSSP CLA is designed to emulate a connection between two DTN neighboring nodes characterized by two delivery mechanisms: (a) a minimal delay, unreliable channel (physical or logical), and (b) a potentially delayed, but reliable channel. The minimal delay channel is emulated by transpot UDP (with a timer mechanism added) and the reliable channel is emulated via TCP transport. 

A DTN user mission may decide to use a single CCSDS AOS or TM downlink with LTP CLA running on top as its reliability mechanism. In that case, it can directly use the LTP CLA in ION and interface it with the CCSDS framing protocol which could be implemented by the mission's avionic system or the radio. 

However, it is also possible that a mission may utilize different types of transports, for example, using multiple downlinks via  S, X, Ka-band or optical, each with different reliability mechanism (or not). Alternatively, a flight system may also use commercial communications services with differentiated delays and levels of reliability. In such case, BSSP can be used to approximate such configuration in a lab environment for prototyping and testing the impact on streaming data delivery, until the actual CLAs are implemented and tested.

The Bundle Streaming Service, on the other hand, is an application-level service that can be used with any underlying CLAs to handle both realtime and delayed, in-order playback of streaming data including video, audio, and telemetry. When the user scenario is appropriate, BSS can certainly be used over BSSP CLA, but that is not a requirement. 

## BSS APIs

The following section describes the BSS library APIs.

### bssOpen

```c
int bssOpen(char *bssName, char *path, char *eid)
```
Opens access to a BSS database, to enable data playback. bssName identifies the specific BSS database that is to be opened. path identifies the directory in which the database resides. eid is ignored. On any failure, returns -1. On success, returns zero.

### bssStart

```c
int bssStart(char *bssName, char *path, char *eid, char *buffer, int bufLen, RTBHandler handler)
```

Starts a BSS data acquisition background thread. bssName identifies the BSS database into which data will be acquired. path identifies the directory in which that database resides. eid is used to open the BP endpoint at which the delivered BSS bundle payload contents will be acquired. buffer identifies a data acquisition buffer, which must be provided by the application, and bufLen indicates the length of that buffer; received bundle payloads in excess of this length will be discarded.

handler identifies the display function to which each in-order bundle payload will be passed. The time and count parameters passed to this function identify the received bundle, indicating the bundle's creation timestamp time (in seconds) and counter value. The buffer and bufLength parameters indicate the location into which the bundle's payload was acquired and the length of the acquired payload. handler must return -1 on any unrecoverable system error, 0 otherwise. A return value of -1 from handler will terminate the BSS data acquisition background thread.

On any failure, returns -1. On success, returns zero.

### bssRun

```c
int bssRun(char *bssName, char *path, char *eid, char *buffer, int bufLen, RTBHandler handler)
```

A convenience function that performs both bssOpen() and bssStart(). On any failure, returns -1. On success, returns zero.

### bssClose

```c
void bssClose()
```
Terminates data playback access to the most recently opened BSS database.

### bssStop

```c
void bssStop()
```

Terminates the most recently initiated BSS data acquisition background thread.

### bssExit

```c
void bssExit()
```

A convenience function that performs both bssClose() and bssStop().

### bssRead

```c
long bssRead(bssNav nav, char *data, int dataLen)
```

Copies the data at the current playback position in the database, as indicated by nav, into data; if the length of the data is in excess of dataLen then an error condition is asserted (i.e., -1 is returned). Note that bssRead() cannot be successfully called until nav has been populated, nominally by a preceding call to bssSeek(), bssNext(), or bssPrev(). Returns the length of data read, or -1 on any error.

### bssSeek

```c
long bssSeek(bssNav *nav, time_t time, time_t *curTime, unsigned long *count)
```

Sets the current playback position in the database, in nav, to the data received in the bundle with the earliest creation time that was greater than or equal to time. Populates nav and also returns the creation time and bundle ID count of that bundle in curTime and count. Returns the length of data at this location, or -1 on any error.

### bssSeek_read

```c
long bssSeek_read(bssNav *nav, time_t time, time_t *curTime, unsigned long *count, char *data, int dataLen)
```

A convenience function that performs bssSeek() followed by an immediate bssRead() to return the data at the new playback position. Returns the length of data read, or -1 on any error.

### bssNext

```c
long bssNext(bssNav *nav, time_t *curTime, unsigned long *count)
```

Sets the playback position in the database, in nav, to the data received in the bundle with the earliest creation time and ID count greater than that of the bundle at the current playback position. Populates nav and also returns the creation time and bundle ID count of that bundle in curTime and count. Returns the length of data at this location (if any), -2 on reaching end of list, or -1 on any error.

### bssNext_read

```c
long bssNext_read(bssNav *nav, time_t *curTime, unsigned long *count, char *data, int dataLen)
```

A convenience function that performs bssNext() followed by an immediate bssRead() to return the data at the new playback position. Returns the length of data read, -2 on reaching end of list, or -1 on any error.

### bssPrev

```c
long bssPrev(bssNav *nav, time_t *curTime, unsigned long *count)
```

Sets the playback position in the database, in nav, to the data received in the bundle with the latest creation time and ID count earlier than that of the bundle at the current playback position. Populates nav and also returns the creation time and bundle ID count of that bundle in curTime and count. Returns the length of data at this location (if any), -2 on reaching end of list, or -1 on any error.

### bssPrev_read

```c
long bssPrev_read(bssNav *nav, time_t *curTime, unsigned long *count, char *data, int dataLen)
```

A convenience function that performs bssPrev() followed by an immediate bssRead() to return the data at the new playback position. Returns the length of data read, -2 on reaching end of list, or -1 on any error

----------------------

## Asynchronous Messaging Service (AMS) APIs

_This section is under construction._

----------------------

## Delay-Tolerant Payload Conditioning (DTPC) communications library

### Description

The dtpc library provides functions enabling application software to use
Delay-Tolerant Payload Conditioning (DTPC) when exchanging information over a
delay-tolerant network.  DTPC is an application service protocol, running in
a layer immediately above Bundle Protocol, that offers delay-tolerant support
for several end-to-end services to applications that may require them.  These
services include delivery of application data items in transmission (rather
than reception) order; detection of reception gaps in the sequence of
transmitted application data items, with end-to-end negative acknowledgment
of the missing data; end-to-end positive acknowledgment of successfully
received data; end-to-end retransmission of missing data, driven either by
negative acknowledgment or timer expiration; suppression of duplicate
application data items; aggregation of small application data items into
large bundle payloads, to reduce bundle protocol overhead; and
application-controlled elision of redundant data items in aggregated payloads,
to improve link utiliization.

### DTPC APIs

```c
int dptc_attach( )
```
Attaches the application to DTPC functionality on the local computer.  Returns
0 on success, -1 on any error.

```c
void dptc_detach( )
```

Terminates all access to DTPC functionality on the local computer.

```c
int dtpc_entity_is_started( )
```
Returns 1 if the local DTPC entity has been started and not yet stopped,
0 otherwise.

```c
int dtpc_open(unsigned int topicID, DtpcElisionFn elisionFn, DtpcSAP *dtpcsapPtr)
```

Establishes the application as the sole authorized client for posting and
receiving application data items on topic _topicID_ within the local BP
node.  On success, the service access point for posting and receiving such
data items is placed in _*dtpcsapPtr_, the elision callback function
_elisionFn_ (if not NULL) is associated with this topic, and 0 is returned.
Returns -1 on any error.

```c
int dtpc_send(unsigned int profileID, DtpcSAP sap, char *destEid, unsigned int maxRtx, unsigned int aggrSizeLimit, unsigned int aggrTimeLimit, int lifespan, BpAncillaryData *ancillaryData, unsigned char srrFlags, BpCustodySwitch custodySwitch, char *reportToEid, int classOfService, Object item, unsigned int length)
```
Inserts an application data item into an outbound DTPC application data unit
destined for _destEid_.

Transmission of that outbound ADU will be subject to the profile identified
by _profileID_, as asserted by dtpcadmin(1), if _profileID_ is non-zero.  In
that case, _maxRtx_, _aggrSizeLimit_, _aggrTimeLimit_, _lifespan_,
_ancillaryData_, _srrFlags_, _custodySwitch_, _reportToEid_, and
_classOfService_ are ignored.

If _profileID_ is zero then the profile asserted by dtpcadmin(1) that matches
_maxRtx_, _aggrSizeLimit_, _aggrTimeLimit_, _lifespan_, _ancillaryData_,
_srrFlags_, _custodySwitch_, _reportToEid_, and _classOfService_ will
govern transmission of the ADU, unless no such profile has been asserted,
in which case dtpc_send() returns 0 indicating user error.

_maxRtx_ is the maximum number of times any single DTPC ADU transmitted
subject to the indicated profile may be retransmitted by the DTPC entity.  If
_maxRtx_ is zero, then the DTPC transport service features (in-order delivery,
end-to-end acknowledgment, etc.) are disabled for this profile.

_aggrSizeLimit_ is the size threshold for concluding aggregation of an
outbound ADU and requesting transmission of that ADU.  If _aggrSizeLimit_ is
zero, then the DTPC transmission optimization features (aggregation and
elision) are disabled for this profile.

_aggrTimeLimit_ is the time threshold for concluding aggregation of an
outbound ADU and requesting transmission of that ADU.  If _aggrTimeLimit_ is
zero, then the DTPC transmission optimization features (aggregation and
elision) are disabled for this profile.

_lifespan_, _ancillaryData_, _srrFlags_, _custodySwitch_, _reportToEid_,
and _classOfService_ are as defined for bp_send (see bp(3)).

_item_ must be an object allocated within ION's SDR "heap", and _length_
must be the length of that object.  The item will be inserted into the outbound
ADU's list of data items posted for the topic associated with _sap_, and
the elision callback function declared for _sap_ (if any, and if the
applicable profile does not disable transmission optimization features)
will be invoked immediately after insertion of the application data item
but before DTPC makes any decision on whether or not to initiate transmission
of the outbound ADU.

The function returns 1 on success, 0 on any user application error, -1 on
any system error.

```c
int dtpc_receive(DtpcSAP sap, DtpcDelivery *dlvBuffer, int timeoutSeconds)
```
Receives a single DTPC application data item, or reports on some failure of
DTPC reception activity.

The "result" field of the dlvBuffer structure will be used to indicate the
outcome of the data reception activity.

If at least one application data item on the topic associated with _sap_
has not yet been delivered to the SAP, then the payload of the oldest such
item will be returned in _dlvBuffer_-__item_ and _dlvBuffer_-__result_
will be set to PayloadPresent.  If there is no such item, dtpc_receive()
blocks for up to _timeoutSeconds_ while waiting for one to arrive.

If _timeoutSeconds_ is DTPC_POLL (i.e., zero) and no application data item is
awaiting delivery, or if _timeoutSeconds_ is greater than zero but no item
arrives before _timeoutSeconds_ have elapsed, then _dlvBuffer_-__result_
will be set to ReceptionTimedOut.  If _timeoutSeconds_ is DTPC_BLOCKING
(i.e., -1) then bp_receive() blocks until either an item arrives or the
function is interrupted by an invocation of dtpc_interrupt().

_dlvBuffer_-__result_ will be set to ReceptionInterrupted in the event
that the calling process received and handled some signal other than SIGALRM
while waiting for a bundle.

_dlvBuffer_-__result_ will be set to DtpcServiceStopped in the event
that DTPC service has been terminated on the local node.

The application data item delivered in the DTPC delivery structure, if
any, will be an object allocated within ION's SDR "heap"; the length of
that object will likewise be provided in the DtpcDelivery structure.

Be sure to call dtpc_release_delivery() after every successful invocation of
dtpc_receive().

The function returns 0 on success, -1 on any error.

```c
void dtpc_interrupt(DtpcSAP sap)
```
Interrupts a dtpc_receive() invocation that is currently blocked.  This
function is designed to be called from a signal handler; for this purpose,
_sap_ may need to be obtained from a static variable.

```c
void dtpc_release_delivery(DtpcDelivery *dlvBuffer)
```
Releases resources allocated to the indicated DTPC delivery.

```c
void dtpc_close(DtpcSAP sap)
```
Removes the application as the sole authorized client for posting and
receiving application data items on the topic indicated in _sap_ within the
local BP node.  The application relinquishes its ability to send and
receive application data items on the indicated topic.