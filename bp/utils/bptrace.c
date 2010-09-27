/*
	bptrace.c:	network trace utility, using BP status reports.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

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
			int ttl, char *svcClass, char *traceText, char *flags)
{
	int		priority = 0;
	BpExtendedCOS	extendedCOS = { 0, 0, 0 };
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	int		srrFlags = 0;
	BpSAP		sap;
	Sdr		sdr;
	int		msgLength = strlen(traceText) + 1;
	Object		msg;
	Object		traceZco;
	Object		newBundle;

	if (!bp_parse_class_of_service(svcClass, &extendedCOS, &custodySwitch,
			&priority))
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

	sdr = bp_get_sdr();
	sdr_begin_xn(sdr);
	msg = sdr_malloc(sdr, msgLength);
	if (msg == 0)
	{
		sdr_cancel_xn(sdr);
		bp_close(sap);
		putErrmsg("No space for bptrace text.", NULL);
		return 0;
	}

	sdr_write(sdr, msg, traceText, msgLength);
	traceZco = zco_create(sdr, ZcoSdrSource, msg, 0, msgLength);
	if (sdr_end_xn(sdr) < 0 || traceZco == 0)
	{
		bp_close(sap);
		putSysErrmsg("bptrace can't create ZCO", NULL);
		return 0;
	}

	if (bp_send(sap, BP_BLOCKING, destEid, reportToEid, ttl, priority,
			custodySwitch, srrFlags, 0, &extendedCOS,
			traceZco, &newBundle) < 1)
	{
		putErrmsg("bptrace can't send message.", NULL);
	}

	bp_close(sap);
	bp_detach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	bptrace(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*ownEid = (char *) a1;
	char	*destEid = (char *) a2;
	char	*traceEid = (char *) a3;
	int	ttl = a4 ? strtol((char *) a4, NULL, 0) : 0;
	char	*classOfService = (char *) a5;
	char	*traceText = (char *) a6;
	char	*flagString = (char *) a7;

	if (ownEid == NULL || destEid == NULL || classOfService == NULL
	|| traceText == NULL)
	{
		PUTS("Missing argument(s) for bptrace.  Ignored.");
		return 0;
	}
#else
int	main(int argc, char **argv)
{
	char	*ownEid;
	char	*destEid;
	char	*traceEid;
	int	ttl;
	char	*classOfService;
	char	*traceText;
	char	*flagString;

	if (argc < 7)
	{
		PUTS("Usage:  bptrace <own EID> <destination EID> <report-to \
EID> <time to live (seconds)> <class of service> '<trace text>' \
[<status report flag string>]");
		PUTS("\tclass of service: " BP_PARSE_CLASS_OF_SERVICE_USAGE);
		PUTS("\tStatus report flag string is a sequence of status \
report flags separated by commas, with no embedded whitespace.");
		PUTS("\tEach status report flag must be one of the following: \
rcv, ct, fwd, dlv, del.");
		PUTS("\tThe status reported in each bundle status report \
message will be the sum of the applicable status flags:");
		PUTS("\t\t 1 = bundle received (rcv)");
		PUTS("\t\t 2 = bundle custody accepted (ct)");
		PUTS("\t\t 4 = bundle forwarded (fwd)");
		PUTS("\t\t 8 = bundle delivered (dlv)");
		PUTS("\t\t16 = bundle deleted (del)");
		return 0;
	}

	ownEid = argv[1];
	destEid = argv[2];
	traceEid = argv[3];
	ttl = atoi(argv[4]);
	classOfService = argv[5];
	traceText = argv[6];
	flagString = argc > 7 ? argv[7] : NULL;
#endif
	return run_bptrace(ownEid, destEid, traceEid, ttl, classOfService,
			traceText, flagString);
}
