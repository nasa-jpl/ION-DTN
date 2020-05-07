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
	10-05-19  SB	   Migrated from SBSP to BPsec.
									*/
#include "bpP.h"
#include "bpsec.h"
#include "bpsec_util.h"
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

	if (eidIsInRule && outputEid[last] != '~')
	{
		writeMemoNote("[?] Security rule not for entire node, rejected",
				outputEid);
		return 0;
	}

	return 1;
}

int	sec_activeKey(char *keyName)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BPsecBibRule, bibRule);
		OBJ_POINTER(BPsecBcbRule, bcbRule);

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->bpsecBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BPsecBibRule, bibRule, ruleObj);
		if ((strncmp(bibRule->keyName, keyName, 32)) == 0)
		{
			sdr_exit_xn(sdr);
			return 1;
		}
	}

	for (elt = sdr_list_first(sdr, secdb->bpsecBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		ruleObj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BPsecBcbRule, bcbRule, ruleObj);
		if ((strncmp(bcbRule->keyName, keyName, 32)) == 0)
		{
			sdr_exit_xn(sdr);
			return 1;
		}
	}

	sdr_exit_xn(sdr);
	return 0;
}

/*		General BPsec Support					*/

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
 * \par Function Name: sec_clearBPsecRules 
 *
 * \par Purpose: Clears BPsec rules configured on the node.  Rules are 
 *		identified by their security source and security destination
 *		and targeted BPsec block type.  For EIDs, ~ is accepted as an
 *		end-of-string wildcard.
 *
 * \param[in]	srcEid  The security source of all rules being cleared. This
 *			may be "~" to match all security sources.
 *		destEid The security destination of all rules being cleared.
 *			This may be "~" to match all security destinations.
 *		blockType This points at the BPsec block of the rule to be
 *			cleared.  *blockType may be BlockIntegrity or
 *			BlockConfidentiality, or blockType may be NULL
 *			to match all BPsec block types.
 * \par Notes:
 *****************************************************************************/

void	sec_clearBPsecRules(char *srcEid, char *destEid, BpBlockType *blockType)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
	Object	temp;
	Object	ruleObj;
	int	rmCount;
	char	rmStr[5];
	int	srcEidLen;
	int	destEidLen;
	int	curEidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	CHKVOID(srcEid);
	CHKVOID(destEid);
	CHKVOID(secdb);
	srcEidLen = istrlen(srcEid, SDRSTRING_BUFSZ);
	destEidLen = istrlen(destEid, SDRSTRING_BUFSZ);
	if (blockType == NULL || *blockType == BlockIntegrityBlk)
	{
		/*	Remove matching integrity rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
		OBJ_POINTER(BPsecBibRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bpsecBibRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BPsecBibRule, rule, ruleObj);
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

		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] integrity rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBPsecRules: failed deleting \
integrity rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBPsecRules: matching integrity \
rules cleared.");
		}
	}

	if (blockType == NULL || *blockType == BlockConfidentialityBlk)
	{
		/*	Remove matching confidentiality rules.		*/

		rmCount = 0;
		CHKVOID(sdr_begin_xn(sdr));
		OBJ_POINTER(BPsecBcbRule, rule);
		for (elt = sdr_list_first(sdr, secdb->bpsecBcbRules); elt;
				elt = temp)
		{
			ruleObj = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, ruleObj);
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

		isprintf(rmStr, 5, "%d", rmCount);
		writeMemoNote("[i] confidentiality rules removed", rmStr);
		if (sdr_end_xn(sdr) < 0)
		{
			writeMemo("[?] sec_clearBPsecRules: failed deleting \
confidentiality rules.");
		}
		else
		{
			writeMemo("[i] sec_clearBPsecRules: matching \
confidentiality rules cleared.");
		}
	}
 
	return;
}

/*		Block Integrity Block Support				*/

void	sec_get_bpsecBibRule(char *secSrcEid, char *secDestEid,
		BpBlockType *blockType, Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BPsecBibRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BPsecBibRule for
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
	for (elt = sdr_list_first(sdr, secdb->bpsecBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BPsecBibRule, rule, *ruleAddr);
		if (blockType && rule->blockType != *blockType)
		{
			continue;	/*	Wrong target blk type.	*/
		}

		/*	Block type matches.				*/

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

int	sec_findBPsecBibRule(char *secSrcEid, char *secDestEid, int blkType,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BPsecBibRule for the specified
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

	sec_get_bpsecBibRule(secSrcEid, secDestEid, &blkType, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBPsecBibRule(char *secSrcEid, char *secDestEid,
		BpBlockType blockType, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	BPsecBibRule	rule;
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

	if (sec_findBPsecBibRule(secSrcEid, secDestEid, blockType, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.destEid = sdr_string_create(sdr, secDestEid);
	rule.blockType = blockType;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BPsecBibRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bpsecBibRules,ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BPsecBibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBPsecBibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	Object		elt;
	Object		ruleObj;
	BPsecBibRule	rule;

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

	if (sec_findBPsecBibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BPsecBibRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BPsecBibRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBPsecBibRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BPsecBibRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	if (sec_findBPsecBibRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BPsecBibRule, rule, ruleObj);
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

void	sec_get_bpsecBcbRule(char *secSrcEid, char *secDestEid,
	       	BpBlockType *blockType, Object *ruleAddr, Object *eltp)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
		OBJ_POINTER(BPsecBcbRule, rule);
	int	eidLen;
	char	eidBuffer[SDRSTRING_BUFSZ];

	/*	This function determines the relevant BPsecBcbRule for
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
	for (elt = sdr_list_first(sdr, secdb->bpsecBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, *ruleAddr);
		if (blockType && rule->blockType != *blockType)
		{
			continue;	/*	Wrong target blk type.	*/
		}

		/*	Block type matches.				*/

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
				break;		/*	Stop searching. */
			}
		}
	}

	sdr_exit_xn(sdr);
}

int	sec_findBPsecBcbRule(char *secSrcEid, char *secDestEid, int blkType,
		Object *ruleAddr, Object *eltp)
{
	/*	This function finds the BPsecBcbRule for the specified
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

	sec_get_bpsecBcbRule(secSrcEid, secDestEid, &blkType, ruleAddr, eltp);
	return (*eltp != 0);
}

int	sec_addBPsecBcbRule(char *secSrcEid, char *secDestEid,
		BpBlockType blockType, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	BPsecBcbRule	rule;
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
	if (sec_findBPsecBcbRule(secSrcEid, secDestEid, blockType, &ruleObj,
			&elt) != 0)
	{
		writeMemoNote("[?] This rule is already defined.", secDestEid);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.securitySrcEid = sdr_string_create(sdr, secSrcEid);
	rule.destEid = sdr_string_create(sdr, secDestEid);
	rule.blockType = blockType;
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(BPsecBcbRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", secDestEid);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->bpsecBcbRules,ruleObj);

	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BPsecBcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_updateBPsecBcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr, char *ciphersuiteName, char *keyName)
{
	Sdr		sdr = getIonsdr();
	int		csNameLen;
	int		keyNameLen;
	char		buffer[256];
	int		bufferLength = sizeof buffer;
	Object		elt;
	Object		ruleObj;
	BPsecBcbRule	rule;

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
	if (sec_findBPsecBcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(BPsecBcbRule));
	istrcpy(rule.ciphersuiteName, ciphersuiteName,
			sizeof rule.ciphersuiteName);
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(BPsecBcbRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", secDestEid);
		return -1;
	}

	return 1;
}

int	sec_removeBPsecBcbRule(char *secSrcEid, char *secDestEid,
		int BlockTypeNbr)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;
		OBJ_POINTER(BPsecBcbRule, rule);

	CHKERR(secSrcEid);
	CHKERR(secDestEid);
	if ((filterEid(secSrcEid, secSrcEid, 1) == 0)
	|| (filterEid(secDestEid, secDestEid, 1) == 0))
	{
		return 0;
	}

	/* Need to have a rule to delete it. */
	if (sec_findBPsecBcbRule(secSrcEid, secDestEid, BlockTypeNbr, &ruleObj,
			&elt) == 0)
	{
		writeMemoNote("[?] No rule defined for this endpoint.",
				secDestEid);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, ruleObj);
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

/*		General bpsec support					*/

Object sec_get_bpsecBibRuleList()
{
	SecDB	*secdb = getSecConstants();

	if(secdb == NULL)
	{
		return 0;
	}

	return secdb->bpsecBibRules;
}

Object sec_get_bpsecBcbRuleList()
{
	SecDB	*secdb = getSecConstants();

	if(secdb == NULL)
	{
		return 0;
	}

	return secdb->bpsecBcbRules;
}

/* Size is the maximum size of a key name. */
int	sec_get_bpsecNumKeys(int *size)
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
void sec_get_bpsecKeys(char *buffer, int length)
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

int  sec_get_bpsecNumCSNames(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	int	result = 0;

	CHKERR(size);

	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->bpsecBibRules);
	result += sdr_list_length(sdr, secdb->bpsecBcbRules);

	/* TODO: This should be a #define. */
	*size = 32;
	sdr_exit_xn(sdr);

	return result;
}

void sec_get_bpsecCSNames(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
		OBJ_POINTER(BPsecBibRule, bibRule);
		OBJ_POINTER(BPsecBcbRule, bcbRule);
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

	for (elt = sdr_list_first(sdr, secdb->bpsecBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BPsecBibRule, bibRule, obj);

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
	for (elt = sdr_list_first(sdr, secdb->bpsecBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BPsecBcbRule, bcbRule, obj);

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

int  sec_get_bpsecNumSrcEIDs(int *size)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	int	result = 0;

	CHKERR(size);

	CHKERR(sdr_begin_xn(sdr));
	result = sdr_list_length(sdr, secdb->bpsecBibRules);
	result += sdr_list_length(sdr, secdb->bpsecBcbRules);

	/* TODO: This should be a #define. */
	*size = SDRSTRING_BUFSZ;
	sdr_exit_xn(sdr);

	return result;
}

void sec_get_bpsecSrcEIDs(char *buffer, int length)
{
	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
		OBJ_POINTER(BPsecBibRule, bibRule);
		OBJ_POINTER(BPsecBcbRule, bcbRule);
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

	for (elt = sdr_list_first(sdr, secdb->bpsecBibRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BPsecBibRule, bibRule, obj);

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
	for (elt = sdr_list_first(sdr, secdb->bpsecBcbRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);

		GET_OBJ_POINTER(sdr, BPsecBcbRule, bcbRule, obj);

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

