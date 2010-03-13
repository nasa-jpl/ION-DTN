/*
	bplist.c:	bundle timeline listing utility.
									*/
/*									*/
/*	Copyright (c) 2009, California Institute of Technology.		*/
/*	All rights reserved.						*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#include <bpP.h>

static void	printDictionary(char *dictionary, int dictionaryLength)
{
	int	offset = 0;
	char	*cursor = dictionary;
	int	length;

	printf("Dictionary:\n");
	while (offset < dictionaryLength)
	{
		length = strlen(cursor) + 1;
		printf("(%3ul) %s", offset, cursor);
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
				puts(line);
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
		puts(line);
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

		printf("****** Extension\n");
		sdr_read(sdr, buf, blk->bytes, blk->length);
		printBytes(buf, blk->length);
	}

	MRELEASE(buf);
}

static void	printPayload(Sdr sdr, Bundle *bundle)
{
	int		buflen;
	char		*buf;
	Object		ref;
	ZcoReader	reader;
	int		len;

	buflen = bundle->payload.length;
	buf = MTAKE(buflen);
	if (buf == NULL)
	{
		putErrmsg("bplist can't allocate buffer.", itoa(buflen));
		return;
	}

	ref = zco_add_reference(sdr, bundle->payload.content);
	if (ref == 0)
	{
		putErrmsg("bplist can't add ZCO reference.", NULL);
		return;
	}

	zco_start_receiving(sdr, ref, &reader);
	len = zco_receive_source(sdr, &reader, bundle->payload.length, buf);
	zco_stop_receiving(sdr, &reader);
	if (len < 0)
	{
		putErrmsg("bplist can't read ZCO source.", NULL);
		return;
	}

	zco_destroy_reference(sdr, ref);
	printf("****** Payload\n");
	printBytes(buf, bundle->payload.length);
	MRELEASE(buf);
}

static void	printBundle(Sdr sdr, Bundle *bundle)
{
	char	*eid;
	char	*dictionary;

	dictionary = retrieveDictionary(bundle);
	printf("\n**** Bundle\n");
	oK(printEid(&(bundle->id.source), dictionary, &eid));
	printf("Source EID      '%s'\n", eid);
	MRELEASE(eid);
	printf("Creation sec   %10lu   count %10lu   frag offset %10lu\n",
			bundle->id.creationTime.seconds,
			bundle->id.creationTime.count,
			bundle->id.fragmentOffset);
	printf("- is a fragment:        %d\n", bundle->bundleProcFlags
		       	& BDL_IS_FRAGMENT ? 1 : 0);
	printf("- is admin:             %d\n", bundle->bundleProcFlags
		       	& BDL_IS_ADMIN ? 1 : 0);
	printf("- does not fragment:    %d\n", bundle->bundleProcFlags
			& BDL_DOES_NOT_FRAGMENT ? 1 : 0);
	printf("- is custodial:         %d\n", bundle->bundleProcFlags
			& BDL_IS_CUSTODIAL ? 1 : 0);
	printf("- dest is singleton:    %d\n", bundle->bundleProcFlags
			& BDL_DEST_IS_SINGLETON ? 1 : 0);
	printf("- app ack requested:    %d\n", bundle->bundleProcFlags
			& BDL_APP_ACK_REQUEST ? 1 : 0);
	printf("Priority                %lu\n",
			COS_FLAGS(bundle->bundleProcFlags) & 0x03);
	printf("Ordinal                 %d\n",
		       	bundle->extendedCOS.ordinal);
	printf("Unreliable:             %d\n", bundle->extendedCOS.flags
			& BP_BEST_EFFORT ? 1 : 0);
	printf("Critical:               %d\n", bundle->extendedCOS.flags
			& BP_MINIMUM_LATENCY ? 1 : 0);
	oK(printEid(&(bundle->destination), dictionary, &eid));
	printf("Destination EID '%s'\n", eid);
	MRELEASE(eid);
	oK(printEid(&(bundle->reportTo), dictionary, &eid));
	printf("Report-to EID   '%s'\n", eid);
	MRELEASE(eid);
	oK(printEid(&(bundle->custodian), dictionary, &eid));
	printf("Custodian EID   '%s'\n", eid);
	MRELEASE(eid);
	printf("Expiration sec %10lu\n", bundle->expirationTime);
	printf("Total ADU len  %10lu\n", bundle->totalAduLength);
	printf("Dictionary len %10lu\n", bundle->dictionaryLength);
	printDictionary(dictionary, bundle->dictionaryLength);
	releaseDictionary(dictionary);
	printExtensions(sdr, bundle->extensions[0]);
	printPayload(sdr, bundle);
	printExtensions(sdr, bundle->extensions[1]);
	printf("**** End of bundle\n");
}

static void	printUsage()
{
	puts("Usage: bplist [ { count | detail } \
[<protocolName>/<outductName>/<priority>]]");
}

#if defined (VXWORKS) || defined (RTEMS)
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
	int		detail = 0;
	char		*cursor;
	char		*protocolName = NULL;
	char		*ductName;
	int		priority;
	char		msgbuf[256];
	Sdr		sdr;
	BpDB		*bpConstants;
	Object		list;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Object		elt;
	Object		addr;
			OBJ_POINTER(BpEvent, event);
			OBJ_POINTER(Bundle, bundle);
	long		bundlesCount = 0;
			OBJ_POINTER(Outduct, duct);
			OBJ_POINTER(XmitRef, xr);

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
		else if (strcmp(rpt, "detail") == 0)
		{
			detail = 1;
		}
		else
		{
			printUsage();
			return 0;
		}

		if (queue)
		{
			protocolName = queue;
			cursor = strchr(protocolName, '/');
			if (cursor == NULL)
			{
				printUsage();
				return 0;
			}

			*cursor = '\0';
			ductName = cursor + 1;
			cursor = strchr(ductName, '/');
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
in outduct '%.64s' of protocol '%.16s', priority %d.", ductName, protocolName,
					priority);
			writeMemo(msgbuf);
		}
	}

	sdr = bp_get_sdr();
	sdr_begin_xn(sdr);	/*	Lock database for duration.	*/
	if (protocolName == NULL)	/*	All bundles.		*/
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
		findOutduct(protocolName, ductName, &vduct, &vductElt);
		if (vductElt == 0)
		{
			writeMemo("No such outduct.");
		}
		else
		{
			addr = sdr_list_data(sdr, vduct->outductElt);
			GET_OBJ_POINTER(sdr, Outduct, duct, addr);
			CHKZERO(duct);
			switch (priority)
			{
			case 2:
				list = duct->urgentQueue;
				break;
		
			case 1:
				list = duct->stdQueue;
				break;
		
			default:
				list = duct->bulkQueue;
			}

			if (count)
			{
				isprintf(msgbuf, sizeof msgbuf, "Count is %ld.",
						sdr_list_length(sdr, list));
				sdr_exit_xn(sdr);
			}
			else
			{
				for (elt = sdr_list_first(sdr, list); elt;
						elt = sdr_list_next(sdr, elt))
				{
					addr = sdr_list_data(sdr, elt);
					GET_OBJ_POINTER(sdr, XmitRef, xr, addr);
					GET_OBJ_POINTER(sdr, Bundle, bundle,
							xr->bundleObj);
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
