/*
	cfdptest.c:	CFDP test shell program.
	
	ENHANCED VERSION: Works with modified CFDP API that exposes 
	closure request information immediately upon metadata reception.
	
	API Enhancement: cfdp_get_event() now includes closureRequested parameter
									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdp.h"
#include "bputa.h"

typedef struct
{
	CfdpHandler		faultHandlers[16];
	CfdpNumber		destinationEntityNbr;
	char			sourceFileNameBuf[256];
	char			*sourceFileName;
	char			destFileNameBuf[256];
	char			*destFileName;
	BpUtParms		utParms;
	unsigned int		closureLatency;
	CfdpMetadataFn		segMetadataFn;
	MetadataList		msgsToUser;
	MetadataList		fsRequests;
	CfdpTransactionId	transactionId;
} CfdpReqParms;

/* plain text of event types, except no access event (-1) */
static char *eventTypes[] = {
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

/* Transaction tracking for closure request detection */
typedef struct {
    CfdpTransactionId transactionId;
    int closureRequested;
    int isReceiver;
    time_t startTime;
} TransactionTracker;

#define MAX_TRACKED_TRANSACTIONS 10
static TransactionTracker trackedTransactions[MAX_TRACKED_TRANSACTIONS];
static int numTrackedTransactions = 0;

// Forward declarations
static void addTransactionTracker(CfdpTransactionId *transId, int isReceiver, int closureRequested);
static TransactionTracker* findTransactionTracker(CfdpTransactionId *transId);
static void inferClosureRequest(CfdpEventType type, CfdpTransactionId *transId, 
                               CfdpCondition condition, CfdpDeliveryCode deliveryCode);
static char* getClosureRequestStatus(CfdpTransactionId *transId);

static void reportCfdpEvent(CfdpEventType type, char *statusReportBuf, 
                           CfdpCondition condition, CfdpDeliveryCode deliveryCode, 
                           CfdpFileStatus fileStatus, uvast progress,
                           CfdpTransactionId *transactionId,
                           unsigned int closureRequested)
{
    if (type < 0)
    {	
        printf("\nCFDP Event: CFDP access had ended.\n");
        return;
    }

    /* With your enhanced API, we now get IMMEDIATE closure detection! */
    if (type == CfdpMetadataRecvInd) { 
        if (closureRequested) {
            printf("  ** CLOSURE REQUEST: YES (Acknowledged Mode - Class 2)\n");
            printf("      → This receiver WILL send Finish PDU back to sender\n");
            addTransactionTracker(transactionId, 1, 1); /* receiver, closure definitely requested */
        } else {
            printf("  ** CLOSURE REQUEST: NO (Unacknowledged Mode - Class 1)\n");
            printf("      → This receiver will NOT send Finish PDU back\n");
            printf("      → Transaction will complete immediately after file processing\n");
            addTransactionTracker(transactionId, 1, 0); /* receiver, no closure request */
        }
        printf("  \n");
    } else {
        /* For non-metadata events, still use inference for sender side */
        inferClosureRequest(type, transactionId, condition, deliveryCode);
    }

    printf("\nCFDP Event: Type = %d: %s\n", type, eventTypes[type]);
    printf("  Transaction Status: %s\n", getClosureRequestStatus(transactionId));
    
    /* Enhanced condition reporting */
    printf("  Condition: %d ", condition);
    switch(condition) {
        case CfdpNoError: 
            printf("(No Error)");
            if (type == CfdpTransactionFinishedInd) {
                printf(" -> TRANSACTION COMPLETED SUCCESSFULLY!");
                
                /* Enhanced completion analysis with definitive closure data */
                TransactionTracker *tracker = findTransactionTracker(transactionId);
                if (tracker && tracker->isReceiver) {
                    printf("\n  📊 RECEIVER COMPLETION ANALYSIS:");
                    if (tracker->closureRequested == 1) {
                        printf("\n      ✅ CONFIRMED: Acknowledged mode completion");
                        printf("\n      📤 Finish PDU was sent back to sender");
                        printf("\n      🎯 Full end-to-end acknowledgment achieved");
                        printf("\n      ⏱️  Sender received confirmation and completed transaction");
                    } else if (tracker->closureRequested == 0) {
                        printf("\n      ⚡ CONFIRMED: Unacknowledged mode completion");
                        printf("\n      🚀 No Finish PDU required or sent");
                        printf("\n      📝 Sender completed when EOF was sent");
                        printf("\n      ⚡ Fast, efficient fire-and-forget transfer");
                    }
                } else if (tracker && !tracker->isReceiver) {
                    printf("\n  📊 SENDER COMPLETION ANALYSIS:");
                    if (tracker->closureRequested == 1) {
                        printf("\n      ✅ CONFIRMED: Acknowledged mode - received Finish PDU");
                        printf("\n      🎯 Full end-to-end confirmation received from receiver");
                    } else if (tracker->closureRequested == 0) {
                        printf("\n      ⚡ CONFIRMED: Unacknowledged mode - EOF sent successfully");
                        printf("\n      🚀 Fire-and-forget transmission complete");
                    }
                }
            }
            break;
            
        case CfdpCheckLimitReached:
            printf("(Check Limit Reached)");
            if (type == CfdpTransactionFinishedInd) {
                printf(" -> FINISH PDU TIMEOUT!");
                printf("\n      ⚠️  This confirms closure was requested but Finish PDU not received");
                printf("\n      🔄 Receiver may have sent Finish PDU but it was lost");
                printf("\n      📡 Network reliability issue or receiver problem");
            }
            break;
        case CfdpAckLimitReached:
            printf("(ACK Limit Reached)"); break;
        case CfdpKeepaliveLimitReached:
            printf("(Keepalive Limit Reached)"); break;
        case CfdpInvalidTransmissionMode:
            printf("(Invalid Transmission Mode)"); break;
        case CfdpFilestoreRejection:
            printf("(Filestore Rejection)"); break;
        case CfdpChecksumFailure:
            printf("(Checksum Failure)"); break;
        case CfdpFileSizeError:
            printf("(File Size Error)"); break;
        case CfdpNakLimitReached:
            printf("(NAK Limit Reached)"); break;
        case CfdpInactivityDetected:
            printf("(Inactivity Detected)"); break;
        case CfdpInvalidFileStructure:
            printf("(Invalid File Structure)"); break;
        case CfdpUnsupportedChecksumType:
            printf("(Unsupported Checksum Type)"); break;
        case CfdpSuspendRequested:
            printf("(Suspend Requested)"); break;
        case CfdpCancelRequested:
            printf("(Cancel Requested)"); break;
        default:
            printf("(Unknown: %d)", condition); break;
    }
    printf("\n");
    
    /* Enhanced EOF handling with definitive closure information */
    if (type == CfdpEofRecvInd) {
        TransactionTracker *tracker = findTransactionTracker(transactionId);
        printf("  📥 EOF RECEIVED - FILE RECEPTION COMPLETED\n");
        if (tracker && tracker->closureRequested == 1) {
            printf("  🎯 ACKNOWLEDGED MODE: Will send Finish PDU back to sender\n");
            printf("  📤 Sender is waiting for our acknowledgment\n");
            printf("  ⏱️  Transaction will complete after Finish PDU is sent and processed\n");
        } else if (tracker && tracker->closureRequested == 0) {
            printf("  ⚡ UNACKNOWLEDGED MODE: No Finish PDU required\n");
            printf("  🚀 Transaction will complete immediately after file processing\n");
            printf("  📝 Sender has already completed (fire-and-forget mode)\n");
        } else {
            printf("  ❓ EOF received but no metadata tracker found\n");
        }
    }
    
    /* Enhanced transaction indication */
    if (type == CfdpTransactionInd) {
        printf("  🚀 TRANSACTION INITIATED\n");
        printf("  📊 Role: SENDER - starting file transmission\n");
        printf("  🔍 Tip: Check closureLatency parameter used in cfdp_put() call\n");
        printf("  ⏳ Receiver will get immediate closure request info in metadata PDU\n");
    }
    
    /* Standard delivery and file status reporting */
    printf("  Delivery: %d ", deliveryCode);
    switch(deliveryCode) {
        case CfdpDataComplete:
            printf("(Data Complete)"); break;
        case CfdpDataIncomplete:
            printf("(Data Incomplete)"); break;
        default:
            printf("(Unknown)"); break;
    }
    printf("\n");
    
    printf("  File Status: %d ", fileStatus);
    switch(fileStatus) {
        case CfdpFileDiscarded:
            printf("(File Discarded)"); break;
        case CfdpFileRejected:
            printf("(File Rejected)"); break;
        case CfdpFileRetained:
            printf("(File Retained)"); break;
        case CfdpFileStatusUnreported:
            printf("(Status Unreported)"); break;
        default:
            printf("(Unknown)"); break;
    }
    printf("\n");
    
    printf("  Progress: %lu bytes\n", (unsigned long)progress);
    
    if (statusReportBuf && strlen(statusReportBuf) > 0) {
        printf("  Status Report: %s\n", statusReportBuf);
    }
}

// Implementation of transaction tracking functions
static void addTransactionTracker(CfdpTransactionId *transId, int isReceiver, int closureRequested)
{
    if (numTrackedTransactions >= MAX_TRACKED_TRANSACTIONS) {
        /* Remove oldest transaction */
        memmove(&trackedTransactions[0], &trackedTransactions[1], 
                (MAX_TRACKED_TRANSACTIONS-1) * sizeof(TransactionTracker));
        numTrackedTransactions--;
    }
    
    memcpy(&trackedTransactions[numTrackedTransactions].transactionId, transId, sizeof(CfdpTransactionId));
    trackedTransactions[numTrackedTransactions].isReceiver = isReceiver;
    trackedTransactions[numTrackedTransactions].closureRequested = closureRequested;
    trackedTransactions[numTrackedTransactions].startTime = time(NULL);
    numTrackedTransactions++;
}

static TransactionTracker* findTransactionTracker(CfdpTransactionId *transId)
{
    int i;
    for (i = 0; i < numTrackedTransactions; i++) {
        if (memcmp(&trackedTransactions[i].transactionId.sourceEntityNbr,
                   &transId->sourceEntityNbr, sizeof(CfdpNumber)) == 0 &&
            memcmp(&trackedTransactions[i].transactionId.transactionNbr,
                   &transId->transactionNbr, sizeof(CfdpNumber)) == 0) {
            return &trackedTransactions[i];
        }
    }
    return NULL;
}

static void inferClosureRequest(CfdpEventType type, CfdpTransactionId *transId, 
                               CfdpCondition condition, CfdpDeliveryCode deliveryCode)
{  
    switch (type) {
        case CfdpTransactionInd:
            /* Sender-initiated transaction - we'll determine closure when we
             * check what closureLatency we sent with cfdp_put() */
            addTransactionTracker(transId, 0, -1); /* sender, unknown closure */
            break;
            
        case CfdpMetadataRecvInd:
            /* Receiver getting metadata - closure request now handled directly
             * in reportCfdpEvent() with definitive data from API */
            break;
            
        default:
            break;
    }
}

static char* getClosureRequestStatus(CfdpTransactionId *transId)
{
    TransactionTracker *tracker = findTransactionTracker(transId);
    if (!tracker) {
        return "Unknown Transaction";
    }
    
    if (tracker->isReceiver) {
        switch (tracker->closureRequested) {
            case 1: return "✅ Receiver: Closure Requested (Acknowledged Mode) - DEFINITIVE";
            case 0: return "⚡ Receiver: No Closure Request (Unacknowledged Mode) - DEFINITIVE";
            case -1: return "❓ Receiver: Closure Request Status: Determining...";
            default: return "❌ Receiver: Invalid Status";
        }
    } else {
        switch (tracker->closureRequested) {
            case 1: return "✅ Sender: Closure Requested (Acknowledged Mode) - Will expect Finish PDU";
            case 0: return "⚡ Sender: No Closure Request (Unacknowledged Mode) - Fire-and-forget";
            case -1: return "❓ Sender: Closure Request Status: Unknown (check cfdp_put parameters)";
            default: return "❌ Sender: Invalid Status";
        }
    }
}

static int	noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
			unsigned int length, int sourceFileFd, char *buffer)
{
	writeTimestampLocal(getCtime(), buffer);
	return strlen(buffer) + 1;
}

static void	handleQuit(int signum)
{
	PUTS("cfdptest interrupted.");
}

static void	printUsage()
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tz\tPause before processing next command. (For test scripts.)");
	PUTS("\t   z <number of seconds to pause>");
	PUTS("\td\tSet destination entity number");
	PUTS("\t   d <destination entity number>");
	PUTS("\tf\tSet source file name");
	PUTS("\t   f <source file name>");
	PUTS("\tt\tSet destination file name");
	PUTS("\t   t <destination file name>");
	PUTS("\tl\tSet time-to-live (lifetime, in seconds)");
	PUTS("\t   l <time-to-live>");
	PUTS("\tp\tSet priority");
	PUTS("\t   p <priority: 0, 1, or 2>");
	PUTS("\to\tSet ordinal (sub-priority within priority 2)");
	PUTS("\t   o <ordinal>");
	PUTS("\tm\tSet transmission mode");
	PUTS("\t   m <0 = CL reliability (the default), 1 = unreliable, 2 = \
custody transfer>");
	PUTS("\ti\tSet custodial retransmission timeout interval");
	PUTS("\t   i <timeout interval, in seconds>");
	PUTS("\ta\tControl CFDP transaction closure");
	PUTS("\t   a <expected round trip latency>");
	PUTS("\t\t0 = no acknowledgment expected");
	PUTS("\tn\tControl insertion of segment metadata");
	PUTS("\t   n { 1 | 0 }");
	PUTS("\t\t1 = time-tag each segment, 0 = no segment metadata");
	PUTS("\tg\tSet status reporting flags");
	PUTS("\t   g <status report flag string>");
	PUTS("\t   Status report flag string is a sequence of status report");
	PUTS("\t   flags, separated by commas, with no embedded whitespace.");
	PUTS("\t   Each flag must be one of: rcv, ct, fwd, dlv, del.");
	PUTS("\tr\tAdd filestore request");
	PUTS("\t   r <action code nbr> <first path name> <second path name>");
	PUTS("\t\t\tAction code numbers are:");
	PUTS("\t\t\t\t0 = create file");
	PUTS("\t\t\t\t1 = delete file");
	PUTS("\t\t\t\t2 = rename file");
	PUTS("\t\t\t\t3 = append file");
	PUTS("\t\t\t\t4 = replace file");
	PUTS("\t\t\t\t5 = create directory");
	PUTS("\t\t\t\t6 = remove directory");
	PUTS("\t\t\t\t7 = deny file");
	PUTS("\t\t\t\t8 = deny directory");
	PUTS("\tu\tAdd message to user");
	PUTS("\t   u '<message text>'");
	PUTS("\t&\tSend file per specified parameters");
	PUTS("\t   &");
	PUTS("\t^\tCancel the current file transmission");
	PUTS("\t   ^");
	PUTS("\t%\tSuspend the current file transmission");
	PUTS("\t   %");
	PUTS("\t$\tResume the current file transmission");
	PUTS("\t   $");
	PUTS("\t#\tReport on the current file transmission");
	PUTS("\t   #");
	PUTS("\t|\tGet file: send request for file per specified parameters");
	PUTS("\t   |");
}

static int      _echo(int *newValue)
{
        static int      state = 0;

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

static void     printText(char *text)
{
        if (_echo(NULL))
        {
                writeMemo(text);
        }

        PUTS(text);
}

static void	setDestinationEntityNbr(int tokenCount, char **tokens,
			CfdpNumber *destinationEntityNbr)
{
	uvast	entityId;

	if (tokenCount != 2)
	{
		PUTS("What's the destination entity number?");
		return;
	}

	entityId = strtol(tokens[1], NULL, 0);
	cfdp_compress_number(destinationEntityNbr, entityId);
}

static void	setSourceFileName(int tokenCount, char **tokens,
			char *sourceFileNameBuf, char **sourceFileName)
{
	if (tokenCount != 2)
	{
		PUTS("What's the source file name?");
		return;
	}

	isprintf(sourceFileNameBuf, 256, "%.255s", tokens[1]);
	*sourceFileName = sourceFileNameBuf;
}

static void	setDestFileName(int tokenCount, char **tokens,
			char *destFileNameBuf, char **destFileName)
{
	if (tokenCount != 2)
	{
		PUTS("What's the destination file name?");
		return;
	}

	isprintf(destFileNameBuf, 256, "%.255s", tokens[1]);
	*destFileName = destFileNameBuf;
}

static void	setClassOfService(int tokenCount, char **tokens,
			BpUtParms *utParms)
{
	unsigned long	priority;

	if (tokenCount != 2)
	{
		PUTS("What's the priority?");
		return;
	}

	priority = strtoul(tokens[1], NULL, 0);
	utParms->classOfService = priority;
}

static void	setOrdinal(int tokenCount, char **tokens, BpUtParms *utParms)
{
	unsigned long	ordinal;

	if (tokenCount != 2)
	{
		PUTS("What's the ordinal?");
		return;
	}

	ordinal = strtoul(tokens[1], NULL, 0);
	utParms->ancillaryData.ordinal = ordinal;
}

static void	setMode(int tokenCount, char **tokens, BpUtParms *utParms)
{
	unsigned long	mode;

	if (tokenCount != 2)
	{
		PUTS("What's the mode?");
		return;
	}

	mode = strtoul(tokens[1], NULL, 0);
	if (mode & 0x01)	/*	Unreliable.			*/
	{
		utParms->ancillaryData.flags |= BP_BEST_EFFORT;
	}
	else	/*	Default: ECOS best-efforts flag = 0.		*/
	{
		if (mode & 0x02)	/*	Native BP reliability.	*/
		{
			utParms->custodySwitch = SourceCustodyRequired;
		}
		else		/*	Convergence-layer reliability.	*/
		{
			utParms->ancillaryData.flags &= (~BP_BEST_EFFORT);
			utParms->custodySwitch = NoCustodyRequested;
		}
	}
}

static void	setCtInterval(int tokenCount, char **tokens, BpUtParms *utParms)
{
	unsigned long	interval;

	if (tokenCount != 2)
	{
		PUTS("What's the custody transfer retransmission interval?");
		return;
	}

	interval = strtoul(tokens[1], NULL, 0);
	utParms->ctInterval = interval;
}

static void	setClosure(int tokenCount, char **tokens, CfdpReqParms *parms)
{
	unsigned long	latency;

	if (tokenCount != 2)
	{
		PUTS("What's the transaction closure latency?");
		return;
	}

	latency = strtoul(tokens[1], NULL, 0);
	parms->closureLatency = latency;
}

static void	setSegMd(int tokenCount, char **tokens, CfdpReqParms *parms)
{
	unsigned long	setting;

	if (tokenCount != 2)
	{
		PUTS("What's the segment metadata mode?");
		return;
	}

	setting = strtoul(tokens[1], NULL, 0);
	parms->segMetadataFn = (setting == 0) ? NULL : noteSegmentTime;
}

static void	setFlag(int *srrFlags, char *arg)
{
	if (strcmp(arg, "rcv") == 0)
	{
		(*srrFlags) |= BP_RECEIVED_RPT;
	}

	if (strcmp(arg, "ct") == 0)
	{
		(*srrFlags) |= BP_CUSTODY_RPT;
	}

	if (strcmp(arg, "fwd") == 0)
	{
		(*srrFlags) |= BP_FORWARDED_RPT;
	}

	if (strcmp(arg, "dlv") == 0)
	{
		(*srrFlags) |= BP_DELIVERED_RPT;
	}

	if (strcmp(arg, "del") == 0)
	{
		(*srrFlags) |= BP_DELETED_RPT;
	}
}

static void	setFlags(int *srrFlags, char *flagString)
{
	char	*cursor = flagString;
	char	*comma;

	while (1)
	{
		comma = strchr(cursor, ',');
		if (comma)
		{
			*comma = '\0';
			setFlag(srrFlags, cursor);
			*comma = ',';
			cursor = comma + 1;
			continue;
		}

		setFlag(srrFlags, cursor);
		return;
	}
}

static void	setSrrFlags(int tokenCount, char **tokens, BpUtParms *utParms)
{
	char	*flagString;
	int	flags;

	if (tokenCount != 2)
	{
		PUTS("What status report flags should be set?");
		return;
	}

	flagString = tokens[1];
	flags = 0;
	setFlags(&flags, flagString);
	utParms->srrFlags = flags;
}

static void	setCriticality(int tokenCount, char **tokens,
			BpUtParms *utParms)
{
	unsigned long	criticality;

	if (tokenCount != 2)
	{
		PUTS("What's the criticality?");
		return;
	}

	criticality = (strtoul(tokens[1], NULL, 0) == 0 ? 0 : 1);
	if (criticality == 1)
	{
		utParms->ancillaryData.flags |= BP_MINIMUM_LATENCY;
	}
	else
	{
		utParms->ancillaryData.flags &= (~BP_MINIMUM_LATENCY);
	}
}

static void	setTTL(int tokenCount, char **tokens, BpUtParms *utParms)
{
	unsigned long	TTL;

	if (tokenCount != 2)
	{
		PUTS("What's the TTL?");
		return;
	}

	TTL = strtoul(tokens[1], NULL, 0);
	utParms->lifespan = TTL;
}

static void	addMsgToUser(int tokenCount, char **tokens,
			MetadataList *msgsToUser)
{
	if (tokenCount != 2)
	{
		PUTS("What's the message text?");
		return;
	}

	if (*msgsToUser == 0)
	{
		*msgsToUser = cfdp_create_usrmsg_list();
	}

	oK(cfdp_add_usrmsg(*msgsToUser, (unsigned char *) tokens[1],
			strlen(tokens[1]) + 1));
}

static void	addFilestoreRequest(int tokenCount, char **tokens,
			MetadataList *fsRequests)
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
			PUTS("Syntax: <action code> <1st name> [<2nd name>]");
			return;
	}

	if (*fsRequests == 0)
	{
		*fsRequests = cfdp_create_fsreq_list();
	}

	oK(cfdp_add_fsreq(*fsRequests, action, firstPathName, secondPathName));
}

static int	processLine(char *line, int lineLength, CfdpReqParms *parms)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	CfdpProxyTask	task;

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
		PUTS("Too many tokens.");
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
			setDestinationEntityNbr(tokenCount, tokens,
					&(parms->destinationEntityNbr));
			return 0;

		case 'f':
			setSourceFileName(tokenCount, tokens,
					parms->sourceFileNameBuf,
					&(parms->sourceFileName));
			return 0;

		case 't':
			setDestFileName(tokenCount, tokens,
					parms->destFileNameBuf,
					&(parms->destFileName));
			return 0;

		case 'l':
			setTTL(tokenCount, tokens, &(parms->utParms));
			return 0;

		case 'p':
			setClassOfService(tokenCount, tokens,
					&(parms->utParms));
			return 0;

		case 'o':
			setOrdinal(tokenCount, tokens, &(parms->utParms));
			return 0;

		case 'm':
			setMode(tokenCount, tokens, &(parms->utParms));
			return 0;

		case 'i':
			setCtInterval(tokenCount, tokens, &(parms->utParms));
			return 0;

		case 'a':
			setClosure(tokenCount, tokens, parms);
			return 0;

		case 'n':
			setSegMd(tokenCount, tokens, parms);
			return 0;

		case 'g':
			setSrrFlags(tokenCount, tokens, &(parms->utParms));
			return 0;

		case 'c':
			setCriticality(tokenCount, tokens, &(parms->utParms));
			return 0;

		case 'u':
			addMsgToUser(tokenCount, tokens, &(parms->msgsToUser));
			return 0;

		case 'r':
			addFilestoreRequest(tokenCount, tokens,
					&(parms->fsRequests));
			return 0;

		case '&':
			if (cfdp_put(&(parms->destinationEntityNbr),
					sizeof(BpUtParms),
					(unsigned char *) &(parms->utParms),
					parms->sourceFileName,
					parms->destFileName, NULL,
					parms->segMetadataFn,
					NULL, 0, NULL,
					parms->closureLatency,
					parms->msgsToUser,
					parms->fsRequests,
					&(parms->transactionId)) < 0)
			{
				putErrmsg("Can't put FDU.", NULL);
				return -1;
			}

			parms->msgsToUser = 0;
			parms->fsRequests = 0;
			return 0;

		case '|':
			task.sourceFileName = parms->sourceFileName;
			task.destFileName = parms->destFileName;
			task.messagesToUser = parms->msgsToUser;
			task.filestoreRequests = parms->fsRequests;
			task.faultHandlers = NULL;
			task.unacknowledged = 1;
			task.flowLabelLength = 0;
			task.flowLabel = NULL;
			task.recordBoundsRespected = 0;
			task.closureRequested = !(parms->closureLatency == 0);
			if (cfdp_get(&(parms->destinationEntityNbr),
					sizeof(BpUtParms),
					(unsigned char *) &(parms->utParms),
					NULL, NULL, NULL, NULL, 0, NULL, 0, 0,
					0, &task, &(parms->transactionId)) < 0)
			{
				putErrmsg("Can't put FDU.", NULL);
				return -1;
			}

			parms->msgsToUser = 0;
			parms->fsRequests = 0;
			return 0;

		case '^':
			if (cfdp_cancel(&(parms->transactionId)) < 0)
			{
				putErrmsg("Can't cancel transaction.", NULL);
				return -1;
			}

			return 0;

		case '%':
			if (cfdp_suspend(&(parms->transactionId)) < 0)
			{
				putErrmsg("Can't suspend transaction.", NULL);
				return -1;
			}

			return 0;

		case '$':
			if (cfdp_resume(&(parms->transactionId)) < 0)
			{
				putErrmsg("Can't resume transaction.", NULL);
				return -1;
			}

			return 0;

		case '#':
			if (cfdp_report(&(parms->transactionId)) < 0)
			{
				putErrmsg("Can't report transaction.", NULL);
				return -1;
			}

			return 0;

		case 'z':
			if (tokenCount == 1)
			{
				snooze(1);
			}
			else
			{
				snooze(strtol(tokens[1], NULL, 0));
			}

			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			PUTS("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static char	*getMessageText(unsigned char *buf, unsigned int length)
{
	unsigned int	msgtype;
	char		*msgtext[] =	{
				"proxy put request",
				"proxy message to user",
				"proxy filestore request",
				"proxy fault handler override",
				"proxy transmission mode",
				"proxy flow label",
				"proxy segmentation control",
				"proxy put response",
				"proxy filestore response",
				"proxy put cancel",
				"originating transaction ID",
				"proxy closure request",
				"undefined",
				"undefined",
				"undefined",
				"undefined",
				"directory listing request",
				"directory listing response"
					};

	if (length < 5 || strncmp((char *) buf, "cfdp", 4) != 0)
	{
		return "undefined";
	}

	msgtype = *(buf + 4);
	if (msgtype > 17)
	{
		return "unknown user operation";
	}

	return msgtext[msgtype];
}

static void	*handleEvents(void *parm)
{
	int			*running = (int *) parm;
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
	unsigned int		recordBoundsRespected;
	CfdpContinuationState	continuationState;
	unsigned int		segMetadataLength;
	char			segMetadata[63];
	CfdpCondition		condition;
	uvast			progress;
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
	unsigned int		closureRequested;  // ✅ NEW VARIABLE FOR YOUR ENHANCED API

	while (*running)
	{
		if (cfdp_get_event(&type, &time, &reqNbr, &transactionId,
				sourceFileNameBuf, destFileNameBuf,
				&fileSize, &messagesToUser, &offset, &length,
				&recordBoundsRespected, &continuationState,
				&segMetadataLength, segMetadata,
				&condition, &progress, &fileStatus,
				&deliveryCode, &originatingTransactionId,
				statusReportBuf, &filestoreResponses,
				&closureRequested) < 0)  // ✅ YOUR ENHANCED API PARAMETER
		{
			putErrmsg("Failed getting CFDP event.", NULL);
			return NULL;
		}

		reportCfdpEvent(type, statusReportBuf, condition, deliveryCode, fileStatus, progress, &transactionId, closureRequested);

		if (type == CfdpAccessEnded)
		{
			break;		/*	Shut down.		*/
		}

		if (type == CfdpNoEvent)
		{
			continue;	/*	Interrupted.		*/
		}

		if (type == CfdpFileSegmentRecvInd)
		{
			if (segMetadataLength > 0)
			{
				printf("...Seg metadata '%s'\n", segMetadata);
			}
		}

		while (messagesToUser)
		{
			if (cfdp_get_usrmsg(&messagesToUser, usrmsgBuf,
					(int *) &length) < 0)
			{
				putErrmsg("Failed getting user msg.", NULL);
				return NULL;
			}

			if (length > 0)
			{
				usrmsgBuf[length] = '\0';
				printf("\tMessage to user: %s\n",
					getMessageText(usrmsgBuf, length));
			}
		}

		while (filestoreResponses)
		{
			if (cfdp_get_fsresp(&filestoreResponses, &action,
					&status, firstPathName, secondPathName,
					msgBuf) < 0)
			{
				putErrmsg("Failed getting FS response.", NULL);
				return NULL;
			}

			if (action != ((CfdpAction) -1))
			{
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
	int		cmdFile;
	char		line[256];
	int		len;
	pthread_t	receiverThread;
	int		running = 1;
	CfdpReqParms	parms;

	/*	Start the receiver thread.				*/

	if (pthread_begin(&receiverThread, NULL, handleEvents, &running))
	{
		putSysErrmsg("cfdptest can't create receiver thread", NULL);
		return 1;
	}

	memset((char *) &parms, 0, sizeof(CfdpReqParms));
	cfdp_compress_number(&parms.destinationEntityNbr, 0);
	parms.utParms.lifespan = 86400;
	parms.utParms.classOfService = BP_STD_PRIORITY;
	parms.utParms.custodySwitch = NoCustodyRequested;
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
			break;			/*	Out of loop.	*/
		}

		if (len == 0)
		{
			continue;
		}

		if (processLine(line, len, &parms))
		{
			break;			/*	Out of loop.	*/
		}
	}

	/*	Stop the receiver thread by interrupting reception.	*/

	running = 0;
	cfdp_interrupt();
	pthread_join(receiverThread, NULL);
	PUTS("Stopping cfdptest.");
	return 0;
}

#if defined (ION_LWT)
int	cfdptest(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*cmdFileName = (char *) a1;
	int		interactive = 0;
#else
int	main(int argc, char **argv)
{
	char		*cmdFileName = argc > 1 ? argv[1] : NULL;
	int		interactive = (argc == 1);
#endif
	int		retval=0;
	CfdpReqParms	parms;
	int		cmdFile;
	char		line[256];
	int		len;

	if (cfdp_attach() < 0)
	{
		putErrmsg("cfdptest can't initialize CFDP.", NULL);
		return 1;
	}

	if (interactive)
	{
#ifndef FSWLOGGER	/*	Need stdin/stdout for interactivity.	*/
		retval = runCfdptestInteractive();
		ionDetach();
#endif
		return retval;
	}

	memset((char *) &parms, 0, sizeof(CfdpReqParms));
	cfdp_compress_number(&parms.destinationEntityNbr, 0);
	parms.utParms.lifespan = 86400;
	parms.utParms.classOfService = BP_STD_PRIORITY;
	parms.utParms.custodySwitch = NoCustodyRequested;
	if (cmdFileName != NULL)	/*	Scripted.	*/
	{
		cmdFile = open(cmdFileName, O_RDONLY, 0777);
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

				if (processLine(line, len, &parms))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping cfdptest.");
	ionDetach();
	return retval;
}
