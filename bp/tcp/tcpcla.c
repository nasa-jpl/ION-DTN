/*
	tcpcla.c:	ION TCP convergence-layer adapter daemon.
			Handles both transmission and reception.

	Author: Scott Burleigh, JPL

	Copyright (c) 2015, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "bpP.h"

#define	BpTcpDefaultPortNbr	4556
#define	TCPCLA_BUFSZ		(64 * 1024)

#define TCPCLA_MAGIC 		"dtn!"
#define TCPCLA_MAGIC_SIZE	4
#define TCPCLA_ID_VERSION	0x03
#define TCPCLA_FLAGS		0x00
#define TCPCLA_TYPE_DATA	0x01
#define TCPCLA_TYPE_ACK		0x02
#define TCPCLA_TYPE_REF_BUN	0x03
#define TCPCLA_TYPE_KEEP_AL	0x04
#define TCPCLA_TYPE_SHUT_DN	0x05
#define SHUT_DN_BUFSZ		32
#define SHUT_DN_DELAY_FLAG	0x01
#define SHUT_DN_REASON_FLAG	0x02
#define SHUT_DN_NO	 	0
#define SHUT_DN_IDLE		1
#define SHUT_DN_IDLE_HEX	0x00
#define SHUT_DN_VER		2
#define SHUT_DN_VER_HEX		0x01
#define SHUT_DN_BUSY		3
#define SHUT_DN_BUSY_HEX	0x02

#ifndef KEEPALIVE_INTERVAL
#define KEEPALIVE_INTERVAL	(15)
#endif
#ifndef IDLE_SHUTDOWN_INTERVAL
#define IDLE_SHUTDOWN_INTERVAL	(60)
#endif
#ifndef MAX_RECONNECT_INTERVAL
#define MAX_RECONNECT_INTERVAL	(3600)
#endif
#define	TCPCLA_SEGMENT_ACKS	(1)
#define	TCPCLA_REACTIVE		(0)
#define	TCPCLA_REFUSALS		(0)
#define	TCPCLA_LENGTH_MSGS	(0)

typedef struct
{
	VOutduct		*vduct;
	int			sock;
	pthread_t		sender;
	pthread_t		receiver;
	char			*eid;		/*	Remote node ID.	*/
	vast			bundleLength;	/*	Curr. outbound.	*/
	vast			lengthAcked;	/*	Curr. outbound.	*/
	int			keepaliveInterval;
	int			secUntilKeepalive;
	int			secUntilShutdown;	/*	(idle)	*/
	int			secUntilReconnect;
	int			mustDelete;	/*	Boolean		*/
	int			segmentAcks;	/*	Boolean		*/
	int			reactiveFrags;	/*	Boolean		*/
	int			bundleRefusals;	/*	Boolean		*/
	int			lengthMessages;	/*	Boolean		*/
	vast			lengthReceived;	/*	Curr. inbound.	*/
	sm_SemId		*eobSemaphore;	/*	End of bundle.	*/
} TcpclConnection;

#ifndef mingw
extern void	handleConnectionLoss();
#endif

static void	interruptThread()
{
	isignal(SIGTERM, interruptThread);
	ionKillMainThread("tcpcla");
}

static int	addConnection(Lyst connections, pthread_mutex_t &connectionLock,
			int newSocket, struct sockaddr *socketName)
{
	TcpclConnection	*connection;
	LystElt		connectionElt;
	ReceiverParms	*rtp;
	SenderParms	*stp;

	pthread_mutex_lock(connectionsLock);
	connection = (ReceiverThreadParms *)
			MTAKE(sizeof(ReceiverThreadParms));
	if (connection == NULL)
	{
		putErrmsg("tcpcla can't allocate new connection", NULL);
		closesocket(newSocket);
		pthread_mutex_unlock(connectionsLock);
		return -1;
	}

	connectionElt = lyst_insert_last(connections, connection);
	if (connectionElt == NULL)
	{
		putErrmsg("tcpcla can't allocate lyst element for new \
connection", NULL);
		MRELEASE(rtp);
		closesocket(newSocket);
		pthread_mutex_unlock(connectionsLock);
		return -1;
	}

	rtp = (ReceiverThreadParms *)
			MTAKE(sizeof(ReceiverThreadParms));
	if (rtp == NULL)
	{
		lyst_delete(connectionElt);
		MRELEASE(connection);
		putErrmsg("tcpcla can't allocate new receiver parms", NULL);
		closesocket(newSocket);
		pthread_mutex_unlock(connectionsLock);
		return -1;
	}

	stp = (SenderThreadParms *)
			MTAKE(sizeof(SenderThreadParms));
	if (stp == NULL)
	{
		MRELEASE(rtp);
		lyst_delete(connectionElt);
		MRELEASE(connection);
		putErrmsg("tcpcla can't allocate new receiver parms", NULL);
		closesocket(newSocket);
		pthread_mutex_unlock(connectionsLock);
		return -1;
	}

/*	Populate connection object, starting sender and receiver
	threads.  Send contact header, receive contact header.
	If eid is already in connections list, send shutdown message
	and destroy the connection; otherwise, negotiate connection
	parameters and return 0.					*/

/*
	connection->bundleSocket = newSocket;
	connection->cloSocketName = cloSocketName;
	etc.
	if (pthread_begin(&(connection->receiver), NULL, receiveBundles,
				parms))
	{
		putSysErrmsg("tcpcla can't create new receiver thread", NULL);
		MRELEASE(parms);
		closesocket(newSocket);
		ionKillMainThread(procName);
		stp->running = 0;
		continue;
	}
*/
	pthread_mutex_unlock(connectionsLock);
	return 0;
}

/*	*	*	Sender thread functions		*	*	*/

typedef struct
{
	int		*running;
	TcpclConnection	*connection;
	pthread_mutex_t	connectionsLock;
} SenderThreadParms;

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	int		*running;
	TcpclConnection	*connection;
	pthread_mutex_t	connectionsLock;
} ReceiverThreadParms;

/*	*	*	Server thread functions		*	*	*/

typedef struct
{
	int		*running;
	int		serverSocket;
	Lyst		connections;
	pthread_mutex_t	connectionsLock;
} ServerThreadParms;

static void	*spawnReceivers(void *parm)
{
	/*	Main loop for acceptance of connections and
	 *	creation of threads to service those connections.	*/

	ServerThreadParms	*stp = (ServerThreadParms *) parm;
	char			*procName = "tcpcla";
	int			newSocket;
	struct sockaddr		cloSocketName;
	socklen_t		nameLength;
	ReceiverThreadParms	*parms;
	LystElt			elt;
	pthread_t		thread;

	snooze(1);	/*	Let main thread become interruptable.	*/

	/*	Can now begin accepting connections from remote
	 *	contacts.  On failure, take down the whole CLA.		*/

	while (stp->running)
	{
		nameLength = sizeof(struct sockaddr);
		newSocket = accept(stp->ductSocket, &cloSocketName,
				&nameLength);
		if (newSocket < 0)
		{
			putSysErrmsg("tcpcla accept() failed", NULL);
			ionKillMainThread(procName);
			stp->running = 0;
			continue;
		}

		if (stp->running == 0)
		{
			closesocket(newSocket);
			break;	/*	Main thread has shut down.	*/
		}

		if (addConnection(connections, connectionLock, newSocket,
				cloSocketName) < 0)
		{
			putErrmsg("tcpcla server can't add connection.", NULL);
			ionKillMainThread(procName);
			stp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] tcpcla server thread has ended.");
	return NULL;
}

/*	*	*	Clock thread functions		*	*	*/

typedef struct
{
	int		*running;
	Lyst		connections;
	pthread_mutex_t	connectionsLock;
} ClockThreadParms;

/*	Once per second, snooze(1) and then loop through all
	connections.  Shut down any that are ready to stop.  For
	others, decrement remaining time as applicable and handle
	when remaining time is zero.  Every N seconds, loop through
	all egress plans and attempt to make new connections for
	all plans whose neighbor nodes are not in the connections
	list but that have any directive (default or rule) that
	cites a tcpcl socket spec.					*/

/*	*	*	Main thread functions		*	*	*/

static void	stopConnection(TcpclConnection *connection)
{
	sm_SemEnd(connection->vduct->semaphore);
	sm_SemEnd(connection->eobSemaphore);
	pthread_join(connection->senderThread, NULL);
	sm_SemDelete(connection->eobSemaphore);
	closesocket(connection->sock);
	pthread_join(connection->receiverThread, NULL);
	MRELEASE(connection->eid);
	MRELEASE(connection);
}

static void	shutDownConnections(pthread_mutex_t *connectionsLock,
			Lyst connections)
{
	LystElt	elt;

	pthread_mutex_lock(connectionsLock);
	for (elt = lyst_first(connections; elt;)
	{
		stopConnection(lyst_data(elt));
		lyst_delete(elt);
	}

	pthread_mutex_unlock(connectionsLock);
}

static void	stopServerThread(pthread_t serverThread, ServerThreadParms *stp)
{
	int	fd;

	/*	Wake up the server thread by connecting to it.		*/

	stp->running = 0;
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd >= 0)
	{
		oK(connect(fd, &(stp->socketName), sizeof(struct sockaddr)));

		/*	Immediately discard the connected socket.	*/

		closesocket(fd);
	}

	pthread_join(serverThread, NULL);
}

#if defined (ION_LWT)
int	tcpcla(int a1, int a2, int a3, int a4, int a5,
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
	unsigned short		portNbr;
	unsigned int		hostNbr;
	ServerThreadParms	stp;
	socklen_t		nameLength;
	Lyst			connections;
	pthread_mutex_t		connectionsLock;
	pthread_t		serverThread;
	pthread_t		clockThread;

	if (ductName == NULL)
	{
		PUTS("Usage: tcpcla <local host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		writeMemo("[?] tcpcla can't attach to BP.");
		return 1;
	}

	if (parseSocketSpec(ductName, &portNbr, &hostNbr) != 0)
	{
		writeMemo("[?] tcpcla: can't get induct IP address.", ductName);
		return 1;
	}

	if (portNbr == 0)
	{
		portNbr = BpTcpDefaultPortNbr;
	}

	findInduct("tcp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		writeMemoNote("[?] tcpcla: no induct", ductName);
		return 1;
	}

	/*	NOTE: no outduct lookup here, because all TCPCL
	 *	outducts are created invisibly and dynamically
	 *	as connections are made.  None are created during
	 *	node configuration.					*/

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		writeMemoNote("[?] tcpcla task is already started.",
				itoa(vduct->cliPid));
		return 1;
	}

	/*	All command-line arguments are now validated, so
		begin initialization by creating the connections lyst.	*/

	connections = lyst_create();
	if (connections == NULL)
	{
		putErrmsg("tcpcla can't create lyst of connections", NULL);
		return 1;
	}

	pthread_mutex_init(&connectionsLock, NULL);

	/*	Now create the server socket.				*/

	portNbr = htons(portNbr);
	hostNbr = htonl(hostNbr);
	stp.vduct = vduct;
	memset((char *) &(stp.socketName), 0, sizeof(struct sockaddr));
	stp.inetName = (struct sockaddr_in *) &(stp.socketName);
	stp.inetName->sin_family = AF_INET;
	stp.inetName->sin_port = portNbr;
	memcpy((char *) &(stp.inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	stp.serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (stp.serverSocket < 0)
	{
		putSysErrmsg("Can't open TCP server socket", NULL);
		pthread_mutex_destroy(&connectionsLock);
		lyst_destroy(connections);
		return 1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(stp.serverSocket)
	|| bind(stp.serverSocket, &(stp.socketName), nameLength) < 0
	|| listen(stp.serverSocket, 5) < 0
	|| getsockname(stp.serverSocket, &(stp.socketName), &nameLength) < 0)
	{
		closesocket(stp.serverSocket);
		pthread_mutex_destroy(&connectionsLock);
		lyst_destroy(connections);
		putSysErrmsg("Can't initialize TCP server socket", NULL);
		return 1;
	}

	/*	Set up signal handling: SIGTERM is shutdown signal.	*/

	ionNoteMainThread("tcpcla");
	isignal(SIGTERM, interruptThread);
#ifndef mingw
	isignal(SIGPIPE, SIG_IGN);
#endif

	/*	Start the clock thread, which does initial load
	 *	of the connections lyst.				*/

	ctp.running = 1;
	if (pthread_begin(&clockThread, NULL, handleEvents, &ctp))
	{
		closesocket(stp.serverSocket);
		pthread_mutex_destroy(&connectionsLock);
		lyst_destroy(connections);
		putSysErrmsg("tcpcla can't create clock thread", NULL);
		return 1;
	}

	/*	Start the server thread.				*/

	stp.running = 1;
	if (pthread_begin(&serverThread, NULL, spawnReceivers, &stp))
	{
		shutDownConnections(connectionsLock, connections);
		ctp.running = 0;
		pthread_join(clockThread, NULL);
		closesocket(stp.serverSocket);
		pthread_mutex_destroy(&connectionsLock);
		lyst_destroy(connections);
		putSysErrmsg("tcpcla can't create server thread", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the CLA.				*/

	{
		char txt[500];

		isprintf(txt, sizeof(txt),
				"[i] tcpcla is running, [%s:%d].", 
				inet_ntoa(stp.inetName->sin_addr),
				ntohs(stp.inetName->sin_port));

		writeMemo(txt);
	}

	ionPauseMainThread(-1);

	/*	Time to shut down.					*/

	stopServerThread(serverThread, &stp);
	shutDownConnections(connectionsLock, connections);
	ctp.running = 0;
	pthread_join(clockThread, NULL);
	closesocket(stp.serverSocket);
	pthread_mutex_destroy(&connectionsLock);
	lyst_destroy(connections);
	writeErrmsgMemos();
	writeMemo("[i] tcpcla duct has ended.");
	bp_detach();
	return 0;
}
--------------------------------------------------

/*
	tcpcla.c:	BP TCP-based convergence-layer input
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
	ionKillMainThread("tcpcla");
}

/*	*	*	Keepalive thread function	*	*	*/

typedef struct
{
	int		ductSocket;
	pthread_mutex_t	*mutex;
	int 		keepalivePeriod;
	int 		*cliRunning;
        int             *receiveRunning;
} KeepaliveThreadParms;

static void	terminateKeepaliveThread(KeepaliveThreadParms *parms)
{
	writeErrmsgMemos();
	writeMemo("[i] tcpcla keepalive thread stopping.");
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
	char			*procName = "tcpcla";
	int 			count = 0;
	int 			bytesSent;
	unsigned char		*buffer;

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in tcpcla.", NULL);
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

		bytesSent = sendBundleByTCPCL("", "", &parms->ductSocket, 0, 0,
				buffer, &parms->keepalivePeriod);
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
	writeMemo("[i] tcpcla receiver thread stopping.");
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
	char			*procName = "tcpcla";
	KeepaliveThreadParms	*kparms;
	AcqWorkArea		*work;
	char			*buffer;
	pthread_t		kthread;
	int			haveKthread = 0;
	int			threadRunning = 1;

	buffer = MTAKE(TCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("tcpcla can't get TCP buffer", NULL);
		terminateReceiverThread(parms);
		MRELEASE(parms);
		ionKillMainThread(procName);
		return NULL;
	}

	work = bpGetAcqArea(parms->vduct);
	if (work == NULL)
	{
		putErrmsg("tcpcla can't get acquisition work area", NULL);
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
		putErrmsg("tcpcla can't allocate for new keepalive thread",
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

	if (sendContactHeader(&parms->bundleSocket, (unsigned char *) buffer)
			< 0)
	{
		putErrmsg("tcpcla couldn't send contact header", NULL);
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
		putErrmsg("tcpcla couldn't receive contact header", NULL);
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
		kparms->mutex = parms->mutex;
		kparms->cliRunning = parms->cliRunning;
                kparms->receiveRunning = &(parms->receiveRunning);
		/*
		 * Creating a thread to send out keep alives, which
		 * makes the TCPCL bi directional
		 */
		if (pthread_begin(&kthread, NULL, sendKeepalives, kparms))
		{
			putSysErrmsg("tcpcla can't create new thread for \
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
		if (bpBeginAcq(work, 0, NULL) < 0)
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

	parms->receiveRunning = 0;
	if (haveKthread)
	{
		/*	Wait for keepalive snooze cycle to notice that
		 *	*(parms->cliRunning) or *(parms->receiveRunning)
		 *	is now zero.					*/

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

/*	*	*	Main thread functions	*	*	*	*/
