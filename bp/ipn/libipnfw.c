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
		ipndbBuf.plans = sdr_list_create(sdr);
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
	Sdr	sdr = getIonsdr();
	Object	outductAddr;
		OBJ_POINTER(Outduct, outduct);
		OBJ_POINTER(ClProtocol, protocol);

	directive->action = xmit;
	directive->outductElt = parms->outductElt;
	outductAddr = sdr_list_data(sdr, directive->outductElt);
	GET_OBJ_POINTER(sdr, Outduct, outduct, outductAddr);
	GET_OBJ_POINTER(sdr, ClProtocol, protocol, outduct->protocol);
	directive->protocolClass = protocol->protocolClass;
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

static int	ruleMatches(IpnPlan *plan, DuctExpression *ductExpression)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnRule, rule);
	char	destDuctName[SDRSTRING_BUFSZ];

	for (elt = sdr_list_first(sdr, plan->rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnRule, rule, sdr_list_data(sdr, elt));
		if (!(rule->directive.action == xmit
		&& rule->directive.outductElt == ductExpression->outductElt))
		{
			continue;	/*	Not a match.		*/
		}

		if (ductExpression->destDuctName == NULL)
		{
			/*	Non-promiscuous protocol; this is
			 *	the egress plan we're looking for.	*/

			return 1;
		}

		/*	ductExpression is for a promiscuous protocol,
		 *	so must match on destDuctName as well.		*/

		if (sdr_string_read(sdr, destDuctName,
				rule->directive.destDuctName) < 0)
		{
			putErrmsg("Can't retrieve rule dest duct name.", NULL);
			return 0;
		}

		if (strcmp(destDuctName, ductExpression->destDuctName) == 0)
		{
			return 1;
		}
	}

	return 0;
}

uvast	ipn_planNodeNbr(DuctExpression *ductExpression)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);
	char	destDuctName[SDRSTRING_BUFSZ];

	/*	This function returns the node number associated with
	 *	the IpnPlan containing a directive that indicates
	 *	transmission to the duct identified by ductExpression,
	 *	if any.							*/

	CHKZERO(ionLocked());
	CHKZERO(ductExpression);
	for (elt = sdr_list_first(sdr, (_ipnConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnPlan, plan, sdr_list_data(sdr, elt));
		if (ruleMatches(plan, ductExpression))
		{
			return plan->nodeNbr;
		}

		/*	No override rule cites this outduct.		*/

		if (!(plan->defaultDirective.action == xmit
		&& plan->defaultDirective.outductElt
				== ductExpression->outductElt))
		{
			continue;	/*	Not a match.		*/
		}

		if (ductExpression->destDuctName == NULL)
		{
			/*	Non-promiscuous protocol; this is
			 *	the egress plan we're looking for.	*/

			return plan->nodeNbr;
		}

		/*	ductExpression is for a promiscuous protocol,
		 *	so must match on destDuctName as well.		*/

		if (sdr_string_read(sdr, destDuctName,
				plan->defaultDirective.destDuctName) < 0)
		{
			putErrmsg("Can't retrieve plan dest duct name.", NULL);
			return 0;
		}

		if (strcmp(destDuctName, ductExpression->destDuctName) == 0)
		{
			return plan->nodeNbr;
		}
	}

	return 0;
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
			uvast sourceNodeNbr, int protClassReqd,
			FwdDirective *dirbuf)
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
		if ((rule->directive.protocolClass & protClassReqd) == 0)
		{
			continue;	/*	Can't use this rule.	*/
		}

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
		uvast sourceNodeNbr, Bundle *bundle, FwdDirective *dirbuf)
{
	Sdr	sdr = getIonsdr();
	int	protClassReqd;
	Object	addr;
	Object	elt;
		OBJ_POINTER(IpnPlan, plan);

	/*	This function determines the relevant FwdDirective for
	 *	the specified eid, if any.  Wild card match is okay.	*/

	CHKERR(ionLocked());
	CHKERR(nodeNbr && dirbuf);

	/*	Determine constraints on directive usability.		*/

	protClassReqd = bundle->extendedCOS.flags & BP_PROTOCOL_BOTH;
	if (protClassReqd == 0)		/*	Don't care.		*/
	{
		protClassReqd = -1;	/*	Matches any.		*/
	}
	else if (protClassReqd == 10)	/*	Need BSS.		*/
	{
		protClassReqd = BP_PROTOCOL_STREAMING;
	}

	/*	Find the matching plan.					*/

	ipn_findPlan(nodeNbr, &addr, &elt);
	if (elt == 0)
	{
		return 0;		/*	No plan found.		*/
	}

	GET_OBJ_POINTER(sdr, IpnPlan, plan, addr);

	/*	Find best matching rule.				*/

	if (lookupRule(plan->rules, sourceServiceNbr, sourceNodeNbr,
			protClassReqd, dirbuf) == 0)	/*	None.	*/
	{
		if ((plan->defaultDirective.protocolClass & protClassReqd) == 0)
		{
			return 0;	/*	Matching plan unusable.	*/
		}

		memcpy((char *) dirbuf, (char *) &plan->defaultDirective,
				sizeof(FwdDirective));
	}

	return 1;
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
	CHKVOID(firstNodeNbr && exitAddr && eltp);
	CHKVOID(firstNodeNbr <= lastNodeNbr);
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
	Sdr		sdr = getIonsdr();
	Object		nextExit;
	IpnExit	exit;
	Object		addr;

	CHKERR(firstNodeNbr && lastNodeNbr && viaEid);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(strlen(viaEid) <= MAX_SDRSTRING);
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
	exit.rules = sdr_list_create(sdr);
	exit.defaultDirective.action = fwd;
	exit.defaultDirective.eid = sdr_string_create(sdr, viaEid);
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
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		addr;
	IpnExit	exit;

	CHKERR(firstNodeNbr && lastNodeNbr && viaEid);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(strlen(viaEid) <= MAX_SDRSTRING);
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
	sdr_free(sdr, exit.defaultDirective.eid);
	exit.defaultDirective.eid = sdr_string_create(sdr, viaEid);
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

	CHKERR(firstNodeNbr && lastNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
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
	if (sdr_list_length(sdr, exit->rules) > 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Can't remove exit; still has rules",
				utoa(firstNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to remove the exit.	*/

	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, exit->defaultDirective.eid);
	sdr_free(sdr, addr);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove exit.", NULL);
		return -1;
	}

	return 1;
}

void	ipn_findExitRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr, IpnExit *exit,
		Object *ruleAddr, Object *eltp)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnExit, exitPtr);

	/*	This function finds the IpnRule for the specified
	 *	service number and source node number, for the
	 *	specified destination node number, if any.		*/

	CHKVOID(ionLocked());
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;
	if (exit == NULL)
	{
		if (firstNodeNbr == 0 || lastNodeNbr < firstNodeNbr)
		{
			return;
		}

		elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
		if (elt == 0)
		{
			return;
		}

		GET_OBJ_POINTER(sdr, IpnExit, exitPtr,
				sdr_list_data(sdr, elt));
		exit = exitPtr;
	}

	elt = locateRule(exit->rules, srcServiceNbr, srcNodeNbr, NULL);
	if (elt == 0)
	{
		return;
	}

	*ruleAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	ipn_addExitRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnExit, exit);
	Object		nextRule;
	IpnRule		ruleBuf;
	Object		addr;

	CHKERR(firstNodeNbr && lastNodeNbr && srcNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Exit is unknown", utoa(firstNodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
	if (locateRule(exit->rules, srcServiceNbr, srcNodeNbr, &nextRule) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate rule", utoa(srcNodeNbr));
		return 0;
	}

	/*	All parameters validated, okay to add the rule.		*/

	memset((char *) &ruleBuf, 0, sizeof(IpnRule));
	ruleBuf.srcServiceNbr = srcServiceNbr;
	ruleBuf.srcNodeNbr = srcNodeNbr;
	ruleBuf.directive.action = fwd;
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
			elt = sdr_list_insert_last(sdr, exit->rules, addr);
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

int	ipn_updateExitRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr, char *viaEid)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnExit, exit);
	Object		ruleAddr;
	IpnRule		ruleBuf;

	CHKERR(firstNodeNbr && lastNodeNbr && srcNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Exit is unknown", utoa(firstNodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
	ipn_findExitRule(firstNodeNbr, lastNodeNbr, srcServiceNbr, srcNodeNbr,
			exit, &ruleAddr, &elt);
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

int	ipn_removeExitRule(uvast firstNodeNbr, uvast lastNodeNbr,
		int argServiceNbr, vast argNodeNbr)
{
	Sdr		sdr = getIonsdr();
	unsigned int	srcServiceNbr = (argServiceNbr == -1 ?
				IPN_ALL_OTHER_SERVICES : argServiceNbr);
	uvast		srcNodeNbr = (argNodeNbr == -1 ? IPN_ALL_OTHER_NODES
				: argNodeNbr);
	Object		elt;
			OBJ_POINTER(IpnExit, exit);
	Object		ruleAddr;
			OBJ_POINTER(IpnRule, rule);

	CHKERR(firstNodeNbr && lastNodeNbr && srcNodeNbr);
	CHKERR(firstNodeNbr <= lastNodeNbr);
	CHKERR(sdr_begin_xn(sdr));
	elt = locateExit(firstNodeNbr, lastNodeNbr, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Exit is unknown", utoa(firstNodeNbr));
		return 0;
	}

	GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));
	ipn_findExitRule(firstNodeNbr, lastNodeNbr, srcServiceNbr, srcNodeNbr,
			exit, &ruleAddr, &elt);
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

int	ipn_lookupExitDirective(uvast nodeNbr, unsigned int sourceServiceNbr,
		uvast sourceNodeNbr, FwdDirective *dirbuf)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		addr;
	IpnExit	exit;

	/*	This function determines the relevant FwdDirective for
	 *	the specified eid, if any.  Wild card match is okay.	*/

	CHKERR(ionLocked());
	CHKERR(nodeNbr && dirbuf);

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

	/*	Find best matching rule.				*/

	if (lookupRule(exit.rules, sourceServiceNbr, sourceNodeNbr,
			-1, dirbuf) == 0)		/*	None.	*/
	{
		memcpy((char *) dirbuf, (char *) &exit.defaultDirective,
				sizeof(FwdDirective));
	}

	return 1;
}

void	ipn_forgetOutduct(Object ductElt)
{
	Sdr	sdr = getIonsdr();
	IpnDB	*db;
	Object	planElt;
	Object	nextPlanElt;
	Object	planAddr;
		OBJ_POINTER(IpnPlan, plan);
	Object	ruleElt;
	Object	nextRuleElt;
	Object	ruleAddr;
		OBJ_POINTER(IpnRule, rule);

	CHKVOID(ionLocked());
	if (ipnInit() < 0 || (db = getIpnConstants()) == NULL)
	{
		return;
	}

	for (planElt = sdr_list_first(sdr, db->plans); planElt;
			planElt = nextPlanElt)
	{
		nextPlanElt = sdr_list_next(sdr, planElt);
		planAddr = sdr_list_data(sdr, planElt);
		GET_OBJ_POINTER(sdr, IpnPlan, plan, planAddr);
		for (ruleElt = sdr_list_first(sdr, plan->rules); ruleElt;
				ruleElt = nextRuleElt)
		{
			nextRuleElt = sdr_list_next(sdr, ruleElt);
			ruleAddr = sdr_list_data(sdr, ruleElt);
			GET_OBJ_POINTER(sdr, IpnRule, rule, ruleAddr);
			if (rule->directive.outductElt == ductElt)
			{
				destroyXmitDirective(&(rule->directive));
				sdr_free(sdr, ruleAddr);
				sdr_list_delete(sdr, ruleElt, NULL, NULL);
			}
		}

		if (plan->defaultDirective.outductElt == ductElt)
		{
			destroyXmitDirective(&(plan->defaultDirective));
			sdr_free(sdr, planAddr);
			sdr_list_delete(sdr, planElt, NULL, NULL);
		}
	}

	/*	Note: Ipn group directives never reference outducts.	*/
}
