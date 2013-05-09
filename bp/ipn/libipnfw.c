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

static int	lookupIpnEid(char *uriBuffer, char *neighborClId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);

	for (elt = sdr_list_first(sdr, (_ipnConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
		switch (clIdMatches(neighborClId, &plan->defaultDirective))
		{
		case -1:
			putErrmsg("Failed looking up IPN EID.", NULL);
			return -1;

		case 0:
			continue;	/*	No match.		*/

		default:

			/*	Found the plan for transmission to
			 *	this neighbor, so now we know the
			 *	neighbor's EID.				*/

			isprintf(uriBuffer, SDRSTRING_BUFSZ,
				"ipn:" UVAST_FIELDSPEC ".0", plan->nodeNbr);
			return 1;
		}
	}

	return 0;
}

int	ipnInit()
{
	Sdr	sdr = getIonsdr();
	Object	ipndbObject;
	IpnDB	ipndbBuf;

	/*	Recover the IPN database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	oK(senderEidLookupFunctions(lookupIpnEid));
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
		ipndbBuf.plans = sdr_list_create(sdr);
		ipndbBuf.groups = sdr_list_create(sdr);
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

static Object	locatePlan(uvast nodeNbr, Object *nextPlan)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);

	/*	This function locates the IpnPlan identified by the
	 *	specified node number, if any.  If none, notes the
	 *	location within the plans list at which such a plan
	 *	should be inserted.					*/

	if (nextPlan) *nextPlan = 0;	/*	Default.		*/
	for (elt = sdr_list_first(sdr, (_ipnConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
		if (plan->nodeNbr < nodeNbr)
		{
			continue;
		}
		
		if (plan->nodeNbr > nodeNbr)
		{
			if (nextPlan) *nextPlan = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	ipn_findPlan(uvast nodeNbr, Object *planAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	/*	This function finds the IpnPlan for the specified
	 *	node, if any.						*/

	CHKVOID(ionLocked());
	CHKVOID(nodeNbr && planAddr && eltp);
	*eltp = 0;
	elt = locatePlan(nodeNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*planAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

static void	createXmitDirective(FwdDirective *directive,
			DuctExpression *parms)
{
	directive->action = xmit;
	directive->outductElt = parms->outductElt;
	if (parms->destDuctName == NULL)
	{
		directive->destDuctName = 0;
	}
	else
	{
		directive->destDuctName = sdr_string_create(getIonsdr(),
				parms->destDuctName);
	}
}

int	ipn_addPlan(uvast nodeNbr, DuctExpression *defaultDuct)
{
	Sdr	sdr = getIonsdr();
	Object	nextPlan;
	IpnPlan	plan;
	Object	planObj;

	CHKERR(nodeNbr && defaultDuct);
	CHKERR(sdr_begin_xn(sdr));
	if (locatePlan(nodeNbr, &nextPlan) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate plan", utoa(nodeNbr));
		return 0;
	}

	/*	Okay to add this plan to the database.			*/

	plan.nodeNbr = nodeNbr;
	createXmitDirective(&(plan.defaultDirective), defaultDuct);
	plan.rules = sdr_list_create(sdr);
	planObj = sdr_malloc(sdr, sizeof(IpnPlan));
	if (planObj)
	{
		if (nextPlan)
		{
			oK(sdr_list_insert_before(sdr, nextPlan, planObj));
		}
		else
		{
			oK(sdr_list_insert_last(sdr,
					(_ipnConstants())->plans, planObj));
		}

		sdr_write(sdr, planObj, (char *) &plan, sizeof(IpnPlan));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add plan.", NULL);
		return -1;
	}

	return 1;
}

static void	destroyXmitDirective(FwdDirective *directive)
{
	if (directive->destDuctName)
	{
		sdr_free(getIonsdr(), directive->destDuctName);
	}
}

int	ipn_updatePlan(uvast nodeNbr, DuctExpression *defaultDuct)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	planObj;
	IpnPlan	plan;

	CHKERR(nodeNbr && defaultDuct);
	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] This plan is not defined.", utoa(nodeNbr));
		return 0;
	}

	/*	Okay to update this plan.				*/

	planObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &plan, planObj, sizeof(IpnPlan));
	destroyXmitDirective(&(plan.defaultDirective));
	createXmitDirective(&(plan.defaultDirective), defaultDuct);
	sdr_write(sdr, planObj, (char *) &plan, sizeof(IpnPlan));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update plan.", utoa(nodeNbr));
		return -1;
	}

	return 1;
}

int	ipn_removePlan(uvast nodeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	planObj;
		OBJ_POINTER(IpnPlan, plan);

	CHKERR(nodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown plan", utoa(nodeNbr));
		return 0;
	}

	planObj = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, IpnPlan, plan, planObj);
	if (sdr_list_length(sdr, plan->rules) > 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Can't remove plan; still has rules",
				utoa(nodeNbr));
		return 0;
	}

	/*	Okay to remove this plan from the database.		*/

	sdr_list_delete(sdr, elt, NULL, NULL);
	destroyXmitDirective(&(plan->defaultDirective));
	sdr_list_destroy(sdr, plan->rules, NULL, NULL);
	sdr_free(sdr, planObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove plan.", utoa(nodeNbr));
		return -1;
	}

	return 1;
}

static Object	locateRule(Object rules, unsigned int srcServiceNbr,
			uvast srcNodeNbr, Object *nextRule)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnRule, rule);

	/*	This function locates the IpnRule for the specified
	 *	service number and source node number, for the
	 *	specified destination node number, if any; if
	 *	none, notes the location within the rules list at
	 *	which such a rule should be inserted.			*/

	if (nextRule) *nextRule = 0;	/*	Default.		*/
	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnRule, rule, sdr_list_data(sdr, elt));
		if (rule->srcServiceNbr < srcServiceNbr)
		{
			continue;
		}

		if (rule->srcServiceNbr > srcServiceNbr)
		{
			if (nextRule) *nextRule = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched rule's service number.			*/

		if (rule->srcNodeNbr < srcNodeNbr)
		{
			continue;
		}

		if (rule->srcNodeNbr > srcNodeNbr)
		{
			if (nextRule) *nextRule = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Exact match.					*/

		return elt;
	}

	return 0;
}

void	ipn_findPlanRule(uvast nodeNbr, int argServiceNbr, vast argNodeNbr,
		IpnPlan *plan, Object *ruleAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnPlan, planPtr);

	/*	This function finds the IpnRule for the specified
	 *	service number and source node number, for the
	 *	specified destination node number, if any.		*/

	CHKVOID(ionLocked());
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;
	if (plan == NULL)
	{
		if (nodeNbr == 0)
		{
			return;
		}

		elt = locatePlan(nodeNbr, NULL);
		if (elt == 0)
		{
			return;
		}

		GET_OBJ_POINTER(sdr, IpnPlan, planPtr, sdr_list_data(sdr, elt));
		plan = planPtr;
	}

	elt = locateRule(plan->rules, srcServiceNbr, srcNodeNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*ruleAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	ipn_addPlanRule(uvast nodeNbr, int argServiceNbr, vast argNodeNbr,
		DuctExpression *directive)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnPlan, plan);
	Object		nextRule;
	IpnRule		ruleBuf;
	Object		addr;

	CHKERR(nodeNbr && srcNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node",
				utoa(nodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
	if (locateRule(plan->rules, srcServiceNbr, srcNodeNbr, &nextRule) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to add the rule.		*/

	memset((char *) &ruleBuf, 0, sizeof(IpnRule));
	ruleBuf.srcServiceNbr = srcServiceNbr;
	ruleBuf.srcNodeNbr = srcNodeNbr;
	createXmitDirective(&ruleBuf.directive, directive);
	addr = sdr_malloc(sdr, sizeof(IpnRule));
	if (addr)
	{
		if (nextRule)
		{
			elt = sdr_list_insert_before(sdr, nextRule, addr);
		}
		else
		{
			elt = sdr_list_insert_last(sdr, plan->rules, addr);
		}

		sdr_write(sdr, addr, (char *) &ruleBuf, sizeof(IpnRule));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_updatePlanRule(uvast nodeNbr, int argServiceNbr,
		vast argNodeNbr, DuctExpression *directive)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnPlan, plan);
	Object		ruleAddr;
	IpnRule		ruleBuf;

	CHKERR(nodeNbr && srcNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node",
				utoa(nodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
	ipn_findPlanRule(nodeNbr, srcServiceNbr, srcNodeNbr, plan, &ruleAddr,
			&elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to update the rule.	*/

	sdr_stage(sdr, (char *) &ruleBuf, ruleAddr, sizeof(IpnRule));
	destroyXmitDirective(&ruleBuf.directive);
	createXmitDirective(&ruleBuf.directive, directive);
	sdr_write(sdr, ruleAddr, (char *) &ruleBuf, sizeof(IpnRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_removePlanRule(uvast nodeNbr, int argServiceNbr, vast argNodeNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnPlan, plan);
	Object		ruleAddr;
			OBJ_POINTER(IpnRule, rule);

	CHKERR(nodeNbr && srcNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node",
				utoa(nodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
	ipn_findPlanRule(nodeNbr, srcServiceNbr, srcNodeNbr, plan, &ruleAddr,
			&elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to remove the rule.	*/

	GET_OBJ_POINTER(sdr, IpnRule, rule, ruleAddr);
	destroyXmitDirective(&rule->directive);
	sdr_free(sdr, ruleAddr);
	sdr_list_delete(sdr, elt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}

static int	lookupRule(Object rules, unsigned int sourceServiceNbr,
			uvast sourceNodeNbr, FwdDirective *dirbuf)
{
	Sdr	sdr = getIonsdr();
	Object	addr;
	Object	elt;
		OBJ_POINTER(IpnRule, rule);

	/*	Universal wild-card match (IPN_ALL_OTHER_xxx), if any,
	 *	is at the end of the list, so there's no way to
	 *	terminate the search early.				*/

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, IpnRule, rule, addr);
		if (rule->srcServiceNbr < sourceServiceNbr)
		{
			continue;
		}

		if (rule->srcServiceNbr > sourceServiceNbr
			&& rule->srcServiceNbr != IPN_ALL_OTHER_SERVICES)
		{
			continue;
		}

		/*	Found a rule with matching service number.	*/

		if (rule->srcNodeNbr < sourceNodeNbr)
		{
			continue;
		}

		if (rule->srcNodeNbr > sourceNodeNbr
			&& rule->srcNodeNbr != IPN_ALL_OTHER_NODES)
		{
			continue;
		}

		break;			/*	Stop searching.		*/
	}

	if (elt)			/*	Found a matching rule.	*/
	{
		memcpy((char *) dirbuf, (char *) &rule->directive,
				sizeof(FwdDirective));
		return 1;
	}

	return 0;
}

int	ipn_lookupPlanDirective(uvast nodeNbr, unsigned int sourceServiceNbr,
		uvast sourceNodeNbr, FwdDirective *dirbuf)
{
	Sdr	sdr = getIonsdr();
	Object	addr;
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);

	/*	This function determines the relevant FwdDirective for
	 *	the specified eid, if any.  Wild card match is okay.	*/

	CHKERR(ionLocked());
	CHKERR(nodeNbr && dirbuf);

	/*	Find the matching plan.					*/

	ipn_findPlan(nodeNbr, &addr, &elt);
	if (elt == 0)
	{
		return 0;		/*	No plan found.		*/
	}

	GET_OBJ_POINTER(sdr, IpnPlan, plan, addr);

	/*	Find best matching rule.				*/

	if (lookupRule(plan->rules, sourceServiceNbr, sourceNodeNbr,
			dirbuf) == 0)	/*	No rule found.		*/
	{
		memcpy((char *) dirbuf, (char *) &plan->defaultDirective,
				sizeof(FwdDirective));
	}

	return 1;
}

static Object	locateGroup(uvast firstNodeNbr, uvast lastNodeNbr,
			Object *nextGroup)
{
	Sdr	sdr = getIonsdr();
	int	targetSize;
	int	groupSize;
	Object	elt;
		OBJ_POINTER(IpnGroup, group);

	/*	This function locates the IpnGroup for the specified
	 *	first node number, if any; if none, notes the
	 *	location within the rules list at which such a rule
	 *	should be inserted.					*/

	if (nextGroup) *nextGroup = 0;	/*	Default.		*/
	targetSize = lastNodeNbr - firstNodeNbr;
	for (elt = sdr_list_first(sdr, (_ipnConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
		groupSize = group->lastNodeNbr - group->firstNodeNbr;
		if (groupSize < targetSize)
		{
			continue;
		}

		if (groupSize > targetSize)
		{
			if (nextGroup) *nextGroup = elt;
			break;		/*	Same as end of list.	*/
		}

		if (group->firstNodeNbr < firstNodeNbr)
		{
			continue;
		}

		if (group->firstNodeNbr > firstNodeNbr)
		{
			if (nextGroup) *nextGroup = elt;
			break;		/*	Same as end of list.	*/
		}

		/*	Matched group's first node number.		*/

		return elt;
	}

	return 0;
}

void	ipn_findGroup(uvast firstNodeNbr, uvast lastNodeNbr, Object *groupAddr,
		Object *eltp)
{
	Sdr	sdr = getIonsdr();
	Object	elt;

	/*	This function finds the IpnGroup for the specified
	 *	node range, if any.					*/

	CHKVOID(ionLocked());
	CHKVOID(firstNodeNbr && groupAddr && eltp);
	CHKVOID(firstNodeNbr <= lastNodeNbr);
	*eltp = 0;
	elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*groupAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	ipn_addGroup(uvast firstNodeNbr, uvast lastNodeNbr, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	Object		nextGroup;
	IpnGroup	group;
	Object		addr;

	CHKERR(firstNodeNbr && lastNodeNbr && viaEid);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(strlen(viaEid) <= MAX_SDRSTRING);
	CHKERR(sdr_begin_xn(sdr));
	if (locateGroup(firstNodeNbr, lastNodeNbr, &nextGroup) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate group", utoa(firstNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to add the group.	*/

	memset((char *) &group, 0, sizeof(IpnGroup));
	group.firstNodeNbr = firstNodeNbr;
	group.lastNodeNbr = lastNodeNbr;
	group.rules = sdr_list_create(sdr);
	group.defaultDirective.action = fwd;
	group.defaultDirective.eid = sdr_string_create(sdr, viaEid);
	addr = sdr_malloc(sdr, sizeof(IpnGroup));
	if (addr)
	{
		if (nextGroup)
		{
			sdr_list_insert_before(sdr, nextGroup, addr);
		}
		else
		{
			sdr_list_insert_last(sdr, (_ipnConstants())->groups,
					addr);
		}

		sdr_write(sdr, addr, (char *) &group, sizeof(IpnGroup));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add group.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_updateGroup(uvast firstNodeNbr, uvast lastNodeNbr, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		addr;
	IpnGroup	group;

	CHKERR(firstNodeNbr && lastNodeNbr && viaEid);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(strlen(viaEid) <= MAX_SDRSTRING);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown group", utoa(firstNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to update the group.	*/

	addr = (Object) sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &group, addr, sizeof(IpnGroup));
	sdr_free(sdr, group.defaultDirective.eid);
	group.defaultDirective.eid = sdr_string_create(sdr, viaEid);
	sdr_write(sdr, addr, (char *) &group, sizeof(IpnGroup));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update group.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_removeGroup(uvast firstNodeNbr, uvast lastNodeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(IpnGroup, group);

	CHKERR(firstNodeNbr && lastNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown group", utoa(firstNodeNbr));
		return 0;
	}

	addr = (Object) sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, IpnGroup, group, addr);
	if (sdr_list_length(sdr, group->rules) > 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Can't remove group; still has rules",
				utoa(firstNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to remove the group.	*/

	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, group->defaultDirective.eid);
	sdr_free(sdr, addr);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove group.", NULL);
		return -1;
	}

	return 1;
}

void	ipn_findGroupRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr, IpnGroup *group,
		Object *ruleAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnGroup, groupPtr);

	/*	This function finds the IpnRule for the specified
	 *	service number and source node number, for the
	 *	specified destination node number, if any.		*/

	CHKVOID(ionLocked());
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;
	if (group == NULL)
	{
		if (firstNodeNbr == 0 || lastNodeNbr < firstNodeNbr)
		{
			return;
		}

		elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
		if (elt == 0)
		{
			return;
		}

		GET_OBJ_POINTER(sdr, IpnGroup, groupPtr,
				sdr_list_data(sdr, elt));
		group = groupPtr;
	}

	elt = locateRule(group->rules, srcServiceNbr, srcNodeNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*ruleAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	ipn_addGroupRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnGroup, group);
	Object		nextRule;
	IpnRule		ruleBuf;
	Object		addr;

	CHKERR(firstNodeNbr && lastNodeNbr && srcNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Group is unknown", utoa(firstNodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
	if (locateRule(group->rules, srcServiceNbr, srcNodeNbr, &nextRule) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to add the rule.		*/

	memset((char *) &ruleBuf, 0, sizeof(IpnRule));
	ruleBuf.srcServiceNbr = srcServiceNbr;
	ruleBuf.srcNodeNbr = srcNodeNbr;
	ruleBuf.directive.eid = sdr_string_create(sdr, viaEid);
	addr = sdr_malloc(sdr, sizeof(IpnRule));
	if (addr)
	{
		if (nextRule)
		{
			elt = sdr_list_insert_before(sdr, nextRule, addr);
		}
		else
		{
			elt = sdr_list_insert_last(sdr, group->rules, addr);
		}

		sdr_write(sdr, addr, (char *) &ruleBuf, sizeof(IpnRule));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_updateGroupRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnGroup, group);
	Object		ruleAddr;
	IpnRule		ruleBuf;

	CHKERR(firstNodeNbr && lastNodeNbr && srcNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Group is unknown", utoa(firstNodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
	ipn_findGroupRule(firstNodeNbr, lastNodeNbr, srcServiceNbr, srcNodeNbr,
			group, &ruleAddr, &elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to update the rule.	*/

	sdr_stage(sdr, (char *) &ruleBuf, ruleAddr, sizeof(IpnRule));
	sdr_free(sdr, ruleBuf.directive.eid);
	ruleBuf.directive.eid = sdr_string_create(sdr, viaEid);
	sdr_write(sdr, ruleAddr, (char *) &ruleBuf, sizeof(IpnRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_removeGroupRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnGroup, group);
	Object		ruleAddr;
			OBJ_POINTER(IpnRule, rule);

	CHKERR(firstNodeNbr && lastNodeNbr && srcNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateGroup(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Group is unknown", utoa(firstNodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
	ipn_findGroupRule(firstNodeNbr, lastNodeNbr, srcServiceNbr, srcNodeNbr,
			group, &ruleAddr, &elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to remove the rule.	*/

	GET_OBJ_POINTER(sdr, IpnRule, rule, ruleAddr);
	sdr_free(sdr, rule->directive.eid);
	sdr_free(sdr, ruleAddr);
	sdr_list_delete(sdr, elt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}

int	ipn_lookupGroupDirective(uvast nodeNbr, unsigned int sourceServiceNbr,
		uvast sourceNodeNbr, FwdDirective *dirbuf)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		addr;
	IpnGroup	group;

	/*	This function determines the relevant FwdDirective for
	 *	the specified eid, if any.  Wild card match is okay.	*/

	CHKERR(ionLocked());
	CHKERR(nodeNbr && dirbuf);

	/*	Find best matching group.  Groups are sorted by first
	 *	node number within group size, both ascending.  So
	 *	the first group whose range encompasses the node number
	 *	is the best fit (narrowest applicable range), but
	 *	there's no way to terminate the search early.		*/

	for (elt = sdr_list_first(sdr, (_ipnConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &group, addr, sizeof(IpnGroup));
		if (group.lastNodeNbr < nodeNbr || group.firstNodeNbr > nodeNbr)
		{
			continue;
		}

		break;
	}

	if (elt == 0)
	{
		return 0;		/*	No group found.		*/
	}

	/*	Find best matching rule.				*/

	if (lookupRule(group.rules, sourceServiceNbr, sourceNodeNbr,
			dirbuf) == 0)	/*	No rule found.		*/
	{
		memcpy((char *) dirbuf, (char *) &group.defaultDirective,
				sizeof(FwdDirective));
	}

	return 1;
}
