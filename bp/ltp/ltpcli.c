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

static int	acquireBundles(AcqWorkArea *work, Object zco,
			unsigned long senderEngineNbr)
{
	char	engineNbrString[21];
	char	senderEidBuffer[SDRSTRING_BUFSZ];
	char	*senderEid;

	isprintf(engineNbrString, sizeof engineNbrString, "%lu",
			senderEngineNbr);
	senderEid = senderEidBuffer;
	getSenderEid(&senderEid, engineNbrString);
	if (bpBeginAcq(work, 0, senderEid) < 0)
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
	Object			data;		/*	ZCO reference.	*/
	Sdr			sdr;

	snooze(1);	/*	Let main thread become interruptable.	*/
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
		putErrmsg("ltpcli can't get acquisition work area", NULL);
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
		case LtpExportSessionComplete:	/*	Xmit success.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			if (bpHandleXmitSuccess(data) < 0)
			{
				putErrmsg("Crashed on xmit success.", NULL);
				pthread_kill(rtp->mainThread, SIGTERM);
				rtp->running = 0;
				break;		/*	Out of switch.	*/
			}

			sdr = getIonsdr();
			sdr_begin_xn(sdr);
			zco_destroy_reference(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed on data cleanup.", NULL);
				pthread_kill(rtp->mainThread, SIGTERM);
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
				pthread_kill(rtp->mainThread, SIGTERM);
				rtp->running = 0;
				break;		/*	Out of switch.	*/
			}

			sdr = getIonsdr();
			sdr_begin_xn(sdr);
			zco_destroy_reference(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed on data cleanup.", NULL);
				pthread_kill(rtp->mainThread, SIGTERM);
				rtp->running = 0;
			}

			break;		/*	Out of switch.		*/

		case LtpImportSessionCanceled:
			break;		/*	Out of switch.		*/

		case LtpRecvGreenSegment:
		case LtpRecvRedPart:
			if (acquireBundles(work, data,
					sessionId.sourceEngineId) < 0)
			{
				putErrmsg("Can't acquire bundle(s).", NULL);
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

	if (vduct->cliPid > 0 && vduct->cliPid != sm_TaskIdSelf())
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
	ionDetach();
	return 0;
}
