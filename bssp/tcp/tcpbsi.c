/*
 *	tcpbsi.c:	BSSP TCP-based link service daemon..
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *		 Scott Burleigh, JPL
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *
 */
#include "tcpbsa.h"

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("tcpbsi");
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	LystElt		elt;
	pthread_mutex_t	*mutex;
	int		blockSocket;
	pthread_t	thread;
	int		*running;
} ReceiverThreadParms;

static void	terminateReceiverThread(ReceiverThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] tcpbsi receiver thread stopping.");
	pthread_mutex_lock(parms->mutex);
	if (parms->blockSocket != -1)
	{
		closesocket(parms->blockSocket);
		parms->blockSocket = -1;
	}

	lyst_delete(parms->elt);
	pthread_mutex_unlock(parms->mutex);
	MRELEASE(parms);
}

static void	*receiveBlocks(void *parm)
{
	/*	Main loop for bundle reception thread on one
	 *	connection, terminating when connection is lost.	*/

	ReceiverThreadParms	*parms = (ReceiverThreadParms *) parm;
	char			*procName = "tcpbsi";
	int			blockLength;
	char			*buffer;
	int			threadRunning = 1;

	buffer = MTAKE(TCPBSA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("tcpbsi can't get TCP buffer.", NULL);
		ionKillMainThread(procName);
		terminateReceiverThread(parms);
		return NULL;
	}

	/*	Now start receiving blocks.				*/

	while (threadRunning && *(parms->running))
	{
		switch (irecv(parms->blockSocket, (char *) &blockLength, 4, 0))
		{
		case -1:
			if (errno != EINTR)	/*	Shutdown.	*/
			{	
				putErrmsg("tcpbsi read() error on socket. \
					TCP-BSI can't acquire block.", NULL);
			}

			/*	Intentional fall-through to next case.	*/
		case 0:	
			threadRunning = 0;
			continue;

		default:
			break;
		}

		if (blockLength == 0)	/*	Just a keep-alive.	*/
		{
			continue;
		}

		switch (irecv(parms->blockSocket, buffer, blockLength, 0))
		{
		case -1:
			if (errno != EINTR)	/*	Shutdown.	*/
			{	
				putErrmsg("tcpbsi read() error on socket. \
					TCP-BSI can't acquire block.", NULL);
			}

			/*	Intentional fall-through to next case.	*/
		case 0:	
			threadRunning = 0;
			continue;

		default:
			break;
		}

		if (bsspHandleInboundBlock(buffer, blockLength) < 0)
		{
			putErrmsg("tcpbsi can't handle inbound block.", NULL);
			ionKillMainThread(procName);
			threadRunning = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	/*	End of receiver thread; release resources.		*/

	MRELEASE(buffer);
	terminateReceiverThread(parms);
	return NULL;
}

/*	*	*	Access thread functions	*	*	*	*/

typedef struct
{
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
	char			*procName = "tcpbsi";
	pthread_mutex_t		mutex;
	Lyst			threads;
	int			newSocket;
	struct sockaddr		bsoSocketName;
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
		putErrmsg("tcpbsi can't create threads list.", NULL);
		ionKillMainThread(procName);
		pthread_mutex_destroy(&mutex);
		return NULL;
	}

	/*	Can now begin accepting connections from remote
	 *	contacts.  On failure, take down the whole BSI.		*/

	while (atp->running)
	{
		nameLength = sizeof(struct sockaddr);
		newSocket = accept(atp->ductSocket, &bsoSocketName,
				&nameLength);
		if (newSocket < 0)
		{
			putSysErrmsg("tcpbsi accept() failed", NULL);
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
			putErrmsg("tcpbsi can't allocate for thread.", NULL);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		pthread_mutex_lock(&mutex);
		parms->elt = lyst_insert_last(threads, parms);
		pthread_mutex_unlock(&mutex);
		if (parms->elt == NULL)
		{
			putErrmsg("tcpbsi can't allocate for thread.", NULL);
			MRELEASE(parms);
			ionKillMainThread(procName);
			atp->running = 0;
			continue;
		}

		parms->mutex = &mutex;
		parms->blockSocket = newSocket;
       		fromAddr = (struct sockaddr_in *) &bsoSocketName;
		memcpy((char *) &hostNbr,
				(char *) &(fromAddr->sin_addr.s_addr), 4);
		hostNbr = ntohl(hostNbr);
		printDottedString(hostNbr, hostName);
		parms->running = &(atp->running);
		if (pthread_begin(&(parms->thread), NULL, receiveBlocks,
					parms))
		{
			putSysErrmsg("tcpbsi can't create new thread", NULL);
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

	/*	Shut down all current BSI threads cleanly.		*/

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
#ifdef mingw
		shutdown(parms->blockSocket, SD_BOTH);
#else
		pthread_kill(thread, SIGTERM);
#endif
		pthread_mutex_unlock(&mutex);
		pthread_join(thread, NULL);
	}

	lyst_destroy(threads);
	writeErrmsgMemos();
	writeMemo("[i] tcpbsi access thread has ended.");
	pthread_mutex_destroy(&mutex);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
int	tcpbsi(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	
	BsspVdb			*vdb;
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
		PUTS("Usage: tcpbsi <local host name>[:<port number>]");
		return 0;
	}

	if (bsspInit(0) < 0)
	{
		putErrmsg("tcpbsi can't initialize BSSP.", NULL);
		return 1;
	}

	vdb = getBsspVdb();
	if (vdb->rlBsiPid != ERROR && vdb->rlBsiPid != sm_TaskIdSelf())
	{
		putErrmsg("TCP-BSI task is already started.", itoa(vdb->rlBsiPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/

	hostName = ductName;
	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		putErrmsg("TCP-BSI can't get IP/port for host.", hostName);
		return -1;
	}

	if (portNbr == 0)
	{
		portNbr = bsspTcpDefaultPortNbr;
	}

	portNbr = htons(portNbr);
	hostNbr = htonl(hostNbr);
	memset((char *) &(atp.socketName), 0, sizeof(struct sockaddr));
	atp.inetName = (struct sockaddr_in *) &(atp.socketName);
	atp.inetName->sin_family = AF_INET;
	atp.inetName->sin_port = portNbr;
	memcpy((char *) &(atp.inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	atp.ductSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (atp.ductSocket < 0)
	{
		putSysErrmsg("TCP-BSI can't open TCP socket", NULL);
		return 1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(atp.ductSocket)
	|| bind(atp.ductSocket, &(atp.socketName), nameLength) < 0
	|| listen(atp.ductSocket, 5) < 0
	|| getsockname(atp.ductSocket, &(atp.socketName), &nameLength) < 0)
	{
		closesocket(atp.ductSocket);
		putSysErrmsg("TCP-BSI can't initialize socket", NULL);
		return 1;
	}

	tcpDelayString = getenv("TCP_DELAY_NSEC_PER_BYTE");
	if (tcpDelayString == NULL)
	{
		tcpDelayEnabled = 0;
	}
	else	/*	Artificial TCP delay, for testing purposes.	*/
	{
		tcpDelayEnabled = 1;
		tcpDelayNsecPerByte = strtol(tcpDelayString, NULL, 0);
		if (tcpDelayNsecPerByte < 0
		|| tcpDelayNsecPerByte > 16384)
		{
			tcpDelayNsecPerByte = 0;
		}
	}

	/*	Set up signal handling: SIGTERM is shutdown signal.	*/

	ionNoteMainThread("tcpbsi");
	isignal(SIGTERM, interruptThread);

	/*	Start the access thread.				*/

	atp.running = 1;
	if (pthread_begin(&accessThread, NULL, spawnReceivers, &atp))
	{
		closesocket(atp.ductSocket);
		putSysErrmsg("tcpbsi can't create access thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	{
		char	txt[500];

		isprintf(txt, sizeof(txt),
			"[i] tcpbsi is running, spec=[%s:%d].", 
			inet_ntoa(atp.inetName->sin_addr), ntohs(portNbr));
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
	writeMemo("[i] tcpbsi duct has ended.");
	ionDetach();
	return 0;
}
