/** bprecvfile2.c:	Test program for receiving a files sent as a bundle. */

/** Adapted from Scott Burleigh's (JPL ret) bprecvfile.c		*/

/** 10-31-2022 dso david.s.orozco@jpl.nasa.gov */
/** Changed from saving testfile1, testfile2 etc to being able to specify */
/** a filename. It will create or append data to that file until the */
/** application is terminated. If no filename is specified it will write */
/** to stdout and the user may pipe it somewhere */

#include <bp.h>

#define	BPRECVBUFSZ	(65536)

typedef struct
	{
	BpSAP	sap;
	int	running;
	} BptestState;


/******************************************************************************/
/** _bptestState() */
/******************************************************************************/
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

/******************************************************************************/
/** handleQuit() */
/******************************************************************************/
static void	handleQuit(int signum)
	{
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	writeMemo("[i] bprecvfile interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
	}

/******************************************************************************/
/** receiveFile() */
/******************************************************************************/
static int	receiveFile(Sdr sdr, BpDelivery *dlv, int testFile)
	{
	static char	buffer[BPRECVBUFSZ];
	int		contentLength;
	int		remainingLength;
	char		fileName[64];
	ZcoReader	reader;
	int		recvLength;
//	char		progressText[80];


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
			putErrmsg("bprecvfile: can't receive bundle content.", fileName);
			oK(sdr_end_xn(sdr));
			return -1;
			}

		if (write(testFile, buffer, recvLength) < 1)
			{
			putSysErrmsg("bprecvfile: can't write to test file", fileName);
			oK(sdr_end_xn(sdr));
			return -1;
			}

		remainingLength -= recvLength;
		}

//	isprintf(progressText, sizeof progressText, "[i] bprecvfile has created '%s', size %d.", fileName, contentLength);
//	writeMemo(progressText);

	if (sdr_end_xn(sdr) < 0)
		{
		return -1;
		}

	return 0;
	}/** end of receiveFile()*/


/******************************************************************************/
/** main() */
/******************************************************************************/
#if defined (ION_LWT)
int	bprecvfile(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5, saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
	{
	char	*ownEid = (char *) a1;
	int		maxFiles = strtol((char *) a2, NULL, 0);

#else
int	main(int argc, char **argv)
	{
	char	* myfilename;
	int		testFile = -1;
	char	*ownEid = (argc > 1 ? argv[1] : NULL);
	char	progressText[80];
	char	answer;

	myfilename = malloc(20);

	/** If we have enough args, the user is specifying a filename */
	/** If not, just dump to stdout and let the user pipe it somewhere */
	if(argc>2)
		{
		strncpy(myfilename, argv[2],20);

		if(access(myfilename, F_OK) == 0)
			{
			isprintf(progressText, sizeof progressText, "[i] File '%s' exists. Do you want to append? (y/n)", myfilename);
			writeMemo(progressText);
			scanf("%c",&answer);
			if(answer != 0x79) { printf("79 %x\n",answer); return(-1);}
			} else
			{
			isprintf(progressText, sizeof progressText, "[i] File '%s' is created.", myfilename);
			writeMemo(progressText);
			}

		/** Open the file here and pass the descriptor to receiveFile() */
		/** This lets us just leave it open for the session */
		testFile = iopen(myfilename, O_WRONLY | O_CREAT | O_APPEND, 0666);
		}
		else /** argc < 3 */
		{
		testFile = STDOUT_FILENO;
		}

#endif

	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;

	if (ownEid == NULL)
		{
		PUTS("Usage: bprecvfile <own endpoint ID> <filename>");
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
			break;

			case BpPayloadPresent:
			if (receiveFile(sdr, &dlv, testFile) < 0)
				{
				putErrmsg("bprecvfile cannot continue.", NULL);
				state.running = 0;
				}

			default:
			break;
			}

		bp_release_delivery(&dlv, 1);
		}

	bp_close(state.sap);
	PUTS("Stopping bprecvfile.");
	writeMemo("[i] Stopping bprecvfile.");
	writeErrmsgMemos();
	bp_detach();

	close(testFile);
	free(myfilename);

	return 0;
	} /** end of main() */

/**
"Copyright 2022, by the California Institute of Technology. ALL RIGHTS RESERVED.
United States Government Sponsorship acknowledged. Any commercial use must be
negotiated with the Office of Technology Transfer at the California Institute
of Technology.
 
This software may be subject to U.S. export control laws.
By accepting this software, the user agrees to comply with all applicable U.S.
export laws and regulations. User has the responsibility to obtain export
licenses, or other export authority as may be required before exporting such
information to foreign countries or providing access to foreign persons."

*/
