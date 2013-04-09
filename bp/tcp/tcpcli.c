/*
	tcpcli.c:	BP TCP-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tcpcla.h"

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("tcpcli");
}

/*	*	*	Keepalive thread function	*	*	*/

typedef struct
{
	int		ductSocket;
	pthread_mutex_t	*mutex;
	struct sockaddr	socketName;
	int 		keepalivePeriod;
	int 		*cliRunning;
        int             *receiveRunning;
} KeepaliveThreadParms;

static void	terminateKeepaliveThread(KeepaliveThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] tcpcli keepalive thread stopping.");
	pthread_mutex_lock(parms->mutex);
	if (parms->ductSocket != -1)
	{
		closesocket(parms->ductSocket);
		parms->ductSocket = -1;
	}

	pthread_mutex_unlock(parms->mutex);
}

static void	*sendKeepalives(void *parm)
{
	KeepaliveThreadParms 	*parms = (KeepaliveThreadParms *) parm;
	char			*procName = "tcpcli";
	int 			count = 0;
	int 			bytesSent;
	unsigned char		*buffer;

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in tcpcli.", NULL);
		ionKillMainThread(procName);
		terminateKeepaliveThread(parms);
		return NULL;
	}
	
	while ( *(parms->cliRunning) && *(parms->receiveRunning) )
	{
		snooze(1);
		count++;
		if (count < parms->keepalivePeriod)
		{
			continue;
		}

		/*	Time to send a keepalive.  Note that the
		 *	interval between keepalive attempts will be
		 *	KEEPALIVE_PERIOD plus (if the remote induct
		 *	is not reachable) the length of time taken
		 *	by TCP to determine that the connection
		 *	attempt will not succeed (e.g., 3 seconds).	*/

		count = 0;
		pthread_mutex_lock(parms->mutex);
		if (parms->ductSocket < 0 || 
                    !((*(parms->cliRunning)) &&  *(parms->receiveRunning)))
		{
			*(parms->receiveRunning) = 0;
			pthread_mutex_unlock(parms->mutex);
			continue;
		}

		bytesSent = sendBundleByTCPCL(&parms->socketName,
				&parms->ductSocket, 0, 0, buffer,
				&parms->keepalivePeriod);
                pthread_mutex_unlock(parms->mutex);
                if (bytesSent < 0)
		{
			*(parms->receiveRunning) = 0;
			ionKillMainThread(procName);
			continue;
		}

		sm_TaskYield();
	}

	MRELEASE(buffer);
	terminateKeepaliveThread(parms);
	return NULL;
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	char		senderEidBuffer[SDRSTRING_BUFSZ];
	char		*senderEid;
	VInduct		*vduct;
	LystElt		elt;
	struct sockaddr	cloSocketName;
	pthread_mutex_t	*mutex;
	int		bundleSocket;
	pthread_t	thread;
	int		*cliRunning;
        int             receiveRunning;
} ReceiverThreadParms;

static void	terminateReceiverThread(ReceiverThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] tcpcli receiver thread stopping.");
	pthread_mutex_lock(parms->mutex);
	if (parms->bundleSocket != -1)
	{
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
	}

	lyst_delete(parms->elt);
	pthread_mutex_unlock(parms->mutex);
}

static void	*receiveBundles(void *parm)
{
	/*	Main loop for bundle reception thread on one
	 *	connection, terminating when connection is lost.	*/

	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "tcpcli";
	KeepaliveThreadParms	*kparms;
	AcqWorkArea		*work;
	char			*buffer;
	pthread_t		kthread;
	int			haveKthread = 0;
	int			threadRunning = 1;

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("tcpcli can't get TCP buffer", NULL);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}

	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("tcpcli can't get acquisition work area", NULL);
		MRELEASE(buffer);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}
	
	/*	Set up parameters for the keepalive thread	*/
	kparms = (KeepaliveThreadParms *)
			MTAKE(sizeof(KeepaliveThreadParms));
	if (kparms == NULL)
	{
		putErrmsg("tcpcli can't allocate for new keepalive thread",
				NULL);
		MRELEASE(buffer);
		bpReleaseAcqArea(work);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}

	pthread_mutex_lock(parms->mutex);

	/*	Making sure there is no race condition when keep alive
	 *	values are set.						*/

	if (sendContactHeader(&parms->bundleSocket, (unsigned char *) buffer,
			NULL) < 0)
	{
		putErrmsg("tcpcli couldn't send contact header", NULL);
		MRELEASE(buffer);
		MRELEASE(kparms);
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
		pthread_mutex_unlock(parms->mutex);
		bpReleaseAcqArea(work);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		return NULL;
	}

	pthread_mutex_unlock(parms->mutex);
	if (receiveContactHeader(&parms->bundleSocket, (unsigned char *) buffer,
			&kparms->keepalivePeriod) < 0)
	{
		putErrmsg("tcpcli couldn't receive contact header", NULL);
		MRELEASE(buffer);
		MRELEASE(kparms);
		pthread_mutex_lock(parms->mutex);
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
		pthread_mutex_unlock(parms->mutex);
		bpReleaseAcqArea(work);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		return NULL;
	}

	/*
	 * The keep alive thread is created only if the negotiated
	 * keep alive is greater than 0.
	 */

	if( kparms->keepalivePeriod > 0 )
	{
		kparms->ductSocket = parms->bundleSocket;
		kparms->socketName = parms->cloSocketName;
		kparms->mutex = parms->mutex;
		kparms->cliRunning = parms->cliRunning;
                kparms->receiveRunning = &(parms->receiveRunning);
		/*
		 * Creating a thread to send out keep alives, which
		 * makes the TCPCL bi directional
		 */
		if (pthread_begin(&kthread, NULL, sendKeepalives, kparms))
		{
			putSysErrmsg("tcpcli can't create new thread for \
keepalives", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
		}
		else
		{
			haveKthread = 1;
		}
	}

	/*	Now start receiving bundles.				*/

	while (threadRunning && *(parms->cliRunning) && parms->receiveRunning)
	{
		if (bpBeginAcq(work, 0, parms->senderEid) < 0)
		{
			putErrmsg("Can't begin acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
			continue;
		}

		switch (receiveBundleByTcpCL(parms->bundleSocket, work, buffer))
		{
		case -1:
			putErrmsg("Can't acquire bundle.", NULL);

			/*	Intentional fall-through to next case.	*/

		case 0:				/*	Normal stop.	*/
			threadRunning = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		if (bpEndAcq(work) < 0)
		{
			putErrmsg("Can't end acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	End of receiver thread; release resources.		*/

	if (haveKthread)
	{
		/*	Wait for keepalive snooze cycle to notice that
		 *	*(parms->cliRunning) is now zero.		*/

		pthread_join(kthread, NULL);
	}

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	terminateReceiverThread(parms);
	MRELEASE(kparms);
	MRELEASE(parms);
	return NULL;
}

/*	*	*	Access thread functions	*	*	*	*/

typedef struct
{
	VInduct			*vduct;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			ductSocket;
	int			running;
} AccessThreadParms;

static void	*spawnReceivers(void *parm)
{
	/*	Main loop for acceptance of connections and
	 *	creation of receivers to service those connections.	*/

	AccessThreadParms	*atp = (AccessThreadParms *) parm;
	char			*procName = "tcpcli";
	pthread_mutex_t		mutex;
	Lyst			threads;
	int			newSocket;
	struct sockaddr		cloSocketName;
	socklen_t		nameLength;
	ReceiverThreadParms	*parms;
	LystElt			elt;
	struct sockaddr_in	*fromAddr;
	unsigned int		hostNbr;
	char			hostName[MAXHOSTNAMELEN + 1];
	pthread_t		thread;

	snooze(1);	/*	Let main thread become interruptable.	*/
	pthread_mutex_init(&mutex, NULL);
	threads = lyst_create_using(getIonMemoryMgr());
	if (threads == NULL)
	{
		putErrmsg("tcpcli can't create threads list", NULL);
		pthread_mutex_destroy(&mutex);
		ionKillMainThread(procName);
		return NULL;
	}

	/*	Can now begin accepting connections from remote
	 *	contacts.  On failure, take down the whole CLI.		*/

	while (atp->running)
	{
		nameLength = sizeof(struct sockaddr);
		newSocket = accept(atp->ductSocket, &cloSocketName,
				&nameLength);
		if (newSocket < 0)
		{
			putSysErrmsg("tcpcli accept() failed", NULL);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		if (atp->running == 0)
		{
			break;	/*	Main thread has shut down.	*/
		}

		parms = (ReceiverThreadParms *)
				MTAKE(sizeof(ReceiverThreadParms));
		if (parms == NULL)
		{
			putErrmsg("tcpcli can't allocate for new thread", NULL);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		parms->vduct = atp->vduct;
		pthread_mutex_lock(&mutex);
		parms->elt = lyst_insert_last(threads, parms);
		pthread_mutex_unlock(&mutex);
		if (parms->elt == NULL)
		{
			putErrmsg("tcpcli can't allocate lyst element for new \
thread", NULL);
			MRELEASE(parms);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		parms->mutex = &mutex;
		parms->bundleSocket = newSocket;
		fromAddr = (struct sockaddr_in *) &cloSocketName;
		memcpy((char *) &hostNbr,
				(char *) &(fromAddr->sin_addr.s_addr), 4);
		hostNbr = ntohl(hostNbr);
		printDottedString(hostNbr, hostName);
		parms->senderEid = parms->senderEidBuffer;
		getSenderEid(&(parms->senderEid), hostName);
		parms->cloSocketName = cloSocketName;
		parms->cliRunning = &(atp->running);
                parms->receiveRunning = 1;
		if (pthread_begin(&(parms->thread), NULL, receiveBundles,
					parms))
		{
			putSysErrmsg("tcpcli can't create new thread", NULL);
			MRELEASE(parms);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	closesocket(atp->ductSocket);
	writeErrmsgMemos();

	/*	Shut down all current CLI threads cleanly.		*/

	while (1)
	{
		pthread_mutex_lock(&mutex);
		elt = lyst_first(threads);
		if (elt == NULL)	/*	All threads shut down.	*/
		{
			pthread_mutex_unlock(&mutex);
			break;
		}

		/*	Trigger termination of thread.			*/

		parms = (ReceiverThreadParms *) lyst_data(elt);
		thread = parms->thread;
		if (sendShutDownMessage(&parms->bundleSocket, SHUT_DN_NO, -1,
				NULL) < 0)
		{
			putErrmsg("Sending Shutdown message failed!!",NULL);
		}

		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
		pthread_kill(thread, SIGTERM);
		pthread_mutex_unlock(&mutex);
		pthread_join(thread, NULL);
		MRELEASE(parms);
	}

	lyst_destroy(threads);
	writeErrmsgMemos();
	writeMemo("[i] tcpcli access thread has ended.");
	pthread_mutex_destroy(&mutex);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	tcpcli(int a1, int a2, int a3, int a4, int a5,
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
	Sdr			sdr;
	Induct			duct;
	ClProtocol		protocol;
	char			*hostName;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	AccessThreadParms	atp;
	socklen_t		nameLength;
	char			*tcpDelayString;
	pthread_t		accessThread;
	int			fd;

	if (ductName == NULL)
	{
		PUTS("Usage: tcpcli <local host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("tcpcli can't attach to BP.", NULL);
		return 1;
	}

	findInduct("tcp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such tcp duct.", ductName);
		return 1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	if (protocol.nominalRate == 0)
	{
		vduct->acqThrottle.nominalRate = DEFAULT_TCP_RATE;
	}
	else
	{
		vduct->acqThrottle.nominalRate = protocol.nominalRate;
	}

	hostName = ductName;
	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		putErrmsg("Can't get IP/port for host.", hostName);
		return 1;
	}

	if (portNbr == 0)
	{
		portNbr = BpTcpDefaultPortNbr;
	}

	portNbr = htons(portNbr);
	hostNbr = htonl(hostNbr);
	atp.vduct = vduct;
	memset((char *) &(atp.socketName), 0, sizeof(struct sockaddr));
	atp.inetName = (struct sockaddr_in *) &(atp.socketName);
	atp.inetName->sin_family = AF_INET;
	atp.inetName->sin_port = portNbr;
	memcpy((char *) &(atp.inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	atp.ductSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	/* set desired keep alive period to 15 and since there is no negotiated
	 * keep alive period it is 0	*/

	tcpDesiredKeepAlivePeriod = KEEPALIVE_PERIOD;

	if (atp.ductSocket < 0)
	{
		putSysErrmsg("Can't open TCP socket", NULL);
		return 1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(atp.ductSocket)
	|| bind(atp.ductSocket, &(atp.socketName), nameLength) < 0
	|| listen(atp.ductSocket, 5) < 0
	|| getsockname(atp.ductSocket, &(atp.socketName), &nameLength) < 0)
	{
		closesocket(atp.ductSocket);
		putSysErrmsg("Can't initialize socket", NULL);
		return 1;
	}

	tcpDelayString = getenv("TCP_DELAY_NSEC_PER_BYTE");
	if (tcpDelayString == NULL)
	{
		tcpDelayEnabled = 0;
	}
	else
	{
		tcpDelayEnabled = 1;
		tcpDelayNsecPerByte = strtol(tcpDelayString, NULL, 0);
		if (tcpDelayNsecPerByte < 0
		|| tcpDelayNsecPerByte > 16384)
		{
			tcpDelayNsecPerByte = 0;
		}
	}

	/*	Initialize sender endpoint ID lookup.			*/

	ipnInit();
	dtn2Init();

	/*	Set up signal handling: SIGTERM is shutdown signal.	*/

	ionNoteMainThread("tcpcli");
	isignal(SIGTERM, interruptThread);
#ifndef mingw
	isignal(SIGPIPE, SIG_IGN);
#endif

	/*	Start the access thread.				*/

	atp.running = 1;
	if (pthread_begin(&accessThread, NULL, spawnReceivers, &atp))
	{
		closesocket(atp.ductSocket);
		putSysErrmsg("tcpcli can't create access thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	{
		char txt[500];

		isprintf(txt, sizeof(txt),
				"[i] tcpcli is running, spec=[%s:%d].", 
				inet_ntoa(atp.inetName->sin_addr),
				ntohs(atp.inetName->sin_port));

		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	atp.running = 0;

	/*	Wake up the access thread by connecting to it.		*/

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd >= 0)
	{
		connect(fd, &(atp.socketName), sizeof(struct sockaddr));

		/*	Immediately discard the connected socket.	*/

		closesocket(fd);
	}

	pthread_join(accessThread, NULL);
	writeErrmsgMemos();
	writeMemo("[i] tcpcli duct has ended.");
	bp_detach();
	return 0;
}
