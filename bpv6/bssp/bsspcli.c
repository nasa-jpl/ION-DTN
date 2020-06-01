/*
 *	bsspcli.c:	BP BSSP-based convergence-layer input
 *			daemon, designed to serve as an input
 *			duct.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *		 Scott Burleigh, JPL
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *
 */
#include "bsspcla.h"

static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("bsspcli");
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct		*vduct;
	int		running;
} ReceiverThreadParms;

int	acquireBundle(AcqWorkArea *work, BsspSessionId *sessionId,
			unsigned int length, Object zco, 
			unsigned int *buflen, char **buffer)
{
	Sdr		sdr = getIonsdr();
	ZcoReader	reader;
	int		result;

	if (zco == 0)		/*	Session canceled.		*/
	{
		bpCancelAcq(work);
		return 0;
	}

	if (zco_source_data_length(sdr, zco) != length)
	{
		return 0;	/* 	Just discard the block.		*/
	}

	/*	Start new bundle acquisition.				*/

	if (bpBeginAcq(work, 0, NULL) < 0)
	{
		putErrmsg("Can't begin acquisition of bundle.", NULL);
		return -1;
	}

	if (length > *buflen)
	{
		/*	Make buffer big enough.				*/

		if (*buffer)
		{
			MRELEASE(*buffer);
			*buflen = 0;
		}

		*buffer = MTAKE(length);
		if (*buffer == NULL)
		{
				
			bpCancelAcq(work);
			return 0;
		}

		*buflen = length;
	}


	/*	Extract data from block ZCO so that it can be
	 *	appended to the bundle acquisition ZCO.			*/

	zco_start_receiving(zco, &reader);
	CHKERR(sdr_begin_xn(sdr));
	result = zco_receive_source(sdr, &reader, length, *buffer);
	if (sdr_end_xn(sdr) < 0 || result < 0)
	{
		putErrmsg("Failed reading bssp block data.", NULL);
		return -1;
	}

	if (bpContinueAcq(work, *buffer, (int) length, 0, 0) < 0)
	{
		putErrmsg("Can't continue bundle acquisition.", NULL);
		return -1;
	}

	if (bpEndAcq(work) < 0)
	{
		putErrmsg("Can't end acquisition of bundle.", NULL);
		return -1;
	}
	return 0;
}

static void	*handleNotices(void *parm)
{
	/*	Main loop for BSSP notice reception and handling.	*/

	Sdr			sdr = getIonsdr();
	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	char			*procName = "bsspcli";
	AcqWorkArea		*work;
	BsspNoticeType		type;
	BsspSessionId		sessionId;
	unsigned char		reasonCode;
	unsigned int		dataLength;
	Object			data;		/*	ZCO reference.	*/
	unsigned int		buflen = 0;
	char			*buffer = NULL;

	snooze(1);	/*	Let main thread become interruptable.	*/
	if (bssp_open(BpBsspClientId) < 0)
	{
		putErrmsg("bsspcli can't open client access.",
				itoa(BpBsspClientId));
		ionKillMainThread(procName);
		return NULL;
	}
	
	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		bssp_close(BpBsspClientId);
		putErrmsg("bsspcli can't get acquisition work areas", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Can now start receiving notices.  On failure, take
	 *	down the CLI.						*/

	while (rtp->running)
	{
		if (bssp_get_notice(BpBsspClientId, &type, &sessionId,
				&reasonCode, &dataLength, &data) < 0)
		{
			putErrmsg("Can't get BSSP notice.", NULL);
			ionKillMainThread(procName);
			rtp->running = 0;
			continue;
		}

		switch (type)
		{
		case BsspXmitSuccess:		/*	Xmit success.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			if (bpHandleXmitSuccess(data, 0) < 0)
			{
				putErrmsg("Crashed on xmit success.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;			/*	Out of switch.	*/

		case BsspXmitFailure:		/*	Xmit failure.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			if (bpHandleXmitFailure(data) < 0)
			{
				putErrmsg("Crashed on xmit failure.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;			/*	Out of switch.	*/

		case BsspRecvSuccess:
			if (acquireBundle(work, &sessionId, dataLength,
					data, &buflen, &buffer) < 0)
			{
				putErrmsg("Can't handle bssp block.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			/*	Discard the ZCO in any case.		*/

			CHKNULL(sdr_begin_xn(sdr));
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Crashed: acquireBundle.", NULL);
				ionKillMainThread(procName);
				rtp->running = 0;
			}

			break;			/*	Out of switch.	*/


		default:
			break;			/*	Out of switch.	*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] bsspcli receiver thread has ended.");

	/*	Free resources.						*/

	if (buffer)
	{
		MRELEASE(buffer);
	}

	bpReleaseAcqArea(work);
	bssp_close(BpBsspClientId);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	bsspcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct			*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			duct;
	ClProtocol		protocol;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;

	if (ductName == NULL)
	{
		PUTS("Usage: bsspcli <local engine number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("bsspcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("bssp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such bssp duct.", ductName);
		return -1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("BSSPCLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return -1;
	}

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/*	All command-line arguments are now validated.		*/

	if (bssp_attach() < 0)
	{
		putErrmsg("bsspcli can't initialize BSSP.", NULL);
		return -1;
	}

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("bsspcli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.vduct = vduct;
	rtp.running = 1;
	if (pthread_begin(&receiverThread, NULL, handleNotices, &rtp, "bsspcli_receiver"))
	{
		putSysErrmsg("bsspcli can't create receiver thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	writeMemo("[i] bsspcli is running.");
	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	rtp.running = 0;

	/*	Stop the receiver thread by interrupting client access.	*/

	bssp_interrupt(BpBsspClientId);
	pthread_join(receiverThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] bsspcli duct has ended.");
	ionDetach();
	return 0;
}
