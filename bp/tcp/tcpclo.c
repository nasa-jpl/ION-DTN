/*
	tcpclo.c:	BP TCP-based convergence-layer output
			daemon.  Note that this convergence-layer
			output daemon is a "dedicated" CLO daemon
			suitable only for a limited number of paths,
			because it manages just a single TCP
			connection.

			Promiscuous CLO daemons need to be based on
			UDP, Dgr, etc.
			
			Modification : This has been made compliant
			to draft-irtf-dtnrg-tcp-clayer-02.

	Author: Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "tcpcla.h"

static sm_SemId		tcpcloSemaphore(sm_SemId *semid)
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
	else				/*	Retreive task variable.	*/
	{
		value = sm_TaskVar(NULL);
	}

	temp = (long) value;
	semaphore = temp;
	return semaphore;
}

static void	shutDownClo()	/*	Commands CLO termination.	*/
{
	isignal(SIGTERM, shutDownClo);
	sm_SemEnd(tcpcloSemaphore(NULL));
}

/*	*	*	Keepalive thread functions	*	*	*/

typedef struct
{
	int		*cloRunning;
	pthread_mutex_t	*mutex;
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		*ductSocket;
	int		*keepalivePeriod;
} KeepaliveThreadParms;

static void	*sendKeepalives(void *parm)
{
	KeepaliveThreadParms	*parms = (KeepaliveThreadParms *) parm;
	int			keepaliveTimer = 0;
	int			bytesSent;
	int			backoffTimer = BACKOFF_TIMER_START;
	int 			backoffTimerCount = 0;
	unsigned char 		*buffer;

	buffer = MTAKE(TCPCLA_BUFSZ);	//To send keepalive bundle
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in tcpclo.", NULL);
		return NULL;
	}

	iblock(SIGTERM);
	while (*(parms->cloRunning))
	{
		snooze(1);
		keepaliveTimer++;
		if (keepaliveTimer < *(parms->keepalivePeriod))
		{
			continue;
		}

		// If the negotiated keep alive interval is 0, then
		// keep alives will not be sent.
		if(*(parms->keepalivePeriod) == 0)
		{
			continue;
		}

		/*	Time to send a keepalive.  Note that the
		 *	interval between keepalive attempts will be
		 *	KEEPALIVE_PERIOD plus (if the remote induct
		 *	is not reachable) the length of time taken
		 *	by TCP to determine that the connection
		 *	attempt will not succeed (e.g., 3 seconds).	*/

		keepaliveTimer = 0;
		pthread_mutex_lock(parms->mutex);
		bytesSent = sendBundleByTCPCL(parms->protocolName,
				parms->ductName, parms->ductSocket,
				0, 0, buffer, parms->keepalivePeriod);
		pthread_mutex_unlock(parms->mutex);
		/*	if the node is unable to establish a TCP connection,
 		 * 	the connection should be tried only after some delay.
 		 *								*/
		if(bytesSent == 0)
		{	
			while((backoffTimerCount < backoffTimer) && (*(parms->ductSocket) < 0))
			{
				snooze(1);
				backoffTimerCount++;
				if(!(*(parms->cloRunning)))
				{
					break;
				}
			}
			backoffTimerCount = 0;
			/*	keepaliveTimer keeps track of when the keepalive needs 
			 *	to be sent. This value is set to keepalive period.
			 *	That way at the end of backoff period a 
			 *	keepalive is sent
			 *							*/
			keepaliveTimer = *(parms->keepalivePeriod);

			if(backoffTimer < BACKOFF_TIMER_LIMIT)
			{
				backoffTimer *= 2;
			}
			continue;
		}
		backoffTimer = BACKOFF_TIMER_START;
		if (bytesSent < 0)
		{
			shutDownClo();
			break;
		}
	}
	MRELEASE(buffer);
	return NULL;
}

/*	*	*	Receiver thread functions	*	*	*/
typedef struct
{
	int		*cloRunning;
	pthread_mutex_t	*mutex;
	int		*bundleSocket;
	VInduct		*vduct;
} ReceiveThreadParms;


static void	*receiveBundles(void *parm)
{
	/*	Main loop for bundle reception thread	*/

	ReceiveThreadParms	*parms = (ReceiveThreadParms *) parm;
	int			threadRunning = 1;
	AcqWorkArea		*work;
	char			*buffer;

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("tcpclo receiver can't get TCP buffer", NULL);
		return NULL;
	}

	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("tcpclo receiver can't get acquisition work area",
				NULL);
		MRELEASE(buffer);
		return NULL;
	}

	iblock(SIGTERM);
	while (threadRunning && *(parms->cloRunning))
	{
		if(*(parms->bundleSocket) < 0)
		{
			snooze(1);
			/*Retry later*/
			continue;
		}

		if (bpBeginAcq(work, 0, NULL) < 0)
		{
			putErrmsg("Can't begin acquisition of bundle.", NULL);
			threadRunning = 0;
			continue;
		}
	
		switch (receiveBundleByTcpCL(*(parms->bundleSocket), work,
					buffer))
		{
		case -1:
			putErrmsg("Can't acquire bundle.", NULL);
			pthread_mutex_lock(parms->mutex);
			closesocket(*(parms->bundleSocket));
			*(parms->bundleSocket) = -1;
			pthread_mutex_unlock(parms->mutex);
			continue;

		case 0:			/*	Shutdown message	*/	
			/*	Go back to the start of the while loop	*/
			pthread_mutex_lock(parms->mutex);
			closesocket(*(parms->bundleSocket));
			*(parms->bundleSocket) = -1;
			pthread_mutex_unlock(parms->mutex);			
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		if (bpEndAcq(work) < 0)
		{
			putErrmsg("Can't end acquisition of bundle.", NULL);
			threadRunning = 0;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	End of receiver thread; release resources.		*/

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	tcpclo(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	unsigned char		*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			duct;
	ClProtocol		protocol;
	Outflow			outflows[3];
	int			i;
	int			running = 1;
	pthread_mutex_t		mutex;
	KeepaliveThreadParms	parms;
	ReceiveThreadParms	rparms;
	pthread_t		keepaliveThread;
	pthread_t		receiverThread;
	Object			bundleZco;
	BpExtendedCOS		extendedCOS;
	char			destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	unsigned int		bundleLength;
	int			ductSocket = -1;
	int			bytesSent;
	int 			keepalivePeriod = 0;
	VInduct			*viduct;

	if (ductName == NULL)
	{
		PUTS("Usage: tcpclo <remote host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("tcpclo can't attach to BP", NULL);
		return 1;
	}

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in tcpclo.", NULL);
		return 1;
	}

	findOutduct("tcp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such tcp duct.", ductName);
		MRELEASE(buffer);
		return 1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		MRELEASE(buffer);
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	if (protocol.nominalRate == 0)
	{
		vduct->xmitThrottle.nominalRate = DEFAULT_TCP_RATE;
	}
	else
	{
		vduct->xmitThrottle.nominalRate = protocol.nominalRate;
	}

	memset((char *) outflows, 0, sizeof outflows);
	outflows[0].outboundBundles = duct.bulkQueue;
	outflows[1].outboundBundles = duct.stdQueue;
	outflows[2].outboundBundles = duct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(tcpcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);
#ifndef mingw
	isignal(SIGPIPE, handleConnectionLoss);
#endif

	/*	Start the keepalive thread for the eventual connection.	*/
	
	keepalivePeriod = tcpDesiredKeepAlivePeriod = KEEPALIVE_PERIOD;
	parms.cloRunning = &running;
	pthread_mutex_init(&mutex, NULL);
	parms.mutex = &mutex;
	istrcpy(parms.protocolName, protocol.name, MAX_CL_PROTOCOL_NAME_LEN);
	istrcpy(parms.ductName, ductName, MAX_CL_DUCT_NAME_LEN);
	parms.ductSocket = &ductSocket;
	parms.keepalivePeriod = &keepalivePeriod;
	if (pthread_begin(&keepaliveThread, NULL, sendKeepalives, &parms))
	{
		putSysErrmsg("tcpclo can't create keepalive thread", NULL);
		MRELEASE(buffer);
		pthread_mutex_destroy(&mutex);
		return 1;
	}

	// Returns the VInduct Object of first induct with same protocol
	// as the outduct. The VInduct is required to create an acq area.
	// The Acq Area inturn uses the throttle information from VInduct
	// object while receiving bundles. The throttle information 
	// of all inducts of the same induct will be the same, so choosing 
	// any induct will serve the purpose.
	
	findVInduct(&viduct,protocol.name);
	if(viduct == NULL)
	{
		putErrmsg("tcpclo can't get VInduct", NULL);
		MRELEASE(buffer);
		pthread_mutex_destroy(&mutex);
		return 1;
	
	}

	rparms.vduct =  viduct;
	rparms.bundleSocket = &ductSocket;
	rparms.mutex = &mutex;
	rparms.cloRunning = &running;
	if (pthread_begin(&receiverThread, NULL, receiveBundles, &rparms))
	{
		putSysErrmsg("tcpclo can't create receive thread", NULL);
		MRELEASE(buffer);
		pthread_mutex_destroy(&mutex);
		return 1;
	}

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] tcpclo is running....");
	while (running && !(sm_SemEnded(tcpcloSemaphore(NULL))))
	{
		if (bpDequeue(vduct, outflows, &bundleZco, &extendedCOS,
				destDuctName, 0, -1) < 0)
		{
			running = 0;	/*	Terminate CLO.		*/
			continue;
		}

		if (bundleZco == 0)	/*	Interrupted.		*/
		{
			continue;
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		pthread_mutex_lock(&mutex);
		bytesSent = sendBundleByTCPCL(protocol.name, ductName,
				&ductSocket, bundleLength, bundleZco,
				buffer, &keepalivePeriod);
		pthread_mutex_unlock(&mutex);
		if(bytesSent < 0)
		{
			running = 0;	/*	Terminate CLO.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeMemo("[i] tcpclo done sending");
	if (sendShutDownMessage(&ductSocket, SHUT_DN_NO, -1) < 0)
	{
		putErrmsg("Sending Shutdown message failed!!",NULL);
	}

	running = 0;
	pthread_join(keepaliveThread, NULL);
	closeOutductSocket(&ductSocket);
	writeMemo("[i] tcpclo keepalive thread killed");

	pthread_join(receiverThread, NULL);
	writeMemo("[i] tcpclo receiver thread killed");

	writeErrmsgMemos();
	writeMemo("[i] tcpclo duct has ended.");
	MRELEASE(buffer);
	pthread_mutex_destroy(&mutex);
	bp_detach();
	return 0;
}
