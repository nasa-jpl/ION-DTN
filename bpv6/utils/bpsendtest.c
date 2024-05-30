/*
	bpsendtest.c:	test program to send a file as a bundle repeatedly to bprecvtest.c.
		also sends a pilot bundle at the beginning of transmission to inform bprecvtest 
		to start its clock
														*/
/*														*/
/*	Modified from bpsendfile.c by Silas Springer 		*/
/*	Random file generation modified from garbage.c 
	Which was originally writen by Dr. Shawn Ostermann	*/


#include <bp.h>

static int	run_bpsendtest(char *ownEid, char *destEid, char *fileName,
			int ttl, char *svcClass, int repetitions)
{
	int		priority = 0;
	BpAncillaryData	ancillaryData = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	BpSAP		sap = NULL;
	Sdr		sdr;
	Object		fileRef;
	struct stat	statbuf;
	int		aduLength;
	Object		bundleZco;
	char		progressText[300];
	Object		newBundle;

	if (svcClass == NULL)
	{
		priority = BP_STD_PRIORITY;
	}
	else
	{
		if (!bp_parse_quality_of_service(svcClass, &ancillaryData,
				&custodySwitch, &priority))
		{
			putErrmsg("Invalid class of service for bpsendtest.",
					svcClass);
			return 0;
		}
	}

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (ownEid)
	{
		if (bp_open(ownEid, &sap) < 0)
		{
			putErrmsg("Can't open own endpoint.", ownEid);
			return 0;
		}
	}

	writeMemo("[i] bpsendtest is running.");
	if (stat(fileName, &statbuf) < 0)
	{
		if (sap)
		{
			bp_close(sap);
		}

		putSysErrmsg("Can't stat the file", fileName);
		return 0;
	}

	aduLength = statbuf.st_size;
	if (aduLength == 0)
	{
		writeMemoNote("[?] bpsendtest can't send file of length zero",
				fileName);
		return 0;
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));
	if (sdr_heap_depleted(sdr))
	{
		sdr_exit_xn(sdr);
		if (sap)
		{
			bp_close(sap);
		}

		putErrmsg("Low on heap space, can't send file.", fileName);
		return 0;
	}
	fileRef = zco_create_file_ref(sdr, fileName, NULL, ZcoOutbound);
	if (sdr_end_xn(sdr) < 0 || fileRef == 0)
	{
		if (sap)
		{
			bp_close(sap);
		}

		putErrmsg("bpsendtest can't create file ref.", NULL);
		return 0;
	}

	/* Create and send pilot bundle */
	
	CHKZERO(sdr_begin_xn(sdr));
	Object pilotAduString = sdr_string_create(sdr, "Go.");
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bpdriver can't create pilot ADU string.", NULL);
		bp_close(sap);
		return 0;
	}

	bundleZco = ionCreateZco(ZcoSdrSource, pilotAduString, 0, 
			sdr_string_length(sdr, pilotAduString),
			BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (Object) ERROR)
	{
		putErrmsg("bpdriver can't create pilot ADU.", NULL);
		bp_close(sap);
		return 0;
	}

	if (bp_send(sap, destEid, NULL, ttl, BP_STD_PRIORITY, custodySwitch, 0,
			0, NULL, bundleZco, &newBundle) < 1)
	{
		putErrmsg("bpdriver can't send pilot bundle.",
				itoa(aduLength));
		bp_close(sap);
		return 0;
	}

	printf("Sent pilot bundle.\n");

	/* end pilot bundle shenanigans */

	isprintf(progressText, sizeof progressText, 
		"[i] bpsendtest is sending '%s', size %d, %d times.", 
		fileName, aduLength, repetitions);
	writeMemo(progressText);

	int i;
	for (i = repetitions; i > 0; --i){
		bundleZco = ionCreateZco(ZcoFileSource, fileRef, 0, aduLength,
				priority, ancillaryData.ordinal, ZcoOutbound, NULL);
		if (bundleZco == 0 || bundleZco == (Object) ERROR) {
			putErrmsg("bpsendtest can't create ZCO.", NULL);
		}
		else
		{
			if (bp_send(sap, destEid, NULL, ttl, priority, custodySwitch,
				0, 0, &ancillaryData, bundleZco, &newBundle) <= 0) {
				putErrmsg("bpsendtest can't send file in bundle.",
						itoa(aduLength));
			}
		}

		CHKZERO(sdr_begin_xn(sdr));
		zco_destroy_file_ref(sdr, fileRef);
		if (sdr_end_xn(sdr) < 0) {
			putErrmsg("bpsendtest can't destroy file reference.", NULL);
		}

	}
	isprintf(progressText, sizeof progressText,
		"[i] bpsendtest sent '%s', size %d, %d times.",
		fileName, aduLength, repetitions);
	writeMemo(progressText);

	if (sap)
	{
		bp_close(sap);
	}

	PUTS("Stopping bpsendtest.");
	writeMemo("[i] bpsendtest has stopped.");
	writeErrmsgMemos();
	bp_detach();
	return 0;
}

#if defined (ION_LWT)
int	bpsendtest(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ownEid = (char *) a1;
	char	*destEid = (char *) a2;
	char	*fileName = (char *) a3;
	char	*classOfService = (char *) a4;
	int		ttl = atoi((char *) a5);
	int 	repetitions = 1;

	if (ownEid == NULL || destEid == NULL || fileName == NULL)
	{
		PUTS("usage: %s [-rep <repetitions>] [-ttl <ttl>] [-qos <qos>] <srcEid> <destEid> <fileName>\n");
		PUTS("\tclass of service: " BP_PARSE_QUALITY_OF_SERVICE_USAGE);
		return 0;
	}

	if (strcmp(ownEid, "dtn:none") == 0)	/*	Anonymous.	*/
	{
		ownEid = NULL;
	}

	return run_bpsendtest(ownEid, destEid, fileName, ttl, classOfService, repetitions);
}
#else

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

	fprintf(stderr, "usage: %s [-rep <repetitions>] [-ttl <ttl>] [-qos <qos>] [-nofile] <srcEid> <destEid> <file>\n", progname);
	fprintf(stderr, "-rep <repetitions>  \tSpecifies the number of times to send the given file as a bundle.\n");
	fprintf(stderr, "-ttl <ttl>  \tInteger number of seconds after which bundle should expire. Default: 100\n");
	fprintf(stderr, "-qos <qos>  \tQuality of service. Default: 0.1 \n\t\t" BP_PARSE_QUALITY_OF_SERVICE_USAGE "\n");
	fprintf(stderr, "-nofile <qos>  \tspecify to interpret <file> as a bundle length in bytes instead of as a filename.\n");

	fprintf(stderr, "<srcEid>  \tSource endpoint to send from. e.g. 'ipn:2.4' \n");
	fprintf(stderr, "<destEid> \tDestination endpoint to send to. e.g. 'ipn:3.2' \n");
	fprintf(stderr, "<file>    \tPath to file to send as a bundle. If -nofile is specified, will instead be interpreted as a number of bytes defining the length of random data to generate for the bundle. Random data generated for the bundle will be saved in file 'bpsendtestRandFile'. WILL OVERWRITE files of the same name.\n");
    exit(-1);
}

#define FILENAME "bpsendtestRandFile"

void makegarbagefile(int len) {
	FILE* f;
	int i, ch;
	if ((f = fopen(FILENAME,"w+")) == NULL) {
		perror(FILENAME);
		exit(-1);
    }

	for (i = 1; i <= len; ++i) {
	    ch = rand()%256; // get random byte
	    if (fputc(ch,f) != ch) {
			perror("fputc");
			exit(-2);
	    }
	}
	fclose(f);
}

int	main(int argc, char **argv)
{
	char	*ownEid = NULL;
	char	*destEid = NULL;
	char	fileName[PATH_MAX];
	char	*classOfService = "0.1";
	int 	numbytes = 0;
	int		ttl = 300;
	int 	repetitions = 1;
	int		isfilename = 1; // true

	if (argc < 4)
		usage(argv[0], "too few arguments.");
	
	int i = 1;
	for (i=1; i < (argc -3); ++i)
	{
		if (*argv[i] == '-') {
			if (strcmp(argv[i],"-ttl") == 0) {
				if (i+1 >= argc) usage(argv[0],"-ttl requires argument");
				ttl = atoi(argv[++i]);
				if (ttl < 1) usage(argv[0], "-ttl requires a positive integer argument");
				continue;	/* iterate back to for (i=1... loop */
	    	}
			else if (strcmp(argv[i],"-qos") == 0) {
				if (i+1 >= argc) usage(argv[0],"-qos requires argument");
				classOfService = argv[++i];
				continue;	/* iterate back to for (i=1... loop */
	    	}
			else if (strcmp(argv[i],"-rep") == 0) {
				if (i+1 >= argc) usage(argv[0],"-rep requires argument");
				repetitions = atoi(argv[++i]);
				if (repetitions < 1) usage(argv[0], "-rep requires a positive integer argument");
				continue;	/* iterate back to for (i=1... loop */
	    	}
			else if (strcmp(argv[i], "-nofile") == 0) {
				isfilename = 0; // false 
				continue;
			}
			else if (strcmp(argv[i],"-h") == 0) {
				usage(argv[0], "");
	    	}
			else {
				usage(argv[0], "unrecognized argument.");
			}
		}
		else {
			usage(argv[0], "unrecognized argument.");
		}
	}
	ownEid = argv[i];
	destEid = argv[++i];
	if(!isfilename){
		numbytes = atoi(argv[++i]);
		if(numbytes < 1) usage(argv[0], "<file> must be positive integer when using -nofile option.");
		makegarbagefile(numbytes);
		if(getcwd(fileName, sizeof(fileName))) {
			if(*(fileName + strlen(fileName)-1) != '/') strcat(fileName, "/");
			strcat(fileName, FILENAME);
		} else {
			perror("getcwd");
			exit(-1);
		}
	} else {
		strcpy(fileName, argv[++i]);
	}

	if (!(ownEid && destEid && strlen(fileName)))
	{
		usage(argv[0], "insufficient arguments.");
	}

	if (strcmp(ownEid, "dtn:none") == 0)	/*	Anonymous.	*/
	{
		ownEid = NULL;
	}

	return run_bpsendtest(ownEid, destEid, fileName, ttl, classOfService, repetitions);
}
#endif
