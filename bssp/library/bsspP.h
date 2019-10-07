/*
 *	bsspP.h:	private definitions supporting the implementation
 *			of BSSP (Bundle Streaming Service Protocol) engines.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *				
 *	Author: Sotirios-Angelos Lenas, Space Internetworking Center
 */

#include "rfx.h"
#include "lyst.h"
#include "smlist.h"
#include "zco.h"
#include "bssp.h"
#include "sdrhash.h"

#ifndef _BSSPP_H_
#define _BSSPP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BSSP_MAX_NBR_OF_CLIENTS	8
#ifndef BSSP_MEAN_SEARCH_LENGTH
#define	BSSP_MEAN_SEARCH_LENGTH	4
#endif
#define	MAX_BSSP_CLIENT_NBR	(BSSP_MAX_NBR_OF_CLIENTS - 1)

/*	BSSP block structure definitions.				*/

#define	BSSP_ACK_FLAG		0x01

typedef enum
{
	BsspDs = 0,
	BsspAck
} BsspBlkTypeCode;

typedef enum
{
	BsspTimerSuspended=0,
	BsspTimerRunning
} BsspTimerState;

typedef struct
{
	time_t			pduArrivalTime;
	time_t			ackDeadline;
	BsspTimerState		state;
} BsspTimer;

typedef enum
{
	BsspCancelByUser = 0,
	//BsspClientSvcUnreachable,	//Note, I'm not sure if we actually need something like this
	BsspRetransmitLimitExceeded,
	BsspCancelByEngine
} BsspCancelReasonCode;

typedef struct
{
	BsspBlkTypeCode	blkTypeCode;

	/*	Fields used for multiple segment classes.		*/

	BsspTimer		timer;	/*	Checkpoint or report.	*/

	/*	Fields for data blocks.					*/

	unsigned int		clientSvcId;	/*	Destination.	*/
	unsigned int		length;
	Object			svcData;/*	Session svcDataObjects.	*/

	/*	Fields for management blocks.				*/

	BsspCancelReasonCode	reasonCode;
} BsspPdu;

typedef enum
{
	BsspData,
	BsspAckn
} BsspPduClass;

/*	An BsspRecvBlk encapsulates a block that has been acquired
 *	by the local BSSP engine, for handling.
 */

typedef struct
{
	Object		sessionObj;
	Object		sessionListElt;
	BsspPduClass	pduClass;
	BsspPdu		pdu;
} BsspRecvBlk;

/*	An BsspXmitBlock encapsulates a block that has been produced
 *	by the local BSSP engine, for transmission.
 */

typedef struct
{
	unsigned int	sessionNbr;
	uvast		remoteEngineId;
	short		ohdLength;
	Object		queueListElt;
	Object		sessionObj;
	BsspPduClass	pduClass;
	BsspPdu		pdu;
} BsspXmitBlock;

/* Session structures */

typedef struct
{
	Object		span;		/*	Transmission span.	*/
	unsigned int	sessionNbr;	/*	Assigned by self.	*/
	Sdnv		sessionNbrSdnv;
	unsigned int	clientSvcId;
	Sdnv		clientSvcIdSdnv;
	int		totalLength;
	BsspTimer	timer;		/*	For cancellation.	*/
	int		reasonCode;	/*	For cancellation.	*/
	Object		svcDataObject;	/*	ZCO			*/
	Object		block;		/* 	BsspXmitBlock 		*/	
} ExportSession;

/* Timeline event structure */

typedef enum
{
	BsspResendBlock = 1,
} BsspEventType;

typedef struct
{
	uvast		refNbr1;	/*	Engine ID.		*/
	unsigned int	refNbr2;	/*	Session number.		*/
	Object		parm;		/*	Non-specific use.	*/
	time_t		scheduledTime;	/*	Seconds since Jan 1970.	*/
	BsspEventType	type;
} BsspEvent;

/* Span structure characterizing the communication span between the
 * local engine and some remote engine.  Note that a single BSSP span
 * might be serviced by multiple communication links, e.g., simultaneous
 * S-band and Ka-band transmission. */

typedef struct
{
	uvast		engineId;	/*	ID of remote engine.	*/
	Sdnv		engineIdSdnv;
	unsigned int	remoteQtime;	/*	In seconds.		*/
	unsigned int	maxExportSessions;
	int		purge;		/*	Boolean.		*/
	Object		bsoBECmd;	/*	Starts Best Effort BSO.	*/
	Object		bsoRLCmd;	/*	Starts Reliable BSO.	*/
	unsigned int 	maxBlockSize;	/*	Bytes 			*/
	Object		currentExportSessionObj;
	unsigned int	lengthOfBufferedBlock;
	unsigned int	clientSvcIdOfBufferedBlock;
	Object		exportSessions;
	
	Object		beBlocks;	/*	SDR list of BsspXmitBlocks
						enqueued for best-effort
						transmission		*/
	Object		rlBlocks;	/*	SDR list of BsspXmitBlocks
						enqueued for reliable
						transmission		*/
} BsspSpan;


/* The volatile span object encapsulates the current volatile state
 * of the corresponding BsspSpan. 					*/

typedef struct
{
	Object		spanElt;	/*	Reference to BsspSpan.	*/
	uvast		engineId;	/*	ID of remote engine.	*/
	unsigned int	localXmitRate;	/*	Bytes per second.	*/
	unsigned int	remoteXmitRate;	/*	Bytes per second.	*/
	unsigned int	owltInbound;	/*	In seconds.		*/
	unsigned int	owltOutbound;	/*	In seconds.		*/
	int		bsoBEPid;	/*	Stops best-effort BSO.	*/
	int		bsoRLPid;	/*	Stops reliable BSO.	*/

	/*	*	*	Work area	*	*	*	*/

	PsmAddress	beBuffer;	/*	Holds 1 max-size block.	*/
	PsmAddress	rlBuffer;	/*	Holds 1 max-size block.	*/

	/*	The blockSemaphore of a BsspVspan is given by the
	 *	sendBlock function every time an outbound block is
	 *	enqueued for transmission over this span.  This
	 *	signifies that the BSO task may now obtain a block
	 *	from the BsspSpan's blocks list and transmit it
	 *	-- thus eliminating polling from BSSP transmission
	 *	processing.  The bsspDequeueOutboundBlock function
	 *	takes this semaphore before the BSO task proceeds to
	 *	transmit the block via its link service protocol.	*/

	sm_SemId	bufOpenSemaphore;

	sm_SemId	beSemaphore;	/*	For best-effort xmit of
						outbound blocks.	*/
	sm_SemId	rlSemaphore;	/*	For reliable xmit of
						outbound blocks.	*/
} BsspVspan;

/* Client and notice structures */

typedef struct
{
	BsspSessionId	sessionId;
	unsigned int	dataLength;
	BsspNoticeType	type;
	unsigned char	reasonCode;
	Object		data;		/*	To be serialized.	*/
} BsspNotice;

typedef struct
{
	Object		notices;	/*	SDR list of LtpNotices	*/
} BsspClient;

/* The volatile client object encapsulates the current volatile state
 * of the corresponding LtpClient. 					*/

typedef struct
{
	Object		notices;	/*	Copied from BsspClient.	*/
	int		pid;
	sm_SemId	semaphore;	/*	For notices.		*/
} BsspVclient;

/* Database structure */

typedef struct
{
	uvast		ownEngineId;
	Sdnv		ownEngineIdSdnv;
	BsspClient	clients[BSSP_MAX_NBR_OF_CLIENTS];
	int		estMaxExportSessions;
	unsigned int	ownQtime;
	unsigned int	sessionCount;
	Object		exportSessionsHash;
	
	Object		spans;		/*	SDR list: BsspSpan	*/
	Object		timeline;	/*	SDR list: BsspEvent	*/
			/*	A catalogue that logs all the pairs of
			 *	node-service numbers and the latest
			 *	creation of each pair forwarded by
			 *	BSSP-CL					*/
} BsspDB;

/*	The volatile database object encapsulates the current volatile
 *	state of the database.						*/

/*	"Watch" switches for BSSP operation.				*/
#define WATCH_d			(1)	/*	bssp send completed		*/
#define WATCH_e			(2)	/*	costructDataBlk			*/
#define WATCH_f			(4)	/*	xmitBlock issuance 		*/
#define WATCH_g			(8)	/*	DequeueBEOutboundBlock		*/
#define WATCH_h			(16)	/*	HandleAck			*/
#define WATCH_s			(32)	/*	HandleInbound			*/
#define WATCH_t			(64)	/*	DequeueRLOutboundBlock		*/
#define WATCH_CS		(128)	/* 	cancel Session by Sender	*/
#define WATCH_resendBlk		(256)	/*	bssp resend xmitBlock "="	*/


typedef struct
{
	uvast		ownEngineId;
	int		beBsiPid;	/*	Stops best-effort BSI.	*/
	int		rlBsiPid;	/*	Stops the reliable BSI.	*/
	int		clockPid;	/*	For stopping bsspclock.	*/
	int		watching;	/*	Boolean activity watch.	*/
	PsmAddress	spans;		/*	SM list: BsspVspan*	*/
	BsspVclient	clients[BSSP_MAX_NBR_OF_CLIENTS];
} BsspVdb;

extern int		bsspInit();
extern void		bsspDropVdb();
extern void		bsspRaiseVdb();
extern int		bsspStart();
extern void		bsspStop();
extern int		bsspAttach();
extern void		bsspDetach();

extern Object		getBsspDbObject();
extern BsspDB		*getBsspConstants();
extern BsspVdb		*getBsspVdb();

extern void		findBsspSpan(uvast engineId, BsspVspan **vspan,
				PsmAddress *vspanElt);
extern int		addBsspSpan(uvast engineId, 
				unsigned int maxExportSessions,
				unsigned int maxBlockSize,
				char *bsoBECmd, char *bsoRLCmd, 
				unsigned int qTime, int purge);
extern int		updateBsspSpan(uvast engineId, 
				unsigned int maxExportSessions,
				unsigned int maxBlockSize,
				char *bsoBECmd, char *bsoRLCmd, 
				unsigned int qTime, int purge);
extern int		removeBsspSpan(uvast engineId);

extern int		bsspStartSpan(uvast engineId);
extern void		bsspStopSpan(uvast engineId);

extern int		startBsspExportSession(Sdr sdr, Object spanObj,
				BsspVspan *vspan);

extern int		issueXmitBlock(Sdr sdr, BsspSpan *span,
				BsspVspan *vspan, ExportSession *session,
				Object sessionObj, int inOrder);

extern int		bsspAttachClient(unsigned int clientSvcId);
extern void		bsspDetachClient(unsigned int clientSvcId);

extern int		enqueueBsspNotice(BsspVclient *client,
				uvast sourceEngineId,
				unsigned int sessionNbr,
				unsigned int dataLength,
				BsspNoticeType type,
				unsigned char reasonCode,
				Object data);

extern int		bsspDequeueBEOutboundBlock(BsspVspan *vspan,
				char **buf);
extern int		bsspDequeueRLOutboundBlock(BsspVspan *vspan,
				char **buf);
extern int		bsspHandleInboundBlock(char *buf, int length);

extern void		bsspStartXmit(BsspVspan *vspan);
extern void		bsspStopXmit(BsspVspan *vspan);
extern int		bsspSuspendTimers(BsspVspan *vspan, PsmAddress vspanElt,
				time_t suspendTime, unsigned int priorXmitRate);
extern int		bsspResumeTimers(BsspVspan *vspan, PsmAddress vspanElt,
				time_t resumeTime, unsigned int remoteXmitRate);

extern int		bsspResendBlock(unsigned int sessionNbr);
#ifdef __cplusplus
}
#endif

#endif
