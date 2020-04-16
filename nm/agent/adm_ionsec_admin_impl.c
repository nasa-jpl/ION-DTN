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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "ion.h"
#include "ionsec.h"
#include "ltpsec.h"
#include "bpsec.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_ionsec_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */

/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ionsecadmin_setup()
{

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

void dtn_ion_ionsecadmin_cleanup()
{

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


tnv_t *dtn_ion_ionsecadmin_meta_name(tnvc_t *parms)
{
	return tnv_from_str("ionsec_admin");
}


tnv_t *dtn_ion_ionsecadmin_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/ION/ionsecadmin");
}


tnv_t *dtn_ion_ionsecadmin_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.0");
}


tnv_t *dtn_ion_ionsecadmin_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * This table lists all LTP segment authentication rulesin the security policy database.
 */
tbl_t *dtn_ion_ionsecadmin_tblt_ltp_rx_rules(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_ltp_rx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr	sdr = getIonsdr();
	OBJ_POINTER(SecDB, db);
	OBJ_POINTER(LtpRecvAuthRule, rule);
	Object	elt;
	Object	obj;
	tnvc_t  *cur_row = NULL;

	GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, db->ltpRecvAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpRecvAuthRule, rule, obj);

		/* (UINT engine_id, (UINT) cipher_nbr, (STR) key ame */
		if((cur_row = tnvc_create(3)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uint(rule->ltpEngineId));
			tnvc_insert(cur_row, tnv_from_uint(rule->ciphersuiteNbr));
			tnvc_insert(cur_row, tnv_from_str(rule->keyName));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_ionsecadmin_tblt_ltp_rx_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_ltp_rx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all LTP segment signing rules in the security policy database.
 */
tbl_t *dtn_ion_ionsecadmin_tblt_ltp_tx_rules(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_ltp_tx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr	sdr = getIonsdr();
	OBJ_POINTER(SecDB, db);
	OBJ_POINTER(LtpXmitAuthRule, rule);
	Object	elt;
	Object	obj;
	tnvc_t  *cur_row = NULL;

	GET_OBJ_POINTER(sdr, SecDB, db, getSecDbObject());

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, db->ltpXmitAuthRules); elt;
			elt = sdr_list_next(sdr, elt))
	{
		obj = sdr_list_data(sdr, elt);
		GET_OBJ_POINTER(sdr, LtpXmitAuthRule, rule, obj);

		/* (UINT engine_id, (UINT) cipher_nbr, (STR) key ame */
		if((cur_row = tnvc_create(3)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uint(rule->ltpEngineId));
			tnvc_insert(cur_row, tnv_from_uint(rule->ciphersuiteNbr));
			tnvc_insert(cur_row, tnv_from_str(rule->keyName));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_ionsecadmin_tblt_ltp_tx_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_ltp_tx_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */

/* Control Functions */

/*
 * This control adds a named key value to the security policy database. The content of file_name is tak
 * en as the value of the key.Named keys can be referenced by other elements of thesecurity policy data
 * base.
 */
tnv_t *dtn_ion_ionsecadmin_ctrl_key_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_key_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *key_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *file_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	if((key_name != NULL) && (file_name != NULL))
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
tnv_t *dtn_ion_ionsecadmin_ctrl_key_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_key_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *key_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *file_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	if((key_name != NULL) && (file_name != NULL))
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
tnv_t *dtn_ion_ionsecadmin_ctrl_key_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_key_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *key_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	sec_removeKey(key_name);
	*status = CTRL_SUCCESS;
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_key_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control adds a rule specifying the manner in which LTP segment authentication will be applied t
 * o LTP segmentsrecieved from the indicated LTP engine. A segment from the indicated LTP engine will o
 * nly be deemed authentic if it contains an authentication extension computed via the ciphersuite iden
 * tified by ciphersuite_nbr using the applicable key value. If ciphersuite_nbr is 255 then the applica
 * ble key value is a hard-coded constant and key_name must be omitted; otherwise key_nameis required a
 * nd the applicable key value is the current value of the key named key_name in the local security pol
 * icy database. Valid values of ciphersuite_nbr are: 0: HMAC-SHA1-80 1: RSA-SHA256 255: NULL
 */
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
 * P engine mustbe signed. Signing a segment destined for the indicated LTP engineentails computing an 
 * authentication extension via the ciphersuite identified by ciphersuite_nbr using the applicable key 
 * value. If ciphersuite_nbr is 255 then the applicable key value is a hard-coded constant and key_name
 *  must be omitted; otherwise key_nameis required and the applicable key value is the current value of
 *  the key named key_name in the local security policy database.Valid values of ciphersuite_nbr are: 0
 * :HMAC_SHA1-80 1: RSA_SHA256 255: NULL
 */
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
 * This control deletes the LTP segment signing rule forthe indicated LTP engine.
 */
tnv_t *dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_ion_ionsecadmin_ctrl_list_keys(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_ion_ionsecadmin_ctrl_list_ltp_rx_rules(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_ion_ionsecadmin_ctrl_list_ltp_tx_rules(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
