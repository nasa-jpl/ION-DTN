/*
	brsccla.c:	BP Bundle Relay Service convergence-layer
			client daemon.  Handles both transmission
			and reception.

	Author: Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "brscla.h"

static sm_SemId		brscclaSemaphore(sm_SemId *semid)
{
	long		temp;
	void		*value;
	sm_SemId	semaphore;

	if (semid)			/*	Add task variable.	*/
	{
		temp = *semid;
		value = (void *) temp;
		value = sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		value = sm_TaskVar(NULL);
	}

	temp = (long) value;
	semaphore = temp;
	return semaphore;
}

static void	killMainThread()
{
	sm_SemEnd(brscclaSemaphore(NULL));
}

static void	interruptThread()	/*	Commands termination.	*/
{
	isignal(SIGTERM, killMainThread);
	killMainThread();
}

/*	*	*	Keepalive thread functions	*	*	*/

typedef struct
{
	pthread_mutex_t	*mutex;
	struct sockaddr	*socketName;
	int		*ductSocket;
	int		*running;
} KeepaliveThreadParms;

static void	*sendKeepalives(void *parm)
{
	KeepaliveThreadParms	*parms = (KeepaliveThreadParms *) parm;
	int			count = KEEPALIVE_PERIOD;
	int			bytesSent;

	snooze(1);	/*	Let main thread become interruptable.	*/
	while (*(parms->running))
	{
		if (count < KEEPALIVE_PERIOD)
		{
			count++;
			snooze(1);
			continue;
		}

		/*	Time to send a keepalive.			*/

		pthread_mutex_lock(parms->mutex);
		bytesSent = sendBundleByTCP(parms->socketName,
				parms->ductSocket, 0, 0, NULL);
		pthread_mutex_unlock(parms->mutex);
		if (bytesSent <= 0)
		{
			/*	Note that I/O error on socket is
			 *	NOT treated as a transient anomaly.
			 *	We have to re-authenticate on
			 *	reconnecting, so the CLO has to
			 *	be shut down altogether; a simple
			 *	automatic reconnect within the
			 *	keepalive thread won't work.		*/

			killMainThread();
			*(parms->running) = 0;
			continue;
		}

		count = 0;
		snooze(1);
	}

	return NULL;
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	char		senderEidBuffer[SDRSTRING_BUFSZ];
	char		*senderEid;
	VInduct		*vduct;
	int		*ductSocket;
	int		*running;
} ReceiverThreadParms;

static void	*receiveBundles(void *parm)
{
	/*	Main loop for the bundle reception thread,
	 *	terminating when connection is lost.			*/

	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	AcqWorkArea		*work;
	char			*buffer;

	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("brsccla can't get acquisition work area.", NULL);
		return NULL;
	}

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("brsccla can't get TCP buffer.", NULL);
		return NULL;
	}

	/*	Now start receiving bundles.				*/

	while (*(parms->running))
	{
		if (bpBeginAcq(work, 0, parms->senderEid) < 0)
		{
			putErrmsg("can't begin acquisition of bundle.", NULL);
			killMainThread();
			*(parms->running) = 0;
			continue;
		}

		switch (receiveBundleByTcp(*(parms->ductSocket), work, buffer))
		{
		case -1:
			putErrmsg("can't acquire bundle.", NULL);
			killMainThread();

			/*	Intentional fall-through to next case.	*/

		case 0:				/*	Normal stop.	*/
			*(parms->running) = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		if (bpEndAcq(work) < 0)
		{
			putErrmsg("can't end acquisition of bundle.", NULL);
			killMainThread();
			*(parms->running) = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	End of receiver thread; release resources.		*/

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	writeErrmsgMemos();
	writeMemo("[i] brsccla receiver thread stopping.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
int	brsccla(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	char			*cursor;
	char			*underscore;
	char			*ductNbrString;
	int			ductNbr;
	Sdnv			ductSdnv;
	char			keyName[32];
	char			key[DIGEST_LEN];
	int			keyBufLen = sizeof key;
	int			keyLen;
	unsigned char		*buffer;
	Sdr			sdr;
	VInduct			*vinduct;
	PsmAddress		vductElt;
	VOutduct		*voutduct;
	Induct			induct;
	Outduct			outduct;
	ClProtocol		protocol;
	Outflow			outflows[3];
	int			i;
	char			hostName[16];
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			ductSocket;
	time_t			timeTag;
	char			registration[REGISTRATION_LEN];
	char			expectedCountersign[DIGEST_LEN];
	char			receivedCountersign[DIGEST_LEN];
	ReceiverThreadParms	receiverParms;
	int			running = 1;
	pthread_t		receiverThread;
	int			haveReceiverThread = 0;
	pthread_mutex_t		mutex;
	KeepaliveThreadParms	ktparms;
	pthread_t		keepaliveThread;
	Object			bundleZco;
	BpExtendedCOS		extendedCOS;
	char			destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	unsigned int		bundleLength;
	int			bytesSent;

	if (ductName == NULL)
	{
		PUTS("Usage: brsccla <server host name>[:<port number>]_<duct \
number>");
		return 0;
	}

	cursor = strchr(ductName, '_');
	if (cursor == NULL)
	{
		PUTS("Duct number omitted from duct name.");
		return 1;
	}

	underscore = cursor;
	ductNbrString = underscore + 1;
	ductNbr = atoi(ductNbrString);
	if (bpAttach() < 0)
	{
		putErrmsg("brsccla can't attach to BP.", NULL);
		return 1;
	}

	encodeSdnv(&ductSdnv, ductNbr);
	isprintf(keyName, sizeof keyName, "%u.brs", ductNbr);
	keyLen = sec_get_key(keyName, &keyBufLen, key);
	if (keyLen == 0)
	{
		putErrmsg("Can't get own HMAC key.", keyName);
		return 1;
	}

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in brsccla.", NULL);
		return 1;
	}

	findInduct("brsc", ductName, &vinduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such brsc induct.", ductName);
		MRELEASE(buffer);
		return 1;
	}

	if (vinduct->cliPid != ERROR && vinduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vinduct->cliPid));
		MRELEASE(buffer);
		return 1;
	}

	findOutduct("brsc", ductName, &voutduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such brsc outduct.", ductName);
		MRELEASE(buffer);
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr,
			vinduct->inductElt), sizeof(Induct));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			voutduct->outductElt), sizeof(Outduct));
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

	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = outduct.bulkQueue;
	outflows[1].outboundBundles = outduct.stdQueue;
	outflows[2].outboundBundles = outduct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	/*	Connect to BRS server.					*/

	*underscore = '\0';
	parseSocketSpec(ductName, &portNbr, &hostNbr);
	if (portNbr == 0)
	{
		portNbr = 80;
	}

	portNbr = htons(portNbr);
	if (hostNbr == 0)
	{
		putErrmsg("Can't get IP address for server.", ductName);
		MRELEASE(buffer);
		return 1;
	}

	hostNbr = htonl(hostNbr);
	printDottedString(hostNbr, hostName);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	if (_tcpOutductId(&socketName, "brsc", ductName) < 0)
	{
		putErrmsg("Can't record TCP Outduct ID for connection.", NULL);
		MRELEASE(buffer);
		return -1;
	}

	if (connectToCLI(&socketName, &ductSocket) < 0 || ductSocket < 0)
	{
		putErrmsg("Can't connect to server.", ductName);
		MRELEASE(buffer);
		return 1;
	}

	/*	Send registration message: duct number (SDNV text),
	 *	timestamp, and message authentication code.  If the
	 *	server rejects the signature, it simply closes the
	 *	connection so that next operation on this socket fails.	*/

	timeTag = time(NULL);
	timeTag = htonl(timeTag);
	memcpy(registration, (char *) &timeTag, 4);
	oK(hmac_authenticate(registration + 4, DIGEST_LEN, key, keyLen,
			(char *) &timeTag, 4));
	if (sendBytesByTCP(&ductSocket, (char *) ductSdnv.text, ductSdnv.length,
			&socketName) < ductSdnv.length
	|| sendBytesByTCP(&ductSocket, registration, REGISTRATION_LEN,
			&socketName) < REGISTRATION_LEN)
	{
		putErrmsg("Can't register with server.", itoa(ductSocket));
		MRELEASE(buffer);
		closesocket(ductSocket);
		return 1;
	}

	/*	Now look for correct countersign.  If not provided,
	 *	the BRS server is inauthentic.				*/

	timeTag = ntohl(timeTag);
	timeTag++;
	timeTag = htonl(timeTag);
	oK(hmac_authenticate(expectedCountersign, DIGEST_LEN, key, keyLen,
			(char *) &timeTag, 4));
	switch (receiveBytesByTCP(ductSocket, receivedCountersign, DIGEST_LEN))
	{
		case DIGEST_LEN:
			break;			/*	Out of switch.	*/

		case -1:
			putErrmsg("Can't get countersign.", NULL);

			/*	Intentional fall-through to next case.	*/

		default:
			writeMemo("[i] brsccla registration failed.");
			MRELEASE(buffer);
			closesocket(ductSocket);
			return 1;
	}

	if (memcmp(receivedCountersign, expectedCountersign, DIGEST_LEN) != 0)
	{
		writeMemo("[i] brs server judged inauthentic.");
		MRELEASE(buffer);
		closesocket(ductSocket);
		return 1;
	}

	/*	Server is now known to be authentic; proceed with
	 *	thread instantiation.  First initialize the sender
	 *	endpoint ID lookup.					*/

	ipnInit();
	dtn2Init();

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(brscclaSemaphore(&(voutduct->semaphore)));
	isignal(SIGTERM, interruptThread);
#ifndef mingw
	isignal(SIGPIPE, handleConnectionLoss);
#endif

	/*	Start the receiver thread.				*/

	receiverParms.vduct = vinduct;
	receiverParms.ductSocket = &ductSocket;
	receiverParms.running = &running;
	receiverParms.senderEid = receiverParms.senderEidBuffer;
	getSenderEid(&(receiverParms.senderEid), hostName);
        if (pthread_begin(&receiverThread, NULL, receiveBundles,
			&receiverParms))
	{
		putSysErrmsg("brsccla can't create receiver thread", NULL);
		MRELEASE(buffer);
		closesocket(ductSocket);
		return 1;
	}

	haveReceiverThread = 1;

	/*	Start the keepalive thread for this connection.		*/

	pthread_mutex_init(&mutex, NULL);
	ktparms.mutex = &mutex;
	ktparms.socketName = &socketName;
	ktparms.ductSocket = &ductSocket;
	ktparms.running = &running;
	if (pthread_begin(&keepaliveThread, NULL, sendKeepalives, &ktparms))
	{
		putSysErrmsg("brsccla can't create keepalive thread", NULL);
		MRELEASE(buffer);
		closesocket(ductSocket);
		return 1;
	}

	/*	Can now begin transmitting to server.			*/

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] brsccla is running, spec=[%s:%d].", 
			inet_ntoa(inetName->sin_addr), ntohs(portNbr));
		writeMemo(txt);
	}

	while (!(sm_SemEnded(brscclaSemaphore(NULL))))
	{
		if (bpDequeue(voutduct, outflows, &bundleZco, &extendedCOS,
				destDuctName, 0, -1) < 0)
		{
			sm_SemEnd(brscclaSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 0)		/*	Interrupted.	*/
		{
			continue;
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		pthread_mutex_lock(&mutex);
		bytesSent = sendBundleByTCP(&socketName, &ductSocket,
				bundleLength, bundleZco, buffer);
		pthread_mutex_unlock(&mutex);
		if (bytesSent <= 0)
		{
			/*	If this is just a transient connection
			 *	anomaly then the outduct has been
			 *	blocked, but we have to stop it
			 *	altogether.  We can't just wait for
			 *	a keepalive to retect reconnection
			 *	and resume: we must re-authenticate.	*/

			sm_SemEnd(brscclaSemaphore(NULL));
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	Finish cleaning up.					*/

	running = 0;
	if (haveReceiverThread)
	{
#ifdef mingw
		shutdown(ductSocket, SD_BOTH);
#else
		pthread_kill(receiverThread, SIGTERM);
#endif
		pthread_join(receiverThread, NULL);
	}

	pthread_join(keepaliveThread, NULL);
	if (ductSocket != -1)
	{
		closesocket(ductSocket);
	}

	pthread_mutex_destroy(&mutex);
	writeErrmsgMemos();
	writeMemo("[i] brsccla duct has ended.");
	oK(_tcpOutductId(&socketName, NULL, NULL));
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
