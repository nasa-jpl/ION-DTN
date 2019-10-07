/*

	ltpsec.c:	API for managing ION's LTP security database.

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
#include "ltpsec.h"

static void	writeRuleMessage(char* ruleMessage, uvast engineId, 
			unsigned char ciphersuiteNbr, char *keyName)
{
	char	buf[512];

	isprintf(buf, sizeof buf, "ltp engine id " UVAST_FIELDSPEC, engineId);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" ciphersuite_nbr %c ", ciphersuiteNbr);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			"key name '%.31s'", keyName);
	writeMemoNote(ruleMessage, buf);
}

/******************************************************************************
 *
 * \par Function Name: sec_findLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to find an LTP signing rule.
 * 		 There is a match if there is a rule in Sdr with the
 *		 same ltpEngineId. 
 *
 *		 Return 1 if there is a match, return 0 if not, return -1 on 
 *		 error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \param[out]	ruleAddr the pointer to the matched rule object
 *		eltp the pointer to the matched rule iterator
 * \par Notes:
 *****************************************************************************/

int	sec_findLtpXmitAuthRule(uvast ltpEngineId, Object *ruleAddr,
		Object * eltp)
{
	CHKERR(ruleAddr);
	CHKERR(eltp);

	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
	int	result = 0;
		OBJ_POINTER(LtpXmitAuthRule, rule);	

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}
	
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpXmitAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, *ruleAddr);
		if (rule->ltpEngineId == ltpEngineId)
		{
			result = 1;
			*eltp = elt;
			break;
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: sec_addLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to add an LTP signing rule. 
 *
 *		 Return 1 if added successfully, return 0 if not, return -1 on 
 *		 error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_addLtpXmitAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	LtpXmitAuthRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secdb);	
	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Don't expect a rule here already...*/
	if (sec_findLtpXmitAuthRule(ltpEngineId, &ruleObj, &elt) != 0)
	{
		writeRuleMessage("[?] This rule is already defined", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.ltpEngineId = ltpEngineId;
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(LtpXmitAuthRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", NULL);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->ltpXmitAuthRules, ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpXmitAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_updateLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to update an LTP signing rule. 
 *
 *		 Return 1 if updated successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_updateLtpXmitAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	Object		ruleObj;
	Object		elt;
	LtpXmitAuthRule	rule;

	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Need to have a rule to update it. */
	if (sec_findLtpXmitAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(LtpXmitAuthRule));
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpXmitAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_removeLtpXmitAuthRule 
 *
 * \par Purpose: This function is used to remove an LTP signing rule. 
 *
 *		 Return 1 if removed successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \par Notes:
 *****************************************************************************/

int	sec_removeLtpXmitAuthRule(uvast ltpEngineId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;

	/* Need to have a rule to delete it. */
	if (sec_findLtpXmitAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine.", 
				ltpEngineId, 0, "");
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_findLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to find an LTP authentication 
 *		 rule.  There is a match if there is a rule in Sdr with the
 *		 same ltpEngineId. 
 *
 *		 Return 1 if there is a match, return 0 if not, return -1 on 
 *		 error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \param[out]	ruleAddr the pointer to the matched rule object
 *		eltp the pointer to the matched rule iterator
 * \par Notes:
 *****************************************************************************/

int	sec_findLtpRecvAuthRule(uvast ltpEngineId, Object *ruleAddr,
		Object * eltp)
{
	CHKERR(ruleAddr);
	CHKERR(eltp);

	Sdr	sdr = getIonsdr();
	SecDB	*secdb = getSecConstants();
	Object	elt;
	int	result = 0;
		OBJ_POINTER(LtpRecvAuthRule, rule);	

	if (secdb == NULL)	/*	No security database declared.	*/
	{
		return 0;
	}
	
	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, secdb->ltpRecvAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		*ruleAddr = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpRecvAuthRule, rule, *ruleAddr);
		if (rule->ltpEngineId == ltpEngineId)
		{
			result = 1;
			*eltp = elt;
			break;
		}

		*ruleAddr = 0;
	}

	sdr_exit_xn(sdr);
	return result;
}

/******************************************************************************
 *
 * \par Function Name: sec_addLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to add an LTP authentication rule. 
 *
 *		Return 1 if added successfully, return 0 if not, return -1 on 
 *		error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_addLtpRecvAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	SecDB		*secdb = getSecConstants();
	LtpRecvAuthRule	rule;
	Object		ruleObj;
	Object		elt;

	CHKERR(secdb);	
	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Don't expect a rule here already...*/
	if (sec_findLtpRecvAuthRule(ltpEngineId, &ruleObj, &elt) != 0)
	{
		writeRuleMessage("[?] This rule is already defined", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	/*	Okay to add this rule to the database.			*/

	CHKERR(sdr_begin_xn(sdr));
	rule.ltpEngineId = ltpEngineId;
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	ruleObj = sdr_malloc(sdr, sizeof(LtpRecvAuthRule));
	if (ruleObj == 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't create rule.", NULL);
		return -1;
	}

	elt = sdr_list_insert_last(sdr, secdb->ltpRecvAuthRules, ruleObj);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpRecvAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't add rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_updateLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to update an LTP authentication 
 *		 rule. 
 *
 *		 Return 1 if updated successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 *		ciphersuiteNbr the ciphersuite number
 *		keyName the key name
 * \par Notes:
 *****************************************************************************/

int	sec_updateLtpRecvAuthRule(uvast ltpEngineId,
		unsigned char ciphersuiteNbr, char *keyName)
{
	int		cipher = ciphersuiteNbr;
	Sdr		sdr = getIonsdr();
	Object		ruleObj;
	Object		elt;
	LtpRecvAuthRule	rule;

	CHKERR(keyName);
	if (cipher != 0 && cipher != 1 && cipher != 255)
	{
		writeMemoNote("[?] Invalid ciphersuite", itoa(cipher));
		return 0;
	}

	if (cipher != 255)
	{
		if (*keyName == '\0')
		{
			writeMemo("[?] Key name is required unless ciphersuite \
is NULL (255).");
			return 0;
		}

		if (istrlen(keyName, 32) > 31)
		{
			writeMemoNote("[?] Key name is too long", keyName);
			return 0;
		}
	}

	/* Need to have a rule to update it. */
	if (sec_findLtpRecvAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine", 
				ltpEngineId, ciphersuiteNbr, keyName);
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &rule, ruleObj, sizeof(LtpRecvAuthRule));
	rule.ciphersuiteNbr = ciphersuiteNbr;
	istrcpy(rule.keyName, keyName, sizeof rule.keyName);
	sdr_write(sdr, ruleObj, (char *) &rule, sizeof(LtpRecvAuthRule));
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't update rule.", NULL);
		return -1;
	}

	return 1;
}

/******************************************************************************
 *
 * \par Function Name: sec_removeLtpRecvAuthRule 
 *
 * \par Purpose: This function is used to remove an LTP authentication 
 *		 rule. 
 *
 *		 Return 1 if removed successfully, return 0 if not, 
 *		 return -1 on error.
 *
 * \param[in]	ltpEngineId the LTP engine ID
 * \par Notes:
 *****************************************************************************/

int	sec_removeLtpRecvAuthRule(uvast ltpEngineId)
{
	Sdr	sdr = getIonsdr();
	Object	elt;
	Object	ruleObj;

	/* Need to have a rule to delete it. */
	if (sec_findLtpRecvAuthRule(ltpEngineId, &ruleObj, &elt) == 0)
	{
		writeRuleMessage("[?] No rule defined for this engine.", 
				ltpEngineId, 0, "");
		return 0;
	}

	CHKERR(sdr_begin_xn(sdr));
	sdr_list_delete(sdr, elt, NULL, NULL);
	sdr_free(sdr, ruleObj);
	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't remove rule.", NULL);
		return -1;
	}

	return 1;
}
