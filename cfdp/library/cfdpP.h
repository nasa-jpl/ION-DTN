/*
 *	cfdpP.h:	private definitions supporting the implementation
 *			of CFDP (CCSDS File Delivery Protocol) entities.
 *
 *
 *	Copyright (c) 2009, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "lyst.h"
#include "zco.h"
#include "crc.h"
#include "cfdp.h"

#ifndef _CFDPP_H_
#define _CFDPP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	CFDP_MAX_PDU_SIZE	65535

typedef struct
{
	Object		text;
	unsigned char	length;
} MsgToUser;

typedef struct
{
	CfdpAction	action;
	Object		firstFileName;		/*	sdrstring	*/
	Object		secondFileName;		/*	sdrstring	*/
} FilestoreRequest;

typedef struct
{
	CfdpAction	action;
	unsigned int	status;			/*	per table 5-18	*/
	Object		firstFileName;		/*	sdrstring	*/
	Object		secondFileName;		/*	sdrstring	*/
	Object		message;		/*	sdrstring	*/
} FilestoreResponse;

typedef struct
{
	CfdpEventType		type;
	time_t			time;
	int			reqNbr;
	CfdpTransactionId	transactionId;
	Object			sourceFileName;	/*	sdrstring	*/
	Object			destFileName;	/*	sdrstring	*/
	uvast			fileSize;
	Object			messagesToUser;		/*	MdList	*/
	uvast			offset;
	unsigned int		length;
	unsigned int		recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		segMetadataLength;
	char			segMetadata[63];
	CfdpCondition		condition;
	uvast			progress;
	CfdpDeliveryCode	deliveryCode;
	CfdpFileStatus		fileStatus;
	CfdpTransactionId	originatingTransactionId;
	Object			statusReport;	/*	sdrstring	*/
	Object			filestoreResponses;	/*	MdList	*/
} CfdpEvent;

typedef enum
{
	FduActive = 0,
	FduSuspended,
	FduCanceled
} FduState;

typedef struct
{
	uvast			offset;
	unsigned int		length;
	CfdpContinuationState	continuationState;
	unsigned int		metadataLength;
	Object			metadata;
} FileDataPdu;

typedef struct
{
	time_t			deadline;
	Object			fdu;	
} FinishPending;

typedef struct
{
	CfdpTransactionId	transactionId;
	CfdpNumber		destinationEntityNbr;
	CfdpCksumType		ckType;
	unsigned char		utParms[128];
	int			utParmsLength;
	int			reqNbr;		/*	Creation req.	*/
	CfdpTransactionId	originatingTransactionId;
	char			sourceFileName[256];
	unsigned int		recordBoundsRespected;	/*	Boolean.*/
	unsigned int		closureLatency;		/*	Seconds.*/
	unsigned int		finishReceived;		/*	Boolean.*/

	/*	File Delivery Unit transmission status			*/

	FduState		state;
	CfdpHandler		faultHandlers[16];
	Object			metadataPdu;	/*	bytes		*/
	unsigned int		mpduLength;	/*	in bytes	*/
	uvast			fileSize;	/*	in bytes	*/
	unsigned int		largeFile;	/*	Boolean		*/
	uvast			progress;	/*	bytes issued	*/
	unsigned int		transmitted;	/*	Boolean		*/
	Object			fileRef;	/*	ZCO file ref	*/
	Object			fileDataPdus;	/*	sdrlist		*/
	Object			eofPdu;		/*	bytes		*/
	unsigned int		epduLength;	/*	in bytes	*/
	Object			closureElt;	/*	in sdrlist	*/
} OutFdu;

/*	Each CfdpExtent in "extents" indicates a range of bytes of file
 *	data received so far in the course of receiving this FDU.  The
 *	extents list in the InFdu is managed in ascending offset
 *	order.  The arrival of a file data PDU creates a CfdpExtent
 *	if necessary but merely increases the length of an existing
 *	CfdpExtent if possible.  When the arrival of a file data PDU
 *	results in the length of one CfdpExtent being extended to
 *	equal or exceed the offset of the next, the two CfdpExtent
 *	objects are combined into a single CfdpExtent whose offset
 *	is the offset of the earlier extent and whose length is
 *	the sum of the offset and length of the later extent.  The
 *	"progress" of an InFdu is the sum of the offset and length
 *	of the last extent in the list.					*/

typedef struct
{
	Object			pdu;		/*	bytes		*/
	unsigned int		length;		/*	in bytes	*/
	int			largeFile;
	int			entityNbrLength;
	int			transactionNbrLength;
	CfdpTransactionId	transactionId;
} FinishPdu;

typedef struct
{
	CfdpTransactionId	transactionId;

	/*	File Delivery Unit metadata				*/

	Object			sourceFileName;	/*	sdrstring	*/
	Object			destFileName;	/*	sdrstring	*/
	unsigned int		closureRequested;	/*	Boolean	*/
	CfdpHandler		faultHandlers[16];
	int			flowLabelLength;
	Object			flowLabel;
	Object			messagesToUser;		/*	sdrlist	*/
	Object			filestoreRequests;	/*	sdrlist	*/

	/*	File reception status					*/

	FduState		state;
	unsigned int		metadataReceived;	/*	Boolean	*/
	unsigned int		eofReceived;		/*	Boolean	*/
	CfdpCondition		eofCondition;
	CfdpNumber		eofFaultLocation;
	unsigned int		eofChecksum;
	CfdpCksumType		ckType;
	unsigned int		computedChecksum;
	int			checksumVerified;
	CfdpCondition		finishCondition;
	uvast			fileSize;
	Object			workingFileName;/*	sdrstring	*/
	uvast			progress;
	time_t			checkTime;
	int			checkTimeouts;
	uvast			bytesReceived;
	Object			extents;		/*	sdrlist	*/
	time_t			inactivityDeadline;
} InFdu;

typedef enum
{
	UtBp = 1,
	UtLtp = 2,
	UtTcp = 3
} UtLayer;

typedef struct
{
	uvast			entityId;
	char			protocolName[32];
	char			endpointName[256];
	UtLayer			utLayer;
	uvast			bpNodeNbr;
	uvast			ltpEngineNbr;
	unsigned int		ipAddress;
	unsigned short		portNbr;
	unsigned int		ackTimerInterval;
	CfdpCksumType		inCkType;
	CfdpCksumType		outCkType;
	Object			inboundFdus;	/*	sdrlist: InFdu	*/
} Entity;

typedef struct
{
	CfdpTransactionId	originatingTransactionId;
	CfdpNumber		proxyDestinationEntityNbr;
	char			proxySourceFileName[256];
	char			proxyDestFileName[256];
	Object			proxyMsgsToUser;	/*	sdrlist	*/
	Object			proxyFilestoreRequests;	/*	sdrlist	*/
	CfdpHandler		proxyFaultHandlers[16];
	unsigned int		proxyUnacknowledged;	/*	Boolean	*/
	int			proxyFlowLabelLength;
	unsigned char		proxyFlowLabel[256];
	unsigned int		proxyRecordBoundsRespected;/*	Boolean	*/
	unsigned int		proxyClosureRequested;	/*	Boolean	*/
	CfdpCondition		proxyCondition;
	CfdpDeliveryCode	proxyDeliveryCode;
	CfdpFileStatus		proxyFileStatus;
	Object			proxyFilestoreResponses;/*	sdrlist	*/
	char			directoryName[256];
	char			directoryDestFileName[256];
	int			directoryListingResponseCode;
} CfdpUserOpsData;

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	uvast		ownEntityId;
	CfdpNumber	ownEntityNbr;
	int		requestCounter;
	unsigned int	transactionCounter;
	unsigned int	maxTransactionNbr;
	unsigned char	fillCharacter;
	unsigned short	discardIncompleteFile;		/*	Boolean	*/
	unsigned short	crcRequired;			/*	Boolean	*/
	unsigned short	maxFileDataLength;
	unsigned int	transactionInactivityLimit;
	unsigned int	checkTimerPeriod;
	unsigned int	checkTimeoutLimit;
	unsigned int	maxQueuedEvents;
	CfdpHandler	faultHandlers[16];

	/*	Fault handlers table is indexed by transaction
	 *	condition code as represented by CfdpCondition.		*/

	Object		usrmsgLists;	/*	SDR list: MetadataList	*/
	Object		fsreqLists;	/*	SDR list: MetadataList	*/
	Object		fsrespLists;	/*	SDR list: MetadataList	*/
	Object		outboundFdus;	/*	SDR list: OutFdu	*/
	Object		events;		/*	SDR list: CfdpEvent	*/
	Object		entities;	/*	SDR list: Entity	*/
	Object		finishPdus;	/*	SDR list: FinishPdu	*/
	Object		finsPending;	/*	SDR list: FinishPending	*/
} CfdpDB;

/*	The volatile database object encapsulates the current volatile
	state of the database.						*/

/*	"Watch" switches for CFDP operation.				*/
#define WATCH_p			(1)
#define WATCH_q			(2)

typedef struct
{
	int		utaPid;		/*	For stopping the UTA.	*/
	int		clockPid;	/*	For stopping cfdpclock.	*/
	int		watching;	/*	Activity watch.		*/
	sm_SemId	eventSemaphore;

	/*	The fduSemaphore of the CFDP entity is given whenever
	 *	a new OutFdu is inserted or a suspended OutFdu is
	 *	resumed (or a Finished PDU is inserted upon completion
	 *	of an FDU reception).  The cfdpDequeueOutboundPDU
	 *	function takes this semaphore when it detects that
	 *	no OutFdu in the outboundFdus list is in Active state
	 *	(i.e., all of the OutFdus in the list are either
	 *	Suspended or Canceled) and the queue of outbound
	 *	Finished PDUs is empty.					*/

	sm_SemId	fduSemaphore;

	/*	currentFdu identifies the FDU that is currently being
	 *	reassembled, if any.  currentFile is the FD that is
	 *	being used to reassemble that FDU.			*/

	Object		currentFdu;
	int		currentFile;

	/*	The "attendant" of the CFDP entity is a coordination
	 *	object used in flow control of ZCO space allocation.
	 *	Since the entity has only a single queue of outbound
	 *	FDUs, only one UTA thread will be creating CFDP PDU
	 *	ZCOs and therefore only a single attendant is needed.	*/

	ReqAttendant	attendant;

	/*	FOR TESTING ONLY: if the environment value named
	 *	CFDP_CORRUPTION_MODULUS exists and is a positive
	 *	integer greater than zero, then its value is stored
	 *	in corruptionModulus at the time CFDP is initialized.
	 *	If corruptionModulus is the non-zero value N, then
	 *	each time the writeSegmentData function is called
	 *	it uses rand() to get a randomly selected integer
	 *	and checks the remainder obtained by dividing that
	 *	integer by N; if the remainder is zero, then the
	 *	first byte of data passed to this function is
	 *	corrupted (increased by 1) before it is written.
	 *	This enables the checksum check function to be
	 *	exercised in testing.					*/

	unsigned int	corruptionModulus;
} CfdpVdb;

extern int		cfdpInit();
extern void		cfdpDropVdb();
extern void		cfdpRaiseVdb();
#define cfdpStart(cmd)	_cfdpStart(cmd)
extern int		_cfdpStart(char *utiCmd);
#define cfdpStop()	_cfdpStop()
extern void		_cfdpStop();
extern int		cfdpAttach();
extern void		cfdpDetach();

extern void		cfdpScrub();

extern Object		getCfdpDbObject();
extern CfdpDB		*getCfdpConstants();
extern CfdpVdb		*getCfdpVdb();

extern Object		findEntity(uvast entityId, Entity *entity);
extern Object		addEntity(uvast entityId, char *protocolName,
				char *endpointName, unsigned int rtt,
				unsigned int inCkType, unsigned int outCkType);
extern int		changeEntity(uvast entityId, char *protocolName,
				char *endpointName, unsigned int rtt,
				unsigned int inCkType, unsigned int outCkType);
extern int		removeEntity(uvast entityId);

extern int		checkFile(char *);

extern int		ckTypeOkay(unsigned int ckType);
extern void		addToChecksum(unsigned char octet, vast *offset,
				unsigned int *checksum, CfdpCksumType ckType);

extern int		getReqNbr();	/*	Returns next req nbr.	*/

extern MetadataList	createMetadataList(Object log);
extern void		destroyUsrmsgList(MetadataList *list);
extern void		destroyFsreqList(MetadataList *list);
extern void		destroyFsrespList(MetadataList *list);

extern Object		findOutFdu(CfdpTransactionId *id, OutFdu *fdu,
				Object *elt);
extern int		suspendOutFdu(CfdpTransactionId *id, CfdpCondition c,
				int reqNbr);
extern int		cancelOutFdu(CfdpTransactionId *id, CfdpCondition c,
				int reqNbr);
extern void		destroyOutFdu(OutFdu *fdu, Object fduObj,
				Object fduElt);

extern Object		findInFdu(CfdpTransactionId *id, InFdu *fdu,
				Object *elt, int createIfNotFound);
extern int		completeInFdu(InFdu *fdu, Object fduObj, Object fduElt,
				CfdpCondition c, int reqNbr);

extern int		enqueueCfdpEvent(CfdpEvent *event);

extern int		handleFault(CfdpTransactionId *id, CfdpCondition c,
				CfdpHandler *handler);

extern int		cfdpDequeueOutboundPdu(Object *pdu, OutFdu *fduBuffer,
				FinishPdu *fpdu, int *direction);
extern int		cfdpHandleInboundPdu(unsigned char *buf, int length);

#ifdef __cplusplus
}
#endif

#endif
