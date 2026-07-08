/*
	cfdptest.c:	CFDP test shell program.

									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/

#include "cfdp.h"
#include "bputa.h"
#include "cfdpP.h"
#include <glob.h>

/* check directory listing extension */
#ifndef NO_DIRLIST
#include "cfdpops.h"
#endif

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

/* Helper macros to use PUTS with formatted output */
#define PUTS_FMT(fmt, ...) do { \
	char _buf[1024]; \
	snprintf(_buf, sizeof(_buf), fmt, __VA_ARGS__); \
	PUTS(_buf); \
	fflush(stdout); \
} while(0)

/* Simple transaction tracker for sender-side closure latency */
typedef struct {
	CfdpTransactionId transactionId;
	unsigned int senderClosureLatency; /* Store original closureLatency from cfdp_put() */
	time_t timestamp;
} SenderTransaction;

#define MAX_SENDER_TRANSACTIONS 50  /* Increased for busy systems */
static SenderTransaction senderTransactions[MAX_SENDER_TRANSACTIONS];
static int numSenderTransactions = 0;

/* Transaction tracker for detailed event history and summaries */
#define MAX_TRACKED_TRANSACTIONS 100
#define MAX_EVENTS_PER_TRANSACTION 50

typedef struct {
	time_t	      timestamp;
	CfdpEventType type;
	uvast	      progress;
} TrackedEvent;

typedef struct {
	CfdpTransactionId transactionId; /* Full ID for uniqueness */
	time_t		  startTime;
	time_t		  endTime;
	int		  isActive;
	char		  sourceFileName[256];
	char		  destFileName[256];
	uvast		  fileSize;
	int		  closureRequested; /* 1 if closureLatency > 0 */

	/* Event history */
	int	     eventCount;
	TrackedEvent events[MAX_EVENTS_PER_TRANSACTION];

	/* Final status */
	CfdpCondition	 finalCondition;
	CfdpDeliveryCode finalDelivery;
	CfdpFileStatus	 finalStatus;
	char		 outcome[64]; /* "SUCCESS", "TIMEOUT", etc. */
} TransactionTracker;

static TransactionTracker transactionTrackers[MAX_TRACKED_TRANSACTIONS];
static int numTrackedTransactions = 0;
static int verboseMode = 0;

static void	handleQuit(int signum);
static void	printUsage(void);
#ifndef NO_DIRLIST
static void	displayDirListing(const char *filename);
#endif

/* Forward declarations for helper functions */
static char *getConditionName(CfdpCondition condition);
static char *getDeliveryCodeName(CfdpDeliveryCode deliveryCode);
static char *getFileStatusName(CfdpFileStatus fileStatus);

/* Function to store sender transaction closure info */
static void storeSenderTransaction(CfdpTransactionId *transId, unsigned int closureLatency)
{
	int i;

	/* Check if transaction already exists (update it) */
	for (i = 0; i < numSenderTransactions; i++) {
		if (memcmp(&senderTransactions[i].transactionId.sourceEntityNbr,
				&transId->sourceEntityNbr, sizeof(CfdpNumber)) == 0 &&
			memcmp(&senderTransactions[i].transactionId.transactionNbr,
				&transId->transactionNbr, sizeof(CfdpNumber)) == 0) {
			senderTransactions[i].senderClosureLatency = closureLatency;
			senderTransactions[i].timestamp = time(NULL);
			return;
		}
	}

	/* Add new transaction (remove oldest if full) */
	if (numSenderTransactions >= MAX_SENDER_TRANSACTIONS) {
		memmove(&senderTransactions[0], &senderTransactions[1],
			(MAX_SENDER_TRANSACTIONS-1) * sizeof(SenderTransaction));
		numSenderTransactions--;
	}

	memcpy(&senderTransactions[numSenderTransactions].transactionId, transId, sizeof(CfdpTransactionId));
	senderTransactions[numSenderTransactions].senderClosureLatency = closureLatency;
	senderTransactions[numSenderTransactions].timestamp = time(NULL);
	numSenderTransactions++;
}

/* Function to clean up completed transactions */
static void cleanupCompletedTransaction(CfdpTransactionId *transId)
{
	int i, j;
	for (i = 0; i < numSenderTransactions; i++) {
		if (memcmp(&senderTransactions[i].transactionId.sourceEntityNbr,
				&transId->sourceEntityNbr, sizeof(CfdpNumber)) == 0 &&
			memcmp(&senderTransactions[i].transactionId.transactionNbr,
				&transId->transactionNbr, sizeof(CfdpNumber)) == 0) {
			/* Remove this transaction by shifting remaining ones down */
			for (j = i; j < numSenderTransactions - 1; j++) {
				memcpy(&senderTransactions[j], &senderTransactions[j + 1],
					sizeof(SenderTransaction));
			}
			numSenderTransactions--;
			break;
		}
	}
}

/* Function to get sender closure info */
static int getSenderClosureLatency(CfdpTransactionId *transId)
{
	int i;
	for (i = 0; i < numSenderTransactions; i++) {
		if (memcmp(&senderTransactions[i].transactionId.sourceEntityNbr,
				&transId->sourceEntityNbr, sizeof(CfdpNumber)) == 0 &&
			memcmp(&senderTransactions[i].transactionId.transactionNbr,
				&transId->transactionNbr, sizeof(CfdpNumber)) == 0) {
			return senderTransactions[i].senderClosureLatency;
		}
	}
	return -1; /* Not found */
}

/* ========== Transaction Tracking Functions ========== */

/* Compare two transaction IDs using full 16-byte structure */
static int compareTransactionIds(CfdpTransactionId *id1, CfdpTransactionId *id2)
{
	return memcmp(id1, id2, sizeof(CfdpTransactionId));
}

/* Find transaction index by full transaction ID */
static int findTransactionIndex(CfdpTransactionId *transactionId)
{
	int i;
	for (i = 0; i < numTrackedTransactions; i++) {
		if (compareTransactionIds(&transactionTrackers[i].transactionId, transactionId) == 0) {
			return i;
		}
	}
	return -1;  /* Not found */
}

/* Clean up old completed transactions (older than 1 hour) */
static void cleanupOldTransactions(void)
{
	time_t now = time(NULL);
	int i, j;

	for (i = 0; i < numTrackedTransactions; ) {
		if (!transactionTrackers[i].isActive &&
			(now - transactionTrackers[i].endTime) > 3600) {
			/* Remove this transaction by shifting remaining ones down */
			for (j = i; j < numTrackedTransactions - 1; j++) {
				memcpy(&transactionTrackers[j], &transactionTrackers[j + 1],
					sizeof(TransactionTracker));
			}
			numTrackedTransactions--;
			/* Don't increment i - check same position again */
		} else {
			i++;
		}
	}
}

/* Initialize transaction tracker on Event 1 (TransactionInd) */
static void initTransactionTracker(CfdpTransactionId *transactionId,
		char *sourceFileName,
		char *destFileName,
		uvast fileSize,
		unsigned int closureRequested)
{
	int idx;

	/* Check if already exists */
	idx = findTransactionIndex(transactionId);
	if (idx >= 0) {
		/* Already tracking this transaction.  CFDP events may be
		 * detected (and delivered) out of semantic order, so a
		 * late Transaction event must not reactivate an entry
		 * that has already been finalized - it would be reported
		 * as active forever, since Finished won't come again. */
		if (transactionTrackers[idx].isActive) {
			transactionTrackers[idx].startTime = time(NULL);
		}
		return;
	}

	/* Clean up old transactions if needed */
	if (numTrackedTransactions >= MAX_TRACKED_TRANSACTIONS) {
		cleanupOldTransactions();
		/* If still full, remove oldest completed transaction */
		if (numTrackedTransactions >= MAX_TRACKED_TRANSACTIONS) {
			int oldestIdx = -1;
			time_t oldestTime = time(NULL);
			int i;
			for (i = 0; i < numTrackedTransactions; i++) {
				if (!transactionTrackers[i].isActive &&
					transactionTrackers[i].endTime < oldestTime) {
					oldestTime = transactionTrackers[i].endTime;
					oldestIdx = i;
				}
			}
			if (oldestIdx >= 0) {
				/* Remove oldest */
				int j;
				for (j = oldestIdx; j < numTrackedTransactions - 1; j++) {
					memcpy(&transactionTrackers[j], &transactionTrackers[j + 1],
						sizeof(TransactionTracker));
				}
				numTrackedTransactions--;
			}
		}
	}

	/* Add new transaction */
	if (numTrackedTransactions < MAX_TRACKED_TRANSACTIONS) {
		int senderClosureLatency;
		idx = numTrackedTransactions;
		memset(&transactionTrackers[idx], 0, sizeof(TransactionTracker));
		memcpy(&transactionTrackers[idx].transactionId, transactionId,
			sizeof(CfdpTransactionId));
		transactionTrackers[idx].startTime = time(NULL);
		transactionTrackers[idx].isActive = 1;

		/* Try to get sender closure info first (more reliable for locally-initiated transactions) */
		senderClosureLatency = getSenderClosureLatency(transactionId);
		if (senderClosureLatency >= 0) {
			transactionTrackers[idx].closureRequested = (senderClosureLatency > 0) ? 1 : 0;
		} else {
			/* Fall back to closureRequested parameter from event */
			transactionTrackers[idx].closureRequested = (closureRequested > 0) ? 1 : 0;
		}

		transactionTrackers[idx].fileSize = fileSize;

		if (sourceFileName) {
			size_t len = strlen(sourceFileName);
			if (len >= sizeof(transactionTrackers[idx].sourceFileName)) {
				len = sizeof(transactionTrackers[idx].sourceFileName) - 1;
			}
			memcpy(transactionTrackers[idx].sourceFileName, sourceFileName, len);
			transactionTrackers[idx].sourceFileName[len] = '\0';
		}
		if (destFileName) {
			size_t len = strlen(destFileName);
			if (len >= sizeof(transactionTrackers[idx].destFileName)) {
				len = sizeof(transactionTrackers[idx].destFileName) - 1;
			}
			memcpy(transactionTrackers[idx].destFileName, destFileName, len);
			transactionTrackers[idx].destFileName[len] = '\0';
		}

		numTrackedTransactions++;
	}
}

/* Add event to transaction history */
static void addTransactionEvent(CfdpTransactionId *transactionId,
		CfdpEventType eventType,
		uvast progress)
{
	int idx = findTransactionIndex(transactionId);
	int senderClosureLatency;

	if (idx < 0) {
		return;  /* Transaction not found */
	}

	/* Update closureRequested if sender info becomes available */
	senderClosureLatency = getSenderClosureLatency(transactionId);
	if (senderClosureLatency >= 0) {
		transactionTrackers[idx].closureRequested = (senderClosureLatency > 0) ? 1 : 0;
	}

	if (transactionTrackers[idx].eventCount < MAX_EVENTS_PER_TRANSACTION) {
		int eventIdx = transactionTrackers[idx].eventCount;
		transactionTrackers[idx].events[eventIdx].timestamp = time(NULL);
		transactionTrackers[idx].events[eventIdx].type = eventType;
		transactionTrackers[idx].events[eventIdx].progress = progress;
		transactionTrackers[idx].eventCount++;
	}
}

/* Determine outcome string from status codes */
static void determineOutcome(char *outcome, int outcomeSize,
		CfdpCondition condition,
		CfdpDeliveryCode deliveryCode,
		CfdpFileStatus fileStatus,
		int acknowledgedMode)
{
	/* Based on CFDP-SW-Architecture.md */
	if (condition == CfdpNoError && deliveryCode == CfdpDataComplete &&
		fileStatus == CfdpFileRetained) {
		snprintf(outcome, outcomeSize, "SUCCESS");
	} else if (condition == CfdpNoError && deliveryCode == CfdpDataIncomplete &&
			fileStatus == CfdpFileStatusUnreported && !acknowledgedMode) {
		snprintf(outcome, outcomeSize, "NORMAL");
	} else if (condition == CfdpCheckLimitReached) {
		snprintf(outcome, outcomeSize, "TIMEOUT");
	} else if (condition == CfdpChecksumFailure) {
		snprintf(outcome, outcomeSize, "CHECKSUM FAILURE");
	} else if (condition == CfdpCancelRequested) {
		snprintf(outcome, outcomeSize, "CANCELLED");
	} else if (condition == CfdpNoError && fileStatus == CfdpFileDiscarded) {
		snprintf(outcome, outcomeSize, "DISCARDED");
	} else if (deliveryCode == CfdpDataIncomplete) {
		snprintf(outcome, outcomeSize, "INCOMPLETE");
	} else {
		snprintf(outcome, outcomeSize, "UNKNOWN");
	}
}

/* Finalize transaction on Events 3 (Finished) or 11 (Abandoned) */
static void finalizeTransaction(CfdpTransactionId *transactionId,
		CfdpCondition condition,
		CfdpDeliveryCode deliveryCode,
		CfdpFileStatus fileStatus)
{
	int idx = findTransactionIndex(transactionId);
	if (idx < 0) {
		/* Finished/Abandoned can be delivered before the
		 * Transaction event when CFDP detects events out of
		 * order.  Create the entry now so the completion is
		 * recorded instead of dropped; otherwise a late
		 * Transaction event would create an entry that stays
		 * active forever. */
		initTransactionTracker(transactionId, NULL, NULL, 0, 0);
		idx = findTransactionIndex(transactionId);
		if (idx < 0) {
			return;  /* Tracker full */
		}
	}

	transactionTrackers[idx].endTime = time(NULL);
	transactionTrackers[idx].isActive = 0;
	transactionTrackers[idx].finalCondition = condition;
	transactionTrackers[idx].finalDelivery = deliveryCode;
	transactionTrackers[idx].finalStatus = fileStatus;

	determineOutcome(transactionTrackers[idx].outcome,
			sizeof(transactionTrackers[idx].outcome),
			condition, deliveryCode, fileStatus,
			transactionTrackers[idx].closureRequested);
}

/* Format duration in seconds to HH:MM:SS */
static void formatDuration(time_t seconds, char *buffer, int bufferSize)
{
	int hours = seconds / 3600;
	int minutes = (seconds % 3600) / 60;
	int secs = seconds % 60;
	snprintf(buffer, bufferSize, "%02d:%02d:%02d", hours, minutes, secs);
}

/* Format timestamp to HH:MM:SS.mmm */
static void formatTimestamp(time_t timestamp, char *buffer, int bufferSize)
{
	struct tm *tm_info = localtime(&timestamp);
	snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
		tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
		tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

/* Print detailed summary for a single transaction */
static void printTransactionSummary(CfdpTransactionId *transactionId)
{
	int idx = findTransactionIndex(transactionId);
	uvast srcEntity, txnNbr;
	char entityBuf[FQN_MAX_LENGTH];
	char timeStr[64];
	char durationStr[32];
	int i;

	if (idx < 0) {
		PUTS("Transaction not found in tracker.");
		fflush(stdout);
		return;
	}

	cfdp_decompress_number(&srcEntity, &transactionId->sourceEntityNbr);
	cfdp_decompress_number(&txnNbr, &transactionId->transactionNbr);
	putFqn(entityBuf, srcEntity);

	PUTS("=== TRANSACTION SUMMARY ===");
	PUTS_FMT("Transaction: %s." UVAST_FIELDSPEC, entityBuf, txnNbr);

	if (transactionTrackers[idx].sourceFileName[0] || transactionTrackers[idx].destFileName[0]) {
		PUTS_FMT("File: %s -> %s",
			transactionTrackers[idx].sourceFileName[0] ? transactionTrackers[idx].sourceFileName : "(unknown)",
			transactionTrackers[idx].destFileName[0] ? transactionTrackers[idx].destFileName : "(unknown)");
	}

	formatTimestamp(transactionTrackers[idx].startTime, timeStr, sizeof(timeStr));
	PUTS_FMT("Start Time: %s", timeStr);

	if (!transactionTrackers[idx].isActive) {
		formatTimestamp(transactionTrackers[idx].endTime, timeStr, sizeof(timeStr));
		PUTS_FMT("End Time: %s", timeStr);

		time_t duration = transactionTrackers[idx].endTime - transactionTrackers[idx].startTime;
		formatDuration(duration, durationStr, sizeof(durationStr));
		PUTS_FMT("Duration: %s", durationStr);
	} else {
		time_t duration = time(NULL) - transactionTrackers[idx].startTime;
		formatDuration(duration, durationStr, sizeof(durationStr));
		PUTS_FMT("Duration (so far): %s", durationStr);
	}

	PUTS_FMT("Mode: %s", transactionTrackers[idx].closureRequested ? "Closure-Requested" : "Unacknowledged");

	if (transactionTrackers[idx].fileSize > 0) {
		PUTS_FMT("File Size: " UVAST_FIELDSPEC " bytes", transactionTrackers[idx].fileSize);
	}

	PUTS("");
	PUTS("Event History:");

	for (i = 0; i < transactionTrackers[idx].eventCount; i++) {
		TrackedEvent *ev = &transactionTrackers[idx].events[i];
		formatTimestamp(ev->timestamp, timeStr, sizeof(timeStr));

		if (ev->type >= 0 && ev->type <= 11) {
			PUTS_FMT("  %s - Event %d: %s (" UVAST_FIELDSPEC " bytes)",
				timeStr, ev->type, eventTypes[ev->type], ev->progress);
		} else {
			PUTS_FMT("  %s - Event %d: unknown (" UVAST_FIELDSPEC " bytes)",
				timeStr, ev->type, ev->progress);
		}
	}

	if (!transactionTrackers[idx].isActive) {
		PUTS("");
		PUTS("Final Status:");
		PUTS_FMT("  Condition: %s (%d)", getConditionName(transactionTrackers[idx].finalCondition),
			transactionTrackers[idx].finalCondition);
		PUTS_FMT("  Delivery: %s (%d)", getDeliveryCodeName(transactionTrackers[idx].finalDelivery),
			transactionTrackers[idx].finalDelivery);
		PUTS_FMT("  File Status: %s (%d)", getFileStatusName(transactionTrackers[idx].finalStatus),
			transactionTrackers[idx].finalStatus);
		PUTS_FMT("  OUTCOME: %s", transactionTrackers[idx].outcome);
	}

	PUTS("===========================");
	fflush(stdout);
}

/* Print summary table of all tracked transactions */
static void printAllTransactionsSummary(void)
{
	char timeStr[64];
	char durationStr[32];
	uvast srcEntity, txnNbr;
	char entityBuf[FQN_MAX_LENGTH];
	int i;
	int activeCount = 0;
	int completedCount = 0;
	int failedCount = 0;
	int successCount = 0;
	uvast totalBytes = 0;
	time_t totalDuration = 0;

	PUTS("");
	PUTS("=== ALL TRANSACTIONS SUMMARY ===");
	formatTimestamp(time(NULL), timeStr, sizeof(timeStr));
	PUTS_FMT("Current Time: %s", timeStr);
	PUTS("");

	/* Count and categorize */
	for (i = 0; i < numTrackedTransactions; i++) {
		if (transactionTrackers[i].isActive) {
			activeCount++;
		} else {
			if (strcmp(transactionTrackers[i].outcome, "SUCCESS") == 0 ||
				strcmp(transactionTrackers[i].outcome, "NORMAL") == 0) {
				completedCount++;
				successCount++;
			} else {
				failedCount++;
			}
			totalDuration += (transactionTrackers[i].endTime - transactionTrackers[i].startTime);
			if (transactionTrackers[i].eventCount > 0) {
				totalBytes += transactionTrackers[i].events[transactionTrackers[i].eventCount - 1].progress;
			}
		}
	}

	/* Display active transactions */
	if (activeCount > 0) {
		PUTS_FMT("Active Transactions (%d):", activeCount);
		PUTS_FMT("%-20s %-7s %-25s %-12s %-20s",
			"Transaction", "Mode", "File", "Started", "Progress");
		PUTS("--------------------------------------------------------------------------------");

		for (i = 0; i < numTrackedTransactions; i++) {
			if (!transactionTrackers[i].isActive) continue;

			cfdp_decompress_number(&srcEntity, &transactionTrackers[i].transactionId.sourceEntityNbr);
			cfdp_decompress_number(&txnNbr, &transactionTrackers[i].transactionId.transactionNbr);

			formatTimestamp(transactionTrackers[i].startTime, timeStr, sizeof(timeStr));

			uvast currentProgress = 0;
			if (transactionTrackers[i].eventCount > 0) {
				currentProgress = transactionTrackers[i].events[transactionTrackers[i].eventCount - 1].progress;
			}

			char fileDisplay[26];
			char transIdStr[FQN_MAX_LENGTH + 21];
			char modeStr[8];
			char progressStr[21];

			if (transactionTrackers[i].sourceFileName[0]) {
				snprintf(fileDisplay, sizeof(fileDisplay), "%.25s", transactionTrackers[i].sourceFileName);
			} else {
				snprintf(fileDisplay, sizeof(fileDisplay), "(unknown)");
			}

			putFqn(entityBuf, srcEntity);
			snprintf(transIdStr, sizeof(transIdStr), "%s." UVAST_FIELDSPEC,
				entityBuf, txnNbr);
			snprintf(modeStr, sizeof(modeStr), "%s",
				transactionTrackers[i].closureRequested ? "ClosReq" : "Unack");
			snprintf(progressStr, sizeof(progressStr), UVAST_FIELDSPEC "/" UVAST_FIELDSPEC,
				currentProgress, transactionTrackers[i].fileSize);

			PUTS_FMT("%-20s %-7s %-25s %-12s %-20s",
				transIdStr, modeStr, fileDisplay,
				timeStr + 11,  /* Just time part */
				progressStr);
		}
		PUTS("");
	}

	/* Display completed transactions */
	if (completedCount > 0) {
		PUTS_FMT("Completed Transactions (%d):", completedCount);
		PUTS_FMT("%-20s %-7s %-25s %-12s %-12s %s",
			"Transaction", "Mode", "File", "Completed", "Duration", "Outcome");
		PUTS("--------------------------------------------------------------------------------");

		for (i = 0; i < numTrackedTransactions; i++) {
			if (transactionTrackers[i].isActive) continue;
			if (strcmp(transactionTrackers[i].outcome, "SUCCESS") != 0 &&
				strcmp(transactionTrackers[i].outcome, "NORMAL") != 0) continue;

			cfdp_decompress_number(&srcEntity, &transactionTrackers[i].transactionId.sourceEntityNbr);
			cfdp_decompress_number(&txnNbr, &transactionTrackers[i].transactionId.transactionNbr);

			formatTimestamp(transactionTrackers[i].endTime, timeStr, sizeof(timeStr));
			formatDuration(transactionTrackers[i].endTime - transactionTrackers[i].startTime,
					durationStr, sizeof(durationStr));

			char fileDisplay[26];
			char transIdStr[FQN_MAX_LENGTH + 21];
			char modeStr[8];

			if (transactionTrackers[i].sourceFileName[0]) {
				snprintf(fileDisplay, sizeof(fileDisplay), "%.25s", transactionTrackers[i].sourceFileName);
			} else {
				snprintf(fileDisplay, sizeof(fileDisplay), "(unknown)");
			}

			putFqn(entityBuf, srcEntity);
			snprintf(transIdStr, sizeof(transIdStr), "%s." UVAST_FIELDSPEC,
				entityBuf, txnNbr);
			snprintf(modeStr, sizeof(modeStr), "%s",
				transactionTrackers[i].closureRequested ? "ClosReq" : "Unack");

			PUTS_FMT("%-20s %-7s %-25s %-12s %-12s %s",
				transIdStr, modeStr, fileDisplay,
				timeStr + 11,  /* Just time part */
				durationStr,
				transactionTrackers[i].outcome);
		}
		PUTS("");
	}

	/* Display failed transactions */
	if (failedCount > 0) {
		PUTS_FMT("Failed/Abandoned Transactions (%d):", failedCount);
		PUTS_FMT("%-20s %-7s %-25s %-12s %-12s %s",
			"Transaction", "Mode", "File", "Failed", "Duration", "Reason");
		PUTS("--------------------------------------------------------------------------------");

		for (i = 0; i < numTrackedTransactions; i++) {
			if (transactionTrackers[i].isActive) continue;
			if (strcmp(transactionTrackers[i].outcome, "SUCCESS") == 0 ||
				strcmp(transactionTrackers[i].outcome, "NORMAL") == 0) continue;

			cfdp_decompress_number(&srcEntity, &transactionTrackers[i].transactionId.sourceEntityNbr);
			cfdp_decompress_number(&txnNbr, &transactionTrackers[i].transactionId.transactionNbr);

			formatTimestamp(transactionTrackers[i].endTime, timeStr, sizeof(timeStr));
			formatDuration(transactionTrackers[i].endTime - transactionTrackers[i].startTime,
					durationStr, sizeof(durationStr));

			char fileDisplay[26];
			char transIdStr[FQN_MAX_LENGTH + 21];
			char modeStr[8];

			if (transactionTrackers[i].sourceFileName[0]) {
				snprintf(fileDisplay, sizeof(fileDisplay), "%.25s", transactionTrackers[i].sourceFileName);
			} else {
				snprintf(fileDisplay, sizeof(fileDisplay), "(unknown)");
			}

			putFqn(entityBuf, srcEntity);
			snprintf(transIdStr, sizeof(transIdStr), "%s." UVAST_FIELDSPEC,
				entityBuf, txnNbr);
			snprintf(modeStr, sizeof(modeStr), "%s",
				transactionTrackers[i].closureRequested ? "ClosReq" : "Unack");

			PUTS_FMT("%-20s %-7s %-25s %-12s %-12s %s",
				transIdStr, modeStr, fileDisplay,
				timeStr + 11,  /* Just time part */
				durationStr,
				transactionTrackers[i].outcome);
		}
		PUTS("");
	}

	/* Summary statistics */
	PUTS("Summary Statistics:");
	PUTS_FMT("  Total Transactions: %d", numTrackedTransactions);
	PUTS_FMT("  Active: %d", activeCount);
	PUTS_FMT("  Successful: %d", successCount);
	PUTS_FMT("  Failed: %d", failedCount);
	PUTS_FMT("  Total Bytes Transferred: " UVAST_FIELDSPEC, totalBytes);

	if (completedCount + failedCount > 0) {
		time_t avgDuration = totalDuration / (completedCount + failedCount);
		formatDuration(avgDuration, durationStr, sizeof(durationStr));
		PUTS_FMT("  Average Duration: %s", durationStr);
	}

	PUTS("================================");
	fflush(stdout);
}

/* Helper function to get state name */
static char *getStateName(FduState state)
{
	switch (state) {
		case FduActive: return "Active";
		case FduSuspended: return "Suspended";
		case FduCanceled: return "Canceled";
		default: return "Unknown";
	}
}

/* Parse transaction ID from string in format "source.transaction" */
static int parseTransactionId(char *idStr, CfdpTransactionId *transId)
{
	char *dot;
	uvast sourceEntity, txnNbr;

	if (idStr == NULL || transId == NULL) {
		return -1;
	}

	/* Find the last dot separator (to handle FQNN dotted notation) */
	dot = strrchr(idStr, '.');
	if (dot == NULL) {
		PUTS("Transaction ID must be in format: sourceEntity.transactionNbr");
		fflush(stdout);
		return -1;
	}

	/* Parse source entity number */
	*dot = '\0';  /* Temporarily terminate the string */
	sourceEntity = getFqn(idStr);
	*dot = '.';   /* Restore the dot */

	/* Parse transaction number */
	txnNbr = strtouvast(dot + 1);

	/* Compress into CfdpNumber format */
	cfdp_compress_number(&transId->sourceEntityNbr, sourceEntity);
	cfdp_compress_number(&transId->transactionNbr, txnNbr);

	return 0;
}

/* Check if transaction is locally initiated */
static int isLocalTransaction(CfdpTransactionId *transId)
{
	CfdpDB *db;
	Sdr sdr;
	int isLocal;

	sdr = getIonsdr();
	CHKERR(sdr_begin_xn(sdr));
	db = getCfdpConstants();
	if (db == NULL) {
		sdr_exit_xn(sdr);
		return 0;
	}

	/* Compare source entity with own entity */
	isLocal = (memcmp(transId->sourceEntityNbr.buffer,
			db->ownEntityNbr.buffer, 8) == 0);
	sdr_exit_xn(sdr);
	return isLocal;
}

/* List all known transactions */
static int listTransactions(void)
{
	Sdr sdr = getIonsdr();
	CfdpDB *db;
	SdrObject elt, obj;
	OutFdu outFdu;
	InFdu inFdu;
	Entity entity;
	uvast srcEntity, txnNbr;
	char entityBuf[FQN_MAX_LENGTH];
	int count = 0;

	CHKERR(sdr_begin_xn(sdr));
	db = getCfdpConstants();
	if (db == NULL) {
		sdr_exit_xn(sdr);
		PUTS("Unable to access CFDP database.");
		fflush(stdout);
		return -1;
	}

	PUTS("=== Active Transactions ===");
	PUTS("");
	PUTS("OUTBOUND Transactions (Locally Initiated):");
	PUTS_FMT("%-20s %-12s %-15s %s", "Transaction ID", "State", "Progress", "File");
	PUTS("--------------------------------------------------------------------");

	/* List outbound transactions */
	for (elt = sdr_list_first(sdr, db->outboundFdus); elt;
			elt = sdr_list_next(sdr, elt)) {
		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &outFdu, obj, sizeof(OutFdu));

		cfdp_decompress_number(&srcEntity, &outFdu.transactionId.sourceEntityNbr);
		cfdp_decompress_number(&txnNbr, &outFdu.transactionId.transactionNbr);
		putFqn(entityBuf, srcEntity);

		PUTS_FMT("%s." UVAST_FIELDSPEC " %-12s " UVAST_FIELDSPEC "/" UVAST_FIELDSPEC " %s",
			entityBuf, txnNbr,
			getStateName(outFdu.state),
			outFdu.progress, outFdu.fileSize,
			outFdu.sourceFileName);
		count++;
	}

	if (count == 0) {
		PUTS("  (none)");
	}

	PUTS("");
	PUTS("INBOUND Transactions (Remotely Initiated):");
	PUTS_FMT("%-20s %-12s %-15s %s", "Transaction ID", "State", "Progress", "File");
	PUTS("--------------------------------------------------------------------");

	count = 0;
	/* List inbound transactions from all entities */
	for (elt = sdr_list_first(sdr, db->entities); elt;
			elt = sdr_list_next(sdr, elt)) {
		SdrObject inElt, inObj;
		char destFileName[256];

		obj = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &entity, obj, sizeof(Entity));

		for (inElt = sdr_list_first(sdr, entity.inboundFdus); inElt;
				inElt = sdr_list_next(sdr, inElt)) {
			inObj = sdr_list_data(sdr, inElt);
			sdr_read(sdr, (char *) &inFdu, inObj, sizeof(InFdu));

			cfdp_decompress_number(&srcEntity, &inFdu.transactionId.sourceEntityNbr);
			cfdp_decompress_number(&txnNbr, &inFdu.transactionId.transactionNbr);
			putFqn(entityBuf, srcEntity);

			strcpy(destFileName, "(unknown)");
			if (inFdu.destFileName) {
				sdr_string_read(sdr, destFileName, inFdu.destFileName);
			}

			PUTS_FMT("%s." UVAST_FIELDSPEC " %-12s " UVAST_FIELDSPEC "/" UVAST_FIELDSPEC " %s",
				entityBuf, txnNbr,
				getStateName(inFdu.state),
				inFdu.progress, inFdu.fileSize,
				destFileName);
			count++;
		}
	}

	if (count == 0) {
		PUTS("  (none)");
	}

	PUTS("=======================");
	fflush(stdout);
	sdr_exit_xn(sdr);
	return 0;
}

/* Helper functions for field name translation */
static char *getConditionName(CfdpCondition condition)
{
	static char *conditionNames[] = {
		"NoError",           // 0
		"AckLimitReached",   // 1
		"KeepaliveLimitReached", // 2
		"InvalidTransmissionMode", // 3
		"FilestoreRejection", // 4
		"ChecksumFailure",   // 5
		"FileSizeError",     // 6
		"NakLimitReached",   // 7
		"InactivityDetected", // 8
		"InvalidFileStructure", // 9
		"CheckLimitReached", // 10
		"UnsupportedChecksumType", // 11
		"Reserved12",        // 12
		"Reserved13",        // 13
		"SuspendRequested",  // 14
		"CancelRequested"    // 15
	};

	if (condition <= 15) {
		return conditionNames[condition];
	}
	return "Unknown";
}

static char *getDeliveryCodeName(CfdpDeliveryCode deliveryCode)
{
	switch (deliveryCode) {
		case 0: return "Complete";
		case 1: return "Incomplete";
		default: return "Unknown";
	}
}

static char *getFileStatusName(CfdpFileStatus fileStatus)
{
	switch (fileStatus) {
		case 0: return "Discarded";
		case 1: return "Rejected";
		case 2: return "Retained";
		case 3: return "Unreported";
		default: return "Unknown";
	}
}

static void reportCfdpEvent(CfdpEventType type, char *statusReportBuf,
		CfdpCondition condition, CfdpDeliveryCode deliveryCode,
		CfdpFileStatus fileStatus, uvast progress,
		CfdpTransactionId *transactionId,
		unsigned int closureRequested, char *sourceFileNameBuf,
		char *destFileNameBuf, uvast fileSize)
{
	uvast srcEntityNbr, txnNbr;
	char entityBuf[FQN_MAX_LENGTH];
	char *ackMode;
	int isReliableAckMode = 0;  /* Flag to indicate if we have reliable ack mode info */

	/* DEBUG: Show what CFDP engine is actually providing
	cfdp_decompress_number(&srcEntityNbr, &transactionId->sourceEntityNbr);
	cfdp_decompress_number(&txnNbr, &transactionId->transactionNbr);
	putFqn(entityBuf, srcEntityNbr);
	printf("[DEBUG] Event %d for %s." UVAST_FIELDSPEC
		": progress=" UVAST_FIELDSPEC ", closureRequested=%u\n",
		type, entityBuf, txnNbr, progress, closureRequested);
	*/

	/* Handle special cases */
	if (type < 0) {
		PUTS("CFDP Event: CFDP access had ended.");
		fflush(stdout);
		return;
	}

	if (type == CfdpNoEvent) {
		return;  /* Skip NoEvent - just an interrupt signal */
	}

	/* Transaction tracking - Event 1: Initialize tracker */
	if (type == CfdpTransactionInd) {
		initTransactionTracker(transactionId, NULL, destFileNameBuf, 0, closureRequested);
	}

	/* Transaction tracking - Record all events */
	addTransactionEvent(transactionId, type, progress);

	/* Decompress transaction ID */
	cfdp_decompress_number(&srcEntityNbr, &transactionId->sourceEntityNbr);
	cfdp_decompress_number(&txnNbr, &transactionId->transactionNbr);
	putFqn(entityBuf, srcEntityNbr);

	/* Determine closure mode with enhanced reliability */
	if (type == CfdpMetadataRecvInd) {
		/* MOST RELIABLE: Receiver side during metadata reception */
		ackMode = closureRequested ? "Closure-requested" : "Unacknowledged";
		isReliableAckMode = 1;
	} else {
		/* For other events, try to get stored sender closure info first */
		int senderClosureLatency = getSenderClosureLatency(transactionId);
		if (senderClosureLatency >= 0) {
			/* We have definitive sender closure info */
			ackMode = (senderClosureLatency > 0) ? "Closure-requested" : "Unacknowledged";
			isReliableAckMode = 1;
		} else {
			/* Fall back to API parameter */
			ackMode = closureRequested ? "Closure-requested" : "Unacknowledged";
			isReliableAckMode = 0;
		}
	}

	/* Display basic event information */
	PUTS("=== CFDP EVENT ===");
	PUTS_FMT("Event: %s (%d)",
		(type >= 0 && type < 12) ? eventTypes[type] : "unknown", type);
	PUTS_FMT("Transaction: %s." UVAST_FIELDSPEC, entityBuf, txnNbr);

	/* Show acknowledge mode with confidence indicator */
	if (type == CfdpMetadataRecvInd) {
		PUTS_FMT(" (%s mode - receiver side, definitive)", ackMode);
	} else if (isReliableAckMode) {
		PUTS_FMT(" (%s mode - confirmed)", ackMode);
	} else {
		PUTS_FMT(" (%s mode - from CFDP API)", ackMode);
	}
		fflush(stdout);

	/* Display event-specific critical parameters based on field applicability analysis */
	switch (type) {
		case CfdpMetadataRecvInd:         // Event 4
			if (sourceFileNameBuf && sourceFileNameBuf[0]) {
				PUTS_FMT("Source File: %s", sourceFileNameBuf);
			}
			if (destFileNameBuf && destFileNameBuf[0]) {
				PUTS_FMT("Dest File: %s", destFileNameBuf);
			}
			if (fileSize > 0) {
				PUTS_FMT("File Size: " UVAST_FIELDSPEC " bytes", fileSize);
			}
			/* Update tracker with file info from metadata */
			{
				int tidx = findTransactionIndex(transactionId);
				if (tidx >= 0) {
					if (sourceFileNameBuf && sourceFileNameBuf[0]
					&& !transactionTrackers[tidx].sourceFileName[0]) {
						istrcpy(transactionTrackers[tidx].sourceFileName,
							sourceFileNameBuf,
							sizeof(transactionTrackers[tidx].sourceFileName));
					}
					if (destFileNameBuf && destFileNameBuf[0]
					&& !transactionTrackers[tidx].destFileName[0]) {
						istrcpy(transactionTrackers[tidx].destFileName,
							destFileNameBuf,
							sizeof(transactionTrackers[tidx].destFileName));
					}
					if (fileSize > 0) {
						transactionTrackers[tidx].fileSize = fileSize;
					}
				}
			}
			if (condition != CfdpNoError) {
				PUTS_FMT("Condition: %s (%d)", getConditionName(condition), condition);
			}
			break;

		case CfdpTransactionInd:          // Event 1
			if (sourceFileNameBuf && sourceFileNameBuf[0]) {
				PUTS_FMT("Source File: %s", sourceFileNameBuf);
			}
			if (destFileNameBuf && destFileNameBuf[0]) {
				PUTS_FMT("Dest File: %s", destFileNameBuf);
			}
			if (fileSize > 0) {
				PUTS_FMT("File Size: " UVAST_FIELDSPEC " bytes", fileSize);
			}
			/* Update tracker with file info from event */
			{
				int tidx = findTransactionIndex(transactionId);
				if (tidx >= 0) {
					if (sourceFileNameBuf && sourceFileNameBuf[0]
					&& !transactionTrackers[tidx].sourceFileName[0]) {
						istrcpy(transactionTrackers[tidx].sourceFileName,
							sourceFileNameBuf,
							sizeof(transactionTrackers[tidx].sourceFileName));
					}
					if (destFileNameBuf && destFileNameBuf[0]
					&& !transactionTrackers[tidx].destFileName[0]) {
						istrcpy(transactionTrackers[tidx].destFileName,
							destFileNameBuf,
							sizeof(transactionTrackers[tidx].destFileName));
					}
					if (fileSize > 0) {
						transactionTrackers[tidx].fileSize = fileSize;
					}
				}
			}
			if (condition != CfdpNoError) {
				PUTS_FMT("Condition: %s (%d)", getConditionName(condition), condition);
			}
			break;

		case CfdpEofSentInd:              // Event 2
		case CfdpFileSegmentRecvInd:      // Event 5
		case CfdpEofRecvInd:              // Event 6
		case CfdpSuspendedInd:            // Event 7
		case CfdpResumedInd:              // Event 8
			/* For these events, only show condition if it indicates an error */
			if (condition != CfdpNoError) {
				PUTS_FMT("Condition: %s (%d)", getConditionName(condition), condition);
			}
			break;

		/* Deliver Code and File status are only applicable at the conclusion of a transaction */
		case CfdpTransactionFinishedInd:  // Event 3 - ALL FIELDS CRITICAL
		case CfdpFaultInd:                // Event 10 - ALL FIELDS CRITICAL
		case CfdpAbandonedInd:            // Event 11 - ALL FIELDS CRITICAL
			PUTS_FMT("Condition: %s (%d)", getConditionName(condition), condition);
			PUTS_FMT("Delivery: %s (%d)", getDeliveryCodeName(deliveryCode), deliveryCode);
			PUTS_FMT("File Status: %s (%d)", getFileStatusName(fileStatus), fileStatus);

			/* Add simple visual indicators for critical conditions */
			if (type == CfdpFaultInd) {
				PUTS(" FAULT DETECTED");
			} else if (type == CfdpAbandonedInd) {
				PUTS(" TRANSACTION ABANDONED");
			} else if (type == CfdpTransactionFinishedInd) {
				if (condition == CfdpNoError && deliveryCode == 0 && fileStatus == 2) {
					PUTS(" SUCCESS: File delivered and retained");
				} else if (condition == CfdpNoError && deliveryCode == 1 && fileStatus == 3) {
					PUTS(" COMPLETED: Unacknowledged mode (status unknown)");
				} else {
					PUTS(" Check condition codes above");
				}
			}
			fflush(stdout);
			break;

		case CfdpReportInd:             // Display report detail
			PUTS_FMT("Condition: %s (%d)", getConditionName(condition), condition);
			if (statusReportBuf && strlen(statusReportBuf) > 0) {
				PUTS_FMT("Report Details: %s", statusReportBuf);
			}
			break;

		default:
			/* Unknown event type - show condition if there's an error */
			if (condition != CfdpNoError) {
				PUTS_FMT("Condition: %s (%d)", getConditionName(condition), condition);
			}
			break;
	}

	/* Always show progress for meaningful events */
	PUTS_FMT("Progress: " UVAST_FIELDSPEC " bytes", progress);
	PUTS("==================");
	fflush(stdout);

	/* Transaction tracking - Finalize on completion */
	if (type == CfdpTransactionFinishedInd || type == CfdpAbandonedInd) {
		finalizeTransaction(transactionId, condition, deliveryCode, fileStatus);

		/* Show transaction summary if verbose mode is enabled */
		if (verboseMode) {
			PUTS("");
			printTransactionSummary(transactionId);
		}

		/* Clean up completed sender transactions to prevent memory buildup */
		cleanupCompletedTransaction(transactionId);
	}
}

static int	noteSegmentTime(uvast fileOffset, unsigned int recordOffset,
			unsigned int length, int sourceFileFd, char *buffer)
{
	/* Parameters intentionally unused. */
	(void)fileOffset;
	(void)recordOffset;
	(void)length;
	(void)sourceFileFd;

	writeTimestampLocal(getCtime(), buffer);
	return strlen(buffer) + 1;
}

static void	handleQuit(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	PUTS("cfdptest interrupted.");
	fflush(stdout);
}

static void	printUsage(void)
{
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tToggle verbose mode (shows transaction summary on completion)");
	PUTS("\t   v");
	PUTS("\ts\tShow summary of all tracked transactions, or specific transaction");
	PUTS("\t   s [sourceEntity.transactionNbr]");
	PUTS("\tw\tView all active transactions with state (from CFDP engine)");
	PUTS("\t   w");
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
	PUTS("\tg\tSet status reporting flags [PROTOTYPE - NOT YET FUNCTIONAL]");
	PUTS("\t   g <status report flag string>");
	PUTS("\t   NOTE: This feature is under development. While flags are");
	PUTS("\t   passed to bundles, CFDP does not yet process status reports.");
	PUTS("\t   Status report flag string is a sequence of status report");
	PUTS("\t   flags, separated by commas, with no embedded whitespace.");
	PUTS("\t   Each flag must be one of: rcv, ct, fwd, dlv, del.");
	PUTS("\tr\tAdd filestore request");
	PUTS("\t   r <action code nbr> <first path name> [second path name]");
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
	PUTS("\tR\tReset source and destination file names to NULL");
	PUTS("\t   R");
	PUTS("\tu\tAdd message to user");
	PUTS("\t   u '<message text>'");
	PUTS("\tx\tList local directory (supports wildcards)");
	PUTS("\t   x [directory_path_or_pattern]");
	PUTS("\tL <remote_dir> <local_file>\tList remote directory (ION extension).");
	PUTS("\tD <local_file>\t\tDisplay directory listing file (ION extension).");
	PUTS("\t&\tSend file per specified parameters");
	PUTS("\t   &");
	PUTS("\t^\tCancel a file transmission");
	PUTS("\t   ^ [sourceEntity.transactionNbr]");
	PUTS("\t   If no transaction ID is specified, cancels the last transaction.");
	PUTS("\t%\tSuspend a file transmission (outbound only)");
	PUTS("\t   % [sourceEntity.transactionNbr]");
	PUTS("\t   If no transaction ID is specified, suspends the last transaction.");
	PUTS("\t   Only works for locally-initiated transactions (suspend/resume).");
	PUTS("\t$\tResume a file transmission (outbound only)");
	PUTS("\t   $ [sourceEntity.transactionNbr]");
	PUTS("\t   If no transaction ID is specified, resumes the last transaction.");
	PUTS("\t   Only works for locally-initiated transactions (suspend/resume).");
	PUTS("\t#\tReport on a file transmission");
	PUTS("\t   # [sourceEntity.transactionNbr]");
	PUTS("\t   If no transaction ID is specified, reports on the last transaction.");
	PUTS("\t|\tGet file: send request for file per specified parameters");
	PUTS("\t   |");
	fflush(stdout);
}

static int      _echo(int *newValue)
{
	static int state = 0;

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
	fflush(stdout);
}

static void	setDestinationEntityNbr(int tokenCount, char **tokens,
			CfdpNumber *destinationEntityNbr)
{
	uvast	entityId;

	if (tokenCount != 2)
	{
		PUTS("Usage: d <destination entity number>");
		fflush(stdout);
		return;
	}

	entityId = getFqn(tokens[1]);
	cfdp_compress_number(destinationEntityNbr, entityId);
}

static void	setSourceFileName(int tokenCount, char **tokens,
			char *sourceFileNameBuf, char **sourceFileName)
{
	if (tokenCount != 2)
	{
		PUTS("Usage: f <source file name>");
		fflush(stdout);
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
		PUTS("Usage: t <destination file name>");
		fflush(stdout);
		return;
	}

	isprintf(destFileNameBuf, 256, "%.255s", tokens[1]);
	*destFileName = destFileNameBuf;
}

static int	hasGlobChars(const char *s)
{
	while (*s)
	{
		if (*s == '*' || *s == '?' || *s == '[')
		{
			return 1;
		}

		s++;
	}

	return 0;
}

static void	listLocalDirectory(int tokenCount, char **tokens)
{
	const char	*path;
	DIR		*dir;
	struct dirent	*entry;
	glob_t		results;
	int		rc;
	size_t		i;

	if (tokenCount > 2)
	{
		PUTS("Usage: x [directory_path_or_pattern]");
		fflush(stdout);
		return;
	}

	path = (tokenCount == 2) ? tokens[1] : ".";

	if (hasGlobChars(path))
	{
		rc = glob(path, GLOB_NOSORT, NULL, &results);
		if (rc == GLOB_NOMATCH)
		{
			PUTS_FMT("No matches for '%s'", path);
			return;
		}

		if (rc != 0)
		{
			PUTS_FMT("Glob error on '%s'", path);
			return;
		}

		PUTS_FMT("Matches for: %s", path);
		for (i = 0; i < results.gl_pathc; i++)
		{
			PUTS_FMT("  %s", results.gl_pathv[i]);
		}

		globfree(&results);
		return;
	}

	dir = opendir(path);
	if (dir == NULL)
	{
		PUTS_FMT("Can't open directory '%s': %s", path,
				strerror(errno));
		return;
	}

	PUTS_FMT("Directory: %s", path);
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0
		|| strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		PUTS_FMT("  %s", entry->d_name);
	}

	closedir(dir);
}

static void	resetFileNames(int tokenCount, char **tokens,
			char **sourceFileName, char **destFileName)
{
	/* Parameter intentionally unused. */
	(void)tokens;

	if (tokenCount != 1)
	{
		PUTS("Usage: R");
		fflush(stdout);
		return;
	}

	*sourceFileName = NULL;
	*destFileName = NULL;
	PUTS("Source and destination file names reset to NULL.");
	fflush(stdout);
}

static void	setClassOfService(int tokenCount, char **tokens,
			BpUtParms *utParms)
{
	uvast		parsed_priority;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the priority?");
		fflush(stdout);
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_priority) < 0
		|| parsed_priority > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid priority: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	utParms->classOfService = (unsigned long) parsed_priority;
}

static void	setOrdinal(int tokenCount, char **tokens, BpUtParms *utParms)
{
	uvast		parsed_ordinal;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the ordinal?");
		fflush(stdout);
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_ordinal) < 0
		|| parsed_ordinal > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid ordinal: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	utParms->ancillaryData.ordinal = (unsigned long) parsed_ordinal;
}

static void	setMode(int tokenCount, char **tokens, BpUtParms *utParms)
{
	unsigned long	mode;
	uvast		parsed_mode;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the mode?");
		fflush(stdout);
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_mode) < 0
		|| parsed_mode > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid mode: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	mode = (unsigned long) parsed_mode;

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
	uvast		parsed_interval;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("Usage: i <custody transfer retransmission interval in seconds>");
		fflush(stdout);
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_interval) < 0
		|| parsed_interval > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid interval: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	utParms->ctInterval = (unsigned long) parsed_interval;
}

static void	setClosure(int tokenCount, char **tokens, CfdpReqParms *parms)
{
	unsigned long	latency;
	uvast		parsed_latency;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the transaction closure latency?");
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_latency) < 0
	|| parsed_latency > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid closure latency: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	latency = (unsigned long) parsed_latency;
	parms->closureLatency = latency;
}

static void	setSegMd(int tokenCount, char **tokens, CfdpReqParms *parms)
{
	unsigned long	setting;
	uvast		parsed_setting;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the segment metadata mode?");
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_setting) < 0
	|| parsed_setting > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid segment metadata mode: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	setting = (unsigned long) parsed_setting;
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
	uvast		parsed_crit;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the criticality?");
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_crit) < 0
		|| parsed_crit > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid criticality: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	if (parsed_crit != 0)
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
	uvast		parsed_TTL;
	char		errMsg[256];

	if (tokenCount != 2)
	{
		PUTS("What's the TTL?");
		return;
	}

	if (platform_parse_uvast(tokens[1], &parsed_TTL) < 0
		|| parsed_TTL > ULONG_MAX)
	{
		isprintf(errMsg, sizeof(errMsg),
				"[?] Invalid TTL: %s", tokens[1]);
		printText(errMsg);
		return;
	}

	utParms->lifespan = (unsigned long) parsed_TTL;
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
	int		parsed_action;
	CfdpAction	action;
	char		*firstPathName = NULL;
	char		*secondPathName = NULL;
	char		errMsg[256];

	switch (tokenCount)
	{
		case 4:
			secondPathName = tokens[3];

			/* FALLTHROUGH */

		case 3:
			firstPathName = tokens[2];
			if (platform_parse_int(tokens[1], &parsed_action) < 0)
			{
				isprintf(errMsg, sizeof(errMsg),
						"[?] Invalid action code: %s",
						tokens[1]);
				printText(errMsg);
				return;
			}
			action = (CfdpAction) parsed_action;
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

#ifndef NO_DIRLIST
static void	requestDirListing(int tokenCount, char **tokens, CfdpReqParms *parms)
{
	CfdpDirListTask	task;

	if (tokenCount != 3)
	{
		PUTS("Syntax: L <remote_directory> <local_temp_file>");
		return;
	}

	task.directoryName = tokens[1];
	task.destFileName = tokens[2];

	if (cfdp_rls(&(parms->destinationEntityNbr),
			sizeof(BpUtParms),
			(unsigned char *) &(parms->utParms),
			NULL, NULL, NULL, NULL, 0, NULL, 0,
			parms->msgsToUser,
			parms->fsRequests,
			&task,
			&(parms->transactionId)) < 0)
	{
		putErrmsg("Can't request directory listing.", NULL);
		return;
	}

	PUTS_FMT("Directory listing requested for: %s -> %s", tokens[1], tokens[2]);
	parms->msgsToUser = 0;
	parms->fsRequests = 0;
}

static void	displayDirListing(const char *filename)
{
	int fd;
	char buffer[256];
	char entry[256];
	int pos = 0;
	int entryPos = 0;
	ssize_t bytesRead;
	int entryCount = 0;

	fd = iopen(filename, O_RDONLY, 0);
	if (fd < 0)
	{
		PUTS_FMT("Cannot open directory listing file: %s", filename);
		return;
	}

	PUTS_FMT("Directory contents from: %s", filename);
	PUTS("   Entries:");

	while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
	{
		for (pos = 0; pos < bytesRead; pos++)
		{
			if (buffer[pos] == '\0')
			{
				if (entryPos > 0)
				{
					entry[entryPos] = '\0';
					entryCount++;
					PUTS_FMT("   %3d. %s", entryCount, entry);
					entryPos = 0;
				}
			}
			else if (entryPos >= 0 && (size_t)entryPos < sizeof(entry) - 1)
			{
				entry[entryPos++] = buffer[pos];
			}
		}
	}

	if (entryCount == 0)
	{
		PUTS("   (empty directory)");
	}
	else
	{
		PUTS_FMT("   Total: %d entries", entryCount);
	}

	close(fd);
}
#endif

static int	processLine(char *line, int lineLength, CfdpReqParms *parms)
{
	int		tokenCount;
	char		*cursor;
	int		i;
	char		*tokens[9];
	CfdpProxyTask	task;

	/* Parameter intentionally unused. */
	(void)lineLength;

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

	while (isspace((unsigned char) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		PUTS("Too many tokens.\n");
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

		case 'v':
			/* Toggle verbose mode */
			verboseMode = !verboseMode;
			if (verboseMode) {
				PUTS("Verbose mode enabled - will show transaction summaries on completion");
			} else {
				PUTS("Verbose mode disabled");
			}
			fflush(stdout);
			return 0;

		case 's':
			/* Show all transactions summary or specific transaction */
			if (tokenCount == 1) {
				/* No arguments - show all transactions summary */
				printAllTransactionsSummary();
			} else if (tokenCount == 2) {
				/* Transaction ID provided - show detailed summary */
				CfdpTransactionId targetId;
				if (parseTransactionId(tokens[1], &targetId) < 0) {
					return 0;
				}
				printTransactionSummary(&targetId);
			} else {
				PUTS("Usage: s [sourceEntity.transactionNbr]");
				fflush(stdout);
			}
			return 0;

		case 'w':
			/* View active transactions from CFDP engine */
			listTransactions();
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

		case 'R':
			resetFileNames(tokenCount, tokens,
					&(parms->sourceFileName),
					&(parms->destFileName));
			return 0;

#ifndef NO_DIRLIST
		case 'L':
			requestDirListing(tokenCount, tokens, parms);
			return 0;

		case 'D':
			if (tokenCount != 2)
			{
				PUTS("What's the directory listing filename?");
				return 0;
			}
			displayDirListing(tokens[1]);
			return 0;
#endif

		case 'x':
			listLocalDirectory(tokenCount, tokens);
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

			/* Store closure latency for robust sender-side tracking */
			storeSenderTransaction(&(parms->transactionId), parms->closureLatency);

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

			/* Store closure info for robust sender-side tracking */
			storeSenderTransaction(&(parms->transactionId), task.closureRequested ? 1 : 0);

			parms->msgsToUser = 0;
			parms->fsRequests = 0;
			return 0;

		case '^':
			{
				CfdpTransactionId targetId;
				if (tokenCount == 1)
				{
					/* Use last transaction */
					memcpy(&targetId, &(parms->transactionId),
						sizeof(CfdpTransactionId));
				}
				else if (tokenCount == 2)
				{
					/* Parse provided transaction ID */
					if (parseTransactionId(tokens[1], &targetId) < 0)
					{
						return 0;
					}
				}
				else
				{
					PUTS("Usage: ^ [sourceEntity.transactionNbr]");
					fflush(stdout);
					return 0;
				}

				if (cfdp_cancel(&targetId) < 0)
				{
					putErrmsg("Can't cancel transaction.", NULL);
					return -1;
				}
				PUTS("Cancel request sent.");
				fflush(stdout);
			}
			return 0;

		case '%':
			{
				CfdpTransactionId targetId;
				if (tokenCount == 1)
				{
					/* Use last transaction */
					memcpy(&targetId, &(parms->transactionId),
						sizeof(CfdpTransactionId));
				}
				else if (tokenCount == 2)
				{
					/* Parse provided transaction ID */
					if (parseTransactionId(tokens[1], &targetId) < 0)
					{
						return 0;
					}
				}
				else
				{
					PUTS("Usage: % [sourceEntity.transactionNbr]");
					fflush(stdout);
					return 0;
				}

				/* Check if transaction is local */
				if (!isLocalTransaction(&targetId))
				{
					PUTS("Error: Suspend only works for locally-initiated transactions.");
					fflush(stdout);
					return 0;
				}

				if (cfdp_suspend(&targetId) < 0)
				{
					putErrmsg("Can't suspend transaction.", NULL);
					return -1;
				}
				PUTS("Suspend request sent.");
				fflush(stdout);
			}
			return 0;

		case '$':
			{
				CfdpTransactionId targetId;
				if (tokenCount == 1)
				{
					/* Use last transaction */
					memcpy(&targetId, &(parms->transactionId),
						sizeof(CfdpTransactionId));
				}
				else if (tokenCount == 2)
				{
					/* Parse provided transaction ID */
					if (parseTransactionId(tokens[1], &targetId) < 0)
					{
						return 0;
					}
				}
				else
				{
					PUTS("Usage: $ [sourceEntity.transactionNbr]");
					fflush(stdout);
					return 0;
				}

				/* Check if transaction is local */
				if (!isLocalTransaction(&targetId))
				{
					PUTS("Error: Resume only works for locally-initiated transactions.");
					fflush(stdout);
					return 0;
				}

				if (cfdp_resume(&targetId) < 0)
				{
					putErrmsg("Can't resume transaction.", NULL);
					return -1;
				}
				PUTS("Resume request sent.");
				fflush(stdout);
			}
			return 0;

		case '#':
			{
				CfdpTransactionId targetId;
				if (tokenCount == 1)
				{
					/* Use last transaction */
					memcpy(&targetId, &(parms->transactionId),
						sizeof(CfdpTransactionId));
				}
				else if (tokenCount == 2)
				{
					/* Parse provided transaction ID */
					if (parseTransactionId(tokens[1], &targetId) < 0)
					{
						return 0;
					}
				}
				else
				{
					PUTS("Usage: # [sourceEntity.transactionNbr]");
					fflush(stdout);
					return 0;
				}

				if (cfdp_report(&targetId) < 0)
				{
					putErrmsg("Can't report transaction.", NULL);
					return -1;
				}
				PUTS("Report request sent.");
				fflush(stdout);
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
	unsigned int		closureRequested;

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
				&closureRequested) < 0)
		{
			putErrmsg("Failed getting CFDP event.", NULL);
			return NULL;
		}

		reportCfdpEvent(type, statusReportBuf, condition, deliveryCode,
			fileStatus, progress, &transactionId, closureRequested,
			sourceFileNameBuf, destFileNameBuf, fileSize);

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
				PUTS_FMT("...Seg metadata '%s'", segMetadata);
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

			if (length >= 5 && strncmp((char *) usrmsgBuf, "cfdp", 4) == 0)
			{
				/* This is a CFDP proxy message */
				unsigned int msgType = usrmsgBuf[4];
				if (msgType == 17) /* Directory listing response */
				{
					unsigned char responseCode = usrmsgBuf[5];
					if (responseCode < 128)
					{
						PUTS_FMT("\tDirectory Listing: SUCCESS (code %d)", responseCode);
						PUTS_FMT("\t   Check destination file: %s", destFileNameBuf);
					}
					else
					{
						PUTS_FMT("\tDirectory Listing: FAILED (code %d)", responseCode);
					}
				}
				else
				{
					PUTS_FMT("\tCFDP Message: %s", getMessageText(usrmsgBuf, length));
				}
			}
			else
			{
				/* This is a regular user message - display the actual text */
				PUTS_FMT("\tUser Message: '%s'", (char *) usrmsgBuf);
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
				PUTS_FMT("\tFilestoreResp action=%d status=%d '%s' '%s' '%s'",
						action, status, firstPathName,
						secondPathName, msgBuf);
			}
		}

		printf(": ");
		fflush(stdout);
	}

	return NULL;
}

static int	runCfdptestInteractive(void)
{
	int		cmdFile;
	char		line[256];
	int		len;
	pthread_t	receiverThread;
	static int	running = 1;
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
	char		*cmdFileName = NULL;
	int		interactive = 1;
	int		i;

	/* Parse command-line arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verboseMode = 1;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			printf("Usage: cfdptest [-v|--verbose] [-h|--help] [script_file]\n");
			printf("  -v, --verbose   Enable verbose mode (show transaction summaries)\n");
			printf("  -h, --help      Show this help message\n");
			printf("  script_file     Run commands from script file instead of interactive mode\n");
			printf("\nEnvironment variables:\n");
			printf("  CFDP_EVENT_DETAIL=1   Enable verbose mode\n");
			return 0;
		} else {
			cmdFileName = argv[i];
			interactive = 0;
		}
	}

	/* Check environment variable for verbose mode */
	if (getenv("CFDP_EVENT_DETAIL") != NULL) {
		if (strcmp(getenv("CFDP_EVENT_DETAIL"), "1") == 0) {
			verboseMode = 1;
		}
	}
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
#ifndef NON_INTERACTIVE	/*	Need stdin/stdout for interactivity.	*/
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
		pthread_t	receiverThread;
		int		running = 1;

		/*	Start the receiver thread for script mode so event can be captured.	*/
		if (pthread_begin(&receiverThread, NULL, handleEvents, &running))
		{
			putSysErrmsg("cfdptest can't create receiver thread", NULL);
			ionDetach();
			return 1;
		}

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

			/*	Wait for events, then stop receiver thread	*/
			snooze(2);
			running = 0;
			cfdp_interrupt();
			pthread_join(receiverThread, NULL);
		}
	}

	writeErrmsgMemos();
	printText("Stopping cfdptest.");
	ionDetach();
	return retval;
}
