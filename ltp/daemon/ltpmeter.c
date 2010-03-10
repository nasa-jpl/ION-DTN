/*

	ltpmeter.c:	LTP flow control and block segmentation daemon.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
	
									*/
#include "ltpP.h"

static int	startExportSession(Sdr sdr, Object spanObj, LtpVspan *vspan)
{
	LtpVdb		*vdb;
	Object		dbobj;
	LtpSpan		span;
	LtpDB		ltpdb;
	unsigned long	sessionNbr;
	Object		sessionObj;
	Object		elt;
	ExportSession	session;

	sdr_begin_xn(sdr);
	vdb = getLtpVdb();
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	/*	Wait until window opens enabling start of next session.	*/

	while (sdr_list_length(sdr, span.exportSessions)
			>= span.maxExportSessions)
	{
		sdr_exit_xn(sdr);
		if (sm_SemTake(vdb->sessionSemaphore) < 0)
		{
			putErrmsg("Can't take sessionSemaphore.", NULL);
			return -1;
		}

		if (sm_SemEnded(vdb->sessionSemaphore))
		{
			putErrmsg("LTP has been stopped.", NULL);
			return 0;
		}

		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	dbobj = getLtpDbObject();
	sdr_stage(sdr, (char *) &ltpdb, dbobj, sizeof(LtpDB));
	ltpdb.sessionCount++;
	sdr_write(sdr, dbobj, (char *) &ltpdb, sizeof(LtpDB));
	sessionNbr = ltpdb.sessionCount;

	/*	exportSessions list element points to the session
	 *	structure.  exportSessionHash entry points to the
	 *	list element.						*/

	sessionObj = sdr_malloc(sdr, sizeof(ExportSession));
	if (sessionObj == 0
	|| (elt = sdr_list_insert_last(sdr, span.exportSessions,
			sessionObj)) == 0
	|| sdr_hash_insert(sdr, ltpdb.exportSessionsHash,
			(char *) &sessionNbr, elt) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't start session.", NULL);
		return -1;
	}

	/*	Write session to database.				*/

	memset((char *) &session, 0, sizeof(ExportSession));
	session.span = spanObj;
	session.sessionNbr = sessionNbr;
	encodeSdnv(&(session.sessionNbrSdnv), session.sessionNbr);
	session.svcDataObjects = sdr_list_create(sdr);
	session.redSegments = sdr_list_create(sdr);
	session.greenSegments = sdr_list_create(sdr);
	session.claims = sdr_list_create(sdr);
	sdr_write(sdr, sessionObj, (char *) &session, sizeof(ExportSession));

	/*	Note session address in span and finish: unless span
	 *	is currently inactive (i.e., localXmitRate is currently
	 *	zero) -- give the buffer-empty semaphore so that the
	 *	pending service data object (if any) can be inserted
	 *	into the buffer.					*/

	span.currentExportSessionObj = sessionObj;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	if (vspan->localXmitRate > 0)
	{
		sm_SemGive(vspan->bufEmptySemaphore);
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Can't start session.", NULL);
		return -1;
	}

	return 1;
}

#if defined (VXWORKS) || defined (RTEMS)
int	ltpmeter(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	unsigned long	remoteEngineId =
				a1 == 0 ? 0 : strtoul((char *) a1, NULL, 0);
#else
int	main(int argc, char *argv[])
{
	unsigned long	remoteEngineId =
				argc > 1 ? strtoul(argv[1], NULL, 0) : 0;
#endif
	Sdr		sdr;
	LtpDB		*ltpConstants;
	LtpVdb		*vdb;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanObj;
	LtpSpan		span;
	int		running = 1;
	int		returnCode = 0;
	char		memo[64];
	ExportSession	session;
	Lyst		extents;
	ExportExtent	*extent;
	int		segmentsIssued;

	if (remoteEngineId == 0)
	{
		puts("Usage: ltpmeter <non-zero remote engine ID>");
		return 0;
	}

	if (ltpInit(0, 0) < 0)
	{
		putErrmsg("ltpmeter can't initialize LTP.",
				utoa(remoteEngineId));
		return 1;
	}

	sdr = getIonsdr();
	ltpConstants = getLtpConstants();
	vdb = getLtpVdb();
	sdr_begin_xn(sdr);
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->meterPid > 0 && vspan->meterPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("ltpmeter task is already started for this engine.",
				itoa(remoteEngineId));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (span.currentExportSessionObj == 0)	/*	New span.	*/
	{
		sdr_exit_xn(sdr);
		switch (startExportSession(sdr, spanObj, vspan))
		{
		case -1:
			putErrmsg("ltpmeter can't start new session.",
				itoa(remoteEngineId));
			return 1;

		case 0:
			return 0;		/*	LTP stopped.	*/

		default:
			sdr_begin_xn(sdr);
			sdr_stage(sdr, (char *) &span, spanObj,
					sizeof(LtpSpan));
		}
	}

	writeMemo("[i] ltpmeter is running.");
	while (running)
	{
		/*	First wait until block aggregation buffer for
		 *	this span is full.				*/

		if (span.lengthOfBufferedBlock < span.aggrSizeLimit)
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vspan->bufFullSemaphore) < 0)
			{
				putErrmsg("Can't take bufFullSemaphore.",
						itoa(remoteEngineId));
				returnCode = 1;
				running = 0;
				continue;
			}

			if (sm_SemEnded(vspan->bufFullSemaphore))
			{
				sprintf(memo, "[i] LTP meter to engine %lu is \
stopped.", remoteEngineId);
				writeMemo(memo);
				running = 0;
				continue;
			}

			sdr_begin_xn(sdr);
			sdr_stage(sdr, (char *) &span, spanObj,
					sizeof(LtpSpan));
		}

		if (span.lengthOfBufferedBlock == 0)
		{
			continue;	/*	Nothing to do yet.	*/
		}

		/*	Now segment the block that is currently
		 *	aggregated in the buffer, giving the span's
		 *	segSemaphore once per segment.			*/

		sdr_stage(sdr, (char *) &session, span.currentExportSessionObj,
				sizeof(ExportSession));
		session.clientSvcId = span.clientSvcIdOfBufferedBlock;
		encodeSdnv(&(session.clientSvcIdSdnv), session.clientSvcId);
		session.totalLength = span.lengthOfBufferedBlock;
		session.redPartLength = span.redLengthOfBufferedBlock;
		if ((extents = lyst_create_using(getIonMemoryMgr())) == NULL
		|| (extent = (ExportExtent *) MTAKE(sizeof(ExportExtent)))
				== NULL
		|| lyst_insert_last(extents, extent) == NULL)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't create extents list.", NULL);
			returnCode = 1;
			running = 0;
			continue;	/*	Breaks main loop.	*/
		}

		extent->offset = 0;
		extent->length = session.totalLength;
		segmentsIssued = issueSegments(sdr, &span, vspan, &session,
				span.currentExportSessionObj, extents, 0);
		MRELEASE(extent);
		lyst_destroy(extents);
		switch (segmentsIssued)
		{
		case -1:		/*	System error.		*/
			sdr_cancel_xn(sdr);
			putErrmsg("Can't segment block.", NULL);
			returnCode = 1;
			running = 0;
			continue;	/*	Breaks main loop.	*/

		case 0:			/*	Database too full.	*/
			sdr_cancel_xn(sdr);

			/*	Wait one second and try again.		*/

			snooze(1);
			sdr_begin_xn(sdr);
			sdr_stage(sdr, (char *) &span, spanObj,
					sizeof(LtpSpan));
			continue;
		}

		/*	Segment issuance succeeded.			*/

		if (vdb->watching & WATCH_f)
		{
			putchar('f');
			fflush(stdout);
		}

		/*	Notify the source client.			*/

		if (enqueueNotice(vdb->clients + session.clientSvcId,
				ltpConstants->ownEngineId, session.sessionNbr,
				0, 0, LtpExportSessionStart, 0, 0, 0) < 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("Can't post ExportSessionStart notice.",
					NULL);
			returnCode = 1;
			running = 0;
			continue;	/*	Breaks main loop.	*/
		}

		/*	Commit changes to current session to the
		 *	database.					*/

		sdr_write(sdr, span.currentExportSessionObj, (char *) &session,
				sizeof(ExportSession));

		/*	Reinitialize span's block buffer.		*/

		span.ageOfBufferedBlock = 0;
		span.lengthOfBufferedBlock = 0;
		span.redLengthOfBufferedBlock = 0;
		span.clientSvcIdOfBufferedBlock = 0;
		span.currentExportSessionObj = 0;
		sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't finish session.", NULL);
			returnCode = 1;
			running = 0;
			continue;	/*	Breaks main loop.	*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();

		/*	Start an export session for the next block.	*/

		switch (startExportSession(sdr, spanObj, vspan))
		{
		case -1:
			putErrmsg("ltpmeter can't start new session.",
					utoa(remoteEngineId));
			returnCode = 1;
			running = 0;
			continue;

		case 0:
			running = 0;
			continue;
		}

		/*	Now start next cycle of main loop, waiting
		 *	for the new session's buffer to be filled.	*/

		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	writeErrmsgMemos();
	writeMemo("[i] ltpmeter has ended.");
	ionDetach();
	return returnCode;
}
