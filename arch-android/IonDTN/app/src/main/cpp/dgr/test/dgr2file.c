/*

	dgr2file.c:	a test consumer of DGR activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <file2dgr.h>

static int	dgr2file_stopped = 0;
static Dgr	dgr2file_dgr;
static int	cycleNbr = 0;

static void	handleQuit()
{
	dgr2file_stopped = 1;
	dgr_interrupt(dgr2file_dgr);
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
		perror("can't open output file");
	}

	return outputFile;
}

#if defined (ION_LWT)
int	dgr2file(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
#else
int	main(int argc, char **argv)
#endif
{
	char		ownHostName[MAXHOSTNAMELEN + 1];
	unsigned int	ownIpAddress;
	unsigned short	portNbr = TEST_PORT_NBR;
	DgrRC		rc;
	FILE		*outputFile;
	unsigned int	remoteIpAddress;
	unsigned short	remotePortNbr;
	char		line[256];
	int		lineSize;
	int		errnbr;

	getNameOfHost(ownHostName, sizeof ownHostName);
	ownIpAddress = getInternetAddress(ownHostName);
	if (dgr_open(ownIpAddress, 2, portNbr, ownIpAddress, NULL,
			&dgr2file_dgr, &rc) < 0)
	{
		perror("can't open dgr service");
		return 0;
	}

	outputFile = openFile();
	if (outputFile == NULL) return 0;
	printf("working on cycle %d.\n", cycleNbr);
	signal(SIGINT, handleQuit);
	while (1)
	{
		if (dgr2file_stopped)
		{
			break;
		}

		lineSize = sizeof line;
		if (dgr_receive(dgr2file_dgr, &remotePortNbr,
				&remoteIpAddress, line, &lineSize, &errnbr,
				DGR_BLOCKING, &rc) < 0)
		{
			putErrmsg("dg_receive failed.", NULL);
			return 0;
		}

		switch (rc)
		{
		case DgrDatagramReceived:
			break;

		case DgrInterrupted:
			dgr2file_stopped = 1;
			continue;

		default:
			putErrmsg("dgr_receive got unrecognized result.",
					itoa(rc));
			return 0;
		}

		/*	Process text of line.				*/

		line[lineSize] = '\0';
		if (strcmp(line, EOF_LINE_TEXT) == 0)
		{
			fclose(outputFile);
			outputFile = openFile();
			if (outputFile == NULL) return 0;
			printf("working on cycle %d.\n", cycleNbr);
		}
		else	/*	Just write line to output file.		*/
		{
			if (fputs(line, outputFile) < 0)
			{
				perror("can't write to output file");
				return 0;
			}
		}
	}

	dgr_close(dgr2file_dgr);
	return 0;
}
