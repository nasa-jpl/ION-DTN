/*
 *	cfdp.h:	definitions supporting the implementation of CFDP
 *		(CCSDS File Delivery Protocol) application software.
 *
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */
#include "ion.h"

#ifndef _CFDP_H_
#define _CFDP_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	CFDP_STD_READER
#define CFDP_STD_READER	NULL
#endif

typedef enum
{
	CksumTypeUnknown = -1,
	ModularChecksum = 0,
	CRC32CChecksum = 2,
	NullChecksum = 15
} CfdpCksumType;

typedef Object		MetadataList;	/*	SDR list		*/

/*	A MetadataList is an SDR list (of user messages, filestore
 *	requests, or filestore responses) that is used to simplify
 *	the interface between CFDP and the user application without
 *	risk of space leak.  During the time that a MetadataList is
 *	pending processing via the CFDP API, but is not yet (or is
 *	no longer) reachable from any FDU object, a pointer to the
 *	list is appended to one of the lists of MetadataList objects
 *	in the CFDP non-volatile database.  This assures that any
 *	unplanned termination of the CFDP daemons won't leave any
 *	SDR lists unreachable -- and therefore un-recyclable -- due
 *	to the absence of references to those lists.  Restarting
 *	CFDP automatically purges any unused MetadataLists from
 *	the CFDP database.
 *
 *	The mechanism used to implement this feature is the 
 *	"user data" variable of the MetadataList itself.  While
 *	the list is reachable only from the database root, its
 *	user data variable points to the database root list from
 *	which it is referenced.  While the list is attached to a
 *	File Delivery Unit, its user data is null.			*/

typedef struct
{
	unsigned int	length;
	unsigned char	buffer[8];	/*	Right-justified value.	*/
} CfdpNumber;

typedef struct
{
	CfdpNumber	sourceEntityNbr;
	CfdpNumber	transactionNbr;
} CfdpTransactionId;

/*	File data segment production always proceeds on the assumption
 *	that the first byte of the first application-significant
 *	"record" in the file to be transmitted is the first byte of
 *	that file.  For the purpose of file data segment production,
 *	the file is opened (setting the current file position to the
 *	start of the file) and all records in the file are identified.
 *
 *	The user application may optionally provides a pointer to a
 *	"reader function".  This function, given an FD opened for the
 *	file that is to be transmitted, is required to read forward
 *	from the current file position to the start of the next record
 *	in the file (and beyond it as necessary) and return the length
 *	of the current record.  It is also required to update the
 *	computed checksum for the file by passing each octet of the
 *	current record to the cfdp_update_checksum() function, along
 *	with the checksum type that is passed to the reader function.
 *
 *	In the absence of a specified reader function, the default
 *	reader function simply returns CFDP_MAX_FILE_DATA or the
 *	total remaining length of the file, whichever is less.		*/

typedef int	(*CfdpReaderFn)(int fd, unsigned int *checksum,
			CfdpCksumType ckType);

/*	File data segment continuation state provides information
 *	about the record structure of the file.				*/

typedef enum
{
	CfdpNoBoundary = 0,
	CfdpStartOfRecord = 1,
	CfdpEndOfRecord = 2,
	CfdpEntireRecord = 3
} CfdpContinuationState;

/*	Per-segment metadata may be provided by the user application.
 *	To enable this, upon formation of each file data segment, CFDP
 *	will invoke the user-provided per-segment metadata composition
 *	callback function (if any).  The callback will be passed the
 *	offset of the segment within the file, the segment's offset
 *	within the current record (as applicable), the length of the
 *	segment, an open file descriptor for the source file (in case
 *	the data must be read in order to construct the metadata), and
 *	a 63-byte buffer in which to place the new metadata.  The
 *	callback function must return the length of metadata to attach
 *	to the file data segment PDU (may be zero) or -1 in the event
 *	of a general system failure.					*/

typedef int	(*CfdpMetadataFn)(uvast fileOffset, unsigned int recordOffset,
			unsigned int length, int sourceFileFD, char *buffer);

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
	CfdpProxyPutRequest = 0,
	CfdpProxyMsgToUser,
	CfdpProxyFilestoreRequest,
	CfdpProxyFaultHandlerOverride,
	CfdpProxyTransmissionMode,
	CfdpProxyFlowLabel,
	CfdpProxySegmentationControl,
	CfdpProxyPutResponse,
	CfdpProxyFilestoreResponse,
	CfdpProxyPutCancel,
	CfdpOriginatingTransactionId,
	CfdpProxyClosureRequest,
	CfdpDirectoryListingRequest = 16,
	CfdpDirectoryListingResponse
} CfdpUserMsgType;

typedef enum
{
	CfdpNoError = 0,			/*	Not a fault.	*/
	CfdpAckLimitReached,
	CfdpKeepaliveLimitReached,
	CfdpInvalidTransmissionMode,
	CfdpFilestoreRejection,
	CfdpChecksumFailure,
	CfdpFileSizeError,
	CfdpNakLimitReached,
	CfdpInactivityDetected,
	CfdpInvalidFileStructure,
	CfdpCheckLimitReached,
	CfdpUnsupportedChecksumType,
	CfdpSuspendRequested = 14,		/*	Not a fault.	*/
	CfdpCancelRequested			/*	Not a fault.	*/
} CfdpCondition;

typedef enum
{
	CfdpNoHandler = 0,
	CfdpCancel,
	CfdpSuspend,
	CfdpIgnore,
	CfdpAbandon
} CfdpHandler;

typedef enum
{
	CfdpAccessEnded = -1,
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

typedef enum
{
	CfdpFileDiscarded = 0,
	CfdpFileRejected,
	CfdpFileRetained,
	CfdpFileStatusUnreported
} CfdpFileStatus;

typedef enum
{
	CfdpDataComplete = 0,
	CfdpDataIncomplete
} CfdpDeliveryCode;

typedef struct
{
	uvast	offset;
	uvast	length;
} CfdpExtent;

/*	*	*	CFDP initialization	*	*	*	*/

extern int	cfdp_attach();
extern void	cfdp_detach();

extern int	cfdp_entity_is_started();
		/*	Returns 1 if the local CFDP entity has been
		 *	started and not yet stopped, 0 otherwise.	*/

/*	*	*	CFDP utility functions	*	*	*	*/

extern void	cfdp_compress_number(CfdpNumber *toNbr, uvast from);
extern void	cfdp_decompress_number(uvast *toNbr, CfdpNumber *from);

extern void	cfdp_update_checksum(unsigned char octet,
			vast		*offset,
			unsigned int	*checksum,
			CfdpCksumType	ckType);
extern
MetadataList	cfdp_create_usrmsg_list();
extern int	cfdp_add_usrmsg(MetadataList list,
			unsigned char	*text,
			int		length);
extern int	cfdp_get_usrmsg(MetadataList *list,
			unsigned char	*textBuf,
			int		*length);
extern void	cfdp_destroy_usrmsg_list(MetadataList *list);
extern
MetadataList	cfdp_create_fsreq_list();
extern int	cfdp_add_fsreq(MetadataList list,
			CfdpAction	action,
			char		*firstFileName,
			char		*secondFileName);
extern int	cfdp_get_fsreq(MetadataList *list,
			CfdpAction	*action,
			char		*firstFileNameBuf,
			char		*secondFileNameBuf);
extern void	cfdp_destroy_fsreq_list(MetadataList *list);
extern int	cfdp_get_fsresp(MetadataList *list,
			CfdpAction	*action,
			int		*status,
			char		*firstFileNameBuf,
			char		*secondFileNameBuf,
			char		*messageBuf);
extern void	cfdp_destroy_fsresp_list(MetadataList *list);

extern char	*cfdp_working_directory();

/*	*	*	CFDP local services	*	*	*	*/

/*	Here are two standard CfdpReader functions.			*/
extern int	cfdp_read_space_packets(int fd, unsigned int *checksum,
			CfdpCksumType ckType);
extern int	cfdp_read_text_lines(int fd, unsigned int *checksum,
			CfdpCksumType ckType);

extern int	cfdp_put(CfdpNumber	*destinationEntityNbr,
			unsigned int	utParmsLength,
			unsigned char	*utParms,
			char		*sourceFileName,
			char		*destFileName,
			CfdpReaderFn	readerFn,
			CfdpMetadataFn	metadataFn,
			CfdpHandler	*faultHandlers,	/*	array	*/
			unsigned int	flowLabelLength,
			unsigned char	*flowLabel,
			unsigned int	closureLatency,
			MetadataList	messagesToUser,
			MetadataList	filestoreRequests,
			CfdpTransactionId *transactionId);
		/*	Returns request number on success, -1 on
			system failure.  On application error, returns
			request number zero.				*/

extern int	cfdp_cancel(CfdpTransactionId *transactionId);
		/*	Returns request number on success, -1 on
			system failure.  On application error, returns
			request number zero.				*/

extern int	cfdp_suspend(CfdpTransactionId *transactionId);
		/*	Returns request number on success, -1 on
			system failure.  On application error, returns
			request number zero.				*/

extern int	cfdp_resume(CfdpTransactionId *transactionId);
		/*	Returns request number on success, -1 on
			system failure.  On application error, returns
			request number zero.				*/

extern int	cfdp_report(CfdpTransactionId *transactionId);
		/*	Returns request number on success, -1 on
			system failure.  On application error, returns
			request number zero.				*/

/*	*	*	CFDP event handling	*	*	*	*/

extern int	cfdp_get_event(CfdpEventType	*type,
			time_t			*time,
			int			*reqNbr,
			CfdpTransactionId	*transactionId,
			char			*sourceFileNameBuf,
			char			*destFileNameBuf,
			uvast			*fileSize,
			MetadataList		*messagesToUser,
			uvast			*offset,
			unsigned int		*length,
			unsigned int		*recordBoundsRespected,
			CfdpContinuationState	*continuationState,
			unsigned int		*segMetadataLength,
			char			*segMetadataBuffer,
			CfdpCondition		*condition,
			uvast			*progress,
			CfdpFileStatus		*fileStatus,
			CfdpDeliveryCode	*deliveryCode,
			CfdpTransactionId	*originatingTransactionId,
			char			*statusReportBuf,
			MetadataList		*filestoreResponses);
		/*	Populates return value fields with data
		 *	from the oldest CFDP event not yet delivered
		 *	to the application.  On application error,
		 *	returns 0 but sets errno to EINVAL.  When no
		 *	undelivered event is pending, blocks until
		 *	an event is posted -- unless interrupted,
		 *	in which case the function returns 0 and
		 *	sets *type to CfdpNoEvent.  On system failure,
		 *	returns -1.  Otherwise returns 0.		*/

extern void	cfdp_interrupt();
		/*	cfdp_get_event always blocks; to interrupt it,
		 *	call cfdp_interrupt.				*/

extern int	cfdp_preview(CfdpTransactionId	*transactionId,
			uvast			offset,
			unsigned int		length,
			char			*buffer);
		/*	Reads "length" bytes starting at "offset"
		 *	bytes from the start of the file that is
		 *	the destination file of the transaction
		 *	identified by "transactionID", into "buffer".
		 *	On user error (transaction is nonexistent or
		 *	is outbound, or offset is beyond the end of
		 *	file) returns 0.  On system failure, returns
		 *	-1.  Otherwise returns number of bytes read.	*/

extern int	cfdp_map(CfdpTransactionId	*transactionId,
			unsigned int		*extentCount,
			CfdpExtent		*extentsArray);
		/*	Lists the received continuous data extents
		 *	in the destination file of the transaction
		 *	identified by "transactionID".  The extents
		 *	(offset and length) are returned in the
		 *	elements of "extentsArray"; the number
		 *	of extents returned in the array is the
		 *	total number of continuous extents received
		 *	so far, or "extentCount", whichever is less.
		 *	The total number of extents received so
		 *	far is returned as the new value of
		 *	"extentCount."  On user error (transaction
		 *	is nonexistent or is outbound), returns 0.
		 *	On system failure, returns -1.  Otherwise
		 *	returns 1.					*/

#include "cfdpops.h"

#ifdef __cplusplus
}
#endif

#endif
