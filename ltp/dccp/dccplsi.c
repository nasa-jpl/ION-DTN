/*
	dccplsi.c:	LTP DCCP-based link service daemon.

	Author: Samuel Jero, Ohio University

	Copyright (c) 2010.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include <config.h>

#ifndef build_dccp

#endif

#ifdef build_dccp

#include "dccplsa.h"
#include <lyst.h>

static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
}

static void siguser_thread(int signum)
{
	isignal(SIGUSR1, siguser_thread);
}

#ifndef mingw
void	handleConnectionLoss(int signum)
{
	isignal(SIGPIPE, handleConnectionLoss);
}
#endif

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	int				sock;
	pthread_t		mainThread;
	pthread_t		me;
	int				running;
	pthread_mutex_t *elk;
	Lyst			*list;
} ReceiverThreadParms;

ReceiverThreadParms* create_new_thread_data(Lyst *list)
{
	ReceiverThreadParms *tmp;

	tmp = MTAKE(sizeof(ReceiverThreadParms));
	if (tmp == NULL)
	{
		return NULL;
	}
	lyst_insert(*list, (void*)tmp);
return tmp;
}

int remove_thread(Lyst *list, ReceiverThreadParms *rtp)
{
	LystElt elmt;
	ReceiverThreadParms *r;

	if (lyst_length(*list) <= 0)
	{
		return 0;
	}

	 for (elmt = lyst_first(*list); elmt; lyst_next(elmt))
	 {
		r=(ReceiverThreadParms*)lyst_data(elmt);
		if (r->sock == rtp->sock && pthread_equal(r->me,rtp->me))
		{
			MRELEASE(r);
			lyst_delete(elmt);
			return 1;
		}
	}
return 0;
}

int no_threads(Lyst *list)
{
return (lyst_length(*list) == 0);
}

ReceiverThreadParms* get_first_thread(Lyst *list)
{
return (ReceiverThreadParms*)lyst_data(lyst_first(*list));
}

static void *Recieve_DCCP(void *param)
{
	/*	Main loop for LTP segment reception thread on one
	 *	connection, terminating when connection is lost.	*/
	ReceiverThreadParms *rtp = (ReceiverThreadParms*)param;
	char	*buffer;
	int segmentLength;
	buffer = MTAKE(DCCPLSA_BUFSZ);

	iblock(SIGTERM);
	isignal(SIGUSR1, siguser_thread);
#ifndef mingw
	isignal(SIGPIPE, handleConnectionLoss);
#endif

	if (buffer == NULL)
	{
		putErrmsg("DCCPLSI can't get DCCP buffer.", NULL);
		pthread_kill(rtp->mainThread, SIGTERM);
		return NULL;
	}

	/*	Can now start receiving segments.  On failure, take
	 *	down just this thread				*/
	while (rtp->running)
	{
		segmentLength = irecv(rtp->sock, buffer, DCCPLSA_BUFSZ, 0);
		if (segmentLength < 0)
		{
			if (errno == EAGAIN ||errno == EINTR)
			{
				continue;
			}
			
			putErrmsg("DCCPLSI recv() call failed.", NULL);
			rtp->running = 0;
			continue;
			/* Take down this thread		*/
		}

		if (segmentLength == 0) /* EOF 		*/
		{
			rtp->running = 0;
			continue;
			/* Take down this thread		*/
		}

		if (rtp->running == 0)
		{
			continue;
			 /* shutdown from accept thread */
			/* Take down this thread		*/
		}

		if (segmentLength == 4)
		{
			/*Keepalive*/
			continue;
		}

		pthread_mutex_lock(rtp->elk);
		if (ltpHandleInboundSegment(buffer, segmentLength) < 0)
		{
			putErrmsg("DCCPLSI can't handle inbound segment.", NULL);
			pthread_mutex_unlock(rtp->elk);
			rtp->running = 0;
			continue;
			/* Take down just this thread	*/
		} else {
			pthread_mutex_unlock(rtp->elk);
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	close(rtp->sock);
	pthread_mutex_lock(rtp->elk);
	remove_thread(rtp->list,rtp);
	pthread_mutex_unlock(rtp->elk);
	writeErrmsgMemos();
	MRELEASE(buffer);
return NULL;
}

/*	*	*	Listener thread functions	*	*	*/
typedef struct
{
	int			linkSocket;
	pthread_t	mainThread;
	int			running;
} ListenerThreadParms;

static void	*Listen_for_connections(void *parm)
{
	/*	Main loop for DCCP connection handling			*/
	ListenerThreadParms	*rtp = (ListenerThreadParms *) parm;
	int			consock;
	struct sockaddr		fromAddr;
	socklen_t		solen;
	ReceiverThreadParms	*rp;
	pthread_mutex_t 	elk;
	Lyst			list;


	list = lyst_create_using(getIonMemoryMgr());
	lyst_clear(list);
	pthread_mutex_init(&elk, NULL);
	
	iblock(SIGTERM);
	isignal(SIGUSR1, siguser_thread);
#ifndef mingw
	iblock(SIGPIPE);
#endif

	/*	Can now begin accepting connections from remote
	 *	contacts.  On failure, take down the whole LSI.		*/
	while (rtp->running)
	{	
		solen = sizeof(fromAddr);
		consock = accept(rtp->linkSocket, &fromAddr, &solen);
		if (consock < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			putSysErrmsg("DCCPLSI accept() failed.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
			/*	Take Down CLI				*/
		}

		if (rtp->running == 0)
		{
			continue;
		}

		/* start new thread to handle new connection 		*/
		pthread_mutex_lock(&elk);
		rp = create_new_thread_data(&list);
		pthread_mutex_unlock(&elk);
		if (rp == NULL)
		{
			putSysErrmsg("DCCPLSI can't allocate thread data structures.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}
		rp->sock = consock;
		rp->mainThread = rtp->mainThread;
		rp->running = 1;
		rp->elk = &elk;
		rp->list = &list;
		if (pthread_begin(&rp->me, NULL, Recieve_DCCP, rp, "dccplsi_receiver"))
		{
			putSysErrmsg("DCCPLSI can't create receiver thread.", NULL);
			close(consock);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/* Exit. End All Receiver Threads Properly.			*/
	while (!no_threads(&list))
	{
		pthread_mutex_lock(&elk);
		rp=get_first_thread(&list);
		pthread_mutex_unlock(&elk);
		if (rp == NULL)
		{
			putSysErrmsg("DCCPLSI can't terminate all threads nicely.", NULL);
			return NULL;
		}
		rp->running = 0;
		pthread_kill(rp->me, SIGUSR1);
		pthread_join(rp->me, NULL);
	}
	pthread_mutex_destroy(&elk);
	lyst_destroy(list);
	writeErrmsgMemos();
	writeMemo("[i] dccplsi listener thread has ended.");
	return NULL;
}


/*	*	*	Main thread functions	*	*	*	*/


int bindDCCPsock(int* sock, struct sockaddr* socketName)
{
	socklen_t nameLength;

	if (sock == NULL || socketName == NULL)
	{
		return -1;
	}

	if ((*sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP)) < 0 )
	{
		putSysErrmsg("DCCPLSI can't open DCCP socket. This probably means DCCP is not supported on your system.", NULL);
		return -1;
	}

	if (reUseAddress(*sock) < 0)
	{
		putSysErrmsg("DCCPLSI can't initialize socket.", "SO_REUSEADDR");
		return -1;
	}

	nameLength = sizeof(struct sockaddr);
	if (bind(*sock, socketName, nameLength) < 0)
	{
		putSysErrmsg("DCCPLSI can't initialize socket.", "bind()");
		return -1;
	}

	if (listen(*sock, DCCP_MAX_CON) < 0)
	{
		putSysErrmsg("DCCPLSI can't initialize socket.", "listen()");
		return -1;
	}
return 0;
}

#if defined (ION_LWT)
int	dccplsi(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*endpointSpec = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*endpointSpec = (argc > 1 ? argv[1] : NULL);
#endif
	LtpVdb			*vdb;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	ListenerThreadParms	rtp;
	pthread_t		listenerThread;

	/*	Note that ltpadmin must be run before the first
	 *	invocation of dccplsi, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/
	if (ltpInit(0) < 0)
	{
		putErrmsg("dccplsi can't initialize LTP.", NULL);
		return 1;
	}

	vdb = getLtpVdb();
	if (vdb->lsiPid != ERROR && vdb->lsiPid != sm_TaskIdSelf())
	{
		putErrmsg("DCCPLSI task is already started.", itoa(vdb->lsiPid));
		return 1;
	}

	/*	All command-line arguments are now validated.		*/
	if (parseSocketSpec(endpointSpec, &portNbr, &ipAddress) != 0)
	{
		putErrmsg("Can't get IP/port for host.", endpointSpec);
		return 1;
	}

	if (portNbr == 0)
	{
		portNbr = LtpDccpDefaultPortNbr;
	}
	portNbr = htons(portNbr);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	ipAddress = htonl(ipAddress);
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);

	if (bindDCCPsock(&rtp.linkSocket, &socketName) < 0)
	{
		close(rtp.linkSocket);
		return 1;
	}

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/
	isignal(SIGTERM, interruptThread);
#ifndef mingw
	iblock(SIGPIPE);
#endif

	/*	Start the receiver thread.				*/
	rtp.running = 1;
	rtp.mainThread = pthread_self();
	if (pthread_begin(&listenerThread, NULL, Listen_for_connections,
		&rtp, "dccplsi_listener"))
	{
		close(rtp.linkSocket);
		putSysErrmsg("DCCPLSI can't create listener thread.", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the link service.			*/
	writeMemo("[i] dccplsi is running.");
	ionPauseMainThread(-1);

	/*	Time to shut down.					*/
	rtp.running = 0;

	/*	Wake up the receiver thread and exit			*/
	pthread_kill(listenerThread, SIGUSR1);
	writeErrmsgMemos();
	pthread_join(listenerThread, NULL);
	close(rtp.linkSocket);
	writeErrmsgMemos();
	writeMemo("[i] dccplsi duct has ended.");
	return 0;
}

#else /*build_dccp*/

#include "ltpP.h"
#if defined (ION_LWT)
int	dccplsi(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif	
putErrmsg("dccplsi (and the DCCP protocol) are only available under Linux (>=3.2.0). Please see the README in the ltp/dccp source directory for more information.", NULL);
writeErrmsgMemos();
return 0;
}

#endif /*build_dccp*/
