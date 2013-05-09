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
static void	shutDownLso()
{
	sm_SemEnd(dccplsoSemaphore(NULL));
}


/*	*	*	Idle thread functions	*	*	*	*/


typedef struct {
	int 			active;
	int				data;
	int				linksocket;
	struct sockaddr	socketName;
	int 			done;
	pthread_mutex_t	mutex;
} lso_state;


void* idle_wait(void* param)
{
	/* Disconnect after a certain amount of idle time 		*/
	lso_state 	*itp = (lso_state*)param;
	long 	count=0;

	iblock(SIGTERM);
	while(!itp->done)
		{
		snooze(1);
		count++;
		if (count < DCCP_IDLE_TIME)
		{
			continue;
		}
		count=0;

		pthread_mutex_lock(&itp->mutex);
		if(itp->active==1 && itp->data==0)
		{
			/*Link has been idle				*/
			close(itp->linksocket);	/*shut it down 		*/
			itp->active=0;
		}
		else
		{
			itp->data=0;
		}
		pthread_mutex_unlock(&itp->mutex);
	}
return NULL;
}


/*	*	*	Main thread functions	*	*	*	*/


int connectDCCPsock(int* sock, struct sockaddr* socketName)
{
	int on;

	if(sock==NULL || socketName==NULL)
	{
		return -1;
	}

	if ((*sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP)) < 0 )
	{
		putSysErrmsg("LSO can't open DCCP socket. This probably means DCCP is not supported on your system.", NULL);
		return -1;
	}

	if(connect(*sock, socketName, sizeof(struct sockaddr_in)) < 0)
	{
		putSysErrmsg("LSO can't connect DCCP socket.", strerror(errno));
		return -1;
	}

	on=1;
	if(setsockopt(*sock, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0)
	{
		putSysErrmsg("DCCP Socket Option Error.", NULL);
		return -1;
	}
return 0;
}

int	sendDataByDCCP(int linkSocket, char *from, int length)
{
	int		bytesWritten;
	long 	count=0;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = send(linkSocket, from, length, 0);
		if (bytesWritten < 0)
		{
			/*	Interrupted.				*/
			if (errno == EINTR || errno==EAGAIN)
			{
				microsnooze(EAGAIN_WAIT);
				count++;
				if(MAX_DCCP_RETRIES > 0 && count > MAX_DCCP_RETRIES){
					close(linkSocket);
					snooze(2); /*Let things settle down*/
					return -2;
				}
				continue;	/*	Retry.		*/
			}

			/* Connection Reset 				*/
			if(errno==ENOTCONN || errno==ECONNRESET)
			{
				return -2;
			}

			/* LSI closed connection 			*/
			if(errno==EPIPE)
			{
				close(linkSocket);
				return -2;
			}
			putSysErrmsg("LSO send() error on socket.", NULL);
		}
		count=0;
		return bytesWritten;
	}
}

int sendSegmentByDCCP(lso_state* itp, char* segment, int segmentLength)
{
	int bytesSent;

	/* 	connect the socket					*/
	if(!itp->active)
	{

		if(connectDCCPsock(&itp->linksocket, &itp->socketName)<0)
		{
			return -1;
		}
		itp->active=1;
	}

	/*Send Data
	 * retry until success or fatal error 				*/
	do{
		bytesSent = sendDataByDCCP(itp->linksocket, segment, segmentLength);
		if (bytesSent < segmentLength)
		{
			if(bytesSent==-2)
			{
				/*There is no connection. Attempt to reestablish it.*/
				if(connectDCCPsock(&itp->linksocket, &itp->socketName)<0)
				{
					return -1;
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
		itp->data=1;
		itp->active=1;
		break; /* sent successfully				*/
	}while(1);
return bytesSent;
}

#if defined (VXWORKS) || defined (RTEMS)
int	dccplso(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
	char				ownHostName[MAXHOSTNAMELEN];
	int					running = 1;
	int					segmentLength;
	char				*segment;
	struct sockaddr_in	*inetName;
	int					bytesSent;
	pthread_t			idle_thread;
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
		putErrmsg("LSO task is already started for this span.",
				itoa(vspan->lsoPid));
		return 1;
	}


	/*	All command-line arguments are now validated.		*/
	sdr_exit_xn(sdr);
	parseSocketSpec(endpointSpec, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = LtpDccpDefaultPortNbr;
	}
	if (ipAddress == 0)		/*	Default to local host.	*/
	{
		getNameOfHost(ownHostName, sizeof ownHostName);
		ipAddress = getInternetAddress(ownHostName);
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
	signal(SIGTERM, shutDownLso);

	/*	Set up idle thread 					*/
	itp.active=0;
	itp.done=0;
	itp.data=0;
	pthread_mutex_init(&itp.mutex, NULL);
	if (pthread_begin(&idle_thread, NULL, idle_wait, (void*)&itp))
	{
		putSysErrmsg("LSO can't create idle thread.", NULL);
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

		if (segmentLength == 0)/*	Interrupted.		*/
		{
			continue;
		}

		if (segmentLength > DCCPLSA_BUFSZ)
		{
			/*Segment Too Big				*/
			putErrmsg("Segment is too big for DCCP LSO.",
					itoa(segmentLength));
			running = 0;
			continue;
			/* Take down LSO				*/
		}

		pthread_mutex_lock(&itp.mutex);
		bytesSent=sendSegmentByDCCP(&itp, segment, segmentLength);
		pthread_mutex_unlock(&itp.mutex);
		if(bytesSent < segmentLength)
		{
			running=0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/* LSO is exiting						*/
	pthread_mutex_lock(&itp.mutex);
	itp.done=1;
	pthread_mutex_unlock(&itp.mutex);
	pthread_join(idle_thread, NULL);
	pthread_mutex_destroy(&itp.mutex);
	close(itp.linksocket);
	writeErrmsgMemos();
	writeMemo("[i] dccplso duct has ended.");
	return 0;
}

#else /*build_dccp*/


#include "ltpP.h"
#if defined (VXWORKS) || defined (RTEMS) || defined (bionic)
int	dccplso(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
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
