/*

	cfdptest.c:	CFDP test shell program.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdp.h"

static CfdpHandler		faultHandlers[16];
static CfdpNumber		destinationEntityNbr;
static char			sourceFileNameBuf[256] = "";
static char			*sourceFileName = NULL;
static char			destFileNameBuf[256] = "";
static char			*destFileName = NULL;
static BpUtParms		utParms = {	0,
						86400,
						BP_STD_PRIORITY,
						SourceCustodyRequired,
						0,
						0,
						{ 0, 0, 0 }	};
static MetadataList		msgsToUser = 0;
static MetadataList		fsRequests = 0;
static CfdpTransactionId	transactionId;

static void	handleQuit()
{
	puts("Please enter command 'q' to stop the program.");
}

static void	printUsage()
{
	puts("Valid commands are:");
	puts("\tq\tQuit");
	puts("\th\tHelp");
	puts("\t?\tHelp");
	puts("\td\tSet destination entity number");
	puts("\t   d <destination entity number>");
	puts("\tf\tSet source file name");
	puts("\t   f <source file name>");
	puts("\tt\tSet destination file name");
	puts("\t   t <destination file name>");
	puts("\tl\tSet time-to-live (lifetime, in seconds)");
	puts("\t   l <time-to-live>");
	puts("\tp\tSet priority");
	puts("\t   p <priority: 0, 1, or 2>");
	puts("\to\tSet ordinal (sub-priority within priority 2)");
	puts("\t   o <ordinal>");
	puts("\tm\tSet transmission mode");
	puts("\t   m <0 = reliable (the default), 1 = unreliable>");
	puts("\tr\tAdd filestore request");
	puts("\t   r <action code nbr> <first path name> <second path name>");
	puts("\t\t\tAction code numbers are:");
	puts("\t\t\t\t0 = create file");
	puts("\t\t\t\t1 = delete file");
	puts("\t\t\t\t2 = rename file");
	puts("\t\t\t\t3 = append file");
	puts("\t\t\t\t4 = replace file");
	puts("\t\t\t\t5 = create directory");
	puts("\t\t\t\t6 = remove directory");
	puts("\t\t\t\t7 = deny file");
	puts("\t\t\t\t8 = deny directory");
	puts("\tu\tAdd message to user");
	puts("\t   u '<message text>'");
	puts("\t&\tSend file per specified parameters");
	puts("\t   &");
	puts("\t^\tCancel the current file transmission");
	puts("\t   ^");
	puts("\t%\tSuspend the current file transmission");
	puts("\t   %");
	puts("\t$\tResume the current file transmission");
	puts("\t   $");
	puts("\t#\tReport on the current file transmission");
	puts("\t   #");
}

static void	setDestinationEntityNbr(int tokenCount, char **tokens)
{
	unsigned long	entityId;

	if (tokenCount != 2)
	{
		puts("What's the destination entity number?");
		return;
	}

	entityId = strtol(tokens[1], NULL, 0);
	cfdp_compress_number(&destinationEntityNbr, entityId);
}

static void	setSourceFileName(int tokenCount, char **tokens)
{
	if (tokenCount != 2)
	{
		puts("What's the source file name?");
		return;
	}

	isprintf(sourceFileNameBuf, sizeof sourceFileNameBuf, "%.255s",
			tokens[1]);
	sourceFileName = sourceFileNameBuf;
}

static void	setDestFileName(int tokenCount, char **tokens)
{
	if (tokenCount != 2)
	{
		puts("What's the destination file name?");
		return;
	}

	isprintf(destFileNameBuf, sizeof destFileNameBuf, "%.255s", tokens[1]);
	destFileName = destFileNameBuf;
}

static void	setClassOfService(int tokenCount, char **tokens)
{
	unsigned long	priority;

	if (tokenCount != 2)
	{
		puts("What's the priority?");
		return;
	}

	priority = strtol(tokens[1], NULL, 0);
	utParms.classOfService = priority;
}

static void	setOrdinal(int tokenCount, char **tokens)
{
	unsigned long	ordinal;

	if (tokenCount != 2)
	{
		puts("What's the ordinal?");
		return;
	}

	ordinal = strtol(tokens[1], NULL, 0);
	utParms.extendedCOS.ordinal = ordinal;
}

static void	setMode(int tokenCount, char **tokens)
{
	unsigned long	mode;

	if (tokenCount != 2)
	{
		puts("What's the mode?");
		return;
	}

	mode = (strtol(tokens[1], NULL, 0) == 0 ? 0 : 1);
	if (mode == 1)
	{
		utParms.extendedCOS.flags |= BP_BEST_EFFORT;
		utParms.custodySwitch = NoCustodyRequested;
	}
	else
	{
		utParms.extendedCOS.flags &= (~BP_BEST_EFFORT);
		utParms.custodySwitch = SourceCustodyRequired;
	}
}

static void	setCriticality(int tokenCount, char **tokens)
{
	unsigned long	criticality;

	if (tokenCount != 2)
	{
		puts("What's the criticality?");
		return;
	}

	criticality = (strtol(tokens[1], NULL, 0) == 0 ? 0 : 1);
	if (criticality == 1)
	{
		utParms.extendedCOS.flags |= BP_MINIMUM_LATENCY;
	}
	else
	{
		utParms.extendedCOS.flags &= (~BP_MINIMUM_LATENCY);
	}
}

static void	setTTL(int tokenCount, char **tokens)
{
	unsigned long	TTL;

	if (tokenCount != 2)
	{
		puts("What's the TTL?");
		return;
	}

	TTL = strtol(tokens[1], NULL, 0);
	utParms.lifespan = TTL;
}

static void	addMsgToUser(int tokenCount, char **tokens)
{
	if (tokenCount != 2)
	{
		puts("What's the message text?");
		return;
	}

	if (msgsToUser == 0)
	{
		msgsToUser = cfdp_create_usrmsg_list();
	}

	cfdp_add_usrmsg(msgsToUser, (unsigned char *) tokens[1],
			strlen(tokens[1]) + 1);
}

static void	addFilestoreRequest(int tokenCount, char **tokens)
{
	CfdpAction	action;
	char		*firstPathName = NULL;
	char		*secondPathName = NULL;

	switch (tokenCount)
	{
		case 4:
			secondPathName = tokens[3];

			/*	Intentional fall-through to next case.	*/

		case 3:
			firstPathName = tokens[2];
			action = atoi(tokens[1]);
			break;

		default:
			puts("Syntax: <action code> <1st name> [<2nd name>]");
			return;
	}

	if (fsRequests == 0)
	{
		fsRequests = cfdp_create_fsreq_list();
	}

	cfdp_add_fsreq(fsRequests, action, firstPathName, secondPathName);
}

static int	processLine(char *line)
{
	int	lineLength;
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];


	lineLength = strlen(line) - 1;
	if (line[lineLength] == 0x0a)		/*	LF (newline)	*/
	{
		line[lineLength] = '\0';	/*	lose it		*/
		lineLength--;
	}

	if (line[lineLength] == 0x0d)		/*	CR (DOS text)	*/
	{
		line[lineLength] = '\0';	/*	lose it		*/
		lineLength--;
	}

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
		puts("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;

		case 'd':
			setDestinationEntityNbr(tokenCount, tokens);
			return 0;

		case 'f':
			setSourceFileName(tokenCount, tokens);
			return 0;

		case 't':
			setDestFileName(tokenCount, tokens);
			return 0;

		case 'l':
			setTTL(tokenCount, tokens);
			return 0;

		case 'p':
			setClassOfService(tokenCount, tokens);
			return 0;

		case 'o':
			setOrdinal(tokenCount, tokens);
			return 0;

		case 'm':
			setMode(tokenCount, tokens);
			return 0;

		case 'c':
			setCriticality(tokenCount, tokens);
			return 0;

		case 'u':
			addMsgToUser(tokenCount, tokens);
			return 0;

		case 'r':
			addFilestoreRequest(tokenCount, tokens);
			return 0;

		case '&':
			if (cfdp_put(&destinationEntityNbr, sizeof(utParms),
					(unsigned char *) &utParms,
					sourceFileName, destFileName, NULL,
					faultHandlers, 0, NULL, msgsToUser,
					fsRequests, &transactionId) < 0)
			{
				putErrmsg("Can't put FDU.", NULL);
				return -1;
			}

			msgsToUser = 0;
			fsRequests = 0;
			return 0;

		case '^':
			if (cfdp_cancel(&transactionId) < 0)
			{
				putErrmsg("Can't cancel transaction.", NULL);
				return -1;
			}

			return 0;

		case '%':
			if (cfdp_suspend(&transactionId) < 0)
			{
				putErrmsg("Can't suspend transaction.", NULL);
				return -1;
			}

			return 0;

		case '$':
			if (cfdp_resume(&transactionId) < 0)
			{
				putErrmsg("Can't resume transaction.", NULL);
				return -1;
			}

			return 0;

		case '#':
			if (cfdp_report(&transactionId) < 0)
			{
				putErrmsg("Can't report transaction.", NULL);
				return -1;
			}

			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			puts("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static void	*handleEvents(void *parm)
{
	int			*running = (int *) parm;
	char			*eventTypes[] =	{
					"no event",
					"transaction started",
					"EOF sent",
					"transaction finished",
					"metadata received",
					"file data segment received",
					"EOF received",
					"suspended",
					"resumed",
					"transaction report",
					"fault",
					"abandoned"
						};
	CfdpEventType		type;
	time_t			time;
	int			reqNbr;
	CfdpTransactionId	transactionId;
	char			sourceFileNameBuf[256];
	char			destFileNameBuf[256];
	unsigned int		fileSize;
	MetadataList		messagesToUser;
	unsigned int		offset;
	unsigned int		length;
	CfdpCondition		condition;
	unsigned int		progress;
	CfdpFileStatus		fileStatus;
	CfdpDeliveryCode	deliveryCode;
	CfdpTransactionId	originatingTransactionId;
	char			statusReportBuf[256];
	MetadataList		filestoreResponses;
	unsigned char		usrmsgBuf[256];
	CfdpAction		action;
	int			status;
	char			firstPathName[256];
	char			secondPathName[256];
	char			msgBuf[256];

	while (*running)
	{
		if (cfdp_get_event(&type, &time, &reqNbr, &transactionId,
				sourceFileNameBuf, destFileNameBuf,
				&fileSize, &messagesToUser, &offset, &length,
				&condition, &progress, &fileStatus,
				&deliveryCode, &originatingTransactionId,
				statusReportBuf, &filestoreResponses) < 0)
		{
			putErrmsg("Failed getting CFDP event.", NULL);
			return NULL;
		}

		if (type == CfdpNoEvent)
		{
			continue;	/*	Interrupted.		*/
		}

		printf("\nEvent: type %d '%s'.\n", type,
				(type > 0 && type < 12) ? eventTypes[type]
				: "(unknown)");
		if (type == CfdpReportInd)
		{
			printf("Report '%s'\n", statusReportBuf);
		}

		while (messagesToUser)
		{
			switch (cfdp_get_usrmsg(&messagesToUser, usrmsgBuf,
					(int *) &length))
			{
			case -1:
				putErrmsg("Failed getting user msg.", NULL);
				return NULL;

			case 0:
				break;

			default:
				usrmsgBuf[length] = '\0';
				printf("\tMessage '%s'\n", usrmsgBuf);
			}
		}

		while (filestoreResponses)
		{
			switch (cfdp_get_fsresp(&filestoreResponses, &action,
					&status, firstPathName, secondPathName,
					msgBuf))
			{
			case -1:
				putErrmsg("Failed getting FS response.", NULL);
				return NULL;

			case 0:
				break;

			default:
				printf("\tResponse %d %d '%s' '%s' '%s'\n",
						action, status, firstPathName,
						secondPathName, msgBuf);
			}
		}

		printf(": ");
		fflush(stdout);
	}

	return NULL;
}

static int	runCfdptestInteractive()
{
	char		line[256];
	pthread_t	receiverThread;
	int		running = 1;

	isignal(SIGINT, handleQuit);

	/*	Start the receiver thread.				*/

	if (pthread_create(&receiverThread, NULL, handleEvents, &running))
	{
		putSysErrmsg("cfdptest can't create receiver thread", NULL);
		return 1;
	}

	memset((char *) faultHandlers, 0, sizeof(faultHandlers));
	cfdp_compress_number(&destinationEntityNbr, 0);
	while (1)
	{
		printf(": ");
		if (fgets(line, sizeof line, stdin) == NULL)
		{
			if (feof(stdin))
			{
				break;
			}

			perror("cfdptest fgets failed");
			break;			/*	Out of loop.	*/
		}

		if (processLine(line))
		{
			break;			/*	Out of loop.	*/
		}
	}

	/*	Stop the receiver thread by interrupting reception.	*/

	running = 0;
	cfdp_interrupt();
	pthread_join(receiverThread, NULL);
	puts("Stopping cfdptest.");
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	cfdptest(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	unsigned long	destNode = a1 ? strtol((char *) a1, NULL, 0) : 0;
	char		*sourcePath = (char *) a2;
	char		*destPath = (char *) a3;
	unsigned int	ttl = a4 ? atoi((char *) a4) : 0;
	unsigned int	priority = a5 ? atoi((char *) a5) : 0;
	unsigned int	ordinal = a6 ? atoi((char *) a6) : 0;
	unsigned int	unreliable = a7 ? atoi((char *) a7) : 0;
	unsigned int	critical = a8 ? atoi((char *) a8) : 0;
	int		interactive = 0;
#else
int	main(int argc, char **argv)
{
	unsigned long	destNode = argc > 1 ? strtol(argv[1], NULL, 0) : 0;
	char		*sourcePath = argc > 2 ? argv[2] : NULL;
	char		*destPath = argc > 3 ? argv[3] : NULL;
	unsigned int	ttl = argc > 4 ? atoi(argv[4]) : 0;
	unsigned int	priority = argc > 5 ? atoi(argv[5]) : 0;
	unsigned int	ordinal = argc > 6 ? atoi(argv[6]) : 0;
	unsigned int	unreliable = argc > 7 ? atoi(argv[7]) : 0;
	unsigned int	critical = argc > 8 ? atoi(argv[8]) : 0;
	int		interactive = (argc == 1);
#endif
	int		retval = 0;

	if (cfdp_init() < 0 || ionAttach() < 0)
	{
		putErrmsg("cfdptest can't initialize CFDP.", NULL);
		return 1;
	}

	if (interactive)
	{
		retval = runCfdptestInteractive();
		ionDetach();
		return retval;
	}

	if (destNode == 0 || sourcePath == NULL || destPath == NULL || ttl == 0)
	{
		ionDetach();
		puts("Usage: cfdptest [<destination entity nbr> <source file \
name> <destination file name> [<time-to-live, in seconds> [<priority: 0, 1, 2> \
[<ordinal: 0-254> [<unreliable: 0 or 1> [<critical: 0 or 1>]]]]]]");
		return 0;
	}

	cfdp_compress_number(&destinationEntityNbr, destNode);
	isprintf(sourceFileNameBuf, sizeof sourceFileNameBuf, "%.255s",
			sourcePath);
	sourceFileName = sourceFileNameBuf;
	isprintf(destFileNameBuf, sizeof destFileNameBuf, "%.255s", destPath);
	destFileName = destFileNameBuf;
	utParms.classOfService = priority;
	utParms.extendedCOS.ordinal = ordinal;
	utParms.extendedCOS.flags = 0;
	if (unreliable)
	{
		utParms.extendedCOS.flags |= BP_BEST_EFFORT;
		utParms.custodySwitch = NoCustodyRequested;
	}
	else
	{
		utParms.custodySwitch = SourceCustodyRequired;
	}

	if (critical)
	{
		utParms.extendedCOS.flags |= BP_MINIMUM_LATENCY;
	}

	utParms.lifespan = ttl;
	if (cfdp_put(&destinationEntityNbr, sizeof utParms,
			(unsigned char *) &utParms, sourceFileName,
			destFileName, NULL, faultHandlers, 0, NULL,
			msgsToUser, fsRequests, &transactionId) < 0)
	{
		putErrmsg("Can't put FDU.", NULL);
		retval = 1;
	}

	ionDetach();
	return retval;
}
