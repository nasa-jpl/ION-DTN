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

static void	handleQuit(int signum)
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
	if (buflen > 100)
	{
		buflen = 100;
	}

	buf = MTAKE(buflen);
	if (buf == NULL)
	{
		putErrmsg("bplist can't allocate buffer.", itoa(buflen));
		return;
	}

	zco_start_receiving(bundle->payload.content, &reader);
	len = zco_receive_source(sdr, &reader, buflen, buf);
	if (len < 0)
	{
		putErrmsg("bplist can't read ZCO source.", NULL);
		return;
	}

	PUTS("****** Payload");
	printBytes(buf, buflen);
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

static void	printBundle(Object bundleObj)
{
	Sdr	sdr = bp_get_sdr();
		OBJ_POINTER(Bundle, bundle);
	char	*dictionary;
	char	*eid;
	char	buf[300];
	int	priority;

	GET_OBJ_POINTER(sdr, Bundle, bundle, bundleObj);
	dictionary = retrieveDictionary(bundle);

	PUTS("\n**** Bundle");

	oK(printEid(&(bundle->id.source), dictionary, &eid));
	if (eid)
	{
		isprintf(buf, sizeof buf, "Source EID      '%s'", eid);
		PUTS(buf);
		MRELEASE(eid);
	}

	oK(printEid(&(bundle->destination), dictionary, &eid));
	if (eid)
	{
		isprintf(buf, sizeof buf, "Destination EID '%s'", eid);
		PUTS(buf);
		MRELEASE(eid);
	}

	oK(printEid(&(bundle->reportTo), dictionary, &eid));
	if (eid)
	{
		isprintf(buf, sizeof buf, "Report-to EID   '%s'", eid);
		PUTS(buf);
		MRELEASE(eid);
	}

	oK(printEid(&(bundle->custodian), dictionary, &eid));
	if (eid)
	{
		isprintf(buf, sizeof buf, "Custodian EID   '%s'", eid);
		PUTS(buf);
		MRELEASE(eid);
	}

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

	priority = bundle->priority;
	isprintf(buf, sizeof buf,
			"Priority                %lu", priority);
	PUTS(buf);

	isprintf(buf, sizeof buf,
			"Ordinal                 %d",
		       	bundle->ordinal);
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
	isprintf(buf, sizeof buf,
			"Payload len %10lu", bundle->payload.length);
	PUTS(buf);
	printPayload(sdr, bundle);
	printExtensions(sdr, bundle->extensions[1]);
	printQueueState(sdr, bundle);
	PUTS("**** End of bundle");
	releaseDictionary(dictionary);
}

static int	printQueue(int detail, Object queue, char *name)
{
	Sdr	sdr = bp_get_sdr();
	int	bundlesCount = 0;
	char	msgbuf[256];
	Object	elt;

	isprintf(msgbuf, sizeof msgbuf, "\n**   %s queue", name);
	PUTS(msgbuf);
	for (elt = sdr_list_first(sdr, queue); elt;
			elt = sdr_list_next(sdr, elt))
	{
		bundlesCount++;
		if (!detail)
		{
			continue;
		}

		/*	Need to print detail of bundle.			*/

		printBundle(sdr_list_data(sdr, elt));
	}

	isprintf(msgbuf, sizeof msgbuf, "Queue count is %ld.", bundlesCount);
	PUTS(msgbuf);
	return bundlesCount;
}

static int	printPlan(int detail, char *eid, int priority)
{
	Sdr	sdr = bp_get_sdr();
	int	bundlesCount = 0;
	VPlan	*vplan;
	char	msgbuf[256];
	Object	planObj;
	BpPlan	plan;
	int	partialCount;

	lookupPlan(eid, &vplan);
	if (vplan == NULL)
	{
		isprintf(msgbuf, sizeof msgbuf, "No egress plan for endpoint \
'%s'.", eid);
		PUTS(msgbuf);
		return 0;
	}

	planObj = sdr_list_data(sdr, vplan->planElt);
	sdr_read(sdr, (char *) &plan, planObj, sizeof(BpPlan));
	switch (priority)
	{
	case 0:
		return printQueue(detail, plan.bulkQueue, "Bulk");

	case 1:
		return printQueue(detail, plan.stdQueue, "Std");

	case 2:
		return printQueue(detail, plan.urgentQueue, "Urgent");

	default:
		partialCount = printQueue(detail, plan.bulkQueue, "Bulk");
		bundlesCount += partialCount;
		partialCount = printQueue(detail, plan.stdQueue, "Std");
		bundlesCount += partialCount;
		partialCount = printQueue(detail, plan.urgentQueue, "Urgent");
		bundlesCount += partialCount;
		return bundlesCount;
	}
}

static int	printAll(int detail)
{
	Sdr	sdr = bp_get_sdr();
	int	bundlesCount = 0;
	BpDB	*bpConstants;
	Object	elt;
	Object	addr;
		OBJ_POINTER(BpEvent, event);

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

		bundlesCount++;
		if (!detail)
		{
			continue;
		}

		/*	Need to print detail of bundle.			*/

		printBundle(event->ref);
	}

	return bundlesCount;
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
	Sdr		sdr;
	int		detail;		/*	Boolean.		*/
	char		*cursor;
	char		*destination = NULL;
	int		priority = -1;
	char		msgbuf[256];
	int		bundlesCount;

	if (bp_attach() < 0)
	{
		putErrmsg("Can't attach to BP.", NULL);
		return 1;
	}

	sdr = bp_get_sdr();
	if (rptType == NULL)	/*	No command-line arguments.	*/
	{
		rptType = "detail";	/*	Default.		*/
		PUTS("Reporting detail of all bundles.");
		detail = 1;
	}
	else
	{
		if (strcmp(rptType, "count") == 0)
		{
			detail = 0;
		}
		else
		{
			if (strcmp(rptType, "detail") == 0)
			{
				detail = 1;
			}
			else
			{
				/*	Report type not specified.	*/

				printUsage();
				return 0;
			}
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

	CHKZERO(sdr_begin_xn(sdr));	/*	Lock db for duration.	*/
	isignal(SIGINT, handleQuit);
	if (destination)
	{
		bundlesCount = printPlan(detail, destination, priority);
	}
	else
	{
		bundlesCount = printAll(detail);
	}

	sdr_exit_xn(sdr);
	isprintf(msgbuf, sizeof msgbuf, "\nReport count is %ld.", bundlesCount);
	PUTS(msgbuf);
	writeErrmsgMemos();
	bp_detach();
	return 0;
}
