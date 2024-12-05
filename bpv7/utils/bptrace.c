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
#define _GNU_SOURCE
#include <bpP.h>

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

const size_t datelen = 32;//strlen("YYYY-MM-DDThh:mm:ss.sss")+1 + 8; // 8 is purely for safety

char* dtnTimeToDate(uvast time){
	char* buffer = malloc(datelen);
	uvast time_sec = time/1000;
	double ms = (((double)time)/1000 - time_sec) * 1000;
	time_t t = time_sec + EPOCH_2000_SEC;
	struct tm *epoch_time = localtime(&t);
	strftime(buffer, datelen, "%Y-%m-%dT%H:%M:%S", epoch_time);
	sprintf(buffer + strlen(buffer), ".%03.f", ms);
	return buffer;
}

char* statusToString(int statusFlags, char* buf, unsigned buflen){
	char* buffer = malloc(32);
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
		statusToString(rpt->statusFlags, buffer, sizeof(buffer)), tmbuffer, rpt->bundleSourceEid,
		rpt->reasonString);
	free(buffer);
	printDBG(3, "statusTime: " UVAST_FIELDSPEC "<=> %s\n", rpt->statusTime, tmbuffer);
}

void sortByStatusTime(statusReport *rpts[], unsigned n_rpts){
	for(unsigned i = 0; i < n_rpts-1; ++i){
		for(unsigned j = 0; j < (n_rpts-i)-1; ++j){
			printDBG(3, "j: %u, J+1: %u, n_rpts: %u\n", j, j+1, n_rpts);
			
			printDBG(3, UVAST_FIELDSPEC"\n", rpts[j]->statusTime);
			printDBG(3, UVAST_FIELDSPEC"\n", rpts[j+1]->statusTime);
			if(rpts[j]->statusTime  > rpts[j+1]->statusTime){
				statusReport *tmp = rpts[j];
				rpts[j] = rpts[j+1];
				rpts[j+1] = tmp;
			} else if(rpts[j]->statusTime == rpts[j+1]->statusTime && 
				strncmp(rpts[j+1]->sourceEid, rpts[j+1]->bundleSourceEid, 
					strchr(rpts[j+1]->sourceEid, '.') - rpts[j+1]->sourceEid) == 0)
			{
				statusReport *tmp = rpts[j];
				rpts[j] = rpts[j+1];
				rpts[j+1] = tmp;
			}
		}
	}
}
// const char* header_rpt = "srcEid/creationTime:count/offset 'status' # 'at' time 'on' statusEid, statusMsg\n";
void print_reports(){
	
	if(reports && n_rpts > 0){
		printf("\nDone, printing in time order: \n");
		printf("------------------------------\n");
		// printf(header_rpt);
		sortByStatusTime(reports, n_rpts);
		for(unsigned i = 0; i < n_rpts; ++i){
			print(reports[i]);
			free(reports[i]);
		}
	}
}

void sighandler(int signum)	{
	printDBG(3, "signal number: %d\n", signum);
	print_reports();
	bp_close(state.sap);
	bp_detach();
	exit(signum);
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
			int ttl, char *svcClass, char *trace, char *flags)
{
	int		priority = 0;
	BpAncillaryData	ancillaryData = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	int		srrFlags = 0;
	BpSAP		sap;
	Sdr		sdr;
	Object		newBundle;

	if (!bp_parse_quality_of_service(svcClass, &ancillaryData,
			&custodySwitch, &priority))
	{
		putErrmsg("Invalid class of service for bptrace.", svcClass);
		return 0;
	}

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
		Object      fileRef;
		struct stat	statbuf;
		int		aduLength;
		Object	traceZco;
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
		Object	msg;
		Object	traceZco;
		
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
		if (traceZco == 0 || traceZco == (Object) ERROR)
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

	if (ownEid == NULL || destEid == NULL || classOfService == NULL
	|| trace == NULL)
	{
		PUTS("Missing argument(s) for bptrace.  Ignored.");
		return 0;
	}
	return run_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString);
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

static int run_terminal_bptrace(char *ownEid, char *destEid, char *traceEid,
			int ttl, char *classOfService, char *trace, char *flagString, int rtt){
	signal(SIGABRT, sighandler); // ensure that quit and interrupt signals still output trace as of that moment.
	signal(SIGINT, sighandler);
	reports = (statusReport **)malloc(sizeof(statusReport)*128); // allow storage of up to 128 reports.

	printDBG(1, "running new code for terminal summary\n");

	int result = run_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString);
	if(result != 0){
		printf("running bptrace unsuccessful, err code %d\n", result);
		return result;
	}
	
	
	Sdr sdr;
	BpDelivery dlv;
	vast		recordLen;
	ZcoReader	reader;
	vast		bytesToParse;
	unsigned char	headerBuf[10];
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	vast		headerLen;
	int		adminRecType;
	unsigned int	buflen;
	unsigned char	*buffer;
	uvast		uvtemp;		

	int rpt_rval = 0;

	struct timeval timeoutTime;
	struct timeval curTime;
	

	getCurrentTime(&timeoutTime);
	curTime = timeoutTime;
	if (!(rtt > 0)) rtt = ttl*2;// allow <ttl> seconds for travel of the packet sent and the returning status reports.
	timeoutTime.tv_sec += rtt; 
	printDBG(2, "current time: %lu timeouttime: %lu\n", curTime.tv_sec, timeoutTime.tv_sec);
	
	if (bp_attach() < 0)
	{
		printf("Can't attach to BP.\n");
		return 0;
	}

	if(bp_open(traceEid, &state.sap) < 0){
		printf("can't open endpoint %s\n", traceEid);
		return -1;
	}
	oK(_bptestState(&state));
	sdr = bp_get_sdr();

	while(state.running && curTime.tv_sec < timeoutTime.tv_sec && n_rpts < 128){
		getCurrentTime(&curTime); // update the current time
		if (bp_receive(state.sap, &dlv, BP_NONBLOCKING) < 0) 
		{
			printf("Bundle reception failed, continuing\n");
			continue;
		}

		switch (dlv.result)
		{
			case BpPayloadPresent:
				printDBG(1, "recieved packet with payload\n");
				break;
			case BpEndpointStopped:
				printf("endpoint has been stopped\n");
				state.running = 0;
				/*	Intentional fall-through to default.	*/
			default:
				continue;
		}

		/* only accept admin bundles */
		if (dlv.adminRecord == 0)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		/*	Read and strip off the admin record header:
		*	array open (1 byte), record type code (up to
		*	9 bytes).					*/
		printDBG(2, "checking sdr and pulling admin header...\n");
		CHKERR(sdr_begin_xn(sdr));
		
		recordLen = zco_source_data_length(sdr, dlv.adu);
		printDBG(2, "data length: " UVAST_FIELDSPEC "\n", recordLen);
		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, 10,
				(char *) headerBuf);
		if (bytesToParse < 2)
		{
			printf("Can't receive admin record header.\n");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		cursor = headerBuf;
		unparsedBytes = bytesToParse;
		uvtemp = 2;	/*	Decode array of size 2.		*/
		if (cbor_decode_array_open(&uvtemp, &cursor, &unparsedBytes)
				< 1)
		{
			printf("[?] Can't decode admin record array open.\n");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			printf("[?] Can't decode admin record type.\n");
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		adminRecType = uvtemp;

		/*	Now strip off the admin record header, leaving
		*	just the admin record content.			*/

		headerLen = cursor - headerBuf;
		zco_delimit_source(sdr, dlv.adu, headerLen,
				recordLen - headerLen);
		zco_strip(sdr, dlv.adu);
		if (sdr_end_xn(sdr) < 0)
		{
			printf("Can't strip admin record.\n");
			oK(sdr_exit_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		// ignore admin bundles other than status reports.
		if (adminRecType != BP_STATUS_REPORT)
		{
			bp_release_delivery(&dlv, 0);
			continue;
		}

		/*	read the entire admin record into memory buffer.	*/
		printDBG(1, "Recieved admin bundle...\n");	
		CHKERR(sdr_begin_xn(sdr));
		buflen = zco_source_data_length(sdr, dlv.adu);
		
		if ((buffer = MTAKE(buflen)) == NULL)
		{
			printf("Can't handle admin record.\n");
			bp_release_delivery(&dlv, 1);
			continue;
		}

		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, buflen,
				(char *) buffer);
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

		printDBG(1, "handling status report...\n");
		reports[n_rpts] = malloc(sizeof(*reports[n_rpts]/*statusReport*/));
		printDBG(3, "report pointer after malloc: %p\n", reports[n_rpts]);
		rpt_rval = handleStatusRpt(&dlv, cursor, unparsedBytes, reports[n_rpts]);
		printDBG(3, "\n"UVAST_FIELDSPEC"\n", reports[n_rpts]->creationTime);
		n_rpts++;
		printDBG(2, "report handler returned %d\n", rpt_rval);			
		bp_release_delivery(&dlv, 1);
		if (rpt_rval < 0)
		{
			printf("Status report handler failed.\n");
		}
	}
	print_reports();
	bp_close(state.sap);
	bp_detach();
	return(0);
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

	fprintf(stderr, "usage: %s [-v] [-msg <msg>] [-ttl <ttl>] [-rtt <rtt>] [-qos <qos>] [-flags <flags>] <srcEid> <destEid> <traceEid>\n", progname);
	fprintf(stderr,"legacy usage: %s <own EID> <destination EID> <report-to EID> <time to live (seconds)> <quality of service> '<trace text>' [<status report flag string>]\n", progname);
	fprintf(stderr, "-v        \tChanges debug level, +1 per 'v' supplied. (e.g. -vv -> debug=2)\n");
	fprintf(stderr, "-msg <msg>  \tSpecifies message to send in data of trace bundle.\n");
	fprintf(stderr, "-ttl <ttl>  \tInteger number of seconds after which bundle should expire. Default: 10\n");
	fprintf(stderr, "-rtt <rtt>  \tInteger number of seconds to wait for status reports. Default: 2 * ttl\n");
	fprintf(stderr, "-qos <qos>  \tQuality of service. Default: 0.1 \n\t\t" BP_PARSE_QUALITY_OF_SERVICE_USAGE "\n");
	fprintf(stderr, "-flags <flags>\tStatus report flags. Default: none\n");
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
	char	*ownEid = NULL;
	char	*destEid = NULL;
	char	*traceEid = NULL;
	int	ttl = 0;
	int rtt = -1;
	char	*classOfService = NULL;
	char	*trace = NULL; 
	char	*flagString = NULL;

	int parsemode = 1;
	if (argc < 4)
		usage(argv[0], "too few arguments.");
	
	int i = 1;
	for (i=1; i < (argc -3); ++i)
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
	ownEid = argv[i];
	destEid = argv[++i];
	traceEid = argv[++i];

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
	if(!(ownEid && destEid && traceEid)){
		usage(argv[0], "insufficient arguments.");
	}

	printDBG(2, "Legacy parsing: %s\n", (!parsemode) ?"on":"off");
	printDBG(2, "Own EID: '%s'\n", ownEid);
	printDBG(2, "Dest EID: '%s'\n", destEid);
	printDBG(2, "report EID: '%s'\n", traceEid);
	printDBG(2, "ttl: '%d'\n", ttl);
	if (parsemode) printDBG(2, "rtt: '%d'\n", rtt);
	printDBG(2, "QoS: '%s'\n", classOfService);
	printDBG(2, "Message: '%s'\n", trace);
	printDBG(2, "Flags: '%s'\n", flagString ? flagString : "");
	printDBG(2, "Debug: '%d'\n", BPTRACE_DEBUG);

	char* traceEid_num = strchr(traceEid, ':')+1;
	char* traceEid_dot = strchr(traceEid, '.')+1;
	char* ownEid_num = strchr(ownEid, ':')+1;
	printDBG(3, "trace id num: %s\n", traceEid_num);
	printDBG(3, "own id num: %s\n", ownEid_num);
	printDBG(3, "ncmp: %d\n", strncmp(traceEid_num, ownEid_num, traceEid_dot - traceEid_num));
	printDBG(3, "cmp: %d\n", strcmp(traceEid_dot, "0"));
	if(strncmp(traceEid_num, ownEid_num, traceEid_dot - traceEid_num) == 0 &&
		strcmp(traceEid_dot, "0") != 0){
		// run terminal interface version if report endpoint is on this node and is not the admin endpoint.
		return run_terminal_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString, rtt);		
	}else{
		return run_bptrace(ownEid, destEid, traceEid, ttl, classOfService, trace, flagString);
	}
}
#endif
