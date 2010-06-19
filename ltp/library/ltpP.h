/*
 *	ltpP.h:		private definitions supporting the implementation
 *			of LTP (Licklider Transmission Protocol) engines.
 *
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "rfx.h"
#include "lyst.h"
#include "smlist.h"
#include "zco.h"
#include "ltp.h"
#include "sdrhash.h"

#ifndef _LTPP_H_
#define _LTPP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LTP_MAX_NBR_OF_CLIENTS	8

#ifndef LTP_MEAN_SEARCH_LENGTH
#define	LTP_MEAN_SEARCH_LENGTH	4
#endif

#define	MAX_LTP_CLIENT_NBR	(LTP_MAX_NBR_OF_CLIENTS - 1)
#define	MAX_RETRANSMISSIONS	9
#define MAX_NBR_OF_CHECKPOINTS	(1 + MAX_RETRANSMISSIONS)
#define MAX_NBR_OF_REPORTS	(MAX_NBR_OF_CHECKPOINTS)
#define MAX_CLAIMS_PER_RS	20
#define	MAX_TIMEOUTS		2

/*	LTP segment structure definitions.				*/

typedef struct
{
	unsigned long	offset;
	unsigned long	length;
} LtpReceptionClaim;

#define	LTP_CTRL_FLAG		0x08
#define	LTP_EXC_FLAG		0x04
#define LTP_FLAG_1		0x02
#define	LTP_FLAG_0		0x01

typedef enum
{
	LtpDsRed = 0,
	LtpDsRedCheckpoint,
	LtpDsRedEORP,
	LtpDsRedEOB,
	LtpDsGreen,
	LtpDsGreenEOB = 7,
	LtpRS,		/*	Report.					*/
	LtpRAS,		/*	Report acknowledgment.			*/
	LtpCS = 12,	/*	Cancel by source of block.		*/
	LtpCAS,		/*	CS acknowledgment.			*/
	LtpCR,		/*	Cancel by block receiver (destination).	*/
	LtpCAR		/*	CR acknowledgment.			*/
} LtpSegmentTypeCode;

typedef enum
{
	LtpTimerSuspended=0,
	LtpTimerRunning
} LtpTimerState;

typedef struct
{
	time_t			segArrivalTime;
	time_t			ackDeadline;
	int			expirationCount;
	LtpTimerState		state;
} LtpTimer;

typedef enum
{
	LtpCancelByUser = 0,
	LtpClientSvcUnreachable,
	LtpRetransmitLimitExceeded,
	LtpMiscoloredSegment,
	LtpCancelByEngine
} LtpCancelReasonCode;

typedef struct
{
	LtpSegmentTypeCode	segTypeCode;
	unsigned char		headerExtensionsCount;
	unsigned char		trailerExtensionsCount;

	/*	Fields used for multiple segment classes.		*/

	unsigned long		ckptSerialNbr;
	unsigned long		rptSerialNbr;
	LtpTimer		timer;	/*	Checkpoint or report.	*/

	/*	Fields for data segments.				*/

	unsigned long		clientSvcId;	/*	Destination.	*/
	unsigned long		offset;		/*	Within block.	*/
	unsigned long		length;
	Object			block;	/*	Session svcDataObjects.	*/

	/*	Fields for report segments.				*/

	unsigned long		upperBound;
	unsigned long		lowerBound;
	Object			receptionClaims;/*	SDR list.	*/

	/*	Fields for management segments.				*/

	LtpCancelReasonCode	reasonCode;
} LtpPdu;

typedef enum
{
	LtpDataSeg,
	LtpReportSeg,
	LtpRptAckSeg,
	LtpMgtSeg
} LtpSegmentClass;

/*	An LtpRecvSeg encapsulates a segment that has been acquired
 *	by the local LTP engine, for handling.
 *
 *	The session referenced by an LtpRecvSeg depends on pdu.segTypeCode:
 *
 *	If the code is 0, 1, 2, 3, 4, 7, 9, 12, or 15, then the
 *	remote engine is the source of the block, the local engine is
 *	the destination, and the session is therefore a ImportSession.
 *
 *	If the code is 8, 13, or 14, then the remote engine is the
 *	destination of the block, the local engine is the source,
 *	and the session is therefore an ExportSession.			*/

typedef struct
{
	unsigned long	fileOffset;	/*	In file; 0 if in heap.	*/
	Object		heapAddress;	/*	In heap; 0 if in file.	*/
	Object		sessionObj;
	Object		sessionListElt;
	LtpSegmentClass	segmentClass;
	LtpPdu		pdu;
} LtpRecvSeg;

/*	An LtpXmitSeg encapsulates a segment that has been produced
 *	by the local LTP engine, for transmission.
 *
 *	The session referenced by an LtpXmitSeg depends on pdu.segTypeCode:
 *
 *	If the code is 0, 1, 2, 3, 4, 7, 9, 12, or 15, then the
 *	remote engine is the destination of the block, the local engine
 *	is the source, and the session number therefore identifies
 *	an ExportSession.
 *
 *	If the code is 8, 13, or 14, then the remote engine is the
 *	source of the block, the local engine is the destination,
 *	and the session number therefore identifies a ImportSession.	*/

typedef struct
{
	unsigned long	sessionNbr;
	unsigned long	remoteEngineId;
	short		ohdLength;
	Object		queueListElt;
	Object		sessionObj;	/*	For codes 1-3, 14 only.	*/
	Object		sessionListElt;	/*	For data segments only.	*/
	LtpSegmentClass	segmentClass;
	LtpPdu		pdu;
} LtpXmitSeg;

/* Session structures */

typedef struct
{
	unsigned long	sessionNbr;	/*	Assigned by source.	*/
	Sdnv		sessionNbrSdnv;
	unsigned long	clientSvcId;
	int		redPartLength;
	int		redPartReceived;
	unsigned char	endOfBlockRecd;	/*	Boolean.		*/
	LtpTimer	timer;		/*	For cancellation.	*/
	int		reasonCode;	/*	For cancellation.	*/
	Object		redSegments;	/*	SDR list of LtpRecvSegs	*/
	Object		rsSegments;	/*	SDR list of LtpXmitSegs	*/
	int		reportsCount;
	Object		blockFileRef;	/*	A ZCO File Ref object.	*/

	/*	Backward reference.					*/

	Object		span;		/*	Reception span.		*/
} ImportSession;

typedef struct
{
	Object		span;		/*	Transmission span.	*/
	unsigned long	sessionNbr;	/*	Assigned by self.	*/
	Sdnv		sessionNbrSdnv;
	unsigned long	clientSvcId;
	Sdnv		clientSvcIdSdnv;
	int		totalLength;
	int		redPartLength;
	LtpTimer	timer;		/*	For cancellation.	*/
	int		reasonCode;	/*	For cancellation.	*/
	Object		svcDataObjects;	/*	SDR list of ZCO refs.	*/
	Object		redSegments;	/*	SDR list of LtpXmitSegs	*/
	Object		greenSegments;	/*	SDR list of LtpXmitSegs	*/
	Object		claims;		/*	reception claims list	*/
	int		checkpointsCount;
} ExportSession;

typedef struct
{
	unsigned int	offset;
	unsigned int	length;
} ExportExtent;

/* Timeline event structure */

typedef enum
{
	LtpResendCheckpoint = 1,
	LtpResendXmitCancel,
	LtpResendReport,
	LtpResendRecvCancel
} LtpEventType;

typedef struct
{
	unsigned long	refNbr1;	/*	Engine ID.		*/
	unsigned long	refNbr2;	/*	Session number.		*/
	unsigned long	refNbr3;	/*	Serial number.		*/
	Object		parm;		/*	Possible future use.	*/
	time_t		scheduledTime;	/*	Seconds since Jan 1970.	*/
	LtpEventType	type;
} LtpEvent;

/* Span structure characterizing the communication span between the
 * local engine and some remote engine.  Note that a single LTP span
 * might be serviced by multiple communication links, e.g., simultaneous
 * S-band and Ka-band transmission. */

typedef struct
{
	unsigned long	engineId;	/*	ID of remote engine.	*/
	Sdnv		engineIdSdnv;
	unsigned int	remoteQtime;	/*	In seconds.		*/
	int		purge;		/*	Boolean.		*/
	Object		lsoCmd;		/*	For starting the LSO.	*/
	unsigned int	maxExportSessions;
	unsigned int	maxExportBlockSize;
	unsigned int	maxImportSessions;
	unsigned int	maxImportBlockSize;
	unsigned int	aggrSizeLimit;	/*	Bytes.			*/
	unsigned int	aggrTimeLimit;	/*	Seconds.		*/
	unsigned int	maxSegmentSize;	/*	MTU size, in bytes.	*/

	Object		currentExportSessionObj;
	unsigned int	ageOfBufferedBlock;
	unsigned int	lengthOfBufferedBlock;
	unsigned int	redLengthOfBufferedBlock;
	unsigned long	clientSvcIdOfBufferedBlock;

	Object		exportSessions;	/*	SDR list: ExportSession	*/
	Object		segments;	/*	SDR list: LtpXmitSeg	*/
	Object		importSessions;	/*	SDR list: ImportSession	*/
	Object		importSessionsHash;
	Object		deadImports;	/*	SDR list: ImportSession	*/
} LtpSpan;

/* The volatile span object encapsulates the current volatile state
 * of the corresponding LtpSpan. 					*/

typedef struct
{
	Object		spanElt;	/*	Reference to LtpSpan.	*/
	unsigned long	engineId;	/*	ID of remote engine.	*/
	unsigned long	localXmitRate;	/*	Bytes per second.	*/
	unsigned long	remoteXmitRate;	/*	Bytes per second.	*/
	unsigned long	receptionRate;	/*	Bytes per second.	*/
	unsigned int	owltInbound;	/*	In seconds.		*/
	unsigned int	owltOutbound;	/*	In seconds.		*/
	int		meterPid;	/*	For stopping ltpmeter.	*/
	int		lsoPid;		/*	For stopping the LSO.	*/

	/*	*	*	Work area	*	*	*	*/

	PsmAddress	segmentBuffer;	/*	Holds one max-size seg.	*/

	/*	The bufEmptySemaphore of an LtpVspan is given by
	 *	the span's ltpmeter task upon construction of a new
	 *	export session.  This signifies that the aggregation
	 *	service client data objects in this session's block
	 *	may now begin.  The LtpSend function takes this
	 *	semaphore when it determines that no more client
	 *	service data objects may be appended to the current
	 *	export session's block, so it must wait for a new
	 *	session to be constructed before the transmission of
	 *	client service data objects can continue.		*/

	sm_SemId	bufEmptySemaphore;

	/*	The bufFullSemaphore of an LtpVspan is given by
	 *	the LtpSend function every time the appending of a
	 *	client service data unit to the list of service data
	 *	objects for the current export session causes the
	 *	aggregate length of data buffered in that session to
	 *	reach the nominal block size for the span.  This
	 *	signifies that the session is ready for transmission.
	 *	(This semaphore is also given by the ltpclock task
	 *	when the aggregate length of data buffered has been
	 *	non-zero for aggrTimeLimit seconds.  This serves to
	 *	prevent a partially filled session buffer from
	 *	remaining untransmitted indefinitely after the end
	 *	of a period of client service activity.)  The span's
	 *	ltpmeter task takes this semaphore before proceeding
	 *	to segment the current outbound block and append
	 *	its segments to the span's segments queue.		*/

	sm_SemId	bufFullSemaphore;

	/*	The segSemaphore of an LtpVspan is given by the
	 *	sendBlock function every time an outbound segment is
	 *	enqueued for transmission over this span.  This
	 *	signifies that the LSO task may now obtain a segment
	 *	from the LtpSpan's segments list and transmit it
	 *	-- thus eliminating polling from LTP transmission
	 *	processing.  The ltpDequeueOutboundSegment function
	 *	takes this semaphore before the LSO task proceeds to
	 *	transmit the segment via its link service protocol.	*/

	sm_SemId	segSemaphore;	/*	For outbound segments.	*/
} LtpVspan;

/* Client and notice structures */

typedef struct
{
	LtpSessionId	sessionId;
	unsigned long	dataOffset;
	unsigned long	dataLength;
	LtpNoticeType	type;
	unsigned char	reasonCode;
	unsigned char	endOfBlock;	/*	Boolean.		*/
	Object		data;		/*	To be serialized.	*/
} LtpNotice;

typedef struct
{
	Object		notices;	/*	SDR list of LtpNotices	*/
} LtpClient;

/* The volatile client object encapsulates the current volatile state
 * of the corresponding LtpClient. 					*/

typedef struct
{
	Object		notices;	/*	Copied from LtpClient.	*/
	int		pid;
	sm_SemId	semaphore;	/*	For notices.		*/
} LtpVclient;

/* Database structure */

typedef struct
{
	unsigned long	ownEngineId;
	Sdnv		ownEngineIdSdnv;

	/*	estMaxExportSessions is used to compute the number
	 *	of rows in the export sessions hash table in the LTP
	 *	database.  If the summation of maxExportSessions over
	 *	all spans exceeds estMaxExportSessions, LTP export
	 *	session lookup performance may be compromised.
	 *
	 *	heapSpaceBytesReserved is used to reserve SDR heap
	 *	space for LTP activity.  This reservation may be
	 *	allocated among the import and export spans in any
	 *	way, and may be reallocated as necessary, so
	 *	long as the reserved limit is not exceeded.		*/

	int		estMaxExportSessions;
	int		heapSpaceBytesReserved;
	int		heapSpaceBytesOccupied;
	int		allBlocksInHeap;/*	Boolean.		*/
	unsigned int	ownQtime;
	unsigned int	enforceSchedule;/*	Boolean.		*/
	LtpClient	clients[LTP_MAX_NBR_OF_CLIENTS];
	unsigned long	sessionCount;
	Object		exportSessionsHash;
	Object		deadExports;	/*	SDR list: ExportSession	*/
	Object		spans;		/*	SDR list: LtpSpan	*/
	Object		timeline;	/*	SDR list: LtpEvent	*/
} LtpDB;

/* The volatile database object encapsulates the current volatile state
 * of the database. */

/*	"Watch" switches for LTP operation.				*/
#define WATCH_d			(1)
#define WATCH_e			(2)
#define WATCH_f			(4)
#define WATCH_g			(8)
#define WATCH_h			(16)
#define WATCH_s			(32)
#define WATCH_t			(64)
#define WATCH_nak		(128)
#define WATCH_CS		(256)
#define WATCH_handleCS		(512)
#define WATCH_CR		(1024)
#define WATCH_handleCR		(2048)
#define WATCH_resendCP		(4096)
#define WATCH_resendRS		(8192)

typedef struct
{
	int		lsiPid;		/*	For stopping the LSI.	*/
	int		clockPid;	/*	For stopping ltpclock.	*/
	int		watching;	/*	Boolean activity watch.	*/
	PsmAddress	spans;		/*	SM list: LtpVspan*	*/
	LtpVclient	clients[LTP_MAX_NBR_OF_CLIENTS];

	/*	The sessionSemaphore of the LTP engine is given by
	 *	the closeExportSession function every time an export
	 *	session is closed.  This signifies that a new export
	 *	session may begin.  The startExportSession function
	 *	in the ltpmeter task(s) takes this semaphore before
	 *	proceeding to construct a new export session.
	 *
	 *	Since the number of concurrent export sessions is
	 *	limited at compile time, this mechanism implements
	 *	LTP flow control.
	 *
	 *	Construction of a new export session for a given span
	 *	causes the bufEmptySemaphore for that span to be given,
	 *	potentially enabling a blocked LtpSend() on that span
	 *	to complete.						*/

	sm_SemId	sessionSemaphore;
} LtpVdb;

extern int		ltpInit(int estMaxExportSessions, int bytesReserved);
extern int		ltpStart();
extern void		ltpStop();
extern int		ltpAttach();

extern Object		getLtpDbObject();
extern LtpDB		*getLtpConstants();
extern LtpVdb		*getLtpVdb();

extern void		findSpan(unsigned long engineId, LtpVspan **vspan,
				PsmAddress *vspanElt);
extern int		addSpan(unsigned long engineId,
				unsigned int maxExportSessions,
				unsigned int maxExportBlockSize,
				unsigned int maxImportSessions,
				unsigned int maxImportBlockSize,
				unsigned int maxSegmentSize,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
				char *lsoCmd, unsigned int qTime, int purge);
extern int		updateSpan(unsigned long engineId,
	       			unsigned int maxExportSessions,
				unsigned int maxExportBlockSize,
				unsigned int maxImportSessions,
				unsigned int maxImportBlockSize,
				unsigned int maxSegmentSize,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
				char *lsoCmd, unsigned int qTime, int purge);
extern int		removeSpan(unsigned long engineId);
extern void		checkReservationLimit();

extern int		ltpStartSpan(unsigned long engineId);
extern void		ltpStopSpan(unsigned long engineId);

extern int		startExportSession(Sdr sdr, Object spanObj,
				LtpVspan *vspan);
extern int		issueSegments(Sdr sdr, LtpSpan *span, LtpVspan *vspan,
				ExportSession *session, Object sessionObj,
				Lyst extents, unsigned long reportSerialNbr);

extern int		ltpAttachClient(unsigned long clientSvcId);
extern void		ltpDetachClient(unsigned long clientSvcId);

extern int		enqueueNotice(LtpVclient *client,
				unsigned long sourceEngineId,
				unsigned long sessionNbr,
				unsigned long dataOffset,
				unsigned long dataLength,
				LtpNoticeType type,
				unsigned char reasonCode,
				unsigned char endOfBlock,
				Object data);

extern int		ltpDequeueOutboundSegment(LtpVspan *vspan, char **buf);
extern int		ltpHandleInboundSegment(char *buf, int length);

extern void		ltpStartXmit(LtpVspan *vspan);
extern void		ltpStopXmit(LtpVspan *vspan);
extern int		ltpSuspendTimers(LtpVspan *vspan, PsmAddress vspanElt,
				time_t suspendTime, unsigned long xmitRate);
extern int		ltpResumeTimers(LtpVspan *vspan, PsmAddress vspanElt,
				time_t resumeTime, unsigned long xmitRate);

extern int		ltpResendCheckpoint(unsigned long sessionNbr,
				unsigned long checkpoint_serial_number);
extern int		ltpResendXmitCancel(unsigned long sessionNbr);
extern int		ltpResendReport(unsigned long engineId,
				unsigned long sessionNbr,
				unsigned long report_serial_number);
extern int		ltpResendRecvCancel(unsigned long engineId,
				unsigned long sessionNbr);

#ifdef __cplusplus
}
#endif

#endif
