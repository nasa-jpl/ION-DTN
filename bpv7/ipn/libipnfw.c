/*
 *	libipnfw.c:	functions enabling the implementation of
 *			a regional forwarder for the IPN endpoint
 *			ID scheme.
 *
 *	Copyright (c) 2006, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	02-06-06  SCB	Original development.
 */

#include "ipnfw.h"

#define	IPN_DBNAME	"ipnRoute"

/*	*	*	Globals used for IPN scheme service.	*	*/

static SdrObject _ipndbObject(SdrObject *newDbObj)
{
	static SdrObject obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static IpnDB	*_ipnConstants(void)
{
	static IpnDB	buf;
	static IpnDB	*db = NULL;
	Sdr		sdr;
	SdrObject	dbObject;

	if (db == NULL)
	{
		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _ipndbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(IpnDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(IpnDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}

	return db;
}

/*	*	*	Routing information mgt functions	*	*/

int	ipnInit(void)
{
	Sdr	sdr = getIonsdr();
	SdrObject ipndbObject;
	IpnDB	ipndbBuf;

	/*	Recover the IPN database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	ipndbObject = sdr_find(sdr, IPN_DBNAME, NULL);
	switch (ipndbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Failed seeking IPN database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		ipndbObject = sdr_malloc(sdr, sizeof(IpnDB));
		if (ipndbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for IPN database.", NULL);
			return -1;
		}

		memset((char *) &ipndbBuf, 0, sizeof(IpnDB));
		ipndbBuf.exits = sdr_list_create(sdr);
		ipndbBuf.overrides = sdr_list_create(sdr);
		sdr_write(sdr, ipndbObject, (char *) &ipndbBuf, sizeof(IpnDB));
		sdr_catlg(sdr, IPN_DBNAME, 0, ipndbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create IPN database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(sdr);
	}

	oK(_ipndbObject(&ipndbObject));
	oK(_ipnConstants());
	return 0;
}

SdrObject getIpnDbObject(void)
{
	return _ipndbObject(NULL);
}

IpnDB	*getIpnConstants(void)
{
	return _ipnConstants();
}

void ipn_findPlan(uvast fqnn, SdrObject *planAddr, SdrObject *eltp)
{
	Sdr		sdr = getIonsdr();
	char		nbrBuf[FQN_MAX_LENGTH];
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;

	/*	This function finds the BpPlan for the specified
	 *	node, if any.						*/

	CHKVOID(ionLocked());
	CHKVOID(planAddr && eltp);
	*eltp = 0;			/*	Default.		*/
	if (fqnn == 0)
	{
		return;
	}

	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return;
	}

	*planAddr = sdr_list_data(sdr, vplan->planElt);
	*eltp = vplan->planElt;
}

int	ipn_addPlan(uvast fqnn, unsigned int nominalRate)
{
	char	nbrBuf[FQN_MAX_LENGTH];
	char	eid[MAX_EID_LEN + 1];
	int	result;

	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	result = addPlan(eid, nominalRate);
	if (result == 1)
	{
		result = bpStartPlan(eid);
	}

	return result;
}

int	ipn_addPlanDuct(uvast fqnn, char *ductExpression)
{
	char		nbrBuf[FQN_MAX_LENGTH];
	char		eid[MAX_EID_LEN + 1];
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	cursor = strchr(ductExpression, '/');
	if (cursor == NULL)
	{
		writeMemoNote("[?] Duct expression lacks duct name",
				ductExpression);
		writeMemoNote("[?]   Attaching duct to plan", eid);
		return -1;
	}

	*cursor = '\0';
	findOutduct(ductExpression, cursor + 1, &vduct, &vductElt);
	*cursor = '/';
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown duct", ductExpression);
		writeMemoNote("[?]   Attaching duct to plan", eid);
		return -1;
	}

	return attachPlanDuct(eid, vduct->outductElt);
}

int	ipn_updatePlan(uvast fqnn, unsigned int nominalRate)
{
	char	nbrBuf[FQN_MAX_LENGTH];
	char	eid[MAX_EID_LEN + 1];

	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	return updatePlan(eid, nominalRate);
}

int	ipn_removePlanDuct(uvast fqnn, char *ductExpression)
{
	char		nbrBuf[FQN_MAX_LENGTH];
	char		eid[MAX_EID_LEN + 1];
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	CHKERR(ductExpression);
	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	cursor = strchr(ductExpression, '/');
	if (cursor == NULL)
	{
		writeMemoNote("[?] Duct expression lacks duct name",
				ductExpression);
		writeMemoNote("[?] (Detaching duct from plan", eid);
		return -1;
	}

	*cursor = '\0';
	findOutduct(ductExpression, cursor + 1, &vduct, &vductElt);
	*cursor = '/';
	if (vductElt == 0)
	{
		writeMemoNote("[?] Unknown duct", ductExpression);
		writeMemoNote("[?] (Detaching duct from plan", eid);
		return -1;
	}

	return detachPlanDuct(vduct->outductElt);
}

int	ipn_removePlan(uvast fqnn)
{
	char	nbrBuf[FQN_MAX_LENGTH];
	char	eid[MAX_EID_LEN + 1];

	putFqn(nbrBuf, fqnn);
	isprintf(eid, sizeof eid, "ipn:%s.0", nbrBuf);
	return removePlan(eid);
}

static SdrObject locateOvrd(unsigned int dataLabel, uvast destFqnn,
		uvast sourceFqnn, SdrObject *nextOvrd)
{
	Sdr		sdr = getIonsdr();
	SdrObject	elt;
	SdrObject	ovrdAddr;
	IpnOverride	ovrd;

	/*	This function locates the IpnOverride for the
	 *	specified data label, destination node number, and
	 *	source node number, if any; if none, notes the
	 *	location within the overrides list at which such an
	 *	override should be inserted.				*/

	if (nextOvrd) *nextOvrd = 0;	/*	Default.		*/
	for (elt = sdr_list_first(sdr, (_ipnConstants())->overrides); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ovrdAddr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &ovrd, ovrdAddr, sizeof(IpnOverride));
		if (ovrd.dataLabel < dataLabel)
		{
			continue;
		}

		if (ovrd.dataLabel > dataLabel)
		{
			if (nextOvrd) *nextOvrd = elt;
			break;		/*	Same as end of list.	*/
		}

		if (ovrd.destFqnn < destFqnn)
		{
			continue;
		}

		if (ovrd.destFqnn > destFqnn)
		{
			if (nextOvrd) *nextOvrd = elt;
			break;		/*	Same as end of list.	*/
		}

		if (ovrd.sourceFqnn < sourceFqnn)
		{
			continue;
		}

		if (ovrd.sourceFqnn > sourceFqnn)
		{
			if (nextOvrd) *nextOvrd = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched all parameters of override.		*/

		return elt;
	}

	return 0;
}

int	ipn_setOvrd(unsigned int dataLabel, uvast destFqnn,
		uvast sourceFqnn, uvast neighborFqnn,
		char *ovrdDuctExpression, unsigned char priority,
		unsigned char ordinal, unsigned char qosFlags)
{
	Sdr		sdr = getIonsdr();
	SdrObject	elt;
	SdrObject	nextElt;
	IpnOverride	ovrd;
	SdrObject	addr;

	if (dataLabel == 0)
	{
		writeMemo("[?] Data label for override is 0.");
		return 0;
	}

	if (destFqnn == 0)
	{
		writeMemo("[?] Destination node for override is 0.");
		return 0;
	}

	if (sourceFqnn == 0)
	{
		writeMemo("[?] Source node for override is 0.");
		return 0;
	}

	if (priority < (unsigned char) -2 && priority > 2)
	{
		writeMemoNote("[?] Invalid override priority", utoa(priority));
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locateOvrd(dataLabel, destFqnn, sourceFqnn, &nextElt);
	if (elt == 0)
	{
		/*	Override doesn't exist, so add it.		*/

		memset((char *) &ovrd, 0, sizeof(IpnOverride));
		ovrd.dataLabel = dataLabel;
		ovrd.destFqnn = destFqnn;
		ovrd.sourceFqnn = sourceFqnn;
		ovrd.neighborFqnn = (uvast) -1;
		ovrd.priority = (unsigned char) -1;
		addr = sdr_malloc(sdr, sizeof(IpnOverride));
		if (addr == 0)
		{
			putErrmsg("Can't add new ipn override.", NULL);
			return -1;
		}

		sdr_write(sdr, addr, (char *) &ovrd, sizeof(IpnOverride));
		if (nextElt)
		{
			elt = sdr_list_insert_before(sdr, nextElt, addr);
		}
		else
		{
			elt = sdr_list_insert_last(sdr,
					(_ipnConstants())->overrides, addr);
		}

		if (elt == 0)
		{
			putErrmsg("Can't add new ipn override.", NULL);
			return -1;
		}
	}

	addr = (SdrObject) sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &ovrd, addr, sizeof(IpnOverride));
	if (neighborFqnn != (uvast) -2)
	{
		ovrd.neighborFqnn = neighborFqnn;
		if (ovrdDuctExpression)
		{
			if (ovrd.ductExpression)
			{
				sdr_free(sdr, ovrd.ductExpression);
				ovrd.ductExpression = 0;
			}

			/*	Expression "" just erases.		*/

			if (strlen(ovrdDuctExpression) > 0)
			{
				ovrd.ductExpression = sdr_string_create(sdr,
						ovrdDuctExpression);
			}
		}
	}

	if (priority != (unsigned char) -2)
	{
		ovrd.priority = priority;
		ovrd.ordinal = ordinal;
		ovrd.qosFlags = qosFlags;
	}

	if (ovrd.neighborFqnn == (uvast) -1
	&& ovrd.priority == (unsigned char) -1)
	{
		/*	Override is moot, so delete it.		*/

		if (ovrd.ductExpression)
		{
			sdr_free(sdr, ovrd.ductExpression);
		}

		sdr_list_delete(sdr, elt, NULL, NULL);
		sdr_free(sdr, addr);
	}
	else
	{
		sdr_write(sdr, addr, (char *) &ovrd,
				sizeof(IpnOverride));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't set override.", NULL);
		return -1;
	}

	return 0;
}

int	ipn_lookupOvrd(unsigned int dataLabel, uvast destFqnn,
		uvast sourceFqnn, SdrObject *addr)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
		OBJ_POINTER(IpnOverride, ovrd);

	/*	This function determines the applicable egress plan
	 *	for the specified eid, if any.				*/

	CHKERR(ionLocked());

	/*	Find best matching override.  Overrides are sorted by
	 *	source node number within destination node number
	 *	within data label, all ascending; node number -1
	 *	sorts last and indicates "all others".			*/

	for (elt = sdr_list_first(sdr, (_ipnConstants())->overrides); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, IpnOverride, ovrd, *addr);
		if (ovrd->dataLabel < dataLabel)
		{
			continue;
		}

		if (ovrd->dataLabel != dataLabel
		&& ovrd->dataLabel != (unsigned int) -1)
		{
			continue;
		}

		/*	Data label matches.				*/

		if (ovrd->destFqnn < destFqnn)
		{
			continue;
		}

		if (ovrd->destFqnn != destFqnn
		&& ovrd->destFqnn != (uvast) -1)
		{
			continue;
		}

		/*	Destination node number matches.		*/

		if (ovrd->sourceFqnn < sourceFqnn)
		{
			continue;
		}

		if (ovrd->sourceFqnn != sourceFqnn
		&& ovrd->sourceFqnn != (uvast) -1)
		{
			continue;
		}

		/*	Source node number matches.			*/

		break;
	}

	if (elt == 0)
	{
		return 0;		/*	No matching override.	*/
	}

	return 1;
}

static SdrObject locateExit(uvast firstFqnn, uvast lastFqnn, SdrObject *nextExit)
{
	Sdr	sdr = getIonsdr();
	int	targetSize;
	int	exitSize;
	SdrObject elt;
		OBJ_POINTER(IpnExit, exit);

	/*	This function locates the IpnExit for the specified
	 *	first node number, if any; if none, notes the
	 *	location within the rules list at which such a rule
	 *	should be inserted.					*/

	if (nextExit) *nextExit = 0;	/*	Default.		*/
	targetSize = lastFqnn - firstFqnn;
	for (elt = sdr_list_first(sdr, (_ipnConstants())->exits); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
		exitSize = exit->lastFqnn - exit->firstFqnn;
		if (exitSize < targetSize)
		{
			continue;
		}

		if (exitSize > targetSize)
		{
			if (nextExit) *nextExit = elt;
			break;		/*	Same as end of list.	*/
		}

		if (exit->firstFqnn < firstFqnn)
		{
			continue;
		}

		if (exit->firstFqnn > firstFqnn)
		{
			if (nextExit) *nextExit = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched exit's first node number.		*/

		return elt;
	}

	return 0;
}

void ipn_findExit(uvast firstFqnn, uvast lastFqnn, SdrObject *exitAddr,
		SdrObject *eltp)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;

	/*	This function finds the IpnExit for the specified
	 *	node range, if any.					*/

	CHKVOID(ionLocked());
	CHKVOID(exitAddr && eltp);
	if (firstFqnn == 0)
	{
		writeMemo("[?] First node for exit is 0.");
		return;
	}

	if (firstFqnn > lastFqnn)
	{
		writeMemo("[?] First node for exit greater than last.");
		return;
	}

	*eltp = 0;
	elt = locateExit(firstFqnn, lastFqnn, NULL);
	if (elt == 0)
	{
		return;
	}

	*exitAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	ipn_addExit(uvast firstFqnn, uvast lastFqnn, char *viaEid)
{
	Sdr	sdr = getIonsdr();
	SdrObject nextExit;
	IpnExit	exit;
	SdrObject addr;

	CHKERR(viaEid);
	if (firstFqnn == 0)
	{
		writeMemo("[?] First node for exit is 0.");
		return 0;
	}

	if (firstFqnn > lastFqnn)
	{
		writeMemo("[?] First node for exit greater than last.");
		return 0;
	}

	if (strlen(viaEid) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Exit's gateway EID is too long", viaEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	if (locateExit(firstFqnn, lastFqnn, &nextExit) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate exit", uvasttoa(firstFqnn));
		return 0;
	}

	/*	All parameters validated, okay to add the exit.	*/

	memset((char *) &exit, 0, sizeof(IpnExit));
	exit.firstFqnn = firstFqnn;
	exit.lastFqnn = lastFqnn;
	exit.eid = sdr_string_create(sdr, viaEid);
	addr = sdr_malloc(sdr, sizeof(IpnExit));
	if (addr)
	{
		if (nextExit)
		{
			sdr_list_insert_before(sdr, nextExit, addr);
		}
		else
		{
			sdr_list_insert_last(sdr, (_ipnConstants())->exits,
					addr);
		}

		sdr_write(sdr, addr, (char *) &exit, sizeof(IpnExit));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add exit.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_updateExit(uvast firstFqnn, uvast lastFqnn, char *viaEid)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject addr;
	IpnExit	exit;

	CHKERR(viaEid);
	if (strlen(viaEid) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Exit's gateway EID is too long", viaEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstFqnn, lastFqnn, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown exit", uvasttoa(firstFqnn));
		return 0;
	}

	/*	All parameters validated, okay to update the exit.	*/

	addr = (SdrObject) sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &exit, addr, sizeof(IpnExit));
	sdr_free(sdr, exit.eid);
	exit.eid = sdr_string_create(sdr, viaEid);
	sdr_write(sdr, addr, (char *) &exit, sizeof(IpnExit));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update exit.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_removeExit(uvast firstFqnn, uvast lastFqnn)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject addr;
		OBJ_POINTER(IpnExit, exit);

	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstFqnn, lastFqnn, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown exit", uvasttoa(firstFqnn));
		return 0;
	}

	addr = (SdrObject) sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, IpnExit, exit, addr);

	/*	All parameters validated, okay to remove the exit.	*/

	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, exit->eid);
	sdr_free(sdr, addr);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove exit.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_lookupExit(uvast fqnn, char *eid)
{
	Sdr	sdr = getIonsdr();
	SdrObject elt;
	SdrObject addr;
	IpnExit	exit;

	/*	This function determines the applicable egress plan
	 *	for the specified eid, if any.				*/

	CHKERR(ionLocked());
	CHKERR(eid);
	if (fqnn == 0)
	{
		writeMemo("[?] Node for exit is 0.");
		return 0;
	}

	/*	Find best matching exit.  Exits are sorted by first
	 *	node number within exit size, both ascending.  So
	 *	the first exit whose range encompasses the node number
	 *	is the best fit (narrowest applicable range), but
	 *	there's no way to terminate the search early.		*/

	for (elt = sdr_list_first(sdr, (_ipnConstants())->exits); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &exit, addr, sizeof(IpnExit));
		if (exit.lastFqnn < fqnn || exit.firstFqnn > fqnn)
		{
			continue;
		}

		break;
	}

	if (elt == 0)
	{
		return 0;		/*	No exit found.		*/
	}

	sdr_string_read(sdr, eid, exit.eid);
	return 1;
}

/*
 * Bulk removal function for runtime reconfiguration.
 *
 * This function collects plan identifiers first, then removes each plan.
 * This avoids iterator invalidation when modifying the list.
 */

#define MAX_BULK_REMOVE_PLANS	256

int	ipn_remove_all_plans(void)
{
	Sdr		sdr = getIonsdr();
	PsmPartition	bpwm = getIonwm();
	BpVdb		*vdb = getBpVdb();
	PsmAddress	elt;
	VPlan		*vplan;
	uvast		fqnns[MAX_BULK_REMOVE_PLANS];
	int		count = 0;
	int		removed = 0;
	int		i;
	int		result;

	if (vdb == NULL)
	{
		writeMemo("[?] ipn_remove_all_plans: BP not initialized.");
		return -1;
	}

	/*	First, collect all IPN plan FQNNs to avoid iterator
	 *	invalidation when removing items.			*/

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sm_list_first(bpwm, vdb->plans); elt;
			elt = sm_list_next(bpwm, elt))
	{
		vplan = (VPlan *) psp(bpwm, sm_list_data(bpwm, elt));
		if (vplan == NULL || count >= MAX_BULK_REMOVE_PLANS)
		{
			continue;
		}

		/*	Only process IPN plans (those with neighborFqnn
		 *	set, i.e., neighborEid starts with "ipn:").	*/

		if (vplan->neighborFqnn == 0)
		{
			continue;	/*	Not an IPN plan.	*/
		}

		fqnns[count++] = vplan->neighborFqnn;
	}

	sdr_exit_xn(sdr);

	/*	Now remove each IPN plan.				*/

	for (i = 0; i < count; i++)
	{
		result = ipn_removePlan(fqnns[i]);
		if (result == 1)
		{
			removed++;
		}
		else if (result < 0)
		{
			writeMemoNote("[?] ipn_remove_all_plans: Error removing \
plan", utoa(fqnns[i]));
		}
		/*	result == 0 means plan not found or has pending
		 *	bundles; continue with others.			*/
	}

	return removed;
}
