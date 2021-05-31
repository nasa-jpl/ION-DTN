/*
	imcadminep.c:	Administrative endpoint application process
			for "imc" scheme, handles IMC petitions.

	Author: Scott Burleigh, JPL

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "imcfw.h"

static int	briefNewNode(uvast nodeNbr)
{
	Sdr		sdr = getIonsdr();
	ImcDB		*imcConstants = getImcConstants();
	char		ownEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	char		destEid[32];
	Lyst		ownGroups;
	Object		elt;
	Object		groupAddr;
	ImcGroup	group;
	int		bufsize;
	unsigned char	*buffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	LystElt		elt2;
	uvast		groupNbr;
	int		aduLength;
	Object		aduObj;
	Object		aduZco;

	isprintf(ownEid, sizeof(ownEid), "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	oK(parseEidString(ownEid, &sourceMetaEid, &vscheme, &vschemeElt));
	isprintf(destEid, sizeof(destEid), "ipn:" UVAST_FIELDSPEC ".0",
			nodeNbr);
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
					((uaddr) group.groupNbr)) == NULL)
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
		groupNbr = (uaddr) lyst_data(elt2);
		oK(cbor_encode_integer(groupNbr, &cursor));
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
	if (aduZco == 0 || aduZco == (Object) ERROR)
	{
		putErrmsg("Failed creating saga message ZCO.", NULL);
		return 0;
	}

#if IMCDEBUG
puts("Sending briefing.");
#endif
	/*	Note that ttl must be expressed in milliseconds for
	 *	BP processing.  The hard-coded TTL here is 1 minute.	*/

	if (bpSend(&sourceMetaEid, destEid, NULL, 60000, BP_STD_PRIORITY,
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
	uvast		ownNodeNbr = getOwnNodeNbr();
	uvast		uvtemp;
	ImcPetition	petition;
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		groupAddr;
	Object		groupElt;
	ImcGroup	group;
	Object		elt;
	uvast		nodeNbr;
	Object		iondbObj;
	IonDB		iondb;
	int		sourceRegionIdx;
	uint32_t	sourceRegionNbr;
	uint32_t	destinationRegionNbr;

	/*	Finish parsing the petition.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IMC petition group number.");
		return 0;
	}

	petition.groupNbr = uvtemp;
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

	/*	Now get the multicast group.				*/

#if IMCDEBUG
printf("Handling type-%d petition from " UVAST_FIELDSPEC " at node "
UVAST_FIELDSPEC ".\n", petition.isMember, metaEid.elementNbr, ownNodeNbr);
fflush(stdout);
#endif
	oK(sdr_begin_xn(sdr));
	imcFindGroup(petition.groupNbr, &groupAddr, &groupElt);
#if IMCDEBUG
printf("Seeking multicast group for group number " UVAST_FIELDSPEC ".\n", petition.groupNbr);
#endif
	if (groupElt == 0)	/*	System failure.			*/
	{
#if IMCDEBUG
puts("Group not found.");
#endif
		if (petition.isMember)	/*	(Else nothing to do.)	*/
		{
			writeMemoNote("Can't handle IMC Join petition",
					itoa(petition.groupNbr));
		}

		/*	Nothing to propagate even if node is a
		 *	passageway.  Can't Join the group, and
		 *	since the group is unknown the passageway
		 *	cannot be an "ex officio" member of that
		 *	group and thus cannot be Leaving in the
		 *	other region.					*/

		sdr_cancel_xn(sdr);
		return -1;
	}

	/*	The multicast group is known, though possibly empty.	*/

#if IMCDEBUG
printf("Adding node " UVAST_FIELDSPEC " to this group.\n", metaEid.elementNbr);
#endif
	sdr_stage(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
	if (petition.isMember)	/*	Source node joining the group.	*/
	{
		for (elt = sdr_list_first(sdr, group.members); elt;
				elt = sdr_list_next(sdr, elt))
		{
			nodeNbr = sdr_list_data(sdr, elt);
			if (nodeNbr < metaEid.elementNbr)
			{
				continue;
			}

			if (nodeNbr == metaEid.elementNbr)
			{
#if IMCDEBUG
puts("Ignoring redundant Join.");
fflush(stdout);
#endif
			/*	Again nothing to propagate even if
			 *	node is a passageway.  Since the
			 *	source node is already a member of
			 *	the group, the passageway's "ex
			 *	officio" membership in the group
			 *	has already been announced in the
			 *	other region.				*/

				sdr_cancel_xn(sdr);
				return 0;
			}

			break;	/*	Source node not in list.	*/
		}

		/*	Must add new member of group at this point.	*/
#if IMCDEBUG
printf("Adding node " UVAST_FIELDSPEC " to group " UVAST_FIELDSPEC ".\n", metaEid.elementNbr, petition.groupNbr);
fflush(stdout);
#endif
		if (elt)
		{
			oK(sdr_list_insert_before(sdr, elt,
					metaEid.elementNbr));
		}
		else
		{
			oK(sdr_list_insert_last(sdr, group.members,
					metaEid.elementNbr));
		}

		if (metaEid.elementNbr == ownNodeNbr)
		{
			group.isMember = 1;
		}

		if (petition.groupNbr == 0 && metaEid.elementNbr != ownNodeNbr)
		{
#if IMCDEBUG
printf("Should be sending a briefing to node " UVAST_FIELDSPEC ".\n", metaEid.elementNbr);
#endif
			/*	This node is subscribing to the IMC
			 *	petitions group, i.e., it is a node
			 *	that is newly announcing itself to
			 *	the multicast community.  So it
			 *	doesn't know about any other nodes'
			 *	subscriptions.  So we must send this
			 *	node a briefing.			*/

			if (briefNewNode(metaEid.elementNbr) < 0)
			{
				putErrmsg("Failed briefing new node.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}
		}

		/*	Any scheduled deletion of the group is now
		 *	canceled.					*/

		group.secUntilDelete = -1;
	}
	else
	{			/*	Source node leaving the group.	*/
		for (elt = sdr_list_first(sdr, group.members); elt;
				elt = sdr_list_next(sdr, elt))
		{
			nodeNbr = sdr_list_data(sdr, elt);
			if (nodeNbr < metaEid.elementNbr)
			{
				continue;
			}

			if (nodeNbr > metaEid.elementNbr)
			{
#if IMCDEBUG
puts("Ignoring Leave by non-member.");
fflush(stdout);
#endif
			/*	Again nothing to propagate even if
			 *	node is a passageway.  Since the
			 *	source node is already missing from
			 *	the group, the passageway's "ex
			 *	officio" withdrawal from the group
			 *	has already been announced in the
			 *	other region.				*/

				sdr_cancel_xn(sdr);
				return 0;
			}

			break;	/*	Source node may be in the list.	*/
		}

		/*	Must delete this member of the group, if found.	*/
#if IMCDEBUG
printf("Deleting member " UVAST_FIELDSPEC ".\n", metaEid.elementNbr);
fflush(stdout);
#endif
		if (elt)	/*	Source node is a member.	*/
		{
			sdr_list_delete(sdr, elt, NULL, NULL);
			if (metaEid.elementNbr == ownNodeNbr)
			{
				group.isMember = 0;
			}

			/*	If group now has no members in any
			 *	region that the node knows about,
			 *	schedule deletion of the group at
			 *	this node.				*/

			if (sdr_list_length(sdr, group.members) == 0)
			{
#if IMCDEBUG
puts("Flagging group for deletion.");
fflush(stdout);
#endif
				group.secUntilDelete = 15;
			}
		}
	}

	/*	If node is a passageway, propagate petition as needed.	*/

	iondbObj = getIonDbObject();
	sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
	if (iondb.regions[1].regionNbr != 0)
	{
		/*	Node is a passageway between its home region
		 *	and the immediate encompassing region.		*/

		sourceRegionIdx = ionRegionOf(metaEid.elementNbr, ownNodeNbr,
				&sourceRegionNbr);
		if (sourceRegionIdx < 0)
		{
			putErrmsg("IMC system error.", NULL);
			sdr_cancel_xn(sdr);
			return -1;
		}

		destinationRegionNbr =
				iondb.regions[0 - sourceRegionIdx].regionNbr;
		if (petition.isMember == 1)		/*	Join	*/
		{
			group.count[sourceRegionIdx] += 1;
			if (group.count[sourceRegionIdx] == 1)
			{
				if (imcSendPetition(&petition,
						destinationRegionNbr) < 0)
				{
					putErrmsg("Join propagation failed.",
							NULL);
					sdr_cancel_xn(sdr);
					return -1;
				}
			}
		}
		else					/*	Leave	*/
		{
			group.count[sourceRegionIdx] -= 1;
			if (group.count[sourceRegionIdx] == 0)
			{
				if (imcSendPetition(&petition,
						destinationRegionNbr) < 0)
				{
					putErrmsg("Leave propagation failed.",
							NULL);
					sdr_cancel_xn(sdr);
					return -1;
				}
			}
		}
	}

	sdr_write(sdr, groupAddr, (char *) &group, sizeof(ImcGroup));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed handling petition.", NULL);
		return -1;
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
	isignal(SIGTERM, shutDownAdminApp);
	sm_SemEnd((_petitionSap(NULL))->recvSemaphore);
}

static int	handlePetitions()
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

			/*	Intentional fall-through to default.	*/

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
int	main(int argc, char *argv[])
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
