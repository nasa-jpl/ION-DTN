/*
	lgagent.c:	agent for BP-based load-and-go system.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bp.h>

static int	running;
static BpSAP	sap;

static void	handleQuit()
{
	running = 0;
	bp_interrupt(sap);
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

	sdr_begin_xn(sdr);
	zco_start_receiving(sdr, dlv->adu, &reader);
	len = zco_receive_source(sdr, &reader, contentLength, content);
	zco_stop_receiving(sdr, &reader);
	if (len < 0)
	{
		sdr_cancel_xn(sdr);
		MRELEASE(content);
		putErrmsg("lgagent: can't receive bundle content.", NULL);
		return -1;
	}

	if (sdr_end_xn(sdr) < 0)
	{
		MRELEASE(content);
		putErrmsg("lgagent: can't handle bundle delivery.", NULL);
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
			nextLine = endOfContent;
			lineLength = nextLine - line;
		}
		else	/*	Not the last line of the file.		*/
		{
			nextLine = delimiter + 1;
			lineLength = nextLine - line;
			*delimiter = 0;	/*	Strip off the LF.	*/
			if (delimiter > line)	/*	Non-empty line.	*/
			{
				/*	Strip off CR if present.	*/

				delimiter--;
				if (*delimiter == 0x0d)
				{
					/*	CR (DOS text); lose it.	*/

					*delimiter = 0;
					lineLength--;
				}
			}
		}

		/*	Case 1: line is start of an operations file.	*/

		if (*line == '[')	/*	Start loading file.	*/
		{
			if (fileName)
			{
				putErrmsg("lgagent: '[' line before end of \
load, no further activity.", itoa(line - content));
				close(opsFile);
				opsFile = -1;
				fileName = NULL;
				break;
			}

			/*	Remainder of line is file name.		*/

			fileName = line + 1;
			opsFile = open(fileName, O_RDWR | O_CREAT, 00777);
			if (opsFile < 0)
			{
				putSysErrmsg("lgagent: can't open operations \
file name, no further activity", fileName);
				fileName = NULL;
				break;
			}

#if TargetFFS
			close(opsFile);
			opsFile = -1;
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
				break;
			}

			/*	Close the current ops file.		*/

			putErrmsg("lgagent: loaded ops file.", fileName);
			close(opsFile);
			opsFile = -1;
			fileName = NULL;
			line = nextLine;
			continue;
		}

		/*	Case 3: line is part of operations file.	*/

		if (fileName)	/*	Currently loading a file.	*/
		{
			/*	Append this command-file line to the
			 *	ops file that is being loaded.		*/

			if (opsFile == -1)	/*	Must reopen.	*/
			{
				if ((opsFile = open(fileName, O_RDWR, 0)) < 0
				|| lseek(opsFile, SEEK_END, 0) < 0)
				{
					putSysErrmsg("lgagent: can't reopen \
operations file name, no further activity", fileName);
					break;
				}
			}

			if (write(opsFile, line, lineLength) < 0
			|| write(opsFile, "\n", 1) < 0)
			{
				putSysErrmsg("lgagent: can't append line to \
operations file, no further activity", fileName);
				close(opsFile);
				opsFile = -1;
				fileName = NULL;
				break;
			}

#if TargetFFS
			close(opsFile);
			opsFile = -1;
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
				break;
			}

			snooze(1);	/*	Let command finish.	*/
			line = nextLine;
			continue;
		}

		/*	Default case: malformed LG command file.	*/

		putErrmsg("lgagent: failure parsing command file, no further \
activity.", itoa(line - content));
		break;
	}

	MRELEASE(content);
	if (fileName)
	{
		close(opsFile);
	}

	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	lgagent(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*ownEid = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char		*ownEid = (argc > 1 ? argv[1] : NULL);
#endif
	Sdr		sdr;
	BpDelivery	dlv;

	if (ownEid == NULL)
	{
		puts("Usage: lgagent <own endpoint ID>");
		return 0;
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);
	running = 1;
	writeMemo("[i] lgagent is running.");
	while (running)
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("lgagent bundle reception failed.", NULL);
			running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent)
		{
			if (processCmdFile(sdr, &dlv) < 0)
			{
				putErrmsg("lgagent cannot continue.", NULL);
				running = 0;
			}
		}

		bp_release_delivery(&dlv, 1);
	}

	bp_close(sap);
	writeErrmsgMemos();
	writeMemo("[i] Stopping lgagent.");
	bp_detach();
	return 0;
}
