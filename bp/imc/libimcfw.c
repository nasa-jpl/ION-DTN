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

#define	IMC_DBNAME	"imcRoute"

static void	destroyGroup(Object groupElt);

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

	if (db == NULL)
	{
		sdr_read(getIonsdr(), (char *) &buf, _imcdbObject(NULL),
				sizeof(ImcDB));
		db = &buf;
	}

	return db;
}

/*	*	*	Petition exchange functions	*	*	*/

static int	sendPetition(unsigned long nodeNbr, char *buffer, int length)
{
	char		sourceEid[32];
	MetaEid		sourceMetaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	unsigned int	ttl = 86400;
	BpExtendedCOS	ecos = { 0, 0, 255 };
	Sdr		sdr = getIonsdr();
	Object		sourceData;
	Object		payloadZco;
	char		destEid[32];
	Object		bundleObj;

	isprintf(sourceEid, sizeof sourceEid, "ipn:%u.0", getOwnNodeNbr());
	oK(parseEidString(sourceEid, &sourceMetaEid, &vscheme, &vschemeElt));
	sdr_begin_xn(sdr);
	sourceData = sdr_malloc(sdr, length);
	if (sourceData == 0)
	{
		putErrmsg("No space for source data.", NULL);
		sdr_exit_xn(sdr);
		return-1;
	}

	sdr_write(sdr, sourceData, buffer, length);
	payloadZco = zco_create(sdr, ZcoSdrSource, sourceData, 0, length);
	if (sdr_end_xn(sdr) < 0 || payloadZco == 0)
	{
		putErrmsg("Can't create IMC petition.", NULL);
		return -1;
	}

	isprintf(destEid, sizeof destEid, "ipn:%lu.0", nodeNbr);
	switch (bpSend(&sourceMetaEid, destEid, NULL, ttl,
			BP_EXPEDITED_PRIORITY, NoCustodyRequested, 0, 0, &ecos,
			payloadZco, &bundleObj, BP_MULTICAST_PETITION))
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
			unsigned long senderNodeNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned char	adminRecordFlag = (BP_MULTICAST_PETITION << 4);
	Sdnv		groupNbrSdnv;
	int		petitionLength;
	char		*buffer;
	char		*cursor;
	int		result = 0;
	Object		elt;
	unsigned long	nodeNbr;

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
		nodeNbr = (unsigned long) sdr_list_data(sdr, elt);
		if (nodeNbr == senderNodeNbr)
		{
			continue;	/*	Would be an echo.	*/
		}

		if (sendPetition(nodeNbr, buffer, petitionLength) < 0)
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

	sdr_begin_xn(sdr);
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

static Object	locateRelative(unsigned long nodeNbr, Object *nextRelative)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	unsigned long	relative;

	if (nextRelative) *nextRelative = 0;	/*	Default.	*/
	for (elt = sdr_list_first(sdr, (_imcConstants())->kin); elt;
			elt = sdr_list_next(sdr, elt))
	{
		relative = (unsigned long) sdr_list_data(sdr, elt);
		if (relative < nodeNbr)
		{
			continue;
		}

		if (relative > nodeNbr)
		{
			if (nextRelative) *nextRelative = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

int	imc_addKin(unsigned long nodeNbr, int isParent)
{
	Sdr		sdr = getIonsdr();
	Object		dbObj = getImcDbObject();
	ImcDB		db;
	Object		nextRelative;
	Object		elt;
			OBJ_POINTER(ImcGroup, group);
	unsigned char	adminRecordFlag = (BP_MULTICAST_PETITION << 4);
	Sdnv		groupNbrSdnv;
	int		petitionLength;
	char		*buffer;
	char		*cursor;

	CHKERR(nodeNbr);
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &db, dbObj, sizeof(ImcDB));
	if (locateRelative(nodeNbr, &nextRelative) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate multicast kin", utoa(nodeNbr));
		return 0;
	}

	/*	Okay to add this relative to the database.		*/

	if (nextRelative)
	{
		oK(sdr_list_insert_before(sdr, nextRelative, nodeNbr));
	}
	else
	{
		oK(sdr_list_insert_last(sdr, db.kin, nodeNbr));
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
		&& group->endpoints == 0)
		{
			continue;
		}

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

int	imc_updateKin(unsigned long nodeNbr, int isParent)
{
	Sdr	sdr = getIonsdr();
	Object	dbObj = getImcDbObject();
	ImcDB	db;

	CHKERR(nodeNbr);
	sdr_begin_xn(sdr);
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

void	imc_removeKin(unsigned long nodeNbr)
{
	Sdr		sdr = getIonsdr();
	Object		dbObj = getImcDbObject();
	unsigned long	ownNodeNbr = getOwnNodeNbr();
	ImcDB		db;
	Object		elt;
	Object		elt2;
	Object		nextElt;
			OBJ_POINTER(ImcGroup, group);
	Object		elt3;

	CHKVOID(nodeNbr);
	sdr_begin_xn(sdr);
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
			if (nodeNbr == (unsigned long) sdr_list_data(sdr, elt3))
			{
				break;
			}
		}

		if (elt3)	/*	Node is a member of this group.	*/
		{
			sdr_list_delete(sdr, elt3, NULL, NULL);
			if (sdr_list_length(sdr, group->members) == 0
			&& group->endpoints == 0)
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

static Object	locateGroup(unsigned long groupNbr, Object *nextGroup)
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

void	imcFindGroup(unsigned long groupNbr, Object *addr, Object *eltp)
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

static Object	createGroup(unsigned long groupNbr, Object nextGroup)
{
	Sdr		sdr = getIonsdr();
	ImcDB		*db = _imcConstants();
	ImcGroup	group;
	Object		addr;
	Object		elt = 0;	/*	Default.		*/

	group.groupNbr = groupNbr;
	group.endpoints = 0;
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
	sdr_list_destroy(sdr, group->members, NULL, NULL);
	sdr_free(sdr, groupAddr);
	sdr_list_delete(sdr, groupElt, NULL, NULL);
}

int	imcHandlePetition(void *arg, BpDelivery *dlv)
{
	Sdr		sdr = getIonsdr();
	ImcPetition	*petition = (ImcPetition *) arg;
	unsigned long	groupNbr;
	int		isMember;	/*	Boolean.		*/
	MetaEid		metaEid;
	VScheme		*vscheme;
	PsmAddress	vschemeElt;
	Object		groupElt;
	Object		nextGroup;
			OBJ_POINTER(ImcGroup, group);
	Object		elt;
	unsigned long	nodeNbr;

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

	sdr_begin_xn(sdr);
	groupElt = locateGroup(groupNbr, &nextGroup);
	if (groupElt == 0)
	{
		if (isMember == 0)	/*	Nothing to do.		*/
		{
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
	if (isMember)		/*	New member of group.		*/
	{
		for (elt = sdr_list_first(sdr, group->members); elt;
				elt = sdr_list_next(sdr, elt))
		{
			nodeNbr = (unsigned long) sdr_list_data(sdr, elt);
			if (nodeNbr < metaEid.nodeNbr)
			{
				continue;
			}

			if (nodeNbr == metaEid.nodeNbr)
			{
				/*	Nothing to do: current member.	*/

				sdr_exit_xn(sdr);
				return 0;
			}

			break;
		}

		/*	Must add new member of group at this point.	*/

		if (elt)
		{
			oK(sdr_list_insert_before(sdr, elt, metaEid.nodeNbr));
		}
		else
		{
			oK(sdr_list_insert_last(sdr, group->members,
					metaEid.nodeNbr));
		}

		/*	If this is the first relative (including self)
		 *	that is a member of this group, then must now
		 *	subscribe on that member's behalf.		*/

		if (sdr_list_length(sdr, group->members) == 1
		&& group->endpoints == 0)
		{
			if (forwardPetition(group, 1, metaEid.nodeNbr) < 0)
			{
				sdr_cancel_xn(sdr);
			}
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
		nodeNbr = (unsigned long) sdr_list_data(sdr, elt);
		if (nodeNbr < metaEid.nodeNbr)
		{
			continue;
		}

		break;
	}

	/*	Either found this subscriber or reached end of list.	*/

	if (elt == 0)
	{
		/*	Nothing to do: not a current member.		*/

		sdr_exit_xn(sdr);
		return 0;
	}

	sdr_list_delete(sdr, elt, NULL, NULL);

	/*	If no relatives (including self) are members of this
	 *	group any longer, then must now unsubscribe from this
	 *	group altogether.					*/

	if (sdr_list_length(sdr, group->members) == 0 && group->endpoints == 0)
	{
		if (forwardPetition(group, 0, metaEid.nodeNbr) < 0)
		{
			sdr_cancel_xn(sdr);
		}
		else
		{
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

int	imcJoin(unsigned long groupNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned long	ownNodeNbr = getOwnNodeNbr();
	Object		groupElt;
	Object		nextGroup;
	Object		groupAddr;
	ImcGroup	group;

	sdr_begin_xn(sdr);
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
	group.endpoints++;
	sdr_write(sdr, groupAddr, (char *) &group, sizeof(ImcGroup));

	/*	If this is the first endpoint in this group that the
	 *	local node has registered in, and no relative is
	 *	already a member of this group, then must now subscribe.*/

	if (sdr_list_length(sdr, group.members) == 0 && group.endpoints == 1)
	{
		if (forwardPetition(&group, 1, ownNodeNbr) < 0)
		{
			sdr_cancel_xn(sdr);
		}
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Failed joining IMC group.", NULL);
		return -1;
	}

	return 0;
}

int	imcLeave(unsigned long groupNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned long	ownNodeNbr = getOwnNodeNbr();
	Object		groupElt;
	Object		nextGroup;
	Object		groupAddr;
	ImcGroup	group;

	sdr_begin_xn(sdr);
	groupElt = locateGroup(groupNbr, &nextGroup);
	if (groupElt == 0)
	{
		/*	Nonexistent group, so nothing to do.		*/

		sdr_exit_xn(sdr);
		return 0;
	}

	groupAddr = sdr_list_data(sdr, groupElt);
	sdr_stage(sdr, (char *) &group, groupAddr, sizeof(ImcGroup));
	group.endpoints--;
	if (group.endpoints < 0)	/*	Defensive coding.	*/
	{
		group.endpoints = 0;
	}

	/*	If the local node is no longer registered in any
	 *	endpoint of this group, and no relatives are members
	 *	of this group any longer, then must now unsubscribe
	 *	from this group altogether.				*/

	if (group.endpoints == 0 && sdr_list_length(sdr, group.members) == 0)
	{
		if (forwardPetition(&group, 0, ownNodeNbr) < 0)
		{
			sdr_cancel_xn(sdr);
		}
		else
		{
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
