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

#ifndef _CFDPOPS_H_
#define _CFDPOPS_H_

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
#endif 

#ifdef __cplusplus
}
#endif

#endif	/* _CFDPOPS_H */
