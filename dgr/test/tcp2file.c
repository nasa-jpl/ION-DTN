/*

	tcp2file.c:	a TCP benchmark receiver.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2tcp.h>

static int	stopped = 0;
static int	cycleNbr = 0;
static int	accessSocket = -1;

static void	handleQuit()
{
	stopped = 1;
}

static int	receiveBytesByTCP(int *sock, char *into, int length)
{
	int		bytesRead;
	struct sockaddr	newSocketName;
	socklen_t	nameLength = sizeof(newSocketName);

	while (1)
	{
		bytesRead = read(*sock, into, length);
		switch (bytesRead)
		{
		case -1:
			if (errno == EINTR)
			{
				continue;
			}

			perror("read() error on socket");
			return -1;

			/*	Intentional fall-through to next case.	*/

		case 0:			/*	Connection closed.	*/
			close(*sock);
			while (1)
			{
				*sock = accept(accessSocket, &newSocketName,
						&nameLength);
				if (*sock < 0)
				{
					if (errno == EINTR)
					{
						continue;
					}

					perror("accept() failed");
					exit(0);
				}

				break;	/*	Out of accept() loop.	*/
			}

			continue;	/*	read() loop.		*/

		default:
			return bytesRead;
		}
	}
}

static int	receiveData(int *sock, char *buffer, int *bufferLength)
{
	unsigned long	preamble;
	int		bytesToReceive;
	char		*into;
	int		bytesReceived;
	int		length;

	/*	Receive length of transmitted data buffer.		*/

	length = sizeof preamble;
	bytesToReceive = length;
	into = (char *) &preamble;
	while (bytesToReceive > 0)
	{
		bytesReceived = receiveBytesByTCP(sock, into, bytesToReceive);
		if (bytesReceived < 0)
		{
			return -1;
		}

		into += bytesReceived;
		bytesToReceive -= bytesReceived;
	}

	/*	Receive the data buffer itself.				*/

	length = ntohl(preamble);
	*bufferLength = bytesToReceive = length;
	into = buffer;
	while (bytesToReceive > 0)
	{
		bytesReceived = receiveBytesByTCP(sock, into, bytesToReceive);
		if (bytesReceived < 0)
		{
			return -1;
		}

		into += bytesReceived;
		bytesToReceive -= bytesReceived;
	}

	return 0;
}

static FILE	*openFile()
{
	char	fileName[256];
	FILE	*outputFile;

	cycleNbr++;
	isprintf(fileName, sizeof fileName, "file_copy_%d", cycleNbr);
	outputFile = fopen(fileName, "a");
	if (outputFile == NULL)
	{
		perror("Can't open output file");
	}

	return outputFile;
}

int	main(int argc, char **argv)
{
	char			ownHostName[MAXHOSTNAMELEN + 1];
	unsigned long		ownIpAddress;
	unsigned short		portNbr = TEST_PORT_NBR;
	unsigned long		hostNbr;
	struct sockaddr		socketName;
	struct sockaddr_in	*inetName;
	socklen_t		nameLength;
	int			newSocket = -1;
	struct sockaddr		newSocketName;
	FILE			*outputFile;
	char			line[256];
	int			lineSize;

	getNameOfHost(ownHostName, sizeof ownHostName);
	ownIpAddress = getInternetAddress(ownHostName);

	/*	Open dial-up socket to accept connections on.		*/

	hostNbr = htonl(ownIpAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	accessSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (accessSocket < 0)
	{
		perror("Can't open TCP socket");
		exit(0);
	}

	nameLength = sizeof(struct sockaddr);
	if (reUseAddress(accessSocket)
	|| bind(accessSocket, &socketName, nameLength) < 0
	|| listen(accessSocket, 5) < 0
	|| getsockname(accessSocket, &socketName, &nameLength) < 0)
	{
		close(accessSocket);
		perror("Can't initialize socket");
		exit(0);
	}

	signal(SIGINT, handleQuit);

	/*	Open first output file copy.				*/

	outputFile = openFile();
	if (outputFile == NULL) exit(0);

	/*	Accept initial connection.				*/

	while (1)
	{
		newSocket = accept(accessSocket, &newSocketName, &nameLength);
		if (newSocket < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			perror("accept() failed");
			exit(0);
		}

		break;
	}

	/*	Begin reception loop.					*/

	while (1)
	{
		if (stopped)
		{
			break;
		}

		lineSize = sizeof line - 1;
		if (receiveData(&newSocket, line, &lineSize))
		{
			exit(0);
		}

		/*	Process text of line.				*/

		line[lineSize] = '\0';
		if (strcmp(line, EOF_LINE_TEXT) == 0)
		{
			fclose(outputFile);
			outputFile = openFile();
			if (outputFile == NULL) exit(0);
			printf("working on cycle %d.\n", cycleNbr);
		}
		else	/*	Just write line to output file.		*/
		{
			if (fputs(line, outputFile) < 0)
			{
				perror("Can't write to output file");
				exit(0);
			}
		}
	}

	return 0;
}
