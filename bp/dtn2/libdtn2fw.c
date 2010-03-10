/*
 *	libdtn2fw.c:	functions enabling the implementation of
 *			a regional forwarder for the DTN endpoint
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

#include "dtn2fw.h"

#define	DTN_DBNAME	"dtn2Route"

/*	*	*	Globals used for DTN scheme service.	*	*/

static Object	dtn2dbObject;
static DtnDB	dtn2ConstantsBuf;
static DtnDB	*dtn2Constants = &dtn2ConstantsBuf;

static char	*NullParmsMemo = "BP error: null input parameter(s).";

/*	*	*	Routing information mgt functions	*	*/

static int	lookupDtn2Eid(char *uriBuffer, char *neighborClId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);
	char	nodeName[SDRSTRING_BUFSZ];
	int	nodeNameLen;
	char	*lastChar;

	for (elt = sdr_list_first(sdr, dtn2Constants->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
		switch (clIdMatches(neighborClId, &plan->defaultDirective))
		{
		case -1:
			putErrmsg("Failed looking up DTN2 EID.", NULL);
			return -1;

		case 0:
			continue;	/*	No match.		*/

		default:

			/*	Found the plan for transmission to
			 *	this neighbor, so now we know the
			 *	neighbor's EID.				*/

			nodeNameLen = sdr_string_read(sdr, nodeName,
					plan->nodeName);
			lastChar = nodeName + (nodeNameLen - 1);
			if (*lastChar == '~')
			{
				*lastChar = '\0';
			}

			sprintf(uriBuffer, "dtn:%.62s", nodeName);
			return 1;
		}
	}

	return 0;
}

int	dtn2Init()
{
	Sdr	sdr = getIonsdr();
	DtnDB	dtn2dbBuf;

	/*	Recover the DTN database, creating it if necessary.	*/

	sdr_begin_xn(sdr);
	oK(senderEidLookupFunctions(lookupDtn2Eid));
	dtn2dbObject = sdr_find(sdr, DTN_DBNAME, NULL);
	switch (dtn2dbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putSysErrmsg("Can't search for DTN database in SDR", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		memset((char *) &dtn2dbBuf, 0, sizeof(DtnDB));
		dtn2dbBuf.plans = sdr_list_create(sdr);
		dtn2dbObject = sdr_malloc(sdr, sizeof(DtnDB));
		if (dtn2dbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putSysErrmsg("No space for DTN database", NULL);
			return -1;
		}

		sdr_write(sdr, dtn2dbObject, (char *) &dtn2dbBuf,
				sizeof(DtnDB));
		sdr_catlg(sdr, DTN_DBNAME, 0, dtn2dbObject);
		if (sdr_end_xn(sdr))
		{
			putSysErrmsg("Can't create DTN database", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(sdr);
	}

	/*	Load constants into a conveniently accessed structure.
	 *	Note that this is NOT a current database image.		*/

	sdr_read(sdr, (char *) dtn2Constants, dtn2dbObject, sizeof(DtnDB));
	return 0;
}

Object	getDtnDbObject()
{
	return dtn2dbObject;
}

DtnDB	*getDtnConstants()
{
	return dtn2Constants;
}

static int	filterNodeName(char *outputNodeName, char *inputNodeName)
{
	int	nameLength = strlen(inputNodeName);
	int	last = nameLength - 1;

	if (nameLength == 0 || nameLength > MAX_SDRSTRING)
	{
		putErrmsg("Invalid node name length.", inputNodeName);
		return -1;
	}

	/*	Note: the '~' character is used internally to
	 *	indicate "all others" (wild card) because it's the
	 *	last printable ASCII character and therefore always
	 *	sorts last in any list.  If the user wants to use
	 *	'*' instead, we just change it silently.		*/

	memcpy(outputNodeName, inputNodeName, nameLength);
	outputNodeName[nameLength] = '\0';
	if (outputNodeName[last] == '*')
	{
		outputNodeName[last] = '~';
	}

	return 0;
}

void	dtn2_destroyDirective(FwdDirective *directive)
{
	Sdr	sdr = getIonsdr();

	if (directive == NULL) return;
	if (directive->action == fwd)
	{
		sdr_begin_xn(sdr);
		sdr_free(sdr, directive->eid);
		sdr_end_xn(sdr);
		return;
	}

	if (directive->action == xmit)
	{
		if (directive->destDuctName)
		{
			sdr_begin_xn(sdr);
			sdr_free(sdr, directive->destDuctName);
			sdr_end_xn(sdr);
		}
	}
}

int	dtn2_lookupDirective(char *nodeName, char *demux, FwdDirective *dirbuf)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	addr;
		OBJ_POINTER(Dtn2Plan, plan);
	int	stringLen;
	char	stringBuffer[SDRSTRING_BUFSZ];
	int	result;
	int	last;
		OBJ_POINTER(Dtn2Rule, rule);

	/*	This function determines the relevant FwdDirective for
	 *	the specified eid, if any.  Wild card match is okay.	*/

checkSafety(sdr);
	if (nodeName == NULL || demux == NULL || dirbuf == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return -1;
	}

	/*	Find best matching plan.  Universal wild-card match,
	 *	if any, is at the end of the list, so there's no way
	 *	to terminate the search early.				*/

	for (elt = sdr_list_first(sdr, dtn2Constants->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, Dtn2Plan, plan, addr);
		stringLen = sdr_string_read(sdr, stringBuffer, plan->nodeName);
		result = strcmp(stringBuffer, nodeName);
		if (result < 0)
		{
			continue;
		}
		
		if (result == 0)	/*	Exact match.		*/
		{
			break;		/*	Stop searching.		*/
		}

		/*	Node name in plan is greater than node name,
		 *	but it might still be a wild-card match.	*/

		last = stringLen - 1;
		if (stringBuffer[last] == '~'	/*	"all nodes"	*/
		&& strncmp(stringBuffer, nodeName, stringLen - 1) == 0)
		{
			break;		/*	Stop searching.		*/
		}
	}

	if (elt == 0)
	{
		return 0;
	}

	/*	Find best matching rule.				*/

	for (elt = sdr_list_first(sdr, plan->rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, Dtn2Rule, rule, addr);
		stringLen = sdr_string_read(sdr, stringBuffer, rule->demux);
		result = strcmp(stringBuffer, demux);
		if (result < 0)
		{
			continue;
		}

		if (result == 0)	/*	Exact match.		*/
		{
			break;		/*	Stop searching.		*/
		}

		/*	Demux in rule is greater than demux, but it
		 *	might still be a wild-card match.		*/

		last = stringLen - 1;
		if (stringBuffer[last] == '~'	/*	"all nodes"	*/
		&& strncmp(stringBuffer, nodeName, stringLen - 1) == 0)
		{
			break;		/*	Stop searching.		*/
		}
	}

	if (elt == 0)			/*	End of list.		*/
	{
		memcpy((char *) dirbuf, (char *) &plan->defaultDirective,
				sizeof(FwdDirective));
		return 1;
	}
		
	/*	Found a matching rule.					*/

	memcpy((char *) dirbuf, (char *) &rule->directive,
			sizeof(FwdDirective));
	return 1;
}

static Object	locatePlan(char *nodeName, Object *nextPlan)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);
	char	nameBuffer[SDRSTRING_BUFSZ];
	int	result;

	/*	This function locates the Dtn2Plan identified by the
	 *	specified node name, if any; must be an exact match.
	 *	If none, notes the location within the plans list at
	 *	which such a plan should be inserted.			*/

	if (nextPlan) *nextPlan = 0;	/*	Default.		*/
	for (elt = sdr_list_first(sdr, dtn2Constants->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
		sdr_string_read(sdr, nameBuffer, plan->nodeName);
		result = strcmp(nameBuffer, nodeName);
		if (result < 0)
		{
			continue;
		}
		
		if (result > 0)
		{
			if (nextPlan) *nextPlan = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	dtn2_findPlan(char *nodeNm, Object *planAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;

	/*	This function finds the Dtn2Plan for the specified
	 *	node, if any.						*/

checkSafety(sdr);
	*eltp = 0;
	if (nodeNm == NULL)
	{
		return;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return;
	}

	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		return;
	}

	*planAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	dtn2_addPlan(char *nodeNm, FwdDirective *defaultDir)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	nextPlan;
	Dtn2Plan	plan;
	Object	planObj;
	Object	elt;

	if (nodeNm == NULL || defaultDir == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		errno = EINVAL;
		return 0;
	}

	sdr_begin_xn(sdr);
	if (locatePlan(nodeName, &nextPlan) != 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("This plan is already defined.", nodeNm);
		errno = EINVAL;
		return 0;
	}

	/*	Okay to add this plan to the database.			*/

	plan.nodeName = sdr_string_create(sdr, nodeName);
	memcpy((char *) &plan.defaultDirective, (char *) defaultDir,
			sizeof(FwdDirective));
	plan.rules = sdr_list_create(sdr);
	planObj = sdr_malloc(sdr, sizeof(Dtn2Plan));
	if (planObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create plan.", nodeNm);
		return -1;
	}

	if (nextPlan)
	{
		elt = sdr_list_insert_before(sdr, nextPlan, planObj);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, dtn2Constants->plans, planObj);
	}

	sdr_write(sdr, planObj, (char *) &plan, sizeof(Dtn2Plan));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add plan.", nodeNm);
		return -1;
	}

	return 1;
}

int	dtn2_updatePlan(char *nodeNm, FwdDirective *defaultDir)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;
	Object	planObj;
	Dtn2Plan	plan;

	if (nodeNm == NULL || defaultDir == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		errno = EINVAL;
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No plan defined for this node.", nodeNm);
		errno = EINVAL;
		return 0;
	}

	planObj = sdr_list_data(sdr, elt);
	sdr_stage(sdr, (char *) &plan, planObj, sizeof(Dtn2Plan));
	dtn2_destroyDirective(&plan.defaultDirective);
	memcpy((char *) &plan.defaultDirective, (char *) defaultDir,
			sizeof(FwdDirective));
	sdr_write(sdr, planObj, (char *) &plan, sizeof(Dtn2Plan));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update plan.", nodeNm);
		return -1;
	}

	return 1;
}

int	dtn2_removePlan(char *nodeNm)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;
	Object	planObj;
		OBJ_POINTER(Dtn2Plan, plan);

	if (nodeNm == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		errno = EINVAL;
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		return 1;
	}

	planObj = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, planObj);
	if (sdr_list_length(sdr, plan->rules) > 0)
	{
		putErrmsg("Can't remove plan; still has rules.", nodeNm);
		sdr_cancel_xn(sdr);
		return 0;
	}

	dtn2_destroyDirective(&(plan->defaultDirective));
	sdr_list_destroy(sdr, plan->rules, NULL, NULL);
	sdr_free(sdr, plan->nodeName);
	sdr_free(sdr, planObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove plan.", nodeNm);
		return -1;
	}

	return 1;
}

static Object	locateRule(Dtn2Plan *plan, char *demux, Object *nextRule)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(Dtn2Rule, rule);
	char	nameBuffer[SDRSTRING_BUFSZ];
	int	result;

	/*	This function locates the Dtn2Rule identified by the
	 *	specified demux token, for the specified destination
	 *	endpoint, if any; must be an exact match.  If
	 *	none, notes the location within the rules list at
	 *	which such a rule should be inserted.			*/

	if (nextRule) *nextRule = 0;	/*	Default.		*/
	for (elt = sdr_list_first(sdr, plan->rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, Dtn2Rule, rule, sdr_list_data(sdr, elt));
		sdr_string_read(sdr, nameBuffer, rule->demux);
		result = strcmp(nameBuffer, demux);
		if (result < 0)
		{
			continue;
		}
		
		if (result > 0)
		{
			if (nextRule) *nextRule = elt;
			break;		/*	Same as end of list.	*/
		}

		return elt;
	}

	return 0;
}

void	dtn2_findRule(char *nodeNm, char *demux, Dtn2Plan *plan,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;
		OBJ_POINTER(Dtn2Plan, planPtr);

	/*	This function finds the Dtn2Rule for the specified
	 *	demux token, for the specified destination node, if
	 *	any.							*/

checkSafety(sdr);
	*eltp = 0;
	if (plan == NULL)
	{
		if (nodeNm == NULL)
		{
			return;
		}

		if (filterNodeName(nodeName, nodeNm) < 0)
		{
			return;
		}

		elt = locatePlan(nodeName, NULL);
		if (elt == 0)
		{
			return;
		}

		GET_OBJ_POINTER(sdr, Dtn2Plan, planPtr, sdr_list_data(sdr,
					elt));
		plan = planPtr;
	}

	elt = locateRule(plan, demux, NULL);
	if (elt == 0)
	{
		return;
	}

	*ruleAddr = sdr_list_data(sdr, elt);
	*eltp = elt;
}

int	dtn2_addRule(char *nodeNm, char *demux, FwdDirective *directive)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);
	Object	nextRule;
	Dtn2Rule	ruleBuf;
	Object	addr;

	if (nodeNm == NULL || demux == NULL || *demux == '\0'
	|| directive == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		errno = EINVAL;
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No plan defined for this node.", nodeNm);
		errno = EINVAL;
		return 0;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
	if (locateRule(plan, demux, &nextRule) != 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Duplicate rule, already in database.", NULL);
		errno = EINVAL;
		return 0;
	}

	/*	All parameters validated, okay to add the rule.		*/

	memset((char *) &ruleBuf, 0, sizeof(Dtn2Rule));
	ruleBuf.demux = sdr_string_create(sdr, demux);
	memcpy((char *) &ruleBuf.directive, (char *) directive,
			sizeof(FwdDirective));
	addr = sdr_malloc(sdr, sizeof(Dtn2Rule));
	if (addr == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("No space for rule.", NULL);
		return -1;
	}

	if (nextRule)
	{
		elt = sdr_list_insert_before(sdr, nextRule, addr);
	}
	else
	{
		elt = sdr_list_insert_last(sdr, plan->rules, addr);
	}

	sdr_write(sdr, addr, (char *) &ruleBuf, sizeof(Dtn2Rule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

int	dtn2_updateRule(char *nodeNm, char *demux, FwdDirective *directive)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);
	Object	ruleAddr;
	Dtn2Rule	ruleBuf;

	if (nodeNm == NULL || demux == NULL || *demux == '\0'
	|| directive == NULL)
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		errno = EINVAL;
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("No plan defined for this node.", nodeNm);
		errno = EINVAL;
		return 0;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
	dtn2_findRule(nodeName, demux, plan, &ruleAddr, &elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		putErrmsg("Unknown rule, not in database.", NULL);
		errno = EINVAL;
		return 0;
	}

	/*	All parameters validated, okay to update the rule.	*/

	sdr_stage(sdr, (char *) &ruleBuf, ruleAddr, sizeof(Dtn2Rule));
	dtn2_destroyDirective(&ruleBuf.directive);
	memcpy((char *) &ruleBuf.directive, (char *) directive,
			sizeof(FwdDirective));
	sdr_write(sdr, ruleAddr, (char *) &ruleBuf, sizeof(Dtn2Rule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

int	dtn2_removeRule(char *nodeNm, char *demux)
{
	Sdr	sdr = getIonsdr();
	char	nodeName[SDRSTRING_BUFSZ];
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);
	Object	ruleAddr;
		OBJ_POINTER(Dtn2Rule, rule);

	if (nodeNm == NULL || demux == NULL || *demux == '\0')
	{
		putErrmsg(NullParmsMemo, NULL);
		errno = EINVAL;
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		errno = EINVAL;
		return 0;
	}

	sdr_begin_xn(sdr);
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		putErrmsg("No plan defined for this node.", nodeNm);
		errno = EINVAL;
		return sdr_end_xn(sdr);
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
	dtn2_findRule(nodeName, demux, plan, &ruleAddr, &elt);
	if (elt == 0)
	{
		errno = EINVAL;
		return sdr_end_xn(sdr);
	}

	/*	All parameters validated, okay to remove the rule.	*/

	GET_OBJ_POINTER(sdr, Dtn2Rule, rule, ruleAddr);
	dtn2_destroyDirective(&(rule->directive));
	sdr_free(sdr, ruleAddr);
	sdr_list_delete(sdr, elt, NULL, NULL);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}
