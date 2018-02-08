/****************************************************************************
 **
 ** File Name: adm_ionsec_admin_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2018-01-08  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "ion.h"

/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_ionsec_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/***
static void	printText(char *text)
{
	if (_echo(NULL))
	{
		writeMemo(text);
	}

	PUTS(text);
}

static void	printBspBabRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BspBabRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BspBabRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->senderEid);
	sdr_string_read(sdr, destEidBuf, rule->receiverEid);
	isprintf(buf, sizeof buf, "rule sender eid '%.255s' receiver eid \
'%.255s' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void	printBspBibRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(BspBibRule, rule);
	char	srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
	char	buf[512];

	GET_OBJ_POINTER(sdr, BspBibRule, rule, ruleAddr);
	sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
	sdr_string_read(sdr, destEidBuf, rule->destEid);
	isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockTypeNbr, rule->ciphersuiteName, rule->keyName);
	printText(buf);
}

static void     printBspBcbRule(Object ruleAddr)
{
        Sdr     sdr = getIonsdr();
                OBJ_POINTER(BspBcbRule, rule);
        char    srcEidBuf[SDRSTRING_BUFSZ], destEidBuf[SDRSTRING_BUFSZ];
        char    buf[512];

        GET_OBJ_POINTER(sdr, BspBcbRule, rule, ruleAddr);
        sdr_string_read(sdr, srcEidBuf, rule->securitySrcEid);
        sdr_string_read(sdr, destEidBuf, rule->destEid);
        isprintf(buf, sizeof buf, "rule src eid '%.255s' dest eid '%.255s' \
type '%d' ciphersuite '%.31s' key name '%.31s'", srcEidBuf, destEidBuf,
		rule->blockTypeNbr, rule->ciphersuiteName, rule->keyName);
        printText(buf);
}

static void	printLtpRecvAuthRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpRecvAuthRule, rule);
	char	buf[512];
	int	temp;

	GET_OBJ_POINTER(sdr, LtpRecvAuthRule, rule, ruleAddr);	
	temp = rule->ciphersuiteNbr;
	isprintf(buf, sizeof buf, "LTPrecv rule: engine id " UVAST_FIELDSPEC,
			rule->ltpEngineId);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" ciphersuite_nbr %d", temp);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" key name '%.31s'", rule->keyName);
	printText(buf);
}

static void	printLtpXmitAuthRule(Object ruleAddr)
{
	Sdr	sdr = getIonsdr();
		OBJ_POINTER(LtpXmitAuthRule, rule);
	char	buf[512];
	int	temp;

	GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, ruleAddr);	
	temp = rule->ciphersuiteNbr;
	isprintf(buf, sizeof buf, "LTPxmit rule: engine id " UVAST_FIELDSPEC,
			rule->ltpEngineId);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" ciphersuite_nbr %d", temp);
	isprintf(buf + strlen(buf), sizeof buf - strlen(buf),
			" key name '%.31s'", rule->keyName);
	printText(buf);
}

***/

/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ionsec_admin_setup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void adm_ionsec_admin_cleanup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


value_t adm_ionsec_admin_meta_name(tdc_t params)
{
	return val_from_str("adm_ionsec_admin");
}


value_t adm_ionsec_admin_meta_namespace(tdc_t params)
{
	return val_from_str("arn:DTN:ionsec_admin");
}


value_t adm_ionsec_admin_meta_version(tdc_t params)
{
	return val_from_str("V0.0");
}


value_t adm_ionsec_admin_meta_organization(tdc_t params)
{
	return val_from_str("JHUAPL");
}


/* Table Functions */


/*
 * This table lists all key names in the security policy database.
 */

table_t* adm_ionsec_admin_tbl_keys()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "key_name", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all LTP segment authentication rules in the security policy database.
 */

table_t* adm_ionsec_admin_tbl_ltp_rx_rules()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "ltp_engine_id", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "ciphersuite_nbr", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "key_name", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_ltp_rx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_ltp_rx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all LTP segment signing rules in the security policy database.
 */

table_t* adm_ionsec_admin_tbl_ltp_tx_rules()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "ltp_engine_id", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "ciphersuite_nbr", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "key_name", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_ltp_tx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_ltp_tx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */

/* Control Functions */

/*
 * This control adds a named key value to the security policy database. The content of file_name is tak
 * en as the value of the key. Named keys can be referenced by other elements of the security policy da
 * tabase.
 */
tdc_t* adm_ionsec_admin_ctrl_key_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_key_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *key_name;
	char *file_name = 0;
	int8_t 	success = 0;

	key_name = adm_extract_string(params,0,&success);

	if(success)
	{
		file_name = adm_extract_string(params,1,&success);
	}

	if(success)
	{
		sec_addKey(key_name, file_name);
		*status = CTRL_SUCCESS;
	}


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_key_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the value of the named key, obtaining the new key value from the content of fil
 * e_name.
 */
tdc_t* adm_ionsec_admin_ctrl_key_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_key_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *key_name;
	char *file_name = 0;
	int8_t 	success = 0;

	key_name = adm_extract_string(params,0,&success);

	if(success)
	{
		file_name = adm_extract_string(params,1,&success);
	}

	if(success)
	{
		sec_updateKey(key_name, file_name);
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_key_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the key identified by name.
 */
tdc_t* adm_ionsec_admin_ctrl_key_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_key_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *key_name;
	int8_t 	success = 0;

	key_name = adm_extract_string(params,0,&success);

	if(success)
	{
		sec_removeKey(key_name);
		*status = CTRL_SUCCESS;
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_key_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control adds a rule specifying the manner in which LTP segment authentication will be applied t
 * o LTP segments recieved from the indicated LTP engine. A segment from the indicated LTP engine will 
 * only be deemed authentic if it contains an authentication extension computed via the ciphersuite ide
 * ntified by ciphersuite_nbr using the applicable key value. If ciphersuite_nbr is 255 then the applic
 * able key value is a hard-coded constant and key_name must be omitted; otherwise key_name is required
 *  and the applicable key value is the current value of the key named key_name in the local security p
 * olicy database. Valid values of ciphersuite_nbr are: 0: HMAC-SHA1-80 1: RSA-SHA256 255: NULL
 */
tdc_t* adm_ionsec_admin_ctrl_ltp_rx_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_ltp_rx_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
		/*TODO
		unsure which function to use ltprecvauthrule vs ltpxmitauthrule for add */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_ltp_rx_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the parameters of the LTP segment authentication rule for the indicated LTP eng
 * ine.
 */
tdc_t* adm_ionsec_admin_ctrl_ltp_rx_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_ltp_rx_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
		/*TODO
		unsure which function to use ltprecvauthrule vs ltpxmitauthrule for change*/
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_ltp_rx_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the LTP segment authentication rule for the indicated LTP engine.
 */
tdc_t* adm_ionsec_admin_ctrl_ltp_rx_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_ltp_rx_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
		/*TODO
		unsure which function to use ltprecvauthrule vs ltpxmitauthrule for remove*/
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_ltp_rx_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control adds a rule specifying the manner in which LTP segments transmitted to the indicated LT
 * P engine must be signed. Signing a segment destined for the indicated LTP engine entails computing a
 * n authentication extension via the ciphersuite identified by ciphersuite_nbr using the applicable ke
 * y value. If ciphersuite_nbr is 255 then the applicable key value is a hard-coded constant and key_na
 * me must be omitted; otherwise key_name is required and the applicable key value is the current value
 *  of the key named key_name in the local security policy database. Valid values of ciphersuite_nbr ar
 * e: 0:HMAC_SHA1-80 1: RSA_SHA256 255: NULL
 */
tdc_t* adm_ionsec_admin_ctrl_ltp_tx_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_ltp_tx_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
		/*TODO
		unsure which function to use ltprecvauthrule vs ltpxmitauthrule for add*/
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_ltp_tx_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the parameters of the LTP segment signing rule for the indicated LTP engine.
 */
tdc_t* adm_ionsec_admin_ctrl_ltp_tx_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_ltp_tx_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
		/*TODO
		unsure which function to use ltprecvauthrule vs ltpxmitauthrule for change*/
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_ltp_tx_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the LTP segment signing rule for the indicated LTP engine.
 */
tdc_t* adm_ionsec_admin_ctrl_ltp_tx_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_ltp_tx_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*TODO
		unsure which function to use ltprecvauthrule vs ltpxmitauthrule for remove*/
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_ltp_tx_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control lists the names of keys available in the key policy database.
 */
tdc_t* adm_ionsec_admin_ctrl_list_keys(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_list_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control lists all LTP segment authentication rules in the security policy database.
 */
tdc_t* adm_ionsec_admin_ctrl_list_ltp_rx_rules(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_list_ltp_rx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_ltp_rx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control lists all LTP segment signing rules in the security policy database.
 */
tdc_t* adm_ionsec_admin_ctrl_list_ltp_tx_rules(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_list_ltp_tx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_ltp_tx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
