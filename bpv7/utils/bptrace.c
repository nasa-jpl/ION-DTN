/*
	bptrace.c:	network trace utility, using BP status reports.
									*/
/*									*/
/*	Copyright (c) 2008, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#define _GNU_SOURCE
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
#else
int	main(int argc, char **argv)
{
	char	*ownEid;
	char	*destEid;
	char	*traceEid;
	int	ttl;
	char	*classOfService;
	char	*trace;
	char	*flagString;

	if (argc < 7)
	{
		PUTS("Usage:  bptrace <own EID> <destination EID> <report-to \
EID> <time to live (seconds)> <quality of service> '<trace text>' \
[<status report flag string>]");
		PUTS("\tquality of service: " \
BP_PARSE_QUALITY_OF_SERVICE_USAGE);
		PUTS("\tStatus report flag string is a sequence of status \
report flags separated by commas, with no embedded whitespace.");
		PUTS("\tEach status report flag must be one of the following: \
rcv, fwd, dlv, del.");
		PUTS("\tThe status reported in each bundle status report \
message will be the sum of the applicable status flags:");
		PUTS("\t\t 1 = bundle received (rcv)");
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
	trace = argv[6];
	flagString = argc > 7 ? argv[7] : NULL;
#endif
	return run_bptrace(ownEid, destEid, traceEid, ttl, classOfService,
			trace, flagString);
}
