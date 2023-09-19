/*
	owlttb.c:	one-way light time delay simulator for
			NetAcquire environment.

	Author: Scott Burleigh, JPL

	Copyright (c) 2008, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "platform.h"
#include "lyst.h"
#include "ion.h"

#define	BUFFER_SIZE	65535

typedef struct
{
	unsigned long	ownUplinkPortNbr;
	unsigned long	ownDownlinkPortNbr;
	struct in_addr	destUplinkIpAddress;
	unsigned long	destUplinkPortNbr;
	struct in_addr	destDownlinkIpAddress;
	unsigned long	destDownlinkPortNbr;
	unsigned long	owlt;
	int		verbose;
	Lyst		uplinkSegments;
	sm_SemId	uplinkMutex;
	int		uplinkXmitSocket;
	Lyst		downlinkSegments;
	sm_SemId	downlinkMutex;
	int		downlinkXmitSocket;
} SimThreadParms;

typedef struct
{
	struct timeval	xmitTime;
	int		length;
	char		content[1];
} Segment;

/*	*	*	Shared functions	*	*	*	*/

static void	deleteSegment(LystElt elt, void *userdata)
{
	void	*segment = lyst_data(elt);

	if (segment)
	{
		free(segment);
	}
}

void	connectToSocket(struct sockaddr *sn, int *sock)
{
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0)
	{
		perror("owlttb can't open TCP socket to NetAcquire");
		exit(1);
	}

	if (connect(*sock, sn, sizeof(struct sockaddr)) < 0)
	{
		perror("owlttb can't connect TCP socket to NetAcquire");
		exit(1);
	}
}

int	sendBytesByTCP(int sock, char *from, int length)
{
	int	bytesWritten;

	while (length > 0)
	{
		bytesWritten = write(sock, from, length);
		if (bytesWritten < 0)
		{
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Try again.	*/
			}

			perror("owlttb write() error on socket");
			return -1;
		}

		length -= bytesWritten;
		from += bytesWritten;
	}

	return 0;
}

int	receiveBytesByTCP(int sock, char *into, int length)
{
	int	bytesRead;

	while (1)
	{
		bytesRead = read(sock, into, length);
		switch (bytesRead)
		{
		case -1:
			if (errno == EINTR)	/*	Interrupted.	*/
			{
				continue;	/*	Try again.	*/
			}

			perror("owlttb read() error on socket");
			exit(1);

		case 0:			/*	Connection closed.	*/
			return 0;

		default:
			return bytesRead;
		}
	}
}

/*	*	*	*	Downlink functions	*	*	*/

static void	*recvDownlink(void *parm)
{
	SimThreadParms		*stp = (SimThreadParms *) parm;
	char			*buffer;
	unsigned short		portNbr = stp->destDownlinkPortNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	int			fwdSocket;
	int			segLength;
	struct timeval		currentTime;
	Segment			*seg;
	char			timebuf[256];

	buffer = malloc(BUFFER_SIZE);
	if (buffer == NULL)
	{
		puts("owlttb out of memory.");
		exit(1);
	}

	memset((char *) &socketName, 0, sizeof socketName);
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	inetName->sin_addr.s_addr = stp->destDownlinkIpAddress.s_addr;
	connectToSocket(&socketName, &fwdSocket);
	while (1)
	{
		segLength = receiveBytesByTCP(fwdSocket, buffer, BUFFER_SIZE);
		if (segLength <= 0)
		{
			perror("owlttb failed reading from NetAcquire");
			exit(1);
		}

		getCurrentTime(&currentTime);
		seg = (Segment *) malloc((sizeof(Segment) - 1) + segLength);
		if (seg == NULL)
		{
			puts("owlttb out of memory.");
			exit(1);
		}

		seg->xmitTime.tv_sec = currentTime.tv_sec + stp->owlt;
		seg->xmitTime.tv_usec = currentTime.tv_usec;
		seg->length = segLength;
		memcpy(seg->content, buffer, segLength);
		if (sm_SemTake(stp->downlinkMutex) < 0)
		{
			exit(1);
		}

		if (lyst_insert_last(stp->downlinkSegments, seg) == NULL)
		{
			puts("owlttb out of memory.");
			exit(1);
		}

		sm_SemGive(stp->downlinkMutex);
		if (stp->verbose)
		{
			writeTimestampLocal(time(NULL), timebuf);
			printf("at %s owlt got a downlink seg of length %d.\n",
					timebuf, segLength);
		}
	}
}

static void	*sendDownlink(void *parm)
{
	SimThreadParms		*stp = (SimThreadParms *) parm;
	struct timeval		currentTime;
	LystElt			elt;
	LystElt			nextElt;
	Segment			*seg;
	char			timebuf[256];

	while (1)
	{
		microsnooze(100000);	/*	Sleep 1/10 second.	*/
		getCurrentTime(&currentTime);
		if (sm_SemTake(stp->downlinkMutex) < 0)
		{
			exit(1);
		}

		for (elt = lyst_first(stp->downlinkSegments); elt;
				elt = nextElt)
		{
			nextElt = lyst_next(elt);
			seg = (Segment *) lyst_data(elt);
			if (seg->xmitTime.tv_sec > currentTime.tv_sec
			|| (seg->xmitTime.tv_sec == currentTime.tv_sec
					&& seg->xmitTime.tv_usec >
					currentTime.tv_usec))
			{
				break;	/*	Not time to send yet.	*/
			}

			/*	It's time to send this segment.		*/

			if (stp->downlinkXmitSocket >= 0)
			{
				if (sendBytesByTCP(stp->downlinkXmitSocket,
						seg->content, seg->length) == 0)
				{
					if (stp->verbose)
					{
						writeTimestampLocal(time(NULL),
							timebuf);
						printf("at %s owlt sent a \
downlink seg of length %d.\n", timebuf, seg->length);
					}

					lyst_delete(elt);
					continue;
				}

				perror("owlttb lost downlink client");
				close(stp->downlinkXmitSocket);
				stp->downlinkXmitSocket = -1;
			}

			if (stp->verbose)
			{
				writeTimestampLocal(time(NULL), timebuf);
				printf("at %s owlt ditched a downlink seg of \
length %d.\n", timebuf, seg->length);
			}

			lyst_delete(elt);
		}

		sm_SemGive(stp->downlinkMutex);
	}
}

static void	*offerDownlink(void *parm)
{
	SimThreadParms		*stp = (SimThreadParms *) parm;
	char			*buffer;
	unsigned short		portNbr = stp->ownDownlinkPortNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			dialupSocket;
	unsigned int		nameLength;
	pthread_t		recvDownlinkThread;
	pthread_t		sendDownlinkThread;
	int			fwdSocket;

	buffer = malloc(BUFFER_SIZE);
	if (buffer == NULL)
	{
		puts("owlttb out of memory.");
		exit(1);
	}

	/*	Create downlink dialup socket.				*/

	memset((char *) &(socketName), 0, sizeof(struct sockaddr));
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	inetName->sin_addr.s_addr = INADDR_ANY;
	dialupSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (dialupSocket < 0)
	{
		perror("owlttb can't open downlink dialup socket");
		exit(1);
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(dialupSocket)
	|| bind(dialupSocket, &socketName, nameLength) < 0
	|| listen(dialupSocket, 5) < 0
	|| getsockname(dialupSocket, &socketName, &nameLength) < 0)
	{
		perror("owlttb can't initialize downlink dialup socket");
		exit(1);
	}

	/*	Create transmission stream list and mutex.		*/

	stp->downlinkSegments = lyst_create();
	if (stp->downlinkSegments == NULL)
	{
		puts("owlttb out of memory.");
		exit(1);
	}

	lyst_delete_set(stp->downlinkSegments, deleteSegment, NULL);
	stp->downlinkMutex = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	stp->downlinkXmitSocket = -1;

	/*	Spawn receiver thread.					*/

	if (pthread_create(&recvDownlinkThread, NULL, recvDownlink, stp))
	{
		perror("owlttb can't spawn downlink receiver thread");
		exit(1);
	}

	/*	Spawn timer/transmitter thread.				*/

	if (pthread_create(&sendDownlinkThread, NULL, sendDownlink, stp))
	{
		perror("owlttb can't spawn downlink sender thread");
		exit(1);
	}

	/*	Main loop for accepting downlink connections.		*/

	while (1)
	{
		nameLength = sizeof(struct sockaddr);
		fwdSocket = accept(dialupSocket, &socketName, &nameLength);
		if (fwdSocket < 0)
		{
			perror("owlttb accept() failed");
			exit(1);
		}

		/*	Let the timer/transmitter thread use this
		 *	new connection, unless it's already using
		 *	another one.					*/

		if (stp->downlinkXmitSocket < 0)
		{
			stp->downlinkXmitSocket = fwdSocket;
		}
		else
		{
			close(fwdSocket);
		}
	}
}

/*	*	*	*	Uplink functions	*	*	*/

static void	*sendUplink(void *parm)
{
	SimThreadParms		*stp = (SimThreadParms *) parm;
	unsigned short		portNbr = stp->destUplinkPortNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName = (struct sockaddr_in *) &socketName;
	struct timeval		currentTime;
	LystElt			elt;
	LystElt			nextElt;
	Segment			*seg;
	char			timebuf[256];

	memset((char *) &socketName, 0, sizeof socketName);
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	inetName->sin_addr.s_addr = stp->destUplinkIpAddress.s_addr;
	connectToSocket(&socketName, &stp->uplinkXmitSocket);
	while (1)
	{
		microsnooze(100000);	/*	Sleep 1/10 second.	*/
		getCurrentTime(&currentTime);
		if (sm_SemTake(stp->uplinkMutex) < 0)
		{
			exit(1);
		}

		for (elt = lyst_first(stp->uplinkSegments); elt; elt = nextElt)
		{
			nextElt = lyst_next(elt);
			seg = (Segment *) lyst_data(elt);
			if (seg->xmitTime.tv_sec > currentTime.tv_sec
			|| (seg->xmitTime.tv_sec == currentTime.tv_sec
				&& seg->xmitTime.tv_usec > currentTime.tv_usec))
			{
				break;	/*	Not time to send yet.	*/
			}

			/*	It's time to send this segment.	*/

			if (sendBytesByTCP(stp->uplinkXmitSocket, 
					seg->content, seg->length) < 0)
			{
				perror("owlttb failed writing to NetAcquire");
				exit(1);
			}

			if (stp->verbose)
			{
				writeTimestampLocal(time(NULL), timebuf);
				printf("at %s owlt sent an uplink seg of \
length %d.\n", timebuf, seg->length);
			}

			lyst_delete(elt);
		}

		sm_SemGive(stp->uplinkMutex);
	}
}

static void	*offerUplink(void *parm)
{
	SimThreadParms		*stp = (SimThreadParms *) parm;
	char			*buffer;
	unsigned short		portNbr = stp->ownUplinkPortNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			dialupSocket;
	unsigned int		nameLength;
	pthread_t		sendUplinkThread;
	int			fwdSocket;
	int			segLength;
	struct timeval		currentTime;
	Segment			*seg;
	char			timebuf[256];

	buffer = malloc(BUFFER_SIZE);
	if (buffer == NULL)
	{
		puts("owlttb out of memory.");
		exit(1);
	}

	/*	Create uplink dialup socket.				*/

	memset((char *) &(socketName), 0, sizeof(struct sockaddr));
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	inetName->sin_addr.s_addr = INADDR_ANY;
	dialupSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (dialupSocket < 0)
	{
		perror("owlttb can't open uplink dialup socket");
		exit(1);
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(dialupSocket)
	|| bind(dialupSocket, &socketName, nameLength) < 0
	|| listen(dialupSocket, 5) < 0
	|| getsockname(dialupSocket, &socketName, &nameLength) < 0)
	{
		perror("owlttb can't initialize uplink dialup socket");
		exit(1);
	}

	/*	Create transmission stream list and mutex.		*/

	stp->uplinkSegments = lyst_create();
	if (stp->uplinkSegments == NULL)
	{
		puts("owlttb out of memory.");
		exit(1);
	}

	lyst_delete_set(stp->uplinkSegments, deleteSegment, NULL);
	stp->uplinkMutex = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	stp->uplinkXmitSocket = -1;

	/*	Spawn timer/transmitter thread.				*/

	if (pthread_create(&sendUplinkThread, NULL, sendUplink, stp))
	{
		perror("owlttb can't spawn uplink sender thread");
		exit(1);
	}

	/*	Main loop for uplink segment reception and handling.	*/

	while (1)
	{
		nameLength = sizeof(struct sockaddr);
		fwdSocket = accept(dialupSocket, &socketName, &nameLength);
		if (fwdSocket < 0)
		{
			perror("owlttb accept() failed");
			exit(1);
		}

		while (1)
		{
			segLength = receiveBytesByTCP(fwdSocket, buffer,
					BUFFER_SIZE);
			if (segLength <= 0)
			{
				puts("owlttb lost uplink client.");
				break;	/*	Out of inner loop.	*/
			}

			getCurrentTime(&currentTime);
			seg = (Segment *) malloc((sizeof(Segment) - 1)
					+ segLength);
			if (seg == NULL)
			{
				puts("owlttb out of memory.");
				exit(1);
			}

			seg->xmitTime.tv_sec = currentTime.tv_sec + stp->owlt;
			seg->xmitTime.tv_usec = currentTime.tv_usec;
			seg->length = segLength;
			memcpy(seg->content, buffer, segLength);
			if (sm_SemTake(stp->uplinkMutex) < 0)
			{
				exit(1);
			}

			if (lyst_insert_last(stp->uplinkSegments, seg) == NULL)
			{
				puts("owlttb out of memory.");
				exit(1);
			}

			sm_SemGive(stp->uplinkMutex);
			if (stp->verbose)
			{
				writeTimestampLocal(time(NULL), timebuf);
				printf("at %s owlt got an uplink seg of \
length %d.\n", timebuf, segLength);
			}
		}

		close(fwdSocket);
	}
}

/*	*	*	Main thread functions	*	*	*	*/

int	main(int argc, char *argv[])
{
	SimThreadParms	stpBuf;
	char		*end;
	pthread_t	uplinkThread;
	pthread_t	downlinkThread;
	int		cmdFile;
	char		line[256];
	int		lineLength;
	unsigned long	owlt;

	/*	Read configuration file, parsing each line.  For
	 *	each line, start a thread that simulates owlt on
	 *	the indicated link.  Then snooze forever.		*/

	stpBuf.verbose = 0;
	switch (argc)
	{
	case 9:
		if (strcmp(argv[8], "-v") == 0)
		{
			stpBuf.verbose = 1;
		}

		/*	Intentional fall-through to next case.		*/

	case 8:
		stpBuf.ownUplinkPortNbr = strtoul(argv[1], &end, 0);
		if (*end != '\0')
		{
			printf("owlttb: invalid own uplink port# '%s'.\n",
					argv[1]);
			exit(1);
		}

		stpBuf.ownDownlinkPortNbr = strtoul(argv[2], &end, 0);
		if (*end != '\0')
		{
			printf("owlttb: invalid own downlink port# '%s'.\n",
					argv[2]);
			exit(1);
		}

		if (inet_pton(AF_INET, argv[3],
				&stpBuf.destUplinkIpAddress.s_addr) < 1)
		{
			printf("owlttb: invalid dest. U/L IP address '%s'.\n",
					argv[3]);
			exit(1);
		}

		stpBuf.destUplinkPortNbr = strtoul(argv[4], &end, 0);
		if (*end != '\0')
		{
			printf("owlttb: invalid dest. uplink port# '%s'.\n",
					argv[4]);
			exit(1);
		}

		if (inet_pton(AF_INET, argv[5],
				&stpBuf.destDownlinkIpAddress.s_addr) < 1)
		{
			printf("owlttb: invalid dest. D/L IP address '%s'.\n",
					argv[5]);
			exit(1);
		}

		stpBuf.destDownlinkPortNbr = strtoul(argv[6], &end, 0);
		if (*end != '\0')
		{
			printf("owlttb: invalid dest. downlink port# '%s'.\n",
					argv[6]);
			exit(1);
		}

		stpBuf.owlt = strtoul(argv[7], &end, 0);
		if (*end != '\0')
		{
			printf("owlttb: invalid OWLT (sec.) '%s'.\n", argv[7]);
			exit(1);
		}

		break;

	default:
		puts("Usage:  owlttb <own uplink port#> <own downlink port#> \
<dest uplink IP address> <dest uplink port#> <dest downlink IP address> \
<dest downlink port#> <owlt sec.> [-v]");
		exit(0);
	}

	isignal(SIGPIPE, SIG_IGN);
	sm_ipc_init();
	if (pthread_create(&uplinkThread, NULL, offerUplink, &stpBuf))
	{
		perror("owlttb can't spawn uplink thread");
		exit(1);
	}

	if (pthread_create(&downlinkThread, NULL, offerDownlink, &stpBuf))
	{
		perror("owlttb can't spawn downlink thread");
		exit(1);
	}

	/*	Now accept OWLT changes in real time.			*/

	cmdFile = fileno(stdin);
	while (1)
	{
		printf(": ");
		fflush(stdout);
		if (igets(cmdFile, line, sizeof line, &lineLength) == NULL)
		{
			putErrmsg("igets failed.", NULL);
			break;		/*	Out of loop.		*/
		}

		if (lineLength < 1)
		{
			continue;
		}

		switch (line[0])
		{
		case 'q':		/*	Quit.			*/
			break;		/*	Out of switch.		*/

		default:
			owlt = strtoul(line, &end, 0);
			if (*end != '\0')
			{
				printf("invalid OWLT entry '%s'.  To exit, \
enter 'q'.\n", line);
				continue;
			}

			stpBuf.owlt = owlt;
			continue;
		}

		break;			/*	Out of loop.		*/
	}

	puts("owlttb is ending.");
	exit(0);
}
