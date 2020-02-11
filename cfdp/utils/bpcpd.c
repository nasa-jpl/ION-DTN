/*	bpcpd.c:	bpcpd, a remote copy daemon that utilizes CFDP
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *	All rights reserved.
 *	Author: Samuel Jero <sj323707@ohio.edu>, Ohio University
 */
#include "bpcp.h"



int debug = 0;	/*Set to non-zero to enable debug output. */
int running=1;



void poll_cfdp_messages();
void dbgprintf(int level, const char *fmt, ...);
void usage(void);
void version();
#ifdef CLEAN_ON_EXIT
void sig_handler(int signum);
#endif

/*Start Here*/
#if defined (ION_LWT)
int	bpcpd(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
/*a1 is the debug flag*/
debug=atoi((char*)a1);

/*a2 is the version option*/
if(atoi((char*)a2)==1)
{
	version();
	return 0;
}

#else
int main(int argc, char **argv)
{
	int ch;

	/*Parse commandline options*/
	while ((ch = getopt(argc, argv, "dv")) != -1)
	{
		switch (ch)
		{
			case 'd':
				/*Debug*/
				debug++;
				break;
			case 'v':
				version();
				break;
			default:
				usage();
		}
	}
#endif

	/*Initialize CFDP*/
	if (cfdp_attach() < 0)
	{
		dbgprintf(0, "Error: Can't initialize CFDP. Is ION running?\n");
		exit(1);
	}
	running=1;

#ifdef SIG_HANDLER
	/*Set SIGTERM and SIGINT handlers*/
	isignal(SIGTERM, sig_handler);
	isignal(SIGINT, sig_handler);
#endif

	poll_cfdp_messages();

#ifdef CLEAN_ON_EXIT
#if defined (unix)
	/*Cleanup all directory listing files*/
	if (system("rm dirlist_* >/dev/null 2>/dev/null")<0)
	{
		dbgprintf(0, "Error running cleanup\n");
	}
#endif
#endif
	exit(0);
}

/*CFDP Event Polling loop*/
void poll_cfdp_messages()
{
	char *eventTypes[] =	{
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
	uvast			fileSize;
	MetadataList		messagesToUser;
	uvast			offset;
	unsigned int		length;
	unsigned int 		recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		segMetadataLength;
	char			segMetadata[63];
	CfdpCondition		condition;
	uvast			progress;
	CfdpFileStatus		fileStatus;
	CfdpDeliveryCode	deliveryCode;
	CfdpTransactionId	originatingTransactionId;
	char			statusReportBuf[256];
	unsigned char		usrmsgBuf[256];
	MetadataList		filestoreResponses;
	uvast 		TID11;
	uvast		TID12;

	/*Main Event loop*/
	while (running) {

		/*Grab a CFDP event*/
		if (cfdp_get_event(&type, &time, &reqNbr, &transactionId,
				sourceFileNameBuf, destFileNameBuf,
				&fileSize, &messagesToUser, &offset, &length,
				&recordBoundsRespected, &continuationState,
				&segMetadataLength, segMetadata,
				&condition, &progress, &fileStatus,
				&deliveryCode, &originatingTransactionId,
				statusReportBuf, &filestoreResponses) < 0)
		{
			dbgprintf(0, "Error: Failed getting CFDP event.", NULL);
			exit(1);
		}

		if (type == CfdpNoEvent)
		{
			continue;	/*	Interrupted.		*/
		}

		/*Decompress transaction ID*/
		cfdp_decompress_number(&TID11,&transactionId.sourceEntityNbr);
		cfdp_decompress_number(&TID12,&transactionId.transactionNbr);

		/*Print Event type if debugging*/
		dbgprintf(1,"\nEvent: type %d, '%s', From Node: %ull, Transaction ID: %ull.%ull.\n", type,
				(type > 0 && type < 12) ? eventTypes[type]
				: "(unknown)",TID11, TID11, TID12);

		/*Parse Messages to User to get directory information*/
		while (messagesToUser)
		{
			/*Get user message*/
			memset(usrmsgBuf, 0, 256);
			if (cfdp_get_usrmsg(&messagesToUser, usrmsgBuf,
					(int *) &length) < 0)
			{
				putErrmsg("Failed getting user msg.", NULL);
				continue;
			}

			/*Set Null character at end of string*/
			if (length > 0)
			{
				usrmsgBuf[length] = '\0';
				dbgprintf(2,"\tUser Message '%s'\n", usrmsgBuf);
			}
		}

	}
	return;
}

/*Debug Printf*/
void dbgprintf(int level, const char *fmt, ...)
{
    va_list args;
    if(debug>=level)
    {
    	va_start(args, fmt);
    	vfprintf(stderr, fmt, args);
    	va_end(args);
    }
}

/*Print Command usage to stderr and exit*/
void usage(void)
{
	(void) fprintf(stderr, "usage: bpcpd [-d | -v]\n");
	exit(1);
}

/*Print Version Information*/
void version()
{
	dbgprintf(0, BPCP_VERSION_STRING);
	exit(1);
}

void sig_handler(int signum)
{
	/*Reset signal handlers for portability*/
	isignal(SIGTERM, sig_handler);
	isignal(SIGINT, sig_handler);

	/*Shutdown event polling loop*/
	running=0;

	/*Interrupt cfdp_get_event()*/
	cfdp_interrupt();
}
