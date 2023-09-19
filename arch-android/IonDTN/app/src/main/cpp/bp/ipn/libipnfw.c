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

static Object	_ipndbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static IpnDB	*_ipnConstants()
{
	static IpnDB	buf;
	static IpnDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;

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

int	ipnInit()
{
	Sdr	sdr = getIonsdr();
	Object	ipndbObject;
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

Object	getIpnDbObject()
{
	return _ipndbObject(NULL);
}

IpnDB	*getIpnConstants()
{
	return _ipnConstants();
}

void	ipn_findPlan(uvast nodeNbr, Object *planAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	char		eid[MAX_EID_LEN + 1];
	VPlan		*vplan;
	PsmAddress	vplanElt;

	/*	This function finds the BpPlan for the specified
	 *	node, if any.						*/

	CHKVOID(ionLocked());
	CHKVOID(planAddr && eltp);
	*eltp = 0;			/*	Default.		*/
	if (nodeNbr == 0)
	{
		return;
	}

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	findPlan(eid, &vplan, &vplanElt);
	if (vplanElt == 0)
	{
		return;
	}

	*planAddr = sdr_list_data(sdr, vplan->planElt);
	*eltp = vplan->planElt;
}

int	ipn_addPlan(uvast nodeNbr, unsigned int nominalRate)
{
	char	eid[MAX_EID_LEN + 1];
	int	result;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	result = addPlan(eid, nominalRate);
	if (result == 1)
	{
		result = bpStartPlan(eid);
	}

	return result;
}

int	ipn_addPlanDuct(uvast nodeNbr, char *ductExpression)
{
	char		eid[MAX_EID_LEN + 1];
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
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

int	ipn_updatePlan(uvast nodeNbr, unsigned int nominalRate)
{
	char	eid[MAX_EID_LEN + 1];

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	return updatePlan(eid, nominalRate);
}

int	ipn_removePlanDuct(uvast nodeNbr, char *ductExpression)
{
	char		eid[MAX_EID_LEN + 1];
	char		*cursor;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
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

	return detachPlanDuct(eid, vduct->outductElt);
}

int	ipn_removePlan(uvast nodeNbr)
{
	char	eid[MAX_EID_LEN + 1];

	isprintf(eid, sizeof eid, "ipn:" UVAST_FIELDSPEC ".0", nodeNbr);
	return removePlan(eid);
}

static Object	locateExit(uvast firstNodeNbr, uvast lastNodeNbr,
			Object *nextExit)
{
	Sdr	sdr = getIonsdr();
	int	targetSize;
	int	exitSize;
	Object	elt;
		OBJ_POINTER(IpnExit, exit);

	/*	This function locates the IpnExit for the specified
	 *	first node number, if any; if none, notes the
	 *	location within the rules list at which such a rule
	 *	should be inserted.					*/

	if (nextExit) *nextExit = 0;	/*	Default.		*/
	targetSize = lastNodeNbr - firstNodeNbr;
	for (elt = sdr_list_first(sdr, (_ipnConstants())->exits); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
		exitSize = exit->lastNodeNbr - exit->firstNodeNbr;
		if (exitSize < targetSize)
		{
			continue;
		}

		if (exitSize > targetSize)
		{
			if (nextExit) *nextExit = elt;
			break;		/*	Same as end of list.	*/
		}

		if (exit->firstNodeNbr < firstNodeNbr)
		{
			continue;
		}

		if (exit->firstNodeNbr > firstNodeNbr)
		{
			if (nextExit) *nextExit = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched exit's first node number.		*/

		return elt;
	}

	return 0;
}

void	ipn_findExit(uvast firstNodeNbr, uvast lastNodeNbr, Object *exitAddr,
		Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	/*	This function finds the IpnExit for the specified
	 *	node range, if any.					*/

	CHKVOID(ionLocked());
	CHKVOID(exitAddr && eltp);
	if (firstNodeNbr == 0)
	{
		writeMemo("[?] First node number for exit is 0.");
		return;
	}

	if (firstNodeNbr > lastNodeNbr)
	{
		writeMemo("[?] First node number for exit greater than last.");
		return;
	}

	*eltp = 0;
	elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*exitAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	ipn_addExit(uvast firstNodeNbr, uvast lastNodeNbr, char *viaEid)
{
	Sdr	sdr = getIonsdr();
	Object	nextExit;
	IpnExit	exit;
	Object	addr;

	CHKERR(viaEid);
	if (firstNodeNbr == 0)
	{
		writeMemo("[?] First node number for exit is 0.");
		return 0;
	}

	if (firstNodeNbr > lastNodeNbr)
	{
		writeMemo("[?] First node number for exit greater than last.");
		return 0;
	}

	if (strlen(viaEid) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Exit's gateway EID is too long",
				viaEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	if (locateExit(firstNodeNbr, lastNodeNbr, &nextExit) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate exit", utoa(firstNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to add the exit.	*/

	memset((char *) &exit, 0, sizeof(IpnExit));
	exit.firstNodeNbr = firstNodeNbr;
	exit.lastNodeNbr = lastNodeNbr;
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

int	ipn_updateExit(uvast firstNodeNbr, uvast lastNodeNbr, char *viaEid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
	IpnExit	exit;

	CHKERR(viaEid);
	if (firstNodeNbr == 0)
	{
		writeMemo("[?] First node number for exit is 0.");
		return 0;
	}

	if (firstNodeNbr > lastNodeNbr)
	{
		writeMemo("[?] First node number for exit greater than last.");
		return 0;
	}

	if (strlen(viaEid) > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Exit's gateway EID is too long",
				viaEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown exit", utoa(firstNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to update the exit.	*/

	addr = (Object) sdr_list_data(sdr, elt);
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

int	ipn_removeExit(uvast firstNodeNbr, uvast lastNodeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(IpnExit, exit);

	if (firstNodeNbr == 0)
	{
		writeMemo("[?] First node number for exit is 0.");
		return 0;
	}

	if (firstNodeNbr > lastNodeNbr)
	{
		writeMemo("[?] First node number for exit greater than last.");
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown exit", utoa(firstNodeNbr));
		return 0;
	}

	addr = (Object) sdr_list_data(sdr, elt);
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

int	ipn_lookupExit(uvast nodeNbr, char *eid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
	IpnExit	exit;

	/*	This function determines the applicable egress plan
	 *	for the specified eid, if any.				*/

	CHKERR(ionLocked());
	CHKERR(eid);
	if (nodeNbr == 0)
	{
		writeMemo("[?] Node number for exit is 0.");
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
		if (exit.lastNodeNbr < nodeNbr || exit.firstNodeNbr > nodeNbr)
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
