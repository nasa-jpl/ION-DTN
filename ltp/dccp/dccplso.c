/*
	dccplso.c:	LTP DCCP-based link service output daemon.
			Dedicated to DCCP datagram transmission to
			a single remote LTP engine.

	Author: Samuel Jero, Ohio University

	Copyright (c) 2010.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include <config.h>

#ifdef build_dccp

#include "dccplsa.h"
#include <errno.h>

/* Return Semaphore to control transmission 				*/
static sm_SemId	dccplsoSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;
	
	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}

/* Commands LSO termination 						*/
static void	shutDownLso(int signum)
{
	isignal(SIGTERM, shutDownLso);
	sm_SemEnd(dccplsoSemaphore(NULL));
}

#ifndef mingw
void	handleConnectionLoss(int signum)
{
	isignal(SIGPIPE, handleConnectionLoss);
}
#endif


/*	*	*	General Functions	*	*	*	*/


int connectDCCPsock(int* sock, struct sockaddr* socketName, int* MPS)
{
	int on;
	unsigned int is;
	unsigned char ccid;

	if (sock == NULL || socketName == NULL)
	{
		return -1;
	}

	if ((*sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP)) < 0 )
	{
		putSysErrmsg("DCCPLSO can't open DCCP socket. This probably means DCCP is not supported on your system.", NULL);
		return -1;
	}

	if (connect(*sock, socketName, sizeof(struct sockaddr_in)) < 0)
	{
		writeMemo("[i] DCCP connection to LSI could not be established. Retrying.");
		return -1;
	}

	on = 1;
	if (setsockopt(*sock, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0)
	{
		putSysErrmsg("DCCP Socket Option Error.", NULL);
		return -1;
	}

	if (DCCP_CCID > 0)
	{
		ccid = DCCP_CCID;
		if (setsockopt(*sock, SOL_DCCP, DCCP_SOCKOPT_CCID, (unsigned char*)&ccid, sizeof (ccid)) < 0)
		{
			putSysErrmsg("DCCP Socket Option Error.", NULL);
			return -1;
		}
	}

	if (DCCP_Q_LEN > 0)
	{
		on = DCCP_Q_LEN;
		if (setsockopt(*sock, SOL_DCCP, DCCP_SOCKOPT_QPOLICY_TXQLEN, (void *)&on, sizeof(on)) < 0)
		{
			putSysErrmsg("DCCP Socket Option Error.", NULL);
			return -1;
		}
	}

	is = sizeof(int);
	if (getsockopt(*sock, SOL_DCCP, DCCP_SOCKOPT_GET_CUR_MPS, (int *) MPS, &is) < 0)
	{
		putSysErrmsg("DCCPLSO can't initialize socket.", "GET_CUR_MPS");
		*MPS=1400;
		return -1;
	}
return 0;
}

int	sendDataByDCCP(int linkSocket, char *from, int length)
{
	int		bytesWritten;
	long 	count=0;

	if (linkSocket < 0)
	{
		return -2;
	}

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = isend(linkSocket, from, length, 0);
		if (bytesWritten < 0)
		{
			/*	Interrupted.				*/
			if (errno == EINTR || errno == EAGAIN)
			{
				microsnooze(EAGAIN_WAIT);
				count++;
				if (MAX_DCCP_RETRIES > 0 && count > MAX_DCCP_RETRIES)
				{
					close(linkSocket);
					snooze(2); /*Let things settle down*/
					return -2;
				}
				continue;	/*	Retry.		*/
			}

			/* Connection Reset 				*/
			if (errno == ENOTCONN || errno == ECONNRESET || errno == ECONNREFUSED)
			{
				close(linkSocket);
				return -2;
			}

			/* LSI closed connection 			*/
			if (errno == EPIPE)
			{
				close(linkSocket);
				return -2;
			}
			putSysErrmsg("DCCPLSO send() error on socket.", NULL);
		}
		count = 0;
		return bytesWritten;
	}
	return 0;
}


/*	*	*	Keep Alive functions	*	*	*	*/


typedef struct {
	int 			active;
	int				linksocket;
	struct sockaddr	socketName;
	int				MPS;
	int 			done;
	pthread_mutex_t	mutex;
} lso_state;


void* send_keepalives(void* param)
{
	/* send keepalives 		*/
	lso_state 	*itp = (lso_state*)param;
	long 	count = 0;
	char 	keepalive[4];
	int 	time;

	iblock(SIGTERM);
#ifndef mingw
	isignal(SIGPIPE, handleConnectionLoss);
#endif

	memset(keepalive,0,4);
	while (!itp->done)
	{
		snooze(1);
		count++;
		if (count < KEEPALIVE_PERIOD)
		{
			continue;
		}
		count = 0;

		pthread_mutex_lock(&itp->mutex);
		time = KEEPALIVE_PERIOD;
		while (!itp->done && sendDataByDCCP(itp->linksocket, keepalive,4) < 0)
		{
			if (!itp->done && connectDCCPsock(&itp->linksocket, &itp->socketName, &itp->MPS) < 0)
			{
				pthread_mutex_unlock(&itp->mutex);
				snooze(time);
				pthread_mutex_lock(&itp->mutex);
				if (time <= MAX_BACKOFF)
				{
					time = time*2;
				}
			}
			else
			{
				itp->active = 1;
			}
		}
		pthread_mutex_unlock(&itp->mutex);
	}
return NULL;
}


/*	*	*	Main thread functions	*	*	*	*/


int sendSegmentByDCCP(lso_state* itp, char* segment, int segmentLength)
{
	int bytesSent;

	/* 	connect the socket					*/
	if (!itp->active)
	{

		if (connectDCCPsock(&itp->linksocket, &itp->socketName, &itp->MPS)<0)
		{
			/* Throw this LTP segment away. LTP will ensure it is reliably
			 * retransmitted.*/
			return 0;
		}
		itp->active = 1;
	}

	if(segmentLength > itp->MPS)
	{
		putErrmsg("Segment is too big for DCCPLSO.", itoa(segmentLength));
		return -1;
	}

	/*Send Data
	 * retry until success or fatal error 				*/
	do {
		bytesSent = sendDataByDCCP(itp->linksocket, segment, segmentLength);
		if (bytesSent < segmentLength)
		{
			if (bytesSent == -2)
			{
				/*There is no connection. Attempt to reestablish it.*/
				if (connectDCCPsock(&itp->linksocket, &itp->socketName, &itp->MPS) < 0)
				{
					/* Throw this LTP segment away. LTP will ensure it is reliably
					 * retransmitted.*/
					return 0;
				}
				else
				{
					continue; /*retry sending 	*/
				}
			}
			else
			{
				return -1;
			}

		}
		itp->active = 1;
		break; /* sent successfully				*/
	} while(1);
return bytesSent;
}

#if defined (ION_LWT)
int	dccplso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*endpointSpec = (char *) a1;
	uvast	remoteEngineId = a2 != 0 ? strtouvast((char *) a2) : 0;
#else
int	main(int argc, char *argv[])
{
	char	*endpointSpec = argc > 1 ? argv[1] : NULL;
	uvast	remoteEngineId = argc > 2 ? strtouvast(argv[2]) : 0;
#endif
	Sdr					sdr;
	LtpVspan			*vspan;
	PsmAddress			vspanElt;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	int					running = 1;
	int					segmentLength;
	char				*segment;
	struct sockaddr_in	*inetName;
	int					bytesSent;
	pthread_t			keepalive_thread;
	lso_state			itp;

	if (remoteEngineId == 0 || endpointSpec == NULL)
	{
		PUTS("Usage: dccplso {<remote engine's host name> | @}[:\
<its port number>] <remote engine ID>");
		return 0;
	}

	/*	Note that ltpadmin must be run before the first
	 *	invocation of dccplso, to initialize the LTP database
	 *	(as necessary) and dynamic database.			*/
	if (ltpInit(0) < 0)
	{
		putErrmsg("dccplso can't initialize LTP.", NULL);
		writeErrmsgMemos();
		return 1;
	}

	sdr = getIonsdr();
	CHKZERO(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No such engine in database.", itoa(remoteEngineId));
		return 1;
	}

	if (vspan->lsoPid != ERROR && vspan->lsoPid != sm_TaskIdSelf())
	{
		sdr_exit_xn(sdr);
		putErrmsg("DCCPLSO task is already started for this span.",
				itoa(vspan->lsoPid));
		return 1;
	}


	/*	All command-line arguments are now validated.		*/
	sdr_exit_xn(sdr);
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
	ipAddress = htonl(ipAddress);
	memset((char *) &itp.socketName, 0, sizeof(itp.socketName));
	inetName = (struct sockaddr_in *) &itp.socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);


	/*	Set up signal handling.  SIGTERM is shutdown signal.	*/
	oK(dccplsoSemaphore(&(vspan->segSemaphore)));
	isignal(SIGTERM, shutDownLso);
#ifndef mingw
	isignal(SIGPIPE, handleConnectionLoss);
#endif

	/*	Set up idle thread 					*/
	itp.active = 0;
	itp.done = 0;
	itp.linksocket = -1;
	pthread_mutex_init(&itp.mutex, NULL);
	if (pthread_begin(&keepalive_thread, NULL, send_keepalives,
		(void*)&itp, "dccplso_keepalive"))
	{
		putSysErrmsg("DCCPLSO can't create idle thread.", NULL);
		pthread_mutex_destroy(&itp.mutex);
		return 1;
	}
	
	/*	Can now begin transmitting to remote engine.		*/
	writeMemo("[i] dccplso is running.");
	while (running && !(sm_SemEnded(vspan->segSemaphore)))
	{
		
		segmentLength = ltpDequeueOutboundSegment(vspan, &segment);
		if (segmentLength < 0)
		{
			running = 0;
			continue;
			/*	Take down LSO				*/
		}

		if (segmentLength == 0)
		{
			/*	Interrupted.				*/
			continue;
		}

		if (segmentLength > DCCPLSA_BUFSZ)
		{
			/* Segment Too Big				*/
			putErrmsg("Segment is too big for DCCPLSO.",
					itoa(segmentLength));
			running = 0;
			continue;
			/* Take down LSO				*/
		}

		pthread_mutex_lock(&itp.mutex);
		bytesSent = sendSegmentByDCCP(&itp, segment, segmentLength);
		pthread_mutex_unlock(&itp.mutex);
		if (bytesSent < 0)
		{
			running = 0;
			continue;
			/* Take down LSO				*/
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/* LSO is exiting						*/
	writeMemo("[i] dccplso duct done sending.");
	pthread_mutex_lock(&itp.mutex);
	itp.done = 1;
	pthread_mutex_unlock(&itp.mutex);
	pthread_join(keepalive_thread, NULL);
	pthread_mutex_destroy(&itp.mutex);
	close(itp.linksocket);
	writeErrmsgMemos();
	writeMemo("[i] dccplso duct has ended.");
	return 0;
}

#else /*build_dccp*/

#include "ltpP.h"
#if defined (ION_LWT)
int	dccplso(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
putErrmsg("dccplso (and the DCCP protocol) are only available under Linux (>=3.2.0). Please see the README in the ltp/dccp source directory for more information.", NULL);
writeErrmsgMemos();
return 0;
}

#endif /*build_dccp*/
