/*
	dgrcli.c:	ION DGR convergence-layer reception daemon.
			Adapted from dgrcla.c, 2006.

	Author: Scott Burleigh, JPL

	Copyright (c) 2017, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "dgr.h"

#define	DGRCLA_PORT_NBR		1113
#define	DGRCLA_BUFSZ		65535

static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("dgrcli");
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct		*vduct;
	int		*running;
	Dgr		dgrSap;
} ReceiverThreadParms;

static void	*receiveBundles(void *parm)
{
	/*	Main loop for bundle reception thread via DGR.		*/

	Sdr			sdr = getIonsdr();
	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "dgrcli";
	char			*buffer;
	AcqWorkArea		*work;
	int			threadRunning = 1;
	DgrRC			rc;
	unsigned short		fromPortNbr;
	unsigned int		fromHostNbr;
	int			length;
	int			errnbr;
	Object			bundleZco;
	char			hostName[MAXHOSTNAMELEN + 1];

	snooze(1);	/*	Let main thread become interruptable.	*/
	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("dgrcli can't get acquisition work area.", NULL);
		*(parms->running) = 0;
		ionKillMainThread(procName);
		return NULL;
	}

	buffer = MTAKE(DGRCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("dgrcli can't get DGR buffer.", NULL);
		*(parms->running) = 0;
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Now start receiving bundles.				*/

	while (threadRunning)
	{
		while (1)
		{
			if (dgr_receive(parms->dgrSap, &fromPortNbr,
					&fromHostNbr, buffer, &length, &errnbr,
					DGR_BLOCKING, &rc) < 0)
			{
				putErrmsg("Failed receiving bundle.", NULL);
				threadRunning = 0;
				rc = DgrFailed;
				break;		/*	Out of loop.	*/
			}

			/*	Note: we use zco_create to obtain ZCO
			 *	space here, because we know that these
			 *	allocations are strictly temporary; the
			 *	ZCOs are destroyed immediately after
			 *	creation.  We pass additive inverse of
			 *	length to zco_create to indicate that
			 *	this allocation is not subject to flow
			 *	control and must always be honored.	*/

			switch (rc)
			{
				case DgrDatagramAcknowledged:
					CHKNULL(sdr_begin_xn(sdr));
					bundleZco = zco_create(sdr,
						ZcoSdrSource, sdr_insert(sdr,
						buffer, length), 0, 0 - length,
						ZcoOutbound);
					if (sdr_end_xn(sdr) < 0
					|| bundleZco == (Object) ERROR)
					{
						putErrmsg("Failed creating \
temporary ZCO.", NULL);
						threadRunning = 0;
						break;	/*	Switch.	*/
					}

					if (bundleZco == 0)
					{
						/*	No ZCO space;
						 *	in effect,
						 *	datagram lost.	*/

						continue;
					}

					if (bpHandleXmitSuccess(bundleZco, 0)
							< 0)
					{
						threadRunning = 0;
						putErrmsg("Crashed handling \
success.", NULL);
					}

					if (threadRunning == 0)
					{
						break;	/*	Switch.	*/
					}

					continue;

				case DgrDatagramNotAcknowledged:
					CHKNULL(sdr_begin_xn(sdr));
					bundleZco = zco_create(sdr,
						ZcoSdrSource, sdr_insert(sdr,
						buffer, length), 0, 0 - length,
						ZcoOutbound);
					if (sdr_end_xn(sdr) < 0
					|| bundleZco == (Object) ERROR)
					{
						putErrmsg("Failed creating \
temporary ZCO.", NULL);
						threadRunning = 0;
						break;	/*	Switch.	*/
					}

					if (bundleZco == 0)
					{
						/*	No ZCO space;
						 *	in effect,
						 *	datagram lost.	*/

						continue;
					}

					if (bpHandleXmitFailure(bundleZco) < 0)
					{
						threadRunning = 0;
						putErrmsg("Crashed handling \
failure.", NULL);
					}

					if (threadRunning == 0)
					{
						break;	/*	Switch.	*/
					}

					continue;

				case DgrFailed:
				case DgrDatagramReceived:
					break;	/*	Out of switch.	*/

				default:
					continue;
			}

			break;			/*	Out of loop.	*/
		}

		if (rc == DgrDatagramAcknowledged
		|| rc == DgrDatagramNotAcknowledged)
		{
			putErrmsg("Crashed handling xmit result.", NULL);
			threadRunning = 0;
			continue;
		}

		if (rc == DgrFailed)
		{
			if (*(parms->running) != 0)
			{
				/*	Not terminated by main thread.	*/

				writeMemo("[?] dgrcli failed in bundle acq.");
			}

			threadRunning = 0;
			continue;
		}

		/*	Must have received a datagram.			*/

		printDottedString(fromHostNbr, hostName);
		if (bpBeginAcq(work, 0, NULL) < 0
		|| bpContinueAcq(work, buffer, length, 0, 0) < 0
		|| bpEndAcq(work) < 0)
		{
			putErrmsg("Can't acquire bundle.", NULL);
			threadRunning = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	*(parms->running) = 0;
	ionKillMainThread(procName);

	/*	Finish releasing receiver thread's resources.		*/

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	writeErrmsgMemos();
	writeMemo("[i] dgrcli receiver thread stopping.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	dgrcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct			*vinduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			induct;
	ClProtocol		protocol;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	Dgr			dgrSap;
	DgrRC			rc;
	int			running = 1;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;

	if (ductName == NULL)
	{
		PUTS("Usage: dgrcli <local host name>[:<port number>]");
		PUTS("[port number defaults to 1113]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("dgrcli can't attach to BP.", NULL);
		return 1;
	}

	findInduct("dgr", ductName, &vinduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such dgr induct.", ductName);
		return 1;
	}

	if (vinduct->cliPid != ERROR && vinduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vinduct->cliPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr, vinduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, induct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		putErrmsg("Can't get IP/port for host.", ductName);
		return 1;
	}

	if (portNbr == 0)
	{
		portNbr = DGRCLA_PORT_NBR;
	}

	if (dgr_open(getOwnNodeNbr(), 1, portNbr, hostNbr, NULL, &dgrSap, &rc)
	|| rc == DgrFailed)
	{
		putErrmsg("dgrcli can't open DGR service access point.", NULL);
		return 1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("dgrcli");
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.vduct = vinduct;
	rtp.running = &running;
	rtp.dgrSap = dgrSap;
	if (pthread_begin(&receiverThread, NULL, receiveBundles, &rtp, "dgrcli_receiver"))
	{
		dgr_close(dgrSap);
		putSysErrmsg("dgrcli can't create receiver thread", NULL);
		return 1;
	}

	writeMemo("[i] dgrcli is running.");

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the ducts.				*/

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	running = 0;

	/*	Shut down the receiver thread.				*/

	dgr_close(dgrSap);
	pthread_join(receiverThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] dgrcli induct ended.");
	bp_detach();
	return 0;
}
