/*
	lgagent.c:	agent for BP-based load-and-go system.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

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
	PUTS("BP reception interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

static void	closeOpsFile(int *opsFile)
{
	if (*opsFile >= 0)
	{
		close(*opsFile);
		*opsFile = -1;
	}
}

static int	processCmdFile(Sdr sdr, BpDelivery *dlv)
{
	int		contentLength;
	ZcoReader	reader;
	int		len;
	char		*content;
	char		*endOfContent;
	char		*line;
	char		*delimiter;
	char		*nextLine;
	int		lineLength;
	char		*fileName = NULL;
	int		opsFile = -1;

	contentLength = zco_source_data_length(sdr, dlv->adu);
	if (contentLength > 64000)
	{
		putErrmsg("lgagent: bundle content length > 64000, ignored.",
				itoa(contentLength));
		return 0;
	}

	content = MTAKE(contentLength + 1);
	if (content == NULL)
	{
		putErrmsg("lgagent: no space for bundle content.", NULL);
		return -1;
	}

	zco_start_receiving(dlv->adu, &reader);
	CHKERR(sdr_begin_xn(sdr));
	len = zco_receive_source(sdr, &reader, contentLength, content);
	if (sdr_end_xn(sdr) < 0)
	{
		return -1;
	}

	if (len < 0)
	{
		MRELEASE(content);
		putErrmsg("lgagent: can't receive bundle content.", NULL);
		return -1;
	}

	endOfContent = content + contentLength;
	*endOfContent = 0;
	line = content;
	while (line < endOfContent)
	{
		delimiter = strchr(line, '\n');	/*	LF (newline)	*/
		if (delimiter == NULL)		/*	No LF or CRLF	*/
		{
			writeMemoNote("[?] lgagent: non-terminated line, \
discarding bundle content", content);
			closeOpsFile(&opsFile);
			fileName = NULL;
			break;			/*	Out of loop.	*/
		}

		nextLine = delimiter + 1;
		lineLength = nextLine - line;
		*delimiter = 0;		/*	Strip off the LF.	*/
		if (lineLength == 1)	/*	Empty line.		*/
		{
			line = nextLine;
			continue;
		}

		/*	Case 1: line is start of an operations file.	*/

		if (*line == '[')	/*	Start loading file.	*/
		{
			if (fileName)
			{
				putErrmsg("lgagent: '[' line before end of \
load, no further activity.", itoa(line - content));
				closeOpsFile(&opsFile);
				fileName = NULL;
				break;		/*	Out of loop.	*/
			}

			/*	Remainder of line is file name.		*/

			fileName = line + 1;
			opsFile = iopen(fileName, O_RDWR | O_CREAT, 0777);
			if (opsFile < 0)
			{
				putSysErrmsg("lgagent: can't open operations \
file name, no further activity", fileName);
				fileName = NULL;
				break;		/*	Out of loop.	*/
			}

#ifdef TargetFFS
			closeOpsFile(&opsFile);
#endif
			line = nextLine;
			continue;
		}

		/*	Case 2: line terminates operations file.	*/

		if (*line == ']')	/*	End loading of file.	*/
		{
			if (fileName == NULL)
			{
				putErrmsg("lgagent: ']' line before start of \
load, no further activity.", itoa(line - content));
				break;		/*	Out of loop.	*/
			}

			/*	Close the current ops file.		*/

			putErrmsg("lgagent: loaded ops file.", fileName);
			closeOpsFile(&opsFile);
			fileName = NULL;
			line = nextLine;
			continue;
		}

		/*	Case 3: line is part of operations file.	*/

		if (fileName)	/*	Currently loading a file.	*/
		{
			/*	Append this command-file line to the
			 *	ops file that is being loaded.		*/
#ifdef TargetFFS
			if (opsFile == -1)	/*	Must reopen.	*/
			{
				if ((opsFile = iopen(fileName, O_RDWR, 0)) < 0
				|| lseek(opsFile, SEEK_END, 0) < 0)
				{
					putSysErrmsg("lgagent: can't reopen \
operations file name, no further activity", fileName);
					break;	/*	Out of loop.	*/
				}
			}
#endif
			lineLength -= 1;	/*	Strip off NULL.	*/
			if (write(opsFile, line, lineLength) < 0
			|| write(opsFile, "\n", 1) < 0)
			{
				putSysErrmsg("lgagent: can't append line to \
operations file, no further activity", fileName);
				closeOpsFile(&opsFile);
				fileName = NULL;
				break;		/*	Out of loop.	*/
			}

#ifdef TargetFFS
			closeOpsFile(&opsFile);
#endif
			line = nextLine;
			continue;
		}

		/*	Case 4: not loading, line is a "Go" command.	*/

		if (*line == '!')
		{
			putErrmsg("lgagent: executing Go command.", line + 1);
			if (pseudoshell(line + 1) < 0)
			{
				putErrmsg("lgagent: pseudoshell failed.", NULL);
				break;		/*	Out of loop.	*/
			}

			snooze(1);	/*	Let command finish.	*/
			line = nextLine;
			continue;
		}

		/*	Default case: malformed LG command file.	*/

		putErrmsg("lgagent: failure parsing command file, no further \
activity.", itoa(line - content));
		break;				/*	Out of loop.	*/
	}

	MRELEASE(content);
	if (fileName)
	{
		closeOpsFile(&opsFile);
	}

	return 0;
}

#if defined (ION_LWT)
int	lgagent(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*ownEid = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
#endif
	BptestState	state = { NULL, 1 };
	Sdr		sdr;
	BpDelivery	dlv;

	if (ownEid == NULL)
	{
		PUTS("Usage: lgagent <own endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &state.sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	writeMemo("[i] lgagent is running.");
	while (state.running)
	{
		if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("lgagent bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpEndpointStopped:
			state.running = 0;
			break;		/*	Out of switch.		*/

		case BpPayloadPresent:
			if (processCmdFile(sdr, &dlv) < 0)
			{
				putErrmsg("lgagent cannot continue.", NULL);
				state.running = 0;
			}

			/*	Intentional fall-through to default.	*/

		default:
			break;		/*	Out of switch.		*/
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(state.sap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping lgagent.");
	bp_detach();
	return 0;
}
