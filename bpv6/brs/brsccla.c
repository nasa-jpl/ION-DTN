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

static ReqAttendant	*_attendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

static sm_SemId		brscclaSemaphore(sm_SemId *semid)
{
	uaddr		temp;
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

	temp = (uaddr) value;
	semaphore = temp;
	return semaphore;
}

static void	killMainThread()
{
	sm_SemEnd(brscclaSemaphore(NULL));
}

static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
	ionPauseAttendant(_attendant(NULL));
	killMainThread();
}

/*	*	*	Keepalive thread functions	*	*	*/

typedef struct
{
	pthread_mutex_t	*mutex;
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		*ductSocket;
	int		*running;
} KeepaliveThreadParms;

static void	*sendKeepalives(void *parm)
{
	KeepaliveThreadParms	*parms = (KeepaliveThreadParms *) parm;
	int			count = STCP_KEEPALIVE_PERIOD;
	int			bytesSent;

	snooze(1);	/*	Let main thread become interruptable.	*/
	while (*(parms->running))
	{
		if (count < STCP_KEEPALIVE_PERIOD)
		{
			count++;
			snooze(1);
			continue;
		}

		/*	Time to send a keepalive.			*/

		pthread_mutex_lock(parms->mutex);
		bytesSent = sendBundleByStcp(parms->protocolName,
				parms->ductName, parms->ductSocket, 0, 0, NULL);
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
	VInduct		*vduct;
	int		*ductSocket;
	int		*running;
} ReceiverThreadParms;

static void	*receiveBundles(void *parm)
{
	/*	Main loop for the bundle reception thread,
	 *	terminating when connection is lost.			*/

	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	ReqAttendant		attendant;
	AcqWorkArea		*work;
	char			*buffer;

	if (ionStartAttendant(&attendant) < 0)
	{
		putErrmsg("Can't initialize blocking TCP reception.", NULL);
		return NULL;
	}

	oK(_attendant(&attendant));
	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("brsccla can't get acquisition work area.", NULL);
		return NULL;
	}

	buffer = MTAKE(STCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("brsccla can't get TCP buffer.", NULL);
		return NULL;
	}

	/*	Now start receiving bundles.				*/

	while (*(parms->running))
	{
		if (bpBeginAcq(work, 0, NULL) < 0)
		{
			putErrmsg("can't begin acquisition of bundle.", NULL);
			killMainThread();
			*(parms->running) = 0;
			continue;
		}

		switch (receiveBundleByStcp(parms->ductSocket, work, buffer,
				&attendant))
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
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	writeMemo("[i] brsccla receiver thread stopping.");
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	brsccla(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
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
	char			*buffer;
	Sdr			sdr;
	VInduct			*vinduct;
	PsmAddress		vductElt;
	VOutduct		*voutduct;
	Induct			induct;
	Outduct			outduct;
	ClProtocol		protocol;
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
	BpAncillaryData		ancillaryData;
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

	buffer = MTAKE(STCPCLA_BUFSZ);
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

	*underscore = '\0';	/*	Truncate ductName.		*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &induct, sdr_list_data(sdr,
			vinduct->inductElt), sizeof(Induct));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
			voutduct->outductElt), sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, induct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/*	Send registration message: duct number (SDNV text),
	 *	timestamp, and message authentication code.  If the
	 *	server rejects the signature, it simply closes the
	 *	connection so that next operation on this socket fails.	*/

	if (openStcpOutductSocket(protocol.name, ductName, &ductSocket) < 0
	|| ductSocket < 0)
	{
		putErrmsg("Can't connect to server.", ductName);
		MRELEASE(buffer);
		return 1;
	}

	timeTag = time(NULL);
	timeTag = htonl(timeTag);
	memcpy(registration, (char *) &timeTag, 4);
	oK(hmac_authenticate(registration + 4, DIGEST_LEN, key, keyLen,
			(char *) &timeTag, 4));
	if (itcp_send(&ductSocket, (char *) ductSdnv.text, ductSdnv.length)
			< ductSdnv.length
	|| itcp_send(&ductSocket, registration, REGISTRATION_LEN)
			< REGISTRATION_LEN)
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
	switch (itcp_recv(&ductSocket, receivedCountersign, DIGEST_LEN))
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
	 *	thread instantiation.					*/

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(brscclaSemaphore(&(voutduct->semaphore)));
	isignal(SIGTERM, interruptThread);
#ifndef mingw
	isignal(SIGPIPE, itcp_handleConnectionLoss);
#endif

	/*	Start the receiver thread.				*/

	receiverParms.vduct = vinduct;
	receiverParms.ductSocket = &ductSocket;
	receiverParms.running = &running;
        if (pthread_begin(&receiverThread, NULL, receiveBundles,
			&receiverParms, "brsccla_receiver"))
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
	istrcpy(ktparms.protocolName, protocol.name, MAX_CL_PROTOCOL_NAME_LEN);
	istrcpy(ktparms.ductName, ductName, MAX_CL_DUCT_NAME_LEN);
	ktparms.ductSocket = &ductSocket;
	ktparms.running = &running;
	if (pthread_begin(&keepaliveThread, NULL, sendKeepalives,
		&ktparms, "brsccla_keepalive"))
	{
		putSysErrmsg("brsccla can't create keepalive thread", NULL);
		MRELEASE(buffer);
		closesocket(ductSocket);
		return 1;
	}

	/*	Can now begin transmitting to server.			*/

	writeMemo("[i] brsccla is running....");
	while (!(sm_SemEnded(brscclaSemaphore(NULL))))
	{
		if (bpDequeue(voutduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)		/*	Outduct closed.	*/
		{
			writeMemo("[i] brsccla outduct closed.");
			sm_SemEnd(brscclaSemaphore(NULL));/*	Stop.	*/
			continue;
		}

		if (bundleZco == 1)		/*	Corrupt bundle.	*/
		{
			continue;		/*	Get next one.	*/
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		pthread_mutex_lock(&mutex);
		bytesSent = sendBundleByStcp(protocol.name, ductName,
				&ductSocket, bundleLength, bundleZco, buffer);
		pthread_mutex_unlock(&mutex);
		if (bytesSent < 0)
		{
			/*	If this is just a transient connection
			 *	anomaly then the outduct has been
			 *	blocked, but we have to stop it
			 *	altogether.  We can't just wait for
			 *	a keepalive to detect reconnection
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
	closeStcpOutductSocket(&ductSocket);
	pthread_mutex_destroy(&mutex);
	writeErrmsgMemos();
	writeMemo("[i] brsccla duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
