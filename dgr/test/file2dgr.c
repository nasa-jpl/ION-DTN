/*

	file2dgr.c:	a test producer of DGR activity.

									*/
/*									*/
/*	Copyright (c) 2003, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <platform.h>
#include <file2dgr.h>
#include <psm.h>
#include <memmgr.h>

static int		wmSize = 10000000;
static char		*wmPtr;
static PsmView		dgrPartition;
static PsmPartition	dgrwm = &dgrPartition;
static char		*eofLine = EOF_LINE_TEXT;
static int		eofLineLen;
static int		cyclesRequested = 1;
static Dgr		dgr;

static void	*allocFromDgrMemory(const char *fileName, int lineNbr,
			size_t length)
{
	PsmAddress	address;
	void		*block;

	address = Psm_zalloc(fileName, lineNbr, dgrwm, length);
	if (address == 0)
	{
		return NULL;
	}

	block = psp(dgrwm, address);
	memset(block, 0, length);
	return block;
}

static void	releaseToDgrMemory(const char *fileName, int lineNbr,
			void *block)
{
	Psm_free(fileName, lineNbr, dgrwm, psa(dgrwm, (char *) block));
}

static void	*dgrAtoP(uaddr address)
{
	return (void *) psp(dgrwm, address);
}

static uaddr	 dgrPtoA(void *pointer)
{
	return (uaddr) psa(dgrwm, pointer);
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

	usec = ((endTime.tv_sec - startTime->tv_sec) * 1000000)
			+ (endTime.tv_usec - startTime->tv_usec);
	printf("Bytes sent = %lu, usec elapsed = %lu.\n", bytesSent, usec);
	rate = (float) (8 * bytesSent) / (float) (usec / 1000000);
	printf("Sending %7.2f bits per second.\n", rate);
}

static int	run_file2dgr(char *remoteHostName, char *fileName)
{
	int		cyclesLeft;
	char		ownHostName[MAXHOSTNAMELEN + 1];
	unsigned int	ownIpAddress;
	unsigned int	remoteIpAddress;
	unsigned short	remotePortNbr = TEST_PORT_NBR;
	PsmMgtOutcome	outcome;
	DgrRC		rc;
	FILE		*inputFile;
	char		line[256];
	int		lineLen;
	struct timeval	startTime;
	unsigned long	bytesSent;

	cyclesLeft = cyclesRequested;
	getNameOfHost(ownHostName, sizeof ownHostName);
	ownIpAddress = getInternetAddress(ownHostName);
	remoteIpAddress = getInternetAddress(remoteHostName);
	sm_ipc_init();
	wmPtr = malloc(wmSize);
	if (wmPtr == NULL
	|| psm_manage(wmPtr, wmSize, "dgr", &dgrwm, &outcome) < 0
	|| outcome == Refused)
	{
		putErrmsg("Can't acquire DGR working memory.", NULL);
		writeErrmsgMemos();
		return 0;
	}
#if 0
psm_start_trace(dgrwm, 10000000, NULL);
#endif

	memmgr_add("dgr", allocFromDgrMemory, releaseToDgrMemory, dgrAtoP,
			dgrPtoA);
	if (dgr_open(ownIpAddress, 2, 0, ownIpAddress, "dgr", &dgr, &rc) < 0
	|| rc != DgrOpened)
	{
		putErrmsg("Can't open dgr service.", NULL);
		writeErrmsgMemos();
		return 0;
	}

	inputFile = fopen(fileName, "r");
	if (inputFile == NULL)
	{
		putSysErrmsg("Can't open input file", fileName);
		writeErrmsgMemos();
		return 0;
	}

	eofLineLen = strlen(eofLine);
	getCurrentTime(&startTime);
	bytesSent = 0;

	/*	Copy text lines from file to SDR.			*/

	while (cyclesLeft > 0)
	{
		if (fgets(line, 256, inputFile) == NULL)
		{
			if (feof(inputFile))
			{
				if (dgr_send(dgr, remotePortNbr,
					remoteIpAddress, DGR_NOTE_FAILED,
					eofLine, eofLineLen, &rc) < 0)
				{
					putErrmsg("dgr_send failed.", NULL);
					writeErrmsgMemos();
					fclose(inputFile);
					return 0;
				}

				bytesSent += eofLineLen;
				fclose(inputFile);
				cyclesLeft--;
				if (cyclesLeft == 0)
				{
					inputFile = NULL;
					break;
				}

				inputFile = fopen(fileName, "r");
				if (inputFile == NULL)
				{
					putSysErrmsg("Can't reopen input file",
							NULL);
					writeErrmsgMemos();
					return 0;
				}

				continue;
			}
			else
			{
				putSysErrmsg("Can't read from input file",
						NULL);
				writeErrmsgMemos();
				fclose(inputFile);
				return 0;
			}
		}

		lineLen = strlen(line);
		if (dgr_send(dgr, remotePortNbr, remoteIpAddress,
				DGR_NOTE_FAILED, line, lineLen, &rc) < 0)
		{
			putErrmsg("dgr_send failed", NULL);
			writeErrmsgMemos();
			fclose(inputFile);
			return 0;
		}

		bytesSent += lineLen;
	}

	report(&startTime, bytesSent);
	writeMemo("[i] file2dgr waiting 10 sec for retransmission to stop.");
	snooze(10);
	dgr_close(dgr);
#if 0
psm_print_trace(dgrwm, 0);
psm_stop_trace(dgrwm);
#endif
	if (inputFile)
	{
		fclose(inputFile);
	}

	return 0;
}

#if defined (ION_LWT)
int	file2dgr(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*remoteHostName = (char *) a1;
	char	*fileName = (char *) a2;

	if (a3 > 0)
	{
		cyclesRequested = a3;
	}
#else
int	main(int argc, char **argv)
{
	char	*remoteHostName;
	char	*fileName;

	if (argc < 3)
	{
		writeMemo("Usage:  file2dgr <remote host> <name of file to \
copy> [<number of cycles; default = 1>]");
		return 0;
	}

	remoteHostName = argv[1];
	fileName = argv[2];
	if (argc > 3)
	{
		cyclesRequested = atoi(argv[3]);
		if (cyclesRequested < 1)
		{
			cyclesRequested = 1;
		}
	}
#endif
	return run_file2dgr(remoteHostName, fileName);
}
