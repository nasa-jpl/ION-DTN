/*
	dccpclo.c:	BP DCCP-based convergence-layer output
			daemon.

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

/* Return Semaphore to control transmission 				*/
static sm_SemId	dccpcloSemaphore(sm_SemId *semid)
{
	static sm_SemId	semaphore = -1;

	if (semid)
	{
		semaphore = *semid;
	}

	return semaphore;
}


/* Commands LSO termination 						*/
static void	shutDownClo()
{
	sm_SemEnd(dccpcloSemaphore(NULL));
}


/*	*	*	Idle thread functions	*	*	*	*/


typedef struct {
	int 				active;
	int					data;
	int					linksocket;
	struct sockaddr		socketName;
	int					MPS;
	int 				done;
	pthread_mutex_t		mutex;
	char*				ductname;
} clo_state;


void* idle_wait(void* param)
{
	/* Disconnect after a certain amount of idle time 		*/
	clo_state 	*itp = (clo_state*)param;
	long 		count=0;

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
			/*Link has been idle 				*/
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


int connectDCCPsock(int* sock, struct sockaddr* socketName, int* MPS)
{
	int on;
	unsigned int is;
	int time;

	if(sock==NULL || socketName==NULL)
	{
		return -1;
	}

	if ((*sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP)) < 0 )
	{
		putSysErrmsg("CLO can't open DCCP socket. This probably means DCCP is not supported on your system.", NULL);
		return -1;
	}

	time=START_BACKOFF;
	while(connect(*sock, socketName, sizeof(struct sockaddr_in)) < 0)
	{
		putSysErrmsg("[i] Lost DCCP connection to CLI; attempting to reconnect.", NULL);
		snooze(time);
		if(time <= MAX_BACKOFF)
		{
			time=time*2;
		}else{
			putSysErrmsg("[i] Lost DCCP connection to CLI; restart CLO \
												when connectivity is restored.", NULL);
			return -1;
		}
	}

	on=1;
	if(setsockopt(*sock, SOL_DCCP, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0)
	{
		putSysErrmsg("CLO can't initialize socket.", "SO_REUSEADDR");
		return -1;
	}

	is=sizeof(int);
	if(getsockopt(*sock, SOL_DCCP, DCCP_SOCKOPT_GET_CUR_MPS, (int *) MPS, &is) < 0)
	{
			putSysErrmsg("CLO can't initialize socket.", "GET_CUR_MPS");
			*MPS=1400;
			return -1;
	}
return 0;
}

int	sendBytesByDCCP(int linkSocket, char *from, int length)
{
	int	bytesWritten;
	long 	count=0;

	while (1)	/*	Continue until not interrupted.		*/
	{
		bytesWritten = send(linkSocket, from, length, 0);
		if (bytesWritten < 0)
		{
			/*Interrupted.					*/
			if (errno == EINTR || errno==EAGAIN)
			{
				microsnooze(EAGAIN_WAIT);
				count++;
				if(MAX_DCCP_RETRIES > 0 && count > MAX_DCCP_RETRIES)
				{
					close(linkSocket);
					snooze(2); /* Let things settle down */
					return -2;
				}
				continue;	/*	Retry.		*/
			}

			/* Connection Reset 				*/
			if(errno==ENOTCONN || errno==ECONNRESET)
			{
				return -2;
			}

			/* CLI closed connection 			*/
			if(errno==EPIPE)
			{
				close(linkSocket);
				return -2;
			}
			putSysErrmsg("CLO send() error on socket.", NULL);
		}
		count=0;
		return bytesWritten;
	}
}

int	handleDccpFailure(char* ductname, struct sockaddr *sn, Object *bundleZco)
{
	Sdr	sdr = getIonsdr();

	/*	First mark the outduct as blocked, if possible.		*/
	if (ductname)
	{
		if (bpBlockOutduct("dccp", ductname) < 0)
		{
			putErrmsg("Can't block outduct.", NULL);
			return -1;
		}
	}

	/*	Handle the de-queued bundle.				*/
	if (bpHandleXmitFailure(*bundleZco) < 0)
	{
		putErrmsg("Can't handle DCCP xmit failure.", NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	zco_destroy(sdr, *bundleZco);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't destroy bundle ZCO.", NULL);
		return -1;
	}

	return 0;
}

int	sendBundleByDCCP(clo_state* itp, Object* bundleZco, BpExtendedCOS *extendedCOS,
			char* dest, char* buffer)
{
			Sdr			sdr;
			ZcoReader	reader;
			int			bytesSent;
			int			bytesToSend;
			int			bundleLength;

			/* Connect socket				*/
			if(!itp->active)
			{
				if(connectDCCPsock(&itp->linksocket, &itp->socketName, &itp->MPS)<0)
				{
					itp->active=0;
					return handleDccpFailure(itp->ductname, &itp->socketName, bundleZco);
				}
				itp->active=1;
			}

			/*check if we can send this size of bundle	*/
			sdr = getIonsdr();
			CHKERR(sdr_begin_xn(sdr));
			bundleLength = zco_length(sdr, *bundleZco);
			if(bundleLength > itp->MPS)
			{
				sdr_exit_xn(sdr);
				putErrmsg("Bundle is too big for DCCP CLO.", itoa(bundleLength));
				return -1;
			}

			/*Get Data to Send from ZCO			*/
			zco_start_transmitting(*bundleZco, &reader);
			bytesToSend = zco_transmit(sdr, &reader, DCCPCLA_BUFSZ, buffer);
			if (sdr_end_xn(sdr) < 0 || bytesToSend < 0)
			{
				putErrmsg("Can't issue from ZCO.", NULL);
				return -1;
			}

			/*Send Data
			 * retry until success or fatal error 		*/
			do{
				bytesSent = sendBytesByDCCP(itp->linksocket, buffer, bundleLength);
				if (bytesSent < bundleLength)
				{
					if(bytesSent==-2)
					{
						/*There is no connection. Attempt to reestablish it. */
						if(connectDCCPsock(&itp->linksocket, &itp->socketName, &itp->MPS)<0)
						{

							writeMemo("[i] Lost DCCP connection to CLI; restart CLO \
							when connectivity is restored.");
							return handleDccpFailure(itp->ductname, &itp->socketName, bundleZco);
						}
						else
						{
							continue; /*retry sending */
						}
					}
					else
					{
						return handleDccpFailure(itp->ductname, &itp->socketName, bundleZco);
					}
				}
				itp->data=1;
				itp->active=1;
				break; /*sent successfully		*/
			}while(1);

			/* Notify BP of success transmitting		*/
			if (bpHandleXmitSuccess(*bundleZco, 0) < 0)
			{
				putErrmsg("Can't handle xmit success.", NULL);
				bytesSent=-1;
			}

			/* Cleanup ZCO					*/
			CHKERR(sdr_begin_xn(sdr));
			zco_destroy(sdr, *bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy bundle ZCO.", NULL);
				bytesSent=-1;
			}
return bytesSent;
}

#if defined (VXWORKS) || defined (RTEMS)
int	dccpclo(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ductName = (char *) a1;
#else
int	main(int argc, char *argv[])
{
	char	*ductName = (argc > 1 ? argv[1] : NULL);
#endif
	VOutduct			*vduct;
	PsmAddress			vductElt;
	Sdr					sdr;
	Outduct				outduct;
	ClProtocol			protocol;
	Outflow				outflows[3];
	char* 				hostName;
	unsigned short		portNbr = 0;
	unsigned int		ipAddress = 0;
	struct sockaddr_in	*inetName;
	pthread_t			idle_thread;
	clo_state			itp;
	Object				bundleZco;
	BpExtendedCOS		extendedCOS;
	char				destDuctName[MAX_CL_DUCT_NAME_LEN + 1];
	unsigned int		bundleLength;
	int					running = 1;
	unsigned int		sentLength;
	char				*buffer;
	int					i;



	if (ductName == NULL)
	{
		PUTS("Usage: dccpclo <remote host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("dccpclo can't attach to BP.", NULL);
		return 1;
	}

	buffer = MTAKE(DCCPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("No memory for DCCP buffer in dccpclo.", NULL);
		return 1;
	}

	findOutduct("dccp",ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such dccp duct.", ductName);
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


	/*	All command-line arguments are now validated.
	 * 	Get Data outduct data structures from SDR 		*/
	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_read(sdr, (char *) &protocol, outduct.protocol, sizeof(ClProtocol));
	sdr_exit_xn(sdr);
	if (protocol.nominalRate <= 0)
	{
		vduct->xmitThrottle.nominalRate = DEFAULT_DCCP_RATE;
	}
	else
	{
		vduct->xmitThrottle.nominalRate = protocol.nominalRate;
	}

	/*setup priority queues						*/
	memset((char *) outflows, 0, sizeof(outflows));
	outflows[0].outboundBundles = outduct.bulkQueue;
	outflows[1].outboundBundles = outduct.stdQueue;
	outflows[2].outboundBundles = outduct.urgentQueue;
	for (i = 0; i < 3; i++)
	{
		outflows[i].svcFactor = 1 << i;
	}

	/* get host to connect to from outduct				*/
	hostName = ductName;
	parseSocketSpec(hostName, &portNbr, &ipAddress);
	if (portNbr == 0)
	{
		portNbr = BpDccpDefaultPortNbr;
	}
	if (ipAddress == 0)
	{
		putSysErrmsg("dccpclo can't get IP address for host.", hostName);
		MRELEASE(buffer);
		return 1;
	}

	portNbr = htons(portNbr);
	ipAddress = htonl(ipAddress);
	memset((char *) &itp.socketName, 0, sizeof itp.socketName);
	inetName = (struct sockaddr_in *) &itp.socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);

	/*	Set up signal handling. SIGTERM is shutdown signal.	*/
	oK(dccpcloSemaphore(&(vduct->semaphore)));
	signal(SIGTERM, shutDownClo);


	/*	Set up idle thread 					*/
	itp.active=0;
	itp.done=0;
	itp.data=0;
	itp.ductname=ductName;
	pthread_mutex_init(&itp.mutex, NULL);
	if (pthread_begin(&idle_thread, NULL, idle_wait, (void*)&itp))
	{
		putSysErrmsg("dccpclo can't create thread.", NULL);
		pthread_mutex_destroy(&itp.mutex);
		return 1;
	}
	
	/*	Can now begin transmitting to remote engine.		*/
	writeMemo("[i] dccpclo is running.");
	while (running && !(sm_SemEnded(vduct->semaphore)))
	{
		
		if (bpDequeue(vduct, outflows, &bundleZco, &extendedCOS,
				destDuctName, 0, -1) < 0)
		{
			sm_SemEnd(dccpcloSemaphore(NULL));
			continue;
			/*	Take Down CLO				*/
		}

		if (bundleZco == 0)	/*	Interrupted.		*/
		{
			continue;
		}

		CHKZERO(sdr_begin_xn(sdr));
		bundleLength = zco_length(sdr, bundleZco);
		sdr_exit_xn(sdr);

		if (bundleLength > DCCPCLA_BUFSZ)
		{
			/*Segment Way Too Big--Terminate CLO 		*/
			putErrmsg("Bundle is too big for DCCP CLO.", itoa(bundleLength));
			sm_SemEnd(dccpcloSemaphore(NULL));
			running = 0;
			continue;
		}

		/* send bundle 						*/
		pthread_mutex_lock(&itp.mutex);
		sentLength=sendBundleByDCCP(&itp, &bundleZco, &extendedCOS, destDuctName, buffer);
		pthread_mutex_unlock(&itp.mutex);
		if(sentLength < bundleLength)
		{
			sm_SemEnd(dccpcloSemaphore(NULL));
			running=0;
			continue;
			/*	Take Down CLO				*/
		}

		/*	Make sure other tasks have a chance to run.	*/
		sm_TaskYield();
	}

	/* CLO is exiting						*/
	pthread_mutex_lock(&itp.mutex);
	itp.done=1;
	pthread_mutex_unlock(&itp.mutex);
	pthread_join(idle_thread, NULL);
	pthread_mutex_destroy(&itp.mutex);
	close(itp.linksocket);
	writeErrmsgMemos();
	writeMemo("[i] dccpclo duct has ended.");
	return 0;
}

#else /*build_dccp*/


#include "bpP.h"
#if defined (VXWORKS) || defined (RTEMS)
int	dccpcli(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char *argv[])
{
#endif	
putErrmsg("dccpclo (and the DCCP protocol) are only available under Linux (>=3.2.0). Please see the README in the bp/dccp source directory for more information.", NULL);
writeErrmsgMemos();
return 0;
}


#endif /*build_dccp*/
