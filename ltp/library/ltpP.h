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
	unsigned int	offset;
	unsigned int	length;
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

	unsigned int		ckptSerialNbr;
	unsigned int		rptSerialNbr;
	LtpTimer		timer;	/*	Checkpoint or report.	*/

	/*	Fields for data segments.				*/

	unsigned int		clientSvcId;	/*	Destination.	*/
	unsigned int		offset;		/*	Within block.	*/
	unsigned int		length;
	Object			block;	/*	Session svcDataObjects.	*/

	/*	Fields for report segments.				*/

	unsigned int		upperBound;
	unsigned int		lowerBound;
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
	unsigned int	acqOffset;	/*	Within acquisition ZCO.	*/
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
	unsigned int	sessionNbr;
	uvast		remoteEngineId;
	short		ohdLength;
	Object		queueListElt;
	Object		ckptListElt;	/*	For checkpoints only.	*/
	Object		sessionObj;	/*	For codes 1-3, 14 only.	*/
	Object		sessionListElt;	/*	For data segments only.	*/
	LtpSegmentClass	segmentClass;
	LtpPdu		pdu;
} LtpXmitSeg;

/* Session structures */

typedef struct
{
	unsigned int	offset;
	unsigned int	length;
	Object		sessionListElt;
} LtpSegmentRef;

typedef struct
{
	unsigned int	sessionNbr;	/*	Assigned by source.	*/
	Sdnv		sessionNbrSdnv;
	unsigned int	clientSvcId;
	int		redPartLength;
	int		redPartReceived;
	unsigned char	endOfBlockRecd;	/*	Boolean.		*/
	LtpTimer	timer;		/*	For cancellation.	*/
	int		reasonCode;	/*	For cancellation.	*/
	Object		redSegments;	/*	SDR list of LtpRecvSegs	*/
	Object		rsSegments;	/*	SDR list of LtpXmitSegs	*/
	unsigned int	lastRptSerialNbr;
	int		reportsCount;
	Object		blockFileRef;	/*	A ZCO File Ref object.	*/
	Object		svcData;	/*	The acquisition ZCO.	*/
	int		congestive;	/*	Boolean: no ZCO space.	*/

	/*	Backward reference.					*/

	Object		span;		/*	Reception span.		*/
} ImportSession;

/*	The volatile import session object encapsulates the current
 *	volatile state of the corresponding ImportSession.  The main
 *	purpose of this structure is to accelerate the insertion of
 *	red-data segments into the very long redSgments list of an
 *	extremely large block; for a block comprising a small number
 *	of red-data segments, there is no performance advantage.	*/

typedef struct
{
	unsigned int	sessionNbr;	/*	ID of ImportSession.	*/
	Object		sessionElt;	/*	Ref. to ImportSession.	*/
	PsmAddress	redSegmentsIdx;	/*	RBT of LtpSegmentRefs	*/
} VImportSession;

/*	An LtpCkpt is a reference to an export session redSegment that
 *	is a transmission checkpoint.  The list of LtpCheckpoints
 *	provides a quick way to locate the specific LtpXmitSeg, out of
 *	a possibly very long list of redSegments, that is a checkpoint
 *	tagged with a given serial number.				*/

typedef struct
{
	unsigned int	serialNbr;
	Object		sessionListElt;
} LtpCkpt;

typedef struct
{
	Object		span;		/*	Transmission span.	*/
	unsigned int	sessionNbr;	/*	Assigned by self.	*/
	Sdnv		sessionNbrSdnv;
	unsigned int	clientSvcId;
	Sdnv		clientSvcIdSdnv;
	int		totalLength;
	int		redPartLength;
	LtpTimer	timer;		/*	For cancellation.	*/
	int		reasonCode;	/*	For cancellation.	*/
	Object		svcDataObjects;	/*	SDR list of ZCOs	*/
	Object		claims;		/*	reception claims list	*/
	Object		checkpoints;	/*	SDR list of LtpCkpts	*/

	/*	Segments are retained in these lists only up to the
	 *	time of initial transmission, and only to support
	 *	ExportSession cancellation prior to transmission of
	 *	the segments.						*/

	Object		redSegments;	/*	SDR list of LtpXmitSegs	*/
	Object		greenSegments;	/*	SDR list of LtpXmitSegs	*/
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
	LtpResendRecvCancel,
	LtpForgetSession
} LtpEventType;

typedef struct
{
	uvast		refNbr1;	/*	Engine ID.		*/
	unsigned int	refNbr2;	/*	Session number.		*/
	unsigned int	refNbr3;	/*	Serial number.		*/
	Object		parm;		/*	Non-specific use.	*/
	time_t		scheduledTime;	/*	Seconds since Jan 1970.	*/
	LtpEventType	type;
} LtpEvent;

/* Span structure characterizing the communication span between the
 * local engine and some remote engine.  Note that a single LTP span
 * might be serviced by multiple communication links, e.g., simultaneous
 * S-band and Ka-band transmission. */

typedef struct
{
	uvast		engineId;	/*	ID of remote engine.	*/
	Sdnv		engineIdSdnv;
	unsigned int	remoteQtime;	/*	In seconds.		*/
	int		purge;		/*	Boolean.		*/
	Object		lsoCmd;		/*	For starting the LSO.	*/
	unsigned int	maxExportSessions;
	unsigned int	maxImportSessions;
	unsigned int	aggrSizeLimit;	/*	Bytes.			*/
	unsigned int	aggrTimeLimit;	/*	Seconds.		*/
	unsigned int	maxSegmentSize;	/*	MTU size, in bytes.	*/
	Object		stats;		/*	LtpSpanStats address.	*/
	int		updateStats;	/*	Boolean.		*/

	Object		currentExportSessionObj;
	unsigned int	ageOfBufferedBlock;
	unsigned int	lengthOfBufferedBlock;
	unsigned int	redLengthOfBufferedBlock;
	unsigned int	clientSvcIdOfBufferedBlock;

	Object		exportSessions;	/*	SDR list: ExportSession	*/
	Object		segments;	/*	SDR list: LtpXmitSeg	*/
	Object		importSessions;	/*	SDR list: ImportSession	*/
	Object		importSessionsHash;
	Object		closedImports;	/*	SDR list: session nbr	*/
	Object		deadImports;	/*	SDR list: ImportSession	*/
} LtpSpan;

/*	*	*	LTP statistics management	*	*	*/

#define	OUT_SEG_QUEUED		0
#define	OUT_SEG_POPPED		1
#define	CKPT_XMIT		2
#define	POS_RPT_RECV		3
#define	NEG_RPT_RECV		4
#define	EXPORT_CANCEL_RECV	5
#define	CKPT_RE_XMIT		6
#define	EXPORT_CANCEL_XMIT	7
#define	EXPORT_COMPLETE		8
#define	CKPT_RECV		9
#define	POS_RPT_XMIT		10
#define	NEG_RPT_XMIT		11
#define	IMPORT_CANCEL_RECV	12
#define	RPT_RE_XMIT		13
#define	IMPORT_CANCEL_XMIT	14
#define	IMPORT_COMPLETE		15
#define	IN_SEG_RECV_RED		16
#define	IN_SEG_RECV_GREEN	17
#define	IN_SEG_REDUNDANT	18
#define	IN_SEG_MALFORMED	19
#define	IN_SEG_UNK_SENDER	20
#define	IN_SEG_UNK_CLIENT	21
#define	IN_SEG_SCREENED		22
#define	IN_SEG_MISCOLORED	23
#define	IN_SEG_SES_CLOSED	24
#define	LTP_SPAN_STATS		25

typedef struct
{
	time_t		resetTime;
	Tally		tallies[LTP_SPAN_STATS];
} LtpSpanStats;

/* The volatile span object encapsulates the current volatile state
 * of the corresponding LtpSpan. 					*/

typedef struct
{
	Object		spanElt;	/*	Reference to LtpSpan.	*/
	Object		stats;		/*	LtpSpanStats address.	*/
	int		updateStats;	/*	Boolean.		*/
	uvast		engineId;	/*	ID of remote engine.	*/
	unsigned int	localXmitRate;	/*	Bytes per second.	*/
	unsigned int	remoteXmitRate;	/*	Bytes per second.	*/
	unsigned int	receptionRate;	/*	Bytes per second.	*/
	unsigned int	owltInbound;	/*	In seconds.		*/
	unsigned int	owltOutbound;	/*	In seconds.		*/
	int		meterPid;	/*	For stopping ltpmeter.	*/
	int		lsoPid;		/*	For stopping the LSO.	*/
	PsmAddress	importSessions;	/*	RBT of VImportSessions	*/
	PsmAddress	avblIdxRbts;	/*	SmList of empty RBTs	*/

	/*	For detecting miscolored segments.			*/

	unsigned int	greenSessionNbr;
	unsigned int	greenOffset;

	/*	*	*	Work area	*	*	*	*/

	PsmAddress	segmentBuffer;	/*	Holds one max-size seg.	*/

	/*	The bufOpenRedSemaphore and bufOpenGreenSemaphore
	 *	of an LtpVspan are given by the span's ltpmeter task
	 *	upon construction of a new export session, or by the
	 *	ltpclo task upon cancellation of an export session.
	 *	This signifies that it may now be possible to begin
	 *	or, if halted, resume the aggregation of service
	 *	client data objects in this session's block.  The
	 *	ltp_send function takes this semaphore when it
	 *	determines that its client service data object
	 *	cannot be appended to the current export session's
	 *	block, for some reason, so it must wait for the
	 *	block to be reopened.  The rules for appending an
	 *	SDU to a block differ depending on whether or not
	 *	the SDU contains any "red" data; ltp_send will take
	 *	the bufOpenRedSemaphore if its SDU's red length is
	 *	greater than zero, the bufOpenGreenSemaphore if not.	*/

	sm_SemId	bufOpenRedSemaphore;
	sm_SemId	bufOpenGreenSemaphore;

	/*	The bufClosedSemaphore of an LtpVspan is given by
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

	sm_SemId	bufClosedSemaphore;

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
	unsigned int	dataOffset;
	unsigned int	dataLength;
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
	uvast		ownEngineId;
	Sdnv		ownEngineIdSdnv;

	/*	estMaxExportSessions is used to compute the number
	 *	of rows in the export sessions hash table in the LTP
	 *	database.  If the summation of maxExportSessions over
	 *	all spans exceeds estMaxExportSessions, LTP export
	 *	session lookup performance may be compromised.		*/

	int		estMaxExportSessions;
	unsigned int	ownQtime;
	unsigned int	enforceSchedule;/*	Boolean.		*/
	LtpClient	clients[LTP_MAX_NBR_OF_CLIENTS];
	unsigned int	sessionCount;
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
	uvast		ownEngineId;
	int		lsiPid;		/*	For stopping the LSI.	*/
	int		clockPid;	/*	For stopping ltpclock.	*/
	int		watching;	/*	Boolean activity watch.	*/
	PsmAddress	spans;		/*	SM list: LtpVspan*	*/
	LtpVclient	clients[LTP_MAX_NBR_OF_CLIENTS];
} LtpVdb;

extern int		ltpInit(int estMaxExportSessions);
extern void		ltpDropVdb();
extern void		ltpRaiseVdb();
extern int		ltpStart();
extern void		ltpStop();
extern int		ltpAttach();
extern void		ltpDetach();

extern Object		getLtpDbObject();
extern LtpDB		*getLtpConstants();
extern LtpVdb		*getLtpVdb();

extern void		findSpan(uvast engineId, LtpVspan **vspan,
				PsmAddress *vspanElt);
extern int		addSpan(uvast engineId,
				unsigned int maxExportSessions,
				unsigned int maxImportSessions,
				unsigned int maxSegmentSize,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
				char *lsoCmd, unsigned int qTime, int purge);
extern int		updateSpan(uvast engineId,
	       			unsigned int maxExportSessions,
				unsigned int maxImportSessions,
				unsigned int maxSegmentSize,
				unsigned int aggrSizeLimit,
				unsigned int aggrTimeLimit,
				char *lsoCmd, unsigned int qTime, int purge);
extern int		removeSpan(uvast engineId);
extern void		checkReservationLimit();

extern int		ltpStartSpan(uvast engineId);
extern void		ltpStopSpan(uvast engineId);

extern int		startExportSession(Sdr sdr, Object spanObj,
				LtpVspan *vspan);
extern int		issueSegments(Sdr sdr, LtpSpan *span, LtpVspan *vspan,
				ExportSession *session, Object sessionObj,
				Lyst extents, unsigned int reportSerialNbr);

extern int		ltpAttachClient(unsigned int clientSvcId);
extern void		ltpDetachClient(unsigned int clientSvcId);

extern int		enqueueNotice(LtpVclient *client,
				uvast sourceEngineId,
				unsigned int sessionNbr,
				unsigned int dataOffset,
				unsigned int dataLength,
				LtpNoticeType type,
				unsigned char reasonCode,
				unsigned char endOfBlock,
				Object data);

extern int		ltpDequeueOutboundSegment(LtpVspan *vspan, char **buf);
extern int		ltpHandleInboundSegment(char *buf, int length);

extern void		ltpStartXmit(LtpVspan *vspan);
extern void		ltpStopXmit(LtpVspan *vspan);
extern int		ltpSuspendTimers(LtpVspan *vspan, PsmAddress vspanElt,
				time_t suspendTime, unsigned int xmitRate);
extern int		ltpResumeTimers(LtpVspan *vspan, PsmAddress vspanElt,
				time_t resumeTime, unsigned int xmitRate);

extern int		ltpResendCheckpoint(unsigned int sessionNbr,
				unsigned int checkpoint_serial_number);
extern int		ltpResendXmitCancel(unsigned int sessionNbr);
extern int		ltpResendReport(uvast engineId,
				unsigned int sessionNbr,
				unsigned int report_serial_number);
extern int		ltpResendRecvCancel(uvast engineId,
				unsigned int sessionNbr);

extern void		ltpSpanTally(LtpVspan *vspan, unsigned int idx,
				unsigned int size);
#ifdef __cplusplus
}
#endif

#endif
