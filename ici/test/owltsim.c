/*
	owltsim.c:	one-way light time delay simulator.

	Author: Scott Burleigh, JPL

	Copyright (c) 2008, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "platform.h"
#include "lyst.h"
#include "ion.h"

#define	MAX_DATAGRAM	65535

typedef struct
{
	char		toNode[33];
	char		fromNode[33];
	unsigned short	myPortNbr;
	char		destHostName[MAXHOSTNAMELEN + 1];
	unsigned short	destPortNbr;
	unsigned short	owlt;
	int		insock;
	int		outsock;
	Lyst		transmission;
	sm_SemId	mutex;
	pthread_t	timerThread;
	int		verbose;
	short		modulus;
} SimThreadParms;

typedef struct
{
	struct timeval	xmitTime;
	int		length;
	char		content[1];
} DG;

static void	owltsimExit(int returnCode)
{
#ifdef mingw
	oK(_winsock(1));
#endif
	exit(returnCode);
}

/*	*	*	Timer thread functions	*	*	*	*/

static void	deleteDG(LystElt elt, void *userdata)
{
	void	*dg = lyst_data(elt);

	if (dg)
	{
		free(dg);
	}
}

static void	*sendUdp(void *parm)
{
	SimThreadParms	*stp = (SimThreadParms *) parm;
	struct timeval	currentTime;
	LystElt		elt;
	LystElt		nextElt;
	DG		*dg;
	char		timebuf[256];

	while (1)
	{
		microsnooze(100000);	/*	Sleep 1/10 second.	*/
		getCurrentTime(&currentTime);
		if (sm_SemTake(stp->mutex) < 0
		|| sm_SemEnded(stp->mutex))
		{
			owltsimExit(0);
		}

		for (elt = lyst_first(stp->transmission); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			dg = (DG *) lyst_data(elt);
			if (dg->xmitTime.tv_sec > currentTime.tv_sec
			|| (dg->xmitTime.tv_sec == currentTime.tv_sec
				&& dg->xmitTime.tv_usec > currentTime.tv_usec))
			{
				break;	/*	Not time to send yet.	*/
			}

			/*	It's time to send this datagram.	*/

			if (send(stp->outsock, dg->content, dg->length, 0) < 0)
			{
				if (errno == ECONNREFUSED)
				{
					writeTimestampLocal(time(NULL),
						timebuf);
					printf("at %s owlt LOST a dg of \
length %d from %s destined for %s due to ECONNREFUSED.\n", timebuf,
						dg->length, stp->fromNode,
						stp->toNode);
				}
				else
				{
					perror("owltsim failed on send");
					printf("host name: %s\n",
							stp->destHostName);
					printf("port number: %hu\n",
							stp->destPortNbr);
					close(stp->insock);
					stp->insock = -1;
					return NULL;
				}
			}
			else
			{
				if (stp->verbose)
				{
					writeTimestampLocal(time(NULL),
						timebuf);
					printf("at %s owlt sent a dg of \
length %d from %s destined for %s.\n", timebuf, dg->length, stp->fromNode,
						stp->toNode);
				}
			}

			lyst_delete(elt);
		}

		sm_SemGive(stp->mutex);
	}
}

/*	*	*	Simulator thread functions	*	*	*/

static void	*receiveUdp(void *parm)
{
	SimThreadParms		*stp = (SimThreadParms *) parm;
	unsigned int		datagramCount = 0;
	char			*buffer;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	socklen_t		nameLength;
	unsigned int		ipAddress;
	int			datagramLen;
	struct sockaddr_in	fromAddr;
	socklen_t		fromSize;
	struct timeval		currentTime;
	DG			*dg;
	char			timebuf[256];

	buffer = malloc(MAX_DATAGRAM);
	if (buffer == NULL)
	{
		puts("owltsim out of memory.");
		owltsimExit(0);
	}

	/*	Create reception socket.				*/

	stp->insock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (stp->insock < 0)
	{
		perror("owltsim can't open reception socket");
		owltsimExit(1);
	}

	inetName = (struct sockaddr_in *) &socketName;
	memset((char *) &socketName, 0, sizeof socketName);
	inetName->sin_family = AF_INET;
	inetName->sin_addr.s_addr = INADDR_ANY;
	inetName->sin_port = htons(stp->myPortNbr);
	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(stp->insock)
	|| bind(stp->insock, &socketName, nameLength) < 0
	|| getsockname(stp->insock, &socketName, &nameLength) < 0)
	{
		perror("owltsim can't initialize reception socket");
		printf("port number: %hu\n", stp->myPortNbr);
		owltsimExit(0);
	}

	/*	Create transmisssion socket.				*/

	memset((char *) &socketName, 0, sizeof socketName);
	inetName->sin_family = AF_INET;
	ipAddress = getInternetAddress(stp->destHostName);
	ipAddress = htonl(ipAddress);
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &ipAddress, 4);
	inetName->sin_port = htons(stp->destPortNbr);
	stp->outsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (stp->outsock < 0)
	{
		perror("owltsim can't open transmission socket");
		owltsimExit(1);
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(stp->outsock)
	|| connect(stp->outsock, &socketName, nameLength) < 0
	|| getsockname(stp->outsock, &socketName, &nameLength) < 0)
	{
		perror("owltsim can't initialize transmission socket");
		printf("host name: %s\n", stp->destHostName);
		printf("port number: %hu\n", stp->destPortNbr);
		owltsimExit(1);
	}

	/*	Create transmission stream list and mutex.		*/

	stp->transmission = lyst_create();
	if (stp->transmission == NULL)
	{
		puts("owltsim out of memory.");
		owltsimExit(1);
	}

	lyst_delete_set(stp->transmission, deleteDG, NULL);
	stp->mutex = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);

	/*	Spawn timer/transmitter thread.				*/

	if (pthread_begin(&(stp->timerThread), NULL, sendUdp,
		stp, "owltsim_timer"))
	{
		perror("owltsim can't spawn timer thread");
		owltsimExit(1);
	}

	/*	Main loop for UDP datagram reception and handling.	*/

	while (1)
	{	
		fromSize = sizeof fromAddr;
		datagramLen = recvfrom(stp->insock, buffer, MAX_DATAGRAM,
				0, (struct sockaddr *) &fromAddr, &fromSize);
		if (datagramLen < 0)
		{
			perror("owltsim can't acquire datagram");
			printf("from node %s to node %s.\n",
					stp->fromNode, stp->toNode);
			break;
		}

		datagramCount++;
		if (stp->modulus < 0)
		{
			if (datagramCount % (0 - stp->modulus) == 0)
			{
				if (stp->verbose)
				{
					writeTimestampLocal(time(NULL),
						timebuf);
					printf("at %s owlt DETERMINISTICALLY \
DROPPED a dg of length %d from %s destined for %s.\n", timebuf, datagramLen,
						stp->fromNode, stp->toNode);
				}

				continue;
			}
		}
		else if (stp->modulus > 0)
		{
			if ((rand() % stp->modulus) == 0)
			{
				if (stp->verbose)
				{
					writeTimestampLocal(time(NULL),
						timebuf);
					printf("at %s owlt RANDOMLY DROPPED \
a dg of length %d from %s destined for %s.\n", timebuf, datagramLen,
						stp->fromNode, stp->toNode);
				}

				continue;
			}
		}

		getCurrentTime(&currentTime);
		dg = (DG *) malloc((sizeof(DG) - 1) + datagramLen);
		if (dg == NULL)
		{
			puts("owltsim out of memory.");
			owltsimExit(0);
		}

		dg->xmitTime.tv_sec = currentTime.tv_sec + stp->owlt;
		dg->xmitTime.tv_usec = currentTime.tv_usec;
		dg->length = datagramLen;
		memcpy(dg->content, buffer, datagramLen);
		if (sm_SemTake(stp->mutex) < 0
		|| sm_SemEnded(stp->mutex))
		{
			owltsimExit(0);
		}

		if (lyst_insert_last(stp->transmission, dg) == NULL)
		{
			puts("owltsim out of memory.");
			owltsimExit(0);
		}

		sm_SemGive(stp->mutex);
		if (stp->verbose)
		{
			writeTimestampLocal(time(NULL), timebuf);
			printf("at %s owlt got a dg of length %d from %s \
destined for %s.\n", timebuf, datagramLen, stp->fromNode, stp->toNode);
		}
	}

	printf("Ending owltsim receiver thread from %s to %s.\n",
			stp->fromNode, stp->toNode);

	/*	Free resources.						*/

	free(buffer);
#if 0
	pthread_end(stp->timerThread);
	pthread_join(stp->timerThread, NULL);
	lyst_destroy(stp->transmission);
#endif
	sm_SemEnd(stp->mutex);
	microsnooze(50000);
	sm_SemDelete(stp->mutex);
pthread_join(stp->timerThread, NULL);
lyst_destroy(stp->transmission);
	if (stp->insock >= 0)
	{
		close(stp->insock);
	}

	if (stp->outsock >= 0)
	{
		close(stp->outsock);
	}

	free(stp);
	return NULL;
}

/*	*	*	Main thread functions	*	*	*	*/

int	main(int argc, char *argv[])
{
	int			verbose = 0;
	char			*fileName = NULL;
	FILE			*configFile;
	int			reading = 1;
	int			lineNbr = 0;
	SimThreadParms		stpBuf;
	SimThreadParms		*stp;
	pthread_t		simThread;

	/*	Read configuration file, parsing each line.  For
	 *	each line, start a thread that simulates owlt on
	 *	the indicated link.  Then snooze forever.		*/

	srand(time(NULL));
#ifdef mingw
	if (_winsock(0) < 0)
	{
		putErrmsg("Can't start WinSock.", NULL);
		exit(1);
	}
#endif
	switch (argc)
	{
	case 3:
		if (strcmp(argv[2], "-v") == 0)
		{
			verbose = 1;
		}

		/*	Intentional fall-through to next case.		*/

	case 2:
		fileName = argv[1];
		break;

	default:
		puts("Usage:  owltsim <config file name> [-v]");
		puts("Each line of config file must be of this form:");
		puts("\t<to> <from> <my port#> <dest hostname> <dest port#> \
<OWLT in seconds> <modulus>");
		puts("where <to> and <from> are ION node numbers.  These node");
		puts("numbers are intended to make the configuration file");
		puts("somewhat self-documenting.  <from> may be '*' if 'all'.");
		puts("Normally <modulus> should always be zero.  If you use");
		puts("a value greater than 0 for <modulus> then owltsim will");
		puts("randomly discard one out of every <modulus> datagrams");
		puts("it receives on this simulated link; if you use a value");
		puts("N where N < 0 for <modulus> then owlt will discard");
		puts("every Nth datagram it receives on this simulated link.");
		owltsimExit(0);
	}

	sm_ipc_init();
	configFile = fopen(fileName, "r");
	if (configFile == NULL)
	{
		perror("owltsim can't open configuration file");
		printf("file name is '%s'.\n", argv[1]);
		owltsimExit(1);
	}

	while (reading)
	{
		lineNbr++;
		memset((char *) &stpBuf, 0, sizeof(SimThreadParms));
		stpBuf.verbose = verbose;
		switch (fscanf(configFile, "%32s %32s %hu %255s %hu %hu %hd",
				stpBuf.toNode, stpBuf.fromNode,
				&stpBuf.myPortNbr, stpBuf.destHostName,
				&stpBuf.destPortNbr, &stpBuf.owlt,
				&stpBuf.modulus))
		{
		case EOF:
			if (feof(configFile))
			{
				fclose(configFile);
				reading = 0;
				continue;
			}

			perror("owltsim failed on fscanf");
			owltsimExit(1);

		case 7:
			stp = (SimThreadParms *) malloc(sizeof(SimThreadParms));
			if (stp == NULL)
			{
				puts("owltsim out of memory.");
				owltsimExit(1);
			}

			memcpy((char *) stp, (char *) &stpBuf,
					sizeof(SimThreadParms));
			if (pthread_begin(&simThread, NULL, receiveUdp,
				stp, "owltsim_receiver"))
			{
				perror("owltsim can't spawn receiver thread");
				owltsimExit(1);
			}

			continue;

		default:
			printf("owlt stopped: malformed config file line %d.\n",					lineNbr);
			owltsimExit(1);
		}
	}

	/*	Now sleep until interrupted by SIGTERM, at which point
	 *	it's time to stop the simulator.			*/

	snooze(2000000000);
	puts("owltsim is ending.");
#ifdef mingw
	oK(_winsock(1));
#endif
	return 0;
}
