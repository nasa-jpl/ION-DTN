/*

	file2tcp.c:	a TCP benchmark sender.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2tcp.h>

static unsigned short		portNbr = TEST_PORT_NBR;
static char			*eofLine = EOF_LINE_TEXT;
static int			eofLineLen;
static int			cyclesRequested = 1;
static int			nbrOfConnections = 0;

static int	connectToRecvr(struct sockaddr *sn, int *sock)
{
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock < 0)
	{
		perror("Can't open TCP socket");
		return -1;
	}

	if (connect(*sock, sn, sizeof(struct sockaddr)) < 0)
	{
		close(*sock);
		*sock = -1;
		perror("Can't TCP-connect to receiver");
		return -1;
	}

	nbrOfConnections++;
	return 0;
}

static int	sendBytesByTCP(int *contactSocket, char *from, int length)
{
	int	bytesWritten;

	while (1)
	{
		bytesWritten = write(*contactSocket, from, length);
		if (bytesWritten < 0)
		{
			switch (errno)
			{
			case EINTR:	/*	Interrupted; retry.	*/
				continue;

			case EPIPE:	/*	Lost connection.	*/
			case EBADF:
			case ECONNRESET:
				close(*contactSocket);
				*contactSocket = -1;
			}

			perror("write() error on socket");
		}

		return bytesWritten;
	}
}

static int	sendBufferByTCP(struct sockaddr *socketName, int *contactSocket,
			unsigned char *buffer, int length, int *totalBytesSent)
{
	int		result;
	unsigned long	preamble;
	int		bytesToSend;
	char		*from;
	int		bytesSent;

	*totalBytesSent = 0;
	if (*contactSocket < 0)
	{
		result = connectToRecvr(socketName, contactSocket);
		if (result)
		{
			/*	Treat I/O error as a transient anomaly,
			 *	note incomplete transmission.		*/

			return 0;
		}
	}

	/*	Send preamble (length), telling Recvr how much to read.	*/

	preamble = htonl(length);
	bytesToSend = sizeof(preamble);
	from = (char *) &preamble;
	while (bytesToSend > 0)
	{
		bytesSent = sendBytesByTCP(contactSocket, from, bytesToSend);
		if (bytesSent < 0)
		{
			if (*contactSocket == -1)
			{
				/*	Just lost connection; treat
				 *	as a transient anomaly, note
				 *	incomplete transmission.	*/

				return 0;
			}

			/*	Big problem; shut down.			*/

			return -1;
		}

		from += bytesSent;
		bytesToSend -= bytesSent;
	}

	/*	Send the buffer itself.					*/

	bytesToSend = length;
	from = (char *) buffer;
	while (bytesToSend > 0)
	{
		bytesSent = sendBytesByTCP(contactSocket, from, bytesToSend);
		if (bytesSent < 0)
		{
			if (*contactSocket == -1)
			{
				/*	Just lost connection; treat
				 *	as a transient anomaly, note
				 *	incomplete transmission.	*/

				return 0;
			}

			/*	Big problem; shut down.			*/

			return -1;
		}

		from += bytesSent;
		bytesToSend -= bytesSent;
		*totalBytesSent += bytesSent;
	}

	return 0;
}

static void	report(struct timeval *startTime, unsigned long bytesSent)
{
	struct timeval	endTime;
	unsigned long	usec;
	float		rate;

	getCurrentTime(&endTime);
	if (endTime.tv_usec < startTime->tv_usec)
	{
		endTime.tv_sec--;
		endTime.tv_usec += 1000000;
	}

	printf("Made %d connections.\n", nbrOfConnections);
	usec = ((endTime.tv_sec - startTime->tv_sec) * 1000000)
			+ (endTime.tv_usec - startTime->tv_usec);
	printf("Bytes sent = %lu, usec elapsed = %lu.\n", bytesSent, usec);
	rate = (float) (8 * bytesSent) / (float) (usec / 1000000);
	printf("Sending %7.2f bits per second.\n", rate);
}

int	main(int argc, char **argv)
{
	int			nbrOfPeers;
	int			fdPoolSize = 250;
	int			cyclesLeft;
	char			*remoteHostName;
	unsigned long		remoteIpAddress;
	unsigned long		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	int			contactSocket = -1;
	char			*fileName;
	FILE			*inputFile;
	char			line[256];
	int			lineLen;
	int			result;
	struct timeval		startTime;
	unsigned long		bytesSent;

	if (argc < 4)
	{
		puts("Usage:  file2tcp <remote host> <name of file to copy> \
<number of peers> [<number of cycles; default = 1>]");
		exit(0);
	}

	nbrOfPeers = atoi(argv[3]);
	if (nbrOfPeers < 1)
	{
		nbrOfPeers = 1;
	}

	if (argc > 4)
	{
		cyclesRequested = atoi(argv[4]);
		if (cyclesRequested < 1)
		{
			cyclesRequested = 1;
		}
	}

	cyclesLeft = cyclesRequested;

	/*	Prepare for TCP connections.				*/

	remoteHostName = argv[1];
	remoteIpAddress = getInternetAddress(remoteHostName);
	hostNbr = htonl(remoteIpAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);

	/*	Get file to copy.					*/

	fileName = argv[2];
	inputFile = fopen(fileName, "r");
	if (inputFile == NULL)
	{
		perror("Can't open input file");
		exit(0);
	}

	eofLineLen = strlen(eofLine);
	getCurrentTime(&startTime);
	bytesSent = 0;

	/*	Copy text lines from file to receiver.			*/

	while (cyclesLeft > 0)
	{
		if (fgets(line, 256, inputFile) == NULL)
		{
			if (feof(inputFile))
			{
				if (sendBufferByTCP(&socketName, &contactSocket,
						(unsigned char *) eofLine,
						eofLineLen, &result))
				{
					perror("tcp send failed");
					exit(0);
				}

				bytesSent += eofLineLen;
	 			if ((rand() % nbrOfPeers) >= fdPoolSize)
	 			{
	 				close(contactSocket);
	 				contactSocket = -1;
	 			}

				fclose(inputFile);
				inputFile = fopen(fileName, "r");
				if (inputFile == NULL)
				{
					perror("Can't reopen input file");
					exit(0);
				}

				cyclesLeft--;
				continue;
			}
			else
			{
				perror("Can't read from input file");
				exit(0);
			}
		}

		lineLen = strlen(line);
		if (sendBufferByTCP(&socketName, &contactSocket,
				(unsigned char *) line, lineLen, &result))
		{
			perror("tcp send failed");
			exit(0);
		}

		bytesSent += lineLen;
	 	if ((rand() % nbrOfPeers) >= fdPoolSize)
	 	{
	 		close(contactSocket);
	 		contactSocket = -1;
	 	}
	}

	report(&startTime, bytesSent);
	if (contactSocket >= 0)
	{
		close(contactSocket);
	}

	return 0;
}
