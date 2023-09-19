/*

	udp2file.c:	a test consumer of DGR activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2udp.h>

static int	stopped = 0;

static void	handleQuit()
{
	stopped = 1;
}

static int	cycleNbr = 0;

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
	struct sockaddr		remoteSocknm;
	socklen_t		remoteSocknmSize;
	int			sock;
	FILE			*outputFile;
	char			line[256];
	int			lineSize;

	/*	Prepare for UDP reception.				*/

	getNameOfHost(ownHostName, sizeof ownHostName);
	ownIpAddress = getInternetAddress(ownHostName);
	hostNbr = htonl(ownIpAddress);
	memset((char *) &socketName, 0, sizeof socketName);
	inetName = (struct sockaddr_in *) &socketName;
	inetName->sin_family = AF_INET;
	inetName->sin_port = htons(portNbr);
	memcpy((char *) &(inetName->sin_addr.s_addr), (char *) &hostNbr, 4);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		perror("Can't open udp socket");
		exit(0);
	}

	if (bind(sock, &socketName, sizeof(socketName)) < 0)
	{
		perror("Can't bind udp socket");
	}

	outputFile = openFile();
	if (outputFile == NULL) exit(0);
	signal(SIGINT, handleQuit);
	while (1)
	{
		if (stopped)
		{
			break;
		}

		lineSize = sizeof line;
		remoteSocknmSize = sizeof remoteSocknm;
		lineSize = recvfrom(sock, line, lineSize, 0, &remoteSocknm,
				&remoteSocknmSize);
		if (lineSize < 0)
		{
			perror("udp receive failed");
			exit(0);
		}

		/*	Acknowledge the transmission.			*/

		if (sendto(sock, line, 4, 0, &remoteSocknm, remoteSocknmSize)
				!= 4)
		{
			perror("udp send failed");
			exit(0);
		}

		/*	Process text of line.				*/

		line[lineSize] = '\0';
		if (strcmp(line + 4, EOF_LINE_TEXT) == 0)
		{
			fclose(outputFile);
			outputFile = openFile();
			if (outputFile == NULL) exit(0);
			printf("working on cycle %d.\n", cycleNbr);
		}
		else	/*	Just write line to output file.		*/
		{
			if (fputs(line + 4, outputFile) < 0)
			{
				perror("Can't write to output file");
				exit(0);
			}
		}
	}

	close(sock);
	return 0;
}
