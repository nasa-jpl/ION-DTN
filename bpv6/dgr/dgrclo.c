/*
	dgrclo.c:	ION DGR convergence-layer transmission daemon.
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
	ionKillMainThread("dgrclo");
}

/*	*	*	Sender thread functions	*	*	*	*/

typedef struct
{
	VOutduct	*vduct;
	int		*running;
	Dgr		dgrSap;
	unsigned short	portNbr;
	unsigned int	hostNbr;
} SenderThreadParms;

static void	*sendBundles(void *parm)
{
	/*	Main loop for single bundle transmission thread
	 *	serving all DGR destination inducts.			*/

	SenderThreadParms	*parms = (SenderThreadParms *) parm;
	char			*procName = "dgrclo";
	char			*buffer;
	Sdr			sdr;
	Outduct			outduct;
	int			threadRunning = 1;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	int			failedTransmissions = 0;
	ZcoReader		reader;
	int			bytesToSend;
	DgrRC			rc;

	snooze(1);	/*	Let main thread become interruptable.	*/
	buffer = MTAKE(DGRCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("dgrclo can't get DGR buffer.", NULL);
		*(parms->running) = 0;
		ionKillMainThread(procName);
		return NULL;
	}

	sdr = getIonsdr();
	CHKNULL(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			parms->vduct->outductElt), sizeof(Outduct));
	sdr_exit_xn(sdr);

	/*	Can now begin transmitting to clients.			*/

	while (threadRunning)
	{
		if (sm_SemEnded(parms->vduct->semaphore))
		{
			break;
		}

		if (bpDequeue(parms->vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)		/*	Outduct closed.	*/
		{
			writeMemo("[i] dgrclo outduct closed.");
			threadRunning = 0;
			continue;
		}

		if (bundleZco == 1)		/*	Corrupt bundle.	*/
		{
			continue;		/*	Get next one.	*/
		}

		CHKNULL(sdr_begin_xn(sdr));
		if (parms->hostNbr == 0)	/*	Can't send it.	*/
		{
			failedTransmissions++;
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy bundle ZCO.", NULL);
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
			if (dgr_send(parms->dgrSap, parms->portNbr,
					parms->hostNbr, DGR_NOTE_ALL,
					buffer, bytesToSend, &rc) < 0)
			{
				threadRunning = 0;
				putErrmsg("Crashed sending bundle.", NULL);
				continue;
			}
			else
			{
				if (rc == DgrFailed)
				{
					failedTransmissions++;
					if (bpHandleXmitFailure(bundleZco) < 0)
					{
						threadRunning = 0;
						putErrmsg("Crashed handling \
failure.", NULL);
					}
					else
					{
						sm_TaskYield();
					}

					continue;
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
	isprintf(buffer, DGRCLA_BUFSZ, "[i] dgrclo outduct ended.  %d \
transmissions failed.", failedTransmissions);
	writeMemo(buffer);
	MRELEASE(buffer);
	return NULL;
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	int		*running;
	Dgr		dgrSap;
} ReceiverThreadParms;

static void	*receiveSegments(void *parm)
{
	/*	Main loop for bundle reception thread via DGR.		*/

	Sdr			sdr = getIonsdr();
	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "dgrclo";
	char			*buffer;
	int			threadRunning = 1;
	DgrRC			rc;
	unsigned short		fromPortNbr;
	unsigned int		fromHostNbr;
	int			length;
	int			errnbr;
	Object			bundleZco;

	snooze(1);	/*	Let main thread become interruptable.	*/
	buffer = MTAKE(DGRCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("dgrclo can't get DGR buffer.", NULL);
		*(parms->running) = 0;
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Now start receiving segments.				*/

	while (threadRunning)
	{
		while (1)
		{
			if (dgr_receive(parms->dgrSap, &fromPortNbr,
					&fromHostNbr, buffer, &length, &errnbr,
					DGR_BLOCKING, &rc) < 0)
			{
				putErrmsg("Failed receiving segment.", NULL);
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

						sm_TaskYield();
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

					sm_TaskYield();
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

						sm_TaskYield();
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

					sm_TaskYield();
					continue;

				case DgrFailed:
					break;	/*	Out of switch.	*/

				default:
					sm_TaskYield();
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

				writeMemo("[?] dgrclo failed in bundle acq.");
			}

			threadRunning = 0;
		}
	}

	*(parms->running) = 0;
	ionKillMainThread(procName);

	/*	Finish releasing receiver thread's resources.		*/

	MRELEASE(buffer);
	writeErrmsgMemos();
	writeMemo("[i] dgrclo receiver thread stopping.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	dgrclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VOutduct		*voutduct;
	PsmAddress		vductElt;
	Dgr			dgrSap;
	DgrRC			rc;
	int			running = 1;
	SenderThreadParms	senderParms;
	ReceiverThreadParms	rtp;
	pthread_t		senderThread;
	pthread_t		receiverThread;

	if (ductName == NULL)
	{
		PUTS("Usage: dgrclo <remote host name>[:<port number>]");
		PUTS("[port number defaults to 1113]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("dgrclo can't attach to BP.", NULL);
		return 1;
	}

	findOutduct("dgr", ductName, &voutduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such dgr outduct.", ductName);
		return 1;
	}

	if (parseSocketSpec(ductName, &senderParms.portNbr,
			&senderParms.hostNbr) != 0)
	{
		putErrmsg("Can't get IP/port for remote host.", ductName);
		return 1;
	}

	if (senderParms.portNbr == 0)
	{
		senderParms.portNbr = DGRCLA_PORT_NBR;
	}

	/*	All command-line arguments are now validated.		*/

	if (dgr_open(getOwnNodeNbr(), 1, 0, 0, NULL, &dgrSap, &rc)
	|| rc == DgrFailed)
	{
		putErrmsg("dgrclo can't open DGR service access point.", NULL);
		return 1;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("dgrclo");
	isignal(SIGTERM, interruptThread);

	/*	Start the sender thread.				*/

	senderParms.vduct = voutduct;
	senderParms.running = &running;
	senderParms.dgrSap = dgrSap;
	if (pthread_begin(&senderThread, NULL, sendBundles, &senderParms, "dgrclo_sender"))
	{
		dgr_close(dgrSap);
		putSysErrmsg("dgrclo can't create sender thread", NULL);
		return 1;
	}

	/*	Start the receiver thread.				*/

	rtp.running = &running;
	rtp.dgrSap = dgrSap;
	if (pthread_begin(&receiverThread, NULL, receiveSegments, &rtp, "dgrclo_receiver"))
	{
		sm_SemEnd(voutduct->semaphore);
		pthread_join(senderThread, NULL);
		dgr_close(dgrSap);
		putSysErrmsg("dgrclo can't create receiver thread", NULL);
		return 1;
	}

	writeMemo("[i] dgrclo is running.");

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
	writeMemo("[i] dgrclo outduct ended.");
	bp_detach();
	return 0;
}
