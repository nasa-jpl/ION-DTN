/*
	irradmin.c:	BP inter-regional routing adminstration
			utility.
									*/
/*									*/
/*	Copyright (c) 2021, IPNGROUP.  All rights reserved.		*/
/*	Author: Scott Burleigh, IPNGROUP				*/
/*									*/

#include "irr.h"

static int	_echo(int *newValue)
{
	static int	state = 0;

	if (newValue)
	{
		if (*newValue == 1)
		{
			state = 1;
		}
		else
		{
			state = 0;
		}
	}

	return state;
}

static void	printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	handleQuit(int signum)
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of irradmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\t+\tBecome a member of a region");
	PUTS("\t   + <region nbr>");
	PUTS("\t-\tLeave: cease being a member of the indicated region");
	PUTS("\t   - <region nbr>");
	PUTS("\tl\tList");
	PUTS("\t   l nodes");
	PUTS("\t   l passageways <destination node nbr>");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static void	executeJoin(int tokenCount, char **tokens)
{
	uvast		nodeNbr = getOwnNodeNbr();
	uint32_t	regionNbr;
	char		contactPlanFileName[64];
	FILE		*rcfile;
	char		cmd[1024];
	FILE		*specFile;
	int		cmdlen;

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	regionNbr = strtol(tokens[1], NULL, 10);
	if (regionNbr < 1)
	{
		SYNTAX_ERROR;
		return;
	}

	isprintf(contactPlanFileName, sizeof contactPlanFileName,
			"contacts.%lu.ionrc", regionNbr);
	rcfile = fopen("join.ionrc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("irradmin can't create join.ionrc file.", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "^ %lu\n", regionNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to join.ionrc", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "! 1\n", regionNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to join.ionrc", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "a contact -1 -1 " UVAST_FIELDSPEC " "
UVAST_FIELDSPEC " 0 1.0\n", nodeNbr, nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to join.ionrc", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "a contact +0 +100000000 " UVAST_FIELDSPEC
" " UVAST_FIELDSPEC " 10000000 1.0\n", nodeNbr, nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to join.ionrc", NULL);
		return;
	}

	specFile = fopen("connect.ionrc", "r");
	if (specFile)
	{
		while (1)
		{
			if (fgets(cmd, sizeof cmd, specFile) == NULL)
			{
				break;
			}

			cmdlen = strlen(cmd);
			if (cmdlen == 0 || *(cmd + (cmdlen - 1)) != '\n')
			{
				continue;	/*      Not a command.	*/
			}

			if (*cmd == 'a')	/*	Adding.		*/
			{
				if (fwrite(cmd, cmdlen, 1, rcfile) < 1)
				{
					fclose(specFile);
					fclose(rcfile);
					putSysErrmsg("irradmin can't write to \
node.ionrc", NULL);
					return;
				}
      			}
		}
	
		fclose(specFile);
        }

	fclose(rcfile);

	/*	Use ION utilities to effect the registration.		*/

	if (pseudoshell("bpadmin connect.bprc") < 0)
	{
		putSysErrmsg("irradmin can't note duct(s).", itoa(nodeNbr));
		return;
	}

	snooze(3);
	isprintf(cmd, sizeof(cmd), "ionadmin %s %lu", contactPlanFileName,
			regionNbr);
	if (pseudoshell(cmd) < 0)
	{
		putSysErrmsg("irradmin can't load contacts.", itoa(nodeNbr));
		return;
	}

	snooze(1);
	if (pseudoshell("ionadmin join.ionrc") < 0)
	{
		putSysErrmsg("irradmin can't register node.", itoa(nodeNbr));
	}
}

static void	executeLeave(int tokenCount, char **tokens)
{
	uvast		nodeNbr = getOwnNodeNbr();
	uint32_t	regionNbr;
	FILE		*rcfile;
	char		cmd[1024];

	if (tokenCount != 2)
	{
		SYNTAX_ERROR;
		return;
	}

	regionNbr = strtol(tokens[1], NULL, 10);
	if (regionNbr < 1)
	{
		SYNTAX_ERROR;
		return;
	}

	/*	Our registration contact deletion serves only to
	 *	announce the unregistration to all members of the
	 *	affected region including the local node.		*/

	rcfile = fopen("leaveA.ionrc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("irradmin can't create leaveA.ionrc file.", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "^ %lu\n", regionNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to leaveA.ionrc", NULL);
		return;
	}

	istrcpy(cmd, "! 1\n", sizeof(cmd));
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to leaveA.ionrc", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "d contact -1 " UVAST_FIELDSPEC " "
UVAST_FIELDSPEC "\n", nodeNbr, nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to leaveA.ionrc", NULL);
		return;
	}

	fclose(rcfile);
	if (pseudoshell("ionadmin leaveA.ionrc") < 0)
	{
		putSysErrmsg("No irradmin announce unregister", itoa(nodeNbr));
	}

	/*	Snooze 3 seconds to give announcement time to spread.	*/

	snooze(3);

	/*	We then do a second registration contact deletion
	 *	with "announce" set to zero; this is the operation
	 *	that actually removes the registratino contact.		*/

	rcfile = fopen("leaveB.ionrc", "w");
	if (rcfile == NULL)
	{
		putSysErrmsg("irradmin can't create leaveB.ionrc file.", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "^ %lu\n", regionNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to leaveB.ionrc", NULL);
		return;
	}

	istrcpy(cmd, "! 0\n", sizeof(cmd));
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to leaveB.ionrc", NULL);
		return;
	}

	isprintf(cmd, sizeof(cmd), "d contact -1 " UVAST_FIELDSPEC " "
UVAST_FIELDSPEC "\n", nodeNbr, nodeNbr);
	if (fwrite(cmd, strlen(cmd), 1, rcfile) < 1)
	{
		fclose(rcfile);
		putSysErrmsg("irradmin can't write to leaveB.ionrc", NULL);
		return;
	}

	fclose(rcfile);
	if (pseudoshell("ionadmin leaveB.ionrc") < 0)
	{
		putSysErrmsg("No irradmin post unregister", itoa(nodeNbr));
	}
}

static void	printNode(RegionMember *member)
{
	char	buffer[1024];

	if (member->outerRegionNbr == 0)
	{
		isprintf(buffer, sizeof buffer,
			"Node " UVAST_FIELDSPEC "\thome=%lu",
			member->nodeNbr, member->homeRegionNbr);
	}
	else
	{
		isprintf(buffer, sizeof buffer,
			"Node " UVAST_FIELDSPEC "\thome=%lu  outer=%lu",
			member->nodeNbr, member->homeRegionNbr,
			member->outerRegionNbr);
	}

	printText(buffer);
}

static void	listNodes()
{
	Sdr	sdr = getIonsdr();
	IonDB	iondb;
	Object	elt;
	Object	addr;
		OBJ_POINTER(RegionMember, member);

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.rolodex); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, RegionMember, member, addr);
		printNode(member);
	}

	sdr_exit_xn(sdr);
}

static void	listPassageways(uvast destinationNodeNbr)
{
	Sdr	sdr = getIonsdr();
	IonDB	iondb;
	Object	elt;
	Object	addr;
		OBJ_POINTER(RegionMember, member);

	CHKVOID(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	for (elt = sdr_list_first(sdr, iondb.rolodex); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, RegionMember, member, addr);
		if (member->homeRegionNbr == destinationNodeNbr
		|| member->outerRegionNbr == destinationNodeNbr)
		{
			printNode(member);
		}
	}

	sdr_exit_xn(sdr);
}

static void	executeList(int tokenCount, char **tokens)
{
	uvast	destinationNodeNbr;

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "nodes") == 0)
	{
		listNodes();
		return;
	}

	if (strcmp(tokens[1], "passageways") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		destinationNodeNbr = strtouvast(tokens[2]);
		listPassageways(destinationNodeNbr);
		return;
	}

	SYNTAX_ERROR;
}

static void	switchEcho(int tokenCount, char **tokens)
{
	int	state;

	if (tokenCount < 2)
	{
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		state = 0;
		oK(_echo(&state));
		break;

	case '1':
		state = 1;
		oK(_echo(&state));
		break;

	default:
		printText("Echo on or off?");
	}
}

static int	processLine(char *line, int lineLength)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];

	tokenCount = 0;
	for (cursor = line, i = 0; i < 9; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			tokenCount++;
		}
	}

	if (tokenCount == 0)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		printText("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;

		case '+':
			executeJoin(tokenCount, tokens);
			return 0;

		case '-':
			executeLeave(tokenCount, tokens);
			return 0;

		case 'l':
			executeList(tokenCount, tokens);
			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static int	run_irradmin(char *cmdFileName)
{
	int	cmdFile;
	char	line[256];
	int	len;

	if (bpAttach() < 0)
	{
		putErrmsg("irradmin can't attach to BP", NULL);
		return -1;
	}

	if (cmdFileName == NULL)	/*	Interactive.		*/
	{
#ifdef FSWLOGGER
		return 0;
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0)
				{
					break;
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Out of loop.	*/
			}

			if (len == 0)
			{
				continue;
			}

			if (processLine(line, len))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else					/*	Scripted.	*/
	{
		cmdFile = iopen(cmdFileName, O_RDONLY, 0777);
		if (cmdFile < 0)
		{
			PERROR("Can't open command file");
		}
		else
		{
			while (1)
			{
				if (igets(cmdFile, line, sizeof line, &len)
						== NULL)
				{
					if (len == 0)
					{
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping irradmin.");
	ionDetach();
	return 0;
}

#if defined (ION_LWT)
int	irradmin(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_irradmin(cmdFileName);
}
