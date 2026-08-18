/*
 *	libltpP.c:	functions enabling the implementation of
 *			LTP engines.
 *
 *	Copyright (c) 2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author:		Scott Burleigh, JPL
 *	Modifications:	TCSASSEMBLER, TopCoder
 *
 *	Modification History:
 *	Date       Who   What
 *	09-24-13    TC   Added NULL_KEY constant variable.
 *	10-08-13    TC   Added Outbound Authentication Support
 *	10-28-13    TC   Added Inbound Authentication Support
 *	12-29-13    TC   Fix incorrect procedure for ignoring unknown
 *			 extensions.
 *	12-29-13    TC   Key IDs are names that should be used to look up
 *			 key values in the ionsec database.
 *	01-07-14    TC   Move cryto code to ici.
 *	01-12-14    TC   Merge of all fixes.
 *	02-06-14    TC   Fix the RSA public/private issues, but move the
 *			 implementation into ext/auth/auth.c.
 *	02-19-14    TC   Modify ltp outbound and inbound to authenticate
 *			 from generalized extensions.
 */

#include "ion_atomic.h"
#include "ltpP.h"
#include "ltpei.h"
#include "platform_smP.h"

/*	Enable LTP_DEBUG to get detailed diagnostics for span management
 *	and RBT operations. This is useful for debugging semaphore issues
 *	but adds significant log volume in production.			*/

#ifndef LTP_DEBUG
#define LTP_DEBUG	0
#endif

#define	EST_LINK_OHD		16

#ifndef LTPDEBUG
#define	LTPDEBUG		0
#endif

#if (!(defined(SIGNAL_REDUNDANCY)) || SIGNAL_REDUNDANCY < 1)
#define SIGNAL_REDUNDANCY	1.0
#endif

#if (!(defined(CHECKPOINT_BURST)) || CHECKPOINT_BURST < 1)
#define CHECKPOINT_BURST	1
#endif

#if (!(defined(REPORTSEGMENT_BURST)) || REPORTSEGMENT_BURST < 1)
#define REPORTSEGMENT_BURST	1
#endif

#if (!(defined(CANCELSEGMENT_BURST)) || CANCELSEGMENT_BURST < 1)
#define CANCELSEGMENT_BURST	1
#endif

#if (!(defined(REPORTACK_BURST)) || REPORTACK_BURST < 1)
#define REPORTACK_BURST		1
#endif

#if (!(defined(CANCELACK_BURST)) || CANCELACK_BURST < 1)
#define CANCELACK_BURST		1
#endif

#define CEIL(x)			(int)(x) + (1 - (int)((int)((x) + 1) - (x)))

#define LTP_VERSION		0;

static SdrObject insertLtpTimelineEvent(LtpEvent *newEvent);
static int	 constructReportAckSegment(LtpSpan *span, SdrObject spanObj,
		unsigned int sessionNbr, unsigned int reportSerialNbr);
static SdrObject enqueueAckSegment(SdrObject spanObj, SdrObject segmentObj);

/*	*	*	Helpful utility functions	*	*	*/

static SdrObject _ltpdbObject(SdrObject *newDbObj)
{
	static SdrObject obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static LtpDB	*_ltpConstants(void)
{
	static LtpDB	buf;
	static LtpDB	*db = NULL;
	Sdr		sdr;
	SdrObject	dbObject;

	if (db == NULL)
	{
		/*	Load constants into a conveniently accessed
		 *	structure.  Note that this CANNOT be treated
		 *	as a current database image in later
		 *	processing.					*/

		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _ltpdbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(LtpDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(LtpDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

/*	Note: to avoid running out of database heap space, LTP uses
 *	flow control based on limiting the number of export sessions
 *	that can be concurrently active; this value constitutes the
 *	flow control "window" for LTP.  The limit is set at the time
 *	LTP is initialized and is used to fix the size of the window
 *	at that time; specifically, it establishes the size of the
 *	exportSessions hash table.					*/

void	ltpSpanTally(LtpVspan *vspan, unsigned int idx, unsigned int size)
{
	CHKVOID(vspan);
	CHKVOID(idx < LTP_SPAN_STATS);
	ion_ipc_atomic_get_and_increment(&vspan->statsDeltas[idx].deltaCount, 1);
	ion_ipc_atomic_get_and_increment(&vspan->statsDeltas[idx].deltaBytes, (uvast) size);
}

int	ltpFlushSpanStats(Sdr sdr, LtpVspan *vspan)
{
	LtpSpanStats	stats;
	int		i;
	unsigned int	dCount;
	uvast		dBytes;
	int		modified = 0;

	CHKERR(vspan);
	if (!(vspan->updateStats) || vspan->stats == 0)
	{
		/*	Stats disabled; just drain deltas.		*/

		for (i = 0; i < LTP_SPAN_STATS; i++)
		{
			ion_ipc_atomic_exchange(&vspan->statsDeltas[i].deltaCount, 0);
			ion_ipc_atomic_exchange(&vspan->statsDeltas[i].deltaBytes, 0);
		}

		return 0;
	}

	sdr_stage(sdr, (char *) &stats, vspan->stats, sizeof(LtpSpanStats));
	for (i = 0; i < LTP_SPAN_STATS; i++)
	{
		dCount = (unsigned int)ion_ipc_atomic_exchange(&vspan->statsDeltas[i].deltaCount, 0);
		dBytes = ion_ipc_atomic_exchange(&vspan->statsDeltas[i].deltaBytes, 0);
		if (dCount > 0 || dBytes > 0)
		{
			stats.tallies[i].totalCount += dCount;
			stats.tallies[i].totalBytes += dBytes;
			stats.tallies[i].currentCount += dCount;
			stats.tallies[i].currentBytes += dBytes;
			modified = 1;
		}
	}

	if (modified)
	{
		sdr_write(sdr, vspan->stats, (char *) &stats,
				sizeof(LtpSpanStats));
	}

	return 0;
}

/*	*	*	Functions for LTP enhancements	*	*	*/

#if CLOSED_EXPORTS_ENABLED
void ltpForgetClosedExport(SdrObject elt)
{
	Sdr	sdr = getIonsdr();
	SdrObject closedExportObj;

	closedExportObj = sdr_list_data(sdr, elt);
#if LTPDEBUG
ClosedExport	closedExportBuf;
sdr_read(sdr, (char *) &closedExportBuf, closedExportObj, sizeof(ClosedExport));
putErrmsg("Forget export session: timer elapsed",
utoa(closedExportBuf.sessionNbr));
#endif
	sdr_free(sdr, closedExportObj);
	sdr_list_delete(sdr, elt, NULL, NULL);
}

static int noteClosedExport(LtpDB *ltpdb, LtpVspan *vspan, SdrObject spanObj,
		unsigned int sessionNbr)
{
	Sdr		sdr = getIonsdr();
	time_t		currentTime = getCtime();
	ClosedExport	closedExportBuf;
	SdrObject 	closedExportObj;
	SdrObject	elt;
	LtpEvent 	closedExportEvent;

	closedExportObj = sdr_malloc(sdr, sizeof(ClosedExport));
	if (closedExportObj == 0)
	{
		putErrmsg("No space for database.", NULL);
		return -1;
	}

	elt = sdr_list_insert_first(sdr, ltpdb->closedExports, closedExportObj);
	if (elt == 0)
	{
		putErrmsg("Can't insert closed export.", NULL);
		return -1;
	}

	memset((char *) &closedExportBuf, 0, sizeof(ClosedExport));
	closedExportBuf.span = spanObj;
	closedExportBuf.sessionNbr = sessionNbr;
	closedExportBuf.responseLimit = vspan->maxTimeouts;

	/*	Set timer for removal of closed export session.		*/

	memset((char *) &closedExportEvent, 0, sizeof(LtpEvent));
	closedExportEvent.parm = elt;
	closedExportEvent.scheduledTime = currentTime + 10 +
			(2 * (vspan->maxTimeouts / SIGNAL_REDUNDANCY)
			 * (vspan->owltOutbound + vspan->owltInbound));
	closedExportEvent.type = LtpForgetExportSession;
	closedExportBuf.timeout = insertLtpTimelineEvent(&closedExportEvent);
	if (closedExportBuf.timeout == 0)
	{
		putErrmsg("Can't insert closed export timeout event.", NULL);
		return -1;
	}

	/*	Record the closed export session.			*/

	sdr_write(sdr, closedExportObj, (char *) &closedExportBuf,
			sizeof(ClosedExport));
#if LTPDEBUG
char	rsbuf[256];
putErrmsg("List of closed export sessions:", NULL);
for (elt = sdr_list_first(sdr, ltpdb->closedExports); elt;
		elt = sdr_list_next(sdr, elt))
	{
		closedExportObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &closedExportBuf, closedExportObj,
				sizeof(ClosedExport));
		isprintf(rsbuf, sizeof rsbuf, "span: %lu session number: %u", closedExportBuf.span,
				closedExportBuf.sessionNbr);
		putErrmsg(rsbuf, NULL);
	}
#endif
	return 0;
}

static int	acknowledgeLateReport(unsigned int sessionNbr,
			unsigned int rptSerialNbr)
{
	Sdr		sdr = getIonsdr();
	SdrObject 	elt;
	SdrObject 	closedExportObj;
	ClosedExport	closedExportBuf;
	LtpSpan		spanBuf;

	for (elt = sdr_list_first(sdr, (_ltpConstants())->closedExports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		closedExportObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &closedExportBuf, closedExportObj,
				sizeof(ClosedExport));
		if (closedExportBuf.sessionNbr == sessionNbr)
		{
#if LTPDEBUG
			writeMemoNote("[i] Ack from closed export session",
					utoa(sessionNbr));
#endif
			sdr_read(sdr, (char *) &spanBuf, closedExportBuf.span,
					sizeof(LtpSpan));
			if (constructReportAckSegment(&spanBuf,
					closedExportBuf.span,
					sessionNbr, rptSerialNbr))
			{
				putErrmsg("Can't send RA segment.", NULL);
				return 0;
			}

			closedExportBuf.responseLimit--;
			if (closedExportBuf.responseLimit < 1)
			{
#if LTPDEBUG
				writeMemoNote("[i] Forget session, retries \
exceeded", utoa(closedExportBuf.sessionNbr));
#endif
				sdr_list_delete(sdr, closedExportBuf.timeout,
						NULL, NULL);
				sdr_free(sdr, closedExportObj);
				sdr_list_delete(sdr, elt, NULL, NULL);
			}
			else
			{
				sdr_write(sdr, closedExportObj,
						(char *) (&closedExportBuf),
						sizeof(ClosedExport));
			}

			return 1;
		}
	}

	return 0;
}
#endif		/*	CLOSED_EXPORTS_ENABLED			*/

#if BURST_SIGNALS_ENABLED
static int enqueueBurst(LtpXmitSeg *segment, LtpSpan *span, SdrObject where,
		int burstType)
{
	Sdr		sdr = getIonsdr();
	int		i;
	LtpXmitSegRef	segRef;
	SdrObject	segRefObj;

	/*	Some segments may be generated by changing a
	 *	previous segment. If we don't save expirationCount
	 *	those segments will be considered as burst segments,
	 *	causing troubles.					*/

	int	oldExpirationCount = segment->pdu.timer.expirationCount;

	/*	Note: expirationCount -1 indicates that the segment
	 *	is part of a burst and prevents the setting of a
	 *	retransmission timer.  Any other non-zero value
	 *	indicates that the segment is a retransmission.		*/

	segment->pdu.timer.expirationCount = -1;
	if (segment->segmentClass == LtpDataSeg)
	{
		segRef.sessionNbr = segment->sessionNbr;
	}
	else
	{
		segRef.sessionNbr = 0;
	}

	for (i = 1; i < burstType; i++)
	{
		/*	First clone the segment itself.			*/

		segRef.segAddr = sdr_malloc(sdr, sizeof(LtpXmitSeg));
		CHKERR(segRef.segAddr);
		if (where)
		{
			segment->sessionListElt = sdr_list_insert_last(sdr,
					where, segRef.segAddr);
			CHKERR(segment->sessionListElt);
		}

		/*	Now enqueue a reference to the clone...		*/

		segRefObj = sdr_malloc(sdr, sizeof(LtpXmitSegRef));
		CHKERR(segRefObj);
		sdr_write(sdr, segRefObj, (char *) &segRef,
				sizeof(LtpXmitSegRef));
		segment->queueListElt = sdr_list_insert_last(sdr,
				span->segments, segRefObj);
		CHKERR(segment->queueListElt);

		/*	...and record the content of the clone.		*/

		sdr_write(sdr, segRef.segAddr, (char *) segment,
				sizeof(LtpXmitSeg));

	}

	segment->pdu.timer.expirationCount = oldExpirationCount;
	return 0;
}

static int enqueueAckBurst(LtpXmitSeg *segment, SdrObject spanObj, int burstType)
{
	Sdr	sdr = getIonsdr();
	int	i;
	SdrObject segmentObj;

	/*	Some segments may be generated by changing a
	 *	previous segment. if we don't save expirationCount
	 *	those segments will be considered as burst segments,
	 *	causing troubles.					*/

	int	oldExpirationCount = segment->pdu.timer.expirationCount;

	/*	Note: expirationCount -1 indicates that the segment
	 *	is part of a burst and prevents the setting of a
	 *	retransmission timer.  Any other non-zero value
	 *	indicates that the segment is a retransmission.		*/

	segment->pdu.timer.expirationCount = -1;
	for (i = 1; i < burstType; i++)
	{
		/*	First clone the segment itself.			*/

		segmentObj = sdr_malloc(sdr, sizeof(LtpXmitSeg));
		CHKERR(segmentObj);

		/*	Now enqueue a reference to the clone...		*/

		segment->queueListElt = enqueueAckSegment(spanObj, segmentObj);
		CHKERR(segment->queueListElt);

		/*	...and record the content of the clone.		*/

		sdr_write(sdr, segmentObj, (char *) segment,
				sizeof(LtpXmitSeg));
	}

	segment->pdu.timer.expirationCount = oldExpirationCount;
	return 0;
}
#endif		/*	BURST_SIGNALS_ENABLED				*/

/*	*	*	LTP service control functions	*	*	*/

static void	resetClient(LtpVclient *client)
{
	if (client->semaphore == SM_SEM_NONE)
	{
		client->semaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
		sm_SemUnend(client->semaphore);
		sm_SemGive(client->semaphore);
	}

	sm_SemTake(client->semaphore);			/*	Lock.	*/
	client->pid = ERROR;				/*	None.	*/
}

static void	raiseClient(LtpVclient *client)
{
	client->semaphore = SM_SEM_NONE;
	resetClient(client);
}

static void	resetSpan(LtpVspan *vspan)
{
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: Entry", NULL);
#endif
	if (vspan->bufOpenRedSemaphore == SM_SEM_NONE)
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Creating bufOpenRedSemaphore", NULL);
#endif
		vspan->bufOpenRedSemaphore =
				sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Resetting bufOpenRedSemaphore", NULL);
#endif
		sm_SemUnend(vspan->bufOpenRedSemaphore);
		sm_SemGive(vspan->bufOpenRedSemaphore);
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: About to take bufOpenRedSemaphore", NULL);
#endif
	sm_SemTake(vspan->bufOpenRedSemaphore);		/*	Lock.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: bufOpenRedSemaphore taken", NULL);
#endif
	if (vspan->bufOpenGreenSemaphore == SM_SEM_NONE)
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Creating bufOpenGreenSemaphore", NULL);
#endif
		vspan->bufOpenGreenSemaphore =
				sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Resetting bufOpenGreenSemaphore", NULL);
#endif
		sm_SemUnend(vspan->bufOpenGreenSemaphore);
		sm_SemGive(vspan->bufOpenGreenSemaphore);
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: About to take bufOpenGreenSemaphore", NULL);
#endif
	sm_SemTake(vspan->bufOpenGreenSemaphore);	/*	Lock.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: bufOpenGreenSemaphore taken", NULL);
#endif
	if (vspan->bufClosedSemaphore == SM_SEM_NONE)
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Creating bufClosedSemaphore", NULL);
#endif
		vspan->bufClosedSemaphore = sm_SemCreate(SM_NO_KEY,
				SM_SEM_FIFO);
	}
	else
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Resetting bufClosedSemaphore", NULL);
#endif
		sm_SemUnend(vspan->bufClosedSemaphore);
		sm_SemGive(vspan->bufClosedSemaphore);
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: About to take bufClosedSemaphore", NULL);
#endif
	sm_SemTake(vspan->bufClosedSemaphore);		/*	Lock.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: bufClosedSemaphore taken", NULL);
#endif
	if (vspan->segSemaphore == SM_SEM_NONE)
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Creating segSemaphore", NULL);
#endif
		vspan->segSemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	}
	else
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] resetSpan: Resetting segSemaphore", NULL);
#endif
		sm_SemUnend(vspan->segSemaphore);
		sm_SemGive(vspan->segSemaphore);
	}

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: About to take segSemaphore", NULL);
#endif
	sm_SemTake(vspan->segSemaphore);		/*	Lock.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: segSemaphore taken", NULL);
#endif
	vspan->meterPid = ERROR;			/*	None.	*/
	vspan->lsoPid = ERROR;				/*	None.	*/
	sm_SemGive(vspan->segSemaphore);		/*	Unlock.	*/
	sm_SemGive(vspan->bufClosedSemaphore);		/*	Unlock.	*/
	sm_SemGive(vspan->bufOpenGreenSemaphore);	/*	Unlock.	*/
	sm_SemGive(vspan->bufOpenRedSemaphore);		/*	Unlock.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] resetSpan: All semaphores unlocked, returning", NULL);
#endif
}

void	computeRetransmissionLimits(LtpVspan *vspan)
{
	OBJ_POINTER(LtpDB, ltpdb);
	float	maxBER;
	float	pBitOk;
	float	pSegmentOk;
	float	pDlvFailure;
	char	nbrBuf[FQN_MAX_LENGTH];
	char	buf[512];

	GET_OBJ_POINTER(getIonsdr(), LtpDB, ltpdb, getLtpDbObject());

	/*	Check if using explicit configuration or legacy mode.	*/

	if (vspan->useExplicitConfig)
	{
		/*	Explicit configuration: use direct parameters.	*/

		if (vspan->useSplitMode)
		{
			/*	Split mode: xmit and recv independent.	*/

			vspan->xmitSegLossRate = vspan->maxSegLossRateXmit;
			vspan->recvSegLossRate = vspan->maxSegLossRateRecv;
			vspan->maxTimeouts = vspan->maxRetriesXmit
					* SIGNAL_REDUNDANCY;
		}
		else
		{
			/*	Unified mode: symmetric configuration.	*/

			vspan->xmitSegLossRate = vspan->maxSegmentLossRate;
			vspan->recvSegLossRate = vspan->maxSegmentLossRate;
			vspan->maxTimeouts = vspan->maxRetries
					* SIGNAL_REDUNDANCY;
		}

		putFqn(nbrBuf, vspan->engineId);
		isprintf(buf, sizeof buf, "[i] Span to engine %s (explicit \
config, max xmit segment size %d, max recv segment size %d): xmit segment loss \
rate %f, recv segment loss rate %f, max timeouts %d.", nbrBuf,
				vspan->maxXmitSegSize, vspan->maxRecvSegSize,
				vspan->xmitSegLossRate,
				vspan->recvSegLossRate, vspan->maxTimeouts);
		writeMemo(buf);
		return;
	}

	/*	Legacy mode: compute from maxBER (bit error rate).	*/

	maxBER = ltpdb->maxBER;
	if (maxBER <= 0.0	/*	Perfect link.			*/
	|| maxBER >= 1.0)	/*	No communication at all.	*/
	{
		maxBER = .000001;	/*	Default.		*/
	}

	pBitOk = 1.0 - maxBER;
	pSegmentOk = pow(pBitOk, (vspan->maxXmitSegSize * 8));
	vspan->xmitSegLossRate = 1.0 - pSegmentOk;
	if (vspan->xmitSegLossRate >= .99)
	{
		vspan->xmitSegLossRate = .99;
	}

	pSegmentOk = pow(pBitOk, (vspan->maxRecvSegSize * 8));
	vspan->recvSegLossRate = 1.0 - pSegmentOk;
	if (vspan->recvSegLossRate >= .99)
	{
		vspan->recvSegLossRate = .99;
	}

	/*	Compute control segment retransmission limit.		*/

	vspan->maxTimeouts = 0;
	pDlvFailure = 1.0;
	while (pDlvFailure > .000001)
	{
		pDlvFailure *= vspan->xmitSegLossRate;
		vspan->maxTimeouts++;
	}

	if (vspan->maxTimeouts < 3)
	{
		vspan->maxTimeouts = 3;
	}

	vspan->maxTimeouts *= SIGNAL_REDUNDANCY;
	putFqn(nbrBuf, vspan->engineId);
	isprintf(buf, sizeof buf, "[i] Span to engine %s (legacy max BER \
%f, max xmit segment size %d, max recv segment size %d): xmit segment loss \
rate %f, recv segment loss rate %f, max timeouts %d.", nbrBuf, maxBER,
			vspan->maxXmitSegSize, vspan->maxRecvSegSize,
			vspan->xmitSegLossRate, vspan->recvSegLossRate,
			vspan->maxTimeouts);
	writeMemo(buf);
}

static int raiseSpan(SdrObject spanElt, LtpVdb *ltpvdb)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	SdrObject	spanObj;
	LtpSpan		span;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	PsmAddress	addr;

	spanObj = sdr_list_data(sdr, spanElt);
	sdr_read(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	findSpan(span.engineId, &vspan, &vspanElt);
	if (vspanElt)	/*	Span is already raised.			*/
	{
		return 0;
	}

	addr = psm_zalloc(ltpwm, sizeof(LtpVspan));
	if (addr == 0)
	{
		return -1;
	}

	vspanElt = sm_list_insert_last(ltpwm, ltpvdb->spans, addr);
	if (vspanElt == 0)
	{
		psm_free(ltpwm, addr);
		return -1;
	}

	vspan = (LtpVspan *) psp(ltpwm, addr);
	memset((char *) vspan, 0, sizeof(LtpVspan));
	vspan->spanElt = spanElt;
	vspan->stats = span.stats;
	vspan->updateStats = span.updateStats;
	{
		int	i;

		for (i = 0; i < LTP_SPAN_STATS; i++)
		{
			ion_ipc_atomic_init(&vspan->statsDeltas[i].deltaCount, 0);
			ion_ipc_atomic_init(&vspan->statsDeltas[i].deltaBytes, 0);
		}
	}

	vspan->engineId = span.engineId;
	vspan->maxXmitSegSize = span.maxSegmentSize;
	vspan->maxRecvSegSize = 1;

	/*	Initialize vspan with global default configuration.
	 *	This ensures that even if no manage commands are issued,
	 *	the span will use reasonable defaults rather than zeros.	*/

	{
		OBJ_POINTER(LtpDB, ltpdb);
		char	nbrBuf[FQN_MAX_LENGTH];
		char	msgBuf[512];

		GET_OBJ_POINTER(sdr, LtpDB, ltpdb, getLtpDbObject());
		vspan->useExplicitConfig = 1;
		vspan->useSplitMode = ltpdb->useGlobalSplitMode;
		vspan->hasSpanOverride = 0;

		putFqn(nbrBuf, vspan->engineId);

		if (ltpdb->useGlobalSplitMode)
		{
			/*	Split mode defaults.			*/

			vspan->maxRetriesXmit = ltpdb->defaultMaxRetriesXmit;
			vspan->maxRetriesRecv = ltpdb->defaultMaxRetriesRecv;
			vspan->maxSegLossRateXmit =
					ltpdb->defaultMaxSegLossRateXmit;
			vspan->maxSegLossRateRecv =
					ltpdb->defaultMaxSegLossRateRecv;

			isprintf(msgBuf, sizeof msgBuf,
				"[i] Span %s initialized with global split \
mode defaults: maxRetriesXmit=%u, maxRetriesRecv=%u, maxSegLossRateXmit=%.4f, \
maxSegLossRateRecv=%.4f",
				nbrBuf, vspan->maxRetriesXmit,
				vspan->maxRetriesRecv,
				vspan->maxSegLossRateXmit,
				vspan->maxSegLossRateRecv);
			writeMemo(msgBuf);
		}
		else
		{
			/*	Unified mode defaults.			*/

			vspan->maxRetries = ltpdb->defaultMaxRetries;
			vspan->maxSegmentLossRate =
					ltpdb->defaultMaxSegLossRate;

			isprintf(msgBuf, sizeof msgBuf,
				"[i] Span %s initialized with global unified \
mode defaults: maxRetries=%u, maxSegmentLossRate=%.4f",
				nbrBuf, vspan->maxRetries,
				vspan->maxSegmentLossRate);
			writeMemo(msgBuf);
		}
	}

	computeRetransmissionLimits(vspan);

	/*	Validate and complete explicit configuration at span
	 *	initialization.  Apply global defaults to any missing
	 *	parameters to ensure safe operation.			*/

	if (vspan->useExplicitConfig)
	{
		char	nbrBuf[FQN_MAX_LENGTH];
		char	warnBuf[512];
		int	incomplete = 0;
		OBJ_POINTER(LtpDB, ltpdb);

		GET_OBJ_POINTER(sdr, LtpDB, ltpdb, getLtpDbObject());
		putFqn(nbrBuf, vspan->engineId);

		if (vspan->useSplitMode)
		{
			/*	Split mode: check all four parameters.	*/

			if (vspan->maxRetriesXmit == 0)
			{
				vspan->maxRetriesXmit =
					ltpdb->defaultMaxRetriesXmit;
				incomplete = 1;
				isprintf(warnBuf, sizeof warnBuf,
					"[!] WARNING: Span %s maxRetriesXmit \
not configured, using global default: %u",
					nbrBuf, vspan->maxRetriesXmit);
				writeMemo(warnBuf);
			}

			if (vspan->maxRetriesRecv == 0)
			{
				vspan->maxRetriesRecv =
					ltpdb->defaultMaxRetriesRecv;
				incomplete = 1;
				isprintf(warnBuf, sizeof warnBuf,
					"[!] WARNING: Span %s maxRetriesRecv \
not configured, using global default: %u",
					nbrBuf, vspan->maxRetriesRecv);
				writeMemo(warnBuf);
			}

			if (vspan->maxSegLossRateXmit == 0.0)
			{
				vspan->maxSegLossRateXmit =
					ltpdb->defaultMaxSegLossRateXmit;
				incomplete = 1;
				isprintf(warnBuf, sizeof warnBuf,
					"[!] WARNING: Span %s \
maxSegLossRateXmit not configured, using global default: %.4f",
					nbrBuf, vspan->maxSegLossRateXmit);
				writeMemo(warnBuf);
			}

			if (vspan->maxSegLossRateRecv == 0.0)
			{
				vspan->maxSegLossRateRecv =
					ltpdb->defaultMaxSegLossRateRecv;
				incomplete = 1;
				isprintf(warnBuf, sizeof warnBuf,
					"[!] WARNING: Span %s \
maxSegLossRateRecv not configured, using global default: %.4f",
					nbrBuf, vspan->maxSegLossRateRecv);
				writeMemo(warnBuf);
			}

			if (incomplete)
			{
				/*	Recompute limits with defaults.	*/

				computeRetransmissionLimits(vspan);
				isprintf(warnBuf, sizeof warnBuf,
					"[i] Span %s split mode configuration \
completed with global defaults. For asymmetric links, specify all four \
parameters (m maxretriesxmit, m maxretriesrecv, m maxseglossratexmit, \
m maxseglossraterecv).",
					nbrBuf);
				writeMemo(warnBuf);
			}
		}
		else
		{
			/*	Unified mode: check both parameters.	*/

			if (vspan->maxRetries == 0)
			{
				vspan->maxRetries = ltpdb->defaultMaxRetries;
				incomplete = 1;
				isprintf(warnBuf, sizeof warnBuf,
					"[!] WARNING: Span %s maxRetries not \
configured, using global default: %u",
					nbrBuf, vspan->maxRetries);
				writeMemo(warnBuf);
			}

			if (vspan->maxSegmentLossRate == 0.0)
			{
				vspan->maxSegmentLossRate =
					ltpdb->defaultMaxSegLossRate;
				incomplete = 1;
				isprintf(warnBuf, sizeof warnBuf,
					"[!] WARNING: Span %s \
maxSegmentLossRate not configured, using global default: %.4f",
					nbrBuf, vspan->maxSegmentLossRate);
				writeMemo(warnBuf);
			}

			if (incomplete)
			{
				/*	Recompute limits with defaults.	*/

				computeRetransmissionLimits(vspan);
				isprintf(warnBuf, sizeof warnBuf,
					"[i] Span %s unified mode configuration \
completed with global defaults. For proper configuration, specify both \
parameters (m maxretries, m maxseglossrate).",
					nbrBuf);
				writeMemo(warnBuf);
			}
		}
	}

	vspan->segmentBuffer = psm_malloc(ltpwm, span.maxSegmentSize);
	if (vspan->segmentBuffer == 0)
	{
		oK(sm_list_delete(ltpwm, vspanElt, NULL, NULL));
		psm_free(ltpwm, addr);
		return -1;
	}

	vspan->importSessions = sm_rbt_create(ltpwm);
	if (vspan->importSessions == 0)
	{
		psm_free(ltpwm, vspan->segmentBuffer);
		oK(sm_list_delete(ltpwm, vspanElt, NULL, NULL));
		psm_free(ltpwm, addr);
		return -1;
	}

	vspan->avblIdxRbts = sm_list_create(ltpwm);
	if (vspan->avblIdxRbts == 0)
	{
		sm_rbt_destroy(ltpwm, vspan->importSessions, NULL, NULL);
		psm_free(ltpwm, vspan->segmentBuffer);
		oK(sm_list_delete(ltpwm, vspanElt, NULL, NULL));
		psm_free(ltpwm, addr);
		return -1;
	}

	vspan->bufOpenRedSemaphore = SM_SEM_NONE;
	vspan->bufOpenGreenSemaphore = SM_SEM_NONE;
	vspan->bufClosedSemaphore = SM_SEM_NONE;
	vspan->segSemaphore = SM_SEM_NONE;
	resetSpan(vspan);
	return 0;
}

static int raiseSeat(SdrObject seatElt, LtpVdb *ltpvdb)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	SdrObject	seatObj;
	LtpSeat		seat;
	char		lsiCmd[256];
	LtpVseat	*vseat;
	PsmAddress	vseatElt;
	PsmAddress	addr;

	seatObj = sdr_list_data(sdr, seatElt);
	sdr_read(sdr, (char *) &seat, seatObj, sizeof(LtpSeat));
	sdr_string_read(sdr, lsiCmd, seat.lsiCmd);
	findSeat(lsiCmd, &vseat, &vseatElt);
	if (vseatElt)	/*	Seat is already raised.			*/
	{
		return 0;
	}

	addr = psm_zalloc(ltpwm, sizeof(LtpVseat));
	if (addr == 0)
	{
		return -1;
	}

	vseatElt = sm_list_insert_last(ltpwm, ltpvdb->seats, addr);
	if (vseatElt == 0)
	{
		psm_free(ltpwm, addr);
		return -1;
	}

	vseat = (LtpVseat *) psp(ltpwm, addr);
	memset((char *) vseat, 0, sizeof(LtpVseat));
	vseat->seatElt = seatElt;
	istrcpy(vseat->lsiCmd, lsiCmd, sizeof vseat->lsiCmd);
	vseat->lsiPid = ERROR;
	return 0;
}

static void	deleteSegmentRef(PsmPartition ltpwm, PsmAddress nodeData,
			void *arg)
{
/* Parameter intentionally unused. */
(void)arg;

	psm_free(ltpwm, nodeData);	/*	Delete LtpSegmentRef.	*/
}

static PsmAddress	recycleIdxRbt(PsmPartition ltpwm, LtpVspan *vspan,
				PsmAddress rbt)
{
#if LTP_DEBUG
	typedef struct { PsmAddress userData; PsmAddress root; size_t length; sm_SemId lock; } SmRbt;
	SmRbt	*rbtPtr = (SmRbt *) psp(ltpwm, rbt);
	char	msgBuf[256];

	isprintf(msgBuf, sizeof msgBuf,
		"[LTP_DEBUG] recycleIdxRbt: Recycling RBT at 0x%lx with lock semaphore ID=%d",
		(unsigned long)rbt, rbtPtr->lock);
	writeMemo(msgBuf);
#endif

	sm_rbt_clear(ltpwm, rbt, deleteSegmentRef, NULL);

#if LTP_DEBUG
	isprintf(msgBuf, sizeof msgBuf,
		"[LTP_DEBUG] recycleIdxRbt: After sm_rbt_clear, RBT at 0x%lx still has lock semaphore ID=%d",
		(unsigned long)rbt, rbtPtr->lock);
	writeMemo(msgBuf);
#endif

	return sm_list_insert_first(ltpwm, vspan->avblIdxRbts, rbt);
}

static void	deleteVImportSession(PsmPartition ltpwm, PsmAddress nodeData,
			void *arg)
{
	LtpVImportSession	*vsession = (LtpVImportSession *)
						psp(ltpwm, nodeData);
	LtpVspan		*vspan = (LtpVspan *) arg;

	if (vsession->redSegmentsIdx)
	{
		/*	If vspan is NULL, we're destroying the span itself,
		 *	so don't try to recycle the RBT back into the span's
		 *	avblIdxRbts list. Just destroy it directly.		*/
		if (vspan == NULL)
		{
			sm_rbt_destroy(ltpwm, vsession->redSegmentsIdx,
					NULL, NULL);
		}
		else
		{
			oK(recycleIdxRbt(ltpwm, vspan, vsession->redSegmentsIdx));
		}
	}

	psm_free(ltpwm, nodeData);	/*	Delete VImportSession.	*/
}

static void	deleteIdxRbt(PsmPartition ltpwm, PsmAddress elt, void *arg)
{
	PsmAddress	rbtAddr;

	/* Parameter intentionally unused. */
	(void)arg;

	/*	Extract the RBT address from the list element.		*/
	rbtAddr = sm_list_data(ltpwm, elt);
	oK(sm_rbt_destroy(ltpwm, rbtAddr, NULL, NULL));
}

static void	dropSpan(LtpVspan *vspan, PsmAddress vspanElt)
{
	PsmPartition	ltpwm = getIonwm();
	PsmAddress	vspanAddr;

	vspanAddr = sm_list_data(ltpwm, vspanElt);

	/*	IMPORTANT: Destroy RBT structures BEFORE deleting semaphores.
	 *	This is critical because:
	 *	1. Each RBT has its own lock semaphore
	 *	2. If we delete the buffer semaphores first, their IDs may
	 *	   get reused by the system
	 *	3. If an RBT's lock semaphore ID happens to match a deleted
	 *	   buffer semaphore ID, the RBT will be corrupted
	 *	Therefore, destroy all RBTs first while semaphores are valid.*/

	/*	When destroying importSessions, pass NULL as the arg to
	 *	deleteVImportSession so it knows we're destroying the span
	 *	and should NOT recycle the redSegmentsIdx RBTs back into
	 *	avblIdxRbts. This prevents corruption when we destroy
	 *	avblIdxRbts immediately after.				*/
	oK(sm_rbt_destroy(ltpwm, vspan->importSessions,
			deleteVImportSession, NULL));

	/*	avblIdxRbts may have already been destroyed in removeSpan()
	 *	before stopSpan() was called, to prevent semaphore ID
	 *	corruption. Only destroy it if it still exists.	*/
	if (vspan->avblIdxRbts != 0)
	{
		oK(sm_list_destroy(ltpwm, vspan->avblIdxRbts,
				deleteIdxRbt, NULL));
	}

	/*	Now it's safe to delete the buffer semaphores.		*/
	/*	Note: If stopSpan() was called before dropSpan() (as in
	 *	removeSpan()), the semaphores have already been ended.
	 *	We only need to check if they've been ended and delete them.
	 *	We should NOT call sm_SemEnd() again, as calling it on
	 *	already-ended semaphores followed by sm_SemDelete() can
	 *	cause semaphore ID reuse issues.				*/

	if (vspan->bufOpenRedSemaphore != SM_SEM_NONE)
	{
		if (!sm_SemEnded(vspan->bufOpenRedSemaphore))
		{
			sm_SemEnd(vspan->bufOpenRedSemaphore);
		}
		microsnooze(50000);
		sm_SemDelete(vspan->bufOpenRedSemaphore);
	}

	if (vspan->bufOpenGreenSemaphore != SM_SEM_NONE)
	{
		if (!sm_SemEnded(vspan->bufOpenGreenSemaphore))
		{
			sm_SemEnd(vspan->bufOpenGreenSemaphore);
		}
		microsnooze(50000);
		sm_SemDelete(vspan->bufOpenGreenSemaphore);
	}

	if (vspan->bufClosedSemaphore != SM_SEM_NONE)
	{
		if (!sm_SemEnded(vspan->bufClosedSemaphore))
		{
			sm_SemEnd(vspan->bufClosedSemaphore);
		}
		microsnooze(50000);
		sm_SemDelete(vspan->bufClosedSemaphore);
	}

	if (vspan->segSemaphore != SM_SEM_NONE)
	{
		if (!sm_SemEnded(vspan->segSemaphore))
		{
			sm_SemEnd(vspan->segSemaphore);
		}
		microsnooze(50000);
		sm_SemDelete(vspan->segSemaphore);
	}
	psm_free(ltpwm, vspan->segmentBuffer);
	oK(sm_list_delete(ltpwm, vspanElt, NULL, NULL));
	psm_free(ltpwm, vspanAddr);
}

static void	dropSeat(LtpVseat *vseat, PsmAddress vseatElt)
{
	PsmPartition	ltpwm = getIonwm();
	PsmAddress	vseatAddr;

	/* Parameter intentionally unused. */
	(void)vseat;

	vseatAddr = sm_list_data(ltpwm, vseatElt);
	sm_list_delete(ltpwm, vseatElt, NULL, NULL);
	psm_free(ltpwm, vseatAddr);
}

static void	startSpan(LtpVspan *vspan)
{
	Sdr	sdr = getIonsdr();
	LtpSpan	span;
	char	nbrBuf[FQN_MAX_LENGTH];
	char	ltpmeterCmdString[64];
	char	cmd[SDRSTRING_BUFSZ];
	char	lsoCmdString[SDRSTRING_BUFSZ + 64];

	sdr_read(sdr, (char *) &span, sdr_list_data(sdr, vspan->spanElt),
			sizeof(LtpSpan));
	putFqn(nbrBuf, span.engineId);
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] startSpan: Entry for engine", nbrBuf);
#endif

	if (vspan->meterPid == ERROR || sm_TaskExists(vspan->meterPid) == 0)
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] startSpan: Spawning ltpmeter", nbrBuf);
#endif
		isprintf(ltpmeterCmdString, sizeof ltpmeterCmdString, "ltpmeter %s",
				nbrBuf);
		vspan->meterPid = pseudoshell(ltpmeterCmdString);
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] startSpan: ltpmeter spawned", nbrBuf);
#endif
	}
#ifdef DEBUG_CRASH_RECOVERY
	else
	{
		writeMemoNote("[DEBUG] startSpan: ltpmeter already running", nbrBuf);
	}
#endif

	if (vspan->lsoPid == ERROR || sm_TaskExists(vspan->lsoPid) == 0)
	{
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] startSpan: Spawning udplso", nbrBuf);
#endif
		sdr_string_read(sdr, cmd, span.lsoCmd);
		isprintf(lsoCmdString, sizeof lsoCmdString, "%s %s", cmd, nbrBuf);
		vspan->lsoPid = pseudoshell(lsoCmdString);
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] startSpan: udplso spawned", nbrBuf);
#endif
	}
#ifdef DEBUG_CRASH_RECOVERY
	else
	{
		writeMemoNote("[DEBUG] startSpan: udplso already running", nbrBuf);
	}

	writeMemoNote("[DEBUG] startSpan: Returning", nbrBuf);
#endif
}

static void	startSeat(LtpVseat *vseat)
{
	if (vseat->lsiPid == ERROR || sm_TaskExists(vseat->lsiPid) == 0)
	{
		vseat->lsiPid = pseudoshell(vseat->lsiCmd);
	}
}

/*	Clean up any empty export sessions left by ltpmeter when it exits.
 *	ltpmeter creates an empty "next" export session after finishing a
 *	block, in anticipation of receiving more data. When the span is
 *	stopped, ltpmeter is killed and this empty session becomes orphaned.
 *	Empty sessions have: totalLength=0, stateFlags=0, clientSvcId=0.
 *	Removing these prevents "span has open sessions" errors during
 *	span removal.							*/

static void	cleanupEmptyExportSessions(LtpVspan *vspan)
{
	Sdr		sdr = getIonsdr();
	SdrObject	spanObj;
	LtpSpan		span;
	SdrObject	elt;
	SdrObject	nextElt;
	SdrObject	sessionObj;
	OBJ_POINTER(LtpExportSession, session);
	int		needUpdate = 0;

	CHKVOID(ionLocked());

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	/*	Iterate through export sessions, removing any that are
	 *	completely empty (created by ltpmeter but never used).	*/

	for (elt = sdr_list_first(sdr, span.exportSessions); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		sessionObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpExportSession, session, sessionObj);

		/*	Check if this is an empty, unused session:
		 *	- No data (totalLength == 0)
		 *	- No state flags set (stateFlags == 0)
		 *	- Invalid client service ID (clientSvcId == 0)
		 *	These indicate a session created by ltpmeter but
		 *	never populated with actual data.		*/

		if (session->totalLength == 0
		&& session->stateFlags == 0
		&& session->clientSvcId == 0)
		{
			/*	This is an orphaned empty session.
			 *	Clean it up to prevent blocking span
			 *	removal.				*/

#if LTPDEBUG
			writeMemo("[i] Cleaning up empty export session.");
#endif
			/*	Clear any allocated resources.		*/

			if (session->svcDataObjects)
			{
				sdr_list_destroy(sdr,
					session->svcDataObjects, NULL, NULL);
			}

			if (session->redSegments)
			{
				sdr_list_destroy(sdr,
					session->redSegments, NULL, NULL);
			}

			if (session->greenSegments)
			{
				sdr_list_destroy(sdr,
					session->greenSegments, NULL, NULL);
			}

			if (session->claims)
			{
				sdr_list_destroy(sdr,
					session->claims, NULL, NULL);
			}

			if (session->checkpoints)
			{
				sdr_list_destroy(sdr,
					session->checkpoints, NULL, NULL);
			}

			if (session->rsSerialNbrs)
			{
				sdr_list_destroy(sdr,
					session->rsSerialNbrs, NULL, NULL);
			}

			/*	Remove from exportSessions list and free.  */

			sdr_list_delete(sdr, elt, NULL, NULL);
			sdr_free(sdr, sessionObj);

			/*	If this was the current export session,
			 *	clear the pointer in the span.		*/

			if (span.currentExportSessionObj == sessionObj)
			{
				span.currentExportSessionObj = 0;
				needUpdate = 1;
			}
		}
	}

	/*	Write back the span if we modified currentExportSessionObj.	*/

	if (needUpdate)
	{
		sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	}
}

static void	stopSpan(LtpVspan *vspan)
{
	if (vspan->bufOpenRedSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufOpenRedSemaphore);
	}

	if (vspan->bufOpenGreenSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufOpenGreenSemaphore);
	}

	if (vspan->bufClosedSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->bufClosedSemaphore);
	}

	if (vspan->segSemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vspan->segSemaphore);
	}
}

static void	stopSeat(LtpVseat *vseat)
{
	if (vseat->lsiPid != ERROR)
	{
		sm_TaskKill(vseat->lsiPid, SIGTERM);
	}
}

static void	waitForSpan(LtpVspan *vspan)
{
	sm_TaskKillWait(vspan->lsoPid, "[!] ltpStop: LSO not responding to \
shutdown, sending SIGKILL.",
			NULL);
	sm_TaskKillWait(vspan->meterPid, "[!] ltpStop: ltpmeter not responding \
to shutdown, sending SIGKILL.",
			NULL);
}

static void	waitForSeat(LtpVseat *vseat)
{
	sm_TaskKillWait(vseat->lsiPid, "[!] ltpStop: LSI not responding to \
SIGTERM, sending SIGKILL.",
			NULL);
}

static char 	*_ltpvdbName(void)
{
	return "ltpvdb";
}

static LtpVdb		*_ltpvdb(char **name)
{
	static LtpVdb	*vdb = NULL;

	if (name)
	{
		if (*name == NULL)	/*	Terminating.		*/
		{
			vdb = NULL;
			return vdb;
		}

		/*	Attaching to volatile database.			*/

		PsmPartition	wm;
		PsmAddress	vdbAddress;
		PsmAddress	elt;

		wm = getIonwm();
		if (psm_locate(wm, *name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (LtpVdb *) psp(wm, vdbAddress);
			return vdb;
		}

		/*	LTP volatile database doesn't exist yet.	*/

		Sdr		sdr;
		LtpDB		*db;
		SdrObject	sdrElt;
		int		i;
		LtpVclient	*client;

		sdr = getIonsdr();
		CHKNULL(sdr_begin_xn(sdr));	/*	To lock memory.	*/

		/*	Create and catalogue the LtpVdb object.		*/

		vdbAddress = psm_zalloc(wm, sizeof(LtpVdb));
		if (vdbAddress == 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("No space for dynamic database.", NULL);
			return NULL;
		}

		db = _ltpConstants();
		vdb = (LtpVdb *) psp(wm, vdbAddress);
		memset((char *) vdb, 0, sizeof(LtpVdb));
		vdb->ownEngineId = db->ownEngineId;
		vdb->clockPid = ERROR;		/*	None yet.	*/
		vdb->delivPid = ERROR;		/*	None yet.	*/
		vdb->deliverySemaphore = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		if ((vdb->spans = sm_list_create(wm)) == 0
		|| (vdb->seats = sm_list_create(wm)) == 0
		|| psm_catlg(wm, *name, vdbAddress) < 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Can't initialize volatile database.", NULL);
			return NULL;
		}

		/*	Raise all clients.				*/

		for (i = 0, client = vdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
				i++, client++)
		{
			client->notices = db->clients[i].notices;
			raiseClient(client);
		}

		/*	Raise all spans.				*/

		for (sdrElt = sdr_list_first(sdr, db->spans);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			if (raiseSpan(sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all spans.", NULL);
				return NULL;
			}
		}

		/*	Raise all seats.				*/

		for (sdrElt = sdr_list_first(sdr, db->seats);
				sdrElt; sdrElt = sdr_list_next(sdr, sdrElt))
		{
			if (raiseSeat(sdrElt, vdb) < 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Can't raise all LSIs.", NULL);
				return NULL;
			}
		}

		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
	}

	return vdb;
}

static char	*_ltpdbName(void)
{
	return "ltpdb";
}

int	ltpInit(int estMaxExportSessions)
{
	Sdr	sdr;
	SdrObject ltpdbObject;
	IonDB	iondb;
	LtpDB	ltpdbBuf;
	int	i;
	char	*ltpvdbName = _ltpvdbName();

	if (ionAttach() < 0)
	{
		putErrmsg("LTP can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	srand(time(NULL) * sm_TaskIdSelf());

	/*	Recover the LTP database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	ltpdbObject = sdr_find(sdr, _ltpdbName(), NULL);
	switch (ltpdbObject)
	{
	case -1:		/*	SDR error.			*/
		putErrmsg("Can't search for LTP database in SDR.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		if (estMaxExportSessions <= 0)
		{
			sdr_exit_xn(sdr);
			putErrmsg("Must supply estMaxExportSessions.", NULL);
			return -1;
		}

		sdr_read(sdr, (char *) &iondb, getIonDbObject(),
				sizeof(IonDB));
		ltpdbObject = sdr_malloc(sdr, sizeof(LtpDB));
		if (ltpdbObject == 0)
		{
			putErrmsg("No space for database.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		/*	Initialize the non-volatile database.		*/

		memset((char *) &ltpdbBuf, 0, sizeof(LtpDB));
		ltpdbBuf.ownEngineId = iondb.ownFqnn;
		encodeSdnv(&(ltpdbBuf.ownEngineIdSdnv), ltpdbBuf.ownEngineId);
		ltpdbBuf.maxBacklog = 10;
		ltpdbBuf.deliverables = sdr_list_create(sdr);
		ltpdbBuf.estMaxExportSessions = estMaxExportSessions;
		ltpdbBuf.ownQtime = 1;		/*	Default.	*/
		ltpdbBuf.enforceSchedule = 1;	/*	Default.	*/
		ltpdbBuf.maxBER = DEFAULT_MAX_BER;

		/*	Initialize explicit config defaults.		*/

		ltpdbBuf.defaultMaxRetries = 5;
		ltpdbBuf.defaultMaxSegLossRate = 0.01;
		ltpdbBuf.useGlobalSplitMode = 0;  /* Unified by default. */
		ltpdbBuf.defaultMaxRetriesXmit = 5;
		ltpdbBuf.defaultMaxRetriesRecv = 5;
		ltpdbBuf.defaultMaxSegLossRateXmit = 0.01;
		ltpdbBuf.defaultMaxSegLossRateRecv = 0.01;

		for (i = 0; i < LTP_MAX_NBR_OF_CLIENTS; i++)
		{
			ltpdbBuf.clients[i].notices = sdr_list_create(sdr);
		}

		ltpdbBuf.exportSessionsHash = sdr_hash_create(sdr,
				sizeof(unsigned int), estMaxExportSessions,
				LTP_MEAN_SEARCH_LENGTH);
#if CLOSED_EXPORTS_ENABLED
		ltpdbBuf.closedExports= sdr_list_create(sdr);
		writeMemo("[i] LTP CLOSED_EXPORTS_ENABLED enhancement active.");
#endif
		ltpdbBuf.deadExports = sdr_list_create(sdr);
		ltpdbBuf.spans = sdr_list_create(sdr);
		ltpdbBuf.seats = sdr_list_create(sdr);
		ltpdbBuf.timeline = sdr_list_create(sdr);
		ltpdbBuf.maxAcqInHeap = 560;
		sdr_write(sdr, ltpdbObject, (char *) &ltpdbBuf,
				sizeof(LtpDB));
		sdr_catlg(sdr, _ltpdbName(), 0, ltpdbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create LTP database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(sdr);
	}

	oK(_ltpdbObject(&ltpdbObject));	/*	Save database location.	*/
	oK(_ltpConstants());

	/*	Load volatile database, initializing as necessary.	*/

	if (_ltpvdb(&ltpvdbName) == NULL)
	{
		putErrmsg("LTP can't initialize vdb.", NULL);
		return -1;
	}

	return 0;		/*	LTP service is available.	*/
}

static void	dropVdb(PsmPartition wm, PsmAddress vdbAddress)
{
	LtpVdb		*vdb;
	int		i;
	LtpVclient	*client;
	PsmAddress	elt;
	LtpVspan	*vspan;
	LtpVseat	*vseat;

	vdb = (LtpVdb *) psp(wm, vdbAddress);
	for (i = 0, client = vdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(client->semaphore);
			microsnooze(50000);
			sm_SemDelete(client->semaphore);
		}
	}

	while ((elt = sm_list_first(wm, vdb->spans)) != 0)
	{
		vspan = (LtpVspan *) psp(wm, sm_list_data(wm, elt));
		dropSpan(vspan, elt);
	}

	sm_list_destroy(wm, vdb->spans, NULL, NULL);
	while ((elt = sm_list_first(wm, vdb->seats)) != 0)
	{
		vseat = (LtpVseat *) psp(wm, sm_list_data(wm, elt));
		dropSeat(vseat, elt);
	}

	sm_list_destroy(wm, vdb->seats, NULL, NULL);
	if (vdb->deliverySemaphore != SM_SEM_NONE)
	{
		sm_SemEnd(vdb->deliverySemaphore);
		microsnooze(50000);
		sm_SemDelete(vdb->deliverySemaphore);
	}
}

void	ltpDropVdb(void)
{
	PsmPartition	wm = getIonwm();
	char		*ltpvdbName = _ltpvdbName();
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	char		*stop = NULL;

	/*	Destroy volatile database.				*/

	if (psm_locate(wm, ltpvdbName, &vdbAddress, &elt) < 0)
	{
		putErrmsg("Failed searching for vdb.", NULL);
		return;
	}

	if (elt)
	{
		dropVdb(wm, vdbAddress);	/*	Destroy Vdb.	*/
		psm_free(wm,vdbAddress);
		if (psm_uncatlg(wm, ltpvdbName) < 0)
		{
			putErrmsg("Failed uncataloging vdb.",NULL);
		}
	}

	oK(_ltpvdb(&stop));			/*	Forget old Vdb.	*/
}

void	ltpRaiseVdb(void)
{
	char	*ltpvdbName = _ltpvdbName();

	if (_ltpvdb(&ltpvdbName) == NULL)	/*	Create new Vdb.	*/
	{
		putErrmsg("LTP can't reinitialize vdb.", NULL);
	}
}

SdrObject getLtpDbObject(void)
{
	return _ltpdbObject(NULL);
}

LtpDB	*getLtpConstants(void)
{
	return _ltpConstants();
}

LtpVdb	*getLtpVdb(void)
{
	return _ltpvdb(NULL);
}

int	ltpStart(void)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	SdrObject	ltpdbobj = getLtpDbObject();
	LtpDB		ltpdb;
	PsmAddress	elt;
	int		i;
	LtpVclient	*client;

	sdr_read(sdr, (char *) &ltpdb, ltpdbobj, sizeof(LtpDB));
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/

	/*	Re-enable semaphores that ltpStop may have ended.
	 *	This is necessary when restarting after ionexit k n,
	 *	which preserves the volatile database in shared
	 *	memory while stopping all LTP daemon processes.		*/

	sm_SemUnend(ltpvdb->deliverySemaphore);
	for (i = 0, client = ltpvdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemUnend(client->semaphore);
		}
	}

	/*	Start the LTP events clock if necessary.		*/

	if (ltpvdb->clockPid == ERROR || sm_TaskExists(ltpvdb->clockPid) == 0)
	{
		ltpvdb->clockPid = pseudoshell("ltpclock");
	}

	/*	Start the LTP delivery daemon if necessary.		*/

	if (ltpvdb->delivPid == ERROR || sm_TaskExists(ltpvdb->delivPid) == 0)
	{
		ltpvdb->delivPid = pseudoshell("ltpdeliv");
	}

	/*	Start output link services for remote spans.		*/

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		startSpan((LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt)));
	}

	/*	Start input link services as necessary.			*/

	for (elt = sm_list_first(ltpwm, ltpvdb->seats); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		startSeat((LtpVseat *) psp(ltpwm, sm_list_data(ltpwm, elt)));
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

void	ltpStop(void)		/*	Reverses ltpStart.		*/
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	int		i;
	LtpVclient	*client;
	PsmAddress	elt;
	LtpVspan	*vspan;
	LtpVseat	*vseat;

	/*	Tell all LTP processes to stop.				*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	sm_SemEnd(ltpvdb->deliverySemaphore);
	for (i = 0, client = ltpvdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemEnd(client->semaphore);
		}
	}

	if (ltpvdb->clockPid != ERROR)
	{
		sm_TaskKill(ltpvdb->clockPid, SIGTERM);
	}

	if (ltpvdb->delivPid != ERROR)
	{
		sm_TaskKill(ltpvdb->delivPid, SIGTERM);
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		stopSpan(vspan);
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->seats); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vseat = (LtpVseat *) psp(ltpwm, sm_list_data(ltpwm, elt));
		stopSeat(vseat);
	}

	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*
	 * Wait until all LTP processes have stopped.  On some
	 * platforms (notably RTEMS) SIGTERM is not reliably delivered
	 * to snooze-based daemon threads such as ltpclock, so escalate
	 * to SIGKILL after a grace period rather than waiting forever;
	 * this mirrors the shutdown logic in bpStop().
	 */
	sm_TaskKillWait(ltpvdb->clockPid, "[!] ltpStop: ltpclock not responding \
to SIGTERM, sending SIGKILL.",
			NULL);
	sm_TaskKillWait(ltpvdb->delivPid, "[!] ltpStop: ltpdeliv not responding \
to SIGTERM, sending SIGKILL.",
			NULL);

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		waitForSpan(vspan);
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->seats); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vseat = (LtpVseat *) psp(ltpwm, sm_list_data(ltpwm, elt));
		waitForSeat(vseat);
	}

	/*	Now erase all the tasks and reset the semaphores.	*/

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	ltpvdb->clockPid = ERROR;
	ltpvdb->delivPid = ERROR;
	for (i = 0, client = ltpvdb->clients; i < LTP_MAX_NBR_OF_CLIENTS;
			i++, client++)
	{
		resetClient(client);
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		resetSpan(vspan);
	}

	for (elt = sm_list_first(ltpwm, ltpvdb->seats); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		vseat = (LtpVseat *) psp(ltpwm, sm_list_data(ltpwm, elt));
		vseat->lsiPid = ERROR;
	}

	sdr_exit_xn(sdr);		/*	Unlock memory.		*/
}

int	ltpAttach(void)
{
	SdrObject ltpdbObject = _ltpdbObject(NULL);
	LtpVdb	*ltpvdb = _ltpvdb(NULL);
	Sdr	sdr;
	char	*ltpvdbName = _ltpvdbName();

	if (ltpdbObject && ltpvdb)
	{
		return 0;		/*	Already attached.	*/
	}

	if (ionAttach() < 0)
	{
		putErrmsg("LTP can't attach to ION.", NULL);
		return -1;
	}

	sdr = getIonsdr();
	srand(time(NULL) * sm_TaskIdSelf());

	/*	Locate the LTP database.				*/

	if (ltpdbObject == 0)
	{
		CHKERR(sdr_begin_xn(sdr));
		ltpdbObject = sdr_find(sdr, _ltpdbName(), NULL);
		sdr_exit_xn(sdr);
		if (ltpdbObject == 0)
		{
			putErrmsg("Can't find LTP database.", NULL);
			return -1;
		}

		oK(_ltpdbObject(&ltpdbObject));
	}

	oK(_ltpConstants());

	/*	Locate the LTP volatile database.			*/

	if (ltpvdb == NULL)
	{
		if (_ltpvdb(&ltpvdbName) == NULL)
		{
			putErrmsg("LTP volatile database not found.", NULL);
			return -1;
		}
	}

	return 0;		/*	LTP service is available.	*/
}

void	ltpDetach(void)
{
	char	*stop = NULL;

	oK(_ltpvdb(&stop));
	return;
}

/*	*	*	LTP seat (input) mgt and access functions	*/

void	findSeat(char *lsiCmd, LtpVseat **vseat, PsmAddress *vseatElt)
{
	PsmPartition	ltpwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(ionLocked());
	CHKVOID(vseat);
	CHKVOID(vseatElt);
	CHKVOID(lsiCmd);
	for (elt = sm_list_first(ltpwm, (_ltpvdb(NULL))->seats); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		*vseat = (LtpVseat *) psp(ltpwm, sm_list_data(ltpwm, elt));
		if (strcmp((*vseat)->lsiCmd, lsiCmd) == 0)
		{
			break;
		}
	}

	*vseatElt = elt;	/*	(Zero if vseat was not found.)	*/
}

int	addSeat(char *lsiCmd)
{
	Sdr		sdr = getIonsdr();
	LtpVseat	*vseat;
	PsmAddress	vseatElt;
	LtpSeat		seatBuf;
	SdrObject	addr;
	SdrObject	seatElt = 0;

	if (lsiCmd == NULL || *lsiCmd == '\0')
	{
		writeMemoNote("[?] No LSI command, can't add LSI", lsiCmd);
		return 0;
	}

	if (strlen(lsiCmd) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Link service input command string too long",
				lsiCmd);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	findSeat(lsiCmd, &vseat, &vseatElt);
	if (vseatElt)		/*	This is a known seat.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate LSI", lsiCmd);
		return 0;
	}

	/*	All parameters validated, okay to add the seat.		*/

	memset((char *) &seatBuf, 0, sizeof(LtpSeat));
	seatBuf.lsiCmd = sdr_string_create(sdr, lsiCmd);
	addr = sdr_malloc(sdr, sizeof(LtpSeat));
	if (addr)
	{
		seatElt = sdr_list_insert_last(sdr, _ltpConstants()->seats,
				addr);
		sdr_write(sdr, addr, (char *) &seatBuf, sizeof(LtpSeat));
	}

	if (sdr_end_xn(sdr) < 0 || seatElt == 0)
	{
		putErrmsg("Can't add LSI.", lsiCmd);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raiseSeat(seatElt, _ltpvdb(NULL)) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't raise LSI.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);
	return 1;
}

int	removeSeat(char *lsiCmd)
{
	Sdr		sdr = getIonsdr();
	LtpVseat	*vseat;
	PsmAddress	vseatElt;
	SdrObject	seatElt;
	SdrObject	seatObj;
	OBJ_POINTER(LtpSeat, seat);

	/*	Must stop the seat before trying to remove it.		*/

	CHKERR(lsiCmd);
	CHKERR(sdr_begin_xn(sdr));	/*	Lock memory.		*/
	findSeat(lsiCmd, &vseat, &vseatElt);
	if (vseatElt == 0)	/*	This is an unknown seat.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown LSI", lsiCmd);
		return 0;
	}

	/*	All parameters validated.				*/

	stopSeat(vseat);
	sdr_exit_xn(sdr);
	waitForSeat(vseat);

	/*	Okay to remove this seat from the database.		*/

	CHKERR(sdr_begin_xn(sdr));
	vseat->lsiPid = ERROR;
	seatElt = vseat->seatElt;
	seatObj = (SdrObject) sdr_list_data(sdr, seatElt);
	GET_OBJ_POINTER(sdr, LtpSeat, seat, seatObj);
	dropSeat(vseat, vseatElt);
	if (seat->lsiCmd)
	{
		sdr_free(sdr, seat->lsiCmd);
	}

	sdr_free(sdr, seatObj);
	sdr_list_delete(sdr, seatElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove LSI.", lsiCmd);
		return -1;
	}

	return 1;
}

/*	*	*	LTP span (output) mgt and access functions	*/

void	findSpan(uvast engineId, LtpVspan **vspan, PsmAddress *vspanElt)
{
	PsmPartition	ltpwm = getIonwm();
	PsmAddress	elt;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	CHKVOID(vspanElt);
	for (elt = sm_list_first(ltpwm, (_ltpvdb(NULL))->spans); elt;
			elt = sm_list_next(ltpwm, elt))
	{
		*vspan = (LtpVspan *) psp(ltpwm, sm_list_data(ltpwm, elt));
		if ((*vspan)->engineId == engineId)
		{
			break;
		}
	}

	*vspanElt = elt;	/*	(Zero if vspan was not found.)	*/
}

void	checkReservationLimit(void)
{
	Sdr	sdr = getIonsdr();
	SdrObject dbobj = getLtpDbObject();
	LtpDB	db;
	int	totalSessionsAvbl;
	SdrObject elt;
	OBJ_POINTER(LtpSpan, span);

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &db, dbobj, sizeof(LtpDB));
	totalSessionsAvbl = db.estMaxExportSessions;
	for (elt = sdr_list_first(sdr, db.spans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr,
				elt));
		totalSessionsAvbl -= span->maxExportSessions;
	}

	if (totalSessionsAvbl < 0)
	{
		writeMemoNote("[?] Total max export sessions exceeds \
estimate.  Session lookup speed may be degraded", itoa(totalSessionsAvbl));
	}
	else
	{
		writeMemo("[i] Total max export sessions does not exceed \
estimate.");
	}

	sdr_exit_xn(sdr);
}

int	addSpan(uvast engineId, unsigned int maxExportSessions,
		unsigned int maxImportSessions, unsigned int maxSegmentSize,
		unsigned int aggrSizeLimit, unsigned int aggrTimeLimit,
		char *lsoCmd, unsigned int qTime, int purge)
{
	Sdr		sdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	LtpSpan		spanBuf;
	LtpSpanStats	statsInit;
	SdrObject	addr;
	SdrObject	spanElt = 0;

	if (lsoCmd == NULL || *lsoCmd == '\0')
	{
		writeMemoNote("[?] No LSO command, can't add span",
				utoa(engineId));
		return 0;
	}

	if (engineId == 0 || maxExportSessions == 0 || maxImportSessions == 0
	|| aggrSizeLimit == 0 || aggrTimeLimit == 0)
	{
		writeMemoNote("[?] Missing span parameter(s)", utoa(engineId));
		return 0;
	}

	if (strlen(lsoCmd) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Link service output command string too long",
				lsoCmd);
		return 0;
	}

#ifdef UDP_MULTISEND
	if (strlen(lsoCmd) > 2 && memcmp(lsoCmd, "udp", 3) == 0)
	{
		/*	Using UDP/IP at link service layer.		*/

		if (maxSegmentSize > MULTISEND_SEGMENT_SIZE)
		{
			maxSegmentSize = MULTISEND_SEGMENT_SIZE;
			writeMemoNote("[i] Note max segment size reduced \
to work with UDP sendmmsg()", itoa(maxSegmentSize));
		}
	}
#endif

	/*	Note: RFC791 says that IPv4 hosts cannot set maximum
	 *	IP packet length to any value less than 576 bytes (the
	 *	PPP MTU size).  IPv4 packet header length ranges from
	 *	20 to 60 bytes, and UDP header length is 8 bytes.  So
	 *	the maximum allowed size for a UDP datagram on a given
	 *	host should not be less than 508 bytes, so we warn if
	 *	maximum LTP segment size is less than 508.		*/

	if (maxSegmentSize < 508)
	{
		writeMemoNote("[i] Note max segment size is less than 508",
				utoa(maxSegmentSize));
	}

	CHKERR(sdr_begin_xn(sdr));
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt)		/*	This is a known span.		*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate span", itoa(engineId));
		return 0;
	}

	/*	All parameters validated, okay to add the span.		*/

	memset((char *) &spanBuf, 0, sizeof(LtpSpan));
	spanBuf.engineId = engineId;
	encodeSdnv(&(spanBuf.engineIdSdnv), spanBuf.engineId);
	spanBuf.remoteQtime = qTime;
	spanBuf.purge = purge ? 1 : 0;
	spanBuf.lsoCmd = sdr_string_create(sdr, lsoCmd);
	spanBuf.maxExportSessions = maxExportSessions;
	spanBuf.maxImportSessions = maxImportSessions;
	spanBuf.aggrSizeLimit = aggrSizeLimit;
	spanBuf.aggrTimeLimit = aggrTimeLimit;
	spanBuf.maxSegmentSize = maxSegmentSize;
	spanBuf.exportSessions = sdr_list_create(sdr);
	spanBuf.segments = sdr_list_create(sdr);
	spanBuf.importBuffers = sdr_list_create(sdr);
	spanBuf.importSessions = sdr_list_create(sdr);
	spanBuf.importSessionsHash = sdr_hash_create(sdr,
			sizeof(unsigned int), maxImportSessions,
			LTP_MEAN_SEARCH_LENGTH);
	spanBuf.closedImports = sdr_list_create(sdr);
	spanBuf.deadImports = sdr_list_create(sdr);
	spanBuf.stats = sdr_malloc(sdr, sizeof(LtpSpanStats));
	if (spanBuf.stats)
	{
		memset((char *) &statsInit, 0, sizeof(LtpSpanStats));
		sdr_write(sdr, spanBuf.stats, (char *) &statsInit,
				sizeof(LtpSpanStats));
	}

	spanBuf.updateStats = 1;	/*	Default.		*/
	addr = sdr_malloc(sdr, sizeof(LtpSpan));
	if (addr)
	{
		spanElt = sdr_list_insert_last(sdr, _ltpConstants()->spans,
				addr);
		sdr_write(sdr, addr, (char *) &spanBuf, sizeof(LtpSpan));
	}

	if (sdr_end_xn(sdr) < 0 || spanElt == 0)
	{
		putErrmsg("Can't add span.", itoa(engineId));
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	if (raiseSpan(spanElt, _ltpvdb(NULL)) < 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't raise span.", NULL);
		return -1;
	}

	sdr_exit_xn(sdr);
	return 1;
}

int	updateSpan(uvast engineId, unsigned int maxExportSessions,
		unsigned int maxImportSessions, unsigned int maxSegmentSize,
		unsigned int aggrSizeLimit, unsigned int aggrTimeLimit,
		char *lsoCmd, unsigned int qTime, int purge)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	SdrObject	addr;
	LtpSpan		spanBuf;

	if (lsoCmd)
	{
		if (*lsoCmd == '\0')
		{
			writeMemoNote("[?] No LSO command, can't update span",
					utoa(engineId));
			return 0;
		}
		else
		{
			if (strlen(lsoCmd) > MAX_SDRSTRING)
			{
				writeMemoNote("[?] Link service output command \
string too long.", lsoCmd);
				return 0;
			}
		}
	}

#ifdef UDP_MULTISEND
	if (strlen(lsoCmd) > 2 && memcmp(lsoCmd, "udp", 3) == 0)
	{
		/*	Using UDP/IP at link service layer.		*/

		if (maxSegmentSize > MULTISEND_SEGMENT_SIZE)
		{
			maxSegmentSize = MULTISEND_SEGMENT_SIZE;
			writeMemoNote("[i] Note max segment size reduced \
to work with UDP sendmmsg()", itoa(maxSegmentSize));
		}
	}
#endif

	if (maxSegmentSize)
	{
		if (maxSegmentSize < 508)
		{
			writeMemoNote("[i] Note max segment size is less than \
508", utoa(maxSegmentSize));
		}
	}

	CHKERR(sdr_begin_xn(sdr));
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	addr = (SdrObject) sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &spanBuf, addr, sizeof(LtpSpan));
	if (maxExportSessions == 0)
	{
		maxExportSessions = spanBuf.maxExportSessions;
	}

	if (maxImportSessions == 0)
	{
		maxImportSessions = spanBuf.maxImportSessions;
	}

	if (aggrSizeLimit == 0)
	{
		aggrSizeLimit = spanBuf.aggrSizeLimit;
	}

	if (aggrTimeLimit == 0)
	{
		aggrTimeLimit = spanBuf.aggrTimeLimit;
	}

	/*	All parameters validated, okay to update the span.	*/

	if (maxSegmentSize > 0 && maxSegmentSize != spanBuf.maxSegmentSize)
	{
		/*	Never shrink segment buffer, as it has to be
		 *	large enough for currently queued segments,
		 *	but expand it as necessary.			*/

		if (maxSegmentSize > spanBuf.maxSegmentSize)
		{
			psm_free(ltpwm, vspan->segmentBuffer);
			vspan->segmentBuffer = psm_malloc(ltpwm,
					maxSegmentSize);
			if (vspan->segmentBuffer == 0)
			{
				sdr_exit_xn(sdr);
				putErrmsg("No space for new segment buffer.",
						itoa(maxSegmentSize));
				return -1;
			}
		}

		spanBuf.maxSegmentSize = maxSegmentSize;
		vspan->maxXmitSegSize = maxSegmentSize;
		computeRetransmissionLimits(vspan);
	}

	spanBuf.maxExportSessions = maxExportSessions;
	spanBuf.maxImportSessions = maxImportSessions;
	if (lsoCmd)
	{
		if (spanBuf.lsoCmd)
		{
			sdr_free(sdr, spanBuf.lsoCmd);
		}

		spanBuf.lsoCmd = sdr_string_create(sdr, lsoCmd);
	}

	spanBuf.remoteQtime = qTime;
	spanBuf.purge = purge ? 1 : 0;
	spanBuf.aggrSizeLimit = aggrSizeLimit;
	if (aggrTimeLimit)
	{
		spanBuf.aggrTimeLimit = aggrTimeLimit;
	}

	sdr_write(sdr, addr, (char *) &spanBuf, sizeof(LtpSpan));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update span.", itoa(engineId));
		return -1;
	}

	return 1;
}

static void releaseImportBuffer(Sdr sdr, SdrObject elt, void *arg)
{
	/* Parameter intentionally unused. */
	(void)arg;

	sdr_free(sdr, sdr_list_data(sdr, elt));
}

static void	cancelForgetImportEvent(SdrObject listElt);

int	removeSpan(uvast engineId)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ltpwm = getIonwm();
	LtpVdb		*ltpvdb = getLtpVdb();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	SdrObject	spanElt;
	SdrObject	spanObj;
	SdrObject	elt;
	OBJ_POINTER(LtpSpan, span);

	/*	Must stop the span before trying to remove it.		*/

	CHKERR(sdr_begin_xn(sdr));	/*	Lock memory.		*/
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	/*	All parameters validated.				*/

	/*	CRITICAL: Destroy avblIdxRbts BEFORE calling stopSpan().
	 *	When stopSpan() ends the buffer semaphores, their IDs may
	 *	accidentally match the lock semaphore IDs of RBTs in the
	 *	avblIdxRbts list, causing corruption. Destroy these RBTs
	 *	now while all semaphores are still valid.		*/
	{
#if LTP_DEBUG
		PsmAddress	elt;
		int		rbtCount = 0;
		char		msgBuf[256];
		typedef struct { PsmAddress userData; PsmAddress root; size_t length; sm_SemId lock; } SmRbt;

		/*	Diagnostic: Log buffer semaphore IDs		*/
		isprintf(msgBuf, sizeof msgBuf,
			"[DEBUG] removeSpan: Buffer semaphore IDs: red=%d green=%d closed=%d seg=%d",
			vspan->bufOpenRedSemaphore,
			vspan->bufOpenGreenSemaphore,
			vspan->bufClosedSemaphore,
			vspan->segSemaphore);
		writeMemo(msgBuf);

		/*	Diagnostic: Log RBT count and semaphore IDs	*/
		for (elt = sm_list_first(ltpwm, vspan->avblIdxRbts); elt;
				elt = sm_list_next(ltpwm, elt))
		{
			PsmAddress	rbtAddr = sm_list_data(ltpwm, elt);
			SmRbt		*rbt = (SmRbt *) psp(ltpwm, rbtAddr);
			rbtCount++;
			isprintf(msgBuf, sizeof msgBuf,
				"[DEBUG] removeSpan: avblIdxRbt #%d at 0x%lx, lock semaphore ID=%d",
				rbtCount, (unsigned long)rbtAddr, rbt->lock);
			writeMemo(msgBuf);
		}

		isprintf(msgBuf, sizeof msgBuf,
			"[DEBUG] removeSpan: About to destroy %d RBTs in avblIdxRbts",
			rbtCount);
		writeMemo(msgBuf);
#endif

		sm_list_destroy(ltpwm, vspan->avblIdxRbts,
				deleteIdxRbt, NULL);
		vspan->avblIdxRbts = 0;	/* Mark as destroyed */

#if LTP_DEBUG
		writeMemo("[DEBUG] removeSpan: Successfully destroyed avblIdxRbts");
#endif
	}

	stopSpan(vspan);

	/*	Copy PIDs to local variables before unlocking memory.
	 *	This prevents use-after-unlock bug where vspan could
	 *	be modified or freed by another process after unlock.	*/

	{
		int	lsoPid = vspan->lsoPid;
		int	meterPid = vspan->meterPid;

		sdr_exit_xn(sdr);

		/*	Wait for tasks to terminate using local PID copies.
		 *	Safe to access local variables after unlock.	*/

		if (lsoPid != ERROR)
		{
			while (sm_TaskExists(lsoPid))
			{
				microsnooze(100000);
			}
		}

		if (meterPid != ERROR)
		{
			while (sm_TaskExists(meterPid))
			{
				microsnooze(100000);
			}
		}
	}

	CHKERR(sdr_begin_xn(sdr));
	cleanupEmptyExportSessions(vspan);	/*	Remove orphans.	*/
	spanElt = vspan->spanElt;
	spanObj = (SdrObject) sdr_list_data(sdr, spanElt);
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);
	/*	cleanupEmptyExportSessions() above modified SDR state, so the
	 *	guard-fail paths below must use sdr_cancel_xn() to discard
	 *	those modifications. Calling sdr_exit_xn() here would trip
	 *	handleUnrecoverableError() (sdr_exit_xn is read-only-only)
	 *	and abort the process. */

	if (sdr_list_length(sdr, span->segments) != 0)
	{
		writeMemoNote("[?] Span has backlog, can't be removed",
				itoa(engineId));
		sdr_cancel_xn(sdr);
		return 0;
	}

	if (sdr_list_length(sdr, span->importSessions) != 0
	|| sdr_list_length(sdr, span->exportSessions) != 0)
	{
		writeMemoNote("[?] Span has open sessions, can't be removed",
				itoa(engineId));
		sdr_cancel_xn(sdr);
		return 0;
	}

	if (sdr_list_length(sdr, span->deadImports) != 0)
	{
		writeMemoNote("[?] Span has canceled sessions, can't be \
removed yet.", itoa(engineId));
		sdr_cancel_xn(sdr);
		return 0;
	}

	/*	closedImports holds only integer session-number tombstones
	 *	whose underlying import sessions and heap buffers have
	 *	already been released; their only purpose is to swallow
	 *	late-arriving segments for already-completed sessions on
	 *	the *live* span.  Once the span is gone, the inbound
	 *	machinery rejects segments earlier and the tombstones
	 *	protect nothing -- so we destroy the list unconditionally
	 *	here instead of waiting for ltpclock to forget every entry.
	 *
	 *	But each tombstone has a pending LtpForgetImportSession
	 *	timeline event that holds the tombstone's list-element
	 *	handle in event.parm; if those events outlive the list,
	 *	their later sdr_list_delete fires on freed memory and
	 *	aborts ltpclock via _xniEnd -> crashXn.  Cancel them
	 *	before destroying closedImports.			*/

	for (elt = sdr_list_first(sdr, span->closedImports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		cancelForgetImportEvent(elt);
	}

	/*	Okay to remove this span from the database.		*/

	dropSpan(vspan, vspanElt);
	if (span->lsoCmd)
	{
		sdr_free(sdr, span->lsoCmd);
	}

	sdr_list_destroy(sdr, span->exportSessions, NULL, NULL);
	sdr_list_destroy(sdr, span->segments, NULL, NULL);
	sdr_list_destroy(sdr, span->importBuffers, releaseImportBuffer, NULL);
	sdr_list_destroy(sdr, span->importSessions, NULL, NULL);
	sdr_hash_destroy(sdr, span->importSessionsHash);
	sdr_list_destroy(sdr, span->closedImports, NULL, NULL);
	sdr_list_destroy(sdr, span->deadImports, NULL, NULL);
	sdr_free(sdr, spanObj);
	sdr_list_delete(sdr, spanElt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove span.", itoa(engineId));
		return -1;
	}

	/*	Check if this was the last span. If so, stop LTP convergence
	 *	layer outducts (ltpclo) to prevent them from crashing when
	 *	they try to transmit to non-existent spans.
	 *
	 *	This is safer than leaving ltpclo running with no spans:
	 *	- ltpclo will crash on next transmission attempt (ltp_send
	 *	  returns -1 for "destination unknown")
	 *	- In multi-span scenarios, this crash affects ALL spans
	 *	- BP will safely buffer bundles in the outduct queue until
	 *	  ltpclo is restarted (via bp_start_outduct or add_span)
	 *
	 *	Note: We don't call bp_stop_outduct here because:
	 *	1. LTP library shouldn't depend on BP library (layering)
	 *	2. Outduct names vary by configuration (can't hardcode)
	 *	3. User/application should manage convergence layer lifecycle
	 *
	 *	Instead, we log a warning message to guide operators.
	 */
	if (sm_list_length(ltpwm, ltpvdb->spans) == 0)
	{
		writeMemoNote("[?] Last LTP span removed",
			"Consider stopping LTP convergence layer outducts \
(ltpclo) to prevent transmission errors. Bundles will be safely buffered in BP \
until outducts are restarted.");
	}

	return 1;
}

int	ltpStartSpan(uvast engineId)
{
	Sdr		sdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	int		result = 1;
	int		meterCrashed = 0;
	int		lsoCrashed = 0;

#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] ltpStartSpan: Entry for engine", itoa(engineId));
#endif
	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] ltpStartSpan: SDR locked", itoa(engineId));
#endif
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return 0;
	}

	/*	Only call resetSpan if ltpmeter or lso crashed (has a PID
	 *	but task no longer exists). Don't call it on first start
	 *	since raiseSpan() already called it during add_span(), or
	 *	after explicit stop since ltpStopSpan() already called it.
	 *	Calling resetSpan() when daemons are running creates a race
	 *	condition where the daemons can grab semaphores between
	 *	sm_SemGive() and sm_SemTake().				*/

	if (vspan->meterPid != ERROR && sm_TaskExists(vspan->meterPid) == 0)
	{
		meterCrashed = 1;
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] ltpStartSpan: ltpmeter crashed", itoa(engineId));
#endif
	}
#ifdef DEBUG_CRASH_RECOVERY
	else if (vspan->meterPid != ERROR && sm_TaskExists(vspan->meterPid) != 0)
	{
		writeMemoNote("[DEBUG] ltpStartSpan: ltpmeter is running", itoa(engineId));
	}
#endif

	if (vspan->lsoPid != ERROR && sm_TaskExists(vspan->lsoPid) == 0)
	{
		lsoCrashed = 1;
#ifdef DEBUG_CRASH_RECOVERY
		writeMemoNote("[DEBUG] ltpStartSpan: udplso crashed", itoa(engineId));
#endif
	}
#ifdef DEBUG_CRASH_RECOVERY
	else if (vspan->lsoPid != ERROR && sm_TaskExists(vspan->lsoPid) != 0)
	{
		writeMemoNote("[DEBUG] ltpStartSpan: udplso is running", itoa(engineId));
	}
#endif

	/*	Only reset if BOTH daemons are stopped. If either daemon
	 *	is still running, calling resetSpan() creates a race where
	 *	the running daemon can grab semaphores between Give and Take.	*/

	if ((meterCrashed || vspan->meterPid == ERROR) &&
	    (lsoCrashed || vspan->lsoPid == ERROR))
	{
		/*	Both daemons stopped - safe to reset if crash detected */
		if (meterCrashed || lsoCrashed)
		{
#ifdef DEBUG_CRASH_RECOVERY
			writeMemoNote("[DEBUG] ltpStartSpan: Both daemons stopped, calling resetSpan", itoa(engineId));
#endif
			resetSpan(vspan);
		}
#ifdef DEBUG_CRASH_RECOVERY
		else
		{
			writeMemoNote("[DEBUG] ltpStartSpan: Skipping resetSpan (first start)", itoa(engineId));
		}
#endif
	}
#ifdef DEBUG_CRASH_RECOVERY
	else
	{
		writeMemoNote("[DEBUG] ltpStartSpan: Skipping resetSpan (daemon still running)", itoa(engineId));
	}

	writeMemoNote("[DEBUG] ltpStartSpan: About to call startSpan", itoa(engineId));
#endif
	startSpan(vspan);
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] ltpStartSpan: startSpan returned", itoa(engineId));
#endif
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
#ifdef DEBUG_CRASH_RECOVERY
	writeMemoNote("[DEBUG] ltpStartSpan: SDR unlocked, returning", itoa(engineId));
#endif
	return result;
}

void	ltpStopSpan(uvast engineId)
{
	Sdr		sdr = getIonsdr();
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	int		lsoPid;
	int		meterPid;

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	This is an unknown span.	*/
	{
		sdr_exit_xn(sdr);	/*	Unlock memory.		*/
		writeMemoNote("[?] Unknown span", itoa(engineId));
		return;
	}

	stopSpan(vspan);

	/*	Copy PIDs to local variables before unlocking memory.
	 *	This prevents use-after-unlock bug where vspan could
	 *	be modified or freed by another process after unlock.	*/

	lsoPid = vspan->lsoPid;
	meterPid = vspan->meterPid;
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/

	/*	Wait for tasks to terminate using local PID copies.
	 *	Safe to access local variables after unlock.		*/

	if (lsoPid != ERROR)
	{
		while (sm_TaskExists(lsoPid))
		{
			microsnooze(100000);
		}
	}

	if (meterPid != ERROR)
	{
		while (sm_TaskExists(meterPid))
		{
			microsnooze(100000);
		}
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	resetSpan(vspan);
	cleanupEmptyExportSessions(vspan);	/*	Remove orphans.	*/
	sdr_end_xn(sdr);	/*	Unlock memory, commit changes.	*/
}

int startExportSession(Sdr sdr, SdrObject spanObj, LtpVspan *vspan)
{
	SdrObject		dbobj;
	LtpSpan			span;
	LtpDB			ltpdb;
	unsigned int		sessionNbr;
	SdrObject		sessionObj;
	SdrObject		elt;
	LtpExportSession	session;

	CHKERR(vspan);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	/*	Get next session number.				*/

	dbobj = getLtpDbObject();
	sdr_stage(sdr, (char *) &ltpdb, dbobj, sizeof(LtpDB));
	ltpdb.sessionCount++;
	sdr_write(sdr, dbobj, (char *) &ltpdb, sizeof(LtpDB));
	sessionNbr = ltpdb.sessionCount;

	/*	Record the session object in the database. The
	 *	exportSessions list element points to the session
	 *	structure.  exportSessionHash entry points to the
	 *	list element.						*/

	sessionObj = sdr_malloc(sdr, sizeof(LtpExportSession));
	if (sessionObj == 0
	|| (elt = sdr_list_insert_last(sdr, span.exportSessions,
			sessionObj)) == 0
	|| sdr_hash_insert(sdr, ltpdb.exportSessionsHash,
			(char *) &sessionNbr, elt, NULL) < 0)
	{
		putErrmsg("Can't start session.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Populate session object in database.			*/

	memset((char *) &session, 0, sizeof(LtpExportSession));
	session.span = spanObj;
	session.sessionNbr = sessionNbr;
	encodeSdnv(&(session.sessionNbrSdnv), session.sessionNbr);
	session.svcDataObjects = sdr_list_create(sdr);
	session.redSegments = sdr_list_create(sdr);
	session.greenSegments = sdr_list_create(sdr);
	session.claims = sdr_list_create(sdr);
	session.checkpoints = sdr_list_create(sdr);
	session.rsSerialNbrs = sdr_list_create(sdr);
	sdr_write(sdr, sessionObj, (char *) &session, sizeof(LtpExportSession));

	/*	Note session address in span, then finish: unless span
	 *	is currently inactive (i.e., localXmitRate is currently
	 *	zero), give the buffer-empty semaphore so that the
	 *	pending service data object (if any) can be inserted
	 *	into the buffer.					*/

	span.currentExportSessionObj = sessionObj;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	if (vspan->localXmitRate > 0)
	{
		sm_SemGive(vspan->bufOpenRedSemaphore);
		sm_SemGive(vspan->bufOpenGreenSemaphore);
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't start session.", NULL);
		return -1;
	}

	return 0;
}

/*	*	*	LTP event mgt and access functions	*	*/

static SdrObject insertLtpTimelineEvent(LtpEvent *newEvent)
{
	Sdr	sdr = getIonsdr();
	LtpDB	*ltpConstants = _ltpConstants();
	SdrObject eventObj;
	SdrObject elt;
	OBJ_POINTER(LtpEvent, event);

	CHKZERO(ionLocked());
	eventObj = sdr_malloc(sdr, sizeof(LtpEvent));
	if (eventObj == 0)
	{
		putErrmsg("No space for timeline event.", NULL);
		return 0;
	}

	/*	Search list from newest to oldest, insert after last
		event with scheduled time less than or equal to that
		of the new event.					*/

	sdr_write(sdr, eventObj, (char *) newEvent, sizeof(LtpEvent));
	for (elt = sdr_list_last(sdr, ltpConstants->timeline); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, LtpEvent, event, sdr_list_data(sdr,
				elt));
		if (event->scheduledTime <= newEvent->scheduledTime)
		{
			return sdr_list_insert_after(sdr, elt, eventObj);
		}
	}

	return sdr_list_insert_first(sdr, ltpConstants->timeline, eventObj);
}

static void	cancelEvent(LtpEventType type, uvast refNbr1,
			unsigned int refNbr2, unsigned int refNbr3)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject eventObj;
	OBJ_POINTER(LtpEvent, event);

	for (elt = sdr_list_first(sdr, (_ltpConstants())->timeline); elt;
			elt = sdr_list_next(sdr, elt))
	{
		eventObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpEvent, event, eventObj);
		if (event->type == type && event->refNbr1 == refNbr1
		&& event->refNbr2 == refNbr2 && event->refNbr3 == refNbr3)
		{
			sdr_free(sdr, eventObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			return;
		}
	}
}

/*	cancelForgetImportEvent removes the LtpForgetImportSession
 *	timeline event that references the given closedImports list
 *	element.  Used by removeSpan() to drop pending forget events
 *	before destroying the closedImports list -- without this, the
 *	event's later attempt to sdr_list_delete a freed element would
 *	hit _xniEnd and abort ltpclock (the "race already handled"
 *	comment in ltpclock.c is too optimistic; _xniEnd calls crashXn
 *	and, with the default _coreFileNeeded(), sm_Abort).  Unlike the
 *	other LtpEvent kinds, LtpForgetImportSession does not encode its
 *	target in refNbr1/2/3, so cancelEvent above cannot find it; we
 *	match on event.parm instead.  At most one event references a
 *	given listElt, so we return on first match.			*/

static void cancelForgetImportEvent(SdrObject listElt)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject eventObj;
	OBJ_POINTER(LtpEvent, event);

	for (elt = sdr_list_first(sdr, (_ltpConstants())->timeline); elt;
			elt = sdr_list_next(sdr, elt))
	{
		eventObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpEvent, event, eventObj);
		if (event->type == LtpForgetImportSession
		&& event->parm == listElt)
		{
			sdr_free(sdr, eventObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			return;
		}
	}
}

/*	*	*	Inactivity event functions	*	*	*/

static int setInactivityEvent(LtpImportSession *session, SdrObject sessionObj,
		LtpVspan *vspan)
{
	Sdr		sdr = getIonsdr();
	time_t		currentTime;
	LtpEvent	event;
	SdrObject	eventObj;

	CHKERR(ionLocked());

	/*	Cancel any existing inactivity event first.		*/

	if (session->inactivityEvent != 0)
	{
		cancelEvent(LtpStaleImportSession, 0, session->sessionNbr, 0);
		session->inactivityEvent = 0;
	}

	/*	If inactivity limit is enabled, schedule new event.	*/

	if (vspan->sessionInactivityLimit > 0)
	{
		currentTime = getCtime();
		event.type = LtpStaleImportSession;
		event.refNbr1 = vspan->engineId;
		event.refNbr2 = session->sessionNbr;
		event.refNbr3 = 0;
		event.parm = 0;
		event.scheduledTime = currentTime
				+ vspan->sessionInactivityLimit;
		eventObj = insertLtpTimelineEvent(&event);
		if (eventObj == 0)
		{
			putErrmsg("Can't set inactivity event.", NULL);
			return -1;
		}

		session->inactivityEvent = eventObj;
	}

	sdr_write(sdr, sessionObj, (char *) session,
			sizeof(LtpImportSession));
	return 0;
}

/*	*	*	LTP client mgt and access functions	*	*/

int	ltpAttachClient(unsigned int clientSvcId)
{
	Sdr		sdr = getIonsdr();
	LtpVclient	*client;

	if (clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		putErrmsg("Client svc number over limit.", itoa(clientSvcId));
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	client = (_ltpvdb(NULL))->clients + clientSvcId;
	if (client->pid != ERROR)
	{
		if (sm_TaskExists(client->pid))
		{
			sdr_exit_xn(sdr);
			if (client->pid == sm_TaskIdSelf())
			{
				return 0;
			}

			putErrmsg("Client service already in use.",
					itoa(clientSvcId));
			return -1;
		}

		/*	Application terminated without closing the
		 *	endpoint, so simply close it now.		*/

		client->pid = ERROR;
	}

	client->pid = sm_TaskIdSelf();
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
	return 0;
}

void	ltpDetachClient(unsigned int clientSvcId)
{
	Sdr		sdr = getIonsdr();
	LtpVclient	*client;

	if (clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	client = (_ltpvdb(NULL))->clients + clientSvcId;
	if (client->pid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("Can't close: not owner of client service.", NULL);
		return;
	}

	client->pid = -1;
	sdr_exit_xn(sdr);	/*	Unlock memory.			*/
}

/*	*	*	Service interface functions	*	*	*/

int	enqueueNotice(LtpVclient *client, uvast sourceEngineId,
		unsigned int sessionNbr, unsigned int dataOffset,
		unsigned int dataLength, LtpNoticeType type,
		unsigned char reasonCode, unsigned char endOfBlock,
		SdrObject data)
{
	Sdr		sdr = getIonsdr();
	SdrObject	noticeObj;
	LtpNotice	notice;

	CHKERR(client);
	if (client->pid == ERROR)
	{
		return 0;	/*	No client task to report to.	*/
	}

	CHKERR(ionLocked());
	noticeObj = sdr_malloc(sdr, sizeof(LtpNotice));
	if (noticeObj == 0)
	{
		return -1;
	}

	if (sdr_list_insert_last(sdr, client->notices, noticeObj) == 0)
	{
		return -1;
	}

	/*	Clear the whole notice, not just its fields: the struct
	 *	has alignment padding that no assignment below covers,
	 *	and the sdr_write stores sizeof(LtpNotice) bytes, so
	 *	that padding would carry uninitialised stack into the
	 *	heap.							*/

	memset((char *) &notice, 0, sizeof notice);
	notice.sessionId.sourceEngineId = sourceEngineId;
	notice.sessionId.sessionNbr = sessionNbr;
	notice.dataOffset = dataOffset;
	notice.dataLength = dataLength;
	notice.type = type;
	notice.reasonCode = reasonCode;
	notice.endOfBlock = endOfBlock;
	notice.data = data;
	sdr_write(sdr, noticeObj, (char *) &notice, sizeof(LtpNotice));

	/*	Tell client that a notice is waiting.			*/

	sm_SemGive(client->semaphore);
	return 0;
}

/*	*	*	Session management functions	*	*	*/

static void getExportSession(unsigned int sessionNbr, SdrObject *sessionObj)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;

	CHKVOID(ionLocked());
	if (sdr_hash_retrieve(sdr, (_ltpConstants())->exportSessionsHash,
			    (char *) &sessionNbr, (SdrAddress *) &elt, NULL) == 1)
	{
		*sessionObj = sdr_list_data(sdr, elt);
		return;
	}

	/*	Unknown session.					*/

	*sessionObj = 0;
}

static void getCanceledExport(unsigned int sessionNbr, SdrObject *sessionObj,
		SdrObject *sessionElt)
{
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(LtpExportSession, session);
	SdrObject elt;
	SdrObject obj;

	CHKVOID(ionLocked());
	for (elt = sdr_list_first(sdr, (_ltpConstants())->deadExports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpExportSession, session, obj);
		if (session->sessionNbr == sessionNbr)
		{
			*sessionObj = obj;
			*sessionElt = elt;
			return;
		}
	}

	/*	Not a known canceled export session.			*/

	*sessionObj = 0;
	*sessionElt = 0;
}

static void destroyDataXmitSeg(SdrObject dsElt, SdrObject dsObj, LtpXmitSeg *ds)
{
	Sdr	sdr = getIonsdr();

	CHKVOID(ionLocked());
	if (ds->pdu.ckptSerialNbr != 0)	/*	A checkpoint segment.	*/
	{
		cancelEvent(LtpResendCheckpoint, 0, ds->sessionNbr,
				ds->pdu.ckptSerialNbr);
	}

	if (ds->pdu.timer.expirationCount != -1)	/*	(burst)	*/
	{
		if (ds->ckptListElt)	/*	A checkpoint segment.	*/
		{
			/*	Destroy LtpCkpt object and its ListElt.	*/

			sdr_free(sdr, sdr_list_data(sdr, ds->ckptListElt));
			sdr_list_delete(sdr, ds->ckptListElt, NULL, NULL);
		}
	}

	if (ds->queueListElt)	/*	Queued for transmission.	*/
	{
		sdr_free(sdr, sdr_list_data(sdr, ds->queueListElt));
		sdr_list_delete(sdr, ds->queueListElt, NULL, NULL);
	}

	if (ds->pdu.headerExtensions)
	{
		sdr_list_destroy(sdr, ds->pdu.headerExtensions,
				ltpei_destroy_extension, NULL);
	}

	if (ds->pdu.trailerExtensions)
	{
		sdr_list_destroy(sdr, ds->pdu.trailerExtensions,
				ltpei_destroy_extension, NULL);
	}

	sdr_free(sdr, dsObj);
	sdr_list_delete(sdr, dsElt, NULL, NULL);
}

static void	stopExportSession(LtpExportSession *session)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject segObj;
	OBJ_POINTER(LtpXmitSeg, ds);

	CHKVOID(ionLocked());
	if (session->redSegments)
	{
		while ((elt = sdr_list_first(sdr, session->redSegments)) != 0)
		{
			segObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, LtpXmitSeg, ds, segObj);
			destroyDataXmitSeg(elt, segObj, ds);
		}
	}

	if (session->greenSegments)
	{
		while ((elt = sdr_list_first(sdr, session->greenSegments)) != 0)
		{
			segObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, LtpXmitSeg, ds, segObj);
			destroyDataXmitSeg(elt, segObj, ds);
		}
	}
}

static void destroySdrListData(Sdr sdr, SdrObject elt, void *arg)
{
/* Parameter intentionally unused. */
(void)arg;

	sdr_free(sdr, sdr_list_data(sdr, elt));
}

static void	clearExportSession(LtpExportSession *session)
{
	Sdr	sdr = getIonsdr();
	int	claimCount;

	sdr_list_destroy(sdr, session->checkpoints, destroySdrListData, NULL);
	session->checkpoints = 0;
	sdr_list_destroy(sdr, session->rsSerialNbrs, NULL, NULL);
	session->rsSerialNbrs = 0;
	sdr_list_destroy(sdr, session->redSegments, NULL, NULL);
	session->redSegments = 0;
	sdr_list_destroy(sdr, session->greenSegments, NULL, NULL);
	session->greenSegments = 0;
	claimCount = sdr_list_length(sdr, session->claims);
	if (session->redPartLength == 0 && claimCount > 0)
	{
		writeMemoNote("[?] Investigate: LTP all-Green session has \
reception claims", itoa(claimCount));
	}

	sdr_list_destroy(sdr, session->claims, destroySdrListData, NULL);
	session->claims = 0;
}

static void closeExportSession(SdrObject sessionObj)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	SdrObject	dbobj = getLtpDbObject();
	OBJ_POINTER(LtpExportSession, session);
	OBJ_POINTER(LtpSpan, span);
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	LtpDB		db;
	SdrObject	elt;
	SdrObject	sdu; /* A ZcoRef object. */

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(sdr, LtpExportSession, session, sessionObj);
	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);
	findSpan(span->engineId, &vspan, &vspanElt);
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(LtpDB));

	/*	Note that cancellation of an export session causes
	 *	the block's service data objects to be passed up to
	 *	the user in LtpExportSessionCanceled notices, destroys
	 *	the svcDataObjects list, and sets the svcDataObjects
	 *	list variable in the session object to zero.  In that
	 *	event, review of the service data objects in this
	 *	function is foregone.					*/

	if (session->svcDataObjects)
	{
		for (elt = sdr_list_first(sdr, session->svcDataObjects); elt;
				elt = sdr_list_next(sdr, elt))
		{
			sdu = sdr_list_data(sdr, elt);

			/*	All service data units are passed back
			 *	up to the client, in either Complete or
			 *	Canceled notices, and the client is
			 *	responsible for destroying them, so
			 *	we don't zco_destroy them here.		*/

			if (enqueueNotice(ltpvdb->clients
					+ session->clientSvcId, db.ownEngineId,
					session->sessionNbr, 0, 0,
					LtpExportSessionComplete, 0, 0, sdu)
					< 0)
			{
				putErrmsg("Can't post ExportSessionComplete \
notice.", NULL);
				sdr_cancel_xn(sdr);
				return;
			}
		}

		sdr_write(sdr, dbobj, (char *) &db, sizeof(LtpDB));
		sdr_list_destroy(sdr, session->svcDataObjects, NULL, NULL);
	}

	clearExportSession(session);

	/*	Finally erase the session itself, reducing the session
	 *	list length and thereby possibly enabling a blocked
	 *	client to append an SDU to the current block.		*/

	if (sdr_hash_remove(sdr, db.exportSessionsHash,
			    (char *) &(session->sessionNbr),
			    (SdrAddress *) &elt) > 0)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	sdr_free(sdr, sessionObj);
#if LTPDEBUG
putErrmsg("Closed export session.", itoa(session->sessionNbr));
#endif
	if (vspanElt == 0)
	{
		putErrmsg("Can't find vspan for engine.", utoa(span->engineId));
	}
	else
	{
		sm_SemGive(vspan->bufOpenRedSemaphore);
		sm_SemGive(vspan->bufOpenGreenSemaphore);
	}
}

static int	orderImportSessions(PsmPartition wm, PsmAddress nodeData,
			void *dataBuffer)
{
	LtpVImportSession	*argSession;
	LtpVImportSession	*nodeSession;

	argSession = (LtpVImportSession *) dataBuffer;
	nodeSession = (LtpVImportSession *) psp(wm, nodeData);
	if (nodeSession->sessionNbr < argSession->sessionNbr)
	{
		return -1;
	}

	if (nodeSession->sessionNbr > argSession->sessionNbr)
	{
		return 1;
	}

	return 0;
}

static PsmAddress	getIdxRbt(PsmPartition ltpwm, LtpVspan *vspan)
{
#if LTP_DEBUG
	typedef struct { PsmAddress userData; PsmAddress root; size_t length; sm_SemId lock; } SmRbt;
#endif
	PsmAddress	elt;
	PsmAddress	rbt;
#if LTP_DEBUG
	SmRbt		*rbtPtr;
	char		msgBuf[256];
#endif

	elt = sm_list_first(ltpwm, vspan->avblIdxRbts);
	if (elt)	/*	Reuse previously created RBT.		*/
	{
		rbt = sm_list_data(ltpwm, elt);
#if LTP_DEBUG
		rbtPtr = (SmRbt *) psp(ltpwm, rbt);

		isprintf(msgBuf, sizeof msgBuf,
			"[LTP_DEBUG] getIdxRbt: Reusing RBT at 0x%lx with lock semaphore ID=%d from avblIdxRbts pool",
			(unsigned long)rbt, rbtPtr->lock);
		writeMemo(msgBuf);
#endif

		sm_list_delete(ltpwm, elt, NULL, NULL);
		return rbt;
	}

	rbt = sm_rbt_create(ltpwm);
#if LTP_DEBUG
	rbtPtr = (SmRbt *) psp(ltpwm, rbt);

	isprintf(msgBuf, sizeof msgBuf,
		"[LTP_DEBUG] getIdxRbt: Created NEW RBT at 0x%lx with lock semaphore ID=%d",
		(unsigned long)rbt, rbtPtr->lock);
	writeMemo(msgBuf);
#endif

	return rbt;
}

static void addVImportSession(LtpVspan *vspan, unsigned int sessionNbr,
		SdrObject sessionElt, LtpVImportSession **vsessionPtr)
{
	PsmPartition		ltpwm = getIonwm();
	PsmAddress		addr;
	LtpVImportSession	*vsession;

	*vsessionPtr = NULL;		/*	Default.		*/
	addr = psm_zalloc(ltpwm, sizeof(LtpVImportSession));
	if (addr == 0)
	{
		return;
	}

	vsession = (LtpVImportSession *) psp(ltpwm, addr);
	vsession->sessionNbr = sessionNbr;
	vsession->sessionElt = sessionElt;
	vsession->redSegmentsIdx = getIdxRbt(ltpwm, vspan);
	if (vsession->redSegmentsIdx == 0)
	{
		psm_free(ltpwm, addr);
		return;
	}

	if (sm_rbt_insert(ltpwm, vspan->importSessions, addr,
			orderImportSessions, vsession) == 0)
	{
		sm_rbt_destroy(ltpwm, vsession->redSegmentsIdx, NULL, NULL);
		psm_free(ltpwm, addr);
		return;
	}

	*vsessionPtr = vsession;
}

static int	orderRedSegments(PsmPartition wm, PsmAddress nodeData,
			void *dataBuffer)
{
	LtpSegmentRef	*argRef;
	LtpSegmentRef	*nodeRef;

	argRef = (LtpSegmentRef *) dataBuffer;
	nodeRef = (LtpSegmentRef *) psp(wm, nodeData);
	if (nodeRef->offset < argRef->offset)
	{
		return -1;
	}

	if (nodeRef->offset > argRef->offset)
	{
		return 1;
	}

	return 0;
}

void getImportSession(LtpVspan *vspan, unsigned int sessionNbr,
		LtpVImportSession **vsessionPtr, SdrObject *sessionObj)
{
	Sdr			sdr = getIonsdr();
	PsmPartition		ltpwm = getIonwm();
	LtpVImportSession	arg;
	PsmAddress		rbtNode;
	PsmAddress		nextRbtNode;
	LtpVImportSession	*vsession;
	OBJ_POINTER(LtpSpan, span);
	SdrObject		elt;
	LtpImportSession	session;
	SdrObject		elt2;
	SdrObject		segObj;
	OBJ_POINTER(LtpRecvSeg, segment);
	LtpSegmentRef		refbuf;
	SdrObject		addr;

	*sessionObj = 0;		/*	Default.		*/
	if (vsessionPtr)
	{
		*vsessionPtr = NULL;	/*	Default.		*/
	}

	CHKVOID(ionLocked());
	arg.sessionNbr = sessionNbr;
	rbtNode = sm_rbt_search(ltpwm, vspan->importSessions,
			orderImportSessions, &arg, &nextRbtNode);
	if (rbtNode)
	{
		vsession = (LtpVImportSession *) psp(ltpwm,
				sm_rbt_data(ltpwm, rbtNode));
		*sessionObj = sdr_list_data(sdr, vsession->sessionElt);
	}
	else	/*	Must resurrect VImportSession.			*/
	{
		GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr,
				vspan->spanElt));
		if (sdr_hash_retrieve(sdr, span->importSessionsHash,
				    (char *) &sessionNbr, (SdrAddress *) &elt,
				    NULL) != 1)
		{
			return;		/*	No such session.	*/
		}

		*sessionObj = sdr_list_data(sdr, elt);

		/*	Need to add this VImportSession and load it
		 *	with all previously acquired red segments.	*/

		addVImportSession(vspan, sessionNbr, elt, &vsession);
		if (vsession == NULL)
		{
			return;
		}

		sdr_read(sdr, (char *) &session, *sessionObj,
				sizeof(LtpImportSession));
		if (session.redSegments)
		{
			for (elt2 = sdr_list_first(sdr, session.redSegments);
					elt2; elt2 = sdr_list_next(sdr, elt2))
			{
				segObj = sdr_list_data(sdr, elt2);
				GET_OBJ_POINTER(sdr, LtpRecvSeg, segment,
						segObj);
				refbuf.offset = segment->pdu.offset;
				refbuf.length = segment->pdu.length;
				refbuf.sessionListElt = segment->sessionListElt;
				addr = psm_zalloc(ltpwm, sizeof(LtpSegmentRef));
				if (addr == 0)
				{
					putErrmsg("Failed resurrecting \
LtpVImportSession.", NULL);
					*sessionObj = 0;
					return;
				}

				memcpy((char *) psp(ltpwm, addr), (char *)
						&refbuf, sizeof(LtpSegmentRef));
				if (sm_rbt_insert(ltpwm,
						vsession->redSegmentsIdx,
						addr, orderRedSegments, &refbuf)
						== 0)
				{
					putErrmsg("Failed resurrecting \
LtpVImportSession.", NULL);
					*sessionObj = 0;
					return;
				}
			}
		}
	}

	if (vsessionPtr)
	{
		*vsessionPtr = vsession;
	}
}

static int	sessionIsClosed(LtpVspan *vspan, unsigned int sessionNbr)
{
	Sdr		sdr = getIonsdr();
	OBJ_POINTER(LtpSpan, span);
	SdrObject	elt;
	unsigned int	closedSessionNbr;

	GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr,
			vspan->spanElt));

	/*	Closed-sessions list is in ascending session number
	 *	order.  Incoming segments are most likely to apply
	 *	to more recent sessions, so we search from end of
	 *	list rather from start.					*/

	for (elt = sdr_list_last(sdr, span->closedImports); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		closedSessionNbr = (unsigned int) sdr_list_data(sdr, elt);
		if (closedSessionNbr > sessionNbr)
		{
			continue;
		}

		if (closedSessionNbr == sessionNbr)
		{
			return 1;
		}

		break;		/*	No need to search further.	*/
	}

	/*	Not a recently closed import session.			*/

	return 0;
}

static void getCanceledImport(LtpVspan *vspan, unsigned int sessionNbr,
		SdrObject *sessionObj, SdrObject *sessionElt)
{
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(LtpSpan, span);
	OBJ_POINTER(LtpImportSession, session);
	SdrObject elt;
	SdrObject obj;

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr,
			vspan->spanElt));
	for (elt = sdr_list_first(sdr, span->deadImports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpImportSession, session, obj);
		if (session->sessionNbr == sessionNbr)
		{
			*sessionObj = obj;
			*sessionElt = elt;
			return;
		}
	}

	/*	Not a known canceled import session.			*/

	*sessionObj = 0;
	*sessionElt = 0;
}

static void destroyRsXmitSeg(SdrObject rsElt, SdrObject rsObj, LtpXmitSeg *rs)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;

	CHKVOID(ionLocked());
	cancelEvent(LtpResendReport, rs->remoteEngineId, rs->sessionNbr,
			rs->pdu.rptSerialNbr);

	/*	No need to change state of rs->pdu.timer because the
		whole segment is about to vanish.			*/

	if (rs->pdu.receptionClaims)
	{
		while ((elt = sdr_list_first(sdr, rs->pdu.receptionClaims)))
		{
			sdr_free(sdr, sdr_list_data(sdr, elt));
			sdr_list_delete(sdr, elt, NULL, NULL);
		}
	}

	if (rs->pdu.timer.expirationCount != -1)	/*	(burst)	*/
	{
		sdr_list_destroy(sdr, rs->pdu.receptionClaims, NULL, NULL);
	}

	if (rs->queueListElt)	/*	Queued for transmission.	*/
	{
		sdr_free(sdr, sdr_list_data(sdr, rs->queueListElt));
		sdr_list_delete(sdr, rs->queueListElt, NULL, NULL);
	}

	if (rs->pdu.headerExtensions)
	{
		sdr_list_destroy(sdr, rs->pdu.headerExtensions,
				ltpei_destroy_extension, NULL);
	}

	if (rs->pdu.trailerExtensions)
	{
		sdr_list_destroy(sdr, rs->pdu.trailerExtensions,
				ltpei_destroy_extension, NULL);
	}

	sdr_free(sdr, rsObj);
	sdr_list_delete(sdr, rsElt, NULL, NULL);
}

static void	stopVImportSession(LtpImportSession *session)
{
	Sdr			sdr = getIonsdr();
	PsmPartition		ltpwm = getIonwm();
	LtpSpan			span;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	LtpVImportSession	arg;

	sdr_read(sdr, (char *) &span, session->span, sizeof(LtpSpan));
	findSpan(span.engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		return;		/*	No such span.			*/
	}

	arg.sessionNbr = session->sessionNbr;
	oK(sm_rbt_delete(ltpwm, vspan->importSessions, orderImportSessions,
			&arg, deleteVImportSession, vspan));
}

static int	recycleImportBuffer(Sdr sdr, LtpSpan *span,
			LtpImportSession *session)
{
	SdrObject elt;

	/*	Recycle the import session's heap buffer.		*/

	if (session->heapBufferObj)
	{
		elt = sdr_list_insert_last(sdr, span->importBuffers,
				session->heapBufferObj);
		CHKERR(elt);
		session->heapBufferObj = 0;
	}

	return 0;
}

void	clearImportSession(LtpImportSession *session)
{
	/*	Note: this function is invoked only when an import
	 *	sesssion is canceled, by either sender or receiver.	*/

	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject segObj;
	OBJ_POINTER(LtpRecvSeg, ds);
	LtpSpan	span;

	/*	Terminate reception of red-part data, release space.	*/

	if (session->redSegments)
	{
		while ((elt = sdr_list_first(sdr, session->redSegments)))
		{
			segObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, LtpRecvSeg, ds, segObj);
			if (ds->pdu.headerExtensions)
			{
				sdr_list_destroy(sdr,
						ds->pdu.headerExtensions,
						ltpei_destroy_extension, NULL);
			}

			if (ds->pdu.trailerExtensions)
			{
				sdr_list_destroy(sdr,
						ds->pdu.trailerExtensions,
						ltpei_destroy_extension, NULL);
			}

			sdr_free(sdr, segObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
		}

		sdr_list_destroy(sdr, session->redSegments, NULL, NULL);
		session->redSegments = 0;
	}

	sdr_read(sdr, (char *) &span, session->span, sizeof(LtpSpan));
	oK(recycleImportBuffer(sdr, &span, session));

	/*	If service data not delivered, then destroying the
	 *	file ref immediately causes its cleanup script to
	 *	be executed, unlinking the file.  Otherwise, the
	 *	service data object passed to the client is a ZCO,
	 *	one of whose extents references this file ref; the
	 *	file ref is retained until the last reference to that
	 *	ZCO is destroyed, at which time the file ref is
	 *	destroyed and the file is consequently unlinked.	*/

	if (session->blockFileRef)
	{
		zco_destroy_file_ref(sdr, session->blockFileRef);
		session->blockFileRef = 0;
	}
}

void	stopImportSession(LtpImportSession *session)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject segObj;
	OBJ_POINTER(LtpXmitSeg, rs);

	CHKVOID(ionLocked());
	if (session->rsSegments)
	{
		while ((elt = sdr_list_first(sdr, session->rsSegments)) != 0)
		{
			segObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, LtpXmitSeg, rs, segObj);
			destroyRsXmitSeg(elt, segObj, rs);
		}

		sdr_list_destroy(sdr, session->rsSegments, NULL, NULL);
		session->rsSegments = 0;
	}

	stopVImportSession(session);
#if LTPDEBUG
putErrmsg("Stopped import session.", itoa(session->sessionNbr));
#endif
}

void removeImportSession(SdrObject sessionObj)
{
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(LtpImportSession, session);
	OBJ_POINTER(LtpSpan, span);
	SdrObject elt;

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(sdr, LtpImportSession, session, sessionObj);
	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);
	if (sdr_hash_remove(sdr, span->importSessionsHash,
			    (char *) &(session->sessionNbr),
			    (SdrAddress *) &elt) > 0)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}
}

static void	noteClosedImport(Sdr sdr, LtpSpan *span,
			LtpImportSession *session)
{
	SdrObject	elt;
	unsigned int	closedSessionNbr;
	SdrObject	elt2;
	LtpEvent	event;
	time_t		currentTime;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;

	/*	The closed-sessions list is in ascending session
	 *	number order, so we insert at the end of the list.	*/

	for (elt = sdr_list_last(sdr, span->closedImports); elt;
			elt = sdr_list_prev(sdr, elt))
	{
		closedSessionNbr = (unsigned int) sdr_list_data(sdr, elt);
		if (closedSessionNbr > session->sessionNbr)
		{
			continue;
		}

		break;
	}

	if (elt)
	{
		elt2 = sdr_list_insert_after(sdr, elt, session->sessionNbr);
	}
	else
	{
		elt2 = sdr_list_insert_first(sdr, span->closedImports,
				session->sessionNbr);
	}

	/*	Schedule removal of this closed-session note from the
	 *	list after (2 * max timeouts) times round-
	 *	trip time (plus 10 seconds of margin to allow for
	 *	processing delay).
	 *
	 *	In the event of the sender unnecessarily retransmitting
	 *	a checkpoint segment before receiving a final RS and
	 *	closing the export session, that late checkpoint will
	 *	arrive (and be discarded) before this scheduled event.
	 *
	 *	An additional checkpoint should never arrive after
	 *	the removal event -- and thereby resurrect the import
	 *	session -- unless the sender has a higher value for
	 *	max timeouts (or RTT) than the local node.  In
	 *	that case the export session's timeout sequence will
	 *	eventually result in re-closure of the reanimated
	 *	import session; there will be erroneous duplicate
	 *	data delivery, but no heap space leak.			*/

	memset((char *) &event, 0, sizeof(LtpEvent));
	event.parm = elt2;
	currentTime = getCtime();
	findSpan(span->engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		return;
	}

	event.scheduledTime = currentTime + 10 +
			(2 * (vspan->maxTimeouts / SIGNAL_REDUNDANCY)
			* (vspan->owltOutbound + vspan->owltInbound));
	event.type = LtpForgetImportSession;
	oK(insertLtpTimelineEvent(&event));
}

void closeImportSession(SdrObject sessionObj)
{
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(LtpImportSession, session);
	OBJ_POINTER(LtpSpan, span);

	CHKVOID(ionLocked());
	GET_OBJ_POINTER(sdr, LtpImportSession, session, sessionObj);
	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);

	/*	Cancel any pending inactivity event for this session.	*/

	if (session->inactivityEvent != 0)
	{
		cancelEvent(LtpStaleImportSession, 0, session->sessionNbr, 0);
	}

	/*	Remove the rest of the import session.			*/

	noteClosedImport(sdr, span, session);
	sdr_free(sdr, sessionObj);
#if LTPDEBUG
putErrmsg("Closed import session.", itoa(session->sessionNbr));
#endif
}

static void findReport(LtpImportSession *session, unsigned int rptSerialNbr,
		SdrObject *rsElt, SdrObject *rsObj)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject obj;
	OBJ_POINTER(LtpXmitSeg, rs);

	*rsElt = 0;			/*	Default.		*/
	*rsObj = 0;			/*	Default.		*/
	if (session->rsSegments == 0)	/*	Import session stopped.	*/
	{
		return;
	}

	for (elt = sdr_list_first(sdr, session->rsSegments); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitSeg, rs, obj);
		if (rs->pdu.rptSerialNbr == rptSerialNbr)
		{
			*rsElt = elt;
			*rsObj = obj;
			return;
		}
	}
}

static void	findCheckpoint(LtpExportSession *session,
			unsigned int ckptSerialNbr,
			SdrObject *dsElt, SdrObject *dsObj)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject obj;
	OBJ_POINTER(LtpCkpt, cp);

	*dsElt = 0;			/*	Default.		*/
	*dsObj = 0;			/*	Default.		*/
	if (session->checkpoints == 0)	/*	Export session cleared.	*/
	{
		return;
	}

	for (elt = sdr_list_first(sdr, session->checkpoints); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpCkpt, cp, obj);
		if (cp->serialNbr < ckptSerialNbr)
		{
			continue;
		}

		if (cp->serialNbr == ckptSerialNbr)
		{
			*dsElt = cp->sessionListElt;
			*dsObj = sdr_list_data(sdr, cp->sessionListElt);
		}

		return;
	}
}

/*	*	*	Segment issuance functions	*	*	*/

static void	serializeLtpExtensionField(LtpExtensionOutbound *extensionField,
			char **cursor)
{
	Sdr	sdr = getIonsdr();
	Sdnv	sdnv;

	**cursor = extensionField->tag;
	(*cursor)++;

	encodeSdnv(&sdnv, extensionField->length);
	memcpy((*cursor), sdnv.text, sdnv.length);
	(*cursor) += sdnv.length;

	sdr_read(sdr, (*cursor), extensionField->value, extensionField->length);
	(*cursor) += extensionField->length;
}

static int	serializeHeader(LtpXmitSeg *segment, char *segmentBuffer,
			Sdnv *engineIdSdnv)
{
	char		firstByte = LTP_VERSION;
	char		*cursor = segmentBuffer;
	Sdnv		sessionNbrSdnv;
	char		extensionCounts;
	Sdr		sdr;
	SdrObject	elt;
	SdrObject	extAddr;
	OBJ_POINTER(LtpExtensionOutbound, headerExt);
	ExtensionDef	*def;

	firstByte <<= 4;
	firstByte += segment->pdu.segTypeCode;
	*cursor = firstByte;
	cursor++;

	memcpy(cursor, engineIdSdnv->text, engineIdSdnv->length);
	cursor += engineIdSdnv->length;

	encodeSdnv(&sessionNbrSdnv, segment->sessionNbr);
	memcpy(cursor, sessionNbrSdnv.text, sessionNbrSdnv.length);
	cursor += sessionNbrSdnv.length;

	extensionCounts = segment->pdu.headerExtensionsCount;
	extensionCounts <<= 4;
	extensionCounts += segment->pdu.trailerExtensionsCount;
	*cursor = extensionCounts;
	cursor++;

	if (segment->pdu.headerExtensions == 0)
	{
		return 0;
	}

	/*	Serialize all segment header extensions.		*/

	sdr = getIonsdr();
	for (elt = sdr_list_first(sdr, segment->pdu.headerExtensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		extAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpExtensionOutbound, headerExt,
				extAddr);
		def = findLtpExtensionDef(headerExt->tag);
		if (def && def->outboundOnHeaderExtensionSerialization)
		{
			if (def->outboundOnHeaderExtensionSerialization
					(extAddr, segment, &cursor) < 0)
			{
				return -1;
			}
			else
			{
				serializeLtpExtensionField(headerExt, &cursor);
			}
		}
	}

	return 0;
}

static void	serializeDataSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	sdnv;

	/*	Origin is the local engine.				*/

	serializeHeader(segment, cursor, &(_ltpConstants()->ownEngineIdSdnv));
	cursor += segment->pdu.headerLength;

	/*	Append client service number.				*/

	encodeSdnv(&sdnv, segment->pdu.clientSvcId);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append offset of data within block.			*/

	encodeSdnv(&sdnv, segment->pdu.offset);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append length of data.					*/

	encodeSdnv(&sdnv, segment->pdu.length);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	If checkpoint, append checkpoint and report serial
	 *	numbers.						*/

	if (!(segment->pdu.segTypeCode & LTP_EXC_FLAG)	/*	Red.	*/
	&& segment->pdu.segTypeCode > 0)	/*	Checkpoint.	*/
	{
		/*	Append checkpoint serial number.		*/

		encodeSdnv(&sdnv, segment->pdu.ckptSerialNbr);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;

		/*	Append report serial number.			*/

		encodeSdnv(&sdnv, segment->pdu.rptSerialNbr);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;
	}

	/*	Note: client service data was copied into the trailing
	 *	bytes of the buffer before this function was called.	*/
}

static void	serializeReportSegment(LtpXmitSeg *segment, char *buf)
{
	Sdr		sdr = getIonsdr();
	char		*cursor = buf;
	Sdnv		sdnv;
	int		count;
	SdrObject	elt;
	OBJ_POINTER(LtpReceptionClaim, claim);
	unsigned int	offset;

	/*	Report is from local engine, so origin is the remote
	 *	engine.							*/

	encodeSdnv(&sdnv, segment->remoteEngineId);
	serializeHeader(segment, cursor, &sdnv);
	cursor += segment->pdu.headerLength;

	/*	Append report serial number.				*/

	encodeSdnv(&sdnv, segment->pdu.rptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append checkpoint serial number.			*/

	encodeSdnv(&sdnv, segment->pdu.ckptSerialNbr);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append report upper bound.				*/

	encodeSdnv(&sdnv, segment->pdu.upperBound);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append report lower bound.				*/

	encodeSdnv(&sdnv, segment->pdu.lowerBound);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append count of reception claims.			*/

	count = sdr_list_length(sdr, segment->pdu.receptionClaims);
	CHKVOID(count >= 0);
	encodeSdnv(&sdnv, count);
	memcpy(cursor, sdnv.text, sdnv.length);
	cursor += sdnv.length;

	/*	Append all reception claims.				*/

	for (elt = sdr_list_first(sdr, segment->pdu.receptionClaims); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, LtpReceptionClaim, claim,
				sdr_list_data(sdr, elt));

		/*	For transmission ONLY (never in processing
		 *	within the LTP engine), claim->offset is
		 *	compressed to offset from report segment's
		 *	lower bound rather than from start of block.	*/

		offset = claim->offset - segment->pdu.lowerBound;
		encodeSdnv(&sdnv, offset);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;
		encodeSdnv(&sdnv, claim->length);
		memcpy(cursor, sdnv.text, sdnv.length);
		cursor += sdnv.length;
	}
}

static void	serializeReportAckSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	serialNbrSdnv;

	/*	Report is from remote engine, so origin is the local
	 *	engine.							*/

	serializeHeader(segment, cursor, &(_ltpConstants()->ownEngineIdSdnv));
	cursor += segment->pdu.headerLength;

	/*	Append report serial number.				*/

	encodeSdnv(&serialNbrSdnv, segment->pdu.rptSerialNbr);
	memcpy(cursor, serialNbrSdnv.text, serialNbrSdnv.length);
}

static void	serializeCancelSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	engineIdSdnv;

	if (segment->pdu.segTypeCode == LtpCS)
	{
		/*	Cancellation by sender, so origin is the
		 *	local engine.					*/

		serializeHeader(segment, cursor,
				&(_ltpConstants()->ownEngineIdSdnv));
	}
	else
	{
		encodeSdnv(&engineIdSdnv, segment->remoteEngineId);
		serializeHeader(segment, cursor, &engineIdSdnv);
	}

	cursor += segment->pdu.headerLength;

	/*	Append reason code.					*/

	*cursor = segment->pdu.reasonCode;
}

static void	serializeCancelAckSegment(LtpXmitSeg *segment, char *buf)
{
	char	*cursor = buf;
	Sdnv	engineIdSdnv;

	if (segment->pdu.segTypeCode == LtpCAR)
	{
		/*	Acknowledging cancel by receiver, so origin
		 *	is the local engine.				*/

		serializeHeader(segment, cursor,
				&(_ltpConstants()->ownEngineIdSdnv));
	}
	else
	{
		encodeSdnv(&engineIdSdnv, segment->remoteEngineId);
		serializeHeader(segment, cursor, &engineIdSdnv);
	}

	/*	No content for cancel acknowledgment, just header.	*/
}

static int setTimer(LtpTimer *timer, SdrAddress timerAddr, time_t currentSec,
		LtpVspan *vspan, int segmentLength, LtpEvent *event)
{
	Sdr	sdr = getIonsdr();
	LtpDB	ltpdb;
	time_t	segArrivalTimeOffset = 0;
	time_t	ackDeadlineOffset = 0;
	int	radTime;
	OBJ_POINTER(LtpSpan, span);

	if (timer->expirationCount == -1)	/*	(burst)		*/
	{
		return 0;	/*	Never set timer for bursts	*/
	}

	CHKERR(ionLocked());
	sdr_read(sdr, (char *) &ltpdb, getLtpDbObject(), sizeof(LtpDB));
	if (vspan->localXmitRate == 0)	/*	Should never be, but...	*/
	{
		radTime = 0;		/*	Avoid divide by zero.	*/
	}
	else
	{
		radTime = (segmentLength + EST_LINK_OHD) / vspan->localXmitRate;
	}

	/*	Segment should arrive at the remote node following
	 *	about half of the local node's telecom processing
	 *	turnaround time (ownQtime) plus the time consumed in
	 *	simply radiating all the bytes of the segment
	 *	(including estimated link-layer overhead) at the
	 *	current transmission rate over this span, plus
	 *	the current outbound signal propagation time (owlt).	*/

	segArrivalTimeOffset = radTime + vspan->owltOutbound
			+ ((ltpdb.ownQtime >> 1) & 0x7fffffff);
	GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr, vspan->spanElt));

	/*	Following arrival of the segment, the response from
	 *	the remote node should arrive here following the
	 *	remote node's entire telecom processing turnaround
	 *	time (remoteQtime) plus the current inbound signal
	 *	propagation time (owlt) plus the other half of the
	 *	local node's telecom processing turnaround time.
	 *
	 *	Technically, we should also include in this interval
	 *	the time consumed in simply transmitting all bytes
	 *	of the response at the current fire rate over this
	 *	span.  But in practice this interval is too small
	 *	to be worth the trouble of managing it (i.e, it is
	 *	not known unless the remote node is currently
	 *	transmitting, it needs to be backed out and later
	 *	restored on suspension/resumption of the link because
	 *	the remote fire rate might change, etc.).		*/

	ackDeadlineOffset = segArrivalTimeOffset
			+ span->remoteQtime + vspan->owltInbound
			+ ((ltpdb.ownQtime >> 1) & 0x7fffffff);
	timer->segArrivalTime = currentSec
			+ CEIL(segArrivalTimeOffset / SIGNAL_REDUNDANCY);
	timer->ackDeadline = currentSec
			+ CEIL(ackDeadlineOffset / SIGNAL_REDUNDANCY);
#if CLOSED_EXPORTS_ENABLED
	if (event->type == LtpForgetExportSession)
	{
		timer->ackDeadline = currentSec + (ackDeadlineOffset
				* vspan->maxTimeouts / SIGNAL_REDUNDANCY);
	}
#endif
	if (vspan->remoteXmitRate > 0)
	{
		event->scheduledTime = timer->ackDeadline;
		if (insertLtpTimelineEvent(event) == 0)
		{
			putErrmsg("Can't set timer.", NULL);
			return -1;
		}

		timer->state = LtpTimerRunning;
	}
	else
	{
		timer->state = LtpTimerSuspended;
	}

	sdr_write(sdr, timerAddr, (char *) timer, sizeof(LtpTimer));
	return 0;
}

static int readFromExportBlock(char *buffer, SdrObject svcDataObjects,
		unsigned int offset, unsigned int length)
{
	Sdr		sdr = getIonsdr();
	SdrObject	elt;
	SdrObject	sdu; /* Each member of list is a ZCO. */
	unsigned int	sduLength;
	int		totalBytesRead = 0;
	ZcoReader	reader;
	unsigned int	bytesToRead;
	int		bytesRead;

	for (elt = sdr_list_first(sdr, svcDataObjects); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sdu = sdr_list_data(sdr, elt);
		sduLength = zco_length(sdr, sdu);
		if (offset >= sduLength)
		{
			offset -= sduLength;	/*	Skip over SDU.	*/
			continue;
		}

		zco_start_transmitting(sdu, &reader);
		zco_track_file_offset(&reader);
		if (offset > 0)
		{
			if (zco_transmit(sdr, &reader, offset, NULL) < 0)
			{
				putErrmsg("Failed skipping offset.", NULL);
				return -1;
			}

			sduLength -= offset;
			offset = 0;
		}

		bytesToRead = length;
		if (bytesToRead > sduLength)
		{
			bytesToRead = sduLength;
		}

		bytesRead = zco_transmit(sdr, &reader, bytesToRead,
				buffer + totalBytesRead);
		if (bytesRead < 0 || (unsigned int)bytesRead != bytesToRead)
		{
			putErrmsg("Failed reading SDU.", NULL);
			return -1;
		}

		totalBytesRead += bytesRead;
		length -= bytesRead;
		if (length == 0)	/*	Have read enough.	*/
		{
			break;
		}
	}

	return totalBytesRead;
}

static int	serializeTrailer(LtpXmitSeg *segment, char *segmentBuffer)
{
	char		*cursor = segmentBuffer + (segment->pdu.headerLength
					+ segment->pdu.contentLength);
	Sdr		sdr;
	SdrObject	elt;
	SdrObject	extAddr;
	OBJ_POINTER(LtpExtensionOutbound, trailerExt);
	ExtensionDef	*def;

	if (segment->pdu.trailerExtensions == 0)
	{
		return 0;
	}

	/*	Serialize all segment trailer extensions.		*/

	sdr = getIonsdr();
	for (elt = sdr_list_first(sdr, segment->pdu.trailerExtensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		extAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpExtensionOutbound, trailerExt,
				extAddr);
		def = findLtpExtensionDef(trailerExt->tag);
		if (def && def->outboundOnTrailerExtensionSerialization)
		{
			if (def->outboundOnTrailerExtensionSerialization
					(extAddr, segment, &cursor) < 0)
			{
				return -1;
			}
		}
		else
		{
			serializeLtpExtensionField(trailerExt, &cursor);
		}
	}

	return 0;
}

int	ltpDequeueOutboundSegment(LtpVspan *vspan, char **buf)
{
	Sdr				sdr = getIonsdr();
	LtpVdb				*ltpvdb = _ltpvdb(NULL);
	LtpDB				*ltpConstants = _ltpConstants();
	SdrObject			spanObj;
	LtpSpan				spanBuf;
	SdrObject			elt;
	char				nbrBuf[FQN_MAX_LENGTH];
	char				memo[64];
	SdrObject			segRefAddr;
	LtpXmitSegRef			segRef;
	SdrObject			sessionObj;
	LtpXmitSeg			segment;
	int				segmentLength;
	SdrObject			sessionElt;
	OBJ_POINTER(LtpReceptionClaim, claim);
	LtpExportSession		xsessionBuf;
	time_t				currentTime;
	LtpEvent			event;
	LtpTimer			*timer;
	LtpImportSession		rsessionBuf;

#if defined (EWCHAR)
	char 	ewchar[256];
	/* initialize as 'g' */
	ewchar[0] = 'g';
	ewchar[1] = '\0';
#endif

	CHKERR(vspan);
	CHKERR(buf);
	*buf = (char *) psp(getIonwm(), vspan->segmentBuffer);
	CHKERR(sdr_begin_xn(sdr));
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));
	elt = sdr_list_first(sdr, spanBuf.segments);
	while (elt == 0 || vspan->localXmitRate == 0)
	{
		sdr_exit_xn(sdr);

		/*	Wait until ltpmeter has announced an outbound
		 *	segment by giving span's segSemaphore.		*/

		if (sm_SemTake(vspan->segSemaphore) < 0)
		{
			putErrmsg("LSO can't take segment semaphore.",
				itoa(vspan->engineId));
			return -1;
		}

		if (sm_SemEnded(vspan->segSemaphore))
		{
			putFqn(nbrBuf, vspan->engineId);
			isprintf(memo, sizeof memo,
				"[i] LSO to engine %s is stopped.", nbrBuf);
			writeMemo(memo);
			return 0;
		}

		CHKERR(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));
		elt = sdr_list_first(sdr, spanBuf.segments);
	}

	/*	Got next outbound segment reference.  Remove it from
	 *	the queue for this span and delete it.			*/

	segRefAddr = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_read(sdr, (char *) &segRef, segRefAddr, sizeof(LtpXmitSegRef));
	sdr_free(sdr, segRefAddr);

	/*	If the referenced segment is a data segment, check
	 *	to see if the export session it's being sent from
	 *	is still open.						*/

	if (segRef.sessionNbr > 0)	/*	Red or green data seg.	*/
	{
		getExportSession(segRef.sessionNbr, &sessionObj);
		if (sessionObj == 0)
		{
			/*	The referenced segment is a data
			 *	segment for an export session that
			 *	has been closed.  Nothing more to
			 *	do, as the referenced segment has
			 *	already been destroyed; just return
			 *	segment length zero.			*/

			return 0;
		}

		/*	Now we know the session is still open, so
		 *	we know that the referenced data segment
		 *	can still be retrieved and transmitted.		*/
	}

	sdr_stage(sdr, (char *) &segment, segRef.segAddr, sizeof(LtpXmitSeg));
	segment.queueListElt = 0;

	/*	If segment is a data segment other than a checkpoint,
	 *	remove it from the relevant list in its session.
	 *	(Note that segments are retained in these lists only
	 *	to support ExportSession cancellation prior to
	 *	transmission of the segments.)				*/

	if (segment.pdu.segTypeCode == LtpDsRed	/*	Non-ckpt red.	*/
	|| segment.pdu.segTypeCode == LtpDsGreen
	|| segment.pdu.segTypeCode == LtpDsGreenEOB)
	{
		sdr_list_delete(sdr, segment.sessionListElt, NULL, NULL);
		segment.sessionListElt = 0;

#if defined (EWCHAR)
			/* segment is data only, non-check point */
			isprintf(ewchar,sizeof(ewchar),"(ds%u)g",segment.sessionNbr);
#endif
	}

	/*	Copy segment's content into buffer.			*/

	segmentLength = segment.pdu.headerLength + segment.pdu.contentLength
			+ segment.pdu.trailerLength;
	if (segment.segmentClass == LtpDataSeg)
	{
		/*	Load client service data at the end of the
		 *	segment first, before filling in the header.	*/

		if (readFromExportBlock((*buf) + segment.pdu.headerLength
				+ segment.pdu.ohdLength, segment.pdu.block,
				segment.pdu.offset, segment.pdu.length) < 0)
		{
			/*	The backing store for this segment's
			 *	payload (typically a file behind a
			 *	bpsendfile-style ZCO) is no longer
			 *	readable.  This is a bundle-level
			 *	failure -- drop the segment.  We cannot
			 *	call sdr_cancel_xn() here: the destructive
			 *	list/free operations earlier in this
			 *	function are not reversible on non-
			 *	reversible SDR profiles, so cancellation
			 *	would trip handleUnrecoverableError()
			 *	-> sm_Abort() and kill the entire node
			 *	(see #965).  Free the segment object
			 *	consistently with the non-retain default
			 *	path below, also clearing any sessionListElt
			 *	that wasn't already removed earlier in this
			 *	function (checkpoint types skip the
			 *	unconditional removal above), drop the
			 *	transaction via sdr_drop_xn so the dead
			 *	segref stays off the queue and the drop
			 *	is logged + counted, and return 0
			 *	(interrupted) so LSO continues with the
			 *	next segment rather than terminating the
			 *	link.					*/

			if (segment.sessionListElt)
			{
				sdr_list_delete(sdr,
					segment.sessionListElt, NULL, NULL);
				segment.sessionListElt = 0;
			}

			if (segment.pdu.headerExtensions)
			{
				sdr_list_destroy(sdr,
					segment.pdu.headerExtensions,
					ltpei_destroy_extension, NULL);
			}

			if (segment.pdu.trailerExtensions)
			{
				sdr_list_destroy(sdr,
					segment.pdu.trailerExtensions,
					ltpei_destroy_extension, NULL);
			}

			sdr_free(sdr, segRef.segAddr);

			sdr_drop_xn(sdr, "ltpDequeueOutboundSegment: "
				"unreadable export block (segment dropped)");
			return 0;
		}
	}

	/*	Remove segment from database if possible, i.e.,
	 *	if it needn't ever be retransmitted.  Otherwise
	 *	rewrite it to record change of queueListElt to 0.	*/

	switch (segment.pdu.segTypeCode)
	{
	case LtpDsRedCheckpoint:	/*	Checkpoint.		*/
	case LtpDsRedEORP:		/*	Checkpoint.		*/
	case LtpDsRedEOB:		/*	Checkpoint.		*/
	case LtpRS:			/*	Report.			*/
		sdr_write(sdr, segRef.segAddr, (char *) &segment,
				sizeof(LtpXmitSeg));
		break;

	default:	/*	No need to retain this segment.		*/
		if (segment.pdu.headerExtensions)
		{
			sdr_list_destroy(sdr, segment.pdu.headerExtensions,
					ltpei_destroy_extension, NULL);
		}

		if (segment.pdu.trailerExtensions)
		{
			sdr_list_destroy(sdr, segment.pdu.trailerExtensions,
					ltpei_destroy_extension, NULL);
		}

		sdr_free(sdr, segRef.segAddr);
	}

	/*	Post timeout event as necessary.			*/

	currentTime = getCtime();
	event.parm = 0;
	switch (segment.pdu.segTypeCode)
	{
	case LtpDsRedCheckpoint:	/*	Checkpoint.		*/
	case LtpDsRedEORP:		/*	Checkpoint.		*/
	case LtpDsRedEOB:		/*	Checkpoint.		*/
		event.type = LtpResendCheckpoint;
		event.refNbr1 = 0;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = segment.pdu.ckptSerialNbr;
		timer = &segment.pdu.timer;
		if (setTimer(timer,
			segRef.segAddr + FLD_OFFSET(timer, &segment),
			currentTime, vspan, segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (timer->expirationCount == 0)
		{
			ltpSpanTally(vspan, CKPT_XMIT, 0);

#if defined (EWCHAR)
			/* Initial check point */
			isprintf(ewchar,sizeof(ewchar),"(cp%u)g",segment.sessionNbr);
#endif
		}
		else
		{
			ltpSpanTally(vspan, CKPT_RE_XMIT, 0);

#if defined (EWCHAR)
			/* Retransmit check point */
			isprintf(ewchar,sizeof(ewchar),"(rcp%u)g",segment.sessionNbr);
#endif

		}

		break;

	case 8:  /* RS - resport segment */
		/* this is scheduling future retx event */
		event.type = LtpResendReport;
		event.refNbr1 = segment.remoteEngineId;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = segment.pdu.rptSerialNbr;
		timer = &segment.pdu.timer;
		if (setTimer(timer,
			segRef.segAddr + FLD_OFFSET(timer, &segment),
			currentTime, vspan, segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (timer->expirationCount == 0)
		{
			GET_OBJ_POINTER(sdr, LtpReceptionClaim, claim,
				sdr_list_data(sdr, sdr_list_first(sdr,
				segment.pdu.receptionClaims)));
			if (claim->offset == segment.pdu.lowerBound
			&& claim->length == segment.pdu.upperBound
					- segment.pdu.lowerBound)
			{
				ltpSpanTally(vspan, POS_RPT_XMIT, 0);
#if defined (EWCHAR)
				/* a "positive" report */
				isprintf(ewchar,sizeof(ewchar),"(prs%u)g",segment.sessionNbr);
#endif
			}
			else
			{
				ltpSpanTally(vspan, NEG_RPT_XMIT, 0);
#if defined (EWCHAR)
				/* a "negative" report */
				isprintf(ewchar,sizeof(ewchar),"(nrs%u)g",segment.sessionNbr);
#endif
			}
		}
		else
		{
			ltpSpanTally(vspan, RPT_RE_XMIT, 0);
#if defined (EWCHAR)
			/* this is a retransmitted report - either positive or negative */
			isprintf(ewchar,sizeof(ewchar),"(rrs%u)g",segment.sessionNbr);
#endif
		}

		break;

	case 12: /* CA - cancel by LTP block source */
		getCanceledExport(segment.sessionNbr, &sessionObj, &sessionElt);
		if (sessionObj == 0)
		{
			break;		/*	Session already closed.	*/
		}

		sdr_stage(sdr, (char *) &xsessionBuf, sessionObj,
				sizeof(LtpExportSession));
		event.type = LtpResendXmitCancel;
		event.refNbr1 = 0;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = 0;
		timer = &(xsessionBuf.timer);
		if (setTimer(timer, sessionObj + FLD_OFFSET(timer,
				&xsessionBuf), currentTime, vspan,
				segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		ltpSpanTally(vspan, EXPORT_CANCEL_XMIT, 0);
#if defined (EWCHAR)
			/* CS - cancel by block source */
			isprintf(ewchar,sizeof(ewchar),"(cs%u)g",segment.sessionNbr);
#endif
		break;

	case 14:
		if (segment.sessionObj == 0)
		{
			break;		/*	No need for timer.	*/
		}

		/*	An ImportSession has been started for this
		 *	session, so must assure that this cancellation
		 *	is delivered -- unless the session is already
		 *	closed.						*/

		getCanceledImport(vspan, segment.sessionNbr, &sessionObj,
				&sessionElt);
		if (sessionObj == 0)
		{
			break;		/*	Session already closed.	*/
		}

		sdr_stage(sdr, (char *) &rsessionBuf, sessionObj,
				sizeof(LtpImportSession));
		event.type = LtpResendRecvCancel;
		event.refNbr1 = segment.remoteEngineId;
		event.refNbr2 = segment.sessionNbr;
		event.refNbr3 = 0;
		timer = &(rsessionBuf.timer);
		if (setTimer(timer, sessionObj + FLD_OFFSET(timer,
				&rsessionBuf), currentTime, vspan,
				segmentLength, &event) < 0)
		{
			putErrmsg("Can't schedule event.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		ltpSpanTally(vspan, IMPORT_CANCEL_XMIT, 0);
#if defined (EWCHAR)
			/* CR - cancel by block receiver */
			isprintf(ewchar,sizeof(ewchar),"(cr%u)g",segment.sessionNbr);
#endif
		break;

	default:
		break;
	}

	/*	Handle end-of-block if necessary.			*/

	if (segment.pdu.segTypeCode < 8
	&& (segment.pdu.segTypeCode & LTP_FLAG_1)
	&& (segment.pdu.segTypeCode & LTP_FLAG_0))
	{
		/*	If initial transmission of EOB, notify the
		 *	client and release the span so that ltpmeter
		 *	can start segmenting the next block.		*/

		if (segment.pdu.timer.expirationCount == 0)
		{
			if (enqueueNotice(ltpvdb->clients
				+ segment.pdu.clientSvcId,
				ltpConstants->ownEngineId, segment.sessionNbr,
				0, 0, LtpXmitComplete, 0, 0, 0) < 0)
			{
				putErrmsg("Can't post XmitComplete notice.",
						NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}

			sdr_write(sdr, spanObj, (char *) &spanBuf,
					sizeof(LtpSpan));
		}

		/*	If entire block is green or all red-part data
		 *	have already been acknowledged, close the
		 *	session (since normal session closure on red-
		 *	part completion or cancellation won't happen).	*/

		if (segment.pdu.segTypeCode == LtpDsGreenEOB)
		{
			/*	We have already already retrieved the
			 *	sessionObj, from the closed-session
			 *	check performed above for all data
			 *	segment references.			*/

			sdr_stage(sdr, (char *) &xsessionBuf,
					sessionObj, sizeof(LtpExportSession));
			if (xsessionBuf.totalLength != 0)
			{
				/*	Found the session.	*/

				if (xsessionBuf.redPartLength == 0
				|| xsessionBuf.stateFlags
						& LTP_FINAL_ACK)
				{
					closeExportSession(sessionObj);
					ltpSpanTally(vspan,
						EXPORT_COMPLETE, 0);
				}
				else
				{
					xsessionBuf.stateFlags |=
						LTP_EOB_SENT;
					sdr_write(sdr, sessionObj,
						(char *) &xsessionBuf,
						sizeof(LtpExportSession));
				}
			}
		}
	}

	/*	Now serialize the segment overhead and prepend that
	 *	overhead to the content of the segment (if any), and
	 *	return to link service output process.			*/

	/*  Every segment type priority to 8 (DS), have
	 *  additional content other than the header */

	if (segment.pdu.segTypeCode < 8)
	{
		ltpSpanTally(vspan, OUT_SEG_POPPED, segment.pdu.length);
		serializeDataSegment(&segment, *buf);
	}
	else
	{
		switch (segment.pdu.segTypeCode)
		{
			case 8:		/*	Report.			*/
				serializeReportSegment(&segment, *buf);
				break;

			case 9:		/*	Report acknowledgment.	*/
				serializeReportAckSegment(&segment, *buf);
#if defined (EWCHAR)
			/* RAS - report ACK */
			isprintf(ewchar,sizeof(ewchar),"(ras%u)g",segment.sessionNbr);
#endif
				break;

			case 12:	/*	Cancel by sender.	*/
			case 14:	/*	Cancel by receiver.	*/
				serializeCancelSegment(&segment, *buf);
				break;

			case 13:	/*	Cancel acknowledgment.	*/
			case 15:	/*	Cancel acknowledgment.	*/
				serializeCancelAckSegment(&segment, *buf);
#if defined (EWCHAR)
			/* CAR or CAS - cancel ACK */
			isprintf(ewchar,sizeof(ewchar),"(ca%u)g",segment.sessionNbr);
#endif
				break;

			default:
				break;
		}
	}

	if (serializeTrailer(&segment, *buf) < 0)
	{
		putErrmsg("Can't serialize segment trailer.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't get outbound segment for span.", NULL);
		return -1;
	}

	if (ltpvdb->watching & WATCH_g)
	{
#if defined (EWCHAR)
		iwatch_str(ewchar);
#else
		iwatch('g');
#endif
	}

	return segmentLength;
}

/*	*	Control segment construction functions		*	*/

static void	signalLso(uvast engineId)
{
	LtpVspan	*vspan;
	PsmAddress	vspanElt;

	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt != 0 && vspan->localXmitRate > 0)
	{
		/*	Tell LSO that output is waiting.	*/

		sm_SemGive(vspan->segSemaphore);
	}
}

static SdrObject enqueueCancelReqSegment(LtpXmitSeg *segment,
			LtpSpan *span, Sdnv *sourceEngineSdnv,
			unsigned int sessionNbr,
			LtpCancelReasonCode reasonCode)
{
	Sdr		sdr = getIonsdr();
	Sdnv		sdnv;
	LtpXmitSegRef	segRef;
	SdrObject	segRefObj;

	CHKZERO(ionLocked());
	segRef.sessionNbr = 0;	/*	Indicates not a data segment.	*/
	segment->sessionNbr = sessionNbr;
	segment->remoteEngineId = span->engineId;
	encodeSdnv(&sdnv, sessionNbr);
	segment->pdu.headerLength =
			1 + sourceEngineSdnv->length + sdnv.length + 1;
	segment->pdu.contentLength = 1;
	segment->pdu.trailerLength = 0;
	segment->sessionListElt = 0;
	segment->segmentClass = LtpMgtSeg;
	segment->pdu.reasonCode = reasonCode;
	segRef.segAddr = sdr_malloc(sdr, sizeof(LtpXmitSeg));
	if (segRef.segAddr == 0)
	{
		return 0;
	}

	segRefObj = sdr_malloc(sdr, sizeof(LtpXmitSegRef));
	if (segRefObj == 0)
	{
		return 0;
	}

	sdr_write(sdr, segRefObj, (char *) &segRef, sizeof(LtpXmitSegRef));
	segment->queueListElt = sdr_list_insert_last(sdr, span->segments,
			segRefObj);
	if (segment->queueListElt == 0)
	{
		return 0;
	}

	if (invokeOutboundOnHeaderExtensionGenerationCallbacks(segment) < 0)
	{
		return 0;
	}

	if (invokeOutboundOnTrailerExtensionGenerationCallbacks(segment) < 0)
	{
		return 0;
	}

	sdr_write(sdr, segRef.segAddr, (char *) segment, sizeof(LtpXmitSeg));
#if BURST_SIGNALS_ENABLED
	if (enqueueBurst(segment, span, 0, CANCELSEGMENT_BURST) < 0)
	{
		return 0;
	}
#endif
	return segRef.segAddr;
}

static int	constructSourceCancelReqSegment(LtpSpan *span,
			Sdnv *sourceEngineSdnv, unsigned int sessionNbr,
			SdrObject sessionObj, LtpCancelReasonCode reasonCode)
{
	LtpXmitSeg	segment;
	SdrObject	segmentObj;

	/* Parameter intentionally unused. */
	(void)sessionObj;

	/*	Cancellation by the local engine.			*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCS;
	segmentObj = enqueueCancelReqSegment(&segment, span, sourceEngineSdnv,
			sessionNbr, reasonCode);
	if (segmentObj == 0)
	{
		return -1;
	}

	signalLso(span->engineId);
	return 0;
}

static int	cancelSessionBySender(LtpExportSession *session,
			SdrObject sessionObj, LtpCancelReasonCode reasonCode)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	SdrObject	dbobj = getLtpDbObject();
	LtpDB		db;
	SdrObject	spanObj = session->span;
	LtpSpan		span;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	SdrObject	elt;
	SdrObject	sdu;			/* A ZcoRef object. */

	CHKERR(ionLocked());
	session->reasonCode = reasonCode;	/*	(For CS resend.)*/
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	findSpan(span.engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		putErrmsg("Can't find vspan for engine.", utoa(span.engineId));
		return -1;
	}

	if (sessionObj == span.currentExportSessionObj)
	{
		/*	Finish up session so it can be reported.	*/

		session->clientSvcId = span.clientSvcIdOfBufferedBlock;
		encodeSdnv(&(session->clientSvcIdSdnv), session->clientSvcId);
		session->totalLength = span.lengthOfBufferedBlock;
		session->redPartLength = span.redLengthOfBufferedBlock;
	}

	if (ltpvdb->watching & WATCH_CLS)
	{
		iwatch('{');
	}

	sdr_stage(sdr, (char *) &db, dbobj, sizeof(LtpDB));
	stopExportSession(session);
	for (elt = sdr_list_first(sdr, session->svcDataObjects); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sdu = sdr_list_data(sdr, elt);
		if (enqueueNotice(ltpvdb->clients + session->clientSvcId,
			db.ownEngineId, session->sessionNbr, 0, 0,
			LtpExportSessionCanceled, reasonCode, 0, sdu) < 0)
		{
			putErrmsg("Can't post ExportSessionCanceled notice.",
					NULL);
			return -1;
		}
	}

	sdr_write(sdr, dbobj, (char *) &db, sizeof(LtpDB));
	sdr_list_destroy(sdr, session->svcDataObjects, NULL, NULL);
	session->svcDataObjects = 0;
	clearExportSession(session);
	sdr_write(sdr, sessionObj, (char *) session, sizeof(LtpExportSession));

	/*	Insert into list of canceled sessions...		*/

	if (sdr_list_insert_last(sdr, db.deadExports, sessionObj) == 0)
	{
		putErrmsg("Can't insert into list of canceled sessions.", NULL);
		return -1;
	}

	/*	...and remove session from active sessions pool, so
	 *	that the cancellation won't affect flow control.	*/

	if (sdr_hash_remove(sdr, db.exportSessionsHash,
			    (char *) &(session->sessionNbr),
			    (SdrAddress *) &elt) > 0)
	{
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	/*	Span now has room for another session to start.		*/

	if (sessionObj == span.currentExportSessionObj)
	{
		/*	Reinitialize span's block buffer.		*/

		span.ageOfBufferedBlock = 0;
		span.lengthOfBufferedBlock = 0;
		span.redLengthOfBufferedBlock = 0;
		span.clientSvcIdOfBufferedBlock = 0;
		span.currentExportSessionObj = 0;
		sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));

		/*	Re-start the current export session.		*/

		if (startExportSession(sdr, spanObj, vspan) < 0)
		{
			putErrmsg("Can't re-start the current session.",
					utoa(span.engineId));
			return -1;
		}
	}
	else
	{
		/*	The canceled session isn't the current
		 *	session, but cancelling this session
		 *	reduced the session list length, possibly
		 *	enabling a blocked client to append an SDU
		 *	to the current block.				*/

		sm_SemGive(vspan->bufOpenRedSemaphore);
		sm_SemGive(vspan->bufOpenGreenSemaphore);
	}

	/*	Finally, inform receiver of cancellation.		*/

	return constructSourceCancelReqSegment(&span, &db.ownEngineIdSdnv,
			session->sessionNbr, sessionObj, reasonCode);
}

static int	constructDestCancelReqSegment(LtpSpan *span,
			Sdnv *sourceEngineSdnv, unsigned int sessionNbr,
			SdrObject sessionObj, LtpCancelReasonCode reasonCode)
{
	LtpXmitSeg	segment;
	SdrObject	segmentObj;

	/*	Cancellation by the local engine.			*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCR;
	segment.sessionObj = sessionObj;
	segmentObj = enqueueCancelReqSegment(&segment, span, sourceEngineSdnv,
			sessionNbr, reasonCode);
	if (segmentObj == 0)	/*	Failed to send segment.		*/
	{
		return -1;
	}

	signalLso(span->engineId);
	return 0;
}

static int	cancelSessionByReceiver(LtpImportSession *session,
			SdrObject sessionObj, LtpCancelReasonCode reasonCode)
{
	Sdr	sdr = getIonsdr();
	LtpVdb	*ltpvdb = _ltpvdb(NULL);
	OBJ_POINTER(LtpSpan, span);

	CHKERR(ionLocked());

	/*	Cancel any pending inactivity event for this session.	*/

	if (session->inactivityEvent != 0)
	{
		cancelEvent(LtpStaleImportSession, 0, session->sessionNbr, 0);
		session->inactivityEvent = 0;
	}

	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);
	if (enqueueNotice(ltpvdb->clients + session->clientSvcId,
			span->engineId, session->sessionNbr, 0, 0,
			LtpImportSessionCanceled, reasonCode, 0, 0) < 0)
	{
		putErrmsg("Can't post ImportSessionCanceled notice.", NULL);
		return -1;
	}

	if (ltpvdb->watching & WATCH_CR)
	{
		iwatch('[');
	}

	clearImportSession(session);
	stopImportSession(session);
	session->reasonCode = reasonCode;	/*	For resend.	*/
	sdr_write(sdr, sessionObj, (char *) session, sizeof(LtpImportSession));

	/*	Insert into list of canceled sessions...		*/

	if (sdr_list_insert_last(sdr, span->deadImports, sessionObj) == 0)
	{
		putErrmsg("Can't insert into list of canceled sessions.", NULL);
		return -1;
	}

	/*	...and remove session from list of live imports, so
	 *	that the cancellation won't affect flow control.	*/

	removeImportSession(sessionObj);

	/*	Finally, inform sender of cancellation.			*/

	return constructDestCancelReqSegment(span, &(span->engineIdSdnv),
			session->sessionNbr, sessionObj, reasonCode);
}

static SdrObject enqueueAckSegment(SdrObject spanObj, SdrObject segmentObj)
{
	Sdr		sdr = getIonsdr();
	LtpXmitSegRef	newSegRef;
	SdrObject	newSegRefObj;
	OBJ_POINTER(LtpSpan, span);
	SdrObject	elt;
	OBJ_POINTER(LtpXmitSegRef, segRef);
	OBJ_POINTER(LtpXmitSeg, segment);

	CHKZERO(ionLocked());

	/*	As in enqueueNotice, clear the padding as well as the
	 *	fields: the sdr_write below stores the whole struct.	*/

	memset((char *) &newSegRef, 0, sizeof newSegRef);
	newSegRef.segAddr = segmentObj;
	newSegRef.sessionNbr = 0;	/*	Indicates not data.	*/
	newSegRefObj = sdr_malloc(sdr, sizeof(LtpXmitSegRef));
	CHKZERO(newSegRefObj);
	sdr_write(sdr, newSegRefObj, (char *) &newSegRef,
			sizeof(LtpXmitSegRef));
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);
	for (elt = sdr_list_first(sdr, span->segments); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, LtpXmitSegRef, segRef,
				sdr_list_data(sdr, elt));
		if (segRef->sessionNbr > 0)	/*	Data segment.	*/
		{
			break;			/*	Out of loop.	*/
		}

		/*	Not data but might be some other non-ACK.	*/

		GET_OBJ_POINTER(sdr, LtpXmitSeg, segment, segRef->segAddr);
		switch (segment->pdu.segTypeCode)
		{
		case LtpRS:
		case LtpRAS:
		case LtpCAS:
		case LtpCAR:
			continue;

		default:	/*	Found first non-ACK segment.	*/
			break;			/*	Out of switch.	*/
		}

		break;				/*	Out of loop.	*/
	}

	if (elt)
	{
		elt = sdr_list_insert_before(sdr, elt, newSegRefObj);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, span->segments, newSegRefObj);
	}

	return elt;
}

static int constructCancelAckSegment(LtpXmitSeg *segment, SdrObject spanObj,
		Sdnv *sourceEngineSdnv, unsigned int sessionNbr)
{
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(LtpSpan, span);
	Sdnv	sdnv;
	SdrObject segmentObj;

	CHKERR(ionLocked());
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);
	segment->sessionNbr = sessionNbr;
	segment->remoteEngineId = span->engineId;
	encodeSdnv(&sdnv, sessionNbr);
	segment->pdu.headerLength =
			1 + sourceEngineSdnv->length + sdnv.length + 1;
	segment->pdu.contentLength = 0;
	segment->pdu.trailerLength = 0;
	segment->sessionListElt = 0;
	segment->segmentClass = LtpMgtSeg;
	segmentObj = sdr_malloc(sdr, sizeof(LtpXmitSeg));
	if (segmentObj == 0)
	{
		return -1;
	}

	segment->queueListElt = enqueueAckSegment(spanObj, segmentObj);
	if (segment->queueListElt == 0)
	{
		return -1;
	}

	if (invokeOutboundOnHeaderExtensionGenerationCallbacks(segment) < 0)
	{
		return -1;
	}

	if (invokeOutboundOnTrailerExtensionGenerationCallbacks(segment) < 0)
	{
		return -1;
	}

	sdr_write(sdr, segmentObj, (char *) segment, sizeof(LtpXmitSeg));
	signalLso(span->engineId);
	return 0;
}

static int constructSourceCancelAckSegment(SdrObject spanObj,
		Sdnv *sourceEngineSdnv, unsigned int sessionNbr)
{
	LtpXmitSeg	segment;

	/*	Cancellation by the remote engine (source), ack by
	 *	local engine (destination).				*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCAS;
	return constructCancelAckSegment(&segment, spanObj, sourceEngineSdnv,
			sessionNbr);
}

static int constructDestCancelAckSegment(SdrObject spanObj,
		Sdnv *sourceEngineSdnv, unsigned int sessionNbr)
{
	LtpXmitSeg	segment;

	/*	Cancellation by the remote engine (destination), ack
	 *	by local engine (source).				*/

	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.pdu.segTypeCode = LtpCAR;
	return constructCancelAckSegment(&segment, spanObj, sourceEngineSdnv,
			sessionNbr);
}

static int	initializeRs(LtpXmitSeg *rs, unsigned int rptSerialNbr,
			int checkpointSerialNbrSdnvLength,
			unsigned int rsLowerBound)
{
	Sdnv	sdnv;

	rs->pdu.contentLength = 0;
	rs->pdu.trailerLength = 0;
	rs->pdu.rptSerialNbr = rptSerialNbr;
	encodeSdnv(&sdnv, rs->pdu.rptSerialNbr);
	rs->pdu.contentLength += sdnv.length;
	rs->pdu.contentLength += checkpointSerialNbrSdnvLength;
	rs->pdu.lowerBound = rsLowerBound;
	encodeSdnv(&sdnv, rs->pdu.lowerBound);
	rs->pdu.contentLength += sdnv.length;
	rs->pdu.receptionClaims = sdr_list_create(getIonsdr());
	if (rs->pdu.receptionClaims == 0)
	{
		return -1;
	}

	return 0;
}

static int	constructReceptionClaim(LtpXmitSeg *rs, int lowerBound,
			int upperBound)
{
	Sdr			sdr = getIonsdr();
	SdrObject		claimObj;
	LtpReceptionClaim	claim;
	Sdnv			sdnv;

	CHKERR(ionLocked());
	CHKERR(upperBound > lowerBound);
	claimObj = sdr_malloc(sdr, sizeof(LtpReceptionClaim));
	if (claimObj == 0)
	{
		return -1;
	}

	claim.offset = lowerBound;
	encodeSdnv(&sdnv, claim.offset);
	rs->pdu.contentLength += sdnv.length;
	claim.length = upperBound - lowerBound;
	encodeSdnv(&sdnv, claim.length);
	rs->pdu.contentLength += sdnv.length;
	sdr_write(sdr, claimObj, (char *) &claim, sizeof(LtpReceptionClaim));
	if (sdr_list_insert_last(sdr, rs->pdu.receptionClaims, claimObj)
			== 0)
	{
		return -1;
	}

	return 0;
}

static int	constructRs(LtpXmitSeg *rs, int claimCount,
			LtpImportSession *session)
{
	Sdr	sdr = getIonsdr();
	Sdnv	sdnv;
	SdrObject rsObj;
	OBJ_POINTER(LtpSpan, span);

	CHKERR(ionLocked());
	encodeSdnv(&sdnv, rs->pdu.upperBound);
	rs->pdu.contentLength += sdnv.length;
	encodeSdnv(&sdnv, claimCount);
	rs->pdu.contentLength += sdnv.length;
	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);
	rsObj = sdr_malloc(sdr, sizeof(LtpXmitSeg));
	if (rsObj == 0)
	{
		return -1;
	}

	rs->sessionListElt = sdr_list_insert_last(sdr, session->rsSegments,
			rsObj);
	rs->queueListElt = enqueueAckSegment(session->span, rsObj);
	if (rs->sessionListElt == 0 || rs->queueListElt == 0)
	{
		return -1;
	}

	if (invokeOutboundOnHeaderExtensionGenerationCallbacks(rs) < 0)
	{
		return -1;
	}

	if (invokeOutboundOnTrailerExtensionGenerationCallbacks(rs) < 0)
	{
		return -1;
	}

	sdr_write(sdr, rsObj, (char *) rs, sizeof(LtpXmitSeg));
#if BURST_SIGNALS_ENABLED
	if (enqueueBurst(rs, span, session->rsSegments, REPORTSEGMENT_BURST)
			< 0)
	{
		return -1;
	}
#endif
	signalLso(span->engineId);
#if LTPDEBUG
char	buf[256];
isprintf(buf, sizeof buf, "Sending RS: to " UVAST_FIELDSPEC ", %u to %u, ckpt %u, rpt %u.",
rs->remoteEngineId, rs->pdu.lowerBound,
rs->pdu.upperBound, rs->pdu.ckptSerialNbr, rs->pdu.rptSerialNbr);
putErrmsg(buf, itoa(session->sessionNbr));
#endif
	return 0;
}

static int sendFinalRpt(LtpImportSession *session, SdrObject sessionObj,
		unsigned int checkpointSerialNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned int	reportLowerBound = 0;
	unsigned int	reportUpperBound = session->redPartLength;
	OBJ_POINTER(LtpSpan, span);
	LtpXmitSeg	rsBuf;
	Sdnv		checkpointSerialNbrSdnv;

	/*	We will be sending a single report segment comprising
	 *	a single claim.						*/

	session->reportsCount++;
	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);
	memset((char *) &rsBuf, 0, sizeof(LtpXmitSeg));
	rsBuf.sessionNbr = session->sessionNbr;
	rsBuf.remoteEngineId = span->engineId;
	rsBuf.segmentClass = LtpReportSeg;
	rsBuf.pdu.segTypeCode = LtpRS;
	rsBuf.pdu.ckptSerialNbr = checkpointSerialNbr;
	rsBuf.pdu.headerLength = 1 + span->engineIdSdnv.length
			+ session->sessionNbrSdnv.length + 1;
	encodeSdnv(&checkpointSerialNbrSdnv, checkpointSerialNbr);
	if (initializeRs(&rsBuf, session->nextRptSerialNbr,
			checkpointSerialNbrSdnv.length, reportLowerBound) < 0)
	{
		return -1;
	}

	if (constructReceptionClaim(&rsBuf, reportLowerBound,
			reportUpperBound) < 0)
	{
		return -1;
	}

	rsBuf.pdu.upperBound = reportUpperBound;
	if (constructRs(&rsBuf, 1, session) < 0)
	{
		return -1;
	}

#if LTPDEBUG
putErrmsg("Reporting all data received.", itoa(session->sessionNbr));
#endif
	session->finalRptSerialNbr = rsBuf.pdu.rptSerialNbr;
	sdr_write(sdr, sessionObj, (char *) session, sizeof(LtpImportSession));
	return 0;
}

static int sendReport(LtpImportSession *session, SdrObject sessionObj,
			unsigned int checkpointSerialNbr,
			unsigned int reportSerialNbr,
			unsigned int reportUpperBound)
{
	Sdr		sdr = getIonsdr();
	unsigned int	reportLowerBound = 0;
	SdrObject	elt;
	SdrObject	obj;
	OBJ_POINTER(LtpXmitSeg, oldRpt);
	OBJ_POINTER(LtpSpan, span);
	LtpXmitSeg	rsBuf;
	Sdnv		checkpointSerialNbrSdnv;
	unsigned int	lowerBound;
	unsigned int	upperBound;
	int		claimCount;
	OBJ_POINTER(LtpRecvSeg, ds);
	unsigned int	segmentEnd;

	CHKERR(ionLocked());
	if (session->finalRptSerialNbr != 0)
	{
		/*	Have already sent final report to sender.
		 *	No need to send any more reports; ignore
		 *	this checkpoint.				*/

		return 0;
	}

	if (session->nextRptSerialNbr == 0)	/*	Need 1st nbr.	*/
	{
		do
		{
			session->nextRptSerialNbr = rand();

			/*	Limit serial number SDNV length.	*/

			session->nextRptSerialNbr %= LTP_SERIAL_NBR_LIMIT;
		} while (session->nextRptSerialNbr == 0);
	}
	else					/*	Just add 1.	*/
	{
		session->nextRptSerialNbr++;
	}

	if (session->nextRptSerialNbr == 0)	/*	Rollover.	*/
	{
		return cancelSessionByReceiver(session, sessionObj,
				LtpRetransmitLimitExceeded);
	}

	if (session->redPartLength > 0
	&& session->redPartReceived == session->redPartLength)
	{
		return sendFinalRpt(session, sessionObj, checkpointSerialNbr);
	}

	if (session->reportsCount >= session->maxReports)
	{
#if LTPDEBUG
putErrmsg("Too many reports, canceling session.", itoa(session->sessionNbr));
#endif
		return cancelSessionByReceiver(session, sessionObj,
				LtpRetransmitLimitExceeded);
	}

	/*	Must look for the gaps.					*/

	if (reportSerialNbr != 0)
	{
		/*	Sending report in response to a checkpoint
		 *	that cites a prior report.  Use that
		 *	report's lower bound as the lower bound
		 *	for this report.				*/

		findReport(session, reportSerialNbr, &elt, &obj);
		if (elt == 0)
		{
			putErrmsg("Checkpoint cites invalid report, ignored.",
					itoa(reportSerialNbr));
		}
		else
		{
			GET_OBJ_POINTER(sdr, LtpXmitSeg, oldRpt, obj);
			reportLowerBound = oldRpt->pdu.lowerBound;
		}
	}

	lowerBound = upperBound = reportLowerBound;
	session->reportsCount++;
	GET_OBJ_POINTER(sdr, LtpSpan, span, session->span);

	/*	Set all values that will be common to all report
	 *	segments of this report.				*/

	memset((char *) &rsBuf, 0, sizeof(LtpXmitSeg));
	rsBuf.sessionNbr = session->sessionNbr;
	rsBuf.remoteEngineId = span->engineId;
	rsBuf.segmentClass = LtpReportSeg;
	rsBuf.pdu.segTypeCode = LtpRS;
	rsBuf.pdu.ckptSerialNbr = checkpointSerialNbr;
	rsBuf.pdu.headerLength = 1 + span->engineIdSdnv.length
			+ session->sessionNbrSdnv.length + 1;
	encodeSdnv(&checkpointSerialNbrSdnv, checkpointSerialNbr);

	/*	Initialize the first report segment and start adding
	 *	reception claims.					*/

	if (initializeRs(&rsBuf, session->nextRptSerialNbr,
			checkpointSerialNbrSdnv.length, lowerBound) < 0)
	{
		return -1;
	}

	claimCount = 0;
	for (elt = sdr_list_first(sdr, session->redSegments); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, LtpRecvSeg, ds, sdr_list_data(sdr, elt));
		segmentEnd = ds->pdu.offset + ds->pdu.length;
		if (segmentEnd <= lowerBound)
		{
			continue;	/*	Not in bounds.		*/
		}

		if (ds->pdu.offset <= upperBound)
		{
			upperBound = MIN(segmentEnd, reportUpperBound);
			continue;	/*	Contiguous.		*/
		}

		if (ds->pdu.offset >= reportUpperBound)
		{
			break;		/*	No more to include.	*/
		}

		/*	Gap found; end of reception claim, so
		 *	post it unless it is of zero length (i.e.,
		 *	missing data at start of report scope).		*/

		if (upperBound != lowerBound)
		{
			if (constructReceptionClaim(&rsBuf, lowerBound,
					upperBound) < 0)
			{
				return -1;
			}

			claimCount++;
		}

		lowerBound = ds->pdu.offset;
		upperBound = MIN(segmentEnd, reportUpperBound);
		if (claimCount < MAX_CLAIMS_PER_RS)
		{
			continue;
		}

		/*	Must ship this RS and start another.	*/

		rsBuf.pdu.upperBound = lowerBound;
		if (constructRs(&rsBuf, claimCount, session) < 0)
		{
			return -1;
		}

		/*	We know the session now has a
		 *	nextRptSerialNbr.			*/

		session->nextRptSerialNbr++;
		if (session->nextRptSerialNbr == 0)
		{
			return cancelSessionByReceiver(session,
					sessionObj, LtpRetransmitLimitExceeded);
		}

		if (initializeRs(&rsBuf, session->nextRptSerialNbr,
				checkpointSerialNbrSdnv.length, lowerBound) < 0)
		{
			return -1;
		}

		claimCount = 0;
	}

	if (upperBound == lowerBound)	/*	Nothing to report.	*/
	{
#if LTPDEBUG
putErrmsg("No report, upper bound == lower bound.", itoa(upperBound));
#endif
		return 0;
	}

	/*	Add last reception claim.				*/

	if (constructReceptionClaim(&rsBuf, lowerBound, upperBound) < 0)
	{
		return -1;
	}

	claimCount++;

	/*	Ship final RS of this report.				*/

	rsBuf.pdu.upperBound = reportUpperBound;
	if (constructRs(&rsBuf, claimCount, session) < 0)
	{
		return -1;
	}

#if LTPDEBUG
int	shortfall;
char	buf[256];
shortfall = session->redPartLength - session->redPartReceived;
isprintf(buf, sizeof buf, "Total of %d bytes missing.", shortfall);
putErrmsg(buf, itoa(session->sessionNbr));
#endif
	sdr_write(sdr, sessionObj, (char *) session, sizeof(LtpImportSession));
	return 0;
}

static int constructReportAckSegment(LtpSpan *span, SdrObject spanObj,
		unsigned int sessionNbr, unsigned int reportSerialNbr)
{
	Sdr		sdr = getIonsdr();
	LtpXmitSeg	segment;
	Sdnv		sdnv;
	unsigned int	sessionNbrLength;
	unsigned int	serialNbrLength;
	SdrObject	segmentObj;

	/*	Report acknowledgment by the local engine (sender).	*/

	CHKERR(ionLocked());
	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.sessionNbr = sessionNbr;
	segment.remoteEngineId = span->engineId;
	encodeSdnv(&sdnv, sessionNbr);
	sessionNbrLength = sdnv.length;
	encodeSdnv(&sdnv, reportSerialNbr);
	serialNbrLength = sdnv.length;
	segment.pdu.headerLength = 1 + (_ltpConstants())->ownEngineIdSdnv.length
			+ sessionNbrLength + 1;
	segment.pdu.contentLength = serialNbrLength;
	segment.pdu.trailerLength = 0;
	segment.sessionListElt = 0;
	segment.segmentClass = LtpMgtSeg;
	segment.pdu.segTypeCode = LtpRAS;
	segment.pdu.rptSerialNbr = reportSerialNbr;
	segmentObj = sdr_malloc(sdr, sizeof(LtpXmitSeg));
	if (segmentObj == 0)
	{
		return -1;
	}

	segment.queueListElt = enqueueAckSegment(spanObj, segmentObj);
	if (segment.queueListElt == 0)
	{
		return -1;
	}

	if (invokeOutboundOnHeaderExtensionGenerationCallbacks(&segment) < 0)
	{
		return -1;
	}

	if (invokeOutboundOnTrailerExtensionGenerationCallbacks(&segment) < 0)
	{
		return -1;
	}

	sdr_write(sdr, segmentObj, (char *) &segment, sizeof(LtpXmitSeg));
#if BURST_SIGNALS_ENABLED
	if (enqueueAckBurst(&segment, spanObj, REPORTACK_BURST) < 0)
	{
		return -1;
	}
#endif
	signalLso(span->engineId);
	return 0;
}

/*	*	*	Segment handling functions	*	*	*/

static int	parseTrailerExtensions(char *endOfHeader, LtpPdu *pdu,
			Lyst trailerExtensions)
{
	char		*cursor;
	int		bytesRemaining;
	unsigned int	extensionOffset;
	int		i;
	int		result;

	cursor = endOfHeader + pdu->contentLength;
	bytesRemaining = pdu->trailerLength;
	extensionOffset = pdu->headerLength + pdu->contentLength;
	for (i = 0; i < pdu->trailerExtensionsCount; i++)
	{
		result = ltpei_parse_extension(&cursor, &bytesRemaining,
				trailerExtensions, &extensionOffset);
		if (result != 1)
		{
			return result;
		}
	}

	return 1;
}

static int startImportSession(SdrObject spanObj, unsigned int sessionNbr,
			LtpImportSession *sessionBuf, SdrObject *sessionObj,
			unsigned int clientSvcId, LtpDB *db, LtpVspan *vspan,
			LtpPdu *pdu, LtpVImportSession **vsessionPtr)
{
	Sdr	sdr = getIonsdr();
	LtpSpan	span;
	uvast	heapBufferSize;
	uvast	blockSize;
	SdrObject importBuffer = 0;
	SdrObject importBufferElt = 0;
	SdrObject elt;

	CHKERR(ionLocked());
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	heapBufferSize = db->maxAcqInHeap;

	/*	Override the default heapBufferSize if it is known
	 *	that the total block size is smaller.			*/

	if (pdu->segTypeCode == LtpDsRedEORP || pdu->segTypeCode == LtpDsRedEOB)
	{
		/*	First segment received for this session is
		 *	end of red part.  Its offset + length is the
		 *	total size of the block.			*/

		blockSize = (uvast) pdu->offset + (uvast) pdu->length;
		if (blockSize < heapBufferSize)
		{
			heapBufferSize = blockSize;
		}
	}

	/*	Must grab one of the import heap buffers.		*/

	if (sdr_list_length(sdr, span.importBuffers) == 0)
	{
		if (span.importBufferCount >= span.maxImportSessions)
		{
			/*	All import buffers are currently in
			 *	use; can't start another session.	*/
#if LTPDEBUG
putErrmsg("Cancel by receiver.", utoa(sessionNbr));
#endif
			return 0;
		}

		/*	Must add another import buffer.		*/

		importBuffer = sdr_malloc(sdr, db->maxAcqInHeap);
		if (importBuffer == 0
		|| (importBufferElt = sdr_list_insert_last(sdr,
				span.importBuffers, importBuffer)) == 0)
		{
			putErrmsg("Can't allocate import buffer.", NULL);
			return -1;
		}

		span.importBufferCount++;
		sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	}

	/*	Allocate first available import buffer.  All buffers
	 *	are the same size, may be more than is needed for
	 *	this block Red Part.  But after initial allocations
	 *	there is no further need for dynamic heap buffer
	 *	allocation.
	 *
	 *	Leave the allocated buffer in the list of all
	 *	available buffers for now, in case transaction needs
	 *	to be reversed.  Remove it only at the end.		*/

	importBufferElt = sdr_list_first(sdr, span.importBuffers);
	CHKERR(importBufferElt);
	importBuffer = sdr_list_data(sdr, importBufferElt);
	CHKERR(importBuffer);

	/* check reused buffer size in case heapmax increased */
	if (sdr_object_length(sdr, importBuffer) < db->maxAcqInHeap)
	{

		/* We don't search the entire list but just
		 * increase allocation from the head of the list
		 * until eventually the list is flushed to new
		 * allocation. This might be slower but we don't
		 * anticipate frequent revision to heapmax
		 * Please note: LTP does not re-allocate heap buffer
		 * down. This can be modified to do so but for
		 * now we assume modification is rare and users
		 * are warned of increasing heapmax consequences. */

		/* Free the buffer first and remove from list */
		sdr_free(sdr,importBuffer);
		sdr_list_delete(sdr,importBufferElt,NULL,NULL);
		importBufferElt = 0;

		/* new allocation */
		importBuffer = sdr_malloc(sdr, db->maxAcqInHeap);
		if (importBuffer == 0)
		{
			putErrmsg("Re-allocation of import heap buffer failed.", NULL);
			return -1;
		}
	}

	/*	importSessions list element points to the session
	 *	structure.  importSessionsHash entry points to the
	 *	list element.						*/

	*sessionObj = sdr_malloc(sdr, sizeof(LtpImportSession));
	if (*sessionObj == 0
	|| (elt = sdr_list_insert_last(sdr, span.importSessions,
			*sessionObj)) == 0
	|| sdr_hash_insert(sdr, span.importSessionsHash,
			(char *) &sessionNbr, elt, NULL) < 0)
	{
		return -1;
	}

#if LTPDEBUG
putErrmsg("Opened import session.", utoa(sessionNbr));
#endif
	memset((char *) sessionBuf, 0, sizeof(LtpImportSession));
	sessionBuf->sessionNbr = sessionNbr;
	encodeSdnv(&(sessionBuf->sessionNbrSdnv), sessionNbr);
	sessionBuf->clientSvcId = clientSvcId;
	sessionBuf->redSegments = sdr_list_create(sdr);
	sessionBuf->rsSegments = sdr_list_create(sdr);
	if (sessionBuf->redSegments == 0
	|| sessionBuf->rsSegments == 0)
	{
		putErrmsg("Can't create import session.", NULL);
		return -1;
	}

	sessionBuf->span = spanObj;
	sessionBuf->heapBufferSize = heapBufferSize;
	sessionBuf->heapBufferObj = importBuffer;

	/*	Make sure the initialized session is recorded to
	 *	the database.						*/

	sdr_write(sdr, *sessionObj, (char *) sessionBuf,
			sizeof(LtpImportSession));

	/*	Also add volatile reference to this session.		*/

	addVImportSession(vspan, sessionNbr, elt, vsessionPtr);
	if (*vsessionPtr == NULL)
	{
		putErrmsg("Can't create volatile import session.", NULL);
		return -1;
	}

	/*	Schedule inactivity timeout if enabled for this span.	*/

	if (vspan->sessionInactivityLimit > 0)
	{
		if (setInactivityEvent(sessionBuf, *sessionObj, vspan) < 0)
		{
			putErrmsg("Can't set inactivity event.", NULL);
			return -1;
		}
	}

	/*	All successful.  Remove import buffer (if any) from
	 *	list of available buffers.				*/

	if (importBufferElt)
	{
		sdr_list_delete(sdr, importBufferElt, NULL, NULL);
	}

	return 1;	/*	Import session creation okay.		*/
}

static int createBlockFile(LtpSpan *span, SdrObject sessionObj,
		LtpImportSession *session)
{
	Sdr	sdr = getIonsdr();
	char	cwd[200];
	char	nbrBuf[FQN_MAX_LENGTH];
	char	name[256];
	int	fd;

	if (igetcwd(cwd, sizeof cwd) == NULL)
	{
		putErrmsg("Can't get CWD for block file name.", NULL);
		return -1;
	}

	putFqn(nbrBuf, span->engineId);
	isprintf(name, sizeof name, "%s%cltpblock.%s.%u", cwd,
			ION_PATH_DELIMITER, nbrBuf, session->sessionNbr);
	fd = iopen(name, O_WRONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		putSysErrmsg("Can't create block file", name);
		return -1;
	}

	close(fd);
	session->blockFileRef = zco_create_file_ref(sdr, name, "",
			ZcoInbound);
	if (session->blockFileRef == 0)
	{
		putErrmsg("Can't create block file reference.", NULL);
		return -1;
	}

	istrcpy(session->fileBufferPath, name, sizeof session->fileBufferPath);
	sdr_write(sdr, sessionObj, (char *) session, sizeof(LtpImportSession));
	return 0;
}

static int	insertDataSegment(LtpImportSession *session,
			LtpVImportSession *vsession, LtpRecvSeg *segment,
			LtpPdu *pdu, SdrObject *segmentObj)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	wm = getIonwm();
	int		segUpperBound;
	LtpSegmentRef	arg;
	PsmAddress	rbtNode;
	PsmAddress	nextRbtNode;
	LtpSegmentRef	*nextRef = NULL;
	PsmAddress	prevRbtNode;
	LtpSegmentRef	*prevRef = NULL;
	LtpSegmentRef	refbuf;
	PsmAddress	addr;

	/* Parameter intentionally unused. */
	(void)pdu;

	CHKERR(ionLocked());
	segUpperBound = segment->pdu.offset + segment->pdu.length;
	if (session->redPartLength > 0)	/*	EORP received.		*/
	{
		if (segUpperBound > session->redPartLength)
		{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
			return 0;	/*	Beyond end of red part.	*/
		}
	}

	arg.offset = segment->pdu.offset;
	rbtNode = sm_rbt_search(wm, vsession->redSegmentsIdx,
			orderRedSegments, &arg, &nextRbtNode);
	if (rbtNode)	/*	Segment has already been received.	*/
	{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
		return 0;			/*	Overlap.	*/
	}

	if (nextRbtNode)
	{
		nextRef = (LtpSegmentRef *)
				psp(wm, sm_rbt_data(wm, nextRbtNode));
		prevRbtNode = sm_rbt_prev(wm, nextRbtNode);
		if (prevRbtNode)
		{
			prevRef = (LtpSegmentRef *)
					psp(wm, sm_rbt_data(wm, prevRbtNode));
		}
	}
	else	/*	No segment with greater offset received so far.	*/
	{
		prevRbtNode = sm_rbt_last(wm, vsession->redSegmentsIdx);
		if (prevRbtNode)
		{
			prevRef = (LtpSegmentRef *)
					psp(wm, sm_rbt_data(wm, prevRbtNode));
		}
	}

	if (prevRbtNode
	&& (prevRef->offset + prevRef->length) > segment->pdu.offset)
	{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
		return 0;			/*	Overlap.	*/
	}

if (nextRbtNode && segUpperBound >= 0 && nextRef->offset < (unsigned int)segUpperBound)
	{
#if LTPDEBUG
putErrmsg("discarded segment", itoa(segment->pdu.offset));
#endif
		return 0;			/*	Overlap.	*/
	}

	/*	If we're low on heap space we can't accept the segment,
	 *	because we don't have enough space for the necessary
	 *	accounting objects.					*/

	if (sdr_heap_depleted(sdr))
	{
		return 0;
	}

	/*	Okay to insert this segment into the list.		*/

	session->redPartReceived += segment->pdu.length;
	*segmentObj = sdr_malloc(sdr, sizeof(LtpRecvSeg));
	if (*segmentObj == 0)
	{
		return -1;
	}

	if (nextRef)
	{
		segment->sessionListElt = sdr_list_insert_before(sdr,
				nextRef->sessionListElt, *segmentObj);
	}
	else
	{
		segment->sessionListElt = sdr_list_insert_last(sdr,
				session->redSegments, *segmentObj);
	}

	if (segment->sessionListElt == 0)
	{
		return -1;
	}

	refbuf.offset = segment->pdu.offset;
	refbuf.length = segment->pdu.length;
	refbuf.sessionListElt = segment->sessionListElt;
	addr = psm_zalloc(wm, sizeof(LtpSegmentRef));
	if (addr == 0)
	{
		return -1;
	}

	memcpy((char *) psp(wm, addr), (char *) &refbuf, sizeof(LtpSegmentRef));
	rbtNode = sm_rbt_insert(wm, vsession->redSegmentsIdx, addr,
			orderRedSegments, &refbuf);
	if (rbtNode == 0)
	{
		return -1;
	}

	return segUpperBound;
}

int	getMaxReports(int redPartLength, LtpVspan *vspan, int asReceiver)
{
	/*	The limit on reports is never less than 2: at least
	 *	one negative report, plus the final positive report.
	 *	Additional reports may be authorized depending on the
	 *	size of the transmitted block and the rate of segment
	 *	loss.							*/

	int		dataGapsPerReport = MAX_CLAIMS_PER_RS - 1;
	float		segmentLossRate;
	unsigned int	maxSegmentSize;
	int		maxReportSegments;
	int		xmitBytes;
	int		xmitSegments;
	float		lostSegments;
	int		dataGaps;
	int		reportsIssued;

	if (asReceiver)
	{
		segmentLossRate = vspan->recvSegLossRate;
		maxSegmentSize = vspan->maxRecvSegSize;
	}
	else
	{
		segmentLossRate = vspan->xmitSegLossRate;
		maxSegmentSize = vspan->maxXmitSegSize;
	}

	maxReportSegments = 2;		/*	Minimum value.		*/
	xmitBytes = redPartLength;	/*	Initial transmission.	*/
	while (1)
	{
		xmitSegments = xmitBytes / maxSegmentSize;
		lostSegments = xmitSegments * segmentLossRate;
		if (lostSegments < 1.0)
		{
			break;		/*	No more loss expected.	*/
		}

		/*	Assume segment losses are uncorrelated, so
		 *	each lost segment results in a gap in the
		 *	report's list of claims.  The maximum number
		 *	of lost segments that can be represented in
		 *	a single report is therefore 1 less than the
		 *	maximum number of claims per report segment.	*/

		dataGaps = (int) lostSegments;
		reportsIssued = dataGaps / dataGapsPerReport;
		if (dataGaps % dataGapsPerReport > 0)
		{
			reportsIssued += 1;
		}

		maxReportSegments += reportsIssued;

		/*	Compute next xmit: retransmission data volume.	*/

		xmitBytes = (int) (lostSegments * maxSegmentSize);
	}

#if LTPDEBUG
char	buf[256];
isprintf(buf, sizeof buf, "[i] Max report segments = %d for red part length %d, max segment \
size %d, segment loss rate %f.", maxReportSegments, redPartLength,
maxSegmentSize, segmentLossRate);
writeMemo(buf);
#endif
	return maxReportSegments;
}

static int handleGreenDataSegment(LtpPdu *pdu, char *cursor,
		unsigned int sessionNbr, SdrObject sessionObj, LtpSpan *span,
		LtpVspan *vspan, SdrObject *clientSvcData)
{
	Sdr			sdr = getIonsdr();
	LtpImportSession	sessionBuf;
	ReqTicket		ticket;
	vast			pduLength = 0;
	SdrObject		pduObj;

	ltpSpanTally(vspan, IN_SEG_RECV_GREEN, pdu->length);

	/*	Check for out-of-order segments.			*/

	if (sessionNbr == vspan->redSessionNbr
	&& pdu->offset < vspan->endOfRed)
	{
		/*	Miscolored segment: green before end of red.	*/

		ltpSpanTally(vspan, IN_SEG_MISCOLORED, pdu->length);
		if (sessionObj)		/*	Session exists.		*/
		{
			sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
					sizeof(LtpImportSession));
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionBuf.sessionNbr));
#endif
			cancelSessionByReceiver(&sessionBuf, sessionObj,
					LtpMiscoloredSegment);
		}
		else	/*	Just send cancel segment to sender.	*/
		{
			if (constructDestCancelReqSegment(span,
					&(span->engineIdSdnv), sessionNbr,
					0, LtpMiscoloredSegment) < 0)
			{
				putErrmsg("Can't send CR segment.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		return 0;
	}

	/*	Update segment sequencing information, to enable
	 *	Green-side check for miscolored segments.		*/

	if (sessionNbr == vspan->greenSessionNbr)
	{
		if (pdu->offset < vspan->startOfGreen)
		{
			vspan->startOfGreen = pdu->offset;
		}
	}
	else
	{
		vspan->greenSessionNbr = sessionNbr;
		vspan->startOfGreen = pdu->offset;
	}

	/*	Deliver the client service data.			*/

	if (ionRequestZcoSpace(ZcoInbound, 0, 0, pdu->length, 0, 0, NULL,
			&ticket) < 0)
	{
		putErrmsg("Failed on ionRequest.", NULL);
		return -1;
	}

	if (!(ionSpaceAwarded(ticket)))
	{
		/*	Couldn't service request immediately.		*/

		ionShred(ticket);	/*	Cancel request.		*/
		*clientSvcData = 0;
#if LTPDEBUG
putErrmsg("Can't handle green data, would exceed available ZCO space.",
utoa(pdu->length));
#endif
		return 1;		/*	No Zco created.		*/
	}

	pduObj = sdr_insert(sdr, cursor, pdu->length);
	if (pduObj == 0)
	{
		putErrmsg("Can't record green segment data.", NULL);
		ionShred(ticket);	/*	Cancel request.		*/
		return -1;
	}

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that space has already been awarded.		*/

	pduLength -= pdu->length;
	*clientSvcData = zco_create(sdr, ZcoSdrSource, pduObj, 0, pduLength,
			ZcoInbound);
	switch (*clientSvcData)
	{
	case (SdrObject) ERROR:
		putErrmsg("Can't record green segment data.", NULL);
		ionShred(ticket);	/*	Cancel request.		*/
		return -1;

	case 0:	/*	No ZCO space.  Silently discard segment.	*/
#if LTPDEBUG
putErrmsg("Can't handle green data, would exceed available ZCO space.",
utoa(pdu->length));
#endif
		ionShred(ticket);	/*	Cancel request.		*/
		return 1;
	}

	ionShred(ticket);	/*	Dismiss reservation.		*/
	return 1;
}

static int acceptRedContent(LtpDB *ltpdb, SdrObject *sessionObj,
		LtpImportSession *sessionBuf, unsigned int sessionNbr,
		LtpVImportSession *vsession, SdrObject spanObj, LtpSpan *span,
		LtpVspan *vspan, LtpRecvSeg *segment, int *segUpperBound,
		LtpPdu *pdu, char **cursor)
{
	Sdr		sdr = getIonsdr();
	uvast		endOfSegment;
	uvast		bytesForHeap;
	uvast		bytesForFile;
	SdrObject	sessionElt;
	SdrObject	segmentObj = 0;
	uvast		offsetInFile;
	uvast		endOfIncrement;
	int		fd;

	*segUpperBound = -1;	/*	Default: discard segment and immediate end transaction. */

	/*	Data segment must be accepted into an import session,
	 *	unless that session is already canceled.		*/

	if (*sessionObj)	/*	Active import session found.	*/
	{
		sdr_stage(sdr, (char *) sessionBuf, *sessionObj,
				sizeof(LtpImportSession));

		/*	If block has been delivered to application,
		 *	the ZCO may have been consumed and the block
		 *	file deleted.  Late/retransmitted segments
		 *	arriving after delivery are redundant and must
		 *	not attempt to access the (possibly deleted)
		 *	file.  Similarly, if the blockFileRef has been
		 *	destroyed (set to 0 by clearImportSession), the
		 *	file is gone and no further writes are possible.	*/

		if (sessionBuf->delivered || sessionBuf->blockFileRef == 0)
		{
#if LTPDEBUG
putErrmsg("Discarded late segment: block already delivered.", itoa(sessionNbr));
#endif
			ltpSpanTally(vspan, IN_SEG_REDUNDANT, pdu->length);
			return 0;
		}

		if (sessionBuf->redSegments == 0)
		{
			/*	Reception already completed, just
			 *	waiting for report acknowledgment.
			 *	Discard the segment.			*/
#if LTPDEBUG
putErrmsg("Discarded redundant data segment.", itoa(sessionNbr));
#endif
			ltpSpanTally(vspan, IN_SEG_REDUNDANT, pdu->length);
			return 0;
		}
	}
	else		/*	Active import session not found.	*/
	{
		getCanceledImport(vspan, sessionNbr, sessionObj, &sessionElt);
		if (*sessionObj)
		{
			/*	Session exists but has already been
			 *	canceled.  Discard the segment.		*/
#if LTPDEBUG
putErrmsg("Discarded data segment for canceled session.", itoa(sessionNbr));
#endif
			ltpSpanTally(vspan, IN_SEG_SES_CLOSED, pdu->length);
			return 0;
		}

		/*	Must start a new import session.		*/

		switch (startImportSession(spanObj, sessionNbr, sessionBuf,
				sessionObj, pdu->clientSvcId, ltpdb, vspan,
				pdu, &vsession))
		{
		case -1:
			putErrmsg("Can't create reception session.", NULL);
			return -1;

		case 0:
			/*	Too many sessions; can't let this
			 *	segment start a new one, so discard it.	*/
#if LTPDEBUG
putErrmsg("Discarded data segment: can't start new session.", itoa(sessionNbr));
#endif
			ltpSpanTally(vspan, IN_SEG_SES_CLOSED, pdu->length);
			return 0;
		}
	}

	/* Determine heap and file spaces needed */
	bytesForHeap = pdu->offset < sessionBuf->heapBufferSize ? \
			sessionBuf->heapBufferSize - pdu->offset : 0;
	if (bytesForHeap > pdu->length)
	{
		bytesForHeap = pdu->length;
	}

	endOfSegment = (uvast) pdu->offset + (uvast) pdu->length;
	bytesForFile = endOfSegment > sessionBuf->heapBufferSize ? \
			endOfSegment - sessionBuf->heapBufferSize : 0;
	if (bytesForFile > pdu->length)
	{
		bytesForFile = pdu->length;
	}

	segment->sessionObj = *sessionObj;
	*segUpperBound = insertDataSegment(sessionBuf, vsession, segment, pdu,
			&segmentObj);
	switch (*segUpperBound)
	{
	case 0:
		/*	Segment was found to be useless.  Discard it.	*/

		ltpSpanTally(vspan, IN_SEG_REDUNDANT, pdu->length);
		return 0;

	case -1:
		putErrmsg("Can't insert segment into ImportSession.", NULL);
		return -1;
	}

	/*	Write the red-part reception segment content to the
	 *	session's reception buffer(s).				*/

	ltpSpanTally(vspan, IN_SEG_RECV_RED, pdu->length);
	if (bytesForHeap > 0)
	{
		sdr_stage(sdr, NULL, sessionBuf->heapBufferObj, 0);
		sdr_write(sdr, sessionBuf->heapBufferObj + pdu->offset,
				*cursor, bytesForHeap);
		*cursor += bytesForHeap;
		endOfIncrement = pdu->offset + bytesForHeap;
		if (endOfIncrement > sessionBuf->heapBufferBytes)
		{
			sessionBuf->heapBufferBytes = endOfIncrement;
		}
	}

	if (bytesForFile > 0)
	{
		if (pdu->offset > sessionBuf->heapBufferSize)
		{
			offsetInFile = pdu->offset - sessionBuf->heapBufferSize;
		}
		else
		{
			offsetInFile = 0;
		}

		/*	Create overflow reception buffer file if
		 *	necessary.					*/

		if (sessionBuf->fileBufferPath[0] == '\0')
		{
			if (createBlockFile(span, *sessionObj, sessionBuf) < 0)
			{
				putErrmsg("Can't create block file.", NULL);
				return -1;
			}
		}

		/*	Now write to the reception buffer file.		*/

		fd = iopen(sessionBuf->fileBufferPath, O_WRONLY, 0666);
		if (fd < 0)
		{
			putSysErrmsg("Can't open block file",
					sessionBuf->fileBufferPath);
			return -1;
		}

		/*	(If offsetInFile is after end of file, lseek()
		 *	will succeed and write() will cause the file
		 *	to be lengthened as necessary, with zeroes
		 *	written between the old EOF and the offset
		 *	at which we are writing.)			*/

		if (lseek(fd, offsetInFile, SEEK_SET) < 0)
		{
			putSysErrmsg("Can't seek to offset within block file",
					sessionBuf->fileBufferPath);
			close(fd);
			return -1;
		}

		ssize_t	bytesWritten;

		bytesWritten = write(fd, *cursor, bytesForFile);
		if (bytesWritten < 0 || (uvast)bytesWritten < bytesForFile)
		{
			putSysErrmsg("Can't write to block file",
					sessionBuf->fileBufferPath);
			close(fd);
			return -1;
		}

		close(fd);
		endOfIncrement = offsetInFile + bytesForFile;
		if (endOfIncrement > sessionBuf->blockFileSize)
		{
			sessionBuf->blockFileSize = endOfIncrement;
		}
	}

	sdr_write(sdr, segmentObj, (char *) segment, sizeof(LtpRecvSeg));
	return 0;
}

static int	queueForDelivery(unsigned int clientSvcId, uvast sourceEngineId,
			unsigned int sessionNbr)
{
	Sdr		sdr = getIonsdr();
	LtpDB		*db = getLtpConstants();
	LtpVdb		*vdb = getLtpVdb();
	LtpDeliverable	delivBuf;
	SdrObject	delivObj;
	SdrObject	delivElt;

	delivBuf.clientSvcId = clientSvcId;
	delivBuf.sourceEngineId = sourceEngineId;
	delivBuf.sessionNbr = sessionNbr;
	delivObj = sdr_malloc(sdr, sizeof(LtpDeliverable));
	CHKERR(delivObj);
	delivElt = sdr_list_insert_last(sdr, db->deliverables, delivObj);
	CHKERR(delivElt);
	sdr_write(sdr, delivObj, (char *) &delivBuf, sizeof(LtpDeliverable));
	sm_SemGive(vdb->deliverySemaphore);
	return 0;
}

static int	handleDataSegment(uvast sourceEngineId, LtpDB *ltpdb,
			unsigned int sessionNbr, LtpRecvSeg *segment,
			char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr			sdr = getIonsdr();
	LtpVdb			*ltpvdb = _ltpvdb(NULL);
	LtpPdu			*pdu = &(segment->pdu);
	char			*endOfHeader;
	unsigned int		ckptSerialNbr = 0;
	unsigned int		rptSerialNbr = 0;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	LtpVImportSession	*vsession;
	SdrObject		sessionObj = 0;
	LtpImportSession	sessionBuf;
	SdrObject		spanObj;
	OBJ_POINTER(LtpSpan, span);
	LtpVclient		*client;
	int			result;
	uvast			endOfRed;
	SdrObject		clientSvcData = 0;
	int			segUpperBound;
	OBJ_POINTER(LtpRecvSeg, firstSegment);

	/*	First finish parsing the segment.			*/

	endOfHeader = *cursor;
	extractSmallSdnv(&(pdu->clientSvcId), cursor, bytesRemaining);
	extractSmallSdnv(&(pdu->offset), cursor, bytesRemaining);
	extractSmallSdnv(&(pdu->length), cursor, bytesRemaining);
	if (pdu->segTypeCode > 0 && !(pdu->segTypeCode & LTP_EXC_FLAG))
	{
		/*	This segment is an LTP checkpoint.		*/

		extractSmallSdnv(&ckptSerialNbr, cursor, bytesRemaining);
		extractSmallSdnv(&rptSerialNbr, cursor, bytesRemaining);
	}

	/*	Now we can determine whether or not the data segment
	 *	is usable.						*/

	CHKERR(sdr_begin_xn(sdr));
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
#if LTPDEBUG
putErrmsg("Discarded mystery data segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an unknown engine, so we
		 *	can't process it.				*/

		return sdr_end_xn(sdr);
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray data segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		ltpSpanTally(vspan, IN_SEG_SCREENED, pdu->length);
		return sdr_end_xn(sdr);
	}

	if (*bytesRemaining < 0 || pdu->length > (unsigned int)*bytesRemaining)
	{
#if LTPDEBUG
putErrmsg("Discarded malformed data segment.", itoa(sessionNbr));
#endif
		/*	Malformed segment: data length is overstated.
		 *	Segment must be discarded.			*/

		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	pdu->contentLength = (*cursor - endOfHeader) + pdu->length;

	/*	At this point, the remaining bytes should all be
	 *	client service data and trailer extensions.  So
	 *	next we parse the trailer extensions.			*/

	pdu->trailerLength = *bytesRemaining - pdu->length;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle data segment.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	/*	Discard segments that arrive after their import session
	 *	has already been closed (tombstone present in
	 *	span->closedImports).  This applies equally to Red and
	 *	Green data segments: a late Green segment for a closed
	 *	session previously fell through this check, reached
	 *	getImportSession with sessionObj=0, and was handed to
	 *	handleGreenDataSegment which would deliver stale data
	 *	to the client.  See #1020.				*/

	if (sessionIsClosed(vspan, sessionNbr))
	{
#if LTPDEBUG
putErrmsg("Discarding late data segment.", itoa(sessionNbr));
#endif
		ltpSpanTally(vspan, IN_SEG_REDUNDANT, pdu->length);
		return sdr_end_xn(sdr);
	}

	if ((pdu->segTypeCode & LTP_EXC_FLAG) == 0)	/*	Red.	*/
	{
		if (sdr_list_length(sdr, ltpdb->deliverables) >=
				ltpdb->maxBacklog)
		{
#if LTPDEBUG
putErrmsg("Delivery backed up, discarding Red segment.", itoa(sessionNbr));
#endif
			/*	Already have reached the limit for
			 *	blocks pending delivery to clients,
			 *	throwing red segments away until the
			 *	backlog comes down.			*/

			ltpSpanTally(vspan, IN_SEG_TOO_FAST, pdu->length);
			return sdr_end_xn(sdr);
		}
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	/*	Now process the segment.				*/

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);
	getImportSession(vspan, sessionNbr, &vsession, &sessionObj);
	segment->segmentClass = LtpDataSeg;
	if (pdu->clientSvcId > MAX_LTP_CLIENT_NBR
	|| (client = ltpvdb->clients + pdu->clientSvcId)->pid == ERROR)
	{
		/*	Data segment is for an unknown client service,
		 *	so must discard it and cancel the session.	*/

		ltpSpanTally(vspan, IN_SEG_UNK_CLIENT, pdu->length);
		if (sessionObj)	/*	Session already exists.		*/
		{
			sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
					sizeof(LtpImportSession));
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionBuf.sessionNbr));
#endif
			cancelSessionByReceiver(&sessionBuf, sessionObj,
					LtpClientSvcUnreachable);
		}
		else
		{
			if (constructDestCancelReqSegment(span,
					&(span->engineIdSdnv), sessionNbr, 0,
					LtpClientSvcUnreachable) < 0)
			{
				putErrmsg("Can't send CR segment.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle data segment.", NULL);
			return -1;
		}

#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		return 0;
	}

	if (pdu->segTypeCode & LTP_EXC_FLAG)
	{
		/*	This is a green-part data segment; if valid,
		 *	deliver immediately to client service.		*/

		result = handleGreenDataSegment(pdu, *cursor, sessionNbr,
				sessionObj, span, vspan, &clientSvcData);
		if (result < 0)
		{
			sdr_cancel_xn(sdr);
			return result;
		}

		if (clientSvcData)
		{
			enqueueNotice(client, sourceEngineId, sessionNbr,
					pdu->offset, pdu->length,
					LtpRecvGreenSegment, 0,
					(pdu->segTypeCode == LtpDsGreenEOB),
					clientSvcData);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle green-part segment.", NULL);
			return -1;
		}

		return result;	/*	Green-part data handled okay.	*/
	}

	/*	This is a red-part data segment.			*/

	endOfRed = (uvast) pdu->offset + (uvast) pdu->length;
	if (sessionNbr == vspan->greenSessionNbr
	&& endOfRed > vspan->startOfGreen)
	{
		/*	Miscolored segment: red after start of green.	*/

		ltpSpanTally(vspan, IN_SEG_MISCOLORED, pdu->length);
		if (sessionObj)		/*	Session exists.		*/
		{
			sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
					sizeof(LtpImportSession));
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionBuf.sessionNbr));
#endif
			cancelSessionByReceiver(&sessionBuf, sessionObj,
					LtpMiscoloredSegment);
		}
		else	/*	Just send cancel segment to sender.	*/
		{
			if (constructDestCancelReqSegment(span,
					&(span->engineIdSdnv), sessionNbr,
					0, LtpMiscoloredSegment) < 0)
			{
				putErrmsg("Can't send CR segment.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle miscolored red seg.", NULL);
			return -1;
		}

#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		return 0;
	}

	/*	Update segment sequencing information, to enable
	 *	Red-side check for miscolored segments.			*/

	if (sessionNbr == vspan->redSessionNbr)
	{
		if (endOfRed > vspan->endOfRed)
		{
			vspan->endOfRed = endOfRed;
		}
	}
	else
	{
		vspan->redSessionNbr = sessionNbr;
		vspan->endOfRed = endOfRed;
	}

	/*	Process the red data segment content.			*/

	if (acceptRedContent(ltpdb, &sessionObj, &sessionBuf, sessionNbr,
			vsession, spanObj, span, vspan, segment,
			&segUpperBound, pdu, cursor) < 0)
	{
		/*	acceptRedContent's failure paths are all I/O on
		 *	the per-session block file (open / lseek / write):
		 *	transient disk errors and the missing-block-file
		 *	race documented in #1020.  Routing those through
		 *	sdr_cancel_xn on a non-reversible SDR with
		 *	already-modified state triggers handleUnrecoverable
		 *	Error -> sm_Abort, which orphans the SDR mutex
		 *	and cascades into every other daemon (the udplsi
		 *	4.66 s death in #1020 ends a whole node this way).
		 *
		 *	Commit the partial state via sdr_drop_xn so the
		 *	transaction closes cleanly.  The session record may
		 *	now point at a missing or partial block file -- a
		 *	bounded leak that the session-expiration path will
		 *	clean up -- but the daemon stays alive and the next
		 *	inbound segment processes normally.		*/

		putErrmsg("Can't accept data segment content.", NULL);
		sdr_drop_xn(sdr,
				"acceptRedContent failed (block file I/O error)");
		return -1;
	}

	if (segUpperBound == -1) /*	segment discarded and not act upon further 	*/
	{
#if LTPDEBUG
putErrmsg("Discarded data segment.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	/*	Based on the segment type code, infer additional
	 *	session information and do additional processing.	*/

	/*  check if segment data was received and inserted into buffer */

	if (segUpperBound > 0)
	{
		if ((pdu->segTypeCode == LtpDsRedEORP
		|| pdu->segTypeCode == LtpDsRedEOB)
		&& sessionBuf.redSegments != 0)
		{
			/*	This segment is the end of the red part of
			 *	the block, so the end of its data is the end
			 *	of the red part.				*/

			sessionBuf.redPartLength = segUpperBound;
			GET_OBJ_POINTER(sdr, LtpRecvSeg, firstSegment,
					sdr_list_data(sdr, sdr_list_first(sdr,
					sessionBuf.redSegments)));
			if (firstSegment->pdu.length > vspan->maxRecvSegSize)
			{
				vspan->maxRecvSegSize = firstSegment->pdu.length;
				computeRetransmissionLimits(vspan);
			}

			/*	We can now compute an upper limit on the
			 *	number of report segments we can send back
			 *	for this session.				*/

			sessionBuf.maxReports = getMaxReports(sessionBuf.redPartLength,
					vspan, 1);
		}

		if ((pdu->segTypeCode & LTP_FLAG_1)
		&& (pdu->segTypeCode & LTP_FLAG_0))
		{
			/*	This segment is the end of the block.		*/

			sessionBuf.endOfBlockRecd = 1;
		}
	}

	/* act on check point, even if data is duplicate */

	if ((pdu->segTypeCode > 0)
		|| ((sessionBuf.redPartLength > 0)
			&& (sessionBuf.redPartReceived == sessionBuf.redPartLength)))
	{
		/* Generate either a synchronous report (respond to checkpoint) or
		 * asynchronous report (respond to receiving all red part out-of-order).
		 */

		ltpSpanTally(vspan, CKPT_RECV, 0);

		/* if the data in CP was duplicate, set upperbound to redPartLength */

		if (segUpperBound == 0)
		{
			/* if red part length is known */
			if (sessionBuf.redPartLength > 0)
			{
				segUpperBound = sessionBuf.redPartLength;
			}
			else
			{
				/* if redpart length unknown, end transaction */
#if LTPDEBUG
putErrmsg("Check point not acted on.", itoa(sessionNbr));
#endif
				return sdr_end_xn(sdr);
			}
		}

		if (sendReport(&sessionBuf, sessionObj, ckptSerialNbr,
				rptSerialNbr, segUpperBound) < 0)
		{
			putErrmsg("Can't send reception report.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (sessionBuf.redPartReceived == sessionBuf.redPartLength
		&& sessionBuf.redSegments != 0)
		{
			/*	The entire red part of the block has
			 *	been received, and has not yet been
			 *	delivered, so deliver it to the client
			 *	service.				*/

			if (queueForDelivery(pdu->clientSvcId, sourceEngineId,
					sessionNbr) < 0)
			{
				putErrmsg("Can't deliver service data.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}
	}

	/*	Reset inactivity timer if enabled.			*/

	if (vspan->sessionInactivityLimit > 0)
	{
		if (setInactivityEvent(&sessionBuf, sessionObj, vspan) < 0)
		{
			putErrmsg("Can't reset inactivity event.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*	Processing of data segment is now complete.  Rewrite
	 *	session to preserve any changes made.			*/

	sdr_write(sdr, sessionObj, (char *) &sessionBuf,
			sizeof(LtpImportSession));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle data segment.", NULL);
		return -1;
	}

	return 0;	/*	Red-part data handled okay.		*/
}

static int	loadClaimsArray(char **cursor, int *bytesRemaining,
			unsigned int claimCount, LtpReceptionClaim *claims,
			unsigned int lowerBound, unsigned int upperBound)
{
	unsigned int			i;
	LtpReceptionClaim	*claim;
	unsigned int		offset;
	unsigned int		dataEnd;

	for (i = 0, claim = claims; i < claimCount; i++, claim++)
	{
		/*	For transmission ONLY (never in processing
		 *	within the LTP engine), claim->offset is
		 *	compressed to offset from report segment's
		 *	lower bound rather than from start of block.	*/

		extractSmallSdnv(&offset, cursor, bytesRemaining);
		claim->offset = offset + lowerBound;
		extractSmallSdnv(&(claim->length), cursor, bytesRemaining);
		if (claim->length == 0)
		{
			return 0;
		}

		dataEnd = claim->offset + claim->length;
		if (dataEnd > upperBound)
		{
			return 0;
		}
	}

	return 1;
}

static int	insertClaim(LtpExportSession *session, LtpReceptionClaim *claim)
{
	Sdr	sdr = getIonsdr();
	SdrObject claimObj;

	CHKERR(ionLocked());
	claimObj = sdr_malloc(sdr, sizeof(LtpReceptionClaim));
	if (claimObj == 0)
	{
		return -1;
	}

	if (sdr_list_insert_last(sdr, session->claims, claimObj) == 0)
	{
		return -1;
	}

	sdr_write(sdr, claimObj, (char *) claim, sizeof(LtpReceptionClaim));
	return 0;
}

static SdrObject insertCheckpoint(LtpExportSession *session, LtpXmitSeg *segment)
{
	Sdr	sdr = getIonsdr();
	LtpCkpt	checkpoint;
	SdrObject obj;

	checkpoint.serialNbr = segment->pdu.ckptSerialNbr;
	checkpoint.sessionListElt = segment->sessionListElt;
	obj = sdr_malloc(sdr, sizeof(LtpCkpt));
	if (obj == 0)
	{
		putErrmsg("Can't create checkpoint reference.", NULL);
		return 0;
	}

	sdr_write(sdr, obj, (char *) &checkpoint, sizeof(LtpCkpt));
	return sdr_list_insert_last(sdr, session->checkpoints, obj);
}

static int	constructDataSegment(Sdr sdr, LtpExportSession *session,
			SdrObject sessionObj, unsigned int reportSerialNbr,
			unsigned int checkpointSerialNbr, LtpVspan *vspan,
			LtpSpan *span, LystElt extentElt)
{
	int		lastExtent = (lyst_next(extentElt) == NULL);
	LtpExportExtent	*extent;
	LtpXmitSegRef	segRef;
	SdrObject	segRefObj;
	LtpXmitSeg	segment;
	Sdnv		offsetSdnv;
	int		remainingRedBytes;
	int		redBytesToSegment;
	int		length;
	int		dataSegmentOverhead;
	int		checkpointOverhead;
	int		worstCaseSegmentSize;
	Sdnv		rsnSdnv;
	Sdnv		cpsnSdnv;
	int		isCheckpoint = 0;
	int		isEor = 0;		/*	End of red part.*/
	int		isEob = 0;		/*	End of block.	*/
	Sdnv		lengthSdnv;

	extent = (LtpExportExtent *) lyst_data(extentElt);

	/*	Create new data segment, create reference to the
	 *	new data segment, enqueue that reference, and note
	 *	the queue element in the new segment itself.		*/

	segRef.sessionNbr = session->sessionNbr;
	segRef.segAddr = sdr_malloc(sdr, sizeof(LtpXmitSeg));
	if (segRef.segAddr == 0)
	{
		return -1;
	}

	segRefObj = sdr_malloc(sdr, sizeof(LtpXmitSegRef));
	if (segRefObj == 0)
	{
		return -1;
	}

	sdr_write(sdr, segRefObj, (char *) &segRef, sizeof(LtpXmitSegRef));
	memset((char *) &segment, 0, sizeof(LtpXmitSeg));
	segment.queueListElt = sdr_list_insert_last(sdr, span->segments,
			segRefObj);
	if (segment.queueListElt == 0)
	{
		return -1;
	}

	/*	Compute length of segment's known overhead.		*/

	segment.pdu.headerLength = 1 + (_ltpConstants())->ownEngineIdSdnv.length
			+ session->sessionNbrSdnv.length + 1;
	segment.pdu.ohdLength = session->clientSvcIdSdnv.length;
	encodeSdnv(&offsetSdnv, extent->offset);
	segment.pdu.ohdLength += offsetSdnv.length;

	/*	Determine length of segment.   Note that any single
	 *	segmentation extent might encompass red data only,
	 *	green data only, or some red data followed by some
	 *	green data.						*/

	remainingRedBytes = session->redPartLength - extent->offset;
	if (remainingRedBytes > 0)	/*	This is a red segment.	*/
	{
		/*	Segment must be all one color, so the maximum
		 *	length of data in the segment is the number of
		 *	red-part bytes remaining in the extent that is
		 *	to be segmented.				*/

		if (remainingRedBytes >= 0 && (unsigned int)remainingRedBytes > extent->length)
		{
			/*	This extent encompasses part (but not
			 *	all) of the block's remaining red data.	*/

			redBytesToSegment = extent->length;
		}
		else
		{
			/*	This extent encompasses all remaining
			 *	red data and zero or more bytes of
			 *	green data as well.			*/

			redBytesToSegment = remainingRedBytes;
		}

		length = redBytesToSegment;	/*	Initial guess.	*/

		/*	Compute worst-case segment size.		*/

		encodeSdnv(&lengthSdnv, length);
		dataSegmentOverhead = segment.pdu.headerLength
				+ segment.pdu.ohdLength + lengthSdnv.length;
		checkpointOverhead = 0;

		/*	In the worst case, this segment will be the
		 *	end of this red-part transmission cycle and
		 *	will therefore contain a non-zero checkpoint
		 *	serial number of sdnv-encoded length up to
		 *	10 and also the report serial number.		*/

		if (lastExtent)
		{
			encodeSdnv(&rsnSdnv, reportSerialNbr);
			checkpointOverhead += rsnSdnv.length;
			encodeSdnv(&cpsnSdnv, checkpointSerialNbr);
			checkpointOverhead += cpsnSdnv.length;
		}

		worstCaseSegmentSize = length
				+ dataSegmentOverhead + checkpointOverhead;
		if (worstCaseSegmentSize >= 0
			&& (unsigned int)worstCaseSegmentSize > span->maxSegmentSize)
		{
			/*	Must reduce length.  So this segment's
			 *	last data byte can't be the last data
			 *	byte of the red data we're sending in
			 *	this red-part transmission cycle, so
			 *	segment can't be a checkpoint.  So
			 *	forget checkpoint overhead and set
			 *	data length to (max segment size minus
			 *	ordinary data segment overhead) or the
			 *	total redBytesToSegment (minus 1 if
			 *	this would otherwise be the last segment
			 *	of the last extent; we must have one
			 *	more segment, serving as checkpoint,
			 *	which must have at least 1 byte of
			 *	data), whichever is less.		*/

			checkpointOverhead = 0;
			length = span->maxSegmentSize - dataSegmentOverhead;
			if (lastExtent)
			{
				if (length >= redBytesToSegment)
				{
					length = redBytesToSegment - 1;
				}
			}
			else
			{
				if (length > redBytesToSegment)
				{
					length = redBytesToSegment;
				}
			}

			encodeSdnv(&lengthSdnv, length);
		}
		else
		{
			/*	The red remainder of this extent fits
			 *	in one segment, no matter what.  If
			 *	this is the last extent, then this
			 *	segment is a checkpoint.		*/

			if (lastExtent)
			{
				isCheckpoint = 1;
				if (length == remainingRedBytes)
				{
					isEor = 1;
					if (session->redPartLength ==
							session->totalLength)
					{
						isEob = 1;
					}
				}
			}
		}
	}
	else
	{
		/*	No remaining red data in this extent, so the
		 *	segment will be green, so there are no serial
		 *	numbers, so segment overhead is now known.	*/

		length = extent->length;
		encodeSdnv(&lengthSdnv, length);
		dataSegmentOverhead = segment.pdu.headerLength +
				segment.pdu.ohdLength + lengthSdnv.length;
		worstCaseSegmentSize = length + dataSegmentOverhead;
		if (worstCaseSegmentSize >= 0
			&& (unsigned int)worstCaseSegmentSize > span->maxSegmentSize)
		{
			/*	Must reduce length, so cannot be end
			 *	of green part (which is end of block).	*/

			length = span->maxSegmentSize - dataSegmentOverhead;
			encodeSdnv(&lengthSdnv, length);
		}
		else	/*	Remainder of extent fits in one segment.*/
		{
			if (lastExtent)
			{
				isEob = 1;
			}
		}
	}

	/*	Now have enough information to finish the segment.	*/

	segment.sessionNbr = session->sessionNbr;
	segment.remoteEngineId = span->engineId;
	segment.segmentClass = LtpDataSeg;
	segment.pdu.segTypeCode = 0;
	if (remainingRedBytes > 0)	/*	Segment is in red part.	*/
	{
		segment.sessionListElt = sdr_list_insert_last(sdr,
				session->redSegments, segRef.segAddr);
		if (segment.sessionListElt == 0)
		{
			return -1;
		}

		/*	Set flags, depending on whether or not isEor.	*/

		if (isEor)	/*	End of red part.		*/
		{
			segment.pdu.segTypeCode |= LTP_FLAG_1;

			/*	End of red part is always a checkpoint.	*/

			segment.sessionObj = sessionObj;
			if (isEob)
			{
				segment.pdu.segTypeCode |= LTP_FLAG_0;
			}
		}
		else		/*	Not end of red part of block.	*/
		{
			if (isCheckpoint)	/*	Retransmission.	*/
			{
				segment.sessionObj = sessionObj;
				segment.pdu.segTypeCode |= LTP_FLAG_0;
			}
		}
	}
	else	/*	Green-part segment.				*/
	{
		segment.sessionListElt = sdr_list_insert_last(sdr,
				session->greenSegments, segRef.segAddr);
		if (segment.sessionListElt == 0)
		{
			return -1;
		}

		segment.pdu.segTypeCode |= LTP_EXC_FLAG;
		if (isEob)
		{
			segment.pdu.segTypeCode |= LTP_FLAG_1;
			segment.pdu.segTypeCode |= LTP_FLAG_0;
		}
	}

	if (isCheckpoint)
	{
		segment.pdu.ckptSerialNbr = checkpointSerialNbr;
		segment.pdu.ohdLength += cpsnSdnv.length;
		segment.pdu.rptSerialNbr = reportSerialNbr;
		segment.pdu.ohdLength += rsnSdnv.length;
		segment.ckptListElt = insertCheckpoint(session, &segment);
		if (segment.ckptListElt == 0)
		{
			return -1;
		}

		session->prevCkptSerialNbr = checkpointSerialNbr;
		sdr_write(sdr, sessionObj, (char *) session,
				sizeof(LtpExportSession));
	}

	segment.pdu.clientSvcId = session->clientSvcId;
	segment.pdu.offset = extent->offset;
	segment.pdu.length = length;
	encodeSdnv(&lengthSdnv, segment.pdu.length);
	segment.pdu.ohdLength += lengthSdnv.length;
	segment.pdu.contentLength = segment.pdu.ohdLength + segment.pdu.length;
	segment.pdu.trailerLength = 0;
	segment.pdu.block = session->svcDataObjects;
	if (invokeOutboundOnHeaderExtensionGenerationCallbacks(&segment) < 0)
	{
		return -1;
	}

	if (invokeOutboundOnTrailerExtensionGenerationCallbacks(&segment) < 0)
	{
		return -1;
	}

	sdr_write(sdr, segRef.segAddr, (char *) &segment, sizeof(LtpXmitSeg));
#if BURST_SIGNALS_ENABLED
	if (isCheckpoint)
	{
		if (enqueueBurst(&segment, span, session->redSegments,
				CHECKPOINT_BURST) < 0)
		{
			return -1;
		}
	}
#endif
	signalLso(span->engineId);
#if LTPDEBUG
char	nbrBuf[FQN_MAX_LENGTH];
char	buf[256];
if (segment.pdu.segTypeCode > 0)
{
putFqn(nbrBuf, segment.remoteEngineId);
isprintf(buf, sizeof buf, "Sending checkpoint: ckpt %u rpt %u to engine %s.",
segment.pdu.ckptSerialNbr, segment.pdu.rptSerialNbr, nbrBuf);
putErrmsg(buf, itoa(session->sessionNbr));
}
#endif
	extent->offset += length;
	extent->length -= length;
	if ((_ltpvdb(NULL))->watching & WATCH_e)
	{
		iwatch('e');
	}

	ltpSpanTally(vspan, OUT_SEG_QUEUED, length);
	if (reportSerialNbr)
	{
		ltpSpanTally(vspan, SEG_RE_XMIT, length);
	}

	return 0;
}

int	issueSegments(Sdr sdr, LtpSpan *span, LtpVspan *vspan,
		LtpExportSession *session, SdrObject sessionObj, Lyst extents,
		unsigned int reportSerialNbr, unsigned int checkpointSerialNbr)
{
	LystElt		extentElt;
	LtpExportExtent	*extent;

	CHKERR(session);
	if (session->svcDataObjects == 0)	/*	Canceled.	*/
	{
		return 0;
	}

	CHKERR(ionLocked());
	CHKERR(span);
	CHKERR(vspan);
	CHKERR(extents);

	/*	For each segment issuance extent, construct as many
	 *	data segments as are needed in order to send all
	 *	service data within that extent of the aggregate block.	*/

	for (extentElt = lyst_first(extents); extentElt;
			extentElt = lyst_next(extentElt))
	{
		extent = (LtpExportExtent *) lyst_data(extentElt);
		while (extent->length > 0)
		{
			if (constructDataSegment(sdr, session, sessionObj,
					reportSerialNbr, checkpointSerialNbr,
					vspan, span, extentElt) < 0)
			{
				putErrmsg("Can't segment block.",
						itoa(vspan->meterPid));
				return -1;
			}
		}
	}

	/*	Return the number of extents processed.			*/

	return lyst_length(extents);
}

static void getSessionContext(LtpDB *ltpdb, unsigned int sessionNbr,
		SdrObject *sessionObj, LtpExportSession *sessionBuf,
		SdrObject *spanObj, LtpSpan *spanBuf, LtpVspan **vspan,
		PsmAddress *vspanElt)
{
	Sdr	sdr = getIonsdr();

	CHKVOID(ionLocked());
	*spanObj = 0;		/*	Default: no context.		*/
	getExportSession(sessionNbr, sessionObj);
	if (*sessionObj != 0)	/*	Known session.			*/
	{
		sdr_stage(sdr, (char *) sessionBuf, *sessionObj,
				sizeof(LtpExportSession));
		if (sessionBuf->totalLength > 0)/*	A live session.	*/
		{
			*spanObj = sessionBuf->span;
		}
	}

	if (*spanObj == 0)	/*	Can't set session context.	*/
	{
		return;
	}

	sdr_read(sdr, (char *) spanBuf, *spanObj, sizeof(LtpSpan));
	findSpan(spanBuf->engineId, vspan, vspanElt);
	if (*vspanElt == 0
	|| ((*vspan)->receptionRate == 0 && ltpdb->enforceSchedule == 1))
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		*spanObj = 0;		/*	Disable acknowledgment.	*/
	}
}

static int	addTransmissionExtent(Lyst extents, unsigned int startOfGap,
			unsigned int endOfGap)
{
	LtpExportExtent	*extent;

	if ((extent = (LtpExportExtent *) MTAKE(sizeof(LtpExportExtent)))
			== NULL)
	{
		putErrmsg("Can't add retransmission extent.", NULL);
		return -1;
	}

	extent->offset = startOfGap;
	extent->length = endOfGap - startOfGap;
#if LTPDEBUG
char	xmitbuf[256];
isprintf(xmitbuf, sizeof xmitbuf, "      retransmitting from %d to %d.", extent->offset,
extent->offset + extent->length);
putErrmsg(xmitbuf, NULL);
#endif
	if (lyst_insert_last(extents, extent) == NULL)
	{
		putErrmsg("Can't add retransmission extent.", NULL);
		return -1;
	}

	return 0;
}

static int	handleRS(LtpDB *ltpdb, unsigned int sessionNbr,
			LtpRecvSeg *segment, char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr			sdr = getIonsdr();
	LtpVdb			*ltpvdb = _ltpvdb(NULL);
	LtpPdu			*pdu = &(segment->pdu);
	int			ltpMemIdx = getIonMemoryMgr();
	char			*endOfHeader;
	unsigned int		rptSerialNbr;
	unsigned int		ckptSerialNbr;
	unsigned int		rptUpperBound;
	unsigned int		rptLowerBound;
	unsigned int		claimCount;
	LtpReceptionClaim	*newClaims;
	SdrObject		sessionObj;
	LtpExportSession	sessionBuf;
	SdrObject		spanObj = 0;
	LtpSpan			spanBuf;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	SdrObject		elt;
	unsigned int		oldRptSerialNbr;
	SdrObject		dsObj;
	Lyst			claims;
	SdrObject		claimObj;
	SdrObject		nextElt;
	LtpReceptionClaim	*claim;
	unsigned int		claimEnd;
	LtpReceptionClaim	*newClaim;
	unsigned int		newClaimEnd;
	LystElt			elt2;
	LystElt			nextElt2;
	unsigned int			i;
	Lyst			extents;
	unsigned int		startOfGap;
	unsigned int		endOfGap;
#if LTPDEBUG
putErrmsg("Handling report.", utoa(sessionNbr));
#endif

	/*	First finish parsing the segment.  Load all the
	 *	reception claims in the report into an array of new
	 *	claims.							*/

	endOfHeader = *cursor;
	extractSmallSdnv(&rptSerialNbr, cursor, bytesRemaining);
	extractSmallSdnv(&ckptSerialNbr, cursor, bytesRemaining);
	extractSmallSdnv(&rptUpperBound, cursor, bytesRemaining);
	extractSmallSdnv(&rptLowerBound, cursor, bytesRemaining);
	extractSmallSdnv(&claimCount, cursor, bytesRemaining);
#if LTPDEBUG
char	rsbuf[256];
isprintf(rsbuf, sizeof rsbuf, "[i] Got RS %u for checkpoint %u; %u claims from %u to %u.",
rptSerialNbr, ckptSerialNbr, claimCount, rptLowerBound, rptUpperBound);
putErrmsg(rsbuf, utoa(sessionNbr));
#endif
	/*	Each reception claim is at least two bytes on the wire
	 *	(lower-bound and length SDNVs), so a claim count that
	 *	exceeds the bytes remaining in the segment is malformed.
	 *	Reject it before sizing the allocation: on 32-bit builds
	 *	claimCount * sizeof(LtpReceptionClaim) would otherwise
	 *	overflow size_t, under-allocate, and then be written past
	 *	by loadClaimsArray().					*/
	if (claimCount > (unsigned int) *bytesRemaining)
	{
		return 0;		/*	Malformed report.	*/
	}

	newClaims = (LtpReceptionClaim *)
			MTAKE(claimCount * sizeof(LtpReceptionClaim));
	if (newClaims == NULL)
	{
		/*	Too many claims; could be a DOS attack.		*/
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		return 0;		/*	Ignore report.		*/
	}

	if (loadClaimsArray(cursor, bytesRemaining, claimCount, newClaims,
			rptLowerBound, rptUpperBound) == 0)
	{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		MRELEASE(newClaims);
		return 0;		/*	Ignore report.		*/
	}

	/*	Now determine whether or not the RS is usable.		*/

	CHKERR(sdr_begin_xn(sdr));
	getSessionContext(ltpdb, sessionNbr, &sessionObj,
			&sessionBuf, &spanObj, &spanBuf, &vspan, &vspanElt);
	if (spanObj == 0)	/*	Invalid provenance.		*/
	{
		MRELEASE(newClaims);
#if CLOSED_EXPORTS_ENABLED
		if (acknowledgeLateReport(sessionNbr, rptSerialNbr))
		{
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't send RA segment.", NULL);
				return -1;
			}

			return 0;
		}
#endif
		/*	Default behavior for late report segment:
		 *
		 *	Either session is unknown (or dead, i.e.,
		 *	session number has been re-used) or else
		 *	this RS was received from an engine that is
		 *	not supposed to be sending at this time.  In
		 *	either case, we simply discard this RS.
		 *
		 *	If the session is unknown, then the span on
		 *	which the data segments of the session were
		 *	transmitted is unknown; in that case, the
		 *	span on which an acknowledgment of this RS
		 *	would have to be sent is likewise unknown, so
		 *	we can't acknowledge the RS.
		 *
		 *	Reception of a report for an unknown session
		 *	probably results from the receiver's response
		 *	to the arrival of retransmitted segments
		 *	following session closure at the receiving
		 *	engine.  In that case the remote import
		 *	session is an erroneous resurrection
		 *	of a closed session and we need to help the
		 *	remote engine terminate it.  Ignoring the
		 *	report does so: the report will time out
		 *	and be retransmitted N times and then will
		 *	cause the session to fail and be canceled
		 *	by receiver -- exactly the correct result.	*/

	#if LTPDEBUG
		putErrmsg("Unknown session: discarding report.", NULL);
	#endif
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	At this point, the remaining bytes should all be
	 *	trailer extensions.  We now parse them.			*/

	pdu->contentLength = (*cursor - endOfHeader);
	pdu->trailerLength = *bytesRemaining;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle report segment.", NULL);
		MRELEASE(newClaims);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		MRELEASE(newClaims);
		return sdr_end_xn(sdr);
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		MRELEASE(newClaims);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		MRELEASE(newClaims);
		return sdr_end_xn(sdr);
	}

	/*	Acknowledge the report if possible.			*/

	if (constructReportAckSegment(&spanBuf, spanObj, sessionNbr,
			rptSerialNbr))
	{
		putErrmsg("Can't send RA segment.", NULL);
		MRELEASE(newClaims);
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Now process the report if possible.  First, discard
	 *	the report if it is a retransmission of a report that
	 *	has already been applied.				*/

	for (elt = sdr_list_first(sdr, sessionBuf.rsSerialNbrs); elt;
			elt = sdr_list_next(sdr, elt))
	{
		oldRptSerialNbr = (unsigned int) sdr_list_data(sdr, elt);
		if (oldRptSerialNbr < rptSerialNbr)
		{
			continue;
		}

		if (oldRptSerialNbr == rptSerialNbr)
		{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
			/*	The report is redundant.		*/

			MRELEASE(newClaims);
			return sdr_end_xn(sdr);	/*	Ignore.		*/
		}

		break;
	}

	/*	This is a report we have not seen before.  Remember
	 *	it for future reference.				*/

	if (elt)
	{
		oK(sdr_list_insert_before(sdr, elt, (SdrObject) rptSerialNbr));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, sessionBuf.rsSerialNbrs,
				(SdrObject) rptSerialNbr));
	}

	/*	Next apply the report to the cited checkpoint, if any.	*/

	if (ckptSerialNbr != 0)	/*	Not an asynchronous report.	*/
	{
		findCheckpoint(&sessionBuf, ckptSerialNbr, &elt, &dsObj);
		if (elt == 0)
		{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
			/*	No such checkpoint; the report is
			 *	erroneous.				*/

			MRELEASE(newClaims);
			return sdr_end_xn(sdr);	/*	Ignore.		*/
		}

		/*	Don't deactivate the checkpoint here. When multiple
		 *	Report Segments are needed for a single checkpoint,
		 *	some RSs may be lost. If we deactivate the checkpoint
		 *	on the first RS, the sender won't retransmit the
		 *	checkpoint if later RSs are lost, causing the session
		 *	to get stuck. The checkpoint will be cleaned up when
		 *	the session closes.				*/
	}

	/*	Now apply reception claims to the transmission session.	*/

	if ((sessionBuf.redPartLength < 0 || rptUpperBound > (unsigned int)sessionBuf.redPartLength) /* Bogus. */
	|| rptLowerBound >= rptUpperBound	/*	Malformed.	*/
	|| claimCount == 0)			/*	Malformed.	*/
	{
#if LTPDEBUG
putErrmsg("Discarding report.", NULL);
#endif
		MRELEASE(newClaims);
		return sdr_end_xn(sdr);	/*	Ignore RS.	*/
	}

	/*	Retrieve all previously received reception claims
	 *	for this transmission session, loading them into a
	 *	temporary linked list within which the new and old
	 *	claims will be merged.  While loading the old claims
	 *	into the list, delete them from the database; they
	 *	will be replaced by the final contents of the linked
	 *	list.							*/

	if ((claims = lyst_create_using(ltpMemIdx)) == NULL)
	{
		putErrmsg("Can't start list of reception claims.", NULL);
		MRELEASE(newClaims);
		sdr_cancel_xn(sdr);
		return -1;
	}

	for (elt = sdr_list_first(sdr, sessionBuf.claims); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		claimObj = sdr_list_data(sdr, elt);
		claim = (LtpReceptionClaim *) MTAKE(sizeof(LtpReceptionClaim));
		if (claim == NULL || (lyst_insert_last(claims, claim)) == NULL)
		{
			putErrmsg("Can't insert reception claim.", NULL);
			MRELEASE(newClaims);
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_read(sdr, (char *) claim, claimObj,
				sizeof(LtpReceptionClaim));
		sdr_free(sdr, claimObj);
		sdr_list_delete(sdr, elt, NULL, NULL);
	}

	/*	Now merge the new claims in the array with the old
	 *	claims in the list.  The final contents of the linked
	 *	list will be the consolidated claims resulting from
	 *	merging the old claims from the database with the new
	 *	claims in the report.					*/

	for (i = 0, newClaim = newClaims; i < claimCount; i++, newClaim++)
	{
		newClaimEnd = newClaim->offset + newClaim->length;
		for (elt2 = lyst_first(claims); elt2; elt2 = nextElt2)
		{
			nextElt2 = lyst_next(elt2);
			claim = (LtpReceptionClaim *) lyst_data(elt2);
			claimEnd = claim->offset + claim->length;
			if (claimEnd < newClaim->offset)
			{
				/*	This old claim is unaffected
				 *	by the new claims; it remains
				 *	in the list.			*/

				continue;
			}

			if (claim->offset > newClaimEnd)
			{
				/*	Must insert new claim into
				 *	list before this old one.	*/

				break;	/*	Out of old-claims loop.	*/
			}

			/*	New claim overlaps with this existing
			 *	claim, so consolidate the existing
			 *	claim with the new claim (in place,
			 *	in the array), delete the old claim
			 *	from the list, and look at the next
			 *	existing claim.				*/

			if (claim->offset < newClaim->offset)
			{
				/*	Start of consolidated claim
				 *	is earlier than start of new
				 *	claim.				*/

				newClaim->length +=
					(newClaim->offset - claim->offset);
				newClaim->offset = claim->offset;
			}

			if (claimEnd > newClaimEnd)
			{
				/*	End of consolidated claim
				 *	is later than end of new
				 *	claim.				*/

				newClaim->length += (claimEnd - newClaimEnd);
			}

			MRELEASE(claim);
			lyst_delete(elt2);
		}

		/*	newClaim has now been consolidated with all
		 *	prior claims with which it overlapped, and all
		 *	of those prior claims have been removed from
		 *	the list.  Now we can insert the consolidated
		 *	new claim into the list.			*/

		claim = (LtpReceptionClaim *) MTAKE(sizeof(LtpReceptionClaim));
		if (claim == NULL)
		{
			putErrmsg("Can't create reception claim.", NULL);
			MRELEASE(newClaims);
			sdr_cancel_xn(sdr);
			return -1;
		}

		claim->offset = newClaim->offset;
		claim->length = newClaim->length;
		if (elt2)
		{
			if (lyst_insert_before(elt2, claim) == NULL)
			{
				putErrmsg("Can't create reception claim.",
						NULL);
				MRELEASE(newClaims);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}
		else
		{
			if (lyst_insert_last(claims, claim) == NULL)
			{
				putErrmsg("Can't create reception claim.",
						NULL);
				MRELEASE(newClaims);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}
	}

	/*	The claims list now contains the consolidated claims
	 *	for all data reception for this export session.  The
	 *	array of new claims is no longer needed.		*/

	MRELEASE(newClaims);
	elt2 = lyst_first(claims);
	claim = (LtpReceptionClaim *) lyst_data(elt2);

	/*	If reception of all data in the block is claimed (i.e,
	 *	there is now only one claim in the list and that claim
	 *	-- the first -- encompasses the entire red part of the
	 *	block), and either the block is all Red data or else
	 *	the last Green segment is known to have been sent,
	 *	end the export session.					*/

	if (claim->offset == 0 && sessionBuf.redPartLength >= 0 && claim->length == (unsigned int)sessionBuf.redPartLength)
	{
		ltpSpanTally(vspan, POS_RPT_RECV, 0);
		MRELEASE(claim);	/*	(Sole claim in list.)	*/
		lyst_destroy(claims);
		if (sessionBuf.redPartLength == sessionBuf.totalLength
		|| sessionBuf.stateFlags & LTP_EOB_SENT)
		{
			stopExportSession(&sessionBuf);
			closeExportSession(sessionObj);
			ltpSpanTally(vspan, EXPORT_COMPLETE, 0);
#if CLOSED_EXPORTS_ENABLED
			noteClosedExport(ltpdb, vspan, spanObj, sessionNbr);
#endif
		}
		else
		{
			sessionBuf.stateFlags |= LTP_FINAL_ACK;
			sdr_write(sdr, sessionObj, (char *) &sessionBuf,
					sizeof(LtpExportSession));
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle report segment.", NULL);
			return -1;
		}

		if (ltpvdb->watching & WATCH_h)
		{
#if defined (EWCHAR)
			char ewchar[256];
			/* spec is for 64 bit, non-Window */
			isprintf(ewchar,sizeof(ewchar),"(%u)h",sessionNbr);
			iwatch_str(ewchar);
#else
			iwatch('h');
#endif

		}

		return 1;	/*	Complete red part exported.	*/
	}

	/*	Not all red data in the block has yet been received.	*/

	ltpSpanTally(vspan, NEG_RPT_RECV, 0);
	ckptSerialNbr = sessionBuf.prevCkptSerialNbr + 1;
	if (ckptSerialNbr == 0	/*	Rollover.			*/
	|| (sessionBuf.maxCheckpoints >= 0
		&& sdr_list_length(sdr, sessionBuf.checkpoints)
			>= (size_t)sessionBuf.maxCheckpoints))
	{
		/*	Limit reached, can't retransmit any more.
		 *	Just destroy the claims list and cancel. 	*/

		while (1)
		{
			MRELEASE(claim);
			elt2 = lyst_next(elt2);
			if (elt2 == NULL)
			{
				break;
			}

			claim = (LtpReceptionClaim *) lyst_data(elt2);
		}

		lyst_destroy(claims);
#if LTPDEBUG
putErrmsg("Cancel by sender.", itoa(sessionNbr));
#endif
		if (cancelSessionBySender(&sessionBuf, sessionObj,
				LtpRetransmitLimitExceeded))
		{
			putErrmsg("Can't cancel export session.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle report segment.", NULL);
			return -1;
		}

		return 1;
	}

	/*	Must retransmit data to fill gaps ("extents") in
	 *	reception.  Start compiling list of retransmission
	 *	ExportExtents.						*/

#if LTPDEBUG
putErrmsg("Incomplete reception.  Claims:", utoa(claimCount));
#endif
	if ((extents = lyst_create_using(ltpMemIdx)) == NULL)
	{
		putErrmsg("Can't start list of retransmission extents.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	startOfGap = rptLowerBound;

	/*	Loop through the claims, writing them to the database
	 *	and adding retransmission extents for the gaps between
	 *	the claims.						*/

	while (1)
	{
#if LTPDEBUG
char	claimbuf[256];
isprintf(claimbuf, sizeof claimbuf, "-   offset %u length %u (%u-%u)", claim->offset,
claim->length, claim->offset, claim->offset + claim->length);
putErrmsg(claimbuf, itoa(sessionBuf.sessionNbr));
#endif
		claimEnd = claim->offset + claim->length;
		if (insertClaim(&sessionBuf, claim) < 0)
		{
			putErrmsg("Can't create new reception claim.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		endOfGap = MIN(claim->offset, rptUpperBound);
		if (endOfGap > startOfGap)	/*	Here's a gap.	*/
		{
			/*	This is a gap that may be repaired
			 *	in response to this report.		*/

			if (addTransmissionExtent(extents, startOfGap,
					endOfGap) < 0)
			{
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		if (claimEnd > startOfGap)
		{
			/*	The start of the next reparable gap
			 *	cannot be before the end of this claim.	*/

			startOfGap = claimEnd;
		}

		MRELEASE(claim);
		elt2 = lyst_next(elt2);
		if (elt2 == NULL)
		{
			break;
		}

		claim = (LtpReceptionClaim *) lyst_data(elt2);
	}

	lyst_destroy(claims);

	/*	There may be one final gap, between the end of the
	 *	last claim and the end of the scope of the report.	*/

	endOfGap = rptUpperBound;
	if (endOfGap > startOfGap)		/*	Final gap.	*/
	{
		if (addTransmissionExtent(extents, startOfGap, endOfGap) < 0)
		{
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	/*	List of retransmission extents is now complete;
	 *	retransmit data as needed.  				*/

	if (issueSegments(sdr, &spanBuf, vspan, &sessionBuf, sessionObj,
			extents, rptSerialNbr, ckptSerialNbr) < 0)
	{
		putErrmsg("Can't retransmit data.", itoa(vspan->meterPid));
		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	Finally, destroy retransmission extents list and
	 *	return.							*/

	for (elt2 = lyst_first(extents); elt2; elt2 = lyst_next(elt2))
	{
		MRELEASE((char *) lyst_data(elt2));
	}

	lyst_destroy(extents);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle report segment.", NULL);
		return -1;
	}

	if (ltpvdb->watching & WATCH_nak)
	{
#if defined (EWCHAR)
		char ewchar[256];
		/* spec is for 64 bit, non-Window */
		isprintf(ewchar,sizeof(ewchar),"(%u)@",sessionNbr);
		iwatch_str(ewchar);
#else
		iwatch('@');
#endif
	}

	return 1;	/*	Report handled successfully.		*/
}

static int	handleRA(uvast sourceEngineId, LtpDB *ltpdb,
			unsigned int sessionNbr, LtpRecvSeg *segment,
			char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr			sdr = getIonsdr();
	LtpPdu			*pdu = &(segment->pdu);
	char			*endOfHeader;
	unsigned int		rptSerialNbr;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	SdrObject		sessionObj;
	LtpImportSession	session;
	SdrObject		elt;
	SdrObject		rsObj;
	LtpXmitSeg		rsBuf;
#if LTPDEBUG
putErrmsg("Handling report ack.", utoa(sessionNbr));
#endif

	/*	First finish parsing the segment.			*/

	endOfHeader = *cursor;
	extractSmallSdnv(&rptSerialNbr, cursor, bytesRemaining);

	/*	Report is being acknowledged.				*/

	CHKERR(sdr_begin_xn(sdr));
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Random segment.			*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(sdr);
		return 0;
	}

	/*	At this point, the remaining bytes should all be
	 *	trailer extensions.  We now parse them.			*/

	pdu->contentLength = (*cursor - endOfHeader);
	pdu->trailerLength = *bytesRemaining;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle report ack.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	getImportSession(vspan, sessionNbr, NULL, &sessionObj);
	if (sessionObj == 0)	/*	Nothing to apply ack to.	*/
	{
		return sdr_end_xn(sdr);
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	/*	Session exists, so find the report.			*/

	sdr_stage(sdr, (char *) &session, sessionObj, sizeof(LtpImportSession));
	findReport(&session, rptSerialNbr, &elt, &rsObj);
	if (elt)	/*	Found the report that is acknowledged.	*/
	{
		sdr_stage(sdr, (char *) &rsBuf, rsObj, sizeof(LtpXmitSeg));
#if LTPDEBUG
char	buf[256];
isprintf(buf, sizeof buf, "Acknowledged report is %u, lowerBound %d, upperBound %d, \
last report serial number %u.", rsBuf.pdu.rptSerialNbr, rsBuf.pdu.lowerBound,
rsBuf.pdu.upperBound, session.finalRptSerialNbr);
putErrmsg(buf, itoa(sessionNbr));
#endif
		/*	This may be an opportunity to close the import
		 *	session.  If this RA is an acknowledgment of
		 *	the final report segment signifying that the
		 *	entire red part has been received, then we no
		 *	longer need to keep the import session open.	*/

		if (rsBuf.pdu.rptSerialNbr == session.finalRptSerialNbr)
		{
			if (session.delivered)
			{
				stopImportSession(&session);
				removeImportSession(sessionObj);
				closeImportSession(sessionObj);
				sessionObj = 0;
				ltpSpanTally(vspan, IMPORT_COMPLETE, 0);
			}
			else
			{
				session.finalRptAcked = 1;
				sdr_write(sdr, sessionObj, (char *) &session,
						sizeof(LtpImportSession));
			}
		}

		if (sessionObj) /*	Can't close import session yet.	*/
		{
			/*	We just deactivate the report segment.
			 *	It has been received, so there will
			 *	never be any need to retransmit it, but
			 *	we retain it in the database for lookup
			 *	purposes when checkpoints arrive.	*/

			rsBuf.pdu.timer.segArrivalTime = 0;
			sdr_write(sdr, rsObj, (char *) &rsBuf,
					sizeof(LtpXmitSeg));
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't handle report ack.", NULL);
			return -1;
		}

		return 1;
	}

	/*	Anomaly: no match on report serial number, so ignore
	 *	the RA.							*/

	return sdr_end_xn(sdr);
}

static int	handleCS(uvast sourceEngineId, LtpDB *ltpdb,
			unsigned int sessionNbr, LtpRecvSeg *segment,
			char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr		sdr = getIonsdr();
	LtpVdb		*ltpvdb = _ltpvdb(NULL);
	LtpPdu		*pdu = &(segment->pdu);
	char		*endOfHeader;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	SdrObject	spanObj;
	OBJ_POINTER(LtpSpan, span);
	SdrObject	sessionObj;
	OBJ_POINTER(LtpImportSession, session);

	endOfHeader = *cursor;

#if LTPDEBUG
putErrmsg("Handling cancel by sender.", utoa(sessionNbr));
#endif

	/*	Source of block is requesting cancellation of session.	*/

	CHKERR(sdr_begin_xn(sdr));
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		/*	Cancellation is from an unknown source engine,
		 *	so we can't even acknowledge.  Ignore it.	*/

		sdr_exit_xn(sdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(sdr);
		return 0;
	}

	/*	At this point, the remaining bytes should all be
	 *	trailer extensions.  We now parse them.			*/

	pdu->contentLength = (*cursor - endOfHeader);
	pdu->trailerLength = *bytesRemaining;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle cancel by sender.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);

	/*	Acknowledge the cancellation request.			*/

	if (constructSourceCancelAckSegment(spanObj, &(span->engineIdSdnv),
			sessionNbr) < 0)
	{
		putErrmsg("Can't send CAS segment.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	ltpSpanTally(vspan, EXPORT_CANCEL_RECV, 0);
	getImportSession(vspan, sessionNbr, NULL, &sessionObj);
	if (sessionObj)	/*	Can cancel session as requested.	*/
	{
		GET_OBJ_POINTER(sdr, LtpImportSession, session, sessionObj);
		if (enqueueNotice(ltpvdb->clients + session->clientSvcId,
				sourceEngineId, sessionNbr, 0, 0,
				LtpImportSessionCanceled, **cursor, 0, 0) < 0)
		{
			putErrmsg("Can't post ImportSessionCanceled notice.",
					NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (ltpvdb->watching & WATCH_handleCLS)
		{
			iwatch('}');
		}

		clearImportSession(session);
		stopImportSession(session);
		removeImportSession(sessionObj);
		closeImportSession(sessionObj);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle cancel by source.", NULL);
		return -1;
	}

	return 1;
}

static int	handleCAS(LtpDB *ltpdb, unsigned int sessionNbr,
			LtpRecvSeg *segment, char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr			sdr = getIonsdr();
	LtpPdu			*pdu = &(segment->pdu);
	char			*endOfHeader;
	SdrObject		sessionObj;
	SdrObject		sessionElt;
	LtpExportSession	sessionBuf;
	LtpSpan			spanBuf;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;

	endOfHeader = *cursor;
#if LTPDEBUG
putErrmsg("Handling ack of cancel by sender.", utoa(sessionNbr));
#endif

	/*	Destination of block is acknowledging source's
	 *	cancellation of session.				*/

	CHKERR(sdr_begin_xn(sdr));
	getCanceledExport(sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Nothing to apply ack to.	*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	sdr_read(sdr, (char *) &sessionBuf, sessionObj,
			sizeof(LtpExportSession));
	sdr_read(sdr, (char *) &spanBuf, sessionBuf.span, sizeof(LtpSpan));
	findSpan(spanBuf.engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		putErrmsg("Missing span.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(sdr);
		return 0;
	}

	/*	At this point, the remaining bytes should all be
	 *	trailer extensions.  We now parse them.			*/

	pdu->contentLength = (*cursor - endOfHeader);
	pdu->trailerLength = *bytesRemaining;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle sender cancel ack.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	/*	Cancel retransmission of the CS segment.		*/

	cancelEvent(LtpResendXmitCancel, 0, sessionNbr, 0);

	/*	No need to change state of session's timer
	 *	because the whole session is about to vanish.		*/

	sdr_list_delete(sdr, sessionElt, NULL, NULL);
	sdr_free(sdr, sessionObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle ack of cancel by source.", NULL);
		return -1;
	}

	return 1;
}

static int	handleCR(uvast sourceEngineId, LtpDB *ltpdb, unsigned int sessionNbr,
			LtpRecvSeg *segment, char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr			sdr = getIonsdr();
	LtpVdb			*ltpvdb = _ltpvdb(NULL);
	SdrObject		dbobj = getLtpDbObject();
	LtpPdu			*pdu = &(segment->pdu);
	char			*endOfHeader;
	LtpDB			db;
	SdrObject		sessionObj;
	LtpExportSession	sessionBuf;
	SdrObject		spanObj;
	LtpSpan			spanBuf;
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	SdrObject		elt;
	SdrObject		sdu;	/*	A ZcoRef object.	*/

	endOfHeader = *cursor;

#if LTPDEBUG
putErrmsg("Handling cancel by receiver.", utoa(sessionNbr));
#endif

	/*	Destination of block is requesting cancellation of
	 *	session.						*/

	CHKERR(sdr_begin_xn(sdr));
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		/*	Cancellation is from an unknown source engine,
		 *	so we can't even acknowledge.  Ignore it.	*/

		sdr_exit_xn(sdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(sdr);
		return 0;
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_read(sdr, (char *) &spanBuf, spanObj, sizeof(LtpSpan));

	/*	At this point, the remaining bytes should all be
	 *	trailer extensions.  We now parse them.			*/

	pdu->contentLength = (*cursor - endOfHeader);
	pdu->trailerLength = *bytesRemaining;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle cancel by receiver.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	/*	Acknowledge the cancellation request.			*/

	sdr_stage(sdr, (char *) &db, dbobj, sizeof(LtpDB));
	if (constructDestCancelAckSegment(spanObj,
			&db.ownEngineIdSdnv, sessionNbr) < 0)
	{
		putErrmsg("Can't send CAR segment.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	ltpSpanTally(vspan, IMPORT_CANCEL_RECV, 0);

	/*	Now look up the session to see if we can cancel it.	*/

	getExportSession(sessionNbr, &sessionObj);
	if (sessionObj != 0)	/*	Known session.			*/
	{
		sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
				sizeof(LtpExportSession));
		if (sessionBuf.totalLength <= 0)/*	Not a live session. */
		{
			sessionObj = 0;		/*	Can't cancel.	*/
		}
	}

	if (sessionObj)	/*	Can cancel session as requested.	*/
	{
		sessionBuf.reasonCode = **cursor;
		if (ltpvdb->watching & WATCH_handleCR)
		{
			iwatch(']');
		}

		stopExportSession(&sessionBuf);
		for (elt = sdr_list_first(sdr, sessionBuf.svcDataObjects);
				elt; elt = sdr_list_next(sdr, elt))
		{
			sdu = sdr_list_data(sdr, elt);
			if (enqueueNotice(ltpvdb->clients
					+ sessionBuf.clientSvcId,
					db.ownEngineId, sessionBuf.sessionNbr,
					0, 0, LtpExportSessionCanceled,
					**cursor, 0, sdu) < 0)
			{
				putErrmsg("Can't post ExportSessionCanceled \
notice.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		sdr_write(sdr, dbobj, (char *) &db, sizeof(LtpDB));

		/*	The service data units in the svcDataObjects
		 *	list must be protected -- the client will be
		 *	receiving them in notices and destroying them
		 *	-- so we must destroy the svcDataObject list
		 *	itself here and prevent closeExportSession
		 *	from accessing it.				*/

		sdr_list_destroy(sdr, sessionBuf.svcDataObjects, NULL, NULL);
		sessionBuf.svcDataObjects = 0;
		sdr_write(sdr, sessionObj, (char *) &sessionBuf,
				sizeof(LtpExportSession));

		/*	Now finish closing the export session.		*/

		closeExportSession(sessionObj);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle cancel by destination.", NULL);
		return -1;
	}

	return 1;
}

static int	handleCAR(uvast sourceEngineId, LtpDB *ltpdb,
			unsigned int sessionNbr, LtpRecvSeg *segment,
			char **cursor, int *bytesRemaining,
			Lyst headerExtensions, Lyst trailerExtensions)
{
	Sdr		sdr = getIonsdr();
	LtpPdu		*pdu = &(segment->pdu);
	char		*endOfHeader;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	SdrObject	sessionObj;
	SdrObject	sessionElt;

	endOfHeader = *cursor;

#if LTPDEBUG
putErrmsg("Handling ack of cancel by receiver.", utoa(sessionNbr));
#endif

	/*	Source of block is acknowledging destination's
	 *	cancellation of session.				*/

	CHKERR(sdr_begin_xn(sdr));
	findSpan(sourceEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Stray segment.			*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	if (vspan->receptionRate == 0 && ltpdb->enforceSchedule == 1)
	{
#if LTPDEBUG
putErrmsg("Discarding stray segment.", itoa(sessionNbr));
#endif
		/*	Segment is from an engine that is not supposed
		 *	to be sending at this time, so we treat it as
		 *	a misdirected transmission.			*/

		sdr_exit_xn(sdr);
		return 0;
	}

	getCanceledImport(vspan, sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Nothing to apply ack to.	*/
	{
		sdr_exit_xn(sdr);
		return 0;
	}

	/*	At this point, the remaining bytes should all be
	 *	trailer extensions.  We now parse them.			*/

	pdu->contentLength = (*cursor - endOfHeader);
	pdu->trailerLength = *bytesRemaining;
	switch (parseTrailerExtensions(endOfHeader, pdu, trailerExtensions))
	{
	case -1:	/*	No available memory.			*/
		putErrmsg("Can't handle receiver cancel ack.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Parsing error.				*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	switch (invokeInboundBeforeContentProcessingCallbacks(segment,
			headerExtensions, trailerExtensions,
			endOfHeader - pdu->headerLength, vspan))
	{
	case -1:	/*	System failure.				*/
		putErrmsg("LTP extension callback failed.", NULL);
		sdr_cancel_xn(sdr);
		return -1;

	case 0:		/*	Callback rejects the segment.		*/
		ltpSpanTally(vspan, IN_SEG_MALFORMED, pdu->length);
		return sdr_end_xn(sdr);
	}

	/*	Cancel retransmission of the CR segment.		*/

	cancelEvent(LtpResendRecvCancel, vspan->engineId, sessionNbr, 0);

	/*	No need to change state of session's timer because
	 *	the whole session is about to vanish.			*/

	sdr_list_delete(sdr, sessionElt, NULL, NULL);
	closeImportSession(sessionObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't handle ack of cancel by destination.", NULL);
		return -1;
	}

	return 1;
}

int	ltpHandleInboundSegment(char *buf, int length)
{
	Sdr		sdr;
	LtpRecvSeg	segment;
	char		versionNbr;
	LtpPdu		*pdu = &segment.pdu;
	char		*cursor = buf;
	int		bytesRemaining = length;
	uvast		sourceEngineId;
	unsigned int	sessionNbr;
	unsigned int	extensionCounts;
	Lyst		headerExtensions;
	Lyst		trailerExtensions;
	unsigned int	extensionOffset;
	int		i;
	OBJ_POINTER(LtpDB, ltpdb);
	int		result = 0;

	CHKERR(buf);
	CHKERR(length > 0);
	memset((char *) &segment, 0, sizeof(LtpRecvSeg));

	/*	Get version number and segment type.			*/

	versionNbr = ((*cursor) >> 4) & 0x0f;
	if (versionNbr != 0)
	{
		return 0;		/*	Ignore the segment.	*/
	}

	pdu->segTypeCode = (*cursor) & 0x0f;
	cursor++;
	bytesRemaining--;

	/*	Get session ID.						*/

	extractSdnv(&sourceEngineId, &cursor, &bytesRemaining);
	extractSmallSdnv(&sessionNbr, &cursor, &bytesRemaining);
	if (sessionNbr == 0)
	{
		return 0;		/*	Ignore the segment.	*/
	}

	/*	Get counts of header and trailer extensions.		*/

	extensionCounts = *cursor;
	pdu->headerExtensionsCount = (extensionCounts >> 4) & 0x0f;
	pdu->trailerExtensionsCount = extensionCounts & 0x0f;
	cursor++;
	bytesRemaining--;
	if ((headerExtensions = lyst_create_using(getIonMemoryMgr())) == NULL)
	{
		return -1;
	}

	if ((trailerExtensions = lyst_create_using(getIonMemoryMgr())) == NULL)
	{
		lyst_destroy(headerExtensions);
		return -1;
	}

	extensionOffset = cursor - buf;
	for (i = 0; i < pdu->headerExtensionsCount; i++)
	{
		switch (ltpei_parse_extension(&cursor, &bytesRemaining,
				headerExtensions, &extensionOffset))
		{
		case -1:
			ltpei_discard_extensions(headerExtensions);
			ltpei_discard_extensions(trailerExtensions);
			return -1;	/*	System failure.		*/

		case 0:
			ltpei_discard_extensions(headerExtensions);
			ltpei_discard_extensions(trailerExtensions);
			return 0;	/*	Ignore segment.		*/
		}
	}

	pdu->headerLength = cursor - buf;

	/*	Handle segment according to its segment type code.	*/

	CHKERR(sdr_begin_xn((sdr = getIonsdr())));
	GET_OBJ_POINTER(sdr, LtpDB, ltpdb, _ltpdbObject(NULL));
	sdr_exit_xn(sdr);
	if ((pdu->segTypeCode & LTP_CTRL_FLAG) == 0)	/*	Data.	*/
	{
		if ((_ltpvdb(NULL))->watching & WATCH_s)
		{
#if defined (EWCHAR)
				char ewchar[256];
				/* data segment*/
				isprintf(ewchar,sizeof(ewchar),"(d%u)s",sessionNbr);
				iwatch_str(ewchar);
#else
				iwatch('s');
#endif
		}
		result = handleDataSegment(sourceEngineId, ltpdb, sessionNbr,
				&segment, &cursor, &bytesRemaining,
				headerExtensions, trailerExtensions);
	}
	else
	{
		/*	Segment is a control segment.			*/

		switch (pdu->segTypeCode)
		{
		case LtpRS:
			if ((_ltpvdb(NULL))->watching & WATCH_s)
			{
#if defined (EWCHAR)
					char ewchar[256];
					/* report segment*/
					isprintf(ewchar,sizeof(ewchar),"(rs%u)s",sessionNbr);
					iwatch_str(ewchar);
#else
					iwatch('s');
#endif
			}
			result = handleRS(ltpdb, sessionNbr,
					&segment, &cursor, &bytesRemaining,
					headerExtensions, trailerExtensions);
			break;

		case LtpRAS:
			if ((_ltpvdb(NULL))->watching & WATCH_s)
			{
#if defined (EWCHAR)
					char ewchar[256];
					/* report ack segment*/
					isprintf(ewchar,sizeof(ewchar),"(ras%u)s",sessionNbr);
					iwatch_str(ewchar);
#else
					iwatch('s');
#endif
			}
			result = handleRA(sourceEngineId, ltpdb, sessionNbr,
					&segment, &cursor, &bytesRemaining,
					headerExtensions, trailerExtensions);
			break;

		case LtpCS:
			if ((_ltpvdb(NULL))->watching & WATCH_s)
			{
#if defined (EWCHAR)
					char ewchar[256];
					/* cancel by source segment*/
					isprintf(ewchar,sizeof(ewchar),"(cs%u)s",sessionNbr);
					iwatch_str(ewchar);
#else
					iwatch('s');
#endif
			}
			result = handleCS(sourceEngineId, ltpdb, sessionNbr,
					&segment, &cursor, &bytesRemaining,
					headerExtensions, trailerExtensions);
			break;

		case LtpCAS:
			if ((_ltpvdb(NULL))->watching & WATCH_s)
			{
#if defined (EWCHAR)
					char ewchar[256];
					/* cancel by source ack segment*/
					isprintf(ewchar,sizeof(ewchar),"(cas%u)s",sessionNbr);
					iwatch_str(ewchar);
#else
					iwatch('s');
#endif
			}
			result = handleCAS(ltpdb, sessionNbr,
					&segment, &cursor, &bytesRemaining,
					headerExtensions, trailerExtensions);
			break;

		case LtpCR:
			if ((_ltpvdb(NULL))->watching & WATCH_s)
			{
#if defined (EWCHAR)
					char ewchar[256];
					/* cancel by receiver segment*/
					isprintf(ewchar,sizeof(ewchar),"(cr%u)s",sessionNbr);
					iwatch_str(ewchar);
#else
					iwatch('s');
#endif
			}
			result = handleCR(sourceEngineId, ltpdb, sessionNbr,
					&segment, &cursor, &bytesRemaining,
					headerExtensions, trailerExtensions);
			break;

		case LtpCAR:
			if ((_ltpvdb(NULL))->watching & WATCH_s)
			{
#if defined (EWCHAR)
					char ewchar[256];
					/* cancel by receiver ack segment*/
					isprintf(ewchar,sizeof(ewchar),"(car%u)s",sessionNbr);
					iwatch_str(ewchar);
#else
					iwatch('s');
#endif
			}
			result = handleCAR(sourceEngineId, ltpdb, sessionNbr,
					&segment, &cursor, &bytesRemaining,
					headerExtensions, trailerExtensions);
			break;

		default:
			break;
		}
	}

	ltpei_discard_extensions(headerExtensions);
	ltpei_discard_extensions(trailerExtensions);
	return result;		/*	Ignore the segment.		*/
}

/*	*	*	Functions that respond to events	*	*/

void	ltpStartXmit(LtpVspan *vspan)
{
	Sdr	sdr = getIonsdr();
	SdrObject spanObj;
	LtpSpan	span;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_read(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	sm_SemGive(vspan->bufOpenRedSemaphore);
	sm_SemGive(vspan->bufOpenGreenSemaphore);
	if (sdr_list_length(sdr, span.segments) > 0)
	{
		sm_SemGive(vspan->segSemaphore);
	}
}

void	ltpStopXmit(LtpVspan *vspan)
{
	Sdr			sdr = getIonsdr();
	SdrObject		spanObj;
	LtpSpan			span;
	SdrObject		elt;
	SdrObject		nextElt;
	SdrObject		sessionObj;
	LtpExportSession	session;

	CHKVOID(ionLocked());
	CHKVOID(vspan);
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_read(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (span.purge)
	{
		/*	At end of transmission on this span we must
		 *	cancel all export sessions that are currently
		 *	in progress.  Notionally this forces re-
		 *	forwarding of the DTN bundles in each session's
		 *	block, to avoid having to wait for the restart
		 *	of transmission on this span before those
		 *	bundles can be successfully transmitted.	*/

		for (elt = sdr_list_first(sdr, span.exportSessions); elt;
				elt = nextElt)
		{
			nextElt = sdr_list_next(sdr, elt);
			sessionObj = sdr_list_data(sdr, elt);
			sdr_stage(sdr, (char *) &session, sessionObj,
					sizeof(LtpExportSession));
			if (session.svcDataObjects == 0
			|| sdr_list_length(sdr, session.svcDataObjects) == 0)
			{
				/*	Session is not yet populated
				 *	with any service data objects.	*/

				continue;
			}

			oK(cancelSessionBySender(&session, sessionObj,
					LtpCancelByEngine));
		}
	}
}

static void	suspendTimer(time_t suspendTime, LtpTimer *timer,
			SdrAddress timerAddr, unsigned int qTime,
			unsigned int remoteXmitRate, LtpEventType eventType,
			uvast eventRefNbr1, unsigned int eventRefNbr2,
			unsigned int eventRefNbr3)
{
	time_t	latestAckXmitStartTime;

	/* Parameter intentionally unused. */
	(void)remoteXmitRate;

	CHKVOID(ionLocked());
	latestAckXmitStartTime = timer->segArrivalTime + qTime;
	if (latestAckXmitStartTime < suspendTime)
	{
		/*	Transmission of ack should have begun before
		 *	link was stopped.  Timer must not be suspended.	*/

		return;
	}

	/*	Must suspend timer while remote engine is unable to
	 *	transmit.						*/

	cancelEvent(eventType, eventRefNbr1, eventRefNbr2, eventRefNbr3);

	/*	Change state of timer object and save it.		*/

	timer->state = LtpTimerSuspended;
	sdr_write(getIonsdr(), timerAddr, (char *) timer, sizeof(LtpTimer));
}

int	ltpSuspendTimers(LtpVspan *vspan, PsmAddress vspanElt,
		time_t suspendTime, unsigned int priorXmitRate)
{
	Sdr			sdr = getIonsdr();
	SdrObject		spanObj;
	OBJ_POINTER(LtpSpan, span);
	unsigned int		qTime;
	SdrObject		elt;
	SdrObject		sessionObj;
	LtpImportSession	rsessionBuf;
	LtpTimer		*timer;
	SdrObject		elt2;
	SdrObject		ckptObj;
	OBJ_POINTER(LtpCkpt, cp);
	SdrObject		segmentObj;
	LtpXmitSeg		rsBuf;
	LtpExportSession	xsessionBuf;
	LtpXmitSeg		dsBuf;

	/* Parameter intentionally unused. */
	(void)vspanElt;

	CHKERR(ionLocked());
	CHKERR(vspan);
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);
	qTime = span->remoteQtime;

	/*	Suspend relevant timers for import sessions.		*/

	for (elt = sdr_list_first(sdr, span->deadImports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &rsessionBuf, sessionObj,
				sizeof(LtpImportSession));

		/*	Suspend receiver-cancel retransmit timer.	*/

		timer = &rsessionBuf.timer;
		suspendTimer(suspendTime, timer,
			sessionObj + FLD_OFFSET(timer, &rsessionBuf),
			qTime, priorXmitRate, LtpResendRecvCancel,
			span->engineId, rsessionBuf.sessionNbr, 0);
	}

	for (elt = sdr_list_first(sdr, span->importSessions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rsessionBuf, sessionObj,
				sizeof(LtpImportSession));

		/*	Suspend report retransmission timers.		*/

		if (rsessionBuf.rsSegments == 0)
		{
			continue;
		}

		for (elt2 = sdr_list_first(sdr, rsessionBuf.rsSegments);
				elt2; elt2 = sdr_list_next(sdr, elt2))
		{
			segmentObj = sdr_list_data(sdr, elt2);
			sdr_stage(sdr, (char *) &rsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (rsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			timer = &rsBuf.pdu.timer;
			suspendTimer(suspendTime, timer,
				segmentObj + FLD_OFFSET(timer, &rsBuf),
				qTime, priorXmitRate, LtpResendReport,
				span->engineId, rsessionBuf.sessionNbr,
				rsBuf.pdu.rptSerialNbr);
		}
	}

	/*	Suspend relevant timers for export sessions.		*/

	for (elt = sdr_list_first(sdr, (_ltpConstants())->deadExports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &xsessionBuf, sessionObj,
				sizeof(LtpExportSession));
		if (xsessionBuf.span != spanObj)
		{
			continue;	/*	Not for this span.	*/
		}

		/*	Suspend sender-cancel retransmit timer.		*/

		timer = &xsessionBuf.timer;
		suspendTimer(suspendTime, timer,
			sessionObj + FLD_OFFSET(timer, &xsessionBuf),
			qTime, priorXmitRate, LtpResendXmitCancel, 0,
			xsessionBuf.sessionNbr, 0);
	}

	for (elt = sdr_list_first(sdr, span->exportSessions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &xsessionBuf, sessionObj,
				sizeof(LtpExportSession));

		/*	Suspend chkpt retransmission timers.		*/

		if (xsessionBuf.checkpoints == 0)
		{
			continue;
		}

		for (elt2 = sdr_list_first(sdr, xsessionBuf.checkpoints);
				elt2; elt2 = sdr_list_next(sdr, elt2))
		{
			ckptObj = sdr_list_data(sdr, elt2);
			GET_OBJ_POINTER(sdr, LtpCkpt, cp, ckptObj);
			segmentObj = sdr_list_data(sdr, cp->sessionListElt);
			sdr_stage(sdr, (char *) &dsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (dsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			timer = &dsBuf.pdu.timer;
			suspendTimer(suspendTime, timer,
				segmentObj + FLD_OFFSET(timer, &dsBuf),
				qTime, priorXmitRate, LtpResendCheckpoint, 0,
				xsessionBuf.sessionNbr,
				dsBuf.pdu.ckptSerialNbr);
		}
	}

	return 0;
}

static int	resumeTimer(time_t resumeTime, LtpTimer *timer,
			SdrAddress timerAddr, unsigned int qTime,
			unsigned int remoteXmitRate, LtpEventType eventType,
			uvast refNbr1, unsigned int refNbr2,
			unsigned int refNbr3)
{
	time_t		earliestAckXmitStartTime;
	int		additionalDelay;
	LtpEvent	event;

	/* Parameter intentionally unused. */
	(void)remoteXmitRate;

	CHKERR(ionLocked());
	earliestAckXmitStartTime = timer->segArrivalTime + qTime;
	additionalDelay = resumeTime - earliestAckXmitStartTime;
	if (additionalDelay > 0)
	{
		/*	Must revise deadline.				*/

		timer->ackDeadline += additionalDelay;
	}

	/*	Change state of timer object and save it.		*/

	timer->state = LtpTimerRunning;
	sdr_write(getIonsdr(), timerAddr, (char *) timer, sizeof(LtpTimer));

	/*	Re-post timeout event.					*/

	event.type = eventType;
	event.refNbr1 = refNbr1;
	event.refNbr2 = refNbr2;
	event.refNbr3 = refNbr3;
	event.parm = 0;
	event.scheduledTime = timer->ackDeadline;
	if (insertLtpTimelineEvent(&event) == 0)
	{
		putErrmsg("Can't insert timeout event.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResumeTimers(LtpVspan *vspan, PsmAddress vspanElt, time_t resumeTime,
		unsigned int remoteXmitRate)
{
	Sdr			sdr = getIonsdr();
	SdrObject		spanObj;
	OBJ_POINTER(LtpSpan, span);
	unsigned int		qTime;
	SdrObject		elt;
	SdrObject		sessionObj;
	LtpImportSession	rsessionBuf;
	LtpTimer		*timer;
	SdrObject		elt2;
	SdrObject		ckptObj;
	OBJ_POINTER(LtpCkpt, cp);
	SdrObject		segmentObj;
	LtpXmitSeg		rsBuf;
	LtpExportSession	xsessionBuf;
	LtpXmitSeg		dsBuf;

	/* Parameter intentionally unused. */
	(void)vspanElt;

	CHKERR(ionLocked());
	CHKERR(vspan);
	spanObj = sdr_list_data(sdr, vspan->spanElt);
	GET_OBJ_POINTER(sdr, LtpSpan, span, spanObj);
	qTime = span->remoteQtime;

	/*	Resume relevant timers for import sessions.		*/

	for (elt = sdr_list_first(sdr, span->deadImports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &rsessionBuf, sessionObj,
				sizeof(LtpImportSession));
		if (rsessionBuf.timer.state != LtpTimerSuspended)
		{
			continue;		/*	Not suspended.	*/
		}

		/*	Must resume: re-insert timeout event.		*/

		timer = &rsessionBuf.timer;
		if (resumeTimer(resumeTime, timer,
			sessionObj + FLD_OFFSET(timer, &rsessionBuf),
			qTime, remoteXmitRate, LtpResendRecvCancel,
			span->engineId, rsessionBuf.sessionNbr, 0) < 0)

		{
			putErrmsg("Can't resume timers for span.",
					itoa(span->engineId));
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	for (elt = sdr_list_first(sdr, span->importSessions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &rsessionBuf, sessionObj,
				sizeof(LtpImportSession));

		/*	Resume report retransmission timers.		*/

		if (rsessionBuf.rsSegments == 0)
		{
			continue;
		}

		for (elt2 = sdr_list_first(sdr, rsessionBuf.rsSegments);
				elt2; elt2 = sdr_list_next(sdr, elt2))
		{
			segmentObj = sdr_list_data(sdr, elt2);
			sdr_stage(sdr, (char *) &rsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (rsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			if (rsBuf.pdu.timer.state != LtpTimerSuspended)
			{
				continue;	/*	Not suspended.	*/
			}

			/*	Must resume: re-insert timeout event.	*/

			timer = &rsBuf.pdu.timer;
			if (resumeTimer(resumeTime, timer,
				segmentObj + FLD_OFFSET(timer, &rsBuf),
				qTime, remoteXmitRate, LtpResendReport,
				span->engineId, rsessionBuf.sessionNbr,
				rsBuf.pdu.rptSerialNbr) < 0)

			{
				putErrmsg("Can't resume timers for span.",
						itoa(span->engineId));
				sdr_cancel_xn(sdr);
				return -1;
			}
		}
	}

	/*	Resume relevant timers for export sessions.		*/

	for (elt = sdr_list_first(sdr, (_ltpConstants())->deadExports); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_stage(sdr, (char *) &xsessionBuf, sessionObj,
				sizeof(LtpExportSession));
		if (xsessionBuf.span != spanObj)
		{
			continue;	/*	Not for this span.	*/
		}

		if (xsessionBuf.timer.state != LtpTimerSuspended)
		{
			continue;		/*	Not suspended.	*/
		}

		/*	Must resume: re-insert timeout event.		*/

		timer = &xsessionBuf.timer;
		if (resumeTimer(resumeTime, timer,
			sessionObj + FLD_OFFSET(timer, &xsessionBuf),
			qTime, remoteXmitRate, LtpResendXmitCancel, 0,
			xsessionBuf.sessionNbr, 0) < 0)

		{
			putErrmsg("Can't resume timers for span.",
					itoa(span->engineId));
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	for (elt = sdr_list_first(sdr, span->exportSessions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		sessionObj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &xsessionBuf, sessionObj,
				sizeof(LtpExportSession));

		/*	Resume chkpt retransmission timers.		*/

		if (xsessionBuf.checkpoints == 0)
		{
			continue;
		}

		for (elt2 = sdr_list_first(sdr, xsessionBuf.checkpoints);
				elt2; elt2 = sdr_list_next(sdr, elt2))
		{
			ckptObj = sdr_list_data(sdr, elt2);
			GET_OBJ_POINTER(sdr, LtpCkpt, cp, ckptObj);
			segmentObj = sdr_list_data(sdr, cp->sessionListElt);
			sdr_stage(sdr, (char *) &dsBuf, segmentObj,
					sizeof(LtpXmitSeg));
			if (dsBuf.pdu.timer.segArrivalTime == 0)
			{
				continue;	/*	Not active.	*/
			}

			if (dsBuf.pdu.timer.state != LtpTimerSuspended)
			{
				continue;	/*	Not suspended.	*/
			}

			/*	Must resume: re-insert timeout event.	*/

			timer = &dsBuf.pdu.timer;
			if (resumeTimer(resumeTime, timer,
				segmentObj + FLD_OFFSET(timer, &dsBuf),
				qTime, remoteXmitRate, LtpResendCheckpoint, 0,
				xsessionBuf.sessionNbr, dsBuf.pdu.ckptSerialNbr)
				< 0)

			{
				putErrmsg("Can't resume timers for span.",
						itoa(span->engineId));
				sdr_cancel_xn(sdr);
				return -1;
			}
		}
	}

	return 0;
}

int	ltpResendCheckpoint(unsigned int sessionNbr, unsigned int ckptSerialNbr)
{
	Sdr			sdr = getIonsdr();
	SdrObject		sessionObj;
	LtpExportSession	sessionBuf;
	SdrObject		elt;
	SdrObject		dsObj;
	LtpXmitSeg		dsBuf;
	LtpXmitSegRef		segRef;
	SdrObject		segRefObj;
	OBJ_POINTER(LtpSpan, span);
	LtpVspan		*vspan;
	PsmAddress		vspanElt;

#if LTPDEBUG
putErrmsg("Resending checkpoint.", itoa(sessionNbr));
#endif
	CHKERR(sdr_begin_xn(sdr));
	getExportSession(sessionNbr, &sessionObj);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
			sizeof(LtpExportSession));
	findCheckpoint(&sessionBuf, ckptSerialNbr, &elt, &dsObj);
	if (dsObj == 0)		/*	Checkpoint is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Checkpoint is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	sdr_stage(sdr, (char *) &dsBuf, dsObj, sizeof(LtpXmitSeg));
	if (dsBuf.pdu.timer.segArrivalTime == 0)
	{
#if LTPDEBUG
putErrmsg("Checkpoint is already acknowledged.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	GET_OBJ_POINTER(sdr, LtpSpan, span, sessionBuf.span);
	findSpan(span->engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		putErrmsg("Vspan not found.", itoa(span->engineId));
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (dsBuf.pdu.timer.expirationCount >= 0
		&& (unsigned int)dsBuf.pdu.timer.expirationCount == vspan->maxTimeouts)
	{
#if LTPDEBUG
putErrmsg("Cancel by sender.", itoa(sessionNbr));
#endif
		cancelSessionBySender(&sessionBuf, sessionObj,
				LtpRetransmitLimitExceeded);
	}
	else
	{
		if (dsBuf.queueListElt)	/*	Still in xmit queue.	*/
		{
#if LTPDEBUG
putErrmsg("Resending checkpoint that is still in queue!", itoa(sessionNbr));
#endif
			sdr_free(sdr, sdr_list_data(sdr, dsBuf.queueListElt));
			sdr_list_delete(sdr, dsBuf.queueListElt, NULL, NULL);
			dsBuf.queueListElt = 0;
		}

		dsBuf.pdu.timer.expirationCount++;
		segRef.sessionNbr = sessionNbr;
		segRef.segAddr = dsObj;
		segRefObj = sdr_malloc(sdr, sizeof(LtpXmitSegRef));
		if (segRefObj == 0)
		{
			putErrmsg("Can't create segref.", itoa(span->engineId));
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_write(sdr, segRefObj, (char *) &segRef,
				sizeof(LtpXmitSegRef));
		dsBuf.queueListElt = sdr_list_insert_last(sdr,
				span->segments, segRefObj);
		if (dsBuf.queueListElt == 0)
		{
			putErrmsg("Can't queue segref.", itoa(span->engineId));
			sdr_cancel_xn(sdr);
			return -1;
		}

		sdr_write(sdr, dsObj, (char *) &dsBuf, sizeof(LtpXmitSeg));
#if BURST_SIGNALS_ENABLED
		if (enqueueBurst(&dsBuf, span, sessionBuf.redSegments,
				CHECKPOINT_BURST) < 0)
		{
			putErrmsg("LTP segment burst failed.",
					itoa(span->engineId));
			sdr_cancel_xn(sdr);
			return -1;
		}
#endif
		signalLso(span->engineId);
		if ((_ltpvdb(NULL))->watching & WATCH_resendCP)
		{
#if defined (EWCHAR)
			char ewchar[256];
			/* spec is for 64 bit, non-Window */
			isprintf(ewchar,sizeof(ewchar),"(%u)=",sessionNbr);
			iwatch_str(ewchar);
#else
			iwatch('=');
#endif
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't resend checkpoint.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResendXmitCancel(unsigned int sessionNbr)
{
	Sdr			sdr = getIonsdr();
	SdrObject		sessionObj;
	SdrObject		sessionElt;
	LtpExportSession	sessionBuf;
	OBJ_POINTER(LtpSpan, span);
	LtpVspan		*vspan;
	PsmAddress		vspanElt;

#if LTPDEBUG
putErrmsg("Resending cancel by sender.", itoa(sessionNbr));
#endif
	CHKERR(sdr_begin_xn(sdr));
	getCanceledExport(sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		sdr_exit_xn(sdr);
		return 0;
	}

	sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
			sizeof(LtpExportSession));
	GET_OBJ_POINTER(sdr, LtpSpan, span, sessionBuf.span);
	findSpan(span->engineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		putErrmsg("Vspan not found.", itoa(span->engineId));
		sdr_exit_xn(sdr);
		return -1;
	}

	if (sessionBuf.timer.expirationCount >= 0
		&& (unsigned int)sessionBuf.timer.expirationCount == vspan->maxTimeouts)
	{
#if LTPDEBUG
putErrmsg("Retransmission limit exceeded.", itoa(sessionNbr));
#endif
		sdr_list_delete(sdr, sessionElt, NULL, NULL);
		sdr_free(sdr, sessionObj);
	}
	else	/*	Haven't given up yet.				*/
	{
		sessionBuf.timer.expirationCount++;
		sdr_write(sdr, sessionObj, (char *) &sessionBuf,
				sizeof(LtpExportSession));
		if (constructSourceCancelReqSegment(span,
			&((_ltpConstants())->ownEngineIdSdnv), sessionNbr,
			sessionObj, sessionBuf.reasonCode) < 0)
		{
			putErrmsg("Can't resend cancel by sender.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't handle cancel request resend.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResendReport(uvast engineId, unsigned int sessionNbr,
		unsigned int rptSerialNbr)
{
	Sdr			sdr = getIonsdr();
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	SdrObject		sessionObj;
	LtpImportSession	sessionBuf;
	SdrObject		elt;
	SdrObject		rsObj;
	LtpXmitSeg		rsBuf;
	LtpXmitSegRef		segRef;
	SdrObject		segRefObj;
	OBJ_POINTER(LtpSpan, span);

#if LTPDEBUG
putErrmsg("Resending report.", itoa(sessionNbr));
#endif
	CHKERR(sdr_begin_xn(sdr));
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Can't search for session.	*/
	{
		return sdr_end_xn(sdr);
	}

	getImportSession(vspan, sessionNbr, NULL, &sessionObj);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
			sizeof(LtpImportSession));
	findReport(&sessionBuf, rptSerialNbr, &elt, &rsObj);
	if (rsObj == 0)		/*	Report is gone.			*/
	{
#if LTPDEBUG
putErrmsg("Report is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	sdr_stage(sdr, (char *) &rsBuf, rsObj, sizeof(LtpXmitSeg));
	if (rsBuf.pdu.timer.segArrivalTime == 0)
	{
#if LTPDEBUG
putErrmsg("Report is already acknowledged.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	if (rsBuf.pdu.timer.expirationCount >= 0
		&& (unsigned int)rsBuf.pdu.timer.expirationCount == vspan->maxTimeouts)
	{
#if LTPDEBUG
putErrmsg("Cancel by receiver.", itoa(sessionNbr));
#endif
		cancelSessionByReceiver(&sessionBuf, sessionObj,
				LtpRetransmitLimitExceeded);
	}
	else
	{
		if (rsBuf.queueListElt)	/*	Still in xmit queue.	*/
		{
#if LTPDEBUG
putErrmsg("Resending report that is still in queue!", itoa(sessionNbr));
#endif
			sdr_free(sdr, sdr_list_data(sdr, rsBuf.queueListElt));
			sdr_list_delete(sdr, rsBuf.queueListElt, NULL, NULL);
			rsBuf.queueListElt = 0;
		}

		rsBuf.pdu.timer.expirationCount++;
		segRef.sessionNbr = 0;	/*	Indicates not data.	*/
		segRef.segAddr = rsObj;
		segRefObj = sdr_malloc(sdr, sizeof(LtpXmitSegRef));
		sdr_write(sdr, segRefObj, (char *) &segRef,
				sizeof(LtpXmitSegRef));
		GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr,
				vspan->spanElt));
		rsBuf.queueListElt = sdr_list_insert_last(sdr,
				span->segments, segRefObj);
		sdr_write(sdr, rsObj, (char *) &rsBuf, sizeof(LtpXmitSeg));
#if BURST_SIGNALS_ENABLED
		oK(enqueueBurst(&rsBuf, span, sessionBuf.rsSegments,
				REPORTSEGMENT_BURST));
#endif
		signalLso(span->engineId);
		if ((_ltpvdb(NULL))->watching & WATCH_resendRS)
		{
#if defined (EWCHAR)
			char ewchar[256];
			/* spec is for 64 bit, non-Window */
			isprintf(ewchar,sizeof(ewchar),"(%u)+",sessionNbr);
			iwatch_str(ewchar);
#else
			iwatch('+');
#endif
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't resend report.", NULL);
		return -1;
	}

	return 0;
}

int	ltpResendRecvCancel(uvast engineId, unsigned int sessionNbr)
{
	Sdr			sdr = getIonsdr();
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	SdrObject		sessionObj;
	SdrObject		sessionElt;
	LtpImportSession	sessionBuf;
	OBJ_POINTER(LtpSpan, span);
#if LTPDEBUG
putErrmsg("Resending cancel by receiver.", itoa(sessionNbr));
#endif
	CHKERR(sdr_begin_xn(sdr));
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Can't search for session.	*/
	{
		return sdr_end_xn(sdr);
	}

	getCanceledImport(vspan, sessionNbr, &sessionObj, &sessionElt);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	GET_OBJ_POINTER(sdr, LtpSpan, span, sdr_list_data(sdr,
			vspan->spanElt));
	sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
			sizeof(LtpImportSession));
	if (sessionBuf.timer.expirationCount >= 0
		&& (unsigned int)sessionBuf.timer.expirationCount == vspan->maxTimeouts)
	{
#if LTPDEBUG
putErrmsg("Retransmission limit exceeded.", itoa(sessionNbr));
#endif
		sdr_list_delete(sdr, sessionElt, NULL, NULL);
		closeImportSession(sessionObj);
	}
	else	/*	Haven't given up yet.				*/
	{
		sessionBuf.timer.expirationCount++;
		sdr_write(sdr, sessionObj, (char *) &sessionBuf,
			sizeof(LtpImportSession));
		if (constructDestCancelReqSegment(span, &(span->engineIdSdnv),
			sessionNbr, sessionObj, sessionBuf.reasonCode) < 0)
		{
			putErrmsg("Can't resend cancel by receiver.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't handle cancel request resend.", NULL);
		return -1;
	}

	return 0;
}

int	ltpHandleStaleImportSession(uvast engineId, unsigned int sessionNbr)
{
	Sdr			sdr = getIonsdr();
	LtpVspan		*vspan;
	PsmAddress		vspanElt;
	SdrObject		sessionObj;
	LtpImportSession	sessionBuf;
#if LTPDEBUG
putErrmsg("Handling stale import session.", itoa(sessionNbr));
#endif
	CHKERR(sdr_begin_xn(sdr));
	findSpan(engineId, &vspan, &vspanElt);
	if (vspanElt == 0)	/*	Can't search for session.	*/
	{
		return sdr_end_xn(sdr);
	}

	getImportSession(vspan, sessionNbr, NULL, &sessionObj);
	if (sessionObj == 0)	/*	Session is gone.		*/
	{
#if LTPDEBUG
putErrmsg("Session is gone.", itoa(sessionNbr));
#endif
		return sdr_end_xn(sdr);
	}

	sdr_stage(sdr, (char *) &sessionBuf, sessionObj,
			sizeof(LtpImportSession));

	/*	Session is still active - cancel it due to inactivity.	*/

	writeMemoNote("[i] Canceling stale import session due to inactivity",
			itoa(sessionNbr));
	sessionBuf.inactivityEvent = 0;	/*	Event is being handled.	*/
	sdr_write(sdr, sessionObj, (char *) &sessionBuf,
			sizeof(LtpImportSession));
	if (cancelSessionByReceiver(&sessionBuf, sessionObj,
			LtpCancelByEngine) < 0)
	{
		putErrmsg("Can't cancel stale import session.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't handle stale import session.", NULL);
		return -1;
	}

	return 0;
}
