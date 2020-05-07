/*
	ltpcli.c:	BP LTP-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "ltpcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("ltpcli");
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct		*vduct;
	int		running;
} ReceiverThreadParms;

static int	acquireRedBundles(AcqWorkArea *work, Object zco,
			uvast senderEngineNbr)
{
	if (bpBeginAcq(work, 0, NULL) < 0)
	{
		putErrmsg("Can't begin acquisition of bundle(s).", NULL);
		return -1;
	}

	if (bpLoadAcq(work, zco) < 0)
	{
		putErrmsg("Can't continue bundle acquisition.", NULL);
		return -1;
	}

	if (bpEndAcq(work) < 0)
	{
		putErrmsg("Can't end acquisition of bundle(s).", NULL);
		return -1;
	}

	return 0;
}

static int	handleGreenSegment(AcqWorkArea *work, LtpSessionId *sessionId,
			unsigned char endOfBlock, unsigned int offset,
			unsigned int length, Object zco, unsigned int *buflen,				char **buffer)
{
	Sdr			sdr = getIonsdr();
	static LtpSessionId	currentSessionId = { 0, 0 };
	static unsigned int	currentOffset = 0;
	//unsigned int		fillLength;
	ZcoReader		reader;
	int			result;

	if (zco == 0)		/*	Import session canceled.	*/
	{
		bpCancelAcq(work);
		currentSessionId.sourceEngineId = 0;
		currentSessionId.sessionNbr = 0;
		currentOffset = 0;
		return 0;
	}

	if (zco_source_data_length(sdr, zco) != length)
	{
		return 0;	/*	Just discard the segment.	*/
	}

	if (sessionId->sourceEngineId != currentSessionId.sourceEngineId
	|| sessionId->sessionNbr != currentSessionId.sessionNbr)
	{
		/*	Did not receive end-of-block segment for the
		 *	block that was being received.  Discard the
		 *	partially received bundle in the work area,
		 *	if any.						*/

		bpCancelAcq(work);
		currentSessionId.sourceEngineId = 0;
		currentSessionId.sessionNbr = 0;
		currentOffset = 0;
	}

	if (currentOffset == 0)
	{
		/*	Start new green bundle acquisition.		*/

		if (bpBeginAcq(work, 0, NULL) < 0)
		{
			putErrmsg("Can't begin acquisition of bundle.", NULL);
			return -1;
		}

		currentSessionId.sourceEngineId = sessionId->sourceEngineId;
		currentSessionId.sessionNbr = sessionId->sessionNbr;
	}

	if (offset < currentOffset)	/*	Out of order.		*/
	{
		return 0;	/*	Just discard the segment.	*/
	}

	if (offset > currentOffset)
	{
		bpCancelAcq(work);
		currentSessionId.sourceEngineId = 0;
		currentSessionId.sessionNbr = 0;
		currentOffset = 0;
		return 0;
	}

	if (length > *buflen)
	{
		/*	Make buffer big enough for the green data.	*/

		if (*buffer)
		{
			MRELEASE(*buffer);
			*buflen = 0;
		}

		*buffer = MTAKE(length);
		if (*buffer == NULL)
		{
			/*	Segment is too large.  Might be a
			 *	DOS attack; cancel acquisition.		*/

			bpCancelAcq(work);
			currentSessionId.sourceEngineId = 0;
			currentSessionId.sessionNbr = 0;
			currentOffset = 0;
			return 0;
		}

		*buflen = length;
	}

	/*	Extract data from segment ZCO so that it can be
	 *	appended to the bundle acquisition ZCO.  Note
	 *	that we're breaking the "zero-copy" model here;
	 *	it would be better to have an alternate version
	 *	of bpContinueAcq that uses zco_clone_source_data
	 *	to append the segment ZCO's source data to the
	 *	acquisition ZCO in the work area. (TODO)		*/

	zco_start_receiving(zco, &reader);
	CHKERR(sdr_begin_xn(sdr));
	result = zco_receive_source(sdr, &reader, length, *buffer);
	if (sdr_end_xn(sdr) < 0 || result < 0)
	{
		putErrmsg("Failed reading green segment data.", NULL);
		return -1;
	}

	if (bpContinueAcq(work, *buffer, (int) length, 0) < 0)
	{
		putErrmsg("Can't continue bundle acquisition.", NULL);
		return -1;
	}

	currentOffset += length;
	if (endOfBlock)
	{
		if (bpEndAcq(work) < 0)
		{
			putErrmsg("Can't end acquisition of bundle.", NULL);
			return -1;
		}

		currentSessionId.sourceEngineId = 0;
		currentSessionId.sessionNbr = 0;
		currentOffset = 0;
	}

	return 0;
}

static void	*handleNotices(void *parm)
{
	/*	Main loop for LTP notice reception and handling.	*/

	Sdr			sdr = getIonsdr();
	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*procName = "ltpcli";
	AcqWorkArea		*redWork;
	AcqWorkArea		*greenWork;
	LtpNoticeType		type;
	LtpSessionId		sessionId;
	unsigned char		reasonCode;
	unsigned char		endOfBlock;
	unsigned int		dataOffset;
	unsigned int		dataLength;
	Object			data;		/*	ZCO reference.	*/
	unsigned int		greenBuflen = 0;
	char			*greenBuffer = NULL;

	snooze(1);	/*	Let main thread become interruptable.	*/
	if (ltp_open(BpLtpClientId) < 0)
	{
		putErrmsg("ltpcli can't open client access.",
				itoa(BpLtpClientId));
		ionKillMainThread(procName);
		return NULL;
	}

	redWork = bpGetAcqArea(rtp->vduct);
	greenWork = bpGetAcqArea(rtp->vduct);
	if (redWork == NULL || greenWork == NULL)
	{
		ltp_close(BpLtpClientId);
		putErrmsg("ltpcli can't get acquisition work areas", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Can now start receiving notices.  On failure, take
	 *	down the CLI.						*/

	while (rtp->running)
	{
		if (ltp_get_notice(BpLtpClientId, &type, &sessionId,
				&reasonCode, &endOfBlock, &dataOffset,
				&dataLength, &data) < 0)
		{
			putErrmsg("Can't get LTP notice.", NULL);
			ionKillMainThread(procName);
			rtp->running = 0;
			continue;
		}

		switch (type)
		{
		case LtpExportSessionComplete:	/*	Xmit success.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			if (bpHandleXmitSuccess(data, 0) < 0)
			{
				putErrmsg("Crashed on xmit success.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
				break;		/*	Out of switch.	*/
			}

			CHKNULL(sdr_begin_xn(sdr));
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed on data cleanup.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		case LtpExportSessionCanceled:	/*	Xmit failure.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			if (bpHandleXmitFailure(data) < 0)
			{
				putErrmsg("Crashed on xmit failure.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
				break;		/*	Out of switch.	*/
			}

			CHKNULL(sdr_begin_xn(sdr));
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed on data cleanup.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		case LtpImportSessionCanceled:
			/*	None of the red data for the import
			 *	session (if any) have been received
			 *	yet, so nothing to discard.  In case
			 *	part or all of the import session was
			 *	green data, force deletion of retained
			 *	data.					*/

			sessionId.sourceEngineId = 0;
			sessionId.sessionNbr = 0;
			if (handleGreenSegment(greenWork, &sessionId,
					0, 0, 0, 0, NULL, NULL) < 0)
			{
				putErrmsg("Can't cancel green session.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		case LtpRecvRedPart:
			if (!endOfBlock)
			{
				/*	Block is partially red and
				 *	partially green.  Too risky
				 *	to wait for green EOB before
				 *	clearing the work area, so
				 *	just discard the data.		*/

				CHKNULL(sdr_begin_xn(sdr));
				zco_destroy(sdr, data);
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Crashed: partially red.",
							NULL);
					ionKillMainThread(procName);
					rtp->running = 0;
				}

				break;		/*	Out of switch.	*/
			}

			if (acquireRedBundles(redWork, data,
					sessionId.sourceEngineId) < 0)
			{
				putErrmsg("Can't acquire bundle(s).", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		case LtpRecvGreenSegment:
			if (handleGreenSegment(greenWork, &sessionId,
					endOfBlock, dataOffset, dataLength,
					data, &greenBuflen, &greenBuffer) < 0)
			{
				putErrmsg("Can't handle green segment.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			/*	Discard the ZCO in any case.		*/

			CHKNULL(sdr_begin_xn(sdr));
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed: green segment.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		default:
			break;		/*	Out of switch.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] ltpcli receiver thread has ended.");

	/*	Free resources.						*/

	if (greenBuffer)
	{
		MRELEASE(greenBuffer);
	}

	bpReleaseAcqArea(greenWork);
	bpReleaseAcqArea(redWork);
	ltp_close(BpLtpClientId);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	ltpcli(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;

	if (ductName == NULL)
	{
		PUTS("Usage: ltpcli <local engine number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("ltpcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("ltp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such ltp duct.", ductName);
		return -1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpcli can't initialize LTP.", NULL);
		return -1;
	}

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("ltpcli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.vduct = vduct;
	rtp.running = 1;
	if (pthread_begin(&receiverThread, NULL, handleNotices, &rtp))
	{
		putSysErrmsg("ltpcli can't create receiver thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	writeMemo("[i] ltpcli is running.");
	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;

	/*	Stop the receiver thread by interrupting client access.	*/

	ltp_interrupt(BpLtpClientId);
	pthread_join(receiverThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] ltpcli duct has ended.");
	ionDetach();
	return 0;
}
