/*

	bpsec.c:	API for managing ION's BP security database.

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	Author:		Scott Burleigh, JPL
	Modifications:	TCSASSEMBLER, TopCoder

	Modification History:
	Date       Who     What
	9-24-13    TC      Added functions (find, add, remove, change) to
			   manage ltpXmitAuthRule and ltpRecvAuthRule
			   Updated secInitialize to initialize SecDB's 
			   ltpXmitAuthRule and ltpRecvAuthRule lists
			   Added writeRuleMessage to print rule-related message
	11-15-13  romanoTC Check for valid ciphersuite values (0,1,255)
	06-27-19  SB	   Extracted from ionsec.c
									*/
#include "bpsec.h"
#include "profiles.h"

static int	filterEid(char *outputEid, char *inputEid, int eidIsInRule)
{
	int	eidLength = istrlen(inputEid, MAX_SDRSTRING + 1);
	int	last = eidLength - 1;

	if (eidLength == 0 || eidLength > MAX_SDRSTRING)
	{
		writeMemoNote("[?] Invalid eid length", inputEid);
		return 0;
	}

	/*	Note: the '~' character is used internally to
	 *	indicate "all others" (wild card) because it's the
	 *	last printable ASCII character and therefore always
	 *	sorts last in any list.  If the user wants to use
	 *	'*' instead, we just change it silently.		*/

	memmove(outputEid, inputEid, eidLength);
	outputEid[eidLength] = '\0';
	if (outputEid[last] == '*')
	{
		outputEid[last] = '~';
	}

#ifndef ORIGINAL_BSP
	if (eidIsInRule && outputEid[last] != '~')
	{
		writeMemoNote("[?] Security rule not for entire node, rejected",
				outputEid);
		return 0;
	}
#endif

	return 1;
}

int	sec_activeKey(char *keyName)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBibRule, bibRule);
		OBJ_POINTER(BspBcbRule, bcbRule);

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBibRule, bibRule, ruleObj);
		if ((strncmp(bibRule->keyName, keyName, 32)) == 0)
		{
			sdr_exit_xn(sdr);
			return 1;
		}
	}

	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBcbRule, bcbRule, ruleObj);
		if ((strncmp(bcbRule->keyName, keyName, 32)) == 0)
		{
			sdr_exit_xn(sdr);
			return 1;
		}
	}

	sdr_exit_xn(sdr);
	return 0;
}

/******************************************************************************
 *
 *	Support for multiple flavors of Bundle Security Protocol.
 *
 *****************************************************************************/

/*		General BSP Support					*/

/******************************************************************************
 *
 * \par Function Name: eidsMatch 
 *
 * \par Purpose: This function accepts two string EIDs and determines if they
 *		match.  Significantly, each EID may contain an end-of-string
 *		wildcard character ("~"). For example, the two EIDs compared
 *		can be "ipn:1.~" and "ipn~".
 *
 * \retval int -- 1 - The EIDs matched, counting wildcards.
 *		0 - The EIDs did not match.
 *
 * \param[in]	firstEid	The first EID to compare. 
 *		firstEidLen	The length of the first EID string.
 *		secondEid	The second EID to compare.
 *		secondEidLen	The length of the second EID string.
 *
 * \par Notes:
 *****************************************************************************/

int	eidsMatch(char *firstEid, int firstEidLen, char *secondEid,
		int secondEidLen)
{
	int	result = 1;
	int	firstPos = -1;
	int	secondPos = -1;

	CHKZERO(firstEid);
	CHKZERO(firstEidLen > 0);
	CHKZERO(secondEid);
	CHKZERO(secondEidLen > 0);

	/*
	 * First, determine if (and the pos of) end-of-line wildcards.
	 */
	if (firstEid[firstEidLen - 1] == '~')
	{
		firstPos = firstEidLen - 1;
	}

	if (secondEid[secondEidLen - 1] == '~')
	{
		secondPos = secondEidLen-1;
	}

	/* If either or both EIDs are simply '~', this is a match. */
	if ((firstPos == 0) || (secondPos == 0))
	{
		result = 0;
	}

	/* If one/the other/both EIDs have wildcards, do a compare
	 * up to the shortest length. */
	else if ((firstPos > 0) && (secondPos > 0))
	{
		result = strncmp(firstEid, secondEid, MIN(firstPos, secondPos));
	}
	else if (firstPos > 0)
	{
		result = strncmp(firstEid, secondEid, MIN(firstPos,
				secondEidLen));
	}
	else if (secondPos > 0)
	{
		result = strncmp(firstEid, secondEid, MIN(firstEidLen,
				secondPos));
	}

	/* If no wildcards are used, do a regular compare. */
	else
	{
		result = strncmp(firstEid, secondEid, MIN(firstEidLen,
				secondEidLen));
	}

	/* If we have no differences, the EIDs must match. */
	return (result == 0);
}

/******************************************************************************
 *
 * \par Function Name: sec_clearBspRules 
 *
 * \par Purpose: Clears bsp rules configured on the node.  Rules are 
 *		identified by their security source and security destination
 *		and targeted BSP block type.  For EIDs, ~ is accepted as a
 *		end-of-string wildcard.
 *
 * \param[in]	srcEid  The security source of all rules being cleared. This
 *			may be "~" to match all security sources.
 *		destEid The security destination of all rules being cleared.
 *			This may be "~" to match all security destinations.
 *		blockType This is the BSP block of the rule to be cleared. 
 *			This is one of "2", "3", "4", or "~" to match all
 *			block types.
 * \par Notes:
 *****************************************************************************/

void	sec_clearBspRules(char *srcEid, char *destEid, char *blockType)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
	Object	temp;
	Object	ruleObj;
	int	rmCount;
	char	rmStr [5];
	int	srcEidLen;
	int	destEidLen;
	int	curEidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	CHKVOID(srcEid);
	CHKVOID(destEid);
	CHKVOID(blockType);
	CHKVOID(secdb);
	srcEidLen = istrlen(srcEid, SDRSTRING_BUFSZ);
	destEidLen = istrlen(destEid, SDRSTRING_BUFSZ);
#ifdef ORIGINAL_BSP
	if (blockType[0] == '~' || blockType[0] == '2')	/*	BAB	*/
	{
		/*	Remove matching authentication rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
		OBJ_POINTER(BspBabRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->securityDestEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->securityDestEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}

		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] authentication rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBspRules: failed deleting \
authentication rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBspRules: matching \
authentication rules cleared.");
		}
	}
#endif
	if (blockType[0] == '~' || blockType[0] == '3')	/*	PIB/BIB	*/
	{
		/*	Remove matching integrity rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
#ifdef ORIGINAL_BSP
		OBJ_POINTER(BspPibRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspPibRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspPibRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->securityDestEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->securityDestEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#else
		OBJ_POINTER(BspBibRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->destEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->destEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#endif
		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] integrity rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBspRules: failed deleting \
integrity rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBspRules: matching integrity \
rules cleared.");
		}
	}

	if (blockType[0] == '~' || blockType[0] == '4')	/*	PCB/BCB	*/
	{
		/*	Remove matching confidentiality rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
#ifdef ORIGINAL_BSP
		OBJ_POINTER(BspPcbRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspPcbRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspPcbRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->securityDestEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->securityDestEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#else
		OBJ_POINTER(BspBcbRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleObj);
			curEidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			temp = sdr_list_next(sdr, elt);
			if (eidsMatch(srcEid, srcEidLen, eidBuffer, curEidLen))
			{
				curEidLen = sdr_string_read(sdr, eidBuffer,
						rule->destEid);
				if (eidsMatch(destEid, destEidLen, eidBuffer,
						curEidLen))
				{
					sdr_list_delete(sdr, elt, NULL, NULL);
					sdr_free(sdr, rule->securitySrcEid);
					sdr_free(sdr, rule->destEid);
					sdr_free(sdr, ruleObj);
					rmCount++;
				}
			}
		}
#endif

		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] confidentiality rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBspRules: failed deleting \
confidentiality rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBspRules: matching \
confidentiality rules cleared.");
		}

	}
 
	return;
}

#ifdef ORIGINAL_BSP

void	sec_get_bspBabRule(char *srcEid, char *destEid, Object *ruleAddr,
		Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BspBabRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBabRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(srcEid);
	CHKVOID(destEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspBabRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBabRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->securityDestEid);
		/* If destinations match... */
		if (eidsMatch(eidBuffer, eidLen, destEid, strlen(destEid)))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, srcEid,
					strlen(srcEid)))
			{
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}
	}

	sdr_exit_xn(sdr);
}

/* 1 if found. 0 if not. and -1 on error. */
int	sec_findBspBabRule(char *srcEid, char *destEid, Object *ruleAddr,
		Object *eltp)
{
	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(srcEid, srcEid, 0) == 0)
	|| (filterEid(destEid, destEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBabRule(srcEid, destEid, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBspBabRule(char *srcEid, char *destEid, char *ciphersuiteName,
		char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	BspBabRule	rule;
	Object		ruleObj;
	Object		elt;
	Object		ruleAddr;
	int		last;

	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(srcEid, srcEid, 1) == 0)
	|| (filterEid(destEid, destEid, 1) == 0))
	{
		return 0;
	}

	/* Don't expect a rule here already...*/
	if (sec_findBspBabRule(srcEid, destEid, &ruleAddr, &elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", destEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	last = istrlen(srcEid, 32) - 1;
	if (*(srcEid + last) != '~')
	{
		writeMemoNote("[?] Warning: this BAB rule authenticates only a \
single sender endpoint, not all endpoints on the sending node", srcEid);
	}

	last = istrlen(destEid, 32) - 1;
	if (*(destEid + last) != '~')
	{
		writeMemoNote("[?] Warning: this BAB rule authenticates only a \
single receiver endpoint, not all endpoints on the receiving node", destEid);
	}

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, srcEid);
	rule.securityDestEid = sdr_string_create(sdr, destEid);
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBabRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", destEid);
		return -1;
	}

	/*	Note that the rules list is not sorted, as there
	 *	is no obviously correct sorting order: rules are
	 *	symmetrical, specifying both security source and
	 *	security destination, and the local node might be
	 *	neither.						*/

	elt = sdr_list_insert_last(sdr, secdb->bspBabRules,ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBabRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", destEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBabRule(char *srcEid, char *destEid, char *ciphersuiteName,
		char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspBabRule	rule;

	CHKERR(srcEid);
	CHKERR(destEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (strlen(keyName) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(srcEid, srcEid, 1) == 0)
	|| (filterEid(destEid, destEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspBabRule(srcEid, destEid, &ruleObj, &elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				destEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspBabRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBabRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", destEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBabRule(char *srcEid, char *destEid)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBabRule, rule);

	CHKERR(srcEid);
	CHKERR(destEid);
	if ((filterEid(srcEid, srcEid, 1) == 0)
	|| (filterEid(destEid, destEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspBabRule(srcEid, destEid, &ruleObj, &elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				destEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->securityDestEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", destEid);
		return -1;
	}

	return 1;
}

/* Payload Integrity Block Support */

int	sec_get_bspPibTxRule(char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	return sec_get_bspPibRule("~", secDestEid, blockTypeNbr, ruleAddr,
			eltp);
}

int	sec_get_bspPibRxRule(char *secSrcEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	return sec_get_bspPibRule(secSrcEid, "~", blockTypeNbr, ruleAddr, eltp);
}

int	sec_get_bspPibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BspPibRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 0;

	/*	This function determines the relevant BspPibRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspPibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspPibRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->securityDestEid);

		/* If destinations match... */
		if ((rule->blockTypeNbr == blockTypeNbr)
		&& (eidsMatch(eidBuffer, eidLen, secDestEid,
				strlen(secDestEid))))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, secSrcEid,
					strlen(secSrcEid)) == 1)
			{
				result = 1;
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/* 1 if found. 0 if not. -1 on error. */
int	sec_findBspPibRule(char *secSrcEid, char *secDestEid, int BlockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspPibRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	return sec_get_bspPibRule(secSrcEid, secDestEid, BlockTypeNbr,
			ruleAddr, eltp);
}

int	sec_addBspPibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	BspPibRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}


	/* Don't expect a rule here already...*/
	if (sec_findBspPibRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.securityDestEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspPibRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspPibRules,ruleObj);

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspPibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspPibRule	rule;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (istrlen(ciphersuiteName, 32) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (istrlen(keyName, 32) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspPibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspPibRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspPibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspPibRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspPibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspPibRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->securityDestEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_get_bspPcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BspPcbRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];
	int	result = 0;

	/*	This function determines the relevant BspPcbRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspPcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspPcbRule, rule, *ruleAddr);
		eidLen = sdr_string_read(sdr, eidBuffer, rule->securityDestEid);

		/* If destinations match... */
		if ((rule->blockTypeNbr == blockTypeNbr)
		&& (eidsMatch(eidBuffer, eidLen, secDestEid,
				strlen(secDestEid))))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, secSrcEid,
					strlen(secSrcEid)) == 1)
			{
				result = 1;
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching. */
			}
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/* 1 if found. 0 if not. -1 on error. */
int	sec_findBspPcbRule(char *secSrcEid, char *secDestEid, int BlockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspPcbRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	return sec_get_bspPcbRule(secSrcEid, secDestEid, BlockTypeNbr,
			ruleAddr, eltp);
}

int	sec_addBspPcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	BspPcbRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	if (strlen(ciphersuiteName) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}
	
	if (strlen(keyName) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Don't expect a rule here already...*/
	if (sec_findBspPcbRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined.", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.securityDestEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspPcbRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspPcbRules,ruleObj);

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspPcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	Object		elt;
	Object		ruleObj;
	BspPcbRule	rule;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	if (strlen(ciphersuiteName) > 31)
	{
		writeMemoNote("[?] Invalid ciphersuiteName", ciphersuiteName);
		return 0;
	}

	if (strlen(keyName) > 31)
	{
		writeMemoNote("[?] Invalid keyName", keyName);
		return 0;
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspPcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspPcbRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspPcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspPcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspPcbRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspPcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspPcbRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->securityDestEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

#else		/*	Streamlined Bundle Security Protocol		*/

/*		Block Integrity Block Support				*/

void	sec_get_bspBibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BspBibRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBibRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(secSrcEid);
	CHKVOID(secDestEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	if (blockTypeNbr == 0)
	{
		return;		/*	Cannot match any valid rule.	*/
	}

	CHKVOID(sdr_begin_xn(sdr));

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBibRule, rule, *ruleAddr);

		if (rule->blockTypeNbr != blockTypeNbr)
		{
			continue;	/*	Wrong target blk type.	*/
		}

		eidLen = sdr_string_read(sdr, eidBuffer, rule->destEid);

		/* If destinations match... */
		if (eidsMatch(eidBuffer, eidLen, secDestEid,
				strlen(secDestEid)))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, secSrcEid,
					strlen(secSrcEid)))
			{
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching.	*/
			}
		}
	}

	sdr_exit_xn(sdr);
}

int	sec_findBspBibRule(char *secSrcEid, char *secDestEid, int blkType,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspBibRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBibRule(secSrcEid, secDestEid, blkType, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBspBibRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	BspBibRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	csNameLen = istrlen(ciphersuiteName, 32);
       	if (csNameLen > 31)
	{
		writeMemoNote("[?] Ciphersuite name too long", ciphersuiteName);
		return 0;
	}

	keyNameLen = istrlen(keyName, 32);
	if (keyNameLen > 31)
	{
		writeMemoNote("[?] Key name too long", keyName);
		return 0;
	}

	if (csNameLen == 0)
	{
		if (keyNameLen != 0)
		{
			writeMemoNote("[?] Ciphersuite name omitted", keyName);
			return 0;
		}
	}
	else
	{
		if (keyNameLen == 0)
		{
			writeMemoNote("[?] Key name omitted", ciphersuiteName);
			return 0;
		}

		if (sec_get_key(keyName, &bufferLength, buffer) < 1)
		{
			writeMemoNote("[?] Invalid key name", keyName);
			return 0;
		}

		if (get_bib_prof_by_name(ciphersuiteName) == NULL)
		{
			writeMemoNote("[?] Not a known BIB ciphersuite",
					ciphersuiteName);
			return 0;
		}
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBibRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.destEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBibRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspBibRules,ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	Object		elt;
	Object		ruleObj;
	BspBibRule	rule;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	csNameLen = istrlen(ciphersuiteName, 32);
       	if (csNameLen > 31)
	{
		writeMemoNote("[?] Ciphersuite name too long", ciphersuiteName);
		return 0;
	}

	keyNameLen = istrlen(keyName, 32);
	if (keyNameLen > 31)
	{
		writeMemoNote("[?] Key name too long", keyName);
		return 0;
	}

	if (csNameLen == 0)
	{
		if (keyNameLen != 0)
		{
			writeMemoNote("[?] Ciphersuite name omitted", keyName);
			return 0;
		}
	}
	else
	{
		if (keyNameLen == 0)
		{
			writeMemoNote("[?] Key name omitted", ciphersuiteName);
			return 0;
		}

		if (sec_get_key(keyName, &bufferLength, buffer) < 1)
		{
			writeMemoNote("[?] Invalid key name", keyName);
			return 0;
		}

		if (get_bib_prof_by_name(ciphersuiteName) == NULL)
		{
			writeMemoNote("[?] Not a known BIB ciphersuite",
					ciphersuiteName);
			return 0;
		}
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspBibRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBibRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBspBibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->destEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

/*		Block Confidentiality Block Support			*/

void	sec_get_bspBcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BspBcbRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BspBcbRule for
	 *	the specified receiving endpoint, if any.  Wild card
	 *	match is okay.						*/

	CHKVOID(secSrcEid);
	CHKVOID(secDestEid);
	CHKVOID(ruleAddr);
	CHKVOID(eltp);
	*eltp = 0;		/*	Default: rule not found.	*/
	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BspBcbRule, rule, *ruleAddr);
		if (blockTypeNbr != 0 && rule->blockTypeNbr != blockTypeNbr)
		{
			continue;	/*	Wrong target blk type.	*/
		}

		eidLen = sdr_string_read(sdr, eidBuffer, rule->destEid);
		/* If destinations match... */
		if ((rule->blockTypeNbr == blockTypeNbr)
		&& (eidsMatch(eidBuffer, eidLen, secDestEid,
				strlen(secDestEid))))
		{
			eidLen = sdr_string_read(sdr, eidBuffer,
					rule->securitySrcEid);
			/* If sources match ... */
			if (eidsMatch(eidBuffer, eidLen, secSrcEid,
					strlen(secSrcEid)) == 1)
			{
				*eltp = elt;	/*	Exact match.	*/
				break;		/*	Stop searching. */
			}
		}
	}

	sdr_exit_xn(sdr);
}

int	sec_findBspBcbRule(char *secSrcEid, char *secDestEid, int blkType,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BspBcbRule for the specified
	 *	endpoint, if any.					*/

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ruleAddr);
	CHKERR(eltp);
	*eltp = 0;
	if ((filterEid(secSrcEid, secSrcEid, 0) == 0)
	|| (filterEid(secDestEid, secDestEid, 0) == 0))
	{
		return -1;
	}

	sec_get_bspBcbRule(secSrcEid, secDestEid, blkType, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBspBcbRule(char *secSrcEid, char *secDestEid, int blockTypeNbr,
		char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	BspBcbRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	CHKERR(secdb);
	csNameLen = istrlen(ciphersuiteName, 32);
       	if (csNameLen > 31)
	{
		writeMemoNote("[?] Ciphersuite name too long", ciphersuiteName);
		return 0;
	}

	keyNameLen = istrlen(keyName, 32);
	if (keyNameLen > 31)
	{
		writeMemoNote("[?] Key name too long", keyName);
		return 0;
	}

	if (csNameLen == 0)
	{
		if (keyNameLen != 0)
		{
			writeMemoNote("[?] Ciphersuite name omitted", keyName);
			return 0;
		}
	}
	else
	{
		if (keyNameLen == 0)
		{
			writeMemoNote("[?] Key name omitted", ciphersuiteName);
			return 0;
		}

		if (sec_get_key(keyName, &bufferLength, buffer) < 1)
		{
			writeMemoNote("[?] Invalid key name", keyName);
			return 0;
		}

		if (get_bcb_prof_by_name(ciphersuiteName) == NULL)
		{
			writeMemoNote("[?] Not a known BCB ciphersuite",
					ciphersuiteName);
			return 0;
		}
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Don't expect a rule here already...*/
	if (sec_findBspBcbRule(secSrcEid, secDestEid, blockTypeNbr, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined.", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.destEid = sdr_string_create(sdr, secDestEid);
	rule.blockTypeNbr = blockTypeNbr;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BspBcbRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bspBcbRules,ruleObj);

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBspBcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	Object		elt;
	Object		ruleObj;
	BspBcbRule	rule;

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	CHKERR(ciphersuiteName);
	CHKERR(keyName);
	csNameLen = istrlen(ciphersuiteName, 32);
       	if (csNameLen > 31)
	{
		writeMemoNote("[?] Ciphersuite name too long", ciphersuiteName);
		return 0;
	}

	keyNameLen = istrlen(keyName, 32);
	if (keyNameLen > 31)
	{
		writeMemoNote("[?] Key name too long", keyName);
		return 0;
	}

	if (csNameLen == 0)
	{
		if (keyNameLen != 0)
		{
			writeMemoNote("[?] Ciphersuite name omitted", keyName);
			return 0;
		}
	}
	else
	{
		if (keyNameLen == 0)
		{
			writeMemoNote("[?] Key name omitted", ciphersuiteName);
			return 0;
		}

		if (sec_get_key(keyName, &bufferLength, buffer) < 1)
		{
			writeMemoNote("[?] Invalid key name", keyName);
			return 0;
		}

		if (get_bcb_prof_by_name(ciphersuiteName) == NULL)
		{
			writeMemoNote("[?] Not a known BCB ciphersuite",
					ciphersuiteName);
			return 0;
		}
	}

	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to update it. */
	if (sec_findBspBcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BspBcbRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BspBcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBspBcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BspBcbRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBspBcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleObj);
	sdr_free(sdr, rule->securitySrcEid);
	sdr_free(sdr, rule->destEid);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", secDestEid);
		return -1;
	}

	return 1;
}

/*		General SBSP Support					*/

Object sec_get_bspBibRuleList()
{
	SecDB	*secdb = getSecConstants();

	if(secdb == NULL)
	{
		return 0;
	}

	return secdb->bspBibRules;
}

Object sec_get_bspBcbRuleList()
{
	SecDB	*secdb = getSecConstants();

	if(secdb == NULL)
	{
		return 0;
	}

	return secdb->bspBcbRules;
}

/* Size is the maximum size of a key name. */
int	sec_get_sbspNumKeys(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	int	result = 0;

	CHKERR(size);
	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->keys);

	/* TODO: This should be a #define. */
	*size = 32;
	sdr_exit_xn(sdr);

	return result;
}

/*
 * Generates NULL-terminated, CSV of key names
 * in the PRE-ALLOCATED buffer of given length.
 */
void sec_get_sbspKeys(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
		OBJ_POINTER(SecKey, key);
	Object	elt;
	Object	obj;

	char *cursor = NULL;
	int idx = 0;
	int key_len = 0;

	CHKVOID(buffer);

	memset(buffer, 0, length);
	cursor = buffer;

	CHKVOID(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->keys); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, SecKey, key, obj);

		if ((key != NULL) && ((key_len = strlen(key->name)) > 0))
		{
			/* Make sure there is room in the buffer to
			 * hold the key name.
			 */
			if ((idx + key_len + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_cancel_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, key->name, key_len);
			cursor += key_len;
			*cursor = ',';
			cursor += 1;
			idx += key_len + 1;
		}
	}

	/*	If we put anything in the buffer, there is now
	 *	a trailing ",". Replace it with a NULL terminator.	*/

	if (buffer != cursor)
	{
		cursor--;
		cursor[0] = '\0';
	}

	sdr_end_xn(sdr);
}

int  sec_get_sbspNumCSNames(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	int	result = 0;

	CHKERR(size);

	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->bspBibRules);
	result += sdr_list_length(sdr, secdb->bspBcbRules);

	/* TODO: This should be a #define. */
	*size = 32;
	sdr_exit_xn(sdr);

	return result;
}

void sec_get_sbspCSNames(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
		OBJ_POINTER(BspBibRule, bibRule);
		OBJ_POINTER(BspBcbRule, bcbRule);
	Object	elt;
	Object	obj;
	char	*cursor = NULL;
	int	idx = 0;
	int	size = 0;

	CHKVOID(buffer);
	memset(buffer, 0, length);
	CHKVOID(sdr_begin_xn(sdr));
	cursor = buffer;

	/* Walk through the BIB rules and gather ciphersuite names. */

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBibRule, bibRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */
		size = strlen(bibRule->ciphersuiteName);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, bibRule->ciphersuiteName, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}

	/* Walk through the BCB rules and gather ciphersuite names. */
	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBcbRule, bcbRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */
		size = strlen(bcbRule->ciphersuiteName);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, bcbRule->ciphersuiteName, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}


	/*	If we put anything in the buffer, there is now
	 *	a trailing ",". Replace it with a NULL terminator.	*/

	if (buffer != cursor)
	{
		cursor--;
		cursor[0] = '\0';
	}

	sdr_exit_xn(sdr);
}

int  sec_get_sbspNumSrcEIDs(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	int	result = 0;

	CHKERR(size);

	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->bspBibRules);
	result += sdr_list_length(sdr, secdb->bspBcbRules);

	/* TODO: This should be a #define. */
	*size = SDRSTRING_BUFSZ;
	sdr_exit_xn(sdr);

	return result;
}

void sec_get_sbspSrcEIDs(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
		OBJ_POINTER(BspBibRule, bibRule);
		OBJ_POINTER(BspBcbRule, bcbRule);
	Object	elt;
	Object	obj;
	char	*cursor = NULL;
	int	idx = 0;
	int	size = 0;
	char	eidBuffer[SDRSTRING_BUFSZ];

	CHKVOID(buffer);
	memset(buffer, 0, length);
	CHKVOID(sdr_begin_xn(sdr));
	cursor = buffer;

	/* Walk through the BIB rules and gather ciphersuite names. */

	for (elt = sdr_list_first(sdr, secdb->bspBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBibRule, bibRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */

		size = sdr_string_read(sdr, eidBuffer, bibRule->securitySrcEid);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, eidBuffer, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}

	/* Walk through the BCB rules and gather ciphersuite names. */
	for (elt = sdr_list_first(sdr, secdb->bspBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BspBcbRule, bcbRule, obj);

		/* Make sure there is room in the buffer to
		 * hold the ciphersuite name.
		 */
		size = sdr_string_read(sdr, eidBuffer, bcbRule->securitySrcEid);
		if ((size > 0) && (size <= 32))
		{
			if ((idx + size + 1) > length)
			{
				memset(buffer, 0, length);
				sdr_exit_xn(sdr);
				return;
			}

			/* Copy current key name into the buffer. */
			memcpy(cursor, eidBuffer, size);
			cursor += size;
			*cursor = ',';
			cursor += 1;
			idx += size + 1;
		}
	}

	/*	If we put anything in the buffer, there is now
	 *	a trailing ",". Replace it with a NULL terminator.	*/

	if (buffer != cursor)
	{
		cursor--;
		cursor[0] = '\0';
	}

	sdr_exit_xn(sdr);
}

#endif		/*	ORIGINAL_BSP					*/
