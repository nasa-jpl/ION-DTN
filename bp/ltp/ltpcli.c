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

static pthread_t	ltpcliMainThread(pthread_t tid)
{
	static pthread_t	mainThread = 0;

	if (tid)
	{
		mainThread = tid;
	}

	return mainThread;
}

static void	interruptThread()
{
	pthread_t	mainThread = ltpcliMainThread(0);

	isignal(SIGTERM, interruptThread);
	if (mainThread != pthread_self())
	{
		pthread_kill(mainThread, SIGTERM);
	}
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct		*vduct;
	pthread_t	mainThread;
	int		running;
} ReceiverThreadParms;

static int	acquireBundles(AcqWorkArea *work, unsigned long dataLength,
			char *data, unsigned long senderEngineNbr)
{
	unsigned char	*cursor = (unsigned char *) data;
	char		*endOfData = data + dataLength;
	char		engineNbrString[21];
	char		senderEidBuffer[SDRSTRING_BUFSZ];
	char		*senderEid;
	char		*startOfBundle;
	char		*endOfBundle;
	unsigned int	bundleLength;
	int		sdnvLength;
	unsigned long	bundleProcFlags;
	unsigned long	remainingPrimaryBlockLength;
	unsigned long	blockProcFlags;
	int		lastBlock;
	unsigned long	eidReferencesCount;
	unsigned long	schemeOffset;
	unsigned long	sspOffset;
	unsigned long	blockDataLength;

	sprintf(engineNbrString, "%lu", senderEngineNbr);
	senderEid = senderEidBuffer;
	getSenderEid(&senderEid, engineNbrString);
	while (dataLength > 0)
	{
		/*	Pre-parse bundle to get its length.		*/

		startOfBundle = endOfBundle = (char *) cursor;

		/*	Skip over version number.			*/

		bundleLength = 1;
		cursor += 1;
		endOfBundle += 1;
		if (endOfBundle > endOfData) return 0;

		/*	Skip over bundle processing flags.		*/

		sdnvLength = decodeSdnv(&bundleProcFlags, cursor);
		bundleLength += sdnvLength;
		cursor += sdnvLength;
		endOfBundle += sdnvLength;
		if (endOfBundle > endOfData) return 0;

		/*	Skip over remaining primary block length.	*/

		sdnvLength = decodeSdnv(&remainingPrimaryBlockLength, cursor);
		bundleLength += sdnvLength;
		cursor += sdnvLength;
		endOfBundle += sdnvLength;
		if (endOfBundle > endOfData) return 0;

		/*	Skip over rest of primary block.		*/

		bundleLength += remainingPrimaryBlockLength;
		cursor += remainingPrimaryBlockLength;
		endOfBundle += remainingPrimaryBlockLength;
		if (endOfBundle > endOfData) return 0;

		/*	Skip over all non-primary blocks in bundle.	*/

		lastBlock = 0;
		while (!lastBlock)
		{
			/*	Skip over block type.			*/

			bundleLength += 1;
			cursor += 1;
			endOfBundle += 1;
			if (endOfBundle > endOfData) return 0;

			/*	Skip over block processing flags.	*/

			sdnvLength = decodeSdnv(&blockProcFlags, cursor);
			bundleLength += sdnvLength;
			cursor += sdnvLength;
			endOfBundle += sdnvLength;
			if (endOfBundle > endOfData) return 0;
			if (blockProcFlags & BLK_IS_LAST)
			{
				lastBlock = 1;
			}

			/*	Skip over EID-reference field.		*/

			if (blockProcFlags & BLK_HAS_EID_REFERENCES)
			{
				/*	Skip over EID references count.	*/

				sdnvLength = decodeSdnv(&eidReferencesCount,
						cursor);
				bundleLength += sdnvLength;
				cursor += sdnvLength;
				endOfBundle += sdnvLength;
				if (endOfBundle > endOfData) return 0;
				while (eidReferencesCount > 0)
				{
					sdnvLength = decodeSdnv(&schemeOffset,
							cursor);
					bundleLength += sdnvLength;
					cursor += sdnvLength;
					endOfBundle += sdnvLength;
					if (endOfBundle > endOfData) return 0;
					sdnvLength = decodeSdnv(&sspOffset,
							cursor);
					bundleLength += sdnvLength;
					cursor += sdnvLength;
					endOfBundle += sdnvLength;
					if (endOfBundle > endOfData) return 0;
					eidReferencesCount--;
				}
			}

			/*	Skip over block data length.		*/

			sdnvLength = decodeSdnv(&blockDataLength, cursor);
			bundleLength += sdnvLength;
			cursor += sdnvLength;
			endOfBundle += sdnvLength;
			if (endOfBundle > endOfData) return 0;

			/*	Skip over block data.			*/

			bundleLength += blockDataLength;
			cursor += blockDataLength;
			endOfBundle += blockDataLength;
			if (endOfBundle > endOfData) return 0;
		}

		if (bpBeginAcq(work, 0, senderEid) < 0)
		{
			putSysErrmsg("can't begin acquisition of bundle", NULL);
			return -1;
		}

		if (bpContinueAcq(work, startOfBundle, bundleLength) < 0)
		{
			putSysErrmsg("can't continue bundle acquisition", NULL);
			return -1;
		}

		if (bpEndAcq(work) < 0)
		{
			putSysErrmsg("can't end acquisition of bundle", NULL);
			return -1;
		}

		dataLength -= bundleLength;
	}

	return 1;
}

static void	*handleNotices(void *parm)
{
	/*	Main loop for LTP notice reception and handling.	*/

	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	AcqWorkArea		*work;
	LtpNoticeType		type;
	LtpSessionId		sessionId;
	unsigned char		reasonCode;
	unsigned char		endOfBlock;
	unsigned long		dataOffset;
	unsigned long		dataLength;
	char			*data;
	int			result;

	snooze(1);	/*	Let main thread get interruptable.	*/
	if (ltp_open(BpLtpClientId) < 0)
	{
		putErrmsg("ltpcli can't open client access.",
				itoa(BpLtpClientId));
		pthread_kill(rtp->mainThread, SIGTERM);
		return NULL;
	}

	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		ltp_close(BpLtpClientId);
		putSysErrmsg("ltpcli can't get acquisition work area", NULL);
		pthread_kill(rtp->mainThread, SIGTERM);
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
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}

		switch (type)
		{
		case LtpExportSessionCanceled:	/*	Xmit failure.	*/
			if (data == NULL)
			{
				break;		/*	Ignore it.	*/
			}

			result = bpHandleXmitFailure(data, dataLength);
			ltp_release_data(data);
			if (result < 0)
			{
				putErrmsg("Crashed on xmit failure.", NULL);
				pthread_kill(rtp->mainThread, SIGTERM);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		case LtpImportSessionCanceled:
			break;		/*	Out of switch.		*/

		case LtpRecvGreenSegment:
		case LtpRecvRedPart:
			result = acquireBundles(work, dataLength, data,
					sessionId.sourceEngineId);
			ltp_release_data(data);
			switch (result)
			{
			case 1:		/*	No problem.		*/
				break;	/*	Out of inner switch.	*/

			case 0:		/*	Malformed block.	*/
				putErrmsg("Malformed LTP block, one or more \
bundles not extracted.", NULL);
				break;	/*	Out of inner switch.	*/

			default:	/*	System failure.		*/
				putErrmsg("Can't process LTP content.", NULL);
				pthread_kill(rtp->mainThread, SIGTERM);
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

	bpReleaseAcqArea(work);
	ltp_close(BpLtpClientId);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
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
		puts("Usage: ltpcli <local engine number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putSysErrmsg("ltpcli can't attach to BP", NULL);
		return 1;
	}

	findInduct("ltp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such ltp duct.", ductName);
		return 1;
	}

	if (vduct->cliPid > 0 && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	if (ltp_attach() < 0)
	{
		putErrmsg("ltpcli can't initialize LTP.", NULL);
		return 1;
	}

	/*	Initialize sender endpoint ID lookup.			*/

	ipnInit();
	dtn2Init();

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	oK(ltpcliMainThread(pthread_self()));
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.vduct = vduct;
	rtp.running = 1;
	rtp.mainThread = pthread_self();
	if (pthread_create(&receiverThread, NULL, handleNotices, &rtp))
	{
		putSysErrmsg("ltpcli can't create receiver thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	writeMemo("[i] ltpcli is running.");
	snooze(2000000000);

	/*	Time to shut down.					*/

	rtp.running = 0;

	/*	Stop the receiver thread by interrupting client access.	*/

	ltp_interrupt(BpLtpClientId);
	pthread_join(receiverThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] ltpcli duct has ended.");
	bp_detach();
	return 0;
}
