/*
        udpcli.c:	BP UDP-based convergence-layer input
                        daemon, designed to serve as an input
                        duct.

        Author: Ted Piotrowski, APL
                Scott Burleigh, JPL

        Copyright (c) 2006, California Institute of Technology.
        ALL RIGHTS RESERVED.  U.S. Government Sponsorship
        acknowledged.

                                                                        */
#include "dtn2fw.h"
#include "ipnfw.h"
#include "udpcla.h"

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct     *vduct;
	UdpClaSocket claSock; /* Dual-stack socket with shutdown support */
	int          running;
} ReceiverThreadParms;

static void *handleDatagrams(void *parm)
{
	/*	Main loop for UDP datagram reception and handling.	*/

	ReceiverThreadParms *rtp = (ReceiverThreadParms *) parm;
	char                *procName = "udpcli";
	AcqWorkArea         *work;
	char                *buffer;
	int                  bundleLength;
	IonNetworkAddress    fromAddr;    /* Dual-stack address */
	int                  is_shutdown; /* Shutdown detection */

	snooze(1); /*	Let main thread become interruptible.	*/
	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		putErrmsg("udpcli can't get acquisition work area.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	buffer = MTAKE(UDPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("udpcli can't get UDP buffer.", NULL);
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Can now start receiving bundles.  On failure, take
	 *	down the CLI.						*/

	while (rtp->running)
	{
		/* Use dual-stack receive with automatic shutdown detection */
		bundleLength = receiveUdpClaDatagram(&rtp->claSock, buffer,
				UDPCLA_BUFSZ, &fromAddr, &is_shutdown);

		/* Handle shutdown signal */
		if (is_shutdown)
		{
			writeMemo("[i] udpcli received shutdown signal");
			rtp->running = 0;
			continue;
		}

		/* Handle receive errors */
		switch (bundleLength)
		{
		case -1:
		case 0:
			putErrmsg("Can't acquire bundle.", NULL);
			ionKillMainThread(procName);

			/* FALLTHROUGH */

		case 1: /*	Normal stop.	*/
			rtp->running = 0;
			continue;

		default:
			break; /*	Out of switch.	*/
		}

		if (bpBeginAcq(work, 0, NULL) < 0
				|| bpContinueAcq(work, buffer, bundleLength, 0, 0)
						< 0
				|| bpEndAcq(work) < 0)
		{
			putErrmsg("Can't acquire bundle.", NULL);
			ionKillMainThread(procName);
			rtp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] udpcli receiver thread has ended.");

	/*	Free resources.						*/

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	return NULL;
}

/* Global reference for signal-based shutdown */
static UdpClaSocket *global_cla_socket = NULL;

static void interruptThread(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	/* Send shutdown signal instead of just setting flags */
	if (global_cla_socket != NULL)
	{
		sendUdpClaShutdown(global_cla_socket);
	}

	isignal(SIGTERM, interruptThread);
	ionKillMainThread("udpcli");
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined(ION_LWT)
int udpcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5, saddr a6, saddr a7,
		saddr a8, saddr a9, saddr a10)
{
	char *ductName = (char *) a1;
#else
int main(int argc, char *argv[])
{
	char *ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VInduct            *vduct;
	PsmAddress          vductElt;
	Sdr                 sdr;
	Induct              duct;
	ClProtocol          protocol;
	ReceiverThreadParms rtp;
	pthread_t           receiverThread;
	char                localAddrStr[INET6_ADDR_WITH_PORT_STRLEN];

	if (ductName == NULL)
	{
		PUTS("Usage: udpcli <local host name>[:<port number>]");
		PUTS("  IPv4: udpcli 192.168.1.1:4556");
		PUTS("  IPv6: udpcli [::1]:4556 or udpcli ::1");
		PUTS("  Any:  udpcli 0.0.0.0:4556 or udpcli [::]:4556");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("udpcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("udp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such udp duct.", ductName);
		return -1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/* Initialize dual-stack socket with automatic IPv4/IPv6 handling */
	if (initUdpClaSocket(ductName, &rtp.claSock) < 0)
	{
		putErrmsg("Can't initialize UDP CLA socket", ductName);
		return -1;
	}

	/* Log what we're listening on */
	formatNetworkAddress(&rtp.claSock.local_addr, localAddrStr,
			sizeof(localAddrStr));
	{
		char memo[256];
		snprintf(memo, sizeof(memo), "[i] udpcli listening on %s (%s)",
				localAddrStr,
				(rtp.claSock.local_addr.family == AF_INET6) ?
						"IPv6" :
						"IPv4");
		writeMemo(memo);
	}

	rtp.vduct = vduct;

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	ionNoteMainThread("udpcli");
	/* Register socket for signal-based shutdown */
	global_cla_socket = &rtp.claSock;
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.running = 1;
	if (pthread_begin(&receiverThread, NULL, handleDatagrams, &rtp))
	{
		cleanupUdpClaSocket(&rtp.claSock);
		putSysErrmsg("udpcli can't create receiver thread", NULL);
		return -1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	{
		char txt[500];

		char addrStr[INET6_ADDR_WITH_PORT_STRLEN];
		formatNetworkAddress(&rtp.claSock.local_addr, addrStr,
				sizeof(addrStr));
		isprintf(txt, sizeof(txt), "[i] udpcli is running on %s (%s).",
				addrStr,
				(rtp.claSock.local_addr.family == AF_INET6) ?
						"IPv6" :
						"IPv4");
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	writeMemo("[i] udpcli shutting down...");
	rtp.running = 0;
	/* Unregister socket */
	global_cla_socket = NULL;

	/*	Send shutdown signal to wake up receiver thread cleanly	*/
	if (sendUdpClaShutdown(&rtp.claSock) < 0)
	{
		writeMemo("[!] Failed to send shutdown signal, forcing thread exit");
		pthread_cancel(receiverThread);
	}
	else
	{
		writeMemo("[i] Waiting for receiver thread to exit cleanly");
		pthread_join(receiverThread, NULL);
	}

	/*	Clean up dual-stack socket	*/
	cleanupUdpClaSocket(&rtp.claSock);

	writeErrmsgMemos();
	writeMemo("[i] udpcli duct has ended.");
	ionDetach();
	return 0;
}
