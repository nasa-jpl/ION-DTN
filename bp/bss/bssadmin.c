/*
 *	bssadmin.c:	BP routing adminstration interface for
 *			the IPN endpoint ID scheme, extended to
 *			support bundle streaming service.
 *									
 *									
 *	Copyright (c) 2006, California Institute of Technology.	
 *	Copyright (c) 2011, Space Internetworking Center,
 *	Democritus University of Thrace.
 *
 *	All rights reserved.						
 *	
 *	Authors: Scott Burleigh, Jet Propulsion Laboratory
 *		 Sotirios-Angelos Lenas, Space Internetworking Center (SPICE)
 *
 *	Modification History:
 *	Date	  Who	What
 *	08-08-11  SAL	Bundle Streaming Service extension.
 */

#include "bssfw.h"

static int	_echo(int *newValue)
{
	static int	state = 0;

	if (newValue)
	{
		if (*newValue == 1)
		{
			state = 1;
		}
		else
		{
			state = 0;
		}
	}

	return state;
}

static void	printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	handleQuit()
{
	printText("Please enter command 'q' to stop the program.");
}

static void	printSyntaxError(int lineNbr)
{
	char	buffer[80];

	isprintf(buffer, sizeof buffer, "Syntax error at line %d of bssadmin.c",
			lineNbr);
	printText(buffer);
}

#define	SYNTAX_ERROR	printSyntaxError(__LINE__)

static void	printUsage()
{
	PUTS("Syntax of 'duct expression' is:");
	PUTS("\t<protocol name>/<outduct name>[,<dest induct name>]");
	PUTS("Syntax of 'qualifier' is:");
	PUTS("\t{ <source service nbr> | * } { <source node nbr> | * }");
	PUTS("Valid commands are:");
	PUTS("\tq\tQuit");
	PUTS("\th\tHelp");
	PUTS("\t?\tHelp");
	PUTS("\tv\tPrint version of ION.");
	PUTS("\ta\tAdd");
	PUTS("\t   a plan <node nbr> <default duct expression> \
<RT mode duct expression> <PB mode duct expression> \
<custody expiration interval>");
	PUTS("\t   a planrule <node nbr> <qualifier> <default duct expression> \
<RT mode duct expression> <PB mode duct expression>");
	PUTS("\t   a group <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\t   a grouprule <first node nbr> <last node nbr> <qualifier> \
<endpoint ID>");
	PUTS("\t   a entry <destination service nbr> <destination node nbr>");
	PUTS("\tc\tChange");
	PUTS("\t   c plan <node nbr> <default duct expression> \
<RT mode duct expression> <PB mode duct expression> \
<custody expiration interval>");
	PUTS("\t   c planrule <node nbr> <qualifier> <default duct expression> \
<RT mode duct expression> <PB mode duct expression>");
	PUTS("\t   c group <first node nbr> <last node nbr> <endpoint ID>");
	PUTS("\t   c grouprule <first node nbr> <last node nbr> <qualifier> \
<endpoint ID>");
	PUTS("\td\tDelete");
	PUTS("\ti\tInfo");
	PUTS("\t   {d|i} plan <node nbr>");
	PUTS("\t   {d|i} planrule <node nbr> <qualifier>");
	PUTS("\t   {d|i} group <first node nbr> <last node nbr>");
	PUTS("\t   {d|i} grouprule <first node nbr> <last node nbr> \
<qualifier>");
	PUTS("\t   {d} entry <destination service nbr> <destination node nbr>");
	PUTS("\tl\tList");
	PUTS("\t   l group");
	PUTS("\t   l plan");
	PUTS("\t   l planrule <node nbr>");
	PUTS("\t   l grouprule <first node nbr> <last node nbr>");
	PUTS("\t   l entry");
	PUTS("\te\tEnable or disable echo of printed output to log file");
	PUTS("\t   e { 0 | 1 }");
	PUTS("\t#\tComment");
	PUTS("\t   # <comment text>");
}

static int	parseDuctExpression(char *token, DuctExpression *expression)
{
	char		*cursor = NULL;
	char		*protocolName;
	char		*outductName;
	VOutduct	*vduct;
	PsmAddress	vductElt;

	memset((char *) expression, 0, sizeof(DuctExpression));
	protocolName = token;
	cursor = strchr(token, '/');
	if (cursor == NULL ) 
	{		
		return 0; 	/* 
				 *  either default, RT or PB mode directive is  
				 *  not configured properly 
				 */
	}
 
	*cursor = '\0';		/*	Delimit protocol name.	*/
	cursor++;
	outductName = cursor;

	/*
	 *  If there's a destination duct name, note end of
	 *  outduct name and start of destination duct name.
	 */

	cursor = strchr(cursor, ',');
	if (cursor == NULL)
	{
		/*	End of token delimits outduct name.	*/

		expression->destDuctName = NULL;
	}
	else
	{
		*cursor = '\0';	/*	Delimit outduct name.	*/
		cursor++;
		expression->destDuctName = cursor;
	}

	findOutduct(protocolName, outductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		/* 
		 *  Flag the expression's outductElt as invalid, so
		 *  that it will be discarded later.
		 *
		 *  The parseDuctExpressions function checks
		 *  expression->outductElt value in order to determine
		 *  why parseDuctExpression returned 0.  A value other
		 *  than zero must be given in order to differentiate
		 *  the "unknown outduct" reason for returning zero
		 *  from the "cursor == NULL" reason.  Setting
		 *  expression->outductElt to 1  won't cause any other
		 *  problems since bssadmin will discard later the whole 
		 *  expression structure.
		 */

		expression->outductElt = 1;
		writeMemoNote("[?] Unknown outduct", outductName);
		return 0;
	}

	expression->outductElt = vduct->outductElt;
	return 1;
}

static int	parseDuctExpressions(char *defaultToken,
				char *rtToken, char *pbToken,
				DuctExpression *defaultExpression, 
				DuctExpression *rtExpression, 
				DuctExpression *pbExpression)
{
	int	rtResult;
	int	pbResult;	
	
	/* 
	 *  The values of defaultProtocolName, rtProtocolName and 
	 *  pbProtocolName are checked in parseDuctExpression 
	 *  function
 	 */	

	if (parseDuctExpression(defaultToken, defaultExpression) == 0 )
	{
		writeMemoNote("[?] Malformed duct expression: <protocol>/<duct \
name>", defaultToken);
		return 0;
	}

	rtResult = parseDuctExpression(rtToken, rtExpression);
	pbResult = parseDuctExpression(pbToken, pbExpression);

	/* 
	 *  If both rtExpression and pbExpression are null, the
	 *  configuration is valid; this is normal non-BSS IPN
	 *  forwarding.
	 */

	if (rtResult == 0 && pbResult == 0
	&& rtExpression->outductElt == 0 && pbExpression->outductElt == 0) 
	{
		writeMemo("[i] No BSS directives are declared");
		return 1;
	}

	if (rtResult == 0 || pbResult == 0)
	{
		writeMemoNote("[?] Either RT or PB mode directive is not \
configured properly.", NULL);
		return 0;	
	}
	
	return 1;
}

static void	executeAdd(int tokenCount, char **tokens)
{
	DuctExpression	defaultExpression;
	DuctExpression	rtExpression;
	DuctExpression	pbExpression;
	int		sourceServiceNbr;
	vast		sourceNodeNbr;
	int		dstServiceNbr;
	vast		dstNodeNbr;
	int		rtt; 		/* BSS - Custody expiration interval */

	if (tokenCount < 2)
	{
		printText("Add what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{				
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDuctExpressions(tokens[3], tokens[4], tokens[5], 
		    &defaultExpression, &rtExpression, &pbExpression) == 0) 
		{
			return;
		}
		
		
		rtt = strtol(tokens[6], NULL, 0);
		if (rtt==0 || rtt<0)
		{
			rtt = 63072000; 	/*	2 years		*/
		}

		bss_addPlan(strtouvast(tokens[2]), &defaultExpression,
			&rtExpression, &pbExpression, rtt);
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{			
		if (tokenCount != 8)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtol(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtovast(tokens[4]);
		}

		if (parseDuctExpressions(tokens[5], tokens[6], tokens[7], 
		    &defaultExpression, &rtExpression, &pbExpression) == 0)
		{
			return;
		}
 
		bss_addPlanRule(strtouvast(tokens[2]), sourceServiceNbr,
			sourceNodeNbr, &defaultExpression, &rtExpression,
			&pbExpression);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{	
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		bss_addGroup(strtouvast(tokens[2]), strtouvast(tokens[3]),
				tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtol(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtovast(tokens[5]);
		}

		bss_addGroupRule(strtouvast(tokens[2]), strtouvast(tokens[3]),
				sourceServiceNbr, sourceNodeNbr, tokens[6]);
		return;
	}

	if (strcmp(tokens[1], "entry") == 0)
	{			
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}
		
		if (strcmp(tokens[2], "*") == 0)
		{
			dstServiceNbr = -1;
		}
		else
		{
			dstServiceNbr = strtol(tokens[2], NULL, 0);
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			dstNodeNbr = -1;
		}
		else
		{
			dstNodeNbr = strtovast(tokens[3]);
		}

		bss_addBssEntry(dstServiceNbr,dstNodeNbr);				
		return;
	}
	
	SYNTAX_ERROR;
}

static void	executeChange(int tokenCount, char **tokens)
{
	DuctExpression	defaultExpression;
	DuctExpression	rtExpression;
	DuctExpression	pbExpression;
	int		sourceServiceNbr;
	vast		sourceNodeNbr;
	int		rtt; 		/* BSS - Custody expiration interval */

	if (tokenCount < 2)
	{
		printText("Change what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (parseDuctExpressions(tokens[3], tokens[4], tokens[5], 
		    &defaultExpression, &rtExpression, &pbExpression) == 0)
		{
			return;
		}
		
		rtt = strtol(tokens[6], NULL, 0);
		if (rtt < 0 || rtt == 0)
		{
			rtt = 63072000; 	/*	2 years		*/
		}

		bss_updatePlan(strtouvast(tokens[2]), &defaultExpression,
				&rtExpression, &pbExpression, rtt);
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount != 8)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtol(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtovast(tokens[4]);
		}

		if (parseDuctExpressions(tokens[5], tokens[6], tokens[7], 
		    &defaultExpression, &rtExpression, &pbExpression) == 0)
		{
			return;
		}

		bss_updatePlanRule(strtouvast(tokens[2]), sourceServiceNbr,
				sourceNodeNbr, &defaultExpression,
				&rtExpression, &pbExpression);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		bss_updateGroup(strtouvast(tokens[2]), strtouvast(tokens[3]),
				tokens[4]);
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount != 7)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtol(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtovast(tokens[5]);
		}

		bss_updateGroupRule(strtouvast(tokens[2]),
				strtouvast(tokens[3]), sourceServiceNbr,
				sourceNodeNbr, tokens[6]);
		return;
	}

	SYNTAX_ERROR;
}

static void	executeDelete(int tokenCount, char **tokens)
{
	int	sourceServiceNbr;
	vast	sourceNodeNbr;
	int	dstServiceNbr;
	vast	dstNodeNbr;

	if (tokenCount < 2)
	{
		printText("Delete what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		if (tokenCount != 3)
		{
			SYNTAX_ERROR;
			return;
		}

		bss_removePlan(strtouvast(tokens[2]));
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount != 5)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtol(tokens[3], NULL, 0);
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtovast(tokens[4]);
		}

		bss_removePlanRule(strtouvast(tokens[2]), sourceServiceNbr,
				sourceNodeNbr);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		bss_removeGroup(strtouvast(tokens[2]), strtouvast(tokens[3]));
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		if (tokenCount != 6)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[4], "*") == 0)
		{
			sourceServiceNbr = -1;
		}
		else
		{
			sourceServiceNbr = strtol(tokens[4], NULL, 0);
		}

		if (strcmp(tokens[5], "*") == 0)
		{
			sourceNodeNbr = -1;
		}
		else
		{
			sourceNodeNbr = strtouvast(tokens[5]);
		}

		bss_removeGroupRule(strtouvast(tokens[2]),
			strtouvast(tokens[3]), sourceServiceNbr,
			sourceNodeNbr);
		return;
	}

	if (strcmp(tokens[1], "entry") == 0)
	{	
		if (tokenCount != 4)
		{
			SYNTAX_ERROR;
			return;
		}

		if (strcmp(tokens[2], "*") == 0)
		{
			dstServiceNbr = -1;
		}
		else
		{
			dstServiceNbr = strtol(tokens[2], NULL, 0);
		}

		if (strcmp(tokens[3], "*") == 0)
		{
			dstNodeNbr = -1;
		}
		else
		{
			dstNodeNbr = strtovast(tokens[3]);
		}

		bss_removeBssEntry(dstServiceNbr, dstNodeNbr);
		return;
	}

	SYNTAX_ERROR;
}

static void	printDirectives(char *context, FwdDirective *dir1, 
			       FwdDirective *dir2, FwdDirective *dir3, 
			       unsigned int rtt)
{
	Sdr		sdr = getIonsdr();
	Object		ductObj;
			OBJ_POINTER(Outduct, duct);
			OBJ_POINTER(ClProtocol, clp);
	char		string[SDRSTRING_BUFSZ + 1];
	char		buffer[1024];
	char		totalBuffer[1024];
	FwdDirective 	*dir;
	int i;

	totalBuffer[0] = '\0';
	for (i = 0; i < 3; i++)
	{
		if 	(i==0) 	dir=dir1;
		else if (i==1) 	dir=dir2;
		else 		dir=dir3;
		if (dir->outductElt != 0) 
		{
			memset(buffer, '\0', 1024);
			switch (dir->action)
			{
			case xmit:
				ductObj = sdr_list_data(sdr, dir->outductElt);
				GET_OBJ_POINTER(sdr, Outduct, duct, ductObj);
				GET_OBJ_POINTER(sdr, ClProtocol, clp, 
					duct->protocol);
				if (dir->destDuctName == 0)
				{
					string[0] = '\0';
				}
				else
				{
					string[0] = ',';
					if (sdr_string_read(sdr, string + 1,
						dir->destDuctName) < 0)
					{
						istrcpy(string + 1, "?",
							sizeof string - 1);
					}
				}

				isprintf(buffer, sizeof buffer, "%.700s x %.8s/\
					%.128s%.128s", totalBuffer, clp->name, 
					duct->name, string);
				istrcpy(totalBuffer, buffer,
					sizeof totalBuffer);
				break;

			case fwd:
				if (sdr_string_read(sdr, string, dir->eid) < 0)
				{
					istrcpy(string, "?", sizeof string);
				}
	
				isprintf(buffer, sizeof buffer,
					"%.700s f %.255s", totalBuffer, string);
				istrcpy(totalBuffer, buffer,
					sizeof totalBuffer);
				break;

			default:
				isprintf(buffer, sizeof buffer, "%.700s ?", 
					 totalBuffer);
				istrcpy(totalBuffer, buffer,
					sizeof totalBuffer);
			}

		}
	}

	if (rtt != 0)
	{
		isprintf(buffer, sizeof buffer, "%.80s %.900s %u", 
			 context, totalBuffer, rtt);
	}
	else
	{
		isprintf(buffer, sizeof buffer, "%.80s %.900s", 
			 context, totalBuffer);
	}

	printText(buffer);
}

static void	printPlan(BssPlan *plan)
{
	char	context[32];

	isprintf(context, sizeof context, UVAST_FIELDSPEC, plan->nodeNbr);
	printDirectives(context, &plan->defaultDirective, &plan->rtDirective, 
		&plan->pbDirective, plan->expectedRTT);
}

static void	infoPlan(int tokenCount, char **tokens)
{
	Sdr	sdr=getIonsdr();
	int	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(BssPlan, plan);

	if (tokenCount != 3)
	{
		SYNTAX_ERROR;
		return;
	}
	
	CHKVOID(sdr_begin_xn(sdr));		
	nodeNbr = strtol(tokens[2], NULL, 0);
	bss_findPlan(nodeNbr, &planAddr, &elt);
	sdr_exit_xn(sdr);
	if (elt == 0)
	{
		printText("Unknown node.");
		return;
	}
	
	GET_OBJ_POINTER(getIonsdr(), BssPlan, plan, planAddr);
	printPlan(plan);
}

static void	printRule(BssRule *rule)
{
	char	sourceServiceString[21];
	char	sourceNodeString[21];
	char	context[80];

	if (rule->srcServiceNbr == 0)
	{
		istrcpy(sourceServiceString, "*", sizeof sourceServiceString);
	}
	else
	{
		isprintf(sourceServiceString, sizeof sourceServiceString,
				"%ld", rule->srcServiceNbr);
	}

	if (rule->srcNodeNbr == 0)
	{
		istrcpy(sourceNodeString, "*", sizeof sourceNodeString);
	}
	else
	{
		isprintf(sourceNodeString, sizeof sourceNodeString, "%ld",
				rule->srcNodeNbr);
	}

	isprintf(context, sizeof context, "rule for service %s from node %s =",
			sourceServiceString, sourceNodeString);
	printDirectives(context, &rule->directive, &rule->rtDirective, 
			&rule->pbDirective, 0);
}

static void	infoPlanRule(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(BssPlan, plan);
	int	sourceServiceNbr;
	vast	sourceNodeNbr;
	Object	ruleAddr;
		OBJ_POINTER(BssRule, rule);

	if (tokenCount != 5)
	{
		SYNTAX_ERROR;
		return;
	}

	nodeNbr = strtouvast(tokens[2]);
	CHKVOID(sdr_begin_xn(sdr));
	bss_findPlan(nodeNbr, &planAddr, &elt);
	sdr_exit_xn(sdr);
	if (elt == 0)
	{
		printText("Unknown node.");
		return;
	}

	GET_OBJ_POINTER(sdr, BssPlan, plan, planAddr);
	printPlan(plan);
	if (strcmp(tokens[3], "*") == 0)
	{
		sourceServiceNbr = -1;
	}
	else
	{
		sourceServiceNbr = strtol(tokens[3], NULL, 0);
	}

	if (strcmp(tokens[4], "*") == 0)
	{
		sourceNodeNbr = -1;
	}
	else
	{
		sourceNodeNbr = strtovast(tokens[4]);
	}

	CHKVOID(sdr_begin_xn(sdr));
	bss_findPlanRule(nodeNbr, sourceServiceNbr, sourceNodeNbr, plan,
			&ruleAddr, &elt);
	sdr_exit_xn(sdr);
	if (elt == 0)
	{
		printText("Unknown rule.");
		return;
	}

	GET_OBJ_POINTER(sdr, BssRule, rule, ruleAddr);
	printRule(rule);
}

static void	printGroup(IpnGroup *group)
{
	char	eidString[SDRSTRING_BUFSZ];
	char	buffer[384];

	sdr_string_read(getIonsdr(), eidString, group->defaultDirective.eid);
	isprintf(buffer, sizeof buffer, "From " UVAST_FIELDSPEC " \
through " UVAST_FIELDSPEC ", forward via %.256s.",
			group->firstNodeNbr, group->lastNodeNbr, eidString);
	printText(buffer);
}

static void	infoGroup(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	firstNodeNbr;
	uvast	lastNodeNbr;
	Object	elt;
		OBJ_POINTER(IpnGroup, group);

	if (tokenCount != 4)
	{
		SYNTAX_ERROR;
		return;
	}

	firstNodeNbr = strtouvast(tokens[2]);
	lastNodeNbr = strtouvast(tokens[3]);
	if (lastNodeNbr < firstNodeNbr)
	{
		printText("Unknown group.");
		return;
	}

	for (elt = sdr_list_first(sdr, (getBssConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
		if (group->firstNodeNbr == firstNodeNbr
		&& group->lastNodeNbr == lastNodeNbr)
		{
			printGroup(group);
			return;
		}
	}

	printText("Unknown group.");
}

static void	infoGroupRule(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();
	uvast	firstNodeNbr;
	uvast	lastNodeNbr;
	Object	groupAddr;
	Object	elt;
		OBJ_POINTER(IpnGroup, group);
	int	sourceServiceNbr;
	vast	sourceNodeNbr;
	Object	ruleAddr;
		OBJ_POINTER(BssRule, rule);

	if (tokenCount != 6)
	{
		SYNTAX_ERROR;
		return;
	}

	firstNodeNbr = strtouvast(tokens[2]);
	lastNodeNbr = strtouvast(tokens[3]);
	CHKVOID(sdr_begin_xn(sdr));
	bss_findGroup(firstNodeNbr, lastNodeNbr, &groupAddr, &elt);
	sdr_exit_xn(sdr);
	if (elt == 0)
	{
		printText("Unknown node.");
		return;
	}

	GET_OBJ_POINTER(sdr, IpnGroup, group, groupAddr);
	printGroup(group);
	if (strcmp(tokens[4], "*") == 0)
	{
		sourceServiceNbr = -1;
	}
	else
	{
		sourceServiceNbr = strtol(tokens[4], NULL, 0);
	}

	if (strcmp(tokens[5], "*") == 0)
	{
		sourceNodeNbr = -1;
	}
	else
	{
		sourceNodeNbr = strtovast(tokens[5]);
	}

	CHKVOID(sdr_begin_xn(sdr));
	bss_findGroupRule(firstNodeNbr, lastNodeNbr, sourceServiceNbr,
			sourceNodeNbr, group, &ruleAddr, &elt);
	sdr_exit_xn(sdr);
	if (elt == 0)
	{
		printText("Unknown rule.");
		return;
	}

	GET_OBJ_POINTER(sdr, BssRule, rule, ruleAddr);
	printRule(rule);
}

static void	printEntry(int count, bssEntry *entry)
{
	char	context[64];

	isprintf(context, sizeof context,
			"   %d:     %u	  -    	 " UVAST_FIELDSPEC, 
			count, entry->dstServiceNbr, entry->dstNodeNbr);
	printText(context);
}

static void	executeInfo(int tokenCount, char **tokens)
{
	if (tokenCount < 2)
	{
		printText("Information on what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		infoPlan(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		infoPlanRule(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		infoGroup(tokenCount, tokens);
		return;
	}

	if (strcmp(tokens[1], "grouprule") == 0)
	{
		infoGroupRule(tokenCount, tokens);
		return;
	}

	SYNTAX_ERROR;
}

static void	listPlans()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(BssPlan, plan);

	for (elt = sdr_list_first(sdr, (getBssConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BssPlan, plan, sdr_list_data(sdr, elt));
		printPlan(plan);
	}
}

static void	listRules(Object rules)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(BssRule, rule);

	for (elt = sdr_list_first(sdr, rules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BssRule, rule, sdr_list_data(sdr, elt));
		printRule(rule);
	}
}

static void	listGroups()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(IpnGroup, group);

	for (elt = sdr_list_first(sdr, (getBssConstants())->groups); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnGroup, group, sdr_list_data(sdr, elt));
		printGroup(group);
	}
}

void	listBssEntries()
{
	Sdr	sdr = getIonsdr();
	Object	elt;
		OBJ_POINTER(bssEntry, entry);
	int	i = 1;

	printText("Entry: <destination Service nbr> - <destination Node nbr>");
	for (elt = sdr_list_first(sdr, (getBssConstants())->bssList); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, bssEntry, entry, sdr_list_data(sdr, elt));
		printEntry(i, entry);
		i++;
	}
}

static void	executeList(int tokenCount, char **tokens)
{
	Sdr	sdr = getIonsdr();	
	uvast	nodeNbr;
	Object	planAddr;
	Object	elt;
		OBJ_POINTER(BssPlan, plan);

	if (tokenCount < 2)
	{
		printText("List what?");
		return;
	}

	if (strcmp(tokens[1], "plan") == 0)
	{
		listPlans();
		return;
	}

	if (strcmp(tokens[1], "planrule") == 0)
	{
		if (tokenCount < 3)
		{
			printText("Must specify node nbr for rules list.");
			return;
		}

		nodeNbr = strtouvast(tokens[2]);
		CHKVOID(sdr_begin_xn(sdr));
		bss_findPlan(nodeNbr, &planAddr, &elt);
		sdr_exit_xn(sdr);
		if (elt == 0)
		{
			printText("Unknown node.");
			return;
		}

		GET_OBJ_POINTER(getIonsdr(), BssPlan, plan, planAddr);
		printPlan(plan);
		listRules(plan->rules);
		return;
	}

	if (strcmp(tokens[1], "group") == 0)
	{
		listGroups();
		return;
	}

	if (strcmp(tokens[1], "entry") == 0)
	{			
		listBssEntries();		
		return;
	}

	SYNTAX_ERROR;
}

static void	switchEcho(int tokenCount, char **tokens)
{
	int	state;

	if (tokenCount < 2)
	{
		printText("Echo on or off?");
		return;
	}

	switch (*(tokens[1]))
	{
	case '0':
		state = 0;
		oK(_echo(&state));
		break;

	case '1':
		state = 1;
		oK(_echo(&state));
		break;

	default:
		printText("Echo on or off?");
	}
}

static int	processLine(char *line, int lineLength)
{
	int	tokenCount;
	char	*cursor;
	int	i;
	char	*tokens[9];
	char		buffer[80];

	tokenCount = 0;
	for (cursor = line, i = 0; i < 9; i++)
	{
		if (*cursor == '\0')
		{
			tokens[i] = NULL;
		}
		else
		{
			findToken(&cursor, &(tokens[i]));
			tokenCount++;
		}
	}

	if (tokenCount == 0)
	{
		return 0;
	}

	/*	Skip over any trailing whitespace.			*/

	while (isspace((int) *cursor))
	{
		cursor++;
	}

	/*	Make sure we've parsed everything.			*/

	if (*cursor != '\0')
	{
		printText("Too many tokens.");
		return 0;
	}

	/*	Have parsed the command.  Now execute it.		*/

	switch (*(tokens[0]))		/*	Command code.		*/
	{
		case 0:			/*	Empty line.		*/
		case '#':		/*	Comment.		*/
			return 0;

		case '?':
		case 'h':
			printUsage();
			return 0;

		case 'v':
			isprintf(buffer, sizeof buffer, "%s",
					IONVERSIONNUMBER);
			printText(buffer);
			return 0;

		case 'a':
			executeAdd(tokenCount, tokens);
			return 0;

		case 'c':
			executeChange(tokenCount, tokens);
			return 0;

		case 'd':
			executeDelete(tokenCount, tokens);
			return 0;

		case 'i':
			executeInfo(tokenCount, tokens);
			return 0;

		case 'l':
			executeList(tokenCount, tokens);
			return 0;

		case 'e':
			switchEcho(tokenCount, tokens);
			return 0;

		case 'q':
			return -1;	/*	End program.		*/

		default:
			printText("Invalid command.  Enter '?' for help.");
			return 0;
	}
}

static int	run_bssadmin(char *cmdFileName)
{
	int	cmdFile;
	char	line[256];
	int	len;

	if (bpAttach() < 0)
	{
		putErrmsg("bssadmin can't attach to BP", NULL);
		return -1;
	}

	if (ipnInit() < 0)
	{
		putErrmsg("bssadmin can't initialize routing database", NULL);
		return -1;
	}

	if (cmdFileName == NULL)	/*	Interactive.		*/
	{
#ifdef FSWLOGGER
		return 0;
#else
		cmdFile = fileno(stdin);
		isignal(SIGINT, handleQuit);
		while (1)
		{
			printf(": ");
			fflush(stdout);
			if (igets(cmdFile, line, sizeof line, &len) == NULL)
			{
				if (len == 0)
				{
					break;
				}

				putErrmsg("igets failed.", NULL);
				break;		/*	Out of loop.	*/
			}

			if (len == 0)
			{
				continue;
			}

			if (processLine(line, len))
			{
				break;		/*	Out of loop.	*/
			}
		}
#endif
	}
	else					/*	Scripted.	*/
	{
		cmdFile = open(cmdFileName, O_RDONLY, 0777);
		if (cmdFile < 0)
		{
			PERROR("Can't open command file");
		}
		else
		{
			while (1)
			{
				if (igets(cmdFile, line, sizeof line, &len)
						== NULL)
				{
					if (len == 0)
					{
						break;	/*	Loop.	*/
					}

					putErrmsg("igets failed.", NULL);
					break;		/*	Loop.	*/
				}

				if (len == 0
				|| line[0] == '#')	/*	Comment.*/
				{
					continue;
				}

				if (processLine(line, len))
				{
					break;	/*	Out of loop.	*/
				}
			}

			close(cmdFile);
		}
	}

	writeErrmsgMemos();
	printText("Stopping bssadmin.");
	ionDetach();
	return 0;
}

#if defined (VXWORKS) || defined (RTEMS)
int	bssadmin(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
	char	*cmdFileName = (char *) a1;
#else
int	main(int argc, char **argv)
{
	char	*cmdFileName = argc > 1 ? argv[1] : NULL;
#endif
	return run_bssadmin(cmdFileName);
}
