/*
	cpsd.c:	contact plan synchronization daemon for ION.

	Author: Scott Burleigh, JPL

	Copyright (c) 2021, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
	
									*/
#include "imcfw.h"

static char	cpsEid[] = "imc:0.1";

static uaddr	_running(uaddr *newValue)
{
	void	*value;
	uaddr	state;
	
	if (newValue)			/*	Changing state.		*/
	{
		value = (void *) (*newValue);
		state = (uaddr) sm_TaskVar(&value);
	}
	else				/*	Just check.		*/
	{
		state = (uaddr) sm_TaskVar(NULL);
	}

	return state;
}

static void	shutDown(int signum)
{
	uaddr	stop = 0;

	isignal(SIGTERM, SIG_IGN);
	oK(_running(&stop));	/*	Terminates cpsd.		*/
}

/*	*	*	Handler thread functions	*	*	*/

static int	handleCpsNotice(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes)
{
	uvast		uvtemp;
	uint32_t	regionNbr;
	time_t		fromTime;
	time_t		toTime;
	uvast		fromNode;
	uvast		toNode;
	size_t		magnitude;
	float		confidence;
	int		result;
	PsmAddress	xaddr;
	int		revisingContact = 0;

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor,
			&unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice region nbr.");
		return 0;
	}

	regionNbr = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice From time.");
		return 0;
	}

	fromTime = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice To time.");
		return 0;
	}

	toTime = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice From node.");
		return 0;
	}

	fromNode = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice From node.");
		return 0;
	}

	toNode = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice magnitude.");
		return 0;
	}

	magnitude = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode CPS notice confidence.");
		return 0;
	}

	if (uvtemp == 400)	/*	Revision, confidence unchanged.	*/
	{
		confidence = -1.0;
		revisingContact = 1;
	}
	else			/*	Contact revision if >= 200.	*/
	{
		confidence = (float) (uvtemp / 100); 
		if (confidence >= 2.0)
		{
			confidence -= 2.0;
			revisingContact = 1;
		}
	}

	/*	Now act on the received contact plan mgt notice.	*/

	if (regionNbr == 0)		/*	This is a Range notice.	*/
	{
		if (toTime == 0)	/*	Delete Range.		*/
		{
			result = rfx_remove_range(&fromTime, fromNode, toNode,
					0);
			switch(result)
			{
			case -1:
				putErrmsg("Failed removing range", NULL);
				return -1;

			case 0:
				return 0;

			default:
				writeMemoNote("[?] Error removing range",
						itoa(result));
				return 0;
			}
		}
		else			/*	Add Range.		*/
		{
			result = rfx_insert_range(fromTime, toTime, fromNode,
					toNode, magnitude, &xaddr, 0);
			switch(result)
			{
			case -1:
				putErrmsg("Failed inserting range", NULL);
				return -1;

			case 0:
				return 0;

			default:
				writeMemoNote("[?] Error inserting range",
						itoa(result));
				return 0;
			}
		}
	}

	/*	This is a Contact notice.				*/

	if (fromTime == (time_t) -1)	/*	Registration contact.	*/
	{
		if (toTime == 0)	/*	Unregister node.	*/
		{
			result = rfx_remove_contact(regionNbr, &fromTime,
					fromNode, toNode, 0);
			switch(result)
			{
			case -1:
				putErrmsg("Failed unregistering node", NULL);
				return -1;

			case 0:
				return 0;

			default:
				writeMemoNote("[?] Error unregistering node",
						itoa(result));
				return 0;
			}
		}
		else			/*	Register node.		*/
		{
			result = rfx_insert_contact(regionNbr, fromTime,
					toTime, fromNode, toNode, magnitude,
					confidence, &xaddr, 0);
			switch(result)
			{
			case -1:
				putErrmsg("Failed registering node", NULL);
				return -1;

			case 0:
				return 0;

			default:
				writeMemoNote("[?] Error registering node",
						itoa(result));
				return 0;
			}
		}
	}

	/*	Notice pertains to a scheduled contact.			*/

	if (revisingContact)
	{
		result = rfx_revise_contact(regionNbr, fromTime,
				fromNode, toNode, magnitude, confidence, 0);
		switch(result)
		{
		case -1:
			putErrmsg("Failed revising contact", NULL);
			return -1;

		case 0:
			return 0;

		default:
			writeMemoNote("[?] Error revising contact",
					itoa(result));
			return 0;
		}
	}

	if (toTime == 0)		/*	Delete contact.		*/
	{
		result = rfx_remove_contact(regionNbr, &fromTime,
				fromNode, toNode, 0);
		switch(result)
		{
		case -1:
			putErrmsg("Failed removing contact", NULL);
			return -1;

		case 0:
			return 0;

		default:
			writeMemoNote("[?] Error removing contact",
					itoa(result));
			return 0;
		}
	}

	/*	Adding a scheduled contact.			*/

	result = rfx_insert_contact(regionNbr, fromTime, toTime, fromNode,
			toNode, magnitude, confidence, &xaddr, 0);
	switch(result)
	{
	case -1:
		putErrmsg("Failed inserting contact", NULL);
		return -1;

	case 0:
		return 0;

	default:
		writeMemoNote("[?] Error inserting contact", itoa(result));
		return 0;
	}
}

static void	*handleNotices(void *parm)
{
	Sdr		sdr = getIonsdr();
	uaddr		stop = 0;
	BpSAP		sap;
	BpDelivery	dlv;
	unsigned int	buflen;
	unsigned char	buffer[256];
	ZcoReader	reader;
	vast		bytesToParse;
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;

	sap = (BpSAP) parm;
	while (_running(NULL) && !(sm_SemEnded(sap->recvSemaphore)))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("CPS notice reception failed.", NULL);
			_running(&stop);
			continue;
		}

		switch (dlv.result)
		{
		case BpPayloadPresent:
			break;

		case BpEndpointStopped:
			_running(&stop);

			/*	Intentional fall-through to default.	*/

		default:
			bp_release_delivery(&dlv, 1);
			continue;
		}

		/*	Process the notice.				*/

		CHKNULL(sdr_begin_xn(sdr));
		buflen = zco_source_data_length(sdr, dlv.adu);
		if (buflen > sizeof buffer)
		{
			putErrmsg("Can't acquire notice.", itoa(buflen));
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, buflen,
				(char *) buffer);
		oK(sdr_end_xn(sdr));
		if (bytesToParse < 0)
		{
			putErrmsg("Can't receive notice.", NULL);
			_running(&stop);
			bp_release_delivery(&dlv, 1);
			continue;
		}

		/*	Start parsing of notice.			*/

		cursor = buffer;
		unparsedBytes = bytesToParse;
		arrayLength = 0;
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode CPS notice array.");
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (arrayLength != 7)
		{
			writeMemoNote("[?] Bad CPS notice array length",
					itoa(arrayLength));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (handleCpsNotice(&dlv, cursor, unparsedBytes) < 0)
		{
			putErrmsg("Can't process CPS notice.", NULL);
			_running(&stop);
		}

		bp_release_delivery(&dlv, 1);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	writeMemo("[i] cpsd handler thread ended.");
	writeErrmsgMemos();
	return NULL;
}

/*	*	*	Main thread functions.	*	*	*	*/

#if defined (ION_LWT)
int	cpsd(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(int argc, char *argv[])
{
#endif
	MetaEid		meid;
	VScheme		*vscheme;
	PsmAddress	velt;
	VEndpoint	*vpoint;
	Sdr		sdr;
	Object		iondbObj;
	IonDB		iondb;
	uaddr		start = 1;
	BpSAP		sap;
	pthread_t	handlerThread;
	Object		elt;
	Object		addr;
	CpsNotice	notice;
	unsigned char	buffer[128];
	unsigned char	*cursor;
	uvast		uvtemp;
	int		noticeLength;
	if (bpAttach() < 0)
	{
		putErrmsg("cpsd can't attach to BP.", NULL);
		return 1;
	}

	sdr = getIonsdr();
	CHKERR(sdr);
	findScheme("imc", &vscheme, &velt);
	if (velt == 0)
	{
		writeMemo("[i] Not configured for multicast; cpsd stopping.");
		return 1;
	}

	oK(parseEidString(cpsEid, &meid, &vscheme, &velt));
	findEndpoint("imc", &meid, vscheme, &vpoint, &velt);
	restoreEidString(&meid);
	if (velt == 0)
	{
		writeMemo("[i] Not configured for CP sync; cpsd stopping.");
		return 1;
	}

	if (imcInit() < 0)
	{
		putErrmsg("cpsd can't attach to IMC database.", NULL);
		return 1;
	}

	iondbObj = getIonDbObject();
	CHKERR(iondbObj);
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	isignal(SIGTERM, shutDown);
	oK(_running(&start));

	/*	Start the CPS notice handler thread.			*/

	if (bp_open(cpsEid, &sap) < 0)
	{
		putErrmsg("Can't open cpsd endpoint.", cpsEid);
		return 1;
	}

	if (pthread_begin(&handlerThread, NULL, handleNotices, sap,
			"cpsd_handler"))
	{
		putSysErrmsg("cpsd can't create notice handler thread.", NULL);
		return 1;
	}

	/*	Main loop: snooze 1 second, then drain queue of pending
	 *	contact plan synchronization notices.			*/

	writeMemo("[i] cpsd is running.");
	while (_running(NULL))
	{
		/*	Sleep for 1 second, then multicast all pending
		 *	CPS notices to propagate changes in contact
		 *	plans.						*/

		snooze(1);
		if (!sdr_begin_xn(sdr))
		{
			putErrmsg("cpsd failed to begin new transaction.",
					NULL);
			break;
		}

		while (1)
		{
			elt = sdr_list_first(sdr, iondb.cpsNotices);
			if (elt == 0)
			{
				break;	/*	No more to send.	*/
			}

			addr = sdr_list_data(sdr, elt);
			sdr_read(sdr, (char *) &notice, addr,
					sizeof(CpsNotice));
			sdr_free(sdr, addr);
			sdr_list_delete(sdr, elt, NULL, NULL);

			/*	Use buffer to serialize CPS notice.	*/

			cursor = buffer;

			/*	Notice is an array of 7 items.		*/

			uvtemp = 7;
			oK(cbor_encode_array_open(uvtemp, &cursor));

			/*	First item of array is region number.	*/

			uvtemp = notice.regionNbr;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then From time.				*/

			uvtemp = notice.fromTime;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then To time.				*/

			uvtemp = notice.toTime;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then From node.				*/

			uvtemp = notice.fromNode;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then To node.				*/

			uvtemp = notice.toNode;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Then magnitude.				*/

			uvtemp = notice.magnitude;
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Finally confidence, which we convert
			 *	from a float (max 4.0) to an integer
			 *	percentage (max 400).			*/

			uvtemp = (notice.confidence >= 4.0 ? 400 :
					(uvast) (notice.confidence * 100.0));
			oK(cbor_encode_integer(uvtemp, &cursor));

			/*	Now multicast the notice.		*/

			noticeLength = cursor - buffer;
			if (imcSendDispatch(cpsEid, notice.regionNbr, buffer,
					noticeLength) < 0)
			{
				putErrmsg("Failed sending CPS notice.", NULL);
				break;
			}
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't send contact plan updates.", NULL);
			break;
		}
	}

	/*	Terminate handler thread.				*/

	sm_SemEnd(sap->recvSemaphore);
	pthread_join(handlerThread, NULL);
	bp_close(sap);

	/*	Wrap up.						*/

	writeErrmsgMemos();
	writeMemo("[i] cpsd has ended.");
	ionDetach();
	return 0;
}
