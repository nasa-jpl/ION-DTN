/*
	brsscla.c:	BP Bundle Relay Service convergence-layer
			server daemon.  Handles both transmission
			and reception.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "brscla.h"

static ReqAttendant	*_attendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

#ifndef mingw
static void	handleStopThread(int signum)
{
	isignal(SIGINT, handleStopThread);
}
#endif
static void	handleStopBrsscla(int signum)
{
	isignal(SIGTERM, handleStopBrsscla);
	ionKillMainThread("brsscla");
}

typedef struct
{
	char		outductName[32];
	VOutduct	*vduct;
	char		eid[MAX_EID_LEN];
} SenderThreadParms;

typedef struct
{
	VInduct			*vduct;
	LystElt			elt;
	pthread_mutex_t		*mutex;
	int			bundleSocket;
	pthread_t		receiverThread;
	int			*running;
	unsigned int		nodeNbr;
	int			*authenticated;
	pthread_t		senderThread;
	int			hasSender;
	SenderThreadParms	stp;
} ReceiverThreadParms;

/*	*	*	Sender thread functions	*	*	*	*/

static void	*terminateSenderThread()
{
	writeErrmsgMemos();
	writeMemo("[i] brsscla sender thread stopping.");
	return NULL;
}

static void	*sendBundles(void *parm)
{
	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "brsscla";
	char			*buffer;
	Sdr			sdr;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			bytesSent;

	parms->hasSender = 1;
	snooze(1);	/*	Let main thread become interruptable.	*/
	buffer = MTAKE(STCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in brsscla.", NULL);
		ionKillMainThread(procName);
		return terminateSenderThread();
	}

	sdr = getIonsdr();

	/*	Can now begin transmitting to clients.			*/

	while (!(sm_SemEnded(parms->stp.vduct->semaphore)))
	{
		if (bpDequeue(parms->stp.vduct, &bundleZco, &ancillaryData, -1)
				< 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			ionKillMainThread(procName);
			break;
		}

		if (bundleZco == 0)		/*	Outduct closed.	*/
		{
			writeMemo("[i] brsscla outduct closed.");
			continue;
		}

		if (bundleZco == 1)		/*	Corrupt bundle.	*/
		{
			continue;		/*	Get next one.	*/
		}

		CHKNULL(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		bytesSent = sendBundleByStcp("", "", &(parms->bundleSocket),
				bundleLength, bundleZco, buffer);

		/*	Note that TCP I/O errors never block the
		 *	brsscla induct's output functions; those
		 *	functions never connect to remote sockets
		 *	and the outduct never behaves like a TCP
		 *	outduct, so the _tcpOutductId table is never
		 *	populated.					*/

		if (bytesSent < 0)
		{
			putErrmsg("Can't send bundle.", NULL);
			ionKillMainThread(procName);
			break;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeMemo("[i] brsscla outduct has ended.");
	MRELEASE(buffer);
	return terminateSenderThread();
}

/*	*	*	Receiver thread functions	*	*	*/

static int	reforwardStrandedBundles()
{
	Sdr	sdr = getIonsdr();
	BpDB	*bpConstants = getBpConstants();
	Object	elt;
	Object	nextElt;

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, bpConstants->limboQueue); elt;
			elt = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt);
		if (releaseFromLimbo(elt, 0) < 0)
		{
			putErrmsg("Failed releasing bundle from limbo.", NULL);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("brss failed limbo release on client connect.", NULL);
		return -1;
	}

	return 0;
}

static void	terminateReceiverThread(ReceiverThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] brsscla receiver thread stopping.");
	pthread_mutex_lock(parms->mutex);
	if (parms->bundleSocket != -1)
	{
		closesocket(parms->bundleSocket);
		parms->bundleSocket = -1;
	}

	lyst_delete(parms->elt);
	pthread_mutex_unlock(parms->mutex);
	MRELEASE(parms);
}

static int	startSendingThread(ReceiverThreadParms *rtp)
{
	PsmAddress	vductElt;
	VPlan		*vplan;
	PsmAddress	vplanElt;

	/*	Create automatic Outduct for this socket.		*/

	isprintf(rtp->stp.outductName, sizeof rtp->stp.outductName,
			"#:%d", rtp->bundleSocket);
	if (addOutduct("brss", rtp->stp.outductName, "", 0) < 0)
	{
		putErrmsg("Can't add automatic outduct.",
				rtp->stp.outductName);
		return -1;
	}

	findOutduct("brss", rtp->stp.outductName, &(rtp->stp.vduct), &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("Can't find brsscla automatic outduct.",
				rtp->stp.outductName);
		return -1;
	}

	/*	Attach automatic Outduct to egress plan.		*/

	isprintf(rtp->stp.eid, sizeof rtp->stp.eid,
			"ipn:" UVAST_FIELDSPEC ".0", rtp->nodeNbr);
	findPlan(rtp->stp.eid, &vplan, &vplanElt);
	if (vplanElt == 0)	/*	Client initial connection.	*/
	{
		if (addPlan(rtp->stp.eid, ION_DEFAULT_XMIT_RATE) < 0
		|| bpStartPlan(rtp->stp.eid) < 0)
		{
			putErrmsg("Can't add automatic egress plan.",
					rtp->stp.eid);
			return -1;
		}

		findPlan(rtp->stp.eid, &vplan, &vplanElt);
	}

	if (attachPlanDuct(rtp->stp.eid, rtp->stp.vduct->outductElt) < 0)
	{
		putErrmsg("Can't attach duct to plan.", rtp->stp.eid);
		return -1;
	}

	if (pthread_begin(&(rtp->senderThread), NULL, sendBundles, rtp))
	{
		putSysErrmsg("brsscla can't create sender thread", NULL);
		return -1;
	}

	return 0;
}

static void	stopSendingThread(ReceiverThreadParms *rtp)
{
	if (rtp->stp.vduct && rtp->stp.vduct->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(rtp->stp.vduct->semaphore);
	}

	if (rtp->hasSender)
	{
		pthread_join(rtp->senderThread, NULL);
	}

	removeOutduct("brss", rtp->stp.outductName);
}

static void	*receiveBundles(void *parm)
{
	/*	Main loop for bundle reception thread on one
	 *	connection, terminating when connection is lost.	*/

	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "brsscla";
	time_t			currentTime;
	unsigned char		sdnvText[10];
	int			sdnvLength = 0;
	unsigned int		nodeNbr;
	uvast			val;
	char			registration[24];
	time_t			timeTag;
	char			keyName[32];
	char			key[DIGEST_LEN];
	int			keyBufLen = sizeof key;
	int			keyLen;
	char			errtext[300];
	char			digest[DIGEST_LEN];
	AcqWorkArea		*work;
	char			*buffer;
	int			running = 1;

	currentTime = time(NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	memset(sdnvText, 0, sizeof sdnvText);

	/*	Get duct number, expressed as an SDNV.			*/

	while (1)
	{
		switch (itcp_recv(&(parms->bundleSocket),
				(char *) (sdnvText + sdnvLength), 1))
		{
			case 1:
				break;		/*	Out of switch.	*/
	
			case -1:
				putErrmsg("Can't get duct number.", NULL);
	
				/*	Intentional fall-through.	*/
	
			default:		/*	Inauthentic.	*/
				*parms->authenticated = 1;
				terminateReceiverThread(parms);
				return NULL;
		}

		if ((*(sdnvText + sdnvLength) & 0x80) == 0)
		{
			break;			/*	Out of loop.	*/
		}

		sdnvLength++;
		if (sdnvLength < 10)
		{
			continue;
		}

		putErrmsg("Duct number SDNV is too long.", NULL);
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	oK(decodeSdnv(&val, sdnvText));
	nodeNbr = val;
	parms->nodeNbr = nodeNbr;
	isprintf(keyName, sizeof keyName, "%u.brs", nodeNbr);
	keyLen = sec_get_key(keyName, &keyBufLen, key);
	if (keyLen == 0)
	{
		putErrmsg("Can't get HMAC key for duct.", keyName);
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	/*	Get time tag and its HMAC-SHA1 digest.			*/

	switch (itcp_recv(&(parms->bundleSocket), registration,
			REGISTRATION_LEN))
	{
	case REGISTRATION_LEN:
		break;				/*	Out of switch.	*/

	case -1:
		putErrmsg("Can't get registration.", NULL);

			/*	Intentional fall-through to next case.	*/

	default:
		*parms->authenticated = 1;
		terminateReceiverThread(parms);
		return NULL;
	}

	memcpy((char *) &timeTag, registration, 4);
	timeTag = ntohl(timeTag);
	if (timeTag - currentTime > BRSTERM || currentTime - timeTag > BRSTERM)
	{
		isprintf(errtext, sizeof errtext, "[?] Registration rejected: \
time tag is %u, must be between %u and %u.", (unsigned int) timeTag,
				(unsigned int) (currentTime - BRSTERM),
				(unsigned int) (currentTime + BRSTERM));
		writeMemo(errtext);
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	oK(hmac_authenticate(digest, DIGEST_LEN, key, keyLen, registration, 4));
	if (memcmp(digest, registration + 4, DIGEST_LEN) != 0)
	{
		writeMemo("[?] Registration rejected: digest is incorrect.");
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	*parms->authenticated = 2;		/*	Authentic.	*/

	/*	Client is authenticated.  Now authenticate self to
	 *	the client and continue.				*/

	timeTag++;
	timeTag = htonl(timeTag);
	oK(hmac_authenticate(digest, DIGEST_LEN, key, keyLen,
			(char *) &timeTag, 4));
	memcpy(registration + 4, digest, DIGEST_LEN);
	if (itcp_send(&(parms->bundleSocket), registration + 4,
			DIGEST_LEN) < DIGEST_LEN)
	{
		putErrmsg("Can't countersign to client.",
				itoa(parms->bundleSocket));
		terminateReceiverThread(parms);
		return NULL;
	}

	/*	Release contents of limbo queue, exactly as if an
	 *	outduct had been unblocked.  This is because that
	 *	queue may contain bundles that went into limbo when
	 *	dequeued, due to brsc disconnection.			*/

	if (reforwardStrandedBundles() < 0)
	{
		putErrmsg("brssscla can't reforward bundles.", NULL);
		terminateReceiverThread(parms);
		return NULL;
	}

	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("brsscla can't get acquisition work area.", NULL);
		terminateReceiverThread(parms);
		return NULL;
	}

	buffer = MTAKE(STCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("brsscla can't get TCP buffer.", NULL);
		terminateReceiverThread(parms);
		return NULL;
	}

	/*	Start the sending thread for this socket.		*/

	if (startSendingThread(parms) < 0)
	{
		putErrmsg("brsscla can't start sending thread.",
				itoa(parms->bundleSocket));
		terminateReceiverThread(parms);
		return NULL;
	}

	/*	Now start receiving bundles.				*/

	while (running && *(parms->running))
	{
		if (bpBeginAcq(work, 0, NULL) < 0)
		{
			putErrmsg("can't begin acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			*(parms->running) = 0;
			continue;
		}

		switch (receiveBundleByStcp(&parms->bundleSocket, work, buffer,
				_attendant(NULL)))
		{
		case -1:
			putErrmsg("Can't acquire bundle.", NULL);
			ionKillMainThread(procName);
			*(parms->running) = 0;
			continue;

			/*	Intentional fall-through to next case.	*/

		case 0:				/*	Normal stop.	*/
			running = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		if (bpEndAcq(work) < 0)
		{
			putErrmsg("Can't end acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			*(parms->running) = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	Shut down sender thread cleanly.			*/

	stopSendingThread(parms);

	/*	Finish releasing receiver thread's resources.		*/

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	terminateReceiverThread(parms);
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
	char			*procName = "brsscla";
	pthread_mutex_t		mutex;
	Lyst			threads;
	int			newSocket;
	struct sockaddr		clientSocketName;
	socklen_t		nameLength = sizeof(struct sockaddr);
	ReceiverThreadParms	*receiverParms;
	int			authenticated;
	LystElt			elt;
	pthread_t		thread;

	snooze(1);	/*	Let main thread become interruptable.	*/
	pthread_mutex_init(&mutex, NULL);
	threads = lyst_create_using(getIonMemoryMgr());
	if (threads == NULL)
	{
		putErrmsg("brsscla can't create threads list.", NULL);
		ionKillMainThread(procName);
		pthread_mutex_destroy(&mutex);
		return NULL;
	}

	/*	Can now begin accepting connections from clients.  On
	 *	failure, take down the whole CLA.			*/

	while (atp->running)
	{
		nameLength = sizeof(struct sockaddr);
		newSocket = accept(atp->ductSocket, &clientSocketName,
				&nameLength);
		if (newSocket < 0)
		{
			putSysErrmsg("brsscla accept() failed", NULL);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		if (atp->running == 0)
		{
			closesocket(newSocket);
			break;	/*	Main thread has shut down.	*/
		}

		receiverParms = (ReceiverThreadParms *)
				MTAKE(sizeof(ReceiverThreadParms));
		if (receiverParms == NULL)
		{
			putErrmsg("brsscla can't allocate for thread", NULL);
			closesocket(newSocket);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		receiverParms->vduct = atp->vduct;
		pthread_mutex_lock(&mutex);
		receiverParms->elt = lyst_insert_last(threads, receiverParms);
		pthread_mutex_unlock(&mutex);
		if (receiverParms->elt == NULL)
		{
			putErrmsg("brsscla can't allocate for thread", NULL);
			MRELEASE(receiverParms);
			closesocket(newSocket);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		receiverParms->mutex = &mutex;
		receiverParms->bundleSocket = newSocket;
		authenticated = 0;		/*	Unknown.	*/
		receiverParms->authenticated = &authenticated;
		receiverParms->nodeNbr = (unsigned int) -1;
		receiverParms->running = &(atp->running);
		if (pthread_begin(&(receiverParms->receiverThread), NULL,
				receiveBundles, receiverParms))
		{
			putSysErrmsg("brsscla can't create new thread", NULL);
			MRELEASE(receiverParms);
			closesocket(newSocket);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		snooze(1);		/*	Wait for authenticator.	*/
		if (authenticated == 0)	/*	Still unknown.		*/
		{
			/*	Assume hung on DOS attack.  Bail out.	*/

			thread = receiverParms->receiverThread;
			pthread_end(thread);
			pthread_join(thread, NULL);
			terminateReceiverThread(receiverParms);
		}
	}

	closesocket(atp->ductSocket);
	writeErrmsgMemos();

	/*	Shut down all clients' receiver threads cleanly.	*/

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

		receiverParms = (ReceiverThreadParms *) lyst_data(elt);
		thread = receiverParms->receiverThread;
//#ifdef mingw
		shutdown(receiverParms->bundleSocket, SD_BOTH);
//#else
//		pthread_kill(thread, SIGINT);
//#endif
		pthread_mutex_unlock(&mutex);
		pthread_join(thread, NULL);
	}

	lyst_destroy(threads);
	writeErrmsgMemos();
	writeMemo("[i] brsscla access thread has ended.");
	pthread_mutex_destroy(&mutex);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

static int	run_brsscla(char *ductName)
{
	VInduct			*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Induct			induct;
	ClProtocol		protocol;
	char			*hostName;
	unsigned short		portNbr;
	unsigned int		hostNbr = INADDR_ANY;
	AccessThreadParms	atp;
	socklen_t		nameLength;
	ReqAttendant		attendant;
	pthread_t		accessThread;
	int			fd;

	findInduct("brss", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such brss induct.", ductName);
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
	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, induct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	hostName = ductName;
	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		putErrmsg("Can't get IP/port for host.", hostName);
		return 1;
	}

	if (portNbr == 0)
	{
		portNbr = 80;	/*	Default to HTTP's port number.	*/
	}

	/*	hostNbr == 0 (INADDR_ANY) is okay for BRS server.	*/

	portNbr = htons(portNbr);
	hostNbr = htonl(hostNbr);
	atp.vduct = vduct;
	memset((char *) &(atp.socketName), 0, sizeof(struct sockaddr));
	atp.inetName = (struct sockaddr_in *) &(atp.socketName);
	atp.inetName->sin_family = AF_INET;
	atp.inetName->sin_port = portNbr;
	memcpy((char *) &(atp.inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	atp.ductSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
		putSysErrmsg("Can't initialize socket (note: must be root for \
port 80)", NULL);
		return 1;
	}

	/*	Set up blocking acquisition of data via TCP.		*/

	if (ionStartAttendant(&attendant) < 0)
	{
		closesocket(atp.ductSocket);
		putErrmsg("Can't initialize blocking TCP reception.", NULL);
		return 1;
	}

	oK(_attendant(&attendant));

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("brsscla");
#ifndef mingw
	isignal(SIGPIPE, itcp_handleConnectionLoss);	/*	Sender.	*/
	isignal(SIGINT, handleStopThread);
#endif
	isignal(SIGTERM, handleStopBrsscla);

	/*	Start the access thread.				*/

	atp.running = 1;
	if (pthread_begin(&accessThread, NULL, spawnReceivers, &atp))
	{
		closesocket(atp.ductSocket);
		putSysErrmsg("brsscla can't create access thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the server.				*/

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] brsscla is running, spec=[%s:%d].", 
			inet_ntoa(atp.inetName->sin_addr), ntohs(portNbr));
		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	atp.running = 0;
	ionPauseAttendant(&attendant);

	/*	Wake up the access thread by connecting to it.		*/

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd >= 0)
	{
		oK(connect(fd, &(atp.socketName), sizeof(struct sockaddr)));

		/*	Immediately discard the connected socket.	*/

		closesocket(fd);
	}

	pthread_join(accessThread, NULL);
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	writeMemo("[i] brsscla induct has ended.");
	return 0;
}

#if defined (ION_LWT)
int	brsscla(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = argc > 1 ? argv[1] : NULL;
#endif
	int	result;

	if (ductName == NULL)
	{
		PUTS("Usage: brsscla <local host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("brsscla can't attach to BP.", NULL);
		return 1;
	}

	result = run_brsscla(ductName);
	bp_detach();
	return result;
}
