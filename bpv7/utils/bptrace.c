/*
	bptrace.c:	network trace utility, using BP status reports.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
/*	Updated to intercept reports and display report directly
	to terminal by Silas Springer	March 24, 2023	*/
/*	Updated to handle CRS (Compressed Reporting Signal) admin
	records in addition to traditional status reports.	*/
#define _GNU_SOURCE
#include <bpP.h>
#include "cbr.h"	/* For CBR_ADMIN_RECORD_CRS, cbr_decodeBundleSequence */

#if defined (ION_LWT)

#define BUNDLE_NOT_SENT 0

#else

#define BUNDLE_NOT_SENT -1

static int BPTRACE_DEBUG = 0;
#define printDBG(level, ...) \
	do{ if(BPTRACE_DEBUG >= level ) \
		fprintf(stderr, __VA_ARGS__); } while(0)

// static int DEBUG = 0;

typedef struct
{
	BpSAP sap;
	int running;
} BptestState;

static BptestState *_bptestState(BptestState *newState)
{
	void *value;
	BptestState *state;

	if(newState){
		value = (void*) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else
	{
		state = (BptestState *) sm_TaskVar(NULL);
	}
	return state;
}

typedef struct {
	char* sourceEid;
	uvast creationTime;
	unsigned creationCount;
	unsigned fragmentOffset;
	int statusFlags;
	uvast statusTime;
	char* bundleSourceEid;
	char* reasonString;
} statusReport;

static unsigned n_rpts = 0;
static statusReport **reports = NULL;

static BptestState state = { NULL, 1 };

/*	Set from the signal handler to request a clean stop.  Writing a
 *	volatile sig_atomic_t is async-signal-safe; the receive loops poll
 *	it at a transaction boundary and unwind normally (releasing the SDR
 *	transaction lock and any nested PSM locks) instead of tearing the
 *	process down from signal context while a transaction is open.	*/
static volatile sig_atomic_t stopRequested = 0;

const size_t datelen = 32;//strlen("YYYY-MM-DDThh:mm:ss.sss")+1 + 8; // 8 is purely for safety

char* dtnTimeToDate(uvast time){
	char* buffer = malloc(datelen);
	uvast time_sec = time/1000;
	double ms = (((double)time)/1000 - time_sec) * 1000;
	time_t t = time_sec + EPOCH_2000_SEC;
	struct tm *epoch_time = localtime(&t);
	if (epoch_time == NULL)
	{
		snprintf(buffer, datelen, "0000-00-00T00:00:00.000");
		return buffer;
	}
	strftime(buffer, datelen, "%Y-%m-%dT%H:%M:%S", epoch_time);
	int current_len = strlen(buffer);
	isprintf(buffer + current_len, datelen - current_len, ".%03.f", ms);
	return buffer;
	}

char* statusToString(int statusFlags, char* buf, unsigned buflen){
	char* buffer = malloc(32);
	if (buffer == NULL)
	{
		if (buflen > 0)
		{
			buf[0] = '\0';
		}

		return buf;
	}

	strcpy(buffer, "");
	// note that BP_CUSTODY_RPT is ignored here, as that is not appliccable in bpv7
	if(statusFlags & BP_RECEIVED_RPT)
		strcat(buffer, strlen(buffer) != 0 ? ", rcv" : "rcv");
	if(statusFlags & BP_FORWARDED_RPT)
		strcat(buffer, strlen(buffer) != 0 ? ", fwd" : "fwd");
	if(statusFlags & BP_DELIVERED_RPT)
		strcat(buffer, strlen(buffer) != 0 ? ", dlv" : "dlv");
	if(statusFlags & BP_DELETED_RPT)
		strcat(buffer, strlen(buffer) != 0 ? ", del" : "del");

	strncpy(buf, buffer, buflen);
	buf[buflen <= strlen(buffer) ? buflen : strlen(buffer)] = '\0';
	free(buffer);
	return buf;
}

void print(statusReport *rpt){
	char* tmbuffer=NULL;
	tmbuffer = dtnTimeToDate(rpt->statusTime);
	if (rpt->creationCount > 0)
		printf("%u/%u ", rpt->creationCount, rpt->fragmentOffset);
	char* buffer = malloc(32);
	printf("%8s at %s on %s, '%s'.\n",
		statusToString(rpt->statusFlags, buffer, 32), tmbuffer, rpt->bundleSourceEid,
		rpt->reasonString);
	free(buffer);
	printDBG(3, "statusTime: " UVAST_FIELDSPEC "<=> %s\n", rpt->statusTime, tmbuffer);
	free(tmbuffer);
}

void sortByStatusTime(statusReport *rpts[], unsigned reportCount){
	for(unsigned i = 0; i < reportCount-1; ++i){
		for(unsigned j = 0; j < (reportCount-i)-1; ++j){
			printDBG(3, "j: %u, J+1: %u, reportCount: %u\n", j, j+1, reportCount);

			printDBG(3, UVAST_FIELDSPEC"\n", rpts[j]->statusTime);
			printDBG(3, UVAST_FIELDSPEC"\n", rpts[j+1]->statusTime);
			if(rpts[j]->statusTime  > rpts[j+1]->statusTime){
				statusReport *tmp = rpts[j];
				rpts[j] = rpts[j+1];
				rpts[j+1] = tmp;
			} else if(rpts[j]->statusTime == rpts[j+1]->statusTime) {
				char *dot = strrchr(rpts[j+1]->sourceEid, '.');
				if (dot != NULL &&
					strncmp(rpts[j+1]->sourceEid,
						rpts[j+1]->bundleSourceEid,
						dot - rpts[j+1]->sourceEid) == 0)
				{
					statusReport *tmp = rpts[j];
					rpts[j] = rpts[j+1];
					rpts[j+1] = tmp;
				}
			}
		}
	}
}
void freeStatusReport(statusReport *rpt)
{
	if (rpt == NULL) {
		return;
	}
	// Free any fields allocated by strdup
	if (rpt->sourceEid)       free(rpt->sourceEid);
	if (rpt->bundleSourceEid) free(rpt->bundleSourceEid);
	if (rpt->reasonString)    free(rpt->reasonString);

	// Now free the structure itself (since handleStatusRpt did `malloc(sizeof(statusReport))`)
	free(rpt);
}

// const char* header_rpt = "srcEid/creationTime:count/offset 'status' # 'at' time 'on' statusEid, statusMsg\n";
void print_reports(void) {
	if (reports && n_rpts > 0) {
		printf("\nDone, printing in time order: \n");
		printf("------------------------------\n");
		sortByStatusTime(reports, n_rpts);
		for (unsigned i = 0; i < n_rpts; ++i) {
			print(reports[i]);
			freeStatusReport(reports[i]); // <---- Instead of just free(reports[i]);
		}
	}
}


void sighandler(int signum) {
	/*	Async-signal-safe: only request a stop.  The receive loops
	 *	poll stopRequested at a transaction boundary and then unwind
	 *	through their normal cleanup (print_reports/bp_close/bp_detach),
	 *	so no SDR transaction lock or nested PSM lock is ever orphaned
	 *	by tearing the process down mid-transaction from here.	*/
	(void) signum;
	stopRequested = 1;
}
#endif


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

static int	run_bptrace(char *ownEid, char *destEid, char *reportToEid,
			int ttl, char *svcClass, char *trace, char *flags,
			uvast seqId)
{
	int		priority = 0;
	BpAncillaryData	ancillaryData = {0};
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	int		srrFlags = 0;
	BpSAP		sap;
	Sdr		sdr;
	SdrObject	newBundle;

	if (!bp_parse_quality_of_service(svcClass, &ancillaryData,
			&custodySwitch, &priority))
	{
		putErrmsg("Invalid class of service for bptrace.", svcClass);
		return 0;
	}

	ancillaryData.cbrSeqId = seqId;

	if (flags)
	{
		setFlags(&srrFlags, flags);
	}

	if (bp_attach() < 0)
	{
		putErrmsg("bptrace can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("bptrace can't open own endpoint.", ownEid);
		return 0;
	}

	if (*trace == '@')
	{
		SdrObject	fileRef;
		struct stat	statbuf;
		int		aduLength;
		SdrObject	traceZco;
		char        *fileName;

		fileName = trace + 1;
		if (stat(fileName, &statbuf) < 0)
		{
			bp_close(sap);
			putSysErrmsg("Can't stat the file", fileName);
			return 0;
		}

		aduLength = statbuf.st_size;
		sdr = bp_get_sdr();
		CHKZERO(sdr_begin_xn(sdr));
		fileRef = zco_create_file_ref(sdr, fileName, NULL, ZcoOutbound);
		if (sdr_end_xn(sdr) < 0 || fileRef == 0)
		{
			bp_close(sap);
			putErrmsg("bptrace can't create file ref.", fileName);
			return 0;
		}

		traceZco = ionCreateZco(ZcoFileSource, fileRef, 0, aduLength,
			priority, ancillaryData.ordinal, ZcoOutbound, NULL);
		if (traceZco == 0)
		{
				putErrmsg("bptrace can't create ZCO.", fileName);
		}
		else
		{
			if (bp_send(sap, destEid, reportToEid, ttl, priority,
				custodySwitch, srrFlags, 0, &ancillaryData,
				traceZco, &newBundle) <= 0)
			{
			putErrmsg("bptrace can't send file in bundle.",
					fileName);
			fprintf(stderr,
				"bptrace: failed to send '%s'.\n", fileName);
			CHKZERO(sdr_begin_xn(sdr));
			zco_destroy(sdr, traceZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy ZCO.", NULL);
			}
			}
		}

		CHKZERO(sdr_begin_xn(sdr));
		zco_destroy_file_ref(sdr, fileRef);
		if (sdr_end_xn(sdr) < 0)
		{
				putErrmsg("bptrace can't destroy file reference.", NULL);
		}
	}
	else
	{
		int		msgLength = strlen(trace) + 1;
		SdrObject	msg;
		SdrObject	traceZco;

		sdr = bp_get_sdr();
		CHKZERO(sdr_begin_xn(sdr));
		msg = sdr_malloc(sdr, msgLength);
		if (msg)
		{
			sdr_write(sdr, msg, trace, msgLength);
		}

		if (sdr_end_xn(sdr) < 0)
		{
				bp_close(sap);
				putErrmsg("No space for bptrace text.", NULL);
				return 0;
		}

		traceZco = ionCreateZco(ZcoSdrSource, msg, 0, msgLength, priority,
			ancillaryData.ordinal, ZcoOutbound, NULL);
		if (traceZco == 0 || traceZco == (SdrObject) ERROR)
		{
				putErrmsg("bptrace can't create ZCO", NULL);
		}
		else
		{
			if (bp_send(sap, destEid, reportToEid, ttl, priority,
			custodySwitch, srrFlags, 0, &ancillaryData,
			traceZco, &newBundle) <= 0)
			{
				putErrmsg("bptrace can't send message.", NULL);
				fprintf(stderr,
					"bptrace: failed to send bundle.\n");
				CHKZERO(sdr_begin_xn(sdr));
				zco_destroy(sdr, traceZco);
				if (sdr_end_xn(sdr) < 0)
				{
					putErrmsg("Can't destroy ZCO.", NULL);
				}
			}
		}
	}

	bp_close(sap);
	bp_detach();
	return 0;
}


#if defined (ION_LWT)
int	bptrace(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ownEid = (char *) a1;
	char	*destEid = (char *) a2;
	char	*traceEid = (char *) a3;
	int	ttl = a4 ? strtol((char *) a4, NULL, 0) : 0;
	char	*classOfService = (char *) a5;
	char	*trace = (char *) a6;
	char	*flagString = (char *) a7;
	uvast	seqId = a8 ? strtouvast((char *) a8) : 0;

	if (ownEid == NULL || destEid == NULL || classOfService == NULL
	|| trace == NULL)
	{
		PUTS("Missing argument(s) for bptrace.  Ignored.");
		return 0;
	}
	return run_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString, seqId);
}
#else


static int	handleStatusRpt(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes, statusReport *report)
{
	int		bundleDeath = 0;
	BpStatusRpt	rpt;
	char		*sourceEid;
	uvast		statusTime = 0;
	char		*reasonString;

	if (parseStatusRpt(&rpt, cursor, unparsedBytes) < 1)
	{
		printf("unable to parse status report\n with %u unsigned bytes\n", unparsedBytes);
		return -1;
	}

	readEid(&rpt.sourceEid, &sourceEid);
	if (sourceEid == NULL)
	{
		eraseEid(&rpt.sourceEid);
		return -1;
	}

	if (rpt.flags & BP_DELETED_RPT)
	{
		statusTime = rpt.deletionTime;
		bundleDeath = 0;
		switch (rpt.reasonCode)
		{
		case SrLifetimeExpired:
			reasonString = "TTL expired";
			break;

		case SrUnidirectionalLink:
			reasonString = "one-way link";
			break;

		case SrCanceled:
			reasonString = "canceled";
			break;

		case SrDepletedStorage:
			reasonString = "out of space";
			break;

		case SrDestinationUnintelligible:
			reasonString = "bad destination";
			break;

		case SrNoKnownRoute:
			reasonString = "no route to destination";
			break;

		case SrNoTimelyContact:
			reasonString = "would expire before contact";
			break;

		case SrBlockUnintelligible:
			reasonString = "bad block";
			break;

		case SrHopCountExceeded:
			reasonString = "hop limit exceeded";
			break;

		case SrTrafficPared:
			reasonString = "bundle discarded";
			break;

		case SrBlockUnsupported:
			reasonString = "block not supported";
			break;

		case SrMissingSecurityService:
			reasonString = "missing security service";
			break;

		case SrUnknownSecurityService:
			reasonString = "unknown security service";
			break;

		case SrUnexpectedSecurityService:
			reasonString = "unexpected security service";
			break;

		case SrFailedSecurityService:
			reasonString = "failed security service";
			break;

		case SrConflictingSecurityServices:
			reasonString = "conflicting security services";
			break;

		default:
			reasonString = "(unknown)";
		}
	}
	else
	{
		reasonString = "okay";
		if (rpt.flags & BP_RECEIVED_RPT)
		{
			bundleDeath = 1;
			statusTime = rpt.receiptTime;
		}

		if (rpt.flags & BP_FORWARDED_RPT)
		{
			statusTime = rpt.forwardTime;
		}

		if (rpt.flags & BP_DELIVERED_RPT)
		{
			statusTime = rpt.deliveryTime;
		}
	}

	// TODO: update to release reports with a different creationTime or sourceEid
	// need to get 'our' bundle's creationTime to do this first though.

	report->sourceEid = strdup(sourceEid);
	report->creationTime = rpt.creationTime.msec;
	report->creationCount = rpt.creationTime.count;
	report->fragmentOffset = rpt.fragmentOffset;
	report->statusFlags = rpt.flags;
	report->statusTime = statusTime;
	report->bundleSourceEid = strdup(dlv->bundleSourceEid);
	report->reasonString = strdup(reasonString);

	MRELEASE(sourceEid);
	eraseEid(&rpt.sourceEid);
	return bundleDeath;
}

/*
 * Convert CRS status code to BPv7-style status flags for display.
 * CRS status codes: 0=received, 1=forwarded, 2=delivered, 3=deleted
 * BPv7 flags: 1=received, 4=forwarded, 8=delivered, 16=deleted
 */
static int	crsStatusToFlags(int crsStatus)
{
	switch (crsStatus)
	{
	case CBR_STATUS_RECEIVED:
		return BP_RECEIVED_RPT;
	case CBR_STATUS_FORWARDED:
		return BP_FORWARDED_RPT;
	case CBR_STATUS_DELIVERED:
		return BP_DELIVERED_RPT;
	case CBR_STATUS_DELETED:
		return BP_DELETED_RPT;
	default:
		return 0;
	}
}

/*
 * Handle a CRS (Compressed Reporting Signal) admin record.
 * Parse the CRS format and populate statusReport structures.
 * Returns number of reports created, or -1 on error.
 *
 * CRS format: { status-code => [Bundle-Sequence, ...], ... }
 */
static int	handleCrsReport(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes, statusReport **reportArray,
			unsigned int maxReports, unsigned int *reportsCreated)
{
	uvast		mapLen;
	uvast		statusCode;
	uvast		arrayLen;
	uvast		seqId;
	uvast		seqNumStart;
	uvast		bundleLen;
	uvast		*rangeArray;
	int		rangeCount;
	char		*sourceEid;
	char		*seqDestEid;
	uvast		i;
	int		statusFlags;
	char		*reasonString;
	struct timeval	curTime;

	*reportsCreated = 0;

	/*	Get current time for status reports (CRS doesn't carry time) */
	getCurrentTime(&curTime);

	/*	Decode map						*/
	mapLen = 0;
	if (cbor_decode_map_open(&mapLen, &cursor, &unparsedBytes) < 1)
	{
		printf("[?] CRS: Can't decode map open.\n");
		return -1;
	}

	printDBG(2, "CRS: map has " UVAST_FIELDSPEC " entries\n", mapLen);

	/*	Process each status-reason entry			*/
	for (i = 0; i < mapLen && *reportsCreated < maxReports; i++)
	{
		/*	Key: status-reason code				*/
		if (cbor_decode_integer(&statusCode, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			printf("[?] CRS: Can't decode status code.\n");
			return -1;
		}

		/*	Convert CRS status to BPv7 flags		*/
		statusFlags = crsStatusToFlags((int)statusCode);
		if (statusFlags == 0)
		{
			reasonString = "(unknown CRS status)";
		}
		else if (statusFlags == BP_DELETED_RPT)
		{
			reasonString = "deleted (CRS)";
		}
		else
		{
			reasonString = "okay (CRS)";
		}

		/*	Value: array of Bundle-Sequence			*/
		arrayLen = 0;
		if (cbor_decode_array_open(&arrayLen, &cursor,
				&unparsedBytes) < 1)
		{
			printf("[?] CRS: Can't decode sequence array.\n");
			return -1;
		}

		printDBG(2, "CRS: status " UVAST_FIELDSPEC " has " UVAST_FIELDSPEC " sequences\n",
				statusCode, arrayLen);

		/*	Decode each Bundle-Sequence			*/
		while (arrayLen > 0 && *reportsCreated < maxReports)
		{
			if (cbr_decodeBundleSequence(&cursor, &unparsedBytes,
					&seqId, &seqNumStart, &bundleLen,
					&rangeArray, &rangeCount,
					&sourceEid, &seqDestEid) < 0)
			{
				printf("[?] CRS: Can't decode Bundle-Sequence.\n");
				return -1;
			}

			/*	Create a status report for this entry.
			 *	Note: CRS aggregates multiple bundles, so
			 *	we may create multiple reports per CRS.	*/

			statusReport *report = reportArray[*reportsCreated];
			if (report == NULL)
			{
				report = malloc(sizeof(statusReport));
				if (report == NULL)
				{
					if (rangeArray) MRELEASE(rangeArray);
					if (sourceEid) MRELEASE(sourceEid);
					if (seqDestEid) MRELEASE(seqDestEid);
					return -1;
				}
				reportArray[*reportsCreated] = report;
			}

			report->sourceEid = sourceEid ? strdup(sourceEid) :
					strdup("(unknown)");
			report->creationTime = 0;	/*	Not in CRS	*/
			report->creationCount = (unsigned)seqNumStart;
			report->fragmentOffset = 0;
			report->statusFlags = statusFlags;
			report->statusTime = (uvast)curTime.tv_sec * 1000 +
					curTime.tv_usec / 1000;
			report->bundleSourceEid = strdup(dlv->bundleSourceEid);
			report->reasonString = strdup(reasonString);

			(*reportsCreated)++;

			/*	Clean up				*/
			if (rangeArray)
			{
				MRELEASE(rangeArray);
			}

			if (sourceEid)
			{
				MRELEASE(sourceEid);
			}

			if (seqDestEid)
			{
				MRELEASE(seqDestEid);
			}

			arrayLen--;
		}
	}

	return (int)(*reportsCreated);
}

static int run_listen_bptrace(char *listenEid)
{
	signal(SIGABRT, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	/* Corrected allocation with sizeof(statusReport *) and calloc */
	reports = (statusReport **)calloc(128, sizeof(statusReport *));
	if (reports == NULL)
	{
		return -1;
	}

	printDBG(1, "running listen-only mode\n");

	Sdr	       sdr;
	BpDelivery     dlv;
	vast	       recordLen;
	ZcoReader      reader;
	vast	       bytesToParse;
	unsigned char  headerBuf[10];
	unsigned char *cursor;
	unsigned int   unparsedBytes;
	vast	       headerLen;
	int	       adminRecType;
	unsigned int   buflen;
	unsigned char *buffer;
	uvast	       uvtemp;
	int rpt_rval = 0;

	if (bp_attach() < 0)
	{
		printf("Can't attach to BP.\n");
		free(reports);
		return 0;
	}

	if(bp_open(listenEid, &state.sap) < 0)
	{
		printf("can't open endpoint %s\n", listenEid);
		free(reports);
		return -1;
	}
	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	printf("Listening for status reports on %s (Ctrl+C to stop)...\n", listenEid);
	fflush(stdout);

	/* Listen indefinitely until interrupted or 128 reports received */
	while(state.running && !stopRequested && n_rpts < 128)
	{
		if (bp_receive(state.sap, &dlv, BP_NONBLOCKING) < 0)
		{
			printf("Bundle reception failed, continuing\n");
			continue;
		}

		switch (dlv.result)
		{
			case BpPayloadPresent:
				printDBG(1, "received packet with payload\n");
				break;
			case BpEndpointStopped:
				printf("endpoint has been stopped\n");
				state.running = 0;
				continue;	/*	Loop exits below.	*/
			default:
				/*	Nothing ready in non-blocking mode: yield
				 *	briefly instead of spinning, so
				 *	we are not repeatedly taking and
				 *	releasing the SDR transaction lock.	*/
				microsnooze(10000);
				continue;
		}

		if (dlv.adminRecord == 0)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		printDBG(2, "checking sdr and pulling admin header...\n");
		CHKERR(sdr_begin_xn(sdr));

		recordLen = zco_source_data_length(sdr, dlv.adu);
		printDBG(2, "data length: " UVAST_FIELDSPEC "\n", recordLen);
		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, 10, (char *) headerBuf);
		if (bytesToParse < 2)
		{
			printf("Can't receive admin record header.\n");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		cursor = headerBuf;
		unparsedBytes = bytesToParse;
		uvtemp = 2;
		if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1)
		{
			printf("Can't decode admin record array.\n");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
		{
			printf("Can't decode admin record type.\n");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		adminRecType = uvtemp;
		headerLen = cursor - headerBuf;
		zco_delimit_source(sdr, dlv.adu, headerLen, recordLen - headerLen);
		zco_strip(sdr, dlv.adu);
		if (sdr_end_xn(sdr) < 0)
		{
			printf("Can't strip admin record.\n");
			oK(sdr_exit_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (adminRecType != BP_STATUS_REPORT && adminRecType != CBR_ADMIN_RECORD_CRS)
		{
			printDBG(2, "Ignoring admin record type %d.\n", adminRecType);
			bp_release_delivery(&dlv, 0);
			continue;
		}

		printDBG(1, "Received admin bundle (type %d)...\n", adminRecType);
		CHKERR(sdr_begin_xn(sdr));
		buflen = zco_source_data_length(sdr, dlv.adu);

		if ((buffer = MTAKE(buflen)) == NULL)
		{
			printf("Can't handle admin record.\n");
			bp_release_delivery(&dlv, 1);
			continue;
		}

		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, buflen, (char *) buffer);
		if (bytesToParse < 0)
		{
			printf("Can't receive admin record.\n");
			MRELEASE(buffer);
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		oK(sdr_end_xn(sdr));
		cursor = buffer;
		unparsedBytes = bytesToParse;

		if (adminRecType == BP_STATUS_REPORT)
		{
			printDBG(1, "handling traditional status report...\n");
			reports[n_rpts] = calloc(1, sizeof(statusReport));
			rpt_rval = handleStatusRpt(&dlv, cursor, unparsedBytes, reports[n_rpts]);
			if (rpt_rval >= 0)
			{
				print(reports[n_rpts]);
				fflush(stdout);
			}
			else
			{
				printf("Status report handler failed.\n");
			}
			freeStatusReport(reports[n_rpts]);
			reports[n_rpts] = NULL;
			n_rpts++;
		}
		else
		{
			unsigned int crsReportsCreated = 0;
			statusReport *crsReports[128];
			memset(crsReports, 0, sizeof(crsReports));

			rpt_rval = handleCrsReport(&dlv, cursor, unparsedBytes, crsReports, 128, &crsReportsCreated);
			if (rpt_rval >= 0)
			{
				for (unsigned int j = 0; j < crsReportsCreated; j++)
				{
					if (crsReports[j])
					{
						print(crsReports[j]);
						fflush(stdout);
						freeStatusReport(crsReports[j]);
						crsReports[j] = NULL;
					}
				}
			}
			else
			{
				printf("CRS handler failed.\n");
			}
			/* CRS Clean Sweep */
			for (unsigned int j = 0; j < crsReportsCreated; j++)
			{
				if (crsReports[j]) freeStatusReport(crsReports[j]);
			}
			MRELEASE(buffer);
			bp_release_delivery(&dlv, 1);
			continue;
		}

		MRELEASE(buffer);
		bp_release_delivery(&dlv, 1);
	}

	if (n_rpts >= 128)
	{
		printf("\nReport limit (128) reached. Exiting.\n");
	}

	free(reports);
	bp_close(state.sap);
	bp_detach();
	return 0;
}

static int run_terminal_bptrace(char *ownEid, char *destEid, char *traceEid,
			int ttl, char *classOfService, char *trace, char *flagString,
			int rtt, uvast seqId)
{
	signal(SIGABRT, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	reports = (statusReport **)calloc(128, sizeof(statusReport *));
	if (reports == NULL)
	{
		return -1;
	}

	printDBG(1, "running new code for terminal summary\n");
	int result = run_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString, seqId);
	if(result != 0)
	{
		printf("running bptrace unsuccessful, err code %d\n", result);
		free(reports);
		return result;
	}

	Sdr	       sdr;
	BpDelivery     dlv;
	vast	       recordLen;
	ZcoReader      reader;
	vast	       bytesToParse;
	unsigned char  headerBuf[10];
	unsigned char *cursor;
	unsigned int   unparsedBytes;
	vast	       headerLen;
	int	       adminRecType;
	unsigned int   buflen;
	unsigned char *buffer;
	uvast	       uvtemp;
	struct timeval timeoutTime;
	struct timeval curTime;

	getCurrentTime(&timeoutTime);
	curTime = timeoutTime;
	if (!(rtt > 0)) rtt = ttl*2;
	timeoutTime.tv_sec += rtt;

	if (bp_attach() < 0)
	{
		printf("Can't attach to BP.\n");
		free(reports);
		return 0;
	}

	if(bp_open(traceEid, &state.sap) < 0)
	{
		printf("can't open endpoint %s\n", traceEid);
		free(reports);
		return -1;
	}
	oK(_bptestState(&state));
	sdr = bp_get_sdr();

	while(state.running && !stopRequested
			&& curTime.tv_sec < timeoutTime.tv_sec && n_rpts < 128)
	{
		getCurrentTime(&curTime); // update the current time

		if (bp_receive(state.sap, &dlv, BP_NONBLOCKING) < 0)
		{
			printf("Bundle reception failed, continuing\n");
			continue;
		}

		switch (dlv.result)
		{
			case BpPayloadPresent:
				printDBG(1, "received packet with payload\n");
				break;
			case BpEndpointStopped:
				printf("endpoint has been stopped\n");
				state.running = 0;
				continue;	/*	Loop exits below.	*/
			default:
				/*	Nothing ready in non-blocking mode: yield
				 *	briefly instead of spinning on the SDR lock.	*/
				microsnooze(10000);
				continue;
		}

		if (dlv.adminRecord == 0)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		CHKERR(sdr_begin_xn(sdr));
		recordLen = zco_source_data_length(sdr, dlv.adu);
		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, 10, (char *) headerBuf);
		if (bytesToParse < 2)
		{
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		cursor = headerBuf;
		unparsedBytes = bytesToParse;
		uvtemp = 2;
		if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes) < 1 ||
			cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
		{
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}
		adminRecType = uvtemp;

		headerLen = cursor - headerBuf;
		zco_delimit_source(sdr, dlv.adu, headerLen, recordLen - headerLen);
		zco_strip(sdr, dlv.adu);
		if (sdr_end_xn(sdr) < 0)
		{
			oK(sdr_exit_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (adminRecType != BP_STATUS_REPORT && adminRecType != CBR_ADMIN_RECORD_CRS)
		{
			bp_release_delivery(&dlv, 0);
			continue;
		}

		CHKERR(sdr_begin_xn(sdr));
		buflen = zco_source_data_length(sdr, dlv.adu);
		if ((buffer = MTAKE(buflen)) == NULL)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}
		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, buflen, (char *) buffer);
		oK(sdr_end_xn(sdr));

		if (bytesToParse < 0)
		{
			MRELEASE(buffer);
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (adminRecType == BP_STATUS_REPORT)
		{
			reports[n_rpts] = calloc(1, sizeof(statusReport));
			if (handleStatusRpt(&dlv, buffer, bytesToParse, reports[n_rpts]) < 0)
			{
				printf("Status report handler failed.\n");
			}
			else
			{
				n_rpts++;
			}
		}
		else
		{
			unsigned int crsReportsCreated = 0;
			statusReport *crsReports[128];
			memset(crsReports, 0, sizeof(crsReports));
			handleCrsReport(&dlv, buffer, bytesToParse, crsReports, 128 - n_rpts, &crsReportsCreated);

			for (unsigned int j = 0; j < crsReportsCreated && n_rpts < 128; j++)
			{
				reports[n_rpts] = crsReports[j];
				n_rpts++;
				crsReports[j] = NULL;
			}
			/* CRS Clean Sweep */
			for (unsigned int j = 0; j < crsReportsCreated; j++)
			{
				if (crsReports[j]) freeStatusReport(crsReports[j]);
			}
		}
		MRELEASE(buffer);
		bp_release_delivery(&dlv, 1);
	}

	print_reports();
	free(reports);
	bp_close(state.sap);
	bp_detach();
	return 0;
}

static void
usage(
	char *progname,
	char *error,
	...)
{
	va_list ap;

	va_start(ap, error);
	if (error) {
		vfprintf(stderr,error,ap);
		    fprintf(stderr,"\n");
	}
	va_end(ap);

	fprintf(stderr, "usage: %s [-v] [-msg <msg>] [-ttl <ttl>] [-rtt <rtt>] [-qos <qos>] [-flags <flags>] [-seqid <seqid>] <srcEid> <destEid> <traceEid>\n", progname);
	fprintf(stderr, "listen mode usage: %s -listen [-v] <listenEid>\n", progname);
	fprintf(stderr,"legacy usage: %s <own EID> <destination EID> <report-to EID> <time to live (seconds)> <quality of service> '<trace text>' [<status report flag string>]\n", progname);
	fprintf(stderr, "-v        \tChanges debug level, +1 per 'v' supplied. (e.g. -vv -> debug=2)\n");
	fprintf(stderr, "-listen   \tListen-only mode: receive and display status reports without sending bundles\n");
	fprintf(stderr, "-msg <msg>  \tSpecifies message to send in data of trace bundle.\n");
	fprintf(stderr, "-ttl <ttl>  \tInteger number of seconds after which bundle should expire. Default: 10\n");
	fprintf(stderr, "-rtt <rtt>  \tInteger number of seconds to wait for status reports. Default: 2 * ttl\n");
	fprintf(stderr, "-qos <qos>  \tQuality of service. Default: 0.1 \n\t\t" BP_PARSE_QUALITY_OF_SERVICE_USAGE "\n");
	fprintf(stderr, "-flags <flags>\tStatus report flags. Default: none\n");
	fprintf(stderr, "-seqid <seqid>\tCBR sequence ID (0=dest-specific, >0=global counter). Default: 0\n");
	fprintf(stderr, "\tStatus report flag string is a sequence of status report flags separated by commas, with no embedded whitespace.\n");
	fprintf(stderr, "\tEach status report flag must be one of the following: rcv, fwd, dlv, del.\n");
	fprintf(stderr, "\tThe status reported in each bundle status report message will be the sum of the applicable status flags:\n");
	fprintf(stderr, "\t\t 1 = bundle received (rcv)\n");
	fprintf(stderr, "\t\t 4 = bundle forwarded (fwd)\n");
	fprintf(stderr, "\t\t 8 = bundle delivered (dlv)\n");
	fprintf(stderr, "\t\t16 = bundle deleted (del)\n");
	exit(-1);
}


int	main(int argc, char **argv)
{
	char *ownEid = NULL;
	char *destEid = NULL;
	char *traceEid = NULL;
	int   ttl = 0;
	int   rtt = -1;
	char *classOfService = NULL;
	char *trace = NULL;
	char *flagString = NULL;
	int   listenOnly = 0;
	uvast seqId = 0;

	int parsemode = 1;
	int i = 1;

	/* Do a first pass to check for -listen flag */
	for (i=1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-listen") == 0)
		{
			listenOnly = 1;
			break;
		}
	}

	/* Validate argument count based on mode */
	if (listenOnly)
	{
		if (argc < 2)
			usage(argv[0], "too few arguments for listen mode.");
	}
	else
	{
		if (argc < 4)
			usage(argv[0], "too few arguments.");
	}

	i = 1;
	int minArgs = listenOnly ? 1 : 3;
	for (i=1; i < (argc - minArgs); ++i)
	{
		if (*argv[i] == '-') {
			if (strcmp(argv[i],"-ttl") == 0) {
				if (i+1 >= argc) usage(argv[0],"-ttl requires argument");
				ttl = atoi(argv[++i]);
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-rtt") == 0) {
				if (i+1 >= argc) usage(argv[0],"-rtt requires argument");
				rtt = atoi(argv[++i]);
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-qos") == 0) {
				if (i+1 >= argc) usage(argv[0],"-qos requires argument");
				classOfService = argv[++i];
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-msg") == 0) {
				if (i+1 >= argc) usage(argv[0],"-msg requires argument");
				trace = argv[++i];
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-flags") == 0) {
				if (i+1 >= argc) usage(argv[0],"-flags requires argument");
				flagString = argv[++i];
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-seqid") == 0) {
				if (i+1 >= argc) usage(argv[0],"-seqid requires argument");
				seqId = strtouvast(argv[++i]);
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-listen") == 0) {
				listenOnly = 1;
				continue;	/* iterate back to for (i=1... loop */
			}
			else if (strcmp(argv[i],"-h") == 0) {
				usage(argv[0], "");
			}
			else {
				for (int j=1; argv[i][j]; ++j) {
					switch (argv[i][j]) {
						case 'v': ++BPTRACE_DEBUG; break;
						default:
							usage(argv[0], "unrecognized argument.");
					}
				}
			}
		}
		else {
			parsemode = 0;
			break;
		}
	}

	/* Parse positional arguments based on mode */
	if (listenOnly)
	{
		traceEid = argv[i];
		ownEid = traceEid;
		destEid = NULL;
	}
	else
	{
		ownEid = argv[i];
		destEid = argv[++i];
		traceEid = argv[++i];
	}

	if(!parsemode){
		if(argc != 7 && argc != 8){
			usage(argv[0], "Unrecognized argument formatting.");
		}
		ownEid = argv[1];
		destEid = argv[2];
		traceEid = argv[3];
		ttl = atoi(argv[4]);
		classOfService = argv[5];
		trace = argv[6];
		flagString = argc > 7 ? argv[7] : NULL;
	}

	if(!classOfService)
		classOfService = "0.1";
	if(!trace)
		trace = "No message supplied.";
	if(!ttl)
		ttl = 10;

	/* Validate required arguments based on mode */
	if (listenOnly)
	{
		if (!traceEid)
		{
			usage(argv[0], "listen mode requires listenEid.");
		}
	}
	else
	{
		if(!(ownEid && destEid && traceEid))
		{
			usage(argv[0], "insufficient arguments.");
		}
	}

	printDBG(2, "Legacy parsing: %s\n", (!parsemode) ?"on":"off");
	printDBG(2, "Listen-only mode: %s\n", listenOnly ? "yes" : "no");
	printDBG(2, "Own EID: '%s'\n", ownEid);
	printDBG(2, "Dest EID: '%s'\n", destEid ? destEid : "(none)");
	printDBG(2, "report EID: '%s'\n", traceEid);
	printDBG(2, "ttl: '%d'\n", ttl);
	if (parsemode) printDBG(2, "rtt: '%d'\n", rtt);
	printDBG(2, "QoS: '%s'\n", classOfService);
	printDBG(2, "Message: '%s'\n", trace);
	printDBG(2, "Flags: '%s'\n", flagString ? flagString : "");
	printDBG(2, "Debug: '%d'\n", BPTRACE_DEBUG);

	/* Handle listen-only mode */
	if (listenOnly)
	{
		return run_listen_bptrace(traceEid);
	}

	/* Normal mode routing */
	char* traceEid_num = strchr(traceEid, ':')+1;
	char* traceEid_dot = strrchr(traceEid, '.')+1;
	char* ownEid_num = strchr(ownEid, ':')+1;
	printDBG(3, "trace id num: %s\n", traceEid_num);
	printDBG(3, "own id num: %s\n", ownEid_num);
	printDBG(3, "ncmp: %d\n", strncmp(traceEid_num, ownEid_num, traceEid_dot - traceEid_num));
	printDBG(3, "cmp: %d\n", strcmp(traceEid_dot, "0"));
	if(strncmp(traceEid_num, ownEid_num, traceEid_dot - traceEid_num) == 0 &&
		strcmp(traceEid_dot, "0") != 0){
		// run terminal interface version if report endpoint is on this node and is not the admin endpoint.
		return run_terminal_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString, rtt, seqId);
	}else{
		return run_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString, seqId);
	}
}
#endif
