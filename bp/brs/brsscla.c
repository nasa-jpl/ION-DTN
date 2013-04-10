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

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("brsscla");
}

/*	*	*	Sender thread functions	*	*	*	*/

typedef struct
{
	VOutduct	*vduct;
	int		baseDuctNbr;
	int		lastDuctNbr;
	int		*brsSockets;
} SenderThreadParms;

static void	*terminateSenderThread(SenderThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] sender thread stopping.");
	return NULL;
}

static void	*sendBundles(void *parm)
{
	/*	Main loop for single bundle transmission thread
	 *	serving all BRS sockets.				*/

	SenderThreadParms	*parms = (SenderThreadParms *) parm;
	char			*procName = "brsscla";
	unsigned char		*buffer;
	Outduct			outduct;
	Sdr			sdr;
	Outflow			outflows[3];
	int			i;
	Object			bundleZco;
	BpExtendedCOS		extendedCOS;
	char			destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	unsigned int		bundleLength;
	int			ductNbr;
	int			bytesSent;
	Object			bundleAddr;
	Bundle			bundle;

	snooze(1);	/*	Let main thread become interruptable.	*/
	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in brsscla.", NULL);
		ionKillMainThread(procName);
		return terminateSenderThread(parms);
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

	while (!(sm_SemEnded(parms->vduct->semaphore)))
	{
		if (bpDequeue(parms->vduct, outflows, &bundleZco,
				&extendedCOS, destDuctName, 0, -1) < 0)
		{
			break;
		}

		if (bundleZco == 0)		/*	Interrupted.	*/
		{
			continue;
		}

		CHKNULL(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		ductNbr = atoi(destDuctName);
		if (ductNbr >= parms->baseDuctNbr
		&& ductNbr <= parms->lastDuctNbr
		&& parms->brsSockets[(i = ductNbr - parms->baseDuctNbr)] != -1)
		{
			bytesSent = sendBundleByTCP(NULL, parms->brsSockets + i,
					bundleLength, bundleZco, buffer);

			/*	Note that TCP I/O errors never block
			 *	the brsscla induct's output functions;
			 *	those functions never connect to remote
			 *	sockets and never behave like a TCP
			 *	outduct, so the _tcpOutductId table is
			 *	never populated.			*/

			if (bytesSent < 0)
			{
				putErrmsg("Can't send bundle.", NULL);
				break;
			}
		}
		else	/*	Can't send it; try again later?		*/
		{
			bytesSent = 0;
		}

		if (bytesSent < bundleLength)
		{
			/*	Couldn't send the bundle, so put it
			 *	in limbo so we can try again later
			 *	-- except that if bundle has already
			 *	been destroyed then just lose the ADU.	*/

			CHKNULL(sdr_begin_xn(sdr));
			if (retrieveInTransitBundle(bundleZco, &bundleAddr) < 0)
			{
				putErrmsg("Can't locate unsent bundle.", NULL);
				sdr_cancel_xn(sdr);
				break;
			}

			if (bundleAddr == 0)
			{
				/*	Bundle not found, so we can't
				 *	put it in limbo for another
				 *	attempt later; discard the ADU.	*/

				zco_destroy(sdr, bundleZco);
			}
			else
			{
				sdr_stage(sdr, (char *) &bundle, bundleAddr,
						sizeof(Bundle));
				if (bundle.extendedCOS.flags
						& BP_MINIMUM_LATENCY)
				{
					/*	We never put critical
					 *	bundles into limbo.	*/

					zco_destroy(sdr, bundleZco);
				}
				else
				{
					if (enqueueToLimbo(&bundle, bundleAddr)
							< 0)
					{
						putErrmsg("Can't save bundle.",
								NULL);
						sdr_cancel_xn(sdr);
						break;
					}
				}
			}

			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Failed handling brss xmit.", NULL);
				break;
			}
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	ionKillMainThread(procName);
	writeMemo("[i] brsscla outduct has ended.");
	MRELEASE(buffer);
	return terminateSenderThread(parms);
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
			sdr_cancel_xn(sdr);
			return -1;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("brss failed limbo release on client connect.", NULL);
		return -1;
	}

	return 0;
}

typedef struct
{
	char		senderEidBuffer[SDRSTRING_BUFSZ];
	char		*senderEid;
	VInduct		*vduct;
	LystElt		elt;
	pthread_mutex_t	*mutex;
	int		bundleSocket;
	pthread_t	thread;
	int		*running;
	unsigned int	ductNbr;
	int		*authenticated;
	int		baseDuctNbr;
	int		lastDuctNbr;
	int		*brsSockets;
} ReceiverThreadParms;

static void	terminateReceiverThread(ReceiverThreadParms *parms)
{
	int	senderSocket;

	writeErrmsgMemos();
	writeMemo("[i] brsscla receiver thread stopping.");
	pthread_mutex_lock(parms->mutex);
	if (parms->bundleSocket != -1)
	{
		closesocket(parms->bundleSocket);
		if (parms->ductNbr != (unsigned int) -1)
		{
			senderSocket = parms->ductNbr - parms->baseDuctNbr;
			if (parms->brsSockets[senderSocket] ==
					parms->bundleSocket)
			{
				/*	Stop sender thread transmission
				 *	over this socket.  Note: does
				 *	not halt the sender thread.	*/

				parms->brsSockets[senderSocket] = -1;
			}
		}

		parms->bundleSocket = -1;
	}

	lyst_delete(parms->elt);
	pthread_mutex_unlock(parms->mutex);
	MRELEASE(parms);
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
	unsigned int		ductNbr;
	uvast			val;
	int			senderSocket;
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

	currentTime = time(NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	memset(sdnvText, 0, sizeof sdnvText);

	/*	Get duct number, expressed as an SDNV.			*/

	while (1)
	{
		switch (receiveBytesByTCP(parms->bundleSocket,
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
	ductNbr = val;
	if (ductNbr < parms->baseDuctNbr || ductNbr > parms->lastDuctNbr)
	{
		putErrmsg("Duct number is too large.", utoa(ductNbr));
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	parms->ductNbr = ductNbr;
	senderSocket = ductNbr - parms->baseDuctNbr;
	if (parms->brsSockets[senderSocket] != -1)
	{
		putErrmsg("Client is already connected.", utoa(ductNbr));
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	isprintf(keyName, sizeof keyName, "%u.brs", ductNbr);
	keyLen = sec_get_key(keyName, &keyBufLen, key);
	if (keyLen == 0)
	{
		putErrmsg("Can't get HMAC key for duct.", keyName);
		*parms->authenticated = 1;	/*	Inauthentic.	*/
		terminateReceiverThread(parms);
		return NULL;
	}

	/*	Get time tag and its HMAC-SHA1 digest.			*/

	switch (receiveBytesByTCP(parms->bundleSocket, registration,
			REGISTRATION_LEN))
	{
		case REGISTRATION_LEN:
			break;			/*	Out of switch.	*/

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
	parms->brsSockets[senderSocket] = parms->bundleSocket;

	/*	Client is authenticated.  Now authenticate self to
	 *	the client and continue.				*/

	timeTag++;
	timeTag = htonl(timeTag);
	oK(hmac_authenticate(digest, DIGEST_LEN, key, keyLen,
			(char *) &timeTag, 4));
	memcpy(registration + 4, digest, DIGEST_LEN);
	if (sendBytesByTCP(&parms->bundleSocket, registration + 4,
			DIGEST_LEN, NULL) < DIGEST_LEN)
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

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("brsscla can't get TCP buffer.", NULL);
		terminateReceiverThread(parms);
		return NULL;
	}

	parms->senderEid = parms->senderEidBuffer;
	isprintf(parms->senderEidBuffer, sizeof parms->senderEidBuffer,
			"ipn:%u.0", ductNbr);

	/*	Now start receiving bundles.				*/

	while (*(parms->running))
	{
		if (bpBeginAcq(work, 0, parms->senderEid) < 0)
		{
			putErrmsg("can't begin acquisition of bundle.", NULL);
			ionKillMainThread(procName);
			*(parms->running) = 0;
			continue;
		}

		switch (receiveBundleByTcp(parms->bundleSocket, work, buffer))
		{
		case -1:
			putErrmsg("Can't acquire bundle.", NULL);
			ionKillMainThread(procName);

			/*	Intentional fall-through to next case.	*/

		case 0:				/*	Normal stop.	*/
			*(parms->running) = 0;
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
	int			baseDuctNbr;
	int			lastDuctNbr;
	int			*brsSockets;
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
	 *	failure, take down the whole CLI.			*/

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
			break;	/*	Main thread has shut down.	*/
		}

		receiverParms = (ReceiverThreadParms *)
				MTAKE(sizeof(ReceiverThreadParms));
		if (receiverParms == NULL)
		{
			putErrmsg("brsscla can't allocate for thread", NULL);
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
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		receiverParms->mutex = &mutex;
		receiverParms->bundleSocket = newSocket;
		authenticated = 0;		/*	Unknown.	*/
		receiverParms->authenticated = &authenticated;
		receiverParms->ductNbr = (unsigned int) -1;
		receiverParms->baseDuctNbr = atp->baseDuctNbr;
		receiverParms->lastDuctNbr = atp->lastDuctNbr;
		receiverParms->brsSockets = atp->brsSockets;
		receiverParms->running = &(atp->running);
		if (pthread_begin(&(receiverParms->thread), NULL,
				receiveBundles, receiverParms))
		{
			putSysErrmsg("brsscla can't create new thread", NULL);
			MRELEASE(receiverParms);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		snooze(1);		/*	Wait for authenticator.	*/
		if (authenticated == 0)	/*	Still unknown.		*/
		{
			/*	Assume hung on DOS attack.  Bail out.	*/

			thread = receiverParms->thread;
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
		thread = receiverParms->thread;
#ifdef mingw
		shutdown(receiverParms->bundleSocket, SD_BOTH);
#else
		pthread_kill(thread, SIGTERM);
#endif
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

static int	run_brsscla(char *ductName, int baseDuctNbr, int lastDuctNbr,
			int *brsSockets)
{
	VInduct			*vinduct;
	PsmAddress		vductElt;
	VOutduct		*voutduct;
	Sdr			sdr;
	Induct			induct;
	ClProtocol		protocol;
	char			*hostName;
	unsigned short		portNbr;
	unsigned int		hostNbr = INADDR_ANY;
	AccessThreadParms	atp;
	socklen_t		nameLength;
	SenderThreadParms	senderParms;
	pthread_t		senderThread;
	pthread_t		accessThread;
	int			fd;

	findInduct("brss", ductName, &vinduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such brss induct.", ductName);
		return 1;
	}

	if (vinduct->cliPid != ERROR && vinduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vinduct->cliPid));
		return 1;
	}

	findOutduct("brss", ductName, &voutduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such brss outduct.", ductName);
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr, vinduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, induct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	if (protocol.nominalRate == 0)
	{
		vinduct->acqThrottle.nominalRate = DEFAULT_BRS_RATE;
		voutduct->xmitThrottle.nominalRate = DEFAULT_BRS_RATE;
	}
	else
	{
		vinduct->acqThrottle.nominalRate = protocol.nominalRate;
		voutduct->xmitThrottle.nominalRate = protocol.nominalRate;
	}

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
	atp.vduct = vinduct;
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

	/*	Initialize sender endpoint ID lookup.			*/

	ipnInit();
	dtn2Init();

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	ionNoteMainThread("brsscla");
#ifndef mingw
	isignal(SIGPIPE, handleConnectionLoss);	/*	For sender.	*/
#endif
	isignal(SIGTERM, interruptThread);

	/*	Start the sender thread; a single sender for all
	 *	connections.						*/

	senderParms.vduct = voutduct;
	senderParms.baseDuctNbr = baseDuctNbr;
	senderParms.lastDuctNbr = lastDuctNbr;
	senderParms.brsSockets = brsSockets;
	if (pthread_begin(&senderThread, NULL, sendBundles, &senderParms))
	{
		closesocket(atp.ductSocket);
		putSysErrmsg("brsscla can't create sender thread", NULL);
		return 1;
	}

	/*	Start the access thread.				*/

	atp.running = 1;
	atp.baseDuctNbr = baseDuctNbr;
	atp.lastDuctNbr = lastDuctNbr;
	atp.brsSockets = brsSockets;
	if (pthread_begin(&accessThread, NULL, spawnReceivers, &atp))
	{
		sm_SemEnd(voutduct->semaphore);
		pthread_join(senderThread, NULL);
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

	/*	Shut down sender thread cleanly.			*/

	if (voutduct->semaphore != SM_SEM_NONE)
	{
		sm_SemEnd(voutduct->semaphore);
	}

	pthread_join(senderThread, NULL);

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
	writeMemo("[i] brsscla induct has ended.");
	return 0;
}

/*	Each member of the brsSockets table is the FD of the open
 *	socket (if any) that has been established for communication
 *	with the BRS client identified by the duct number whose value
 *	is the base duct number for this BRS daemon plus this member's
 *	position within the table.  That is, if the base duct number
 *	for the server is 1 then brsSockets[19] is the FD for
 *	communicating with the BRS client identified by BRS duct
 *	number 20.							*/

#if defined (VXWORKS) || defined (RTEMS)
int	brsscla(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
	int	baseDuctNbr = a2 ? atoi((char *) a2) : 1;
	int	lastDuctNbr = a3 ? atoi((char *) a3) : baseDuctNbr + 255;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = argc > 1 ? argv[1] : NULL;
	int	baseDuctNbr = argc > 2 ? atoi(argv[2]) : 1;
	int	lastDuctNbr = argc > 3 ? atoi(argv[3]) : baseDuctNbr + 255;
#endif
	int	scope;
	int	i;
	int	result;
	int	*brsSockets;

	if (ductName == NULL)
	{
		PUTS("Usage: brsscla <local host name>[:<port number>] \
[<first duct nbr in scope>[ <last duct nbr in scope>]]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("brsscla can't attach to BP.", NULL);
		return 1;
	}

	if (baseDuctNbr < 1 || lastDuctNbr < baseDuctNbr)
	{
		putErrmsg("brsscla scope error: first duct nbr must be > 0, \
last ductNbr must not be less than first.", itoa(baseDuctNbr));
		return 1;
	}

	scope = lastDuctNbr - baseDuctNbr;
	brsSockets = (int *) MTAKE(sizeof(int) * scope);
	if (brsSockets == NULL)
	{
		putErrmsg("Can't allocate BRS sockets array.", itoa(scope));
		return 1;
	}

	for (i = 0; i < scope; i++)
	{
		brsSockets[i] = -1;
	}

	result = run_brsscla(ductName, baseDuctNbr, lastDuctNbr, brsSockets);
	MRELEASE(brsSockets);
	bp_detach();
	return result;
}
