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

static void	printBundle(Sdr sdr, Bundle *bundle)
{
	char	*eid;
	char	*dictionary;
	char	buf[300];

	dictionary = retrieveDictionary(bundle);
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
			"Priority                %lu",
			COS_FLAGS(bundle->bundleProcFlags) & 0x03);
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
	oK(printEid(&(bundle->destination), dictionary, &eid));
	isprintf(buf, sizeof buf, "Destination EID '%s'", eid);
	PUTS(buf);
	MRELEASE(eid);
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
	releaseDictionary(dictionary);
	printExtensions(sdr, bundle->extensions[0]);
	printPayload(sdr, bundle);
	printExtensions(sdr, bundle->extensions[1]);
	PUTS("**** End of bundle");
}

static void	printUsage()
{
	PUTS("Usage: bplist [{count | detail} [<node ID (EID)>/<priority>]]");
}

#if defined (ION_LWT)
int	bplist(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char		*rpt = (char *) a1;
	char		*queue = (char *) a2;
#else
int	main(int argc, char **argv)
{
	char		*rpt = argc > 1 ? argv[1] : NULL;
	char		*queue = argc > 2 ? argv[2] : NULL;
#endif
	int		count = 0;
	char		*cursor;
	char		*eid = NULL;
	int		priority = 0;
	char		msgbuf[256];
	Sdr		sdr;
	BpDB		*bpConstants;
	Object		list;
	VPlan		*vplan;
	PsmAddress	vplanElt;
	Object		elt;
	Object		addr;
			OBJ_POINTER(BpEvent, event);
			OBJ_POINTER(Bundle, bundle);
	int		bundlesCount = 0;
			OBJ_POINTER(BpPlan, plan);

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 1;
	}

	if (rpt)
	{
		if (strcmp(rpt, "count") == 0)
		{
			count = 1;
		}
		else if (strcmp(rpt, "detail") != 0)
		{
			printUsage();
			return 0;
		}

		if (queue)
		{
			eid = queue;
			cursor = strchr(eid, '/');
			if (cursor == NULL)
			{
				printUsage();
				return 0;
			}

			*cursor = '\0';
			priority = atoi(cursor + 1);
			if (priority < 0 || priority > 2)
			{
				printUsage();
				return 0;
			}

			isprintf(msgbuf, sizeof msgbuf, "reporting on bundles \
queued for node '%.255s', priority %d.", eid, priority);
			writeMemo(msgbuf);
		}
	}

	sdr = bp_get_sdr();
	CHKZERO(sdr_begin_xn(sdr));	/*	Lock db for duration.	*/
	isignal(SIGINT, handleQuit);
	if (eid == NULL)		/*	All bundles.		*/
	{
		bpConstants = getBpConstants();
		list = bpConstants->timeline;
		for (elt = sdr_list_first(sdr, bpConstants->timeline); elt;
				elt = sdr_list_next(sdr, elt))
		{
			addr = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BpEvent, event, addr);
			if (event->type != expiredTTL)
			{
				continue;	/*	Not bundle ref.	*/
			}

			GET_OBJ_POINTER(sdr, Bundle, bundle, event->ref);
			if (count)
			{
				bundlesCount++;
			}
			else
			{
				printBundle(sdr, bundle);
			}
		}

		if (count)
		{
			isprintf(msgbuf, sizeof msgbuf, "Count is %ld.",
					bundlesCount);
			writeMemo(msgbuf);
		}
	}
	else				/*	Bundles in one queue.	*/
	{
		findPlan(eid, &vplan, &vplanElt);
		if (vplanElt == 0)
		{
			writeMemo("No such egress plan.");
		}
		else
		{
			addr = sdr_list_data(sdr, vplan->planElt);
			GET_OBJ_POINTER(sdr, BpPlan, plan, addr);
			CHKZERO(plan);
			switch (priority)
			{
			case 2:
				list = plan->urgentQueue;
				break;
		
			case 1:
				list = plan->stdQueue;
				break;
		
			default:
				list = plan->bulkQueue;
			}

			if (count)
			{
				isprintf(msgbuf, sizeof msgbuf, "Count is %ld.",
						sdr_list_length(sdr, list));
				writeMemo(msgbuf);
			}
			else
			{
				for (elt = sdr_list_first(sdr, list); elt;
						elt = sdr_list_next(sdr, elt))
				{
					addr = sdr_list_data(sdr, elt);
					GET_OBJ_POINTER(sdr, Bundle, bundle,
							addr);
					printBundle(sdr, bundle);
				}
			}
		}
	}

	if (sdr_end_xn(sdr))
	{
		putErrmsg("Failed listing bundles.", NULL);
	}

	writeErrmsgMemos();
	bp_detach();
	return 0;
}
