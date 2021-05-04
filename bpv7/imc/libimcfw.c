/*
 *	libimcfw.c:	functions enabling the implementation of
 *			a multicast forwarder for the IMC endpoint
 *			ID scheme.
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "imcfw.h"

#define	IMC_DBNAME	"imcRoute"

static	char	imcEid[] = "imc:0.0";

/*	*	*	Globals used for IMC scheme service.	*	*/

static Object	_imcdbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static ImcDB	*_imcConstants()
{
	static ImcDB	buf;
	static ImcDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;

	if (db == NULL)
	{
		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _imcdbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(ImcDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(ImcDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

/*	*	*	IMC database mgt functions	*	*	*/

int	imcInit()
{
	Sdr	sdr = getIonsdr();
	Object	imcdbObject;
	ImcDB	imcdbBuf;

	/*	Recover the IMC database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	imcdbObject = sdr_find(sdr, IMC_DBNAME, NULL);
	switch (imcdbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Failed seeking IMC database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		imcdbObject = sdr_malloc(sdr, sizeof(ImcDB));
		if (imcdbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for IMC database.", NULL);
			return -1;
		}

		memset((char *) &imcdbBuf, 0, sizeof(ImcDB));
		imcdbBuf.groups = sdr_list_create(sdr);
		sdr_write(sdr, imcdbObject, (char *) &imcdbBuf, sizeof(ImcDB));
		sdr_catlg(sdr, IMC_DBNAME, 0, imcdbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create IMC database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(sdr);
	}

	oK(_imcdbObject(&imcdbObject));
	oK(_imcConstants());
	return 0;
}

Object	getImcDbObject()
{
	return _imcdbObject(NULL);
}

ImcDB	*getImcConstants()
{
	return _imcConstants();
}

/*	*	*	Multicast group mgt functions	*	*	*/

static Object	createGroup(uvast groupNbr, Object nextGroup)
{
	Sdr		sdr = getIonsdr();
	ImcDB		*db = _imcConstants();
	ImcGroup	group;
	Object		addr;
	Object		elt = 0;	/*	Default.		*/

#if IMCDEBUG
printf("Creating group (" UVAST_FIELDSPEC ").\n", groupNbr);
fflush(stdout);
#endif
	group.groupNbr = groupNbr;
	group.secUntilDelete = -1;
	group.isMember = 0;
	group.members = sdr_list_create(sdr);
	group.count[0] = 0;
	group.count[1] = 0;
	addr = sdr_malloc(sdr, sizeof(ImcGroup));
	if (addr)
	{
		sdr_write(sdr, addr, (char *) &group, sizeof(ImcGroup));
		if (nextGroup)
		{
			elt = sdr_list_insert_before(sdr, nextGroup, addr);
		}
		else
		{
			elt = sdr_list_insert_last(sdr, db->groups, addr);
		}
	}

	return elt;
}

static Object	locateGroup(uvast groupNbr, Object *nextGroup)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(ImcGroup, group);

	if (nextGroup)
	{
		*nextGroup = 0;	/*	Default.		*/
	}

	for (elt = sdr_list_first(sdr, (_imcConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, ImcGroup, group, sdr_list_data(sdr, elt));
		if (group->groupNbr < groupNbr)
		{
			continue;
		}

		if (group->groupNbr > groupNbr)
		{
			if (nextGroup)
			{
				*nextGroup = elt;
			}

			break;		/*	Same as end of list.	*/
		}

		return elt;		/*	Found group.		*/
	}

	return 0;
}

void	imcFindGroup(uvast groupNbr, Object *addr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	nextGroupElt;

	CHKVOID(addr);
	CHKVOID(eltp);
	CHKVOID(ionLocked());
	*eltp = 0;			/*	Default.		*/
	elt = locateGroup(groupNbr, &nextGroupElt);
	if (elt == 0)			/*	Not found.		*/
	{
		elt = createGroup(groupNbr, nextGroupElt);
	       	if (elt == 0)
		{
			putErrmsg("Can't create multicast group.", NULL);
			return;
		}
	}

	*addr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

/*	*	Public IMC library functions.	*	*	*	*/

int	imcHandleBriefing(BpDelivery *dlv, unsigned char *cursor,
		unsigned int unparsedBytes)
{
	Sdr		sdr = getIonsdr();
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	uvast		arrayLength;
	uvast		groupNbr;
	Object		groupAddr;
	Object		groupElt;
	ImcGroup	group;
	Object		elt;
	uvast		nodeNbr;
	Object		iondbObj;
	IonDB		iondb;
	int		sourceRegion;
	uint32_t	sourceRegionNbr;
	int		destinationRegion;
	ImcPetition	petition;

#if IMCDEBUG
puts("Handling briefing.");
#endif
	if (imcInit() < 0)
	{
		putErrmsg("Can't initialize IMC database.", NULL);
		return -1;
	}

	if (parseEidString(dlv->bundleSourceEid, &metaEid, &vscheme,
			&vschemeElt) == 0 || vscheme->codeNumber != ipn)
	{
		/*	Can't determine sending node number.		*/

		writeMemoNote("[?] Invalid sender of IMC briefing",
				dlv->bundleSourceEid);
		return 0;
	}

	/*	Get number of group numbers in the briefing.		*/

	arrayLength = 0;	/*	Decode array of any size.	*/
	if (cbor_decode_array_open(&arrayLength, &cursor, &unparsedBytes) < 1)
	{
		writeMemo("[?] Can't decode IMC briefing array.");
		return 0;
	}

	while (arrayLength > 0)
	{
		arrayLength--;
		if (cbor_decode_integer(&groupNbr, CborAny, &cursor,
				&unparsedBytes) < 1)
		{
			writeMemo("[?] Can't decode IMC briefing group nbr.");
			return 0;
		}

		CHKERR(sdr_begin_xn(sdr));
		imcFindGroup(groupNbr, &groupAddr, &groupElt);
		if (groupElt == 0)	/*	System failure.		*/
		{
			sdr_cancel_xn(sdr);
			break;
		}
		
		sdr_stage(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
		nodeNbr = 0;
		for (elt = sdr_list_first(sdr, group.members); elt;
				elt = sdr_list_next(sdr, elt))
		{
			nodeNbr = sdr_list_data(sdr, elt);
			if (nodeNbr < metaEid.elementNbr)
			{
				continue;
			}


			break;	/*	Insertion point for node.	*/
		}

		if (nodeNbr == metaEid.elementNbr)
		{
			/*	Duplicate group number in briefing.	*/

			sdr_exit_xn(sdr);
			continue;
		}

		/*	Must add new member of group at this point.	*/
#if IMCDEBUG
printf("Adding node " UVAST_FIELDSPEC " to group " UVAST_FIELDSPEC ".\n", metaEid.elementNbr, groupNbr);
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

		/*	If node is a passageway, propagate the
		 *	asserted group membership to the other
		 *	region as necessary.	 			*/

		iondbObj = getIonDbObject();
		sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
		if (iondb.regions[1].regionNbr != 0)
		{
		/*	Node is a passageway from its home region to
		 *	the immediate encompassing region.		*/

			sourceRegion = ionRegionOf(metaEid.elementNbr,
					getOwnNodeNbr(), &sourceRegionNbr);
			if (sourceRegion < 0)
			{
				putErrmsg("IMC system error.", NULL);
				sdr_cancel_xn(sdr);
				return -1;
			}

			destinationRegion = 0 - sourceRegion;
			group.count[sourceRegion] += 1;
			if (group.count[sourceRegion] == 1)
			{
				petition.groupNbr = groupNbr;
				petition.isMember = 1;
				if (imcSendPetition(&petition,
						destinationRegion) < 0)
				{
					putErrmsg("Join propagation failed.",
							NULL);
					sdr_cancel_xn(sdr);
					return -1;
				}
			}

			sdr_write(sdr, groupAddr, (char *) &group,
					sizeof(ImcGroup));
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed handling briefing.", NULL);
			return -1;
		}
	}

	return 0;
}

int	imcSendDispatch(char *destEid, uint32_t toRegion, unsigned char *buffer,
		int length)
{
	Sdr		sdr = getIonsdr();
	char		sourceEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		sourceData;
	Object		payloadZco;
	unsigned int	ttl = 86400;	/*	Seconds; 1 day.		*/
	BpAncillaryData	ancillary = { 0, 0, 255 };

	ancillary.imcRegionNbr = toRegion;
	isprintf(sourceEid, sizeof sourceEid, "ipn:" UVAST_FIELDSPEC ".0",
			getOwnNodeNbr());
	CHKERR(parseEidString(sourceEid, &sourceMetaEid, &vscheme,
			&vschemeElt));
	CHKERR(sdr_begin_xn(sdr));
	sourceData = sdr_malloc(sdr, length);
	if (sourceData == 0)
	{
		putErrmsg("No space for source data.", NULL);
		sdr_exit_xn(sdr);
		return -1;
	}

	sdr_write(sdr, sourceData, (char *) buffer, length);

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that allocating this ZCO space is non-
	 *	negotiable: for IMC petitions, allocation of ZCO
	 *	space can never be denied or delayed.			*/

	payloadZco = zco_create(sdr, ZcoSdrSource, sourceData, 0, 0 - length,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0
	|| payloadZco == (Object) ERROR || payloadZco == 0)
	{
		putErrmsg("Can't create IMC dispatch payload.", NULL);
		return -1;
	}

	/*	Note: it is possible for an IMC bundle to be sent
	 *	to a node that does not exist yet or does not yet
	 *	have all convergence-layer interfaces fully
	 *	configured.  (In particular, a bundle sent via
	 *	LTP may arrive at its proximate destination before
	 *	BP has started its ltpcli daemon.  In this case,
	 *	the receiving LTP service-layer adapter will be
	 *	unable to deliver the block content because it's
	 *	destined for an LTP client - BP - that the LTP
	 *	engine doesn't know about yet; the LTP engine
	 *	will thereupon cancel the import session,
	 *	causing LTP session failure at the sender.)
	 *	Any such convergence-layer transmission failure
	 *	will cause the sending CLA to call the BPA's
	 *	handleXmitFailure function, causing the bundle
	 *	to be reforwarded.  Reforwarding a transmitted
	 *	IMC bundle looks - to imcfw - exactly like
	 *	relaying a received IMC bundle except that the
	 *	local node is not present in the imc extension
	 *	block's array of destinations, and therefore
	 *	need not be removed.					*/

#if IMCDEBUG
puts("Transmitting dispatch.");
#endif

	/*	Note that ttl must be converted from seconds to
	 *	milliseconds for BP processing.				*/

	switch (bpSend(&sourceMetaEid, destEid, NULL, ttl * 1000,
			BP_EXPEDITED_PRIORITY, NoCustodyRequested, 0, 0,
			&ancillary, payloadZco, NULL, 0))
	{
	case -1:
		putErrmsg("Can't send IMC dispatch.", NULL);
		return -1;

	case 0:
		putErrmsg("IMC dispatch not sent.", NULL);

		/*	Intentional fall-through to next case.	*/

	default:
		return 0;
	}
}

int	imcSendPetition(ImcPetition *petition, uint32_t toRegion)
{
	unsigned char	buffer[64];
	unsigned char	*cursor;
	uvast		uvtemp;
	int		petitionLength;
	int		result = 0;

#if IMCDEBUG
printf("Sending petition for group " UVAST_FIELDSPEC ".\n", petition->groupNbr);
#endif
	if (imcInit() < 0)
	{
		putErrmsg("Can't attach to IMC database.", NULL);
		return -1;
	}

	/*	Use buffer to serialize petition message.		*/

	cursor = buffer;

	/*	Dispatch message is an array of 2 items.		*/

	uvtemp = 2;
	oK(cbor_encode_array_open(uvtemp, &cursor));

	/*	First item of array (petition) is the group number.	*/

	uvtemp = petition->groupNbr;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Second item of array is the membership switch.		*/

	uvtemp = petition->isMember;
	oK(cbor_encode_integer(uvtemp, &cursor));

	/*	Now multicast the petition.				*/

	petitionLength = cursor - buffer;
	if (imcSendDispatch(imcEid, toRegion, buffer, petitionLength) < 0)
	{
		result = -1;
	}

	return result;
}
