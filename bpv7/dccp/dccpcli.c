/*
	dccpcli.c:	BP DCCP-based convergence-layer input
			daemon, designed to serve as an input
			duct.
	Author: Samuel Jero, Ohio University

	Copyright (c) 2010.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	This convergence-layer does not provide ANY
	fragmentation support. Maximum packet size is
	link MTU.
									*/
#include <config.h>

#ifdef build_dccp


#include "dccpcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"
#include <lyst.h>

static void	interruptThread(int signum)
{
	isignal(SIGTERM, interruptThread);
}

static void siguser_thread(int signum)
{
	isignal(SIGUSR1, siguser_thread);
}

/*	*	*	Reciever thread functions	*	*	*/

typedef struct
{
	int			sock;
	pthread_t		mainThread;
	pthread_t		me;
	int			running;
	struct sockaddr_in 	fromAddr;
	VInduct			*vduct;
	pthread_mutex_t 	*elk;
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
	if (lyst_insert(*list, (void*)tmp) == NULL)
	{
		return NULL;
	}
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
		r = (ReceiverThreadParms*)lyst_data(elmt);
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

int bindDCCPsock(int* sock, struct sockaddr* socketName)
{
	socklen_t nameLength;

	if (sock == NULL || socketName == NULL)
	{
		return -1;
	}

	if ((*sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP)) < 0 )
	{
		putSysErrmsg("dccpcli can't open DCCP socket. This probably means DCCP is not supported on your system.", NULL);
		return -1;
	}

	if (reUseAddress(*sock) < 0)
	{
		putSysErrmsg("dccpcli can't initialize socket.", "reuse");
		return -1;
	}

	nameLength = sizeof(struct sockaddr);
	if (bind(*sock, socketName, nameLength) < 0)
	{
		putSysErrmsg("dccpcli can't initialize socket.", "bind()");
		return -1;
	}

	if (listen(*sock, DCCP_MAX_CON) < 0)
	{
		putSysErrmsg("dccpcli can't initialize socket.", "listen()");
		return -1;
	}
return 0;
}

static void *Recieve_DCCP(void *param)
{
	/*	Main loop for bundle reception thread on one
	 *	connection, terminating when connection is lost.	*/
	ReceiverThreadParms 	*rtp = (ReceiverThreadParms*)param;
	char			*buffer;
	AcqWorkArea		*work;
	int 			bundleLength;
	
	iblock(SIGTERM);
	isignal(SIGUSR1, siguser_thread);
#ifndef mingw
	isignal(SIGPIPE, itcp_handleConnectionLoss);
#endif

	/* Get buffers							*/
	writeErrmsgMemos();
	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		putErrmsg("dccpcli can't get acquisition work area.", NULL);
		pthread_kill(rtp->mainThread, SIGTERM);
		return NULL;
	}

	buffer = MTAKE(DCCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		pthread_kill(rtp->mainThread, SIGTERM);
		return NULL;
	}

	/*	Can now start receiving bundles.  On failure, take
	 *	down just this thread					*/
	while (rtp->running)
	{
		bundleLength = irecv(rtp->sock, buffer, DCCPCLA_BUFSZ, 0);
		if (bundleLength < 0)
		{
			if (errno == EAGAIN ||errno == EINTR)
			{
				continue;
			}
			
			putErrmsg("dccpcli recv() call failed.", NULL);
			rtp->running = 0;
			continue;
			/* Take down this thread			*/
		}

		if (bundleLength == 0) /*EOF 			*/
		{
			rtp->running = 0;
			continue;
			/* Take down this thread			*/
		}

		if (rtp->running == 0)
		{
			 /* shutdown from accept thread */
			continue;
			/* Take down this thread			*/
		}

		if (bundleLength == 4)
		{
			/*Keepalive*/
			continue;
		}

		pthread_mutex_lock(rtp->elk);
		if (bpBeginAcq(work, 0, NULL) < 0 
		|| bpContinueAcq(work, buffer, bundleLength, 0, 0) < 0
		|| bpEndAcq(work) < 0)
		{
			putErrmsg("Can't acquire bundle.", NULL);
			pthread_mutex_unlock(rtp->elk);
			rtp->running = 0;
			continue;
			/* Take down this thread			*/
		}
		pthread_mutex_unlock(rtp->elk);

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
		
	}

	close(rtp->sock);
	pthread_mutex_lock(rtp->elk);
	remove_thread(rtp->list,rtp);
	pthread_mutex_unlock(rtp->elk);
	writeErrmsgMemos();
	MRELEASE(buffer);
	bpReleaseAcqArea(work);
	return NULL;
}

/*	*	*	Listener thread functions	*	*	*/
typedef struct
{
	int		linkSocket;
	VInduct		*vduct;
	pthread_t	mainThread;
	int		running;
} ListenerThreadParms;


static void	*Listen_for_connections(void *parm)
{
	/*	Main loop for DCCP connection handling			*/
	ListenerThreadParms	*rtp = (ListenerThreadParms *) parm;
	Lyst			list;
	pthread_mutex_t 	elk;
	struct sockaddr		fromAddr;
	int			consock;
	socklen_t		solen;
	ReceiverThreadParms	*rp;

	list = lyst_create_using(getIonMemoryMgr());
	lyst_clear(list);
	pthread_mutex_init(&elk, NULL);
	
	iblock(SIGTERM);
#ifndef mingw
	iblock(SIGPIPE);
#endif
	isignal(SIGUSR1, siguser_thread);

	/*	Can now begin accepting connections from remote
	 *	contacts.  On failure, take down the whole CLI.		*/
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
			putSysErrmsg("dccpcli accept() failed.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
			/*	Take Down CLI */
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
			putSysErrmsg("dccpcli can't allocate memory for new thread.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}
		rp->sock = consock;
		memcpy(&rp->fromAddr, &fromAddr, sizeof(struct sockaddr));
		rp->mainThread = rtp->mainThread;
		rp->running = 1;
		rp->elk = &elk;
		rp->list = &list;
		rp->vduct = rtp->vduct;
		if (pthread_begin(&rp->me, NULL, Recieve_DCCP, rp))
		{
			putSysErrmsg("dccpcli can't create new thread.", NULL);
			close(consock);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}

		/* Make sure other tasks have a chance to run.		*/
		sm_TaskYield();
	}

	/* Exit. End All Receiver Threads Properly.			*/
	while (!no_threads(&list)){
		rp = get_first_thread(&list);
		if (rp == NULL)
		{
			putSysErrmsg("dccpcli can't terminate all threads nicely.", NULL);
			return NULL;
		}
		rp->running = 0;
		pthread_kill(rp->me, SIGUSR1);
		pthread_join(rp->me, NULL);
	}
	pthread_mutex_destroy(&elk);
	lyst_destroy(list);
	writeErrmsgMemos();
	writeMemo("[i] dccpcli listener thread has ended.");
	return NULL;
}


/*	*	*	Main thread functions	*	*	*	*/
#if defined (ION_LWT)
int	dccpcli(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr			sdr;
	VInduct			*vduct;
	PsmAddress		vductElt;
	Induct			duct;
	ClProtocol		protocol;
	char			*hostName;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	ListenerThreadParms	rtp;
	pthread_t		listenerThread;

	if (ductName == NULL)
	{
		PUTS("Usage: dccpcli <local host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("dccpcli can't attach to BP.", NULL);
		return 1;
	}

	findInduct("dccp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such dccp duct.", ductName);
		return 1;
	}

	if (vduct->cliPid != ERROR && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("dccpcli task is already started for this duct.",
					itoa(vduct->cliPid));
		return 1;
	}
	
	/*	All command-line arguments are now validated.
	 * 	Get Data induct data structures from SDR 		*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);

	/* get my host and port						*/
	hostName = ductName;
	if (parseSocketSpec(ductName, &portNbr, &ipAddress) != 0)
	{
		putErrmsg("Can't get IP/port for host.", hostName);
		return 1;
	}
	if (portNbr == 0)
	{
		portNbr = BpDccpDefaultPortNbr;
	}

	portNbr = htons(portNbr);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	ipAddress = htonl(ipAddress);
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);

	rtp.vduct=vduct;
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
	if (pthread_begin(&listenerThread, NULL, Listen_for_connections, &rtp))
	{
		close(rtp.linkSocket);
		putSysErrmsg("dccpcli can't create new thread.", NULL);
		return 1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the link service.			*/
	writeMemo("[i] dccpcli is running.");
	ionPauseMainThread(-1);

	/*	Time to shut down.					*/
	rtp.running = 0;

	/*	Wake up the receiver thread and exit			*/
	pthread_kill(listenerThread, SIGUSR1);
	writeErrmsgMemos();
	pthread_join(listenerThread, NULL);
	close(rtp.linkSocket);
	writeErrmsgMemos();
	writeMemo("[i] dccpcli duct has ended.");
	return 0;
}

#else /*build_dccp*/

#include "bpP.h"
#if defined (ION_LWT)
int	dccpcli(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif	
putErrmsg("dccpcli (and the DCCP protocol) are only available under Linux (>=3.2.0). Please see the README in the bp/dccp source directory for more information.", NULL);
writeErrmsgMemos();
return 0;
}

#endif /*build_dccp*/
