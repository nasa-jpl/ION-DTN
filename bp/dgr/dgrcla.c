/*
	dgrcla.c:	ION DGR convergence-layer server daemon.
			Handles both transmission and reception.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"
#include "dgr.h"
#include "ipnfw.h"
#include "dtn2fw.h"

#define	DGRCLA_PORT_NBR		1113
#define	DGRCLA_BUFSZ		65535
#define	DEFAULT_DGR_RATE	125000000

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("dgrcla");
}

/*	*	*	Sender thread functions	*	*	*	*/

typedef struct
{
	VOutduct	*vduct;
	int		*running;
	Dgr		dgrSap;
} SenderThreadParms;

static void	*sendBundles(void *parm)
{
	/*	Main loop for single bundle transmission thread
	 *	serving all DGR destination inducts.			*/

	SenderThreadParms	*parms = (SenderThreadParms *) parm;
	char			*procName = "dgrcla";
	char			*buffer;
	Sdr			sdr;
	Outduct			outduct;
	Outflow			outflows[3];
	int			i;
	int			threadRunning = 1;
	Object			bundleZco;
	BpExtendedCOS		extendedCOS;
	char			destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	unsigned short		portNbr;
	unsigned int		hostNbr;
	int			failedTransmissions = 0;
	ZcoReader		reader;
	int			bytesToSend;
	DgrRC			rc;

	snooze(1);	/*	Let main thread become interruptable.	*/
	buffer = MTAKE(DGRCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("dgrcla can't get DGR buffer.", NULL);
		*(parms->running) = 0;
		ionKillMainThread(procName);
		return NULL;
	}

	sdr = getIonsdr();
	CHKNULL(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			parms->vduct->outductElt), sizeof(Outduct));
	sdr_exit_xn(sdr);
	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = outduct.bulkQueue;
	outflows[1].outboundBundles = outduct.stdQueue;
	outflows[2].outboundBundles = outduct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	/*	Can now begin transmitting to clients.			*/

	while (threadRunning)
	{
		if (sm_SemEnded(parms->vduct->semaphore))
		{
			break;
		}

		if (bpDequeue(parms->vduct, outflows, &bundleZco,
				&extendedCOS, destDuctName, 64000, -1) < 0)
		{
			threadRunning = 0;
			writeMemo("[?] dgrcla failed de-queueing bundle.");
			continue;
		}

		if (bundleZco == 0)		/*	Interrupted.	*/
		{
			continue;
		}

		parseSocketSpec(destDuctName, &portNbr, &hostNbr);
		if (portNbr == 0)
		{
			portNbr = DGRCLA_PORT_NBR;
		}

		CHKNULL(sdr_begin_xn(sdr));
		if (hostNbr == 0)		/*	Can't send it.	*/
		{
			failedTransmissions++;
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy ZCO reference.", NULL);
				threadRunning = 0;
			}

			continue;
		}

		zco_start_transmitting(bundleZco, &reader);
		zco_track_file_offset(&reader);
		bytesToSend = zco_transmit(sdr, &reader, DGRCLA_BUFSZ, buffer);
		oK(sdr_end_xn(sdr));
		if (bytesToSend < 0)
		{
			threadRunning = 0;
			putErrmsg("Can't issue from ZCO.", NULL);
			continue;
		}

		/*	Concatenated bundle is now in buffer.  Send it.	*/

		if (bytesToSend > 0)
		{
			if (dgr_send(parms->dgrSap, portNbr, hostNbr,
				DGR_NOTE_ALL, buffer, bytesToSend, &rc) < 0)
			{
				threadRunning = 0;
				putErrmsg("Crashed sending bundle.", NULL);
			}
			else
			{
				if (rc == DgrFailed)
				{
					failedTransmissions++;
					if (bpHandleXmitFailure(bundleZco))
					{
						threadRunning = 0;
						putErrmsg("Crashed handling \
failure.", NULL);
					}
				}
			}
		}

		CHKNULL(sdr_begin_xn(sdr));
		zco_destroy(sdr, bundleZco);
		if (sdr_end_xn(sdr) < 0)
		{
			threadRunning = 0;
			putErrmsg("Failed destroying bundle ZCO.", NULL);
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	*(parms->running) = 0;
	ionKillMainThread(procName);
	writeErrmsgMemos();
	isprintf(buffer, DGRCLA_BUFSZ, "[i] dgrcla outduct ended.  %d \
transmissions failed.", failedTransmissions);
	writeMemo(buffer);
	MRELEASE(buffer);
	return NULL;
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
	char			*procName = "dgrcla";
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
	char			senderEidBuffer[SDRSTRING_BUFSZ];
	char			*senderEid;

	snooze(1);	/*	Let main thread become interruptable.	*/
	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("dgrcla can't get acquisition work area.", NULL);
		*(parms->running) = 0;
		ionKillMainThread(procName);
		return NULL;
	}

	buffer = MTAKE(DGRCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("dgrcla can't get DGR buffer.", NULL);
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

			switch (rc)
			{
				case DgrDatagramAcknowledged:
					CHKNULL(sdr_begin_xn(sdr));
					bundleZco = zco_create(sdr,
						ZcoSdrSource, sdr_insert(sdr,
						buffer, length), 0, length);
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

					if (bpHandleXmitSuccess(bundleZco, 0))
					{
						threadRunning = 0;
						putErrmsg("Crashed handling \
success.", NULL);
					}

					CHKNULL(sdr_begin_xn(sdr));
					zco_destroy(sdr, bundleZco);
					if (sdr_end_xn(sdr) < 0)
					{
						threadRunning = 0;
						putErrmsg("Failed destroying \
bundle ZCO.", NULL);
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
						buffer, length), 0, length);
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

					if (bpHandleXmitFailure(bundleZco))
					{
						threadRunning = 0;
						putErrmsg("Crashed handling \
failure.", NULL);
					}

					CHKNULL(sdr_begin_xn(sdr));
					zco_destroy(sdr, bundleZco);
					if (sdr_end_xn(sdr) < 0)
					{
						threadRunning = 0;
						putErrmsg("Failed destroying \
bundle ZCO.", NULL);
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

				writeMemo("[?] dgrcla failed in bundle acq.");
			}

			threadRunning = 0;
			continue;
		}

		/*	Must have received a datagram.			*/

		printDottedString(fromHostNbr, hostName);
		senderEid = senderEidBuffer;
		getSenderEid(&senderEid, hostName);
		if (bpBeginAcq(work, 0, senderEid) < 0
		|| bpContinueAcq(work, buffer, length) < 0
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
	writeMemo("[i] dgrcla receiver thread stopping.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	dgrcla(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct			*vinduct;
	PsmAddress		vductElt;
	VOutduct		*voutduct;
	Sdr			sdr;
	Induct			induct;
	ClProtocol		protocol;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	Dgr			dgrSap;
	DgrRC			rc;
	int			running = 1;
	SenderThreadParms	senderParms;
	ReceiverThreadParms	rtp;
	pthread_t		senderThread;
	pthread_t		receiverThread;

	if (ductName == NULL)
	{
		PUTS("Usage: dgrcla <local host name>[:<port number>]");
		PUTS("[port number defaults to 1113]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("dgrcla can't attach to BP.", NULL);
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

	findOutduct("dgr", "*", &voutduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such dgr outduct.", ductName);
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr, vinduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, induct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	if (protocol.nominalRate == 0)
	{
		vinduct->acqThrottle.nominalRate = DEFAULT_DGR_RATE;
		voutduct->xmitThrottle.nominalRate = DEFAULT_DGR_RATE;
	}
	else
	{
		vinduct->acqThrottle.nominalRate = protocol.nominalRate;
		voutduct->xmitThrottle.nominalRate = protocol.nominalRate;
	}

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
		putErrmsg("dgrcla can't open DGR service access point.", NULL);
		return 1;
	}

	/*	Initialize sender endpoint ID lookup.			*/

	ipnInit();
	dtn2Init();

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("dgrcla");
	isignal(SIGTERM, interruptThread);

	/*	Start the sender thread; a single sender for all
	 *	destinations.						*/

	senderParms.vduct = voutduct;
	senderParms.running = &running;
	senderParms.dgrSap = dgrSap;
	if (pthread_begin(&senderThread, NULL, sendBundles, &senderParms))
	{
		dgr_close(dgrSap);
		putSysErrmsg("dgrcla can't create sender thread", NULL);
		return 1;
	}

	/*	Start the receiver thread.				*/

	rtp.vduct = vinduct;
	rtp.running = &running;
	rtp.dgrSap = dgrSap;
	if (pthread_begin(&receiverThread, NULL, receiveBundles, &rtp))
	{
		sm_SemEnd(voutduct->semaphore);
		pthread_join(senderThread, NULL);
		dgr_close(dgrSap);
		putSysErrmsg("dgrcla can't create receiver thread", NULL);
		return 1;
	}

	writeMemo("[i] dgrcla is running.");

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the ducts.				*/

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	running = 0;

	/*	Shut down the sender thread.				*/

	if (voutduct->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(voutduct->semaphore);
	}

	pthread_join(senderThread, NULL);

	/*	Shut down the receiver thread.				*/

	dgr_close(dgrSap);
	pthread_join(receiverThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] dgrcla induct ended.");
	bp_detach();
	return 0;
}
