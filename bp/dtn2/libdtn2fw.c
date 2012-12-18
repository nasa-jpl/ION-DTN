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

static Object	_dtn2dbObject(Object *newDbObj)
{
	static Object	obj = 0;

	if (newDbObj)
	{
		obj = *newDbObj;
	}

	return obj;
}

static DtnDB	*_dtn2Constants()
{
	static DtnDB	buf;
	static DtnDB	*db = NULL;
	Sdr		sdr;
	Object		dbObject;
	
	if (db == NULL)
	{
		sdr = getIonsdr();
		CHKNULL(sdr);
		dbObject = _dtn2dbObject(NULL);
		if (dbObject)
		{
			if (sdr_heap_is_halted(sdr))
			{
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtnDB));
			}
			else
			{
				CHKNULL(sdr_begin_xn(sdr));
				sdr_read(sdr, (char *) &buf, dbObject,
						sizeof(DtnDB));
				sdr_exit_xn(sdr);
			}

			db = &buf;
		}
	}
	
	return db;
}

/*	*	*	Routing information mgt functions	*	*/

static int	lookupDtn2Eid(char *uriBuffer, char *neighborClId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(Dtn2Plan, plan);
	char	nodeName[SDRSTRING_BUFSZ];
	int	nodeNameLen;
	char	*lastChar;

	for (elt = sdr_list_first(sdr, (_dtn2Constants())->plans); elt;
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

			isprintf(uriBuffer, SDRSTRING_BUFSZ, "dtn:%.62s",
					nodeName);
			return 1;
		}
	}

	return 0;
}

int	dtn2Init()
{
	Sdr	sdr = getIonsdr();
	Object	dtn2dbObject;
	DtnDB	dtn2dbBuf;

	/*	Recover the DTN database, creating it if necessary.	*/

	CHKERR(sdr_begin_xn(sdr));
	oK(senderEidLookupFunctions(lookupDtn2Eid));
	dtn2dbObject = sdr_find(sdr, DTN_DBNAME, NULL);
	switch (dtn2dbObject)
	{
	case -1:		/*	SDR error.			*/
		sdr_cancel_xn(sdr);
		putErrmsg("Failed seeking DTN database in SDR.", NULL);
		return -1;

	case 0:			/*	Not found; must create new DB.	*/
		dtn2dbObject = sdr_malloc(sdr, sizeof(DtnDB));
		if (dtn2dbObject == 0)
		{
			sdr_cancel_xn(sdr);
			putErrmsg("No space for DTN database.", NULL);
			return -1;
		}

		memset((char *) &dtn2dbBuf, 0, sizeof(DtnDB));
		dtn2dbBuf.plans = sdr_list_create(sdr);
		sdr_write(sdr, dtn2dbObject, (char *) &dtn2dbBuf,
				sizeof(DtnDB));
		sdr_catlg(sdr, DTN_DBNAME, 0, dtn2dbObject);
		if (sdr_end_xn(sdr))
		{
			putErrmsg("Can't create DTN database.", NULL);
			return -1;
		}

		break;

	default:		/*	Found DB in the SDR.		*/
		sdr_exit_xn(sdr);
	}

	oK(_dtn2dbObject(&dtn2dbObject));
	oK(_dtn2Constants());
	return 0;
}

Object	getDtnDbObject()
{
	return _dtn2dbObject(NULL);
}

DtnDB	*getDtnConstants()
{
	return _dtn2Constants();
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

	CHKVOID(directive);
	if (directive->action == fwd)
	{
		CHKVOID(sdr_begin_xn(sdr));
		sdr_free(sdr, directive->eid);
		if (sdr_end_xn(sdr) < 0)
		{
			putErrmsg("Can't destroy directive EID.", NULL);
		}

		return;
	}

	if (directive->action == xmit)
	{
		if (directive->destDuctName)
		{
			CHKVOID(sdr_begin_xn(sdr));
			sdr_free(sdr, directive->destDuctName);
			if (sdr_end_xn(sdr) < 0)
			{
				putErrmsg("Can't destroy destDuctName.", NULL);
			}

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

	CHKERR(ionLocked());
	CHKERR(nodeName && demux && dirbuf);

	/*	Find best matching plan.  Universal wild-card match,
	 *	if any, is at the end of the list, so there's no way
	 *	to terminate the search early.				*/

	for (elt = sdr_list_first(sdr, (_dtn2Constants())->plans); elt;
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
		return 0;		/*	No plan found.		*/
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
		if (stringBuffer[last] == '~'	/*	"all demuxes"	*/
		&& strncmp(stringBuffer, demux, stringLen - 1) == 0)
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
	for (elt = sdr_list_first(sdr, (_dtn2Constants())->plans); elt;
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

	CHKVOID(ionLocked());
	CHKVOID(nodeNm && planAddr && eltp);
	*eltp = 0;
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
	Sdr		sdr = getIonsdr();
	char		nodeName[SDRSTRING_BUFSZ];
	Object		nextPlan;
	Dtn2Plan	plan;
	Object		planObj;

	CHKERR(nodeNm && defaultDir);
	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	if (locatePlan(nodeName, &nextPlan) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate plan", nodeNm);
		return 0;
	}

	/*	Okay to add this plan to the database.			*/

	plan.nodeName = sdr_string_create(sdr, nodeName);
	memcpy((char *) &plan.defaultDirective, (char *) defaultDir,
			sizeof(FwdDirective));
	plan.rules = sdr_list_create(sdr);
	planObj = sdr_malloc(sdr, sizeof(Dtn2Plan));
	if (planObj)
	{
		if (nextPlan)
		{
			oK(sdr_list_insert_before(sdr, nextPlan, planObj));
		}
		else
		{
			oK(sdr_list_insert_last(sdr,
					(_dtn2Constants())->plans, planObj));
		}

		sdr_write(sdr, planObj, (char *) &plan, sizeof(Dtn2Plan));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add plan.", nodeNm);
		return -1;
	}

	return 1;
}

int	dtn2_updatePlan(char *nodeNm, FwdDirective *defaultDir)
{
	Sdr		sdr = getIonsdr();
	char		nodeName[SDRSTRING_BUFSZ];
	Object		elt;
	Object		planObj;
	Dtn2Plan	plan;

	CHKERR(nodeNm && defaultDir);
	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node", nodeNm);
		return 0;
	}

	/*	Okay to update this plan.				*/

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

	CHKERR(nodeNm);
	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown plan", nodeNm);
		return 0;
	}

	planObj = sdr_list_data(sdr, elt);
	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, planObj);
	if (sdr_list_length(sdr, plan->rules) > 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Can't remove plan; still has rules", nodeNm);
		return 0;
	}

	/*	Okay to remove this plan from the database.		*/

	sdr_list_delete(sdr, elt, NULL, NULL);
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

	CHKVOID(ionLocked());
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
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
	Sdr		sdr = getIonsdr();
	char		nodeName[SDRSTRING_BUFSZ];
	Object		elt;
			OBJ_POINTER(Dtn2Plan, plan);
	Object		nextRule;
	Dtn2Rule	ruleBuf;
	Object		addr;

	CHKERR(nodeNm && demux && directive);
	if (*demux == '\0')
	{
		writeMemo("[?] Zero-length DTN2 rule demux.");
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node", nodeNm);
		return 0;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
	if (locateRule(plan, demux, &nextRule) != 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Duplicate rule", demux);
		return 0;
	}

	/*	All parameters validated, okay to add the rule.		*/

	memset((char *) &ruleBuf, 0, sizeof(Dtn2Rule));
	ruleBuf.demux = sdr_string_create(sdr, demux);
	memcpy((char *) &ruleBuf.directive, (char *) directive,
			sizeof(FwdDirective));
	addr = sdr_malloc(sdr, sizeof(Dtn2Rule));
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

		sdr_write(sdr, addr, (char *) &ruleBuf, sizeof(Dtn2Rule));
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

int	dtn2_updateRule(char *nodeNm, char *demux, FwdDirective *directive)
{
	Sdr		sdr = getIonsdr();
	char		nodeName[SDRSTRING_BUFSZ];
	Object		elt;
			OBJ_POINTER(Dtn2Plan, plan);
	Object		ruleAddr;
	Dtn2Rule	ruleBuf;

	CHKERR(nodeNm && demux && directive);
	if (*demux == '\0')
	{
		writeMemo("[?] Zero-length DTN2 rule demux.");
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node", nodeNm);
		return 0;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
	dtn2_findRule(nodeName, demux, plan, &ruleAddr, &elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown rule", demux);
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

	CHKERR(nodeNm && demux);
	if (*demux == '\0')
	{
		writeMemo("[?] Zero-length DTN2 rule demux.");
		return 0;
	}

	if (filterNodeName(nodeName, nodeNm) < 0)
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	elt = locatePlan(nodeName, NULL);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] No plan defined for this node", nodeNm);
		return 0;
	}

	GET_OBJ_POINTER(sdr, Dtn2Plan, plan, sdr_list_data(sdr, elt));
	dtn2_findRule(nodeName, demux, plan, &ruleAddr, &elt);
	if (elt == 0)
	{
		sdr_exit_xn(sdr);
		writeMemoNote("[?] Unknown rule", demux);
		return 0;
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
