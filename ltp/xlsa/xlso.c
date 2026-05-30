/*
	xlso.c:		Generic LTP link service OUTPUT daemon.

	Pulls outbound LTP segments from the engine and hands each one to
	the transport seam (xport_send).  This file is the part you should
	NOT need to change when retargeting to new hardware: the engine
	coupling (findSpan, ltpDequeueOutboundSegment, segSemaphore,
	shutdown) is identical to ltp/udp/udplso.c, just with the UDP
	socket / DNS / IPv6 / token-bucket machinery removed.  Rate
	control and impairments belong in the transport backend, not here.

	Patterned on ltp/udp/udplso.c from nasa-jpl/ion-dtn (integration).
									*/

#include "xlsa.h"

static sm_SemId	xlsoSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;

	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

static void	shutDownLso(int signum)		/*	Commands LSO stop.*/
{
	(void) signum;
	sm_SemEnd(xlsoSemaphore(NULL));
}

#if defined (ION_LWT)
int	xlso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*endpointSpec = (char *) a1;
	uvast	remoteEngineId = a3 != 0 ? getFqn((char *) a3) :
				(a2 != 0 ? getFqn((char *) a2) : 0);
#else
int	main(int argc, char *argv[])
{
	char	*endpointSpec = argc > 1 ? argv[1] : NULL;
	uvast	remoteEngineId = argc > 3 ? getFqn(argv[3]) :
				(argc > 2 ? getFqn(argv[2]) : 0);
#endif
	Sdr		sdr;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	XportLink	*link;
	int		segmentLength;
	char		*segment;
	int		bytesSent;

	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: xlso <transport endpoint spec> <remote engine ID>");
		PUTS("  e.g.: xlso shm:ring0 2");
		return 0;
	}

	/*	ltpadmin must have been run first to create the span.	*/

	if (ltpInit(0) < 0)
	{
		putErrmsg("xlso can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));		/*	Lock memory.	*/
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->lsoPid != ERROR && vspan->lsoPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("LSO task is already started for this span.",
				itoa(vspan->lsoPid));
		return 1;
	}

	sdr_exit_xn(sdr);

	/*	Open the transport.					*/

	link = xport_open(endpointSpec, 1, remoteEngineId);
	if (link == NULL)
	{
		putErrmsg("xlso can't open transport.", endpointSpec);
		return 1;
	}

	/*	Arm the segment semaphore so shutDownLso can release a
	 *	blocked ltpDequeueOutboundSegment().			*/

	oK(xlsoSemaphore(&(vspan->segSemaphore)));
	isignal(SIGTERM, shutDownLso);

	writeMemo("[i] xlso is running.");

	/*	Main transmission loop.  ltpDequeueOutboundSegment()
	 *	blocks on segSemaphore until ltpmeter posts a segment,
	 *	writes it into the engine-owned vspan->segmentBuffer, and
	 *	returns its length.  A return of 0 means "interrupted or
	 *	stopped"; we check the semaphore to decide which.	*/

	while (1)
	{
		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			break;			/*	Fatal.		*/
		}

		if (segmentLength == 0)		/*	Interrupted.	*/
		{
			if (sm_SemEnded(vspan->segSemaphore))
			{
				break;		/*	Told to stop.	*/
			}

			continue;
		}

		if (segmentLength > XLSA_BUFSZ)
		{
			putErrmsg("Segment too big for xlso transport.",
					itoa(segmentLength));
			break;
		}

		bytesSent = xport_send(link, segment, segmentLength);
		if (bytesSent < 0)
		{
			putErrmsg("xlso failed sending segment.", NULL);
			break;
		}

		/*	Let other ION tasks run.			*/

		sm_TaskYield();
	}

	/*	Shut down.						*/

	xport_close(link);
	writeErrmsgMemos();
	writeMemo("[i] xlso has ended.");
	ionDetach();
	return 0;
}
