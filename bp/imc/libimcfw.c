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

#include "imcP.h"

#ifndef IMCDEBUG
#define	IMCDEBUG	0
#endif

#define	IMC_DBNAME	"imcRoute"

static void	destroyGroup(Object groupElt);

static Object	createNodeId(Sdr sdr, uvast nodeNbr)
{
	NodeId	node;
	Object	obj;

	node.nbr = nodeNbr;
	obj = sdr_malloc(sdr, sizeof(NodeId));
	if (obj)
	{
		sdr_write(sdr, obj, (char *) &node, sizeof(NodeId));
	}

	return obj;
}

static void	destroyNodeId(Sdr sdr, Object elt, void *arg)
{
	sdr_free(sdr, sdr_list_data(sdr, elt));
}

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

/*	*	*	Petition exchange functions	*	*	*/

static int	sendPetition(uvast nodeNbr, char *buffer, int length)
{
	char		sourceEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	unsigned int	ttl = 86400;
	BpAncillaryData	ancillary = { 0, 0, 255 };
	Sdr		sdr = getIonsdr();
	Object		sourceData;
	Object		payloadZco;
	char		destEid[32];

	isprintf(sourceEid, sizeof sourceEid, "ipn:%u.0", getOwnNodeNbr());
	oK(parseEidString(sourceEid, &sourceMetaEid, &vscheme, &vschemeElt));
	CHKERR(sdr_begin_xn(sdr));
	sourceData = sdr_malloc(sdr, length);
	if (sourceData == 0)
	{
		putErrmsg("No space for source data.", NULL);
		sdr_exit_xn(sdr);
		return -1;
	}

	sdr_write(sdr, sourceData, buffer, length);

	/*	Pass additive inverse of length to zco_create to
	 *	indicate that allocating this ZCO space is non-
	 *	negotiable: for IMC petitions, allocation of ZCO
	 *	space can never be denied or delayed.			*/

	payloadZco = zco_create(sdr, ZcoSdrSource, sourceData, 0, 0 - length,
			ZcoOutbound);
	if (sdr_end_xn(sdr) < 0 || payloadZco == (Object) ERROR
	|| payloadZco == 0)
	{
		putErrmsg("Can't create IMC petition.", NULL);
		return -1;
	}

	isprintf(destEid, sizeof destEid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	switch (bpSend(&sourceMetaEid, destEid, NULL, ttl,
			BP_EXPEDITED_PRIORITY, NoCustodyRequested, 0, 0,
			&ancillary, payloadZco, NULL, BP_MULTICAST_PETITION))
	{
	case -1:
		putErrmsg("Can't send IMC petition.", NULL);
		return -1;

	case 0:
		putErrmsg("IMC petition not sent.", NULL);

		/*	Intentional fall-through to next case.	*/

	default:
		return 0;
	}
}

static int	forwardPetition(ImcGroup *group, int isMember,
			uvast senderNodeNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned char	adminRecordFlag = (BP_MULTICAST_PETITION << 4);
	Sdnv		groupNbrSdnv;
	int		petitionLength;
	char		*buffer;
	char		*cursor;
	int		result = 0;
	Object		elt;
			OBJ_POINTER(NodeId, node);

	encodeSdnv(&groupNbrSdnv, group->groupNbr);
	petitionLength = 1 + groupNbrSdnv.length + 1;
	buffer = MTAKE(petitionLength);
	if (buffer == NULL)
	{
		putErrmsg("Can't construct IMC petition.", NULL);
		return -1;
	}

	cursor = buffer;

	*cursor = adminRecordFlag;
	cursor++;

	memcpy(cursor, groupNbrSdnv.text, groupNbrSdnv.length);
	cursor += groupNbrSdnv.length;

	*cursor = (isMember ? 1 : 0);
	cursor++;

	for (elt = sdr_list_first(sdr, (_imcConstants())->kin); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, NodeId, node, sdr_list_data(sdr, elt));
		if (node->nbr == senderNodeNbr)
		{
			continue;	/*	Would be an echo.	*/
		}

		if (sendPetition(node->nbr, buffer, petitionLength) < 0)
		{
			result = -1;
			break;
		}
	}

	MRELEASE(buffer);
	return result;
}

int	imcParsePetition(void **argp, unsigned char *cursor, int unparsedBytes)
{
	ImcPetition	*petition;

	*argp = NULL;		/*	Default.			*/
	petition = MTAKE(sizeof(ImcPetition));
	if (petition == NULL)
	{
		putErrmsg("Can't allocate IMC petition work area.", NULL);
		return -1;
	}

	memset((char *) petition, 0, sizeof(ImcPetition));
	extractSdnv(&(petition->groupNbr), &cursor, &unparsedBytes);
	if (unparsedBytes < 1)
	{
		writeMemo("[?] IMC petition too short to parse.");
		return 0;
	}

	petition->isMember = (*cursor != 0);
	*argp = petition;
	return 1;
}

/*	*	*	Routing information mgt functions	*	*/

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
		imcdbBuf.kin = sdr_list_create(sdr);
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

/*	*	*	Multicast tree mgt functions	*	*	*/

static Object	locateRelative(uvast nodeNbr, Object *nextRelative)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(NodeId, node);

	if (nextRelative) *nextRelative = 0;	/*	Default.	*/
	for (elt = sdr_list_first(sdr, (_imcConstants())->kin); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, NodeId, node, sdr_list_data(sdr, elt));
		if (node->nbr < nodeNbr)
		{
			continue;
		}

		if (node->nbr > nodeNbr)
		{
			if (nextRelative) *nextRelative = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

int	imc_addKin(uvast nodeNbr, int isParent)
{
	Sdr		sdr = getIonsdr();
	Object		dbObj = getImcDbObject();
	ImcDB		db;
	Object		nextRelative;
	Object		addr;
	Object		elt;
			OBJ_POINTER(ImcGroup, group);
	unsigned char	adminRecordFlag = (BP_MULTICAST_PETITION << 4);
	Sdnv		groupNbrSdnv;
	int		petitionLength;
	char		*buffer;
	char		*cursor;

	CHKERR(nodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbObj, sizeof(ImcDB));
	if (locateRelative(nodeNbr, &nextRelative) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate multicast kin", utoa(nodeNbr));
		return 0;
	}

	/*	Okay to add this relative to the database.		*/

	addr = createNodeId(sdr, nodeNbr);
	if (nextRelative)
	{
		oK(sdr_list_insert_before(sdr, nextRelative, addr));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, db.kin, addr));
	}

	if (isParent)
	{
		db.parent = nodeNbr;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(ImcDB));
	}

	/*	Send new relative an assertion petition for every
	 *	group that has at least one member.			*/

	for (elt = sdr_list_first(sdr, (_imcConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, ImcGroup, group, sdr_list_data(sdr, elt));
		if (sdr_list_length(sdr, group->members) == 0
		&& group->isMember == 0)
		{
			continue;
		}

		encodeSdnv(&groupNbrSdnv, group->groupNbr);
		petitionLength = 1 + groupNbrSdnv.length + 1;
		buffer = MTAKE(petitionLength);
		if (buffer == NULL)
		{
			putErrmsg("Can't construct IMC petition.", NULL);
			sdr_cancel_xn(sdr);
			break;
		}

		cursor = buffer;

		*cursor = adminRecordFlag;
		cursor++;

		memcpy(cursor, groupNbrSdnv.text, groupNbrSdnv.length);
		cursor += groupNbrSdnv.length;

		*cursor = 1;
		cursor++;

		if (sendPetition(nodeNbr, buffer, petitionLength) < 0)
		{
			putErrmsg("Can't send subscription to new relative.",
					itoa(group->groupNbr));
			sdr_cancel_xn(sdr);
			break;
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add relative.", itoa(nodeNbr));
		return -1;
	}

	return 1;
}

int	imc_updateKin(uvast nodeNbr, int isParent)
{
	Sdr	sdr = getIonsdr();
	Object	dbObj = getImcDbObject();
	ImcDB	db;

	CHKERR(nodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbObj, sizeof(ImcDB));
	if (locateRelative(nodeNbr, NULL) == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This node is not kin", utoa(nodeNbr));
		return 0;
	}

	/*	Okay to update status of this relative.			*/

	if (isParent)	/*	This is the local node's new parent.	*/
	{
		db.parent = nodeNbr;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(ImcDB));
	}
	else	/*	This node is no longer the local node's parent.	*/
	{
		if (db.parent == nodeNbr)
		{
			db.parent = 0;
			sdr_write(sdr, dbObj, (char *) &db, sizeof(ImcDB));
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update relative.", itoa(nodeNbr));
		return -1;
	}

	return 1;
}

void	imc_removeKin(uvast nodeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	dbObj = getImcDbObject();
	uvast	ownNodeNbr = getOwnNodeNbr();
	ImcDB	db;
	Object	elt;
	Object	elt2;
	Object	nextElt;
		OBJ_POINTER(ImcGroup, group);
	Object	elt3;
		OBJ_POINTER(NodeId, node);

	CHKVOID(nodeNbr);
	CHKVOID(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbObj, sizeof(ImcDB));
	elt = locateRelative(nodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This node is not kin", utoa(nodeNbr));
		return;
	}

	/*	Okay to remove this relative.				*/

	sdr_list_delete(sdr, elt, NULL, NULL);
	if (db.parent == nodeNbr)
	{
		db.parent = 0;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(ImcDB));
	}

	/*	Cancel all of this relative's group memberships.	*/
	
	for (elt2 = sdr_list_first(sdr, (_imcConstants())->groups); elt2;
			elt2 = nextElt)
	{
		nextElt = sdr_list_next(sdr, elt2);
		GET_OBJ_POINTER(sdr, ImcGroup, group, sdr_list_data(sdr, elt2));
		for (elt3 = sdr_list_first(sdr, group->members); elt3;
				elt3 = sdr_list_next(sdr, elt3))
		{
			GET_OBJ_POINTER(sdr, NodeId, node,
					sdr_list_data(sdr, elt3));
			if (node->nbr == nodeNbr)
			{
				break;
			}
		}

		if (elt3)	/*	Node is a member of this group.	*/
		{
			sdr_list_delete(sdr, elt3, NULL, NULL);
			if (sdr_list_length(sdr, group->members) == 0
			&& group->isMember == 0)
			{
				/*	No more members; unsubscribe.	*/

				if (forwardPetition(group, 0, ownNodeNbr) < 0)
				{
					sdr_cancel_xn(sdr);
					break;
				}
				else
				{
					destroyGroup(elt2);
				}
			}
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove relative.", itoa(nodeNbr));
	}
}

/*	*	*	Multicast group mgt functions	*	*	*/

static Object	locateGroup(uvast groupNbr, Object *nextGroup)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(ImcGroup, group);

	if (nextGroup) *nextGroup = 0;	/*	Default.		*/
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
			if (nextGroup) *nextGroup = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	imcFindGroup(uvast groupNbr, Object *addr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	CHKVOID(addr);
	CHKVOID(eltp);
	CHKVOID(ionLocked());
	*eltp = 0;			/*	Default.		*/
	elt = locateGroup(groupNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*addr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

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
	group.isMember = 0;
	group.timestamp.seconds = 0;
	group.timestamp.count = 0;
	group.members = sdr_list_create(sdr);
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

static void	destroyGroup(Object groupElt)
{
	Sdr	sdr = getIonsdr();
	Object	groupAddr;
		OBJ_POINTER(ImcGroup, group);

	groupAddr = sdr_list_data(sdr, groupElt);
	GET_OBJ_POINTER(sdr, ImcGroup, group, groupAddr);
	sdr_list_destroy(sdr, group->members, destroyNodeId, NULL);
	sdr_free(sdr, groupAddr);
	sdr_list_delete(sdr, groupElt, NULL, NULL);
}

int	imcHandlePetition(void *arg, BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	ImcPetition	*petition = (ImcPetition *) arg;
	uvast		groupNbr;
	int		isMember;	/*	Boolean.		*/
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		nextRelative;
	Object		groupElt;
	Object		nextGroup;
			OBJ_POINTER(ImcGroup, group);
	Object		elt;
			OBJ_POINTER(NodeId, node);
	Object		addr;

	groupNbr = petition->groupNbr;
	isMember = petition->isMember;
	MRELEASE(petition);
	if (parseEidString(dlv->bundleSourceEid, &metaEid, &vscheme,
			&vschemeElt) < 0
	|| vscheme->cbhe == 0 || vscheme->unicast == 0)
	{
		/*	Can't determine sending node number.		*/

		writeMemoNote("[?] Invalid sender of IMC petition",
				dlv->bundleSourceEid);
		return 0;
	}

#if IMCDEBUG
printf("Handling type-%d petition from " UVAST_FIELDSPEC " at \
node " UVAST_FIELDSPEC ".\n", isMember, metaEid.nodeNbr, getOwnNodeNbr());
fflush(stdout);
#endif
	CHKERR(sdr_begin_xn(sdr));
	if (locateRelative(metaEid.nodeNbr, &nextRelative) == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Ignoring petition from non-kin",
				utoa(metaEid.nodeNbr));
		return 0;
	}

	groupElt = locateGroup(groupNbr, &nextGroup);
	if (groupElt == 0)
	{
		if (isMember == 0)	/*	Nothing to do.		*/
		{
#if IMCDEBUG
puts("Ignoring cancellation of membership in nonexistent group.");
fflush(stdout);
#endif
			sdr_exit_xn(sdr);
			return 0;
		}

		groupElt = createGroup(groupNbr, nextGroup);
		if (groupElt == 0)
		{
			putErrmsg("Can't handle IMC petition.", NULL);
			return sdr_end_xn(sdr);
		}
	}

	GET_OBJ_POINTER(sdr, ImcGroup, group, sdr_list_data(sdr, groupElt));
	if (dlv->bundleCreationTime.seconds < group->timestamp.seconds
	|| (dlv->bundleCreationTime.seconds == group->timestamp.seconds
	 && dlv->bundleCreationTime.count < group->timestamp.count))
	{
#if IMCDEBUG
puts("Silently ignoring non-current petition.");
fflush(stdout);
#endif
		return sdr_end_xn(sdr);	/*	Not a current petition.	*/
	}

	group->timestamp.seconds = dlv->bundleCreationTime.seconds;
	group->timestamp.count = dlv->bundleCreationTime.count;
	if (isMember)		/*	New member of group.		*/
	{
		for (elt = sdr_list_first(sdr, group->members); elt;
				elt = sdr_list_next(sdr, elt))
		{
			GET_OBJ_POINTER(sdr, NodeId, node,
					sdr_list_data(sdr, elt));
			if (node->nbr < metaEid.nodeNbr)
			{
				continue;
			}

			if (node->nbr == metaEid.nodeNbr)
			{
#if IMCDEBUG
puts("Ignoring assertion.");
fflush(stdout);
#endif
				/*	Nothing to do: current member.	*/

				return sdr_end_xn(sdr);
			}

			break;
		}

		/*	Must add new member of group at this point.	*/
#if IMCDEBUG
printf("Adding member " UVAST_FIELDSPEC " to group " UVAST_FIELDSPEC ".\n", metaEid.nodeNbr, groupNbr);
fflush(stdout);
#endif
		addr = createNodeId(sdr, metaEid.nodeNbr);
		if (elt)
		{
			oK(sdr_list_insert_before(sdr, elt, addr));
		}
		else
		{
			oK(sdr_list_insert_last(sdr, group->members, addr));
		}

		/*	Assert interest (perhaps redundant) to every
		 *	relative other than the new member itself.	*/

		if (forwardPetition(group, 1, metaEid.nodeNbr) < 0)
		{
			sdr_cancel_xn(sdr);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Failed handling assertion petition.", NULL);
			return -1;
		}

		return 0;
	}

	/*	Member is withdrawing from the group.			*/

	for (elt = sdr_list_first(sdr, group->members); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, NodeId, node, sdr_list_data(sdr, elt));
		if (node->nbr < metaEid.nodeNbr)
		{
			continue;
		}

		break;
	}

	/*	Either found this subscriber or reached end of list.	*/

	if (elt == 0)
	{
		/*	Nothing to do: not a current member.		*/
#if IMCDEBUG
puts("Ignoring cancellation by non-member.");
fflush(stdout);
#endif
		return sdr_end_xn(sdr);
	}

#if IMCDEBUG
printf("Deleting member " UVAST_FIELDSPEC ".\n", metaEid.nodeNbr);
fflush(stdout);
#endif
	sdr_list_delete(sdr, elt, NULL, NULL);

	/*	If no relatives (including self) are members of this
	 *	group any longer, then must now unsubscribe from this
	 *	group altogether.					*/

	if (sdr_list_length(sdr, group->members) == 0 && group->isMember == 0)
	{
#if IMCDEBUG
printf("Canceling own membership in group (" UVAST_FIELDSPEC ").\n",
getOwnNodeNbr());
fflush(stdout);
#endif
		if (forwardPetition(group, 0, getOwnNodeNbr()) < 0)
		{
			sdr_cancel_xn(sdr);
		}
		else
		{
#if IMCDEBUG
puts("Destroying group.");
fflush(stdout);
#endif
			destroyGroup(groupElt);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed handling cancellation petition.", NULL);
		return -1;
	}

	return 0;
}

int	imcJoin(uvast groupNbr)
{
	Sdr		sdr = getIonsdr();
	uvast		ownNodeNbr = getOwnNodeNbr();
	Object		groupElt;
	Object		nextGroup;
	Object		groupAddr;
	ImcGroup	group;

	CHKERR(sdr_begin_xn(sdr));
	groupElt = locateGroup(groupNbr, &nextGroup);
	if (groupElt == 0)
	{
		groupElt = createGroup(groupNbr, nextGroup);
		if (groupElt == 0)
		{
			putErrmsg("Can't join IMC group.", NULL);
			return sdr_end_xn(sdr);
		}
	}

	groupAddr = sdr_list_data(sdr, groupElt);
	sdr_stage(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
	group.isMember = 1;
	sdr_write(sdr, groupAddr, (char *) &group, sizeof(ImcGroup));

	/*	Propagate membership to immediate relatives.  (This
	 *	may be redundant.)					*/

#if IMCDEBUG
printf("Node " UVAST_FIELDSPEC " asserting own membership in group " UVAST_FIELDSPEC ".\n", getOwnNodeNbr(), groupNbr);
fflush(stdout);
#endif
	if (forwardPetition(&group, 1, ownNodeNbr) < 0)
	{
		sdr_cancel_xn(sdr);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed joining IMC group.", NULL);
		return -1;
	}

	return 0;
}

int	imcLeave(uvast groupNbr)
{
	Sdr		sdr = getIonsdr();
	uvast		ownNodeNbr = getOwnNodeNbr();
	Object		groupElt;
	Object		nextGroup;
	Object		groupAddr;
	ImcGroup	group;

	CHKERR(sdr_begin_xn(sdr));
	groupElt = locateGroup(groupNbr, &nextGroup);
	if (groupElt == 0)
	{
		/*	Nonexistent group, so nothing to do.		*/

		sdr_exit_xn(sdr);
		return 0;
	}

	groupAddr = sdr_list_data(sdr, groupElt);
	sdr_stage(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
	group.isMember = 0;

	/*	If no relatives are members of this group any longer,
	 *	then must now unsubscribe from this group altogether.	*/

	if (sdr_list_length(sdr, group.members) == 0)
	{
#if IMCDEBUG
printf("Cancelling own membership in group (" UVAST_FIELDSPEC ").\n",
getOwnNodeNbr());
fflush(stdout);
#endif
		if (forwardPetition(&group, 0, ownNodeNbr) < 0)
		{
			sdr_cancel_xn(sdr);
		}
		else
		{
#if IMCDEBUG
printf("Destroying group (" UVAST_FIELDSPEC ").\n", getOwnNodeNbr());
fflush(stdout);
#endif
			destroyGroup(groupElt);
		}
	}
	else
	{
		sdr_write(sdr, groupAddr, (char *) &group, sizeof(ImcGroup));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed leaving IMC group.", NULL);
		return -1;
	}

	return 0;
}
