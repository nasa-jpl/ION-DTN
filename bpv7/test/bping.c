/*
 * bping.c <andrew.jenkins@colorado.edu>
 * Performs an ICMP-echo-like "ping" over BP, for ION.
 * This program attaches to the source endpoint, then sends a bundle every
 * INTERVAL seconds to the destination endpoint.  The contents of the bundle
 * are a unique identifier.  If the destination endpoint echos these bundles
 * back (for example, via bpecho), the round-trip time is displayed.
 */

#include <getopt.h>     /* getopt */
#include <bp.h>
#include <lyst.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

/*	Note: bping originally used strtok_r to parse responses,
	but neither strtok_r nor strtok_s are provided by MinGW.	*/

const char usage[] =
  "Usage: bping [options] <source EID> <destination EID> [report-to EID]\n\n" 
  "Sends bundles from <source EID> to <destination EID>.  If responses are\n"
  "received, prints the elapsed round trip time.\n"
  "Options:\n"
  "  -c <count>     Sends <count> bundles before stopping.\n"
  "  -i <interval>  Wait <interval> seconds between bundles.  Default: 1.\n"
  "  -p <priority>  Bundles have priority <priority> (default 0 = bulk).\n"
  "  -q <wait>      Wait <wait> seconds after sending the last bundle to\n"
  "                   accumulate responses.  Defaults to 10s, pass -1 to\n"
  "                   wait until all bundles are acked before quitting.\n"
  "  -r <flags>     <flags> can be any combination of rcv,ct,fwd,dlv,del,ctr\n"
  "                   delimited by ',' (without spaces).  Sets the \n"
  "                   corresponding report request flag.\n"
  "  -t <ttl>       Bundles have lifetime of <ttl> seconds.\n"
  "                   (default 3600)\n"
  "  -v             Increase verbosity level (can be specified repeatedly)\n";

static int count = -1;        /* -1: Indefinite.  Set from command line, 
                                 never written again. */
static double interval = 1.0; /* Wait one second between bundles */
static int payloadSize = 64;  /* Number of bytes of bping payload. */
static int verbosity = 0;
static int waitdelay = 10;    /* Number of seconds to wait after last 
                                 bundle for its response. */
static int ttl = 3600;        /* Lifetime to set in bundles. */
static int priority = 0;      /* Priority level of bundles. */
static int rrFlags = 0;       /* Report request flags */
static int totalsent = 0;     /* Only written by sendRequests thread */
static int totalreceived = 0; /* Only written by receiveResponses thread */
static int shutdownnow = 0;      /* 1: Cleanup and shutdown. */
static pthread_mutex_t sdrmutex;
static pthread_t receiveResponsesThread;
static pthread_t sendRequestsThread;
static int sendRequestsThreadRunning = 0;
static BpCustodySwitch custodySwitch = NoCustodyRequested;

static Sdr      sdr;
static BpSAP    xmitsap;
static BpSAP    recvsap;
static char     *srcEid, *dstEid, *rptEid;

#define BPING_PAYLOAD_MAX_LEN 10000

/* These exit codes are the same as iputils' ping. */
#define BPING_EXIT_SUCCESS              (0)
#define BPING_EXIT_NOTALLRESPONDED      (1)
#define BPING_EXIT_ERROR                (2)

/* All values are kept in us (us^2 for sum2), just like iputils. */
static long min = LONG_MAX, max = 0, dev;
static long long sum = 0, sum2 = 0;

/* iputils uses Newton's method to ping; we copy that here.  The method below 
 * will find the largest integer less than or equal to the square root of a, 
 * unless (a+1) is a perfect square, in which case it will return sqrt(a+1).
 * From iputils' ping_common.c */
static long llsqrt(long long a)
{
	long long prev = ~((long long)1 << 63);
	long long x = a;

	if (x > 0) {
		while (x < prev) {
			prev = x;
			x = (x+(a/x))/2;
		}
	}

	return (long)x;
}

static void handleQuit()
{
	signal(SIGINT, handleQuit);
	shutdownnow = 1;
	bp_interrupt(recvsap);
	if(sendRequestsThreadRunning
	&& !pthread_equal(sendRequestsThread, pthread_self()))
	{
		pthread_kill(sendRequestsThread, SIGINT);
	}
}

/* Subtract the `struct timeval' values X and Y, storing the result in RESULT.
 * Return 1 if the difference is negative, otherwise 0.
 * From GNU libc reference manual sec. 21.2. */
static int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	   tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}


static void *receiveResponses(void *x)
{
	BpDelivery  dlv;
	struct timeval tvNow, tvResp, tvDiff;
	ZcoReader   reader;
	int         contentLength, bytesToRead, result;
	char        buffer[BPING_PAYLOAD_MAX_LEN];
	char        respSrcEid[64];
	char        *countstr, *secstr, *usecstr, *endptr;
	unsigned long respcount;
	long        diff_in_us;

	while((shutdownnow == 0) && (count == -1 || totalreceived < count) &&
			bp_receive(recvsap, &dlv, BP_BLOCKING) >= 0)
	{
		/* Get the time the response was received */
		if(gettimeofday(&tvNow, NULL) < 0) {
			putErrmsg("Couldn't gettimeofday", NULL);
			perror("Couldn't gettimeofday");
			pthread_exit(NULL);
		}

		/* ReceptionInterrupted means this process received a signal but not
		 * a bundle.  Try receiving again... */
		if(dlv.result == BpReceptionInterrupted || dlv.adu == 0) {
			if(verbosity) fprintf(stderr, "Reception interrupted.\n");
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (dlv.result == BpEndpointStopped)
		{
			if(verbosity) fprintf(stderr, "Endpoint stopped.\n");
			bp_release_delivery(&dlv, 1);
			shutdownnow = 1;
			continue;
		}

		/* Buffer the response so we can bp_release_delivery before parsing. */
		if(pthread_mutex_lock(&sdrmutex) != 0)
		{
			putErrmsg("Couldn't take sdr mutex in receiveResponses.", NULL);
			fprintf(stderr, "Couldn't take sdr mutex in receiveResponses.\n");
			pthread_exit(NULL);
		}
		contentLength = zco_source_data_length(sdr, dlv.adu);
		bytesToRead = MIN(contentLength, sizeof(buffer)-1); 
		zco_start_receiving(dlv.adu, &reader);
		oK(sdr_begin_xn(sdr));
		result = zco_receive_source(sdr, &reader, bytesToRead, buffer);
		if (sdr_end_xn(sdr) < 0 || result < 0)
		{
			putErrmsg("Can't receive payload.", NULL);
			fprintf(stderr, "Can't receive payload.\n");
			pthread_exit(NULL);
		}
		buffer[bytesToRead] = '\0';
		strncpy(respSrcEid, dlv.bundleSourceEid, 63);
		respSrcEid[63] = '\0';
		bp_release_delivery(&dlv, 1);
		if(pthread_mutex_unlock(&sdrmutex) != 0)
		{
			putErrmsg("Couldn't give sdr mutex in receiveResponses.", NULL);
			fprintf(stderr, "Couldn't give sdr mutex in receiveResponses.\n");
			pthread_exit(NULL);
		}

		/* If this bundle isn't from the right source, ignore. */
		if(strcmp(respSrcEid, dstEid) != 0) {
			if(verbosity) {
				fprintf(stderr, "Ignoring a bundle from %s\n", respSrcEid);
			}
			continue;
		}

		/* Parse out the values in the response */
		countstr = strtok(buffer, " ");
		if(countstr == NULL) {
			putErrmsg("Couldn't parse countstr", buffer);
			fprintf(stderr, "Couldn't parse countstr.\n");
			continue;
		}
		secstr = strtok(NULL, " ");
		if(secstr == NULL) {
			putErrmsg("Couldn't parse secstr", NULL);
			fprintf(stderr, "Couldn't parse secstr.\n");
			continue;
		}
		usecstr = strtok(NULL, " ");
		if(usecstr == NULL) {
			putErrmsg("Couldn't parse usecstr", NULL);
			fprintf(stderr, "Couldn't parse usecstr.\n");
			continue;
		}

		respcount = strtoul(countstr, &endptr, 0);
		if(endptr == NULL) {
			putErrmsg("Couldn't convert countstr", countstr);
			fprintf(stderr, "Couldn't convert countstr: %s\n", countstr);
			continue;
		}
		tvResp.tv_sec = strtoul(secstr, &endptr, 0);
		if(endptr == NULL) {
			putErrmsg("Couldn't convert secstr", secstr);
			fprintf(stderr, "Couldn't convert secstr: %s\n", secstr);
			continue;
		}
		tvResp.tv_usec = strtoul(usecstr, &endptr, 0);
		if(endptr == NULL) {
			putErrmsg("Couldn't convert usecstr", usecstr);
			fprintf(stderr, "Couldn't convert usecstr: %s\n", usecstr);
			continue;
		}

		/* Print the result */
		if(timeval_subtract(&tvDiff, &tvNow, &tvResp) == 1) {
			/* Diff is negative */
			diff_in_us = - (tvDiff.tv_sec * 1000000 + tvDiff.tv_usec);
		} else {
			/* Diff is positive */
			diff_in_us =   (tvDiff.tv_sec * 1000000 + tvDiff.tv_usec);
		}

		if(diff_in_us < 0) {
			printf("%d bytes from %s  seq=%lu time=-%lu.%06lu \
s(future!)\n", contentLength, respSrcEid, respcount,
					(unsigned long)tvDiff.tv_sec, 
					(unsigned long)tvDiff.tv_usec);
		} else {
			printf("%d bytes from %s  seq=%lu time=%lu.%06lu s\n",
					contentLength, respSrcEid, respcount,
					(unsigned long)tvDiff.tv_sec, 
					(unsigned long)tvDiff.tv_usec);
		}

		/* Update statistics */
		if(diff_in_us < min) min = diff_in_us;
		if(diff_in_us > max) max = diff_in_us;
		sum += diff_in_us;
		sum2 += (long long)(diff_in_us) * (long long)(diff_in_us);

		++totalreceived; /* Successful receipt */
	}
	if(verbosity) fprintf(stderr, "receiveResponses done.\n");
	return NULL;
}

/* Makes a new bundle Object.  The payload is a unique identifier. */
static Object bping_new_ping(void)
{
	Object  bundleZco;
	Object  bundleMessage;
	struct timeval tvNow;
	char    pingPayload[BPING_PAYLOAD_MAX_LEN];
	int     pingPayloadLen;
	pid_t	myPid;


	if(gettimeofday(&tvNow, NULL) < 0) {
		putErrmsg("Couldn't gettimeofday()", NULL);
		perror("bping_new_ping gettimeofday()");
		return 0;
	}

	myPid = getpid();

	/* Construct the bundle payload */
	pingPayloadLen = snprintf(pingPayload, sizeof(pingPayload), 
			"%d %lu %lu %lu bping payload", totalsent,
			(unsigned long) tvNow.tv_sec, 
			(unsigned long) tvNow.tv_usec,
			(unsigned long) myPid);
	if(pingPayloadLen < 0) {
		putErrmsg("Couldn't construct bping payload.", NULL);
		fprintf(stderr, "Couldn't construct bping payload.");
		return 0;
	}

	//if(pingPayloadLen >= sizeof(pingPayload)) {
	//	pingPayloadLen = sizeof(pingPayload) - 1;
	//}

	CHKZERO(sdr_begin_xn(sdr));
	bundleMessage = sdr_malloc(sdr, payloadSize);
	if(bundleMessage) {
		sdr_write(sdr, bundleMessage, pingPayload, payloadSize);
	}

	if(sdr_end_xn(sdr)) {
		putErrmsg("No space for bping text.", NULL);
		fprintf(stderr, "No space for bping text.\n");
		return 0;
	}

	/* Craft the bundle object */
	bundleZco = ionCreateZco(ZcoSdrSource, bundleMessage, 0, 
			payloadSize, priority, 0, ZcoOutbound, NULL);
	if(bundleZco == 0 || bundleZco == (Object) ERROR)
	{
		putErrmsg("bping can't create bundle ZCO", NULL);
		fprintf(stderr, "bping can't create bundle ZCO.\n");
		return 0;
	}
	return bundleZco;
}

static void *sendRequests(void *x)
{
	Object  bundleZco;

	/* Send bundles until we are told to shutdownnow, or we've sent "count". */
	sendRequestsThreadRunning = 1;
	while((shutdownnow == 0) && (count == -1 || totalsent < count)) {
		if(pthread_mutex_lock(&sdrmutex) != 0)
		{
			putErrmsg("Couldn't take sdr mutex in sendRequests.", NULL);
			fprintf(stderr, "Couldn't take sdr mutex in sendRequests.\n");
			shutdownnow = 1;
			bp_interrupt(recvsap);
			pthread_exit(NULL);
		}
		bundleZco = bping_new_ping();
		if(bundleZco == 0) {
			putErrmsg("Couldn't make new ping bundle.", NULL);
			fprintf(stderr, "Couldn't make new ping bundle.\n");
			shutdownnow = 1;
			bp_interrupt(recvsap);
			pthread_exit(NULL);
		}

		if(bp_send(xmitsap, dstEid, rptEid, ttl, priority, custodySwitch,
				rrFlags, 0, NULL, bundleZco, NULL) <= 0)
		{
			putErrmsg("bping can't send ping bundle.", NULL);
			fprintf(stderr, "bping can't send ping bundle.\n");
			shutdownnow = 1;
			bp_interrupt(recvsap);
			pthread_exit(NULL);
		}
		if(pthread_mutex_unlock(&sdrmutex) != 0)
		{
			putErrmsg("Couldn't give sdr mutex in sendRequests.", NULL);
			fprintf(stderr, "Couldn't give sdr mutex in sendRequests.\n");
			shutdownnow = 1;
			bp_interrupt(recvsap);
			pthread_exit(NULL);
		}

		++totalsent;    /* Successful send */
		if(interval > 0 && (count == -1 || totalsent < count)) {
			microsnooze((unsigned int) (interval*1000000));
		}
	}
	if(verbosity) fprintf(stderr, "Sent %d bundles.\n", totalsent);
	if(waitdelay > 0) {
		snooze(waitdelay);
		handleQuit();
	}
	return NULL;
}

static void parse_report_flags(int *srrFlags, const char *flags) {
	char myflags[1024];
	char *token;

	/* make a local copy of flags */
	strncpy(myflags, flags, 1023);
	myflags[1023] = '\0';

	/* parse flags */
	token = strtok(myflags, ",");
	while(token != NULL) {
		if (strcmp(token, "rcv") == 0)
			(*srrFlags) |= BP_RECEIVED_RPT;
		if (strcmp(token, "ct") == 0)
			(*srrFlags) |= BP_CUSTODY_RPT;
		if (strcmp(token, "fwd") == 0)
			(*srrFlags) |= BP_FORWARDED_RPT;
		if (strcmp(token, "dlv") == 0)
			(*srrFlags) |= BP_DELIVERED_RPT;
		if (strcmp(token, "del") == 0)
			(*srrFlags) |= BP_DELETED_RPT;
		if (strcmp(token, "ctr") == 0)
			custodySwitch = SourceCustodyRequired;

		token = strtok(NULL, ",");
	}
}

#if defined (ION_LWT)
int	bping(	saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	count = a1 ? strtol((char *) a1, NULL, 0) : -1;
	interval = a2 ? strtod((char *) a2, NULL) : 1;
	priority = a3 ? strtol((char *) a3, NULL, 0) : 0;
	waitdelay = a4 ? strtol((char *) a4, NULL, 0) : 10;
	if (a5)
	{
		parse_report_flags(&rrFlags, (char *) a5);
	}

	ttl = a6 ? strtol((char *) a6, NULL, 0) : 3600;
	verbosity = a7 ? strtol((char *) a7, NULL, 0) : 0;
	srcEid = a8 ? ((char *) a8) : NULL;
	dstEid = a9 ? ((char *) a9) : NULL;
	rptEid = a10 ? ((char *) a10) : NULL;
	if (srcEid == NULL || dstEid == NULL)
	{
		PUTS("Missing argument(s) for bping.  Ignored.");
		return BPING_EXIT_ERROR;
	}
#else
int main(int argc, char **argv)
{
	int ch;
	struct timeval tvStart, tvStop, tvDiff;

	signal(SIGINT, handleQuit);

	if(gettimeofday(&tvStart, NULL) < 0) {
		putErrmsg("Couldn't get start time.", NULL);
		perror("Couldn't get start time");
		exit(BPING_EXIT_ERROR);
	}

	while ((ch = getopt(argc, argv, "+c:i:hp:q:r:s:t:v")) != EOF) {
		switch(ch) {
			case 'c':
				count = atoi(optarg);
				break;
			case 'i':
				interval = atof(optarg);
				printf("read interval of %s ad %f\n", optarg, interval);
				break;
			case 'h':
				fprintf(stderr, usage);
				exit(BPING_EXIT_ERROR);
				break;
			case 'p':
				priority = atoi(optarg);
				break;
			case 'q':
				waitdelay = atoi(optarg);
				break;
			case 'r':
				parse_report_flags(&rrFlags, optarg);
				break;
			case 's':
				payloadSize = atoi(optarg);
				printf("payloadSize is: %d\n", payloadSize);
				break;
			case 't':
				ttl = atoi(optarg);
				break;
			case 'v':
				verbosity++;
				break;
			default:
				fprintf(stderr, "Couldn't handle option %c (%02x)\n", ch, ch);
				exit(BPING_EXIT_ERROR);
		}
	}

	if(argc - optind < 2) {
		fprintf(stderr, usage);
		exit(BPING_EXIT_ERROR);
	}

	srcEid = argv[optind];
	dstEid = argv[optind + 1];
	rptEid = (argc - optind > 2) ? argv[optind + 2] : NULL;
#endif

	if(pthread_mutex_init(&sdrmutex, NULL) != 0)
	{
		putErrmsg("Couldn't init sdrmutex.", NULL);
		fprintf(stderr, "Couldn't init sdrmutex.\n");
		exit(BPING_EXIT_ERROR);
	}

	if(verbosity) {
		fprintf(stderr, "Sending %d bundles from %s to %s (rpt-to: %s) "
				"every %0.2f seconds\n", count, srcEid, dstEid, 
				rptEid ? rptEid : "none", interval);
	}

	/* Attach to ION, open our source endpoint, get a hook to the SDR. */
	if (bp_attach() < 0) {
		putErrmsg("Can't attach to BP.", NULL);
		fprintf(stderr, "Can't attach to BP.\n");
		exit(BPING_EXIT_ERROR);
	}

	if (bp_open_source(srcEid, &xmitsap, 0) < 0) {
		putErrmsg("Can't open source endpoint.", srcEid);
		fprintf(stderr, "Can't open source endpoint (%s).\n", 
				srcEid);
		bp_detach();
		exit(BPING_EXIT_ERROR);
	}

	if (bp_open(srcEid, &recvsap) < 0) {
		bp_close(xmitsap);
		putErrmsg("Can't open reception endpoint.", srcEid);
		fprintf(stderr, "Can't open reception endpoint (%s).\n", 
				srcEid);
		bp_detach();
		exit(BPING_EXIT_ERROR);
	}

	sdr = bp_get_sdr();


	if(pthread_begin(&receiveResponsesThread, NULL, receiveResponses, 
				NULL) < 0) {
		putErrmsg("Can't make recvResponsesThread.", NULL);
		fprintf(stderr, "Can't make recvResponsesThread.\n");
		bp_close(xmitsap);
		bp_close(recvsap);
		bp_detach();
		exit(BPING_EXIT_ERROR);
	}

	if(pthread_begin(&sendRequestsThread, NULL, sendRequests, NULL) < 0) {
		putErrmsg("Can't make sendRequestsThread.", NULL);
		fprintf(stderr, "Can't make sendRequestsThread.\n");
		shutdownnow = 1;
		bp_interrupt(recvsap);
		bp_close(xmitsap);
		bp_close(recvsap);
		bp_detach();
		exit(BPING_EXIT_ERROR);
	}

	pthread_join(sendRequestsThread, NULL);
	pthread_join(receiveResponsesThread, NULL);

	bp_close(xmitsap);
	bp_close(recvsap);
	bp_detach();

	/* Calculate statistics. */
	if(gettimeofday(&tvStop, NULL) < 0) {
		putErrmsg("Couldn't get stop time.", NULL);
		perror("Couldn't get stop time");
		exit(BPING_EXIT_ERROR);
	}
	if(timeval_subtract(&tvDiff, &tvStop, &tvStart) < 0) {
		putErrmsg("Problem subtracting timevals.", NULL);
		fprintf(stderr, "Problem subtracting timevals\n");
		exit(BPING_EXIT_ERROR);
	}

	printf("%d bundles transmitted, %d bundles received, %.2f%% bundle"
			" loss, time %lu.%06lu s\n", totalsent, totalreceived,
			100.0*(1 - ((double)totalreceived)/((double)totalsent)),
			(unsigned long)tvDiff.tv_sec,
			(unsigned long)tvDiff.tv_usec);

	if(totalreceived > 0) {
		sum  /= totalreceived;
		sum2 /= totalreceived;
		dev   = llsqrt(sum2 - sum * sum);

		printf("rtt min/avg/max/sdev = "
				"%ld.%03lu/%ld.%03lu/%ld.%03lu/%ld.%03ld ms\n",
				min/1000L,          (unsigned long)(min)%1000UL, 
				(long)(sum/1000LL), (unsigned long)(sum)%1000UL,
				max/1000L,          (unsigned long)(max)%1000UL,
				dev/1000L,          (unsigned long)(dev)%1000UL);
	}

	if(totalreceived == totalsent) return BPING_EXIT_SUCCESS;
	return BPING_EXIT_NOTALLRESPONDED;
}
