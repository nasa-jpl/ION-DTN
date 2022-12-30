/*
	libirf.c:	functions for inter-regional forwarding.

	Author: Scott Burleigh, IPNGROUP

	Copyright (c) 2021, IPNGROUP.  ALL RIGHTS RESERVED.
									*/
#include "ipnfw.h"
#include "bei.h"

int	irf_add_candidate(uvast nodeNbr, IonNode *node, PsmAddress nextElt)
{
	PsmPartition	ionwm = getIonwm();
	PsmAddress	addr;
	PsmAddress	elt;
	IrfCandidate	*candidate;

	CHKERR(ionwm);
        addr = psm_zalloc(ionwm, sizeof(IrfCandidate));
        if (addr == 0)
        {
                putErrmsg("Can't add IRF candidate.", NULL);
                return -1;
        }

        if (nextElt)
        {
                elt = sm_list_insert_before(ionwm, nextElt, addr);
        }
        else
        {
                elt = sm_list_insert_last(ionwm, node->viaPassageways, addr);
        }

        if (elt == 0)
        {
                psm_free(ionwm, addr);
                putErrmsg("Can't add IRF candidate.", NULL);
                return -1;
        }

        candidate = (IrfCandidate *) psp(ionwm, addr);
	candidate->nodeNbr = nodeNbr;
	candidate->confirmTime = 0;
	return 0;
}

int	irf_initialize(IonNode *terminusNode)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IonDB		iondb;
	Object		elt;
	Object		addr;
			OBJ_POINTER(RegionMember, member);

	terminusNode->viaPassageways = sm_list_create(ionwm);
	if (terminusNode->viaPassageways == 0)
	{
		putErrmsg("Can't initialize IRF routing.", NULL);
		return -1;
	}

	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));

	/*	Add to the viaPassageways list for this remote node
	 *	one entry for every passageway residing in either of
	 *	the local node's regions.  The viaPassageways list
	 *	is in ascending node number sequence, because the
	 *	rolodex list is in ascending node number sequence.	*/

	for (elt = sdr_list_first(sdr, iondb.rolodex); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, RegionMember, member, addr);
		if (member->outerRegionNbr != 0)
		{
			/*	Node is a potentially usable passageway
			 *	for transmission to this destination.	*/

			if (irf_add_candidate(member->nodeNbr, terminusNode, 0)
					< 0)
			{
				putErrmsg("Can't note IRF candidate.", NULL);
				return -1;
			}
		}
	}

	return 0;
}

static int	serializeIrfMsg(uvast fromNodeNbr, uvast toNodeNbr,
			int isReachable, Lyst passageways, char **msgbuf)
{
	int		traceLen;
	int		bufsize;
	char		*buffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	LystElt		elt;
	int		msgLength;

	traceLen = lyst_length(passageways);
	bufsize =	1		/*	Array of 4 items	*/
		+	9		/*	Integer: source node nbr*/
		+	9		/*	Integer: dest node nbr	*/
		+	1		/*	Integer: isReachable	*/
		+	1		/*	Array of N items	*/
		+	(9 * traceLen);	/*	Integers: pwy node nbrs	*/
	buffer = MTAKE(bufsize);
	if (buffer == NULL)
	{
		putErrmsg("Failed taking buffer for IRF message.", NULL);
		*msgbuf = NULL;
		return -1;
	}

	memset(buffer, 0, bufsize);
	cursor = (unsigned char *) buffer;

	/*	IRF message is an array of 4 items.			*/

	uvtemp = 4;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	The first item of the IRF message is the node number
	 *	of the source of a bundle destined for a node located
	 *	in some other region.					*/

	uvtemp = fromNodeNbr;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The second item of the IRF message is the node number
	 *	of the destination of that bundle, the node for which
	 *	this message reports reachability.			*/

	uvtemp = toNodeNbr;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The third item of the IRF message is the reachability
	 *	flag, a Boolean value.					*/

	uvtemp = isReachable ? 1 : 0;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The fourth item of the IRF message is an array of N
	 *	node numbers identifying additional passageway nodes
	 *	that relayed the bundle toward its destination.		*/

	uvtemp = traceLen;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Each item of this array is the node number of another
	 *	passageway node (in addition to the recipient of this
	 *	message) that must be informed of this reachability.	*/

	for (elt = lyst_first(passageways); elt; elt = lyst_next(elt))
	{
		uvtemp = (uvast) (uintptr_t)lyst_data(elt);
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	IRF message has now been serialized.			*/

	*msgbuf = buffer;
	msgLength = cursor - ((unsigned char *) buffer);
	return msgLength;
}

static int	constructIrfMsg(uvast fromNodeNbr, uvast toNodeNbr,
			int isReachable, Lyst passageways, Object *zco)
{
	Sdr	sdr = getIonsdr();
	char	*msgbuf;
	int	msgLength;
	Object	sourceData;

	msgLength = serializeIrfMsg(fromNodeNbr, toNodeNbr, isReachable,
			passageways, &msgbuf);
	if (msgLength < 0)
	{
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));
	sourceData = sdr_malloc(sdr, msgLength);
	if (sourceData == 0)
	{
		MRELEASE(msgbuf);
		putErrmsg("No space for source data.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, sourceData, (char *) msgbuf, msgLength);
	MRELEASE(msgbuf);

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that allocating this ZCO space is non-
	 *	negotiable: for IRF messages, allocation of ZCO
	 *	space can never be denied or delayed.			*/

	*zco = zco_create(sdr, ZcoSdrSource, sourceData, 0, 0 - msgLength,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0 || *zco == (Object) ERROR || *zco == 0)
	{
		putErrmsg("Can't create IRF message.", NULL);
		return -1;
	}

	return 0;
}

int	irf_send_msg(uvast fromNodeNbr, uvast toNodeNbr, int isReachable,
		Lyst passageways)
{
	char		sourceEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	LystElt		elt;
	uvast		destinationNodeNbr;
	char		destinationEid[32];
	Object		payloadZco;
	uvast		ttl = 604800;	/*	Seconds; 1 week.	*/
	BpAncillaryData	ecos = { 0, 0, 255 };

	CHKERR(passageways);
	isprintf(sourceEid, sizeof sourceEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	CHKERR(parseEidString(sourceEid, &sourceMetaEid, &vscheme,
			&vschemeElt));
	elt = lyst_last(passageways);
	if (elt)
	{
		/*	Send message to last passageway in trace list.	*/

		destinationNodeNbr = (uvast) (uintptr_t)lyst_data(elt);
	}
	else
	{
		/*	Send message to orignal source of bundle.	*/

		destinationNodeNbr = fromNodeNbr;
	}

	isprintf(destinationEid, sizeof destinationEid,
			"ipn:" UVAST_FIELDSPEC ".271", destinationNodeNbr);
	if (constructIrfMsg(fromNodeNbr, toNodeNbr, isReachable, passageways,
			&payloadZco) < 0)
	{
		putErrmsg("Can't construct IRF message.", NULL);
		return -1;
	}

	switch (bpSend(&sourceMetaEid, destinationEid, NULL, ttl * 1000,
			BP_EXPEDITED_PRIORITY, NoCustodyRequested, 0, 0,
			&ecos, payloadZco, NULL, 0))
	{
	case -1:
		putErrmsg("Can't send IRF message.", NULL);
		return -1;

	case 0:
		writeMemo("[?] IRF message not transmitted.");

			/*	Intentional fall-through to next case.	*/
	default:
		return 0;
	}
}

int	irf_source_msg(Bundle *bundle, int isReachable)
{
	Sdr	sdr = getIonsdr();
	uvast	fromNodeNbr;
	uvast	toNodeNbr;
	Lyst	passageways;
	Object	elt;
	int	result;

	CHKERR(bundle);
	fromNodeNbr = bundle->id.source.ssp.ipn.nodeNbr;
	toNodeNbr = bundle->destination.ssp.ipn.nodeNbr;
	CHKERR(bundle->passageways);

	/*	Copy passageways list out of bundle.			*/

	passageways = lyst_create_using(getIonMemoryMgr());
	CHKERR(passageways);
	for (elt = sdr_list_first(sdr, bundle->passageways); elt;
			elt = sdr_list_next(sdr, elt))
	{
		if (lyst_insert_last(passageways,
			(void *) sdr_list_data(sdr, elt)) == NULL)
		{
			lyst_destroy(passageways);
			putErrmsg("Can't copy passageways.", NULL);
			return -1;
		}
	}

	result = irf_send_msg(fromNodeNbr, toNodeNbr, isReachable, passageways);
	lyst_destroy(passageways);
	if (result < 0)
	{
		putErrmsg("Can't source IRF message.", NULL);
	}

	return result;
}

int	irf_issue_ipt_rpt(Bundle *bundle)
{
	Sdr		sdr = getIonsdr();
	uvast		fromNodeNbr;
	uvast		toNodeNbr;
	char		sourceEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Lyst		passageways;
	Object		pwyElt;
	int		traceLen;
	int		bufsize;
	char		*buffer;
	unsigned char	*cursor;
	uvast		uvtemp;
	LystElt		elt;
	int		msgLength;
	Object		payloadZco;
	char		*destinationEid;
	uvast		ttl = 604800;	/*	Seconds; 1 week.	*/
	BpAncillaryData	ecos = { 0, 0, 255 };
	Object		sourceData;
	int		result;

	CHKERR(bundle);
	CHKERR(bundle->passageways);
	fromNodeNbr = bundle->id.source.ssp.ipn.nodeNbr;
	toNodeNbr = bundle->destination.ssp.ipn.nodeNbr;
	isprintf(sourceEid, sizeof sourceEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	CHKERR(parseEidString(sourceEid, &sourceMetaEid, &vscheme,
			&vschemeElt));

	/*	Copy passageways list out of bundle.			*/

	passageways = lyst_create_using(getIonMemoryMgr());
	CHKERR(passageways);
	for (pwyElt = sdr_list_first(sdr, bundle->passageways); pwyElt;
			pwyElt = sdr_list_next(sdr, pwyElt))
	{
		if (lyst_insert_last(passageways,
			(void *) sdr_list_data(sdr, pwyElt)) == NULL)
		{
			lyst_destroy(passageways);
			putErrmsg("Can't copy passageways.", NULL);
			return -1;
		}
	}

	/*	IPT report is the content of a BP admin record of type
	 *	9; it is an array of 4 items, the fourth of which is
	 *	an array of passageway node numbers.  This BP admin
	 *	record is the payload of the bundle that will be sent.	*/

	traceLen = lyst_length(passageways);
	bufsize =	1		/*	Array of 2 items	*/
		+	9		/*	Integer: admin rec type	*/
		+	1		/*	Array of 4 items	*/
		+	9		/*	Integer: Unix epoch time*/
		+	9		/*	Integer: source node nbr*/
		+	9		/*	Integer: dest node nbr	*/
		+	1		/*	Array of traceLen items	*/
		+	(9 * traceLen);	/*	Integers: pwy node nbrs	*/
	buffer = MTAKE(bufsize);
	if (buffer == NULL)
	{
		lyst_destroy(passageways);
		putErrmsg("Failed taking buffer for IPT report.", NULL);
		return -1;
	}

	/*	Can now serialize the record into the buffer.		*/

	memset(buffer, 0, bufsize);
	cursor = (unsigned char *) buffer;

	/*	Admin record is an array of 2 items.			*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	The first item of the admin record is the admin
	 *	record type.						*/

	uvtemp = BP_IPT_REPORT;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The second item of the admin record is its content,
	 *	the IPT report itself.
	 *
	 *	The IPT report is itself an array of 4 items.		*/

	uvtemp = 4;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	The first item of the IPT report is the Unix epoch
	 *	time at which this report is being issued.		*/

	uvtemp = getCtime();
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The second item of the IPT report is the node number
	 *	of the source of a bundle that was destined for a node
	 *	located in some other region.				*/

	uvtemp = fromNodeNbr;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The third item of the IPT report is the node number
	 *	of the destination of that bundle, the node for which
	 *	this report traces the trans-regional path.		*/

	uvtemp = toNodeNbr;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	The fourth item of the IPT report is an array of N
	 *	node numbers identifying all of the passageway nodes
	 *	that relayed the bundle toward its destination.		*/

	uvtemp = traceLen;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	Each item of this array is the node number of one of
	 *	the passageway node on that path.			*/

	for (elt = lyst_first(passageways); elt; elt = lyst_next(elt))
	{
		uvtemp = (uvast) (uintptr_t)lyst_data(elt);
		oK(cbor_encode_integer(uvtemp, &cursor));
	}

	/*	IPT report has now been serialized.			*/

	lyst_destroy(passageways);
	msgLength = cursor - ((unsigned char *) buffer);
	CHKERR(sdr_begin_xn(sdr));
	sourceData = sdr_malloc(sdr, msgLength);
	if (sourceData == 0)
	{
		MRELEASE(buffer);
		putErrmsg("No space for source data.", NULL);
		sdr_cancel_xn(sdr);
		return -1;
	}

	sdr_write(sdr, sourceData, buffer, msgLength);
	MRELEASE(buffer);

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that allocating this ZCO space is non-
	 *	negotiable: for IPT reports, allocation of ZCO
	 *	space can never be denied or delayed.			*/

	payloadZco = zco_create(sdr, ZcoSdrSource, sourceData, 0, 0 - msgLength,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0
	|| payloadZco == (Object) ERROR
	|| payloadZco == 0)
	{
		putErrmsg("Can't create IPT report.", NULL);
		return -1;
	}

	/*	Construct destination endpoint ID and send report.	*/

	readEid(&(bundle->reportTo), &destinationEid);
	result = bpSend(&sourceMetaEid, destinationEid, NULL, ttl * 1000,
			BP_STD_PRIORITY, NoCustodyRequested, 0, 0, &ecos,
			payloadZco, NULL, BP_IPT_REPORT);
	MRELEASE(destinationEid);
	switch (result)
	{
	case -1:
		putErrmsg("Can't send IPT report.", NULL);
		break;

	case 0:
		writeMemo("[?] IPT report not transmitted.");

			/*	Intentional fall-through to next case.	*/
	default:
		break;
	}

	return result;
}

int	irf_print_ipt_rpt(BpDelivery *dlv, unsigned char *cursor,
		unsigned int unparsedBytes)
{
	uvast	arrayLength;
	uvast	uvtemp;
	time_t	timestamp;
	char	timestampBuffer[32];
	uvast	fromNodeNbr;
	uvast	toNodeNbr;
	uvast	pwyNodeNbr;
	char	buf[256];

	/*	Parse past IPT report array header.			*/

	arrayLength = 4;	/*	Decode array of size 4.		*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IPT report.");
		return 0;
	}

	/*	Get report effective time.				*/

	if (cbor_decode_integer(&uvtemp, CborAny, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IPT report effective time.");
		return 0;
	}

	timestamp = uvtemp;
	writeTimestampLocal(timestamp, timestampBuffer);

	/*	Get node number of source node.				*/

	if (cbor_decode_integer(&fromNodeNbr, CborAny, &cursor, &unparsedBytes)
			< 1)
	{
		writeMemo("[?] Can't decode IPT report source node nbr.");
		return 0;
	}

	/*	Get node number of destination node.			*/

	if (cbor_decode_integer(&toNodeNbr, CborAny, &cursor, &unparsedBytes)
			< 1)
	{
		writeMemo("[?] Can't decode IPT report destination node nbr.");
		return 0;
	}

	/*	Print report heading.					*/

	isprintf(buf, sizeof buf, "[i] IRF path trace reported at %s",
			timestampBuffer);
	writeMemo(buf);
	isprintf(buf, sizeof buf, "[i] Passageways from node " UVAST_FIELDSPEC
" to node " UVAST_FIELDSPEC ":", fromNodeNbr, toNodeNbr);
	writeMemo(buf);

	/*	Get number of passageways in the path.			*/

	arrayLength = 0;	/*	Decode array of any size.	*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IMC briefing array.");
		return 0;
	}

	while (arrayLength > 0)
	{
		arrayLength--;
		if (cbor_decode_integer(&pwyNodeNbr, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode passageway node nbr.");
			return 0;
		}

		isprintf(buf, sizeof buf, "[i]\t" UVAST_FIELDSPEC, pwyNodeNbr);
		writeMemo(buf);
	}

	return 0;
}

int	irf_load_passageways(Bundle *bundle, Object bundleAddr)
{
	Sdr		sdr = getIonsdr();
	Object		iptblkElt;
	Object		iptblkAddr;
	ExtensionBlock	iptblock;
	int		passagewaysCount;
	char		*nodeNbrsArray;
	int		i;
	uvast		*nodeNbrPtr;

	/*	Load the bundle's list of passageways from the
	 *	array of passageway node numbers in the bundle's
	 *	IRF extension block.					*/

	iptblkElt = findExtensionBlock(bundle, IrfPassagewaysBlk, 0);
	if (iptblkElt == 0)
	{
		return 0;
	}

	iptblkAddr = sdr_list_data(sdr, iptblkElt);
	sdr_read(sdr, (char *) &iptblock, iptblkAddr, sizeof(ExtensionBlock));
 	if (iptblock.object == 0)
	{
		return 0;
	}

	passagewaysCount = iptblock.size / sizeof(uvast);
	if (passagewaysCount < 1)
	{
		return 0;
	}

	nodeNbrsArray = MTAKE(iptblock.size);
	if (nodeNbrsArray == NULL)
	{
		putErrmsg("Can't read node numbers array.",
				itoa(passagewaysCount));
		return -1;
	}

	sdr_read(sdr, nodeNbrsArray, iptblock.object, iptblock.size);
	nodeNbrPtr = (uvast *) nodeNbrsArray;
	for (i = 0; i < passagewaysCount; i++, nodeNbrPtr++)
	{
		if (sdr_list_insert_last(sdr, bundle->passageways, *nodeNbrPtr)
				< 0)
		{
			MRELEASE(nodeNbrsArray);
			putErrmsg("Can't load from IRF extension block.", NULL);
			return -1;
		}
	}

	MRELEASE(nodeNbrsArray);

	/*	Reinitialize the IRF extension block.			*/

	bundle->extensionsLength -= iptblock.length;
	sdr_write(sdr, bundleAddr, (char *) bundle, sizeof(Bundle));
	sdr_free(sdr, iptblock.bytes);
	iptblock.bytes = 0;
	iptblock.length = 0;
	sdr_free(sdr, iptblock.object);
	iptblock.object = 0;
	iptblock.size = 1;
	sdr_write(sdr, iptblkAddr, (char *) &iptblock, sizeof(ExtensionBlock));
	return 0;
}

int 	irf_identify_passageways(IonNode *terminusNode, Bundle *bundle,
		Lyst nominees)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	IrfCandidate	*bestCandidate = NULL;
	uvast		sourceNodeNbr;
	IonDB		iondb;
	Object		pwyElt;
	uint32_t	okayHomeRegion;
	uint32_t	okayOuterRegion;
	uint32_t	excludedHome;
	uvast		senderNodeNbr;
	RegionMember	senderMember;
	Object		senderMemberElt;
	PsmAddress	elt;
	PsmAddress	addr;
	IrfCandidate	*candidate;
	RegionMember	candidateMember;
	Object		candidateMemberElt;

	sourceNodeNbr = bundle->id.source.ssp.ipn.nodeNbr;
	if (terminusNode->viaPassageways == 0)
	{
		if (irf_initialize(terminusNode) < 0)
		{
			return -1;
		}
	}

	/*	Must now establish criteria for selecting among
	 *	possible next-hop passageways.  Selection depends
	 *	on the direction of forward progress of the bundle;
	 *	we must propagate throughout the network but never
	 *	backward through regions that have already been
	 *	traversed.
	 *
	 *	There are two general cases:
	 *
	 *	(a)	Propagation is inward, through the sub-
	 *		regions of the sending node's home region.
	 *
	 *	(b)	Propagation is outward, through the super-
	 *		regions of the sending node's outer region
	 *		(or of the sending node's home region, if
	 *		the sending node is not a passageway) and
	 *		the sub-regions of those super-regions
	 *		(peer regions).
	 *
	 *	In case (a) we will select only passageways for
	 *	which outer region number is the sending node's
	 *	home region number.
	 *
	 *	In case (b) we will select only passageways for
	 *	which either home region number is the sending
	 *	node's outer region number (these are passageways
	 *	to the super-region; for this purpose only, if
	 *	the sender has no outer region then its home region
	 *	number is used as its outer region number) OR outer
	 *	region number is the sending node's outer region
	 *	number (these are peer passageways) but home region
	 *	number is not the home region number of the node
	 *	from which the bundle was received (if any).
	 *
	 *	At the source of the bundle, upon initial xmit,
	 *	both cases apply: we initiate propagation in both
	 *	directions through the region tree.
	 *
	 *	The sending node never selects itself as a passageway.
	 *
	 *	Propagation of a bundle received from a node whose
	 *	home region or outer region is the receiving node's
	 *	outer region is inward.	 All other IRF propagation
	 *	is outward.						*/

	sdr_read(sdr, (char *) &iondb, getIonDbObject(), sizeof(IonDB));
	if (sourceNodeNbr == getOwnNodeNbr())
	{
		/*	Initial transmission of bundle at source.	*/

		senderNodeNbr = getOwnNodeNbr();
		if (iondb.regions[1].regionNbr != 0)
		{
			/*	Source node is itself a passageway,
			 *	so only outward passageways from
			 *	the source node's outer region are
			 *	helpful.				*/

			okayHomeRegion = iondb.regions[1].regionNbr;
			okayOuterRegion = iondb.regions[1].regionNbr;
			excludedHome = iondb.regions[0].regionNbr;
		}
		else
		{
			/*	Source node is a terminal node, so
			 *	outward passageways from the same
			 *	home region are fine.			*/

			okayHomeRegion = iondb.regions[0].regionNbr;
			okayOuterRegion = iondb.regions[0].regionNbr;
			excludedHome = 0;
		}
	}
	else
	{
		/*	Forwarding the bundle at a passageway node.
		 *	The last node in the bundle's passageways
		 *	list is the local node.				*/

		if (sdr_list_length(sdr, bundle->passageways) < 2)
		{
			/*	Bundle was received directly from
			 *	the source node.			*/

			senderNodeNbr = sourceNodeNbr;
		}
		else
		{
			/*	Bundle was received from the last
			 *	prior passageway; that's the sender.
			 *	So first get the last passageway in
			 *	the list (the current node)....		*/

			pwyElt = sdr_list_last(sdr, bundle->passageways);

			/*	...and then get the passageway that
			 *	preceded this one.			*/

			pwyElt = sdr_list_prev(sdr, pwyElt);
			senderNodeNbr = (uvast) sdr_list_data(sdr, pwyElt);
		}

		oK(findLocalNode(senderNodeNbr, &senderMember,
				&senderMemberElt));
		if (senderMember.homeRegionNbr
				== iondb.regions[1].regionNbr
		|| senderMember.outerRegionNbr
				== iondb.regions[1].regionNbr)
		{
			/*	Propagation is inward.		*/

			okayOuterRegion = iondb.regions[0].regionNbr;
			okayHomeRegion = 0;
			excludedHome = 0;
		}
		else	/*	Propagation is outward.		*/
		{
			okayHomeRegion = iondb.regions[1].regionNbr;
			okayOuterRegion = iondb.regions[1].regionNbr;
			excludedHome = senderMember.homeRegionNbr;
		}
	}

	/*	Populate the nominees list with candidate passageways.	*/

	for (elt = sm_list_first(ionwm, terminusNode->viaPassageways); elt;
			elt = sm_list_next(ionwm, elt))
	{
		addr = sm_list_data(ionwm, elt);
		candidate = (IrfCandidate *) psp(ionwm, addr);
		if (candidate->nodeNbr == getOwnNodeNbr())
		{
			continue;
		}

		oK(findLocalNode(candidate->nodeNbr, &candidateMember,
					&candidateMemberElt));
		if (!(candidateMember.homeRegionNbr == okayHomeRegion
		|| (candidateMember.outerRegionNbr == okayOuterRegion
		&& candidateMember.homeRegionNbr != excludedHome)))
		{
			/*	Don't propagate to this passageway.	*/

			continue;
		}

		switch (candidate->confirmTime)
		{
		case MAX_POSIX_TIME:	/*	Blacklisted.		*/
			continue;

		case 0:			/*	Potential, unconfirmed.	*/
			if (!lyst_insert_last(nominees,
					(void *) (uintptr_t)(candidate->nodeNbr)))
			{
				putErrmsg("Can't note potential passageway.",
						NULL);
				return -1;
			}

			continue;

		default:		/*	Confirmed.		*/
			if (bestCandidate == NULL
			|| candidate->confirmTime < bestCandidate->confirmTime)
			{
				bestCandidate = candidate;
			}
		}
	}

	if (bestCandidate)		/*	Found best way out.	*/
	{
		lyst_clear(nominees);
		if (!lyst_insert_last(nominees,
				(void *) (uintptr_t)(bestCandidate->nodeNbr)))
		{
			putErrmsg("Can't note best passageway.", NULL);
			return -1;
		}

		return 0;
	}

	/*	No confirmed passageway.				*/

	bundle->bundleProcFlags |= BDL_IS_NODE_LOCATOR;
	if (lyst_length(nominees) == 0
	&& sm_list_length(ionwm, terminusNode->viaPassageways) > 0)
	{
		/*	Re-locating this destination node.  Retry
		 *	all candidate passageways.			*/

		lyst_clear(nominees);
		for (elt = sm_list_first(ionwm, terminusNode->viaPassageways);
				elt; elt = sm_list_next(ionwm, elt))
		{
			addr = sm_list_data(ionwm, elt);
			candidate = (IrfCandidate *) psp(ionwm, addr);
			if (candidate->nodeNbr == getOwnNodeNbr())
			{
				continue;
			}

			oK(findLocalNode(candidate->nodeNbr, &candidateMember,
					&candidateMemberElt));
			if (!(candidateMember.homeRegionNbr == okayHomeRegion
			|| (candidateMember.outerRegionNbr == okayOuterRegion
			&& candidateMember.homeRegionNbr != excludedHome)))
			{
				/*	Skip this passageway.		*/

				continue;
			}

			candidate->confirmTime = 0;
			if (!lyst_insert_last(nominees,
					(void *) (uintptr_t)(candidate->nodeNbr)))
			{
				putErrmsg("Can't note potential passageway.",
						NULL);
				return -1;
			}
		}
	}

	/*	This nominees list is the best we can do.		*/

	return 0;
}
