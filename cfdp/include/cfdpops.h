/*
 *	cfdpops.h:	definitions supporting the implementation
 *			of CFDP standard user operations.
 *
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#ifndef CFDPOPS_H
#define CFDPOPS_H

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	CFDP remote services	*	*	*	*/

#ifndef NO_PROXY

typedef struct
{
	char		*sourceFileName;
	char		*destFileName;
	MetadataList	messagesToUser;
	MetadataList	filestoreRequests;
	CfdpHandler	*faultHandlers;			/*	array	*/
	int		unacknowledged;			/*	Boolean	*/
	unsigned int	flowLabelLength;
	unsigned char	*flowLabel;
	int		recordBoundsRespected;		/*	Boolean	*/
	int		closureRequested;		/*	Boolean	*/
} CfdpProxyTask;

extern int	cfdp_rput(CfdpNumber	*respondentEntityNbr,
			unsigned int	utParmsLength,
			unsigned char	*utParms,
			char		*sourceFileName,
			char		*destFileName,
			CfdpReaderFn	readerFn,
			CfdpHandler	*faultHandlers,	/*	array	*/
			unsigned int	flowLabelLength,
			unsigned char	*flowLabel,
			unsigned int	closureLatency,
			MetadataList	messagesToUser,
			MetadataList	filestoreRequests,
			CfdpNumber	*beneficiaryEntityNbr,
			CfdpProxyTask	*proxyTask,
			CfdpTransactionId *transactionId);

extern int	cfdp_rput_cancel(CfdpNumber *respondentEntityNbr,
			unsigned int	utParmsLength,
			unsigned char	*utParms,
			char		*sourceFileName,
			char		*destFileName,
			CfdpReaderFn	readerFn,
			CfdpHandler	*faultHandlers,	/*	array	*/
			unsigned int	flowLabelLength,
			unsigned char	*flowLabel,
			unsigned int	closureLatency,
			MetadataList	messagesToUser,
			MetadataList	filestoreRequests,
			CfdpTransactionId *rputTransactionId,
			CfdpTransactionId *transactionId);

extern int	cfdp_get(CfdpNumber	*respondentEntityNbr,
			unsigned int	utParmsLength,
			unsigned char	*utParms,
			char		*sourceFileName,
			char		*destFileName,
			CfdpReaderFn	readerFn,
			CfdpHandler	*faultHandlers,	/*	array	*/
			unsigned int	flowLabelLength,
			unsigned char	*flowLabel,
			unsigned int	closureLatency,
			MetadataList	messagesToUser,
			MetadataList	filestoreRequests,
			CfdpProxyTask	*retrievalTask,
			CfdpTransactionId *transactionId);
#endif

#ifndef NO_DIRLIST

typedef struct
{
	char		*directoryName;
	char		*destFileName;
} CfdpDirListTask;

/*	Maximum length of a single directory entry name carried in a
 *	v2 directory listing manifest, in bytes (not counting the NUL
 *	terminator).  Comfortably exceeds POSIX NAME_MAX (255) so on
 *	real filesystems this cap is never reached.			*/
#define CFDP_DIRLIST_MAX_NAME		511

/*	listingOptions bit values for cfdp_rls_extended().		*/
#define CFDP_DIRLIST_OPTION_STATUS_BYTES	0x01

/*	Per-entry status bit values in a v2 manifest.			*/
#define CFDP_DIRENT_NAME_TRUNCATED	0x01

/*	Per-entry type values in a v2 manifest.				*/
#define CFDP_DIRENT_UNKNOWN		0
#define CFDP_DIRENT_REGULAR		1
#define CFDP_DIRENT_DIRECTORY		2
#define CFDP_DIRENT_SYMLINK		3
#define CFDP_DIRENT_OTHER		4

extern int	cfdp_rls(CfdpNumber	*respondentEntityNbr,
			unsigned int	utParmsLength,
			unsigned char	*utParms,
			char		*sourceFileName,
			char		*destFileName,
			CfdpReaderFn	readerFn,
			CfdpHandler	*faultHandlers,	/*	array	*/
			unsigned int	flowLabelLength,
			unsigned char	*flowLabel,
			unsigned int	closureLatency,
			MetadataList	messagesToUser,
			MetadataList	filestoreRequests,
			CfdpDirListTask	*dirListTask,
			CfdpTransactionId *transactionId);

/*	cfdp_rls_extended() is identical to cfdp_rls() except that
 *	listingOptions may be used to request the v2 directory
 *	listing format.  Currently the only defined option bit is
 *	CFDP_DIRLIST_OPTION_STATUS_BYTES, which causes each manifest
 *	entry to be prefixed with a one-byte status flag and a
 *	one-byte type indicator, and causes the response to carry an
 *	"incomplete" flag indicating whether the responder dropped
 *	any entries.						*/

extern int	cfdp_rls_extended(CfdpNumber *respondentEntityNbr,
			unsigned int	utParmsLength,
			unsigned char	*utParms,
			char		*sourceFileName,
			char		*destFileName,
			CfdpReaderFn	readerFn,
			CfdpHandler	*faultHandlers,	/*	array	*/
			unsigned int	flowLabelLength,
			unsigned char	*flowLabel,
			unsigned int	closureLatency,
			MetadataList	messagesToUser,
			MetadataList	filestoreRequests,
			CfdpDirListTask	*dirListTask,
			unsigned int	listingOptions,
			CfdpTransactionId *transactionId);
#endif

#ifdef __cplusplus
}
#endif

#endif	/* CFDPOPS_H */
