/*
 * bpstats2.c
 * Very heavily based on bpstats, in the original ION distribution.
 * Andrew Jenkins <andrew.jenkins@colorado.edu>
 * A BP client that fills a bundle with statistics about the current BPA
 * and sends it.
 */

#include <stdlib.h>
#include "bpP.h"
#include "ion.h"

static BpSAP        sap;
static BpVdb        *vdb;
static Sdr          sdr;
char *defaultDestEid = NULL;
char *ownEid = NULL;
static BpCustodySwitch custodySwitch = NoCustodyRequested;
char   theBuffer[2048];
static int needSendDefault = 0;
static int needShutdown = 0;

const char usage[] = 
"Usage: bpstats2 <source EID> [<default dest EID>] [ct]\n\n"
"Replies to any bundles it receives with a bundle containing the statistics\n"
"of the BPA to which it is attached.\n\n"
"If a default destination EID is specified, then statistics bundles can be\n"
"triggered to be sent to that EID by sending SIGUSR1 to bpstats2.\n"
"If ct specified, the bundles are sent with custody transfer.\n";

static ReqAttendant	*_attendant(ReqAttendant *newAttendant)
{
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)
	{
		attendant = newAttendant;
	}

	return attendant;
}

void handleQuit(int sig)
{
	needShutdown = 1;
	bp_interrupt(sap);
	ionPauseAttendant(_attendant(NULL));
}

void sendDefault(int sig)
{
	needSendDefault = 1;
	bp_interrupt(sap);
	ionPauseAttendant(_attendant(NULL));
}

/* This is basically libbpP.c's "reportStateStats()" except to a buffer. */
int appendStateStats(char *buffer, size_t len, int stateIdx)
{
	static char *classnames[] = 
	{ "src", "fwd", "xmt", "rcv", "dlv", "ctr", "rfw", "exp" };
	Sdr		sdr = getIonsdr();
	Object		bpDbObject = getBpDbObject();
	BpDB		bpdb;
	time_t startTime;
	time_t currentTime;
	BpCosStats	cosStats;
	BpDbStats	dbStats;
	Tally		tallies[4];

	if (stateIdx < 0 || stateIdx > 7) { return -1; }

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) &bpdb, bpDbObject, sizeof(BpDB));
	startTime = bpdb.resetTime;
	currentTime = getCtime();
	switch (stateIdx)
	{
	case 0:
		sdr_read(sdr, (char *) &cosStats, bpdb.sourceStats,
				sizeof(BpCosStats));
		memcpy((char *) &tallies[0], (char *) &cosStats.tallies[0],
				sizeof(Tally));
		memcpy((char *) &tallies[1], (char *) &cosStats.tallies[1],
				sizeof(Tally));
		memcpy((char *) &tallies[2], (char *) &cosStats.tallies[2],
				sizeof(Tally));
		tallies[3].currentCount = tallies[0].currentCount
			+ tallies[1].currentCount + tallies[2].currentCount;
		tallies[3].currentBytes = tallies[0].currentBytes
			+ tallies[1].currentBytes + tallies[2].currentBytes;
		break;

	case 1:
		sdr_read(sdr, (char *) &dbStats, bpdb.dbStats,
				sizeof(BpDbStats));
		memset((char *) &tallies[0], 0, sizeof(Tally));
		memset((char *) &tallies[1], 0, sizeof(Tally));
		memset((char *) &tallies[2], 0, sizeof(Tally));
		memcpy((char *) &tallies[3], (char *)
				&dbStats.tallies[BP_DB_FWD_OKAY],
				sizeof(Tally));
		break;

	case 2:
		sdr_read(sdr, (char *) &cosStats, bpdb.xmitStats,
				sizeof(BpCosStats));
		memcpy((char *) &tallies[0], (char *) &cosStats.tallies[0],
				sizeof(Tally));
		memcpy((char *) &tallies[1], (char *) &cosStats.tallies[1],
				sizeof(Tally));
		memcpy((char *) &tallies[2], (char *) &cosStats.tallies[2],
				sizeof(Tally));
		tallies[3].currentCount = tallies[0].currentCount
			+ tallies[1].currentCount + tallies[2].currentCount;
		tallies[3].currentBytes = tallies[0].currentBytes
			+ tallies[1].currentBytes + tallies[2].currentBytes;
		break;

	case 3:
		sdr_read(sdr, (char *) &cosStats, bpdb.recvStats,
				sizeof(BpCosStats));
		memcpy((char *) &tallies[0], (char *) &cosStats.tallies[0],
				sizeof(Tally));
		memcpy((char *) &tallies[1], (char *) &cosStats.tallies[1],
				sizeof(Tally));
		memcpy((char *) &tallies[2], (char *) &cosStats.tallies[2],
				sizeof(Tally));
		tallies[3].currentCount = tallies[0].currentCount
			+ tallies[1].currentCount + tallies[2].currentCount;
		tallies[3].currentBytes = tallies[0].currentBytes
			+ tallies[1].currentBytes + tallies[2].currentBytes;
		break;

	case 4:
		memset((char *) &tallies[0], 0, sizeof(Tally));
		memset((char *) &tallies[1], 0, sizeof(Tally));
		memset((char *) &tallies[2], 0, sizeof(Tally));
		memset((char *) &tallies[3], 0, sizeof(Tally));
		break;

	case 5:
		memset((char *) &tallies[0], 0, sizeof(Tally));
		memset((char *) &tallies[1], 0, sizeof(Tally));
		memset((char *) &tallies[2], 0, sizeof(Tally));
		memset((char *) &tallies[3], 0, sizeof(Tally));
		break;

	case 6:
		sdr_read(sdr, (char *) &dbStats, bpdb.dbStats,
				sizeof(BpDbStats));
		memset((char *) &tallies[0], 0, sizeof(Tally));
		memset((char *) &tallies[1], 0, sizeof(Tally));
		memset((char *) &tallies[2], 0, sizeof(Tally));
		memcpy((char *) &tallies[3], (char *)
				&dbStats.tallies[BP_DB_REQUEUED_FOR_FWD],
				sizeof(Tally));
		break;

	default:		/*	Can only be 7.			*/
		sdr_read(sdr, (char *) &dbStats, bpdb.dbStats,
				sizeof(BpDbStats));
		memset((char *) &tallies[0], 0, sizeof(Tally));
		memset((char *) &tallies[1], 0, sizeof(Tally));
		memset((char *) &tallies[2], 0, sizeof(Tally));
		memcpy((char *) &tallies[3], (char *)
				&dbStats.tallies[BP_DB_EXPIRED],
				sizeof(Tally));
	}

	sdr_exit_xn(sdr);
	return snprintf(buffer, len, "  [x] %s from %u to %u: (0) \
%u " UVAST_FIELDSPEC " (1) %u " UVAST_FIELDSPEC " (2) \
%u " UVAST_FIELDSPEC " (+) %u " UVAST_FIELDSPEC "\n", classnames[stateIdx],
			(unsigned int) startTime,
			(unsigned int) currentTime,
			tallies[0].currentCount, tallies[0].currentBytes,
			tallies[1].currentCount, tallies[1].currentBytes,
			tallies[2].currentCount, tallies[2].currentBytes,
			tallies[3].currentCount, tallies[3].currentBytes);
}

int sendStats(char *destEid, char *buffer, size_t len)
{
	int bytesWritten = 0, rc;
	int i = 0;
	Object bundleZco, extent;
	Object newBundle;   /* We never use but bp_send requires it. */

	if(destEid == NULL || (strcmp(destEid, "dtn:none") == 0)) {
		putErrmsg("Can't send stats: bad dest EID", destEid);
		return -1;
	}

	/* Write the stats to a buffer. */
	rc = snprintf(buffer, len, "stats %s: \n",  ownEid);
	if(rc < 0) return -1;
	bytesWritten += rc;

	for(i = 0; bytesWritten < len && i < 8; i++) {
		rc = appendStateStats(buffer + bytesWritten, len - bytesWritten, i);
		if(rc < 0) return -1;
		bytesWritten += rc;
	}

	/* Wrap bundleZco around the stats buffer. */
	CHKERR(sdr_begin_xn(sdr));
	extent = sdr_malloc(sdr, bytesWritten);
	if(extent) {
		sdr_write(sdr, extent, buffer, bytesWritten);
	}

	if(sdr_end_xn(sdr) < 0) {
		putSysErrmsg("No space for ZCO extent", NULL);
		return -1;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, bytesWritten,
			BP_STD_PRIORITY, 0, ZcoOutbound, _attendant(NULL));
	if(bundleZco == 0 || bundleZco == (Object) ERROR)
	{
		putErrmsg("Can't create ZCO.", NULL);
		return -1;
	}

	/* Send bundleZco, the stats bundle. */
	if(bp_send(sap, destEid, NULL, 86400, BP_STD_PRIORITY, custodySwitch,
			0, 0, NULL, bundleZco, &newBundle) <= 0)
	{
		putSysErrmsg("bpstats2 can't send stats bundle.", NULL);
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	ownEid          = (argc > 1 ? argv[1] : NULL);
	defaultDestEid  = (argc > 2 ? argv[2] : NULL);
	char *ctArg     = (argc > 3 ? argv[3] : NULL);
	ReqAttendant	attendant;
	BpDelivery      dlv;


	if(argc < 2 || (argv[1][0] == '-')) {
		fprintf(stderr, usage);
		exit(1);
	}

	/* See if the args request ct.  ION eschews the use of getopt. */
	if(ctArg && strncmp(ctArg, "ct", 3) == 0) {
		custodySwitch = SourceCustodyRequired;
	} else if(defaultDestEid && strncmp(defaultDestEid, "ct", 3) == 0) {
		/* args specify 'ct' but no defaultDestEid. */
		custodySwitch = SourceCustodyRequired;
		defaultDestEid = NULL;
	}


	if(bp_attach() < 0) {
		putErrmsg("Can't bp_attach()", NULL);
		exit(1);
	}

	/* Hook up to ION's private side to get the stats. */
	vdb = getBpVdb();

	if(bp_open(ownEid, &sap) < 0)
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		exit(1);
	}

	sdr = bp_get_sdr();
	if (ionStartAttendant(&attendant) < 0)
	{
		putErrmsg("Can't initialize blocking transmission.", NULL);
		exit(1);
	}

	oK(_attendant(&attendant));
	signal(SIGINT, handleQuit); 
	signal(SIGUSR1, sendDefault);

	while(needShutdown == 0)
	{
		/* Wait for a bundle. */
		if(bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			bp_close(sap);
			putErrmsg("bpstats2 bundle reception failed.", NULL);
			exit(1);
		}

		if(dlv.result == BpPayloadPresent)
		{
			sendStats(dlv.bundleSourceEid, theBuffer,
					sizeof(theBuffer));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if(dlv.result == BpReceptionInterrupted)
		{
			if(needSendDefault)
			{
				needSendDefault = 0;
				sendStats(defaultDestEid, theBuffer,
						sizeof(theBuffer));
				ionResumeAttendant(&attendant);
			}

			bp_release_delivery(&dlv, 1);
			continue;
		}

		/*	On any other delivery result, shut down.	*/

		bp_release_delivery(&dlv, 1);
		break;
	}

	bp_close(sap);
	bp_detach();
	ionStopAttendant(&attendant);
	writeErrmsgMemos();
	return 0;
}
