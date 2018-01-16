/*

	ltpmeter.c:	LTP flow control and block segmentation daemon.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
	
									*/
#include "ltpP.h"

#if defined (ION_LWT)
int	ltpmeter(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	uvast	remoteEngineId = a1 == 0 ? 0 : strtouvast((char *) a1);
#else
int	main(int argc, char *argv[])
{
	uvast	remoteEngineId = argc > 1 ? strtouvast(argv[1]) : 0;
#endif
	Sdr		sdr;
	LtpVdb		*vdb;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanObj;
	LtpSpan		span;
	int		returnCode = 0;
	char		memo[64];
	ExportSession	session;
	Lyst		extents;
	ExportExtent	*extent;
	unsigned int	ckptSerialNbr;
	int		result;

	if (remoteEngineId == 0)
	{
		PUTS("Usage: ltpmeter <non-zero remote engine ID>");
		return 0;
	}

	if (ltpInit(0) < 0)
	{
		putErrmsg("ltpmeter can't initialize LTP.",
				utoa(remoteEngineId));
		return 1;
	}

	sdr = getIonsdr();
	vdb = getLtpVdb();
	CHKZERO(sdr_begin_xn(sdr));
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->meterPid != ERROR && vspan->meterPid != sm_TaskIdSelf())
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
		/*	Must start span's initial session.		*/

		sdr_exit_xn(sdr);
		if (startExportSession(sdr, spanObj, vspan) < 0)
		{
			putErrmsg("ltpmeter can't start new session.",
					itoa(remoteEngineId));
			return 1;
		}

		CHKZERO(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	writeMemo("[i] ltpmeter is running.");
	while (returnCode == 0)
	{
		/*	First wait until block aggregation buffer for
		 *	this span is closed.				*/

		if (span.lengthOfBufferedBlock < span.aggrSizeLimit)
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vspan->bufClosedSemaphore) < 0)
			{
				putErrmsg("Can't take bufClosedSemaphore.",
						itoa(remoteEngineId));
				returnCode = 1;
				continue;	/*	Failure.	*/
			}

			if (sm_SemEnded(vspan->bufClosedSemaphore))
			{
				isprintf(memo, sizeof memo, "[i] LTP meter to \
engine " UVAST_FIELDSPEC " is stopped.", remoteEngineId);
				writeMemo(memo);
				break;		/*	Outer loop.	*/
			}

			CHKZERO(sdr_begin_xn(sdr));
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

		/*	We can now compute the upper limit on the number
		 *	of checkpoints we will send in the course of
		 *	this session.  We send one initial checkpoint
		 *	plus one more checkpoint in response to every
		 *	report except the last, which elicits only a
		 *	report acknowledgment.  So the maximum number
		 *	of reports that we expect from the receiver
		 *	determines the maximum number of checkpoints
		 *	we will send.					*/

		session.maxCheckpoints = getMaxReports(session.redPartLength,
				vspan, 0);
		if ((extents = lyst_create_using(getIonMemoryMgr())) == NULL
		|| (extent = (ExportExtent *) MTAKE(sizeof(ExportExtent)))
				== NULL
		|| lyst_insert_last(extents, extent) == NULL)
		{
			putErrmsg("Can't create extents list.", NULL);
			sdr_cancel_xn(sdr);
			returnCode = 1;
			continue;	/*	Failure.		*/
		}

		extent->offset = 0;
		extent->length = session.totalLength;
		do
		{
			ckptSerialNbr = rand();

			/*	Limit serial number SDNV length.	*/

			ckptSerialNbr %= LTP_SERIAL_NBR_LIMIT;
		} while (ckptSerialNbr == 0);
		result = issueSegments(sdr, &span, vspan, &session,
				span.currentExportSessionObj, extents, 0,
				ckptSerialNbr);
		MRELEASE(extent);
		lyst_destroy(extents);
		if (result < 0)
		{
			putErrmsg("Can't segment block.", NULL);
			sdr_cancel_xn(sdr);
			returnCode = 1;
			continue;	/*	Failure.		*/
		}

		/*	Segment issuance succeeded.			*/

		if (vdb->watching & WATCH_f)
		{
			iwatch('f');
		}

		if (enqueueNotice(vdb->clients + session.clientSvcId,
				vdb->ownEngineId, session.sessionNbr,
				0, 0, LtpExportSessionStart, 0, 0, 0) < 0)
		{
			putErrmsg("Can't post ExportSessionStart notice.",
					NULL);
			sdr_cancel_xn(sdr);
			returnCode = 1;
			continue;	/*	Failure.		*/
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
			continue;	/*	Failure.		*/
		}

		/*	Start an export session for the next block.	*/

		if (startExportSession(sdr, spanObj, vspan) < 0)
		{
			putErrmsg("ltpmeter can't start new session.",
					utoa(remoteEngineId));
			returnCode = 1;
			continue;	/*	Failure.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();

		/*	Now start next cycle of main loop, waiting
		 *	for the new session's buffer to be closed.	*/

		CHKZERO(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	writeErrmsgMemos();
	writeMemo("[i] ltpmeter has ended.");
	ionDetach();
	return returnCode;
}
