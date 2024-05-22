/*
	bprecvtest.c:	Test program for receiving files from bptestfiles 
		expects a pilot bundle to start timer followed by a specified 
		number of files, whose length are recorded then the contents 
		are discarded.									*/
/*	Modified from bprecvfile.c by Silas Springer		*/
/*														*/

#include <bp.h>
#include <time.h>

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
	writeMemo("[i] bprecvtest interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static int	receiveFile(Sdr sdr, BpDelivery *dlv)
{
	static char	buffer[BPRECVBUFSZ];
	int		contentLength;
	int		remainingLength;
	char		fileName[64];
	int		testFile = -1;
	ZcoReader	reader;
	int		recvLength;

	contentLength = zco_source_data_length(sdr, dlv->adu);

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
			putErrmsg("bprecvtest: can't receive bundle content.",
					fileName);
			close(testFile);
			oK(sdr_end_xn(sdr));
			return -1;
		}
		/* count number of bytes recieved, but do not store them */
		remainingLength -= recvLength;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		return -1;
	}

	return contentLength;
}

#if defined (ION_LWT)
int	bprecvtest(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
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
	int		pilotReceived = 0;
	int		filesReceived = 0;
	int		bytesReceived = 0;
	struct timeval	startt;
	struct timeval	endt;

	if (ownEid == NULL)
	{
		PUTS("Usage: bprecvtest <own endpoint ID> [<max # files>]");
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
	writeMemo("[i] bprecvtest is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bprecvtest bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			state.running = 0;
			break;		/*	Out of switch.		*/

		case BpPayloadPresent:
			/* receive pilot bundle */
			if(!pilotReceived){
				pilotReceived = 1;
				getCurrentTime(&startt);
				printf("Recieved pilot bundle, timer started.\n");
				break;
			}
			filesReceived++;
			int filelength = receiveFile(sdr, &dlv);
			if (filelength < 0)
			{
				putErrmsg("bprecvtest cannot continue.", NULL);
				state.running = 0;
			}else{
				bytesReceived += filelength;
			}

			/*	Intentional fall-through to default.	*/

		default:
			break;		/*	Out of switch.		*/
		}

		if (maxFiles > 0)
		{
			if (filesReceived == maxFiles)
			{
				writeMemo("[i] bprecvtest has reached limit.");
				state.running = 0;
			}
		}

		bp_release_delivery(&dlv, 1);
	}
	getCurrentTime(&endt);
	if (endt.tv_usec < startt.tv_usec){
		endt.tv_usec += 1e6;
		endt.tv_sec -= 1;
	}
	double interval = (endt.tv_usec - startt.tv_usec)
	+ (1e6 * (endt.tv_sec - startt.tv_sec));
	interval /= 1e6;
	
	printf("Received %d bytes in %lf seconds: %f Mbps\n", bytesReceived, interval, (((double)bytesReceived)/interval)/125000 );

	bp_close(state.sap);
	PUTS("Stopping bprecvtest.");
	writeMemo("[i] Stopping bprecvtest.");
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
