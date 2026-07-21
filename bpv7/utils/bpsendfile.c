/*
	bpsendfile.c:	test program to send a file as a bundle.
									*/
/*									*/
/*	Copyright (c) 2006, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

static void	parseSrrFlag(int *srrFlags, char *arg)
{
	if (strcmp(arg, "rcv") == 0)
		(*srrFlags) |= BP_RECEIVED_RPT;
	if (strcmp(arg, "fwd") == 0)
		(*srrFlags) |= BP_FORWARDED_RPT;
	if (strcmp(arg, "dlv") == 0)
		(*srrFlags) |= BP_DELIVERED_RPT;
	if (strcmp(arg, "del") == 0)
		(*srrFlags) |= BP_DELETED_RPT;
}

static void	parseSrrFlags(int *srrFlags, char *flagString)
{
	char	*cursor = flagString;
	char	*comma;

	while (1)
	{
		comma = strchr(cursor, ',');
		if (comma)
		{
			*comma = '\0';
			parseSrrFlag(srrFlags, cursor);
			*comma = ',';
			cursor = comma + 1;
			continue;
		}

		parseSrrFlag(srrFlags, cursor);
		return;
	}
}

static int run_bpsendfile(char *ownEid, char *destEid, char *rptToEid,
		char *fileName, int ttl, char *svcClass, int srrFlags)
{
	int		priority = 0;
	BpAncillaryData	ancillaryData = {0};
	BpCustodySwitch	custodySwitch = NoCustodyRequested;
	BpSAP		sap = NULL;
	Sdr		sdr;
	SdrObject	fileRef;
	struct stat	statbuf;
	int		aduLength;
	SdrObject	bundleZco;
	char		progressText[300];
	SdrObject	newBundle;
	int		sendFailed = 0;

	if (svcClass == NULL)
	{
		priority = BP_STD_PRIORITY;
	}
	else
	{
		if (!bp_parse_quality_of_service(svcClass, &ancillaryData,
				&custodySwitch, &priority))
		{
			putErrmsg("Invalid class of service for bpsendfile.",
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

	writeMemo("[i] bpsendfile is running.");
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
		writeMemoNote("[?] bpsendfile can't send file of length zero",
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

		putErrmsg("bpsendfile can't create file ref.", NULL);
		return 0;
	}

	bundleZco = ionCreateZco(ZcoFileSource, fileRef, 0, aduLength,
			priority, ancillaryData.ordinal, ZcoOutbound, NULL);
	if (bundleZco == 0 || bundleZco == (SdrObject) ERROR)
	{
		putErrmsg("bpsendfile can't create ZCO.", NULL);
	}
	else
	{
		isprintf(progressText, sizeof progressText, "[i] bpsendfile \
is sending '%s', size %d, to %s.", fileName, aduLength, destEid);
		writeMemo(progressText);
		if (bp_send(sap, destEid, rptToEid, ttl, priority, custodySwitch,
			srrFlags, 0, &ancillaryData, bundleZco, &newBundle) <= 0)
		{
			putErrmsg("bpsendfile can't send file in bundle.",
					itoa(aduLength));
			fprintf(stderr,
				"bpsendfile: failed to send '%s'.\n",
				fileName);
			CHKZERO(sdr_begin_xn(sdr));
			zco_destroy(sdr, bundleZco);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy ZCO.", NULL);
			}

			sendFailed = 1;
		}
		else
		{
			isprintf(progressText, sizeof progressText,
					"[i] bpsendfile sent '%s', size %d.",
					fileName, aduLength);
			writeMemo(progressText);
		}
	}

	CHKZERO(sdr_begin_xn(sdr));
	zco_destroy_file_ref(sdr, fileRef);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("bpsendfile can't destroy file reference.", NULL);
	}

	if (sap)
	{
		bp_close(sap);
	}

	PUTS("Stopping bpsendfile.");
	writeMemo("[i] bpsendfile has stopped.");
	writeErrmsgMemos();
	bp_detach();
	return sendFailed;
}

#if defined (ION_LWT)
int	bpsendfile(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	*ownEid = (char *) a1;
	char	*destEid = (char *) a2;
	char	*fileName = (char *) a3;
	char	*classOfService = (char *) a4;
	int	ttl = atoi((char *) a5);
	char    *rptToEid = (char *) a6;
	char    *flagString = (char *) a7;
#else
int	main(int argc, char **argv)
{
	char	*ownEid = NULL;
	char	*destEid = NULL;
	char	*fileName = NULL;
	char	*classOfService = NULL;
	int	ttl = 300;
	char    *rptToEid = NULL;
	char    *flagString = NULL;

	if (argc > 8) argc = 8;
	switch (argc)
	{
	case 8:
		flagString = argv[7];
		/* FALLTHROUGH */
	case 7:
		rptToEid = argv[6];
		/* FALLTHROUGH */
	case 6:
		ttl = atoi(argv[5]);
		/* FALLTHROUGH */

	case 5:
		classOfService = argv[4];
		/* FALLTHROUGH */

	case 4:
		fileName = argv[3];
		/* FALLTHROUGH */

	case 3:
		destEid = argv[2];
		/* FALLTHROUGH */

	case 2:
		ownEid = argv[1];
		/* FALLTHROUGH */

	default:
		break;
	}
#endif
	if (ownEid == NULL || destEid == NULL || fileName == NULL)
	{
		PUTS("Usage: bpsendfile <own endpoint ID> <destination \
endpoint ID> <file name> [<class of service> [<time to live (seconds)> \
[<report-to endpoint ID> [<status report flags>]]]]");
		PUTS("\tclass of service: " BP_PARSE_QUALITY_OF_SERVICE_USAGE);
		PUTS("\tstatus report flags: comma-separated list of rcv,fwd,dlv,del");
		return 0;
	}

	if (strcmp(ownEid, "dtn:none") == 0)	/*	Anonymous.	*/
	{
		ownEid = NULL;
	}

	{
		int	srrFlags = 0;

		if (flagString)
		{
			parseSrrFlags(&srrFlags, flagString);
		}

		return run_bpsendfile(ownEid, destEid, rptToEid, fileName,
				ttl, classOfService, srrFlags);
	}
}
