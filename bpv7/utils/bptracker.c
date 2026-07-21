#include "bpP.h"
#include "sdr.h"
#include <signal.h>
#include <pthread.h>

/*	Global debug level variable - follows ION pattern */
static int BPTRACKER_DEBUG = 0;

/*	Debug printing macro - follows ION pattern */
#define printDBG(level, ...) \
    do{ if(BPTRACKER_DEBUG >= level ) \
        fprintf(stderr, __VA_ARGS__); } while(0)

/*	Enhanced tracking with simple queue status */
typedef struct {
	SdrObject bundleObj;	    /* ION bundle object reference */
	SdrObject trackingElt;	    /* SDR list element for tracking */
	char   destEid[64];	    /* Destination endpoint ID */
	char   sourceEid[64];	    /* Source endpoint ID from bundle */
	time_t sendTime;	    /* When bundle was sent */
	int    bundleId;	    /* Sequential bundle identifier */
	enum {
		BUNDLE_PENDING,	    /* Awaiting transmission */
		BUNDLE_SUSPENDED,   /* Transmission suspended */
		BUNDLE_TRANSMITTED, /* Successfully transmitted, now detained */
		BUNDLE_COMPLETED    /* Manually released */
	} status;

	/* Bundle creation info */
	uvast creationTimeMsec; /* Bundle creation time (msec since epoch 2000) */
	unsigned int creationTimeCount; /* Bundle creation sequence count */

	/* Simple queue status - boolean flags */
	int inPlanQueue;     /* Boolean: in egress plan queue */
	int inDuctQueue;     /* Boolean: in outduct queue */
	int inFwdQueue;	     /* Boolean: in forwarding queue */
	int inDlvQueue;	     /* Boolean: in delivery queue */
	int isDetained;	     /* Boolean: bundle is detained */

	time_t transmitTime; /* When transmission completed */

	/* Bundle Protocol status report tracking */
	int    receivedReportCount;  /* Number of "received" reports */
	int    forwardedReportCount; /* Number of "forwarded" reports */
	int    deliveredReportCount; /* Number of "delivered" reports */
	int    deletedReportCount;   /* Number of "deleted" reports */
	time_t lastReportTime;	     /* Time of most recent status report */
	char   lastReportNode[64];   /* EID of node that sent last report */
	char   deletionReason[128];  /* If deleted, why */
} TrackedBundle;

typedef struct {
	BpSAP  sap;		 /* Bundle Protocol SAP for sending */
	BpSAP  reportSap;	 /* SAP for receiving status reports */
	SdrObject trackedBundles;	 /* SDR list of TrackedBundle objects */
	int    totalSent;	 /* Total bundles sent */
	int    totalTransmitted; /* Total bundles transmitted (not released) */
	int    totalCompleted;	 /* Total bundles manually released */

	/* Configuration */
	char sourceEid[64]; /* Source endpoint */
	char destEid[64]; /* Default destination (can be overridden per send) */
	char reportToEid[64];	       /* Where status reports are sent */
	unsigned char srrFlags;	       /* Status report request flags */
	int	      defaultTTL;      /* Default time-to-live */
	int	      defaultPriority; /* Default priority */

	/* Status report statistics */
	int totalReportsReceived;
	int totalBundlesWithReports;
} BundleTracker;

static volatile int g_quit = 0;

/*	Forward declarations	*/
static void initializeConfig(BundleTracker *tracker, char *sourceEid, char *destEid);
static void showConfig(BundleTracker *tracker);
static int cmdConfig(BundleTracker *tracker, char *args);
static int cmdSend(BundleTracker *tracker, char *args);

void signalHandler(int sig)
{
	printf("\nReceived signal %d. Shutting down...\n", sig);
	fflush(stdout);
	g_quit = 1;
}

/*	Helper functions for status reports	*/

static void initializeConfig(BundleTracker *tracker, char *sourceEid, char *destEid)
{
	memset(tracker->sourceEid, 0, sizeof(tracker->sourceEid));
	memset(tracker->destEid, 0, sizeof(tracker->destEid));
	memset(tracker->reportToEid, 0, sizeof(tracker->reportToEid));

	if (sourceEid)
	{
		istrcpy(tracker->sourceEid, sourceEid, sizeof(tracker->sourceEid));
	}

	if (destEid)
	{
		istrcpy(tracker->destEid, destEid, sizeof(tracker->destEid));
	}

	/* By default, reports come back to source endpoint */
	if (sourceEid)
	{
		istrcpy(tracker->reportToEid, sourceEid, sizeof(tracker->reportToEid));
	}

	tracker->srrFlags = 0;  /* No reports by default */
	tracker->defaultTTL = 300;  /* 5 minutes */
	tracker->defaultPriority = BP_STD_PRIORITY;
	tracker->totalReportsReceived = 0;
	tracker->totalBundlesWithReports = 0;
}

/*	Convert bundle status to string	*/
const char *statusToString(int status)
{
	switch (status) {
		case BUNDLE_PENDING:     return "PENDING";
		case BUNDLE_SUSPENDED:   return "SUSPENDED";
		case BUNDLE_TRANSMITTED: return "TRANSMITTED";
		case BUNDLE_COMPLETED:   return "COMPLETED";
		default:                 return "UNKNOWN";
	}
}

/*	Convert creation time to readable string	*/
void formatCreationTime(uvast msec, unsigned int count, char *buffer, size_t bufSize)
{
	/* ION epoch is January 1, 2000, 00:00:00 UTC */
	/* Convert ION time (milliseconds since 2000) to Unix time */
	time_t unixTime = (time_t)(msec / 1000) + 946684800;  /* Add seconds from 1970 to 2000 */

	struct tm *tm_info = gmtime(&unixTime);
	if (tm_info) {
		snprintf(buffer, bufSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d.%u",
			tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
			tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
			(int)(msec % 1000), count);
	} else {
		snprintf(buffer, bufSize, UVAST_FIELDSPEC ".%u", msec, count);
	}
}

/*	Simple queue status string - safe approach	*/
void getSimpleQueueStatus(TrackedBundle *tbundle, char *buffer, size_t bufSize)
{
	/* Build simple status using safe individual checks */
	buffer[0] = '\0';  /* Start with empty string */

	if (tbundle->inPlanQueue) {
		strncat(buffer, "PLAN ", bufSize - strlen(buffer) - 1);
	}
	if (tbundle->inDuctQueue) {
		strncat(buffer, "DUCT ", bufSize - strlen(buffer) - 1);
	}
	if (tbundle->inFwdQueue) {
		strncat(buffer, "FWD ", bufSize - strlen(buffer) - 1);
	}
	if (tbundle->inDlvQueue) {
		strncat(buffer, "DLV ", bufSize - strlen(buffer) - 1);
	}
	if (tbundle->isDetained) {
		strncat(buffer, "DETAINED ", bufSize - strlen(buffer) - 1);
	}

	/* If no queues, show CLEAR or RELEASED based on bundle status */
	if (strlen(buffer) == 0) {
		if (tbundle->status == BUNDLE_COMPLETED) {
			strncpy(buffer, "RELEASED", bufSize - 1);
		} else {
			strncpy(buffer, "CLEAR", bufSize - 1);
		}
		buffer[bufSize - 1] = '\0';
	}
}

/*	Suspend a specific bundle by ID	*/
int suspendBundle(BundleTracker *tracker, int bundleId)
{
	Sdr sdr = getIonsdr();
	SdrObject elt;
	SdrObject tbundleObj;
	TrackedBundle tbundle;
	int found = 0;

	printDBG(1, "Attempting to suspend bundle %d\n", bundleId);

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin suspend transaction\n");
		return -1;
	}

	for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt;
			elt = sdr_list_next(sdr, elt)) {
		tbundleObj = sdr_list_data(sdr, elt);

		if (tbundleObj == 0) continue;

		sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));

		if (tbundle.bundleId == bundleId) {
			found = 1;
			printDBG(2, "Found bundle %d for suspension\n", bundleId);

			if (tbundle.status == BUNDLE_COMPLETED) {
				printf("Bundle %d is already completed\n", bundleId);
			} else if (tbundle.status == BUNDLE_SUSPENDED) {
				printf("Bundle %d is already suspended\n", bundleId);
			} else if (tbundle.status == BUNDLE_TRANSMITTED) {
				printf("Bundle %d is already transmitted (cannot suspend)\n", bundleId);
			} else if (tbundle.bundleObj != 0) {
				printDBG(2, "Calling bp_suspend for bundle %d\n", bundleId);

				/* Use ION's built-in suspend function */
				if (bp_suspend(tbundle.bundleObj) == 0) {
					printf("Bundle %d suspended successfully\n", bundleId);
					tbundle.status = BUNDLE_SUSPENDED;
					sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
				} else {
					printf("Failed to suspend bundle %d\n", bundleId);
				}
			}
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't complete suspend transaction\n");
		return -1;
	}

	if (!found) {
		printf("Bundle %d not found\n", bundleId);
		return 0;
	}

	return 1;
}

/*	Resume a specific bundle by ID	*/
int resumeBundle(BundleTracker *tracker, int bundleId)
{
	Sdr sdr = getIonsdr();
	SdrObject elt;
	SdrObject tbundleObj;
	TrackedBundle tbundle;
	int found = 0;

	printDBG(1, "Attempting to resume bundle %d\n", bundleId);

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin resume transaction\n");
		return -1;
	}

	for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt;
			elt = sdr_list_next(sdr, elt)) {
		tbundleObj = sdr_list_data(sdr, elt);

		if (tbundleObj == 0) continue;

		sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));

		if (tbundle.bundleId == bundleId) {
			found = 1;
			printDBG(2, "Found bundle %d for resumption\n", bundleId);

			if (tbundle.status == BUNDLE_COMPLETED) {
				printf("Bundle %d is already completed\n", bundleId);
			} else if (tbundle.status == BUNDLE_TRANSMITTED) {
				printf("Bundle %d is already transmitted (cannot resume)\n", bundleId);
			} else if (tbundle.status != BUNDLE_SUSPENDED) {
				printf("Bundle %d is not suspended (status: %s)\n",
					bundleId, statusToString(tbundle.status));
			} else if (tbundle.bundleObj != 0) {
				printDBG(2, "Calling bp_resume for bundle %d\n", bundleId);

				/* Use ION's built-in resume function */
				if (bp_resume(tbundle.bundleObj) == 0) {
					printf("Bundle %d resumed successfully\n", bundleId);
					tbundle.status = BUNDLE_PENDING;
					sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
				} else {
					printf("Failed to resume bundle %d\n", bundleId);
				}
			}
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't complete resume transaction\n");
		return -1;
	}

	if (!found) {
		printf("Bundle %d not found\n", bundleId);
		return 0;
	}

	return 1;
}

/*	Release a specific bundle by ID	*/
int releaseBundle(BundleTracker *tracker, int bundleId)
{
	Sdr sdr = getIonsdr();
	SdrObject elt, nextElt;
	SdrObject tbundleObj;
	TrackedBundle tbundle;
	int found = 0;

	printDBG(1, "Attempting to release bundle %d\n", bundleId);

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin release transaction\n");
		return -1;
	}

	for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
		nextElt = sdr_list_next(sdr, elt);
		tbundleObj = sdr_list_data(sdr, elt);

		if (tbundleObj == 0) {
			sdr_list_delete(sdr, elt, NULL, NULL);
			continue;
		}

		sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));

		if (tbundle.bundleId == bundleId) {
			found = 1;
			printDBG(2, "Found bundle %d for release\n", bundleId);

			if (tbundle.status == BUNDLE_COMPLETED) {
				printf("Bundle %d is already completed/released\n", bundleId);
			} else if (tbundle.bundleObj != 0) {
				printf("Releasing bundle %d from detention\n", bundleId);
				fflush(stdout);

				bp_release(tbundle.bundleObj);

				tbundle.status = BUNDLE_COMPLETED;
				tracker->totalCompleted++;
				/* Clear queue status since bundle is no longer in ION */
				tbundle.inPlanQueue = 0;
				tbundle.inDuctQueue = 0;
				tbundle.inFwdQueue = 0;
				tbundle.inDlvQueue = 0;
				tbundle.isDetained = 0;
				sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));

				printDBG(2, "Bundle %d released successfully\n", bundleId);
			}
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't complete release transaction\n");
		return -1;
	}

	if (!found) {
		printf("Bundle %d not found\n", bundleId);
		return 0;
	}

	return 1;
}

/*	Release all bundles	*/
void releaseAllBundles(BundleTracker *tracker)
{
	Sdr sdr = getIonsdr();
	SdrObject elt, nextElt;
	SdrObject tbundleObj;
	TrackedBundle tbundle;
	int released = 0;

	printf("Releasing all detained bundles...\n");
	fflush(stdout);

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin release-all transaction\n");
		return;
	}

	for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
		nextElt = sdr_list_next(sdr, elt);
		tbundleObj = sdr_list_data(sdr, elt);

		if (tbundleObj != 0) {
			sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));

			if (tbundle.bundleObj != 0 && tbundle.status != BUNDLE_COMPLETED) {
				printf("Releasing bundle %d (%s)\n", tbundle.bundleId,
						statusToString(tbundle.status));
				fflush(stdout);

				bp_release(tbundle.bundleObj);
				tbundle.status = BUNDLE_COMPLETED;
				tracker->totalCompleted++;
				/* Clear queue status since bundle is no longer in ION */
				tbundle.inPlanQueue = 0;
				tbundle.inDuctQueue = 0;
				tbundle.inFwdQueue = 0;
				tbundle.inDlvQueue = 0;
				tbundle.isDetained = 0;
				sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
				released++;
			}
		}
	}

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't complete release-all transaction\n");
	}

	printf("Released %d bundles.\n", released);
	fflush(stdout);
}

/*	Create a payload of specified size	*/
SdrObject createPayload(int payloadSize)
{
	Sdr sdr = getIonsdr();
	SdrObject payloadObj;
	char *buffer;
	int i;

	printDBG(2, "Creating payload of size %d\n", payloadSize);

	if (payloadSize <= 0 || payloadSize > 65536) {
		printf("ERROR: Invalid payload size %d\n", payloadSize);
		return 0;
	}

	buffer = malloc(payloadSize);
	if (buffer == NULL) {
		printf("ERROR: Can't allocate payload buffer\n");
		return 0;
	}

	/* Fill buffer with test pattern */
	for (i = 0; i < payloadSize; i++) {
		buffer[i] = (char)(i % 256);
	}

	if (sdr_begin_xn(sdr) < 0) {
		free(buffer);
		printf("ERROR: Can't begin payload transaction\n");
		return 0;
	}

	payloadObj = sdr_malloc(sdr, payloadSize);
	if (payloadObj == 0) {
		sdr_cancel_xn(sdr);
		free(buffer);
		printf("ERROR: No SDR space for payload\n");
		return 0;
	}

	sdr_write(sdr, payloadObj, buffer, payloadSize);
	payloadObj = zco_create(sdr, ZcoSdrSource, payloadObj, 0, payloadSize, ZcoOutbound);

	if (sdr_end_xn(sdr) < 0 || payloadObj == 0) {
		free(buffer);
		printf("ERROR: Can't create payload ZCO\n");
		return 0;
	}

	free(buffer);
	printDBG(2, "Payload created successfully\n");
	return payloadObj;
}

/*	Send a bundle with enhanced tracking	*/
int sendTrackedBundle(BundleTracker *tracker, SdrObject adu, int lifespan,
		int bundleId)
{
	Sdr sdr = getIonsdr();
	SdrObject bundleObj;
	TrackedBundle tbundle;
	SdrObject tbundleObj;
	SdrObject trackingElt;
	Bundle bundle;
	char *sourceEidString = NULL;

	printDBG(1, "Sending bundle %d\n", bundleId);

	/* Send bundle with detention */
	if (bp_send(tracker->sap, tracker->destEid, NULL, lifespan, 0,
			NoCustodyRequested, 0, 0, NULL, adu, &bundleObj) != 1) {
		printf("ERROR: Failed to send bundle to %s\n", tracker->destEid);
		if (sdr_begin_xn(sdr) >= 0)
		{
			zco_destroy(sdr, adu);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy ZCO.", NULL);
			}
		}

		return -1;
	}

	printDBG(2, "Bundle %d sent successfully (obj: " ADDR_FIELDSPEC ")\n",
			bundleId, bundleObj);

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin tracking transaction\n");
		return -1;
	}

	/* Read the bundle to get source EID and creation time */
	printDBG(3, "Reading bundle %d to extract source info\n", bundleId);

	memset(&bundle, 0, sizeof(Bundle));
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

	/* Extract source EID string */
	readEid(&bundle.id.source, &sourceEidString);
	if (sourceEidString == NULL) {
		printf("WARNING: Can't read source EID for bundle %d\n", bundleId);
		sourceEidString = strdup("unknown");
	}

	printDBG(3, "Bundle %d source EID: %s, creation time: " UVAST_FIELDSPEC ".%u\n",
			bundleId, sourceEidString,
			bundle.id.creationTime.msec, bundle.id.creationTime.count);

	/* Create tracking bundle object */
	tbundleObj = sdr_malloc(sdr, sizeof(TrackedBundle));
	if (tbundleObj == 0) {
		sdr_cancel_xn(sdr);
		printf("ERROR: No space for tracked bundle\n");
		if (sourceEidString) MRELEASE(sourceEidString);
		bp_cancel(bundleObj);
		return -1;
	}

	printDBG(3, "Creating enhanced tracking record for bundle %d\n", bundleId);

	/* Initialize tracking data with source info and initial queue status */
	tbundle.bundleObj = bundleObj;
	tbundle.sendTime = time(NULL);
	tbundle.transmitTime = 0;  /* Not transmitted yet */
	tbundle.status = BUNDLE_PENDING;
	tbundle.bundleId = bundleId;

	/* Store destination EID */
	strncpy(tbundle.destEid, tracker->destEid, sizeof(tbundle.destEid) - 1);
	tbundle.destEid[sizeof(tbundle.destEid) - 1] = '\0';

	/* Store source EID */
	strncpy(tbundle.sourceEid, sourceEidString, sizeof(tbundle.sourceEid) - 1);
	tbundle.sourceEid[sizeof(tbundle.sourceEid) - 1] = '\0';

	/* Store creation time */
	tbundle.creationTimeMsec = bundle.id.creationTime.msec;
	tbundle.creationTimeCount = bundle.id.creationTime.count;

	/* Initialize simple queue status flags */
	tbundle.inPlanQueue = (bundle.planXmitElt != 0) ? 1 : 0;
	tbundle.inDuctQueue = (bundle.ductXmitElt != 0) ? 1 : 0;
	tbundle.inFwdQueue = (bundle.fwdQueueElt != 0) ? 1 : 0;
	tbundle.inDlvQueue = (bundle.dlvQueueElt != 0) ? 1 : 0;
	tbundle.isDetained = (bundle.detained != 0) ? 1 : 0;

	printDBG(3, "Initial queue status - Plan:%d Duct:%d Fwd:%d Dlv:%d Detained:%d\n",
			tbundle.inPlanQueue, tbundle.inDuctQueue, tbundle.inFwdQueue,
			tbundle.inDlvQueue, tbundle.isDetained);

	/* Clean up source EID string */
	if (sourceEidString) {
		MRELEASE(sourceEidString);
	}

	/* Add to tracking list */
	trackingElt = sdr_list_insert_last(sdr, tracker->trackedBundles, tbundleObj);
	if (trackingElt == 0) {
		sdr_cancel_xn(sdr);
		printf("ERROR: Can't add bundle to tracking list\n");
		sdr_free(sdr, tbundleObj);
		bp_cancel(bundleObj);
		return -1;
	}

	tbundle.trackingElt = trackingElt;
	sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));

	printDBG(3, "Registering bundle tracking with ION\n");

	/* Register tracking with ION */
	if (bp_track(bundleObj, trackingElt) < 0) {
		sdr_cancel_xn(sdr);
		printf("ERROR: Can't register bundle tracking\n");
		sdr_list_delete(sdr, trackingElt, NULL, NULL);
		sdr_free(sdr, tbundleObj);
		bp_cancel(bundleObj);
		return -1;
	}

	tracker->totalSent++;

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't complete tracked bundle send\n");
		return -1;
	}

	printDBG(1, "Bundle %d enhanced tracking completed\n", bundleId);

	return 0;
}

/*	Check status of all tracked bundles with queue status	*/
int checkBundleStatus(BundleTracker *tracker)
{
	Sdr sdr = getIonsdr();
	SdrObject elt, nextElt;
	SdrObject tbundleObj;
	TrackedBundle tbundle;
	Bundle bundle;
	int bundlesActive = 0;
	time_t currentTime = time(NULL);

	printDBG(2, "Checking bundle status (with queue status)...\n");

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin status check transaction\n");
		return -1;
	}

	for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt; elt = nextElt) {
		nextElt = sdr_list_next(sdr, elt);
		tbundleObj = sdr_list_data(sdr, elt);

		if (tbundleObj == 0) {
			printDBG(3, "Found null tracking object, removing\n");
			sdr_list_delete(sdr, elt, NULL, NULL);
			continue;
		}

		sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));

		printDBG(3, "Checking bundle %d (status=%s)\n",
				tbundle.bundleId, statusToString(tbundle.status));

		/* Skip already processed bundles */
		if (tbundle.status == BUNDLE_COMPLETED) {
			continue;
		}

		/* Skip suspended bundles - don't check transmission status */
		if (tbundle.status == BUNDLE_SUSPENDED) {
			bundlesActive++;
			continue;
		}

		/* Try to read bundle object to check if it still exists */
		if (tbundle.bundleObj == 0) {
			printDBG(3, "Bundle %d object is null - marking completed\n", tbundle.bundleId);
			tbundle.status = BUNDLE_COMPLETED;
			tracker->totalCompleted++;
			sdr_free(sdr, tbundleObj);
			sdr_list_delete(sdr, elt, NULL, NULL);
			continue;
		}

		printDBG(3, "Reading bundle %d object details\n", tbundle.bundleId);

		/* Read bundle to check current status */
		memset(&bundle, 0, sizeof(Bundle));
		sdr_read(sdr, (char *) &bundle, tbundle.bundleObj, sizeof(Bundle));

		printDBG(3, "Bundle %d object read successfully\n", tbundle.bundleId);

		/* Update simple queue status flags */
		int wasInAnyQueue = (tbundle.inPlanQueue || tbundle.inDuctQueue ||
				tbundle.inFwdQueue || tbundle.inDlvQueue);

		tbundle.inPlanQueue = (bundle.planXmitElt != 0) ? 1 : 0;
		tbundle.inDuctQueue = (bundle.ductXmitElt != 0) ? 1 : 0;
		tbundle.inFwdQueue = (bundle.fwdQueueElt != 0) ? 1 : 0;
		tbundle.inDlvQueue = (bundle.dlvQueueElt != 0) ? 1 : 0;
		tbundle.isDetained = (bundle.detained != 0) ? 1 : 0;

		int nowInAnyQueue = (tbundle.inPlanQueue || tbundle.inDuctQueue ||
				tbundle.inFwdQueue || tbundle.inDlvQueue);

		printDBG(3, "Bundle %d queue update - Plan:%d Duct:%d Fwd:%d Dlv:%d Detained:%d\n",
				tbundle.bundleId, tbundle.inPlanQueue, tbundle.inDuctQueue,
				tbundle.inFwdQueue, tbundle.inDlvQueue, tbundle.isDetained);

		/* Detect transmission completion */
		if (wasInAnyQueue && !nowInAnyQueue && tbundle.status == BUNDLE_PENDING) {
			printf("Bundle %d transmission completed (with queue status)\n", tbundle.bundleId);
			printf("\n> ");
			fflush(stdout);

			printDBG(2, "About to update bundle %d status to TRANSMITTED\n", tbundle.bundleId);

			tbundle.status = BUNDLE_TRANSMITTED;
			tbundle.transmitTime = currentTime;
			tracker->totalTransmitted++;

			printDBG(2, "Updating bundle %d tracking record\n", tbundle.bundleId);

			sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));

			printDBG(2, "Bundle %d status update completed\n", tbundle.bundleId);
		} else {
			/* Just update the queue status */
			printDBG(3, "Updating bundle %d queue status only\n", tbundle.bundleId);

			sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));
		}

		/* Count active bundles */
		if (tbundle.status == BUNDLE_PENDING || tbundle.status == BUNDLE_SUSPENDED ||
				tbundle.status == BUNDLE_TRANSMITTED) {
			bundlesActive++;
		}
	}

	printDBG(2, "About to end status check transaction\n");

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't end status check transaction\n");
		return -1;
	}

	printDBG(1, "Status check completed successfully, active bundles: %d\n", bundlesActive);

	return bundlesActive;
}

/*	List all tracked bundles with enhanced info and simple queue status	*/
void listActiveBundles(BundleTracker *tracker)
{
	Sdr sdr = getIonsdr();
	SdrObject elt;
	SdrObject tbundleObj;
	TrackedBundle tbundle;
	time_t currentTime = time(NULL);
	int totalCount = 0;
	char creationTimeStr[64];
	char queueStatusStr[32];

	printf("\n=== Tracked Bundles (With Queue Status) ===\n");
	printf("%-3s %-12s %-7s %-20s %-20s %-15s\n",
		"ID", "Status", "Age(s)", "Source_EID", "Creation_Time", "Queue_Status");
	printf("%-3s %-12s %-7s %-20s %-20s %-15s\n",
		"---", "------------", "-------", "--------------------", "--------------------", "---------------");

	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin list transaction\n");
		return;
	}

	for (elt = sdr_list_first(sdr, tracker->trackedBundles); elt;
			elt = sdr_list_next(sdr, elt)) {
		tbundleObj = sdr_list_data(sdr, elt);

		if (tbundleObj == 0) continue;

		sdr_read(sdr, (char *) &tbundle, tbundleObj, sizeof(TrackedBundle));
		totalCount++;

		/* Format creation time */
		formatCreationTime(tbundle.creationTimeMsec, tbundle.creationTimeCount,
				creationTimeStr, sizeof(creationTimeStr));

		/* Get simple queue status */
		getSimpleQueueStatus(&tbundle, queueStatusStr, sizeof(queueStatusStr));

		printf("%-3d %-12s %-7lld %-20s %-20s %-15s\n",
				tbundle.bundleId,
				statusToString(tbundle.status),
				(long long)(currentTime - tbundle.sendTime),
				tbundle.sourceEid,
				creationTimeStr,
				queueStatusStr);

		/* Show additional info for transmitted bundles */
		if (tbundle.status == BUNDLE_TRANSMITTED && tbundle.transmitTime > 0) {
			printf("    └─ Transmitted: %lld seconds ago, Object: " ADDR_FIELDSPEC "\n",
				(long long)(currentTime - tbundle.transmitTime), tbundle.bundleObj);
		} else if (tbundle.bundleObj != 0) {
			printf("    └─ Object: " ADDR_FIELDSPEC "\n", tbundle.bundleObj);
		}
	}

	sdr_exit_xn(sdr);

	if (totalCount == 0) {
		printf("No bundles tracked\n");
	}
	printf("=== Total: %d bundles ===\n\n", totalCount);
}

/*	Print current status	*/
void printStatus(BundleTracker *tracker)
{
	int active = checkBundleStatus(tracker);
	printf("\n=== Bundle Tracker Status (With Queue Status) ===\n");
	printf("Destination: %s\n", tracker->destEid);
	printf("Total sent: %d\n", tracker->totalSent);
	printf("Transmitted (detained): %d\n", tracker->totalTransmitted);
	printf("Manually released: %d\n", tracker->totalCompleted);
	printf("Still active: %d\n", active);
	printf("Debug level: %d\n", BPTRACKER_DEBUG);
	printf("==================================================\n\n");
}

void* inputHandler(void* arg)
{
	char input[100];
	int bundleId;
	BundleTracker *tracker = (BundleTracker*)arg;

	sleep(2);  /* Let main thread settle */

	while (!g_quit) {
		printf("> ");
		fflush(stdout);

		if (fgets(input, sizeof(input), stdin) == NULL) {
			if (g_quit) break;
			sleep(1);
			continue;
		}

		input[strcspn(input, "\n")] = 0;

		if (strcmp(input, "q") == 0 || strcmp(input, "quit") == 0) {
			printf("Quitting...\n");
			fflush(stdout);
			g_quit = 1;
			break;
		} else if (strncmp(input, "config", 6) == 0) {
			/* Handle config command */
			char *args = input + 6;
			while (*args == ' ') args++;  /* Skip spaces */
			cmdConfig(tracker, args);
		} else if (strncmp(input, "send ", 5) == 0) {
			/* Handle send command */
			char *args = input + 5;
			while (*args == ' ') args++;  /* Skip spaces */
			cmdSend(tracker, args);
		} else if (strncmp(input, "reports", 7) == 0) {
			/* Handle reports command - will implement in Phase 3 */
			printf("Reports command not yet implemented\n");
			fflush(stdout);
		} else if (strcmp(input, "s") == 0 || strcmp(input, "status") == 0) {
			printStatus(tracker);
		} else if (strcmp(input, "l") == 0 || strcmp(input, "list") == 0) {
			listActiveBundles(tracker);
		} else if (sscanf(input, "suspend %d", &bundleId) == 1 ||
			sscanf(input, "sp %d", &bundleId) == 1) {
			suspendBundle(tracker, bundleId);
		} else if (sscanf(input, "resume %d", &bundleId) == 1 ||
			sscanf(input, "res %d", &bundleId) == 1) {
			resumeBundle(tracker, bundleId);
		} else if (sscanf(input, "release %d", &bundleId) == 1 ||
			sscanf(input, "r %d", &bundleId) == 1) {
			releaseBundle(tracker, bundleId);
		} else if (strcmp(input, "release-all") == 0 || strcmp(input, "ra") == 0) {
			releaseAllBundles(tracker);
		} else if (strcmp(input, "h") == 0 || strcmp(input, "help") == 0) {
			printf("\nAvailable commands:\n");
			printf("  config [param] [value]   - Configure or show settings\n");
			printf("  send [<dest>] <size>     - Send bundle (dest optional, uses config default)\n");
			printf("  reports [bundle_id]      - Show status report statistics\n");
			printf("  q, quit                  - Quit program\n");
			printf("  s, status                - Show current status\n");
			printf("  l, list                  - List all bundles with detailed info\n");
			printf("  suspend <id>, sp <id>    - Suspend specific bundle by ID\n");
			printf("  resume <id>, res <id>    - Resume specific bundle by ID\n");
			printf("  release <id>, r <id>     - Release specific bundle from detention\n");
			printf("  release-all, ra          - Release all bundles from detention\n");
			printf("  h, help                  - Show this help\n\n");
			printf("Configuration parameters:\n");
			printf("  dest <eid>               - Set default destination\n");
			printf("  report <eid>             - Set report-to endpoint\n");
			printf("  srr <flags>              - Set status report flags\n");
			printf("                             (hex: 0f, or text: rcv,fwd,dlv,del)\n");
			printf("  ttl <seconds>            - Set default TTL\n");
			printf("  priority <0-2>           - Set priority\n\n");
			printf("Enhanced Bundle Information:\n");
			printf("  Source_EID      - Original source endpoint of bundle\n");
			printf("  Creation_Time   - When bundle was created (YYYY-MM-DD HH:MM:SS.mmm.count)\n");
			printf("  Queue_Status    - Current ION queue location:\n");
			printf("    PLAN          - In egress plan queue\n");
			printf("    DUCT          - In outduct transmission queue\n");
			printf("    FWD           - In forwarding queue\n");
			printf("    DLV           - In delivery queue\n");
			printf("    DETAINED      - Held in detention post-transmission\n");
			printf("    CLEAR         - Not in any queues (transmission complete)\n");
			printf("  Age(s)          - Seconds since we sent the bundle\n\n");
			printf("Bundle Status Meanings:\n");
			printf("  PENDING       - Bundle queued for transmission\n");
			printf("  SUSPENDED     - Bundle transmission suspended\n");
			printf("  TRANSMITTED   - Bundle transmitted by CLA, now detained\n");
			printf("  COMPLETED     - Bundle destroyed/completed\n\n");
			printf("Debug level: %d (-v flags control verbosity)\n\n", BPTRACKER_DEBUG);
			fflush(stdout);
		} else if (strlen(input) > 0) {
			printf("Unknown command: %s (type 'h' for help)\n", input);
			fflush(stdout);
		}
	}

	return NULL;
}

/*	Configuration command handlers	*/

static void showConfig(BundleTracker *tracker)
{
	printf("\n=== Configuration ===\n");
	printf("Source EID:       %s\n", tracker->sourceEid);
	printf("Destination EID:  %s\n", tracker->destEid[0] ? tracker->destEid : "(not set)");
	printf("Report-To EID:    %s\n", tracker->reportToEid);
	printf("SRR Flags:        0x%02x", tracker->srrFlags);
	if (tracker->srrFlags)
	{
		printf(" (");
		if (tracker->srrFlags & BP_RECEIVED_RPT) printf("RCV ");
		if (tracker->srrFlags & BP_FORWARDED_RPT) printf("FWD ");
		if (tracker->srrFlags & BP_DELIVERED_RPT) printf("DLV ");
		if (tracker->srrFlags & BP_DELETED_RPT) printf("DEL");
		printf(")");
	}
	printf("\n");
	printf("Default TTL:      %d seconds\n", tracker->defaultTTL);
	printf("Default Priority: %d", tracker->defaultPriority);
	switch (tracker->defaultPriority)
	{
		case BP_BULK_PRIORITY: printf(" (Bulk)"); break;
		case BP_STD_PRIORITY: printf(" (Standard)"); break;
		case BP_EXPEDITED_PRIORITY: printf(" (Expedited)"); break;
	}
	printf("\n");
	printf("=====================\n\n");
	fflush(stdout);
}

static int cmdConfig(BundleTracker *tracker, char *args)
{
	char param[64], value[256];
	int parsed;

	/* If no arguments, show current config */
	if (!args || strlen(args) == 0)
	{
		showConfig(tracker);
		return 0;
	}

	/* Parse parameter and value */
	parsed = sscanf(args, "%63s %255s", param, value);
	if (parsed < 2)
	{
		printf("Usage: config <param> <value>\n");
		printf("Type 'help' to see available parameters\n");
		fflush(stdout);
		return -1;
	}

	/* Handle each configuration parameter */
	if (strcmp(param, "dest") == 0)
	{
		istrcpy(tracker->destEid, value, sizeof(tracker->destEid));
		printf("Default destination set to: %s\n", tracker->destEid);
	}
	else if (strcmp(param, "report") == 0)
	{
		istrcpy(tracker->reportToEid, value, sizeof(tracker->reportToEid));
		printf("Report-to endpoint set to: %s\n", tracker->reportToEid);
	}
	else if (strcmp(param, "srr") == 0)
	{
		unsigned int flags = 0;
		int isHex = 0;

		/* Check if it's a hex number (starts with 0x or is all hex digits without text flag chars) */
		if (strncmp(value, "0x", 2) == 0 || strncmp(value, "0X", 2) == 0)
		{
			isHex = 1;
		}
		else
		{
			/* Check if it contains comma or known flag names - if so, it's text */
			if (strchr(value, ',') != NULL ||
				strstr(value, "rcv") != NULL ||
				strstr(value, "fwd") != NULL ||
				strstr(value, "dlv") != NULL ||
				strstr(value, "del") != NULL)
			{
				isHex = 0;
			}
			else
			{
				/* Try parsing as hex only if it doesn't contain flag keywords */
				isHex = 1;
			}
		}

		/* Try parsing as hex first if determined to be hex format */
		if (isHex && sscanf(value, "%x", &flags) == 1)
		{
			tracker->srrFlags = (unsigned char)flags;
			printf("Status report request flags set to: 0x%02x\n", tracker->srrFlags);
		}
		else
		{
			/* Parse as comma-separated text flags (like cfdptest) */
			char *cursor = value;
			char *comma;
			char flagCopy[256];

			strncpy(flagCopy, value, sizeof(flagCopy) - 1);
			flagCopy[sizeof(flagCopy) - 1] = '\0';
			cursor = flagCopy;

			while (1)
			{
				comma = strchr(cursor, ',');
				if (comma)
				{
					*comma = '\0';
				}

				/* Parse individual flag */
				if (strcmp(cursor, "rcv") == 0)
				{
					flags |= BP_RECEIVED_RPT;
				}
				else if (strcmp(cursor, "fwd") == 0)
				{
					flags |= BP_FORWARDED_RPT;
				}
				else if (strcmp(cursor, "dlv") == 0)
				{
					flags |= BP_DELIVERED_RPT;
				}
				else if (strcmp(cursor, "del") == 0)
				{
					flags |= BP_DELETED_RPT;
				}
				else if (strlen(cursor) > 0)
				{
					printf("Unknown flag: %s (valid: rcv, fwd, dlv, del)\n", cursor);
					fflush(stdout);
					return -1;
				}

				if (comma)
				{
					cursor = comma + 1;
					continue;
				}

				break;
			}

			tracker->srrFlags = (unsigned char)flags;
			printf("Status report request flags set to: 0x%02x", tracker->srrFlags);
			if (flags)
			{
				printf(" (");
				if (flags & BP_RECEIVED_RPT) printf("rcv ");
				if (flags & BP_FORWARDED_RPT) printf("fwd ");
				if (flags & BP_DELIVERED_RPT) printf("dlv ");
				if (flags & BP_DELETED_RPT) printf("del");
				printf(")");
			}
			printf("\n");
		}
	}
	else if (strcmp(param, "ttl") == 0)
	{
		int ttl;
		if (sscanf(value, "%d", &ttl) == 1 && ttl > 0)
		{
			tracker->defaultTTL = ttl;
			printf("Default TTL set to: %d seconds\n", tracker->defaultTTL);
		}
		else
		{
			printf("Invalid TTL value: %s (must be positive integer)\n", value);
			fflush(stdout);
			return -1;
		}
	}
	else if (strcmp(param, "priority") == 0)
	{
		int priority;
		if (sscanf(value, "%d", &priority) == 1 && priority >= 0 && priority <= 2)
		{
			tracker->defaultPriority = priority;
			printf("Default priority set to: %d\n", tracker->defaultPriority);
		}
		else
		{
			printf("Invalid priority: %s (must be 0-2)\n", value);
			fflush(stdout);
			return -1;
		}
	}
	else
	{
		printf("Unknown parameter: %s\n", param);
		printf("Valid parameters: dest, report, srr, ttl, priority\n");
		fflush(stdout);
		return -1;
	}

	fflush(stdout);
	return 0;
}

/*	Send command handler	*/

static int cmdSend(BundleTracker *tracker, char *args)
{
	char destEid[256];
	int payloadSize;
	SdrObject adu;
	int parsed;

	/* Try parsing with both destination and size first */
	parsed = sscanf(args, "%255s %d", destEid, &payloadSize);

	if (parsed == 1)
	{
		/* Only one argument - assume it's the payload size, use default destination */
		payloadSize = atoi(destEid);  /* destEid actually contains the size */

		if (tracker->destEid[0] == '\0')
		{
			printf("ERROR: No destination specified and no default destination configured\n");
			printf("Usage: send <dest_eid> <payload_size>  OR  send <payload_size> (uses config dest)\n");
			printf("Example: send ipn:2.1 1024  OR  config dest ipn:2.1; send 1024\n");
			fflush(stdout);
			return -1;
		}

		/* Use configured default destination */
		strncpy(destEid, tracker->destEid, sizeof(destEid) - 1);
		destEid[sizeof(destEid) - 1] = '\0';
	}
	else if (parsed != 2)
	{
		printf("Usage: send <dest_eid> <payload_size>  OR  send <payload_size> (uses config dest)\n");
		printf("Example: send ipn:2.1 1024  OR  config dest ipn:2.1; send 1024\n");
		fflush(stdout);
		return -1;
	}

	/* Validate payload size */
	if (payloadSize < 1 || payloadSize > 65536)
	{
		printf("ERROR: Payload size must be between 1 and 65536 bytes\n");
		fflush(stdout);
		return -1;
	}

	/* Create payload */
	adu = createPayload(payloadSize);
	if (adu == 0)
	{
		printf("ERROR: Failed to create payload\n");
		fflush(stdout);
		return -1;
	}

	/* Send bundle with status report flags */
	Sdr sdr = getIonsdr();
	SdrObject bundleObj;
	BpAncillaryData ancillaryData = { 0 };

	/* Set report-to EID if status reports are enabled */
	char *reportToEid = NULL;
	if (tracker->srrFlags != 0 && tracker->reportToEid[0] != '\0')
	{
		reportToEid = tracker->reportToEid;
	}

	printDBG(1, "Sending bundle to %s (size=%d, srr=0x%02x, reportTo=%s)\n",
			destEid, payloadSize, tracker->srrFlags,
			reportToEid ? reportToEid : "none");

	/* Send bundle */
	if (bp_send(tracker->sap, destEid, reportToEid, tracker->defaultTTL,
			tracker->defaultPriority, NoCustodyRequested, tracker->srrFlags,
			0, &ancillaryData, adu, &bundleObj) != 1)
	{
		printf("ERROR: Failed to send bundle to %s\n", destEid);
		fflush(stdout);
		if (sdr_begin_xn(sdr) >= 0)
		{
			zco_destroy(sdr, adu);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy ZCO.", NULL);
			}
		}

		return -1;
	}

	printDBG(2, "Bundle sent successfully (obj: " ADDR_FIELDSPEC ")\n", bundleObj);

	/* Track the bundle */
	if (sdr_begin_xn(sdr) < 0)
	{
		printf("ERROR: Can't begin tracking transaction\n");
		fflush(stdout);
		return -1;
	}

	/* Read the bundle to get source EID and creation time */
	Bundle bundle;
	char *sourceEidString = NULL;

	memset(&bundle, 0, sizeof(Bundle));
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));

	/* Extract source EID string */
	readEid(&bundle.id.source, &sourceEidString);
	if (sourceEidString == NULL)
	{
		sourceEidString = strdup("unknown");
	}

	printDBG(3, "Bundle source EID: %s, creation time: " UVAST_FIELDSPEC ".%u\n",
		sourceEidString, bundle.id.creationTime.msec, bundle.id.creationTime.count);

	/* Create tracking bundle object */
	SdrObject tbundleObj = sdr_malloc(sdr, sizeof(TrackedBundle));
	if (tbundleObj == 0)
	{
		sdr_cancel_xn(sdr);
		printf("ERROR: No space for tracked bundle\n");
		if (sourceEidString) MRELEASE(sourceEidString);
		bp_cancel(bundleObj);
		fflush(stdout);
		return -1;
	}

	/* Initialize tracking data */
	TrackedBundle tbundle;
	memset(&tbundle, 0, sizeof(TrackedBundle));

	tbundle.bundleObj = bundleObj;
	tbundle.sendTime = time(NULL);
	tbundle.transmitTime = 0;
	tbundle.status = BUNDLE_PENDING;
	tbundle.bundleId = ++tracker->totalSent;

	/* Store destination EID */
	strncpy(tbundle.destEid, destEid, sizeof(tbundle.destEid) - 1);
	tbundle.destEid[sizeof(tbundle.destEid) - 1] = '\0';

	/* Store source EID */
	strncpy(tbundle.sourceEid, sourceEidString, sizeof(tbundle.sourceEid) - 1);
	tbundle.sourceEid[sizeof(tbundle.sourceEid) - 1] = '\0';

	/* Store creation time */
	tbundle.creationTimeMsec = bundle.id.creationTime.msec;
	tbundle.creationTimeCount = bundle.id.creationTime.count;

	/* Initialize queue status flags */
	tbundle.inPlanQueue = (bundle.planXmitElt != 0) ? 1 : 0;
	tbundle.inDuctQueue = (bundle.ductXmitElt != 0) ? 1 : 0;
	tbundle.inFwdQueue = (bundle.fwdQueueElt != 0) ? 1 : 0;
	tbundle.inDlvQueue = (bundle.dlvQueueElt != 0) ? 1 : 0;
	tbundle.isDetained = (bundle.detained != 0) ? 1 : 0;

	/* Initialize status report tracking fields */
	tbundle.receivedReportCount = 0;
	tbundle.forwardedReportCount = 0;
	tbundle.deliveredReportCount = 0;
	tbundle.deletedReportCount = 0;
	tbundle.lastReportTime = 0;
	memset(tbundle.lastReportNode, 0, sizeof(tbundle.lastReportNode));
	memset(tbundle.deletionReason, 0, sizeof(tbundle.deletionReason));

	/* Write tracking data to SDR */
	sdr_write(sdr, tbundleObj, (char *) &tbundle, sizeof(TrackedBundle));

	/* Clean up source EID string */
	if (sourceEidString)
	{
		MRELEASE(sourceEidString);
	}

	/* Add to tracking list */
	SdrObject trackingElt = sdr_list_insert_last(sdr, tracker->trackedBundles, tbundleObj);
	if (trackingElt == 0)
	{
		sdr_cancel_xn(sdr);
		printf("ERROR: Can't add bundle to tracking list\n");
		sdr_free(sdr, tbundleObj);
		bp_cancel(bundleObj);
		fflush(stdout);
		return -1;
	}

	/* Commit transaction */
	if (sdr_end_xn(sdr) < 0)
	{
		printf("ERROR: Can't complete tracking\n");
		fflush(stdout);
		return -1;
	}

	printf("Bundle #%d sent to %s (%d bytes)\n", tbundle.bundleId, destEid, payloadSize);
	if (tracker->srrFlags != 0)
	{
		printf("  Status reports requested: 0x%02x", tracker->srrFlags);
		if (tracker->srrFlags & BP_RECEIVED_RPT) printf(" RCV");
		if (tracker->srrFlags & BP_FORWARDED_RPT) printf(" FWD");
		if (tracker->srrFlags & BP_DELIVERED_RPT) printf(" DLV");
		if (tracker->srrFlags & BP_DELETED_RPT) printf(" DEL");
		printf("\n");
	}
	fflush(stdout);

	return 0;
}

/*	Print usage information	*/
void printUsage(char *progName)
{
	printf("Usage: %s [-v] [-vv] [-vvv] <source_eid> [dest_eid]\n", progName);
	printf("\nOptions:\n");
	printf("  -v        Verbose output (debug level 1)\n");
	printf("  -vv       More verbose output (debug level 2)\n");
	printf("  -vvv      Most verbose output (debug level 3)\n");
	printf("  -h        Show this help\n");
	printf("\nArguments:\n");
	printf("  source_eid    Source endpoint identifier (required)\n");
	printf("  dest_eid      Default destination endpoint (optional)\n");
	printf("\nInteractive Commands:\n");
	printf("  config [param] [value]  Configure parameters or show current config\n");
	printf("    dest <eid>            Set default destination endpoint\n");
	printf("    report <eid>          Set report-to endpoint\n");
	printf("    srr <flags>           Set status report request flags (hex)\n");
	printf("    ttl <seconds>         Set default time-to-live\n");
	printf("    priority <0-2>        Set priority (0=bulk, 1=standard, 2=expedited)\n");
	printf("  send [<dest>] <size>    Send bundle (dest optional, overrides config default)\n");
	printf("  list                    List all tracked bundles\n");
	printf("  reports [bundle_id]     Show status report statistics\n");
	printf("  quit                    Exit the program\n");
	printf("  help                    Show this help\n");
	printf("\nDebug Levels:\n");
	printf("  0 = No debug output (default)\n");
	printf("  1 = Basic operations and status changes\n");
	printf("  2 = Detailed function calls and tracking\n");
	printf("  3 = Verbose internal state and queue details\n");
	printf("\nExample:\n");
	printf("  %s -vv ipn:1.1 ipn:2.1\n", progName);
	printf("  Then use 'send ipn:2.1 1024' to send a bundle\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	BundleTracker tracker;
	char *sourceEid = NULL;
	char *destEid = NULL;
	int argIndex = 1;
	pthread_t inputThread;

	/* Parse command line options */
	while (argIndex < argc && argv[argIndex][0] == '-') {
		if (strcmp(argv[argIndex], "-h") == 0 || strcmp(argv[argIndex], "--help") == 0) {
			printUsage(argv[0]);
			return 0;
		} else if (argv[argIndex][1] == 'v') {
			/* Count 'v' characters to set debug level */
			for (int j = 1; argv[argIndex][j] == 'v'; j++) {
				BPTRACKER_DEBUG++;
			}
			argIndex++;
		} else {
			printf("Unknown option: %s\n", argv[argIndex]);
			printUsage(argv[0]);
			return 1;
		}
	}

	/* Check remaining arguments - source_eid required, dest_eid optional */
	int remainingArgs = argc - argIndex;
	if (remainingArgs < 1 || remainingArgs > 2) {
		printf("ERROR: Wrong number of arguments\n");
		printUsage(argv[0]);
		return 1;
	}

	sourceEid = argv[argIndex];
	if (remainingArgs == 2) {
		destEid = argv[argIndex + 1];
	}

	printf("Interactive Bundle Tracker Starting...\n");
	printf("Source: %s\n", sourceEid);
	if (destEid) {
		printf("Default Destination: %s\n", destEid);
	}
	printf("Debug level: %d\n", BPTRACKER_DEBUG);
	printf("Type 'help' for available commands\n");
	fflush(stdout);

	/* Initialize Bundle Protocol */
	printDBG(1, "Attaching to Bundle Protocol\n");

	if (bpAttach() < 0) {
		printf("ERROR: Can't attach to Bundle Protocol\n");
		return 1;
	}

	/* Set up signal handlers */
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	/* Open SAP with detention */
	printDBG(1, "Opening SAP with detention enabled\n");

	if (bp_open_source(sourceEid, &(tracker.sap), 1) < 0) {
		printf("ERROR: Can't open source endpoint with detention\n");
		bpDetach();
		return 1;
	}

	/* Initialize tracker */
	Sdr sdr = getIonsdr();
	if (sdr_begin_xn(sdr) < 0) {
		printf("ERROR: Can't begin tracker init transaction\n");
		bp_close(tracker.sap);
		bpDetach();
		return 1;
	}

	tracker.trackedBundles = sdr_list_create(sdr);
	if (tracker.trackedBundles == 0) {
		sdr_cancel_xn(sdr);
		printf("ERROR: Can't create tracked bundles list\n");
		bp_close(tracker.sap);
		bpDetach();
		return 1;
	}

	tracker.totalSent = 0;
	tracker.totalTransmitted = 0;
	tracker.totalCompleted = 0;

	/* Initialize configuration */
	initializeConfig(&tracker, sourceEid, destEid);

	if (sdr_end_xn(sdr) < 0) {
		printf("ERROR: Can't complete tracker initialization\n");
		bp_close(tracker.sap);
		bpDetach();
		return 1;
	}

	/* Start input handler thread */
	printDBG(1, "Starting input thread\n");

	if (pthread_create(&inputThread, NULL, inputHandler, &tracker) != 0) {
		printf("ERROR: Can't create input thread\n");
		g_quit = 1;
	}

	/* Monitor bundle status */
	printDBG(1, "Entering monitoring loop\n");

	printf("\nbptracker> ");
	fflush(stdout);

	while (!g_quit) {
		int result = checkBundleStatus(&tracker);
		if (result < 0) {
			printf("ERROR: Status check failed, continuing...\n");
			fflush(stdout);
		}
		sleep(5);  /* Check every 5 seconds */
	}

	/* Cleanup */
	printDBG(1, "Cleaning up\n");

	if (!g_quit) g_quit = 1;
	pthread_cancel(inputThread);
	pthread_join(inputThread, NULL);

	bp_close(tracker.sap);
	bpDetach();

	printDBG(1, "Program completed\n");
	return 0;
}
