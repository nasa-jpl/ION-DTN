/*
	imcadminep.c:	Administrative endpoint application process
			for "imc" scheme, handles IMC petitions.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "imcfw.h"

static int	briefNewNode(uvast fqnn)
{
	Sdr		sdr = getIonsdr();
	ImcDB		*imcConstants = getImcConstants();
	char		nbrBuf[FQN_MAX_LENGTH];
	char		ownEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	char		destEid[32];
	Lyst		ownGroups;
	SdrObject	elt;
	SdrObject	groupAddr;
	ImcGroup	group;
	int		bufsize;
	unsigned char	*buffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	LystElt		elt2;
	uvast		fqgn;
	int		aduLength;
	SdrObject	aduObj;
	SdrObject	aduZco;

	putFqn(nbrBuf, getOwnFqnn());
	isprintf(ownEid, sizeof(ownEid), "ipn:%s.0", nbrBuf);
	oK(parseEidString(ownEid, &sourceMetaEid, &vscheme, &vschemeElt));
	putFqn(nbrBuf, fqnn);
	isprintf(destEid, sizeof(destEid), "ipn:%s.0", nbrBuf);
	ownGroups = lyst_create_using(getIonMemoryMgr());
	if (ownGroups == NULL)
	{
		putErrmsg("Can't compile groups list for briefing.", NULL);
		return -1;
	}

	for (elt = sdr_list_first(sdr, imcConstants->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		groupAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
		if (group.isMember)
		{
			if (lyst_insert_last(ownGroups, (void *)
					((uaddr) group.fqgn)) == NULL)
			{
				sdr_exit_xn(sdr);
				lyst_destroy(ownGroups);
				putErrmsg("Can't add group to list.", NULL);
				return -1;
			}
		}
	}

	/*	Create buffer for serializing briefing message.		*/

	bufsize = 1	/*	admin record array (2 items)		*/
		+ 9	/*	admin record type, an integer		*/
		+ 9	/*	group number array (N items)		*/
		+ (lyst_length(ownGroups) * 9);
	buffer = MTAKE(bufsize);
	if (buffer == NULL)
	{
		lyst_destroy(ownGroups);
		putErrmsg("Can't allocate buffer for briefing.", NULL);
		return -1;
	}

	cursor = buffer;

	/*	Sending an admin record, an array of 2 items.		*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	First item of admin record is record type code.		*/

	uvtemp = BP_MULTICAST_BRIEFING;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Second item of admin record is content, the
	 *	briefing message, which is a definite-length array.	*/

	uvtemp = lyst_length(ownGroups);
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Groups in ownGroups list are the elements of the array.	*/

	for (elt2 = lyst_first(ownGroups); elt2; elt2 = lyst_next(elt2))
	{
		fqgn = (uaddr) lyst_data(elt2);
		oK(cbor_encode_integer(fqgn, &cursor));
	}

	lyst_destroy(ownGroups);

	/*	Now wrap the record buffer in a ZCO and send it to
	 *	the destination node.					*/

	aduLength = cursor - buffer;
	aduObj = sdr_malloc(sdr, aduLength);
	if (aduObj == 0)
	{
		putErrmsg("Can't create briefing message.", NULL);
		return -1;
	}

	sdr_write(sdr, aduObj, (char *) buffer, aduLength);
	MRELEASE(buffer);
	aduZco = ionCreateZco(ZcoSdrSource, aduObj, 0, aduLength,
			BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
	if (aduZco == 0 || aduZco == (SdrObject) ERROR)
	{
		putErrmsg("Failed creating saga message ZCO.", NULL);
		return 0;
	}

#if IMCDEBUG
puts("Sending briefing.");
#endif
	/*	Note that ttl must be expressed in milliseconds for
	 *	BP processing.  The hard-coded TTL here is 1 day.	*/

	if (bpSend(&sourceMetaEid, destEid, NULL, 86400000, BP_STD_PRIORITY,
			NoCustodyRequested, 0, 0, NULL, aduZco, NULL,
			BP_MULTICAST_BRIEFING) <= 0)
	{
		writeMemo("[?] Unable to send IMC briefing message.");
	}

	return 0;
}

static int	handlePetition(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes)
{
	Sdr		sdr = getIonsdr();
	uvast		ownFqnn = getOwnFqnn();
	uvast		uvtemp;
	ImcPetition	petition;
	MetaEid		metaEid;
	VScheme		*vscheme = NULL;
	PsmAddress	vschemeElt;
	IonDB		iondb;
	int		newMember;

	/*	Finish parsing the petition.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IMC petition group number.");
		return 0;
	}

	petition.fqgn = uvtemp;
	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IMC petition membership switch.");
		return 0;
	}

	petition.isMember = (uvtemp != 0);

	/*	Determine the node that sent the petition.		*/

	if (parseEidString(dlv->bundleSourceEid, &metaEid, &vscheme,
			&vschemeElt) == 0 || vscheme->codeNumber != ipn)
	{
		/*	Can't determine sending node number.		*/

		writeMemoNote("[?] Invalid sender of IMC petition",
				dlv->bundleSourceEid);
		return 0;
	}

#if IMCDEBUG
printf("Handling type-%d petition from " UVAST_FIELDSPEC " at node "
UVAST_FIELDSPEC ".\n", petition.isMember, metaEid.elementNbr, ownFqnn);
fflush(stdout);
#endif
	if (metaEid.elementNbr == ownFqnn)
	{
		/*
		 * A node applies its own Join or Leave when it adds or
		 * removes the endpoint, so only a passageway, which also
		 * petitions on behalf of other nodes, acts on a petition
		 * from itself.
		 */
		oK(sdr_begin_xn(sdr));
		sdr_read(sdr, (char *) &iondb, getIonDbObject(),
				sizeof(IonDB));
		sdr_exit_xn(sdr);
		if (iondb.regions[1].regionNbr == 0)
		{
			return 0;
		}
	}

	if (imcApplyPetition(&petition, metaEid.elementNbr, &newMember) < 0)
	{
		putErrmsg("Failed handling petition.", NULL);
		return -1;
	}

	if (newMember && metaEid.elementNbr != ownFqnn)
	{
		/*
		 * A newly joined node doesn't know about any other nodes'
		 * subscriptions, so it must be briefed.
		 */
		CHKERR(sdr_begin_xn(sdr));
		if (briefNewNode(metaEid.elementNbr) < 0)
		{
			putErrmsg("Failed briefing new node.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed briefing new node.", NULL);
			return -1;
		}
	}

	return 0;
}

static BpSAP	_petitionSap(BpSAP *newSap)
{
	void	*value;
	BpSAP	sap;

	if (newSap)			/*	Add task variable.	*/
	{
		value = (void *) (*newSap);
		sap = (BpSAP) sm_TaskVar(&value);
	}
	else				/*	Retrieve task variable.	*/
	{
		sap = (BpSAP) sm_TaskVar(NULL);
	}

	return sap;
}

static void	shutDownAdminApp(int signum)
{
	/* Tell the compiler that we are not using 'signum' */
	(void)signum;

	isignal(SIGTERM, shutDownAdminApp);
	sm_SemEnd((_petitionSap(NULL))->recvSemaphore);
}

static int	handlePetitions(void)
{
	Sdr		sdr = getIonsdr();
	char		receptionEid[] = "imc:0.0";
	int		running = 1;
	BpSAP		sap;
	BpDelivery	dlv;
	unsigned int	buflen;
	unsigned char	buffer[256];
	ZcoReader	reader;
	vast		bytesToParse;
	unsigned char	*cursor;
	unsigned int	unparsedBytes;
	uvast		arrayLength;

	if (bp_open(receptionEid, &sap) < 0)
	{
		putErrmsg("Can't open imcadmin endpoint 'imc:0.0'.", NULL);
		return 1;
	}

	oK(_petitionSap(&sap));
	isignal(SIGTERM, shutDownAdminApp);
	while (running && !(sm_SemEnded(sap->recvSemaphore)))
	{
		if (bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("IMC petition reception failed.", NULL);
			running = 0;
			continue;
		}

		switch (dlv.result)
		{
		case BpPayloadPresent:
			break;

		case BpEndpointStopped:
			running = 0;

			/* FALLTHROUGH */

		default:
			bp_release_delivery(&dlv, 1);
			continue;
		}

		/*	Process the petition.				*/

		CHKERR(sdr_begin_xn(sdr));
		buflen = zco_source_data_length(sdr, dlv.adu);
		if (buflen > sizeof buffer)
		{
			putErrmsg("Can't acquire petition.", itoa(buflen));
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		zco_start_receiving(dlv.adu, &reader);
		bytesToParse = zco_receive_source(sdr, &reader, buflen,
				(char *) buffer);
		if (bytesToParse < 0)
		{
			putErrmsg("Can't receive petition.", NULL);
			oK(sdr_end_xn(sdr));
			bp_release_delivery(&dlv, 1);
			running = 0;
			continue;
		}

		oK(sdr_end_xn(sdr));

		/*	Start parsing of petition.			*/

		cursor = buffer;
		unparsedBytes = bytesToParse;
		arrayLength = 0;
		if (cbor_decode_array_open(&arrayLength, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode IMC petition array.");
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (arrayLength != 2)
		{
			writeMemoNote("[?] Bad IMC petition array length",
					itoa(arrayLength));
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if (handlePetition(&dlv, cursor, unparsedBytes) < 0)
		{
			putErrmsg("Can't process IMC petition.", NULL);
			running = 0;
		}

		bp_release_delivery(&dlv, 1);

		/*	Make sure other tasks have a chance to run.	*/

		sm_TaskYield();
	}

	bp_close(sap);
	writeMemo("[i] Administrative endpoint terminated.");
	writeErrmsgMemos();
	return 0;
}

#if defined (ION_LWT)
int	imcadminep(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
#else
int	main(void)
{
#endif
	if (bpAttach() < 0)
	{
		putErrmsg("imcadminep can't attach to BP.", NULL);
		return 1;
	}

	if (imcInit() < 0)
	{
		putErrmsg("imcadminep can't load multicast database.", NULL);
		return 1;
	}

	writeMemo("[i] imcadminep is running.");
	if (handlePetitions() < 0)
	{
		putErrmsg("imcadminep crashed.", NULL);
	}

	writeErrmsgMemos();
	writeMemo("[i] imcadminep has ended.");
	ionDetach();
	return 0;
}
