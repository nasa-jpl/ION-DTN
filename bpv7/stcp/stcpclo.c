/*
	stcpclo.c:	BP TCP-based convergence-layer output
			daemon.  Note that this convergence-layer
			output daemon is a "dedicated" CLO daemon
			suitable only for a limited number of paths,
			because it manages just a single TCP
			connection.

			Promiscuous CLO daemons need to be based on
			UDP, Dgr, etc.

	Author: Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "stcpcla.h"
#include "ipnfw.h"

#define	MAX_RECONNECT_PAUSE	(30)

static sm_SemId		stcpcloSemaphore(sm_SemId *semid)
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

static void	shutDownClo(int signum)
{
	isignal(SIGTERM, shutDownClo);
	sm_SemEnd(stcpcloSemaphore(NULL));
}

/*	*	*	Keepalive thread functions	*	*	*/

typedef struct
{
	int		*cloRunning;
	pthread_mutex_t	*mutex;
	char		protocolName[MAX_CL_PROTOCOL_NAME_LEN + 1];
	char		ductName[MAX_CL_DUCT_NAME_LEN + 1];
	int		*ductSocket;
} KeepaliveThreadParms;

static void	*sendKeepalives(void *parm)
{
	KeepaliveThreadParms	*parms = (KeepaliveThreadParms *) parm;
	int			count = STCP_KEEPALIVE_PERIOD;
	int			bytesSent;

	iblock(SIGTERM);
	while (*(parms->cloRunning))
	{
		snooze(1);
		count++;
		if (count < STCP_KEEPALIVE_PERIOD)
		{
			continue;
		}

		/*	Time to send a keepalive.  Note that the
		 *	interval between keepalive attempts will be
		 *	STCP_KEEPALIVE_PERIOD plus (if the remote
		 *	induct is not reachable) the length of time
		 *	taken by TCP to determine that the connection
		 *	attempt will not succeed (e.g., 3 seconds).	*/

		count = 0;
		pthread_mutex_lock(parms->mutex);
		bytesSent = sendBundleByStcp(parms->protocolName,
				parms->ductName, parms->ductSocket, 0, 0, NULL);
		pthread_mutex_unlock(parms->mutex);
		if (bytesSent < 0)
		{
			shutDownClo(SIGTERM);
			break;
		}
	}

	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (ION_LWT)
int	stcpclo(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	char			*buffer;
	VOutduct		*vduct;
	PsmAddress		vductElt;
	Sdr			sdr;
	Outduct			duct;
	ClProtocol		protocol;
	int			running = 1;
	pthread_mutex_t		mutex;
	KeepaliveThreadParms	parms;
	pthread_t		keepaliveThread;
	Object			bundleZco;
	BpAncillaryData		ancillaryData;
	unsigned int		bundleLength;
	int			ductSocket = -1;
	int			bytesSent;
	int			pause = 0;

	if (ductName == NULL)
	{
		PUTS("Usage: stcpclo <remote host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("stcpclo can't attach to BP.", NULL);
		return -1;
	}

	buffer = MTAKE(STCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for TCP buffer in stcpclo.", NULL);
		return -1;
	}

	findOutduct("stcp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such stcp duct.", ductName);
		MRELEASE(buffer);
		return -1;
	}

	if (vduct->cloPid != ERROR && vduct->cloPid != sm_TaskIdSelf())
	{
		putErrmsg("CLO task is already started for this duct.",
				itoa(vduct->cloPid));
		MRELEASE(buffer);
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	ipnInit();
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));		/*	Lock the heap.	*/
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);			/*	Unlock.		*/

	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/

	oK(stcpcloSemaphore(&(vduct->semaphore)));
	isignal(SIGTERM, shutDownClo);
#ifndef mingw
	isignal(SIGPIPE, itcp_handleConnectionLoss);
#endif

	/*	Start the keepalive thread to manage the connection.	*/

	parms.cloRunning = &running;
	pthread_mutex_init(&mutex, NULL);
	parms.mutex = &mutex;
	istrcpy(parms.protocolName, protocol.name, MAX_CL_PROTOCOL_NAME_LEN);
	istrcpy(parms.ductName, ductName, MAX_CL_DUCT_NAME_LEN);
	parms.ductSocket = &ductSocket;
	if (pthread_begin(&keepaliveThread, NULL, sendKeepalives, &parms))
	{
		putSysErrmsg("stcpclo can't create keepalive thread", NULL);
		MRELEASE(buffer);
		pthread_mutex_destroy(&mutex);
		return 1;
	}

	/*	Can now begin transmitting to remote duct.		*/

	writeMemo("[i] stcpclo is running....");
	while (!(sm_SemEnded(stcpcloSemaphore(NULL))))
	{
		if (bpDequeue(vduct, &bundleZco, &ancillaryData, -1) < 0)
		{
			putErrmsg("Can't dequeue bundle.", NULL);
			break;
		}

		if (bundleZco == 0)	/*	Outduct closed.		*/
		{
			writeMemo("[i] stcpclo outduct closed.");
			sm_SemEnd(stcpcloSemaphore(NULL));
			continue;
		}

		if (bundleZco == 1)	/*	Got a corrupt bundle.	*/
		{
			continue;	/*	Get next bundle.	*/
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);
		pthread_mutex_lock(&mutex);
		bytesSent = sendBundleByStcp(protocol.name, ductName,
				&ductSocket, bundleLength, bundleZco, buffer);
		pthread_mutex_unlock(&mutex);
		if (bytesSent < 0)	/*	System error.		*/
		{
			sm_SemEnd(stcpcloSemaphore(NULL));
			continue;
		}

		/*	Check for closed socket.			*/

		if (ductSocket < 0)
		{
			/*	Wait a bit before trying to reconnect.	*/

			if (pause == 0)
			{
				pause = 1;
			}
			else
			{
				pause <<= 1;
				if (pause > MAX_RECONNECT_PAUSE)
				{
					pause = MAX_RECONNECT_PAUSE;
				}
			}

			snooze(pause);
		}
		else
		{
			/*	Give other tasks a chance to run.	*/

			pause = 0;
			sm_TaskYield();
		}
	}

	running = 0;		/*	Terminate keepalive thread.	*/
	pthread_join(keepaliveThread, NULL);
	closeStcpOutductSocket(&ductSocket);
	pthread_mutex_destroy(&mutex);
	writeErrmsgMemos();
	writeMemo("[i] stcpclo duct has ended.");
	MRELEASE(buffer);
	ionDetach();
	return 0;
}
