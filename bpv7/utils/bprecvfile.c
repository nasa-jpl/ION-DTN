/*
	bprecvfile.c:	Test program for receiving a files sent as
			a bundle.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

#define	BPRECVBUFSZ	(65536)

typedef struct
{
	BpSAP	sap;
	int	running;
} BptestState;

static BptestState	*_bptestState(BptestState *newState)
{
	void		*value;
	BptestState	*state;

	if (newState)			/*	Add task variable.	*/
	{
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	handleQuit(int signum)
{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	writeMemo("[i] bprecvfile interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static int	receiveFile(Sdr sdr, BpDelivery *dlv)
{
	static char	buffer[BPRECVBUFSZ];
	static int	fileCount = 0;
	int		contentLength;
	int		remainingLength;
	char		fileName[64];
	int		testFile = -1;
	ZcoReader	reader;
	int		recvLength;
	char		progressText[80];

	fileCount++;
	isprintf(fileName, sizeof fileName, "testfile%d", fileCount);
	contentLength = zco_source_data_length(sdr, dlv->adu);
	isprintf(progressText, sizeof progressText, "[i] bprecvfile is \
creating '%s', size %d.", fileName, contentLength);
	writeMemo(progressText);
	testFile = iopen(fileName, O_WRONLY | O_CREAT, 0666);
	if (testFile < 0)
	{
		putSysErrmsg("bprecvfile: can't open test file", fileName);
		return -1;
	}

	zco_start_receiving(dlv->adu, &reader);
	remainingLength = contentLength;
	oK(sdr_begin_xn(sdr));
	while (remainingLength > 0)
	{
		recvLength = BPRECVBUFSZ;
		if (remainingLength < recvLength)
		{
			recvLength = remainingLength;
		}

		if (zco_receive_source(sdr, &reader, recvLength, buffer) < 0)
		{
			putErrmsg("bprecvfile: can't receive bundle content.",
					fileName);
			close(testFile);
			oK(sdr_end_xn(sdr));
			return -1;
		}

		if (write(testFile, buffer, recvLength) < 1)
		{
			putSysErrmsg("bprecvfile: can't write to test file",
					fileName);
			close(testFile);
			oK(sdr_end_xn(sdr));
			return -1;
		}

		remainingLength -= recvLength;
	}

	isprintf(progressText, sizeof progressText, "[i] bprecvfile has \
created '%s', size %d.", fileName, contentLength);
	writeMemo(progressText);
	close(testFile);
	if (sdr_end_xn(sdr) < 0)
	{
		return -1;
	}

	return 0;
}

#if defined (ION_LWT)
int	bprecvfile(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownEid = (char *) a1;
	int		maxFiles = strtol((char *) a2, NULL, 0);
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
	int		maxFiles = (argc > 2 ? strtol(argv[2], NULL, 0) : 0);
#endif
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;
	int		filesReceived = 0;

	if (ownEid == NULL)
	{
		PUTS("Usage: bprecvfile <own endpoint ID> [<max # files>]");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return -1;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return -1;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	writeMemo("[i] bprecvfile is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bprecvfile bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			state.running = 0;
			break;		/*	Out of switch.		*/

		case BpPayloadPresent:
			filesReceived++;
			if (receiveFile(sdr, &dlv) < 0)
			{
				putErrmsg("bprecvfile cannot continue.", NULL);
				state.running = 0;
			}

			/*	Intentional fall-through to default.	*/

		default:
			break;		/*	Out of switch.		*/
		}

		if (maxFiles > 0)
		{
			if (filesReceived == maxFiles)
			{
				writeMemo("[i] bprecvfile has reached limit.");
				state.running = 0;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	PUTS("Stopping bprecvfile.");
	writeMemo("[i] Stopping bprecvfile.");
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
