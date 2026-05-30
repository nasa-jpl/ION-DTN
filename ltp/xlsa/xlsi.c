/*
	xlsi.c:		Generic LTP link service INPUT daemon.

	Spawns the receiver thread (xlsa_handle_datagrams, in libxlsa.c)
	which loops on xport_recv() -> ltpHandleInboundSegment().  As with
	xlso.c, the engine coupling (findSeat, vseat/lsiPid checks,
	shutdown signalling) mirrors ltp/udp/udplsi.c with the socket
	machinery removed.

	Patterned on ltp/udp/udplsi.c from nasa-jpl/ion-dtn (integration).
									*/

#include "xlsa.h"

static void	interruptThread(int signum)
{
	(void) signum;
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("xlsi");
}

#if defined (ION_LWT)
int	xlsi(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*endpointSpec = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*endpointSpec = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr				sdr;
	char				lsiCmd[256];
	LtpVseat			*vseat;
	PsmAddress			vseatElt;
	static xlsa_ReceiverThreadParms	rtp;	/*	Off main stack.	*/
	pthread_t			receiverThread;

	if (endpointSpec == NULL)
	{
		PUTS("Usage: xlsi <transport endpoint spec>");
		PUTS("  e.g.: xlsi shm:ring0");
		return 0;
	}

	if (ltpInit(0) < 0)
	{
		putErrmsg("xlsi can't initialize LTP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	isprintf(lsiCmd, sizeof lsiCmd, "xlsi %s", endpointSpec);
	CHKERR(sdr_begin_xn(sdr));
	findSeat(lsiCmd, &vseat, &vseatElt);
	sdr_exit_xn(sdr);
	if (vseatElt == 0)
	{
		writeMemoNote("[?] Undefined LSI", lsiCmd);
		return 1;
	}

	if (vseat->lsiPid != ERROR && vseat->lsiPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] LSI task is already started",
				itoa(vseat->lsiPid));
		return 1;
	}

	/*	Set up receiver thread parameters.			*/

	pthread_mutex_init(&rtp.lock, NULL);

	rtp.link = xport_open(endpointSpec, 0, 0);
	if (rtp.link == NULL)
	{
		putErrmsg("xlsi can't open transport.", endpointSpec);
		return 1;
	}

	/*	Register the main thread BEFORE arming SIGTERM, so the
	 *	signal handler's ionKillMainThread() lookup succeeds.
	 *	Without this, ionPauseMainThread() never returns: the
	 *	handler can't find a registered thread to signal, and the
	 *	process hangs until SIGKILL.  Mirrors udplsi.c.		*/

	ionNoteMainThread("xlsi");
	isignal(SIGTERM, interruptThread);
	ion_atomic_init(&rtp.running, 1);

	if (pthread_begin(&receiverThread, NULL, xlsa_handle_datagrams,
			&rtp, "xlsi_receiver"))
	{
		xport_close(rtp.link);
		putSysErrmsg("xlsi can't create receiver thread.", NULL);
		return 1;
	}

	writeMemo("[i] xlsi is running.");

	/*	Now sleep until interrupted, then wake the receiver and
	 *	join it.  ionPauseMainThread blocks until ionKillMainThread
	 *	is called from the SIGTERM handler.			*/

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	ion_atomic_set(&rtp.running, 0);
	xport_wake(rtp.link);		/*	Unblock xport_recv().	*/
	pthread_join(receiverThread, NULL);
	xport_close(rtp.link);
	ion_atomic_mutex_destroy(&rtp.running);

	writeErrmsgMemos();
	writeMemo("[i] xlsi has ended.");
	ionDetach();
	return 0;
}
