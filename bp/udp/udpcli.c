/*
	udpcli.c:	BP UDP-based convergence-layer input
			daemon, designed to serve as an input
			duct.

	Author: Ted Piotrowski, APL
		Scott Burleigh, JPL

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "udpcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"

static pthread_t	udpcliMainThread(pthread_t tid)
{
	static pthread_t	mainThread = 0;

	if (tid)
	{
		mainThread = tid;
	}

	return mainThread;
}

static void	interruptThread()
{
	pthread_t	mainThread = udpcliMainThread(0);

	isignal(SIGTERM, interruptThread);
	if (mainThread != pthread_self())
	{
		pthread_kill(mainThread, SIGTERM);
	}
}

/*	*	*	Receiver thread functions	*	*	*/

typedef struct
{
	VInduct		*vduct;
	int		ductSocket;
	pthread_t	mainThread;
	int		running;
} ReceiverThreadParms;

static void	*handleDatagrams(void *parm)
{
	/*	Main loop for UDP datagram reception and handling.	*/

	ReceiverThreadParms	*rtp = (ReceiverThreadParms *) parm;
	AcqWorkArea		*work;
	char			*buffer;
	int			bundleLength;
	struct sockaddr_in	fromAddr;
	unsigned int		hostNbr;
	char			hostName[MAXHOSTNAMELEN + 1];
	char			senderEidBuffer[SDRSTRING_BUFSZ];
	char			*senderEid;

	work = bpGetAcqArea(rtp->vduct);
	if (work == NULL)
	{
		putErrmsg("udpcli can't get acquisition work area.", NULL);
		pthread_kill(rtp->mainThread, SIGTERM);
		return NULL;
	}

	buffer = MTAKE(UDPCLA_BUFSZ);
	if (buffer == NULL)
	{
		putErrmsg("udpcli can't get UDP buffer.", NULL);
		pthread_kill(rtp->mainThread, SIGTERM);
		return NULL;
	}

	/*	Can now start receiving bundles.  On failure, take
	 *	down the CLI.						*/

	while (rtp->running)
	{	
		bundleLength = receiveBytesByUDP(rtp->ductSocket, &fromAddr,
				buffer, UDPCLA_BUFSZ);
		switch (bundleLength)
		{
		case -1:
		case 0:
			putErrmsg("Can't acquire bundle.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);

			/*	Intentional fall-through to next case.	*/

		case 1:				/*	Normal stop.	*/
			rtp->running = 0;
			continue;

		default:
			break;			/*	Out of switch.	*/
		}

		memcpy((char *) &hostNbr,
				(char *) &(fromAddr.sin_addr.s_addr), 4);
		hostNbr = ntohl(hostNbr);
		if (getInternetHostName(hostNbr, hostName))
		{
			senderEid = senderEidBuffer;
			getSenderEid(&senderEid, hostName);
		}
		else
		{
			senderEid = NULL;
		}

		if (bpBeginAcq(work, 0, senderEid) < 0
		|| bpContinueAcq(work, buffer, bundleLength) < 0
		|| bpEndAcq(work) < 0)
		{
			putErrmsg("Can't acquire bundle.", NULL);
			pthread_kill(rtp->mainThread, SIGTERM);
			rtp->running = 0;
			continue;
		}

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeErrmsgMemos();
	writeMemo("[i] udpcli receiver thread has ended.");

	/*	Free resources.						*/

	bpReleaseAcqArea(work);
	MRELEASE(buffer);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

#if defined (VXWORKS) || defined (RTEMS)
int	udpcli(int a1, int a2, int a3, int a4, int a5,
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
	ClProtocol		protocol;
	char			*hostName;
	unsigned short		portNbr;
	unsigned int		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	socklen_t		nameLength;
	ReceiverThreadParms	rtp;
	pthread_t		receiverThread;
	int			fd;
	char			quit = 0;

	if (ductName == NULL)
	{
		PUTS("Usage: udpcli <local host name>[:<port number>]");
		return 0;
	}

	if (bpAttach() < 0)
	{
		putErrmsg("udpcli can't attach to BP.", NULL);
		return -1;
	}

	findInduct("udp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		putErrmsg("No such udp duct.", ductName);
		return -1;
	}

	if (vduct->cliPid > 0 && vduct->cliPid != sm_TaskIdSelf())
	{
		putErrmsg("CLI task is already started for this duct.",
				itoa(vduct->cliPid));
		return -1;
	}

	/*	All command-line arguments are now validated.		*/

	sdr = getIonsdr();
	sdr_read(sdr, (char *) &duct, sdr_list_data(sdr, vduct->inductElt),
			sizeof(Induct));
	sdr_read(sdr, (char *) &protocol, duct.protocol, sizeof(ClProtocol));
	if (protocol.nominalRate <= 0)
	{
		vduct->acqThrottle.nominalRate = DEFAULT_UDP_RATE;
	}
	else
	{
		vduct->acqThrottle.nominalRate = protocol.nominalRate;
	}

	hostName = ductName;
	parseSocketSpec(ductName, &portNbr, &hostNbr);
	if (portNbr == 0)
	{
		portNbr = BpUdpDefaultPortNbr;
	}

	portNbr = htons(portNbr);
	if (hostNbr == 0)
	{
		putErrmsg("Can't get IP address for host.", hostName);
		return -1;
	}

	rtp.vduct = vduct;
	hostNbr = htonl(hostNbr);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = portNbr;
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	rtp.ductSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (rtp.ductSocket < 0)
	{
		putSysErrmsg("Can't open UDP socket", NULL);
		return -1;
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(rtp.ductSocket)
	|| bind(rtp.ductSocket, &socketName, nameLength) < 0
	|| getsockname(rtp.ductSocket, &socketName, &nameLength) < 0)
	{
		close(rtp.ductSocket);
		putSysErrmsg("Can't initialize socket", NULL);
		return -1;
	}

	/*	Initialize sender endpoint ID lookup.			*/

	ipnInit();
	dtn2Init();

	/*	Set up signal handling; SIGTERM is shutdown signal.	*/

	oK(udpcliMainThread(pthread_self()));
	isignal(SIGTERM, interruptThread);

	/*	Start the receiver thread.				*/

	rtp.running = 1;
	rtp.mainThread = pthread_self();
	if (pthread_create(&receiverThread, NULL, handleDatagrams, &rtp))
	{
		close(rtp.ductSocket);
		putSysErrmsg("udpcli can't create receiver thread", NULL);
		return -1;
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the induct.				*/

	{
		char txt[500];

		isprintf(txt, sizeof(txt), "[i] udpcli is running, spec=[%s:%d].", 
			inet_ntoa(inetName->sin_addr), ntohs(inetName->sin_port) );

		writeMemo(txt );
	}
	snooze(2000000000);

	/*	Time to shut down.					*/

	rtp.running = 0;

	/*	Wake up the receiver thread by sending it a 1-byte
	 *	datagram.						*/

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd >= 0)
	{
		sendto(fd, &quit, 1, 0, &socketName, sizeof(struct sockaddr));
		close(fd);
	}

	pthread_join(receiverThread, NULL);
	close(rtp.ductSocket);
	writeErrmsgMemos();
	writeMemo("[i] udpcli duct has ended.");
	ionDetach();
	return 0;
}
