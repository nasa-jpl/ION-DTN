/*
	bplist.c:	bundle timeline listing utility.
									*/
/*									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>
#include <bei.h>

static void	handleQuit()
{
	oK(sdr_end_xn(bp_get_sdr()));
	isignal(SIGINT, SIG_DFL);
	sm_TaskKill(sm_TaskIdSelf(), SIGINT);
}

static void	printDictionary(char *dictionary, int dictionaryLength)
{
	int	offset = 0;
	char	*cursor = dictionary;
	int	length;
	char	entry[300];

	PUTS("Dictionary:");
	while (offset < dictionaryLength)
	{
		length = strlen(cursor) + 1;
		isprintf(entry, sizeof entry, "(%3ul) %s", offset, cursor);
		PUTS(entry);
		offset += length;
		cursor += length;
	}
}

static void	printBytes(char *text, int length)
{
	char	line[100];
	int	bytesPrinted = 0;
	char	*cursor = text;
	int	i;
	int	high;
	int	low;
	int	digit;
	char	digits[16] = "0123456789abcdef";

	while (1)
	{
		isprintf(line, sizeof line, "(%6d)", bytesPrinted);
		memset(line + 8, ' ', 92);
		high = 10;
		low = 11;
		i = 0;
		while (i < 20)
		{
			if (bytesPrinted == length)
			{
				/*	Print last line.		*/

				line[57 + i] = '\0';
				PUTS(line);
				return;
			}

			digit = (((unsigned char) *cursor) >> 4) & 0x0f;
			line[high] = digits[digit];
			digit = ((unsigned char) *cursor) & 0x0f;
			line[low] = digits[digit];
			line[57 + i] = isprint((int) *cursor) ? *cursor : '.';
			i++;
			bytesPrinted++;
			cursor++;
			if ((((high - 10) / 3) % 3) == 2)
			{
				high += 3; 
			}
			else
			{
				high += 2;
			}

			low = high + 1;
		}

		line[57 + i] = '\0';
		PUTS(line);
	}
}

static void	printExtensions(Sdr sdr, Object extensions)
{
	int	buflen = 0;
	char	*buf = NULL;
	Object	elt;
	Object	addr;
		OBJ_POINTER(ExtensionBlock, blk);

	for (elt = sdr_list_first(sdr, extensions); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, ExtensionBlock, blk, addr);
		if (blk->length == 0)
		{
			continue;
		}

		if (blk->length > buflen)
		{
			if (buf)
			{
				MRELEASE(buf);
			}

			buflen = blk->length;
			buf = MTAKE(buflen);
			if (buf == NULL)
			{
				putErrmsg("bplist can't allocate buffer.",
						itoa(buflen));
				return;
			}
		}

		PUTS("****** Extension");
		sdr_read(sdr, buf, blk->bytes, blk->length);
		printBytes(buf, blk->length);
	}

	if (buf)
	{
		MRELEASE(buf);
	}
}

static void	printPayload(Sdr sdr, Bundle *bundle)
{
	int		buflen;
	char		*buf;
	ZcoReader	reader;
	int		len;

	buflen = bundle->payload.length;
	buf = MTAKE(buflen);
	if (buf == NULL)
	{
		putErrmsg("bplist can't allocate buffer.", itoa(buflen));
		return;
	}

	zco_start_receiving(bundle->payload.content, &reader);
	len = zco_receive_source(sdr, &reader, bundle->payload.length, buf);
	if (len < 0)
	{
		putErrmsg("bplist can't read ZCO source.", NULL);
		return;
	}

	PUTS("****** Payload");
	printBytes(buf, bundle->payload.length);
	MRELEASE(buf);
}

static void	printQueueState(Sdr sdr, Bundle *bundle)
{
	Object	queue;
	Object	planAddr;
	BpPlan	planBuf;
	char	buf[300];

	if (bundle->dlvQueueElt)
	{
		PUTS("****** Queued for delivery.");
		return;
	}

	if (bundle->fwdQueueElt)
	{
		PUTS("****** Queued for proximate destination selection.");
		return;
	}

	if (bundle->planXmitElt)
	{
		queue = sdr_list_list(sdr, bundle->planXmitElt);
		planAddr = sdr_list_user_data(sdr, queue);
		if (planAddr == 0)
		{
			PUTS("****** In limbo.");
			return;
		}

		sdr_read(sdr, (char *) &planBuf, planAddr, sizeof(BpPlan));
		isprintf(buf, sizeof buf, "Queued for forwarding to '%s'.",
				planBuf.neighborEid);
		PUTS(buf);
		return;
	}

	if (bundle->ductXmitElt)
	{
		PUTS("****** Queued for transmission at convergence layer.");
		return;
	}

	/*	Not queued anywhere.					*/

	PUTS("****** Awaiting completion of convergence-layer transmission.");
	return;
}

static void	printBundle(Sdr sdr, Bundle *bundle, char *dictionary,
			char *destination, int priority)
{
	char	*eid;
	char	buf[300];

	PUTS("\n**** Bundle");
	oK(printEid(&(bundle->id.source), dictionary, &eid));
	isprintf(buf, sizeof buf, "Source EID      '%s'", eid);
	PUTS(buf);
	MRELEASE(eid);
	isprintf(buf, sizeof buf,
		"Creation sec   %10lu   count %10lu   frag offset %10lu",
			bundle->id.creationTime.seconds,
			bundle->id.creationTime.count,
			bundle->id.fragmentOffset);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"- is a fragment:        %d", bundle->bundleProcFlags
		       	& BDL_IS_FRAGMENT ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"- is admin:             %d", bundle->bundleProcFlags
		       	& BDL_IS_ADMIN ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"- does not fragment:    %d", bundle->bundleProcFlags
			& BDL_DOES_NOT_FRAGMENT ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"- is custodial:         %d", bundle->bundleProcFlags
			& BDL_IS_CUSTODIAL ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"- dest is singleton:    %d", bundle->bundleProcFlags
			& BDL_DEST_IS_SINGLETON ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"- app ack requested:    %d", bundle->bundleProcFlags
			& BDL_APP_ACK_REQUEST ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"Priority                %lu", priority);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"Ordinal                 %d",
		       	bundle->ancillaryData.ordinal);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"Unreliable:             %d",
			bundle->ancillaryData.flags & BP_BEST_EFFORT ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"Critical:               %d",
			bundle->ancillaryData.flags
			& BP_MINIMUM_LATENCY ? 1 : 0);
	PUTS(buf);
	isprintf(buf, sizeof buf, "Destination EID '%s'", destination);
	PUTS(buf);
	oK(printEid(&(bundle->reportTo), dictionary, &eid));
	isprintf(buf, sizeof buf, "Report-to EID   '%s'", eid);
	PUTS(buf);
	MRELEASE(eid);
	oK(printEid(&(bundle->custodian), dictionary, &eid));
	isprintf(buf, sizeof buf, "Custodian EID   '%s'", eid);
	PUTS(buf);
	MRELEASE(eid);
	isprintf(buf, sizeof buf,
			"Expiration sec %10lu", bundle->expirationTime);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"Total ADU len  %10lu", bundle->totalAduLength);
	PUTS(buf);
	isprintf(buf, sizeof buf,
			"Dictionary len %10lu", bundle->dictionaryLength);
	PUTS(buf);
	printDictionary(dictionary, bundle->dictionaryLength);
	printExtensions(sdr, bundle->extensions[0]);
	printPayload(sdr, bundle);
	printExtensions(sdr, bundle->extensions[1]);
	printQueueState(sdr, bundle);
	PUTS("**** End of bundle");
}

static void	printUsage()
{
	PUTS("Usage: bplist [{count | detail} [<node ID (EID)>[/<priority>]]]");
}

#if defined (ION_LWT)
int	bplist(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char		*rptType = (char *) a1;
	char		*filter = (char *) a2;
#else
int	main(int argc, char **argv)
{
	char		*rptType = argc > 1 ? argv[1] : NULL;
	char		*filter = argc > 2 ? argv[2] : NULL;
#endif
	char		*cursor;
	char		*destination = NULL;
	int		priority = -1;
	char		msgbuf[256];
	Sdr		sdr;
	int		bundlesCount = 0;
	BpDB		*bpConstants;
	Object		elt;
	Object		addr;
			OBJ_POINTER(BpEvent, event);
			OBJ_POINTER(Bundle, bundle);
	char		*dictionary;
	char		*eid = NULL;
	int		pri;

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 1;
	}

	if (rptType == NULL)
	{
		rptType = "detail";	/*	Default.		*/
		PUTS("Reporting detail of all bundles.");
	}
	else
	{
		if (strcmp(rptType, "count") != 0
		&& strcmp(rptType, "detail") != 0)
		{
			/*	Report type not specified.		*/

			printUsage();
			return 0;
		}

		if (filter)
		{
			destination = filter;
			cursor = strchr(destination, '/');
			if (cursor)
			{
				*cursor = '\0';
				priority = atoi(cursor + 1);
				if (priority < 0 || priority > 2)
				{
					printUsage();
					return 0;
				}
			}

			isprintf(msgbuf, sizeof msgbuf, "Reporting %s of all \
bundles destined for '%.255s', priority %d.", rptType, destination, priority);
		}
		else
		{
			isprintf(msgbuf, sizeof msgbuf, "Reporting %s of \
all bundles.", rptType);
		}

		PUTS(msgbuf);
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));	/*	Lock db for duration.	*/
	isignal(SIGINT, handleQuit);
	bpConstants = getBpConstants();
	for (elt = sdr_list_first(sdr, bpConstants->timeline); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BpEvent, event, addr);
		if (event->type != expiredTTL)
		{
			continue;	/*	Not bundle ref.		*/
		}

		/*	Event is a reference to a bundle.		*/

		GET_OBJ_POINTER(sdr, Bundle, bundle, event->ref);
		dictionary = retrieveDictionary(bundle);
		oK(printEid(&(bundle->destination), dictionary, &eid));
		if (eid == NULL)
		{
			PUTS("Can't print destination endpoint for bundle!");
			break;
		}

		pri = COS_FLAGS(bundle->bundleProcFlags) & 0x03;
		if (destination)
		{
			/*	Limit to bundles for one destination.	*/

			if (strcmp(eid, destination) != 0)
			{
				/*	Doesn't match.			*/

				MRELEASE(eid);
				continue;
			}

			if (priority >= -1)
			{
				/*	Limit to bundles of one COS.	*/

				if (pri != priority)
				{
					/*	Doesn't match.		*/

					MRELEASE(eid);
					continue;
				}
			}
		}

		/*	This is one of the bundles we're looking for.	*/

		bundlesCount++;
		if (strcmp(rptType, "detail") == 0)
		{
			printBundle(sdr, bundle, dictionary, eid, pri);
		}

		MRELEASE(eid);
		releaseDictionary(dictionary);
	}

	if (strcmp(rptType, "count") == 0)
	{
		isprintf(msgbuf, sizeof msgbuf, "Count is %ld.",
				bundlesCount);
		PUTS(msgbuf);
	}

	sdr_exit_xn(sdr);
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
