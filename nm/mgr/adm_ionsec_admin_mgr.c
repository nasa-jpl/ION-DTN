/****************************************************************************
 **
 ** File Name: adm_ionsec_admin_mgr.c
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


#include "ion.h"
#include "platform.h"
#include "adm_ionsec_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "metadata.h"
#include "nm_mgr_ui.h"




#define _HAVE_DTN_ION_IONSECADMIN_ADM_
#ifdef _HAVE_DTN_ION_IONSECADMIN_ADM_
static vec_idx_t g_dtn_ion_ionsecadmin_idx[11];

void dtn_ion_ionsecadmin_init()
{
	adm_add_adm_info("dtn_ion_ionsecadmin", ADM_ENUM_DTN_ION_IONSECADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONSECADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_ionsecadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONSECADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_ionsecadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONSECADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_ionsecadmin_init_meta();
	dtn_ion_ionsecadmin_init_cnst();
	dtn_ion_ionsecadmin_init_edd();
	dtn_ion_ionsecadmin_init_op();
	dtn_ion_ionsecadmin_init_var();
	dtn_ion_ionsecadmin_init_ctrl();
	dtn_ion_ionsecadmin_init_mac();
	dtn_ion_ionsecadmin_init_rpttpl();
	dtn_ion_ionsecadmin_init_tblt();
}

void dtn_ion_ionsecadmin_init_meta()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONSECADMIN, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONSECADMIN, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONSECADMIN, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONSECADMIN, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_ion_ionsecadmin_init_cnst()
{

}

void dtn_ion_ionsecadmin_init_edd()
{

}

void dtn_ion_ionsecadmin_init_op()
{

}

void dtn_ion_ionsecadmin_init_var()
{

}

void dtn_ion_ionsecadmin_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* KEY_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_KEY_ADD);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "key_add", "This control adds a named key value to the security policy database. The content of file_name is taken as the value of the key.Named keys can be referenced by other elements of thesecurity policy database.");

	meta_add_parm(meta, "key_name", AMP_TYPE_STR);
	meta_add_parm(meta, "key_value", AMP_TYPE_BYTESTR);

	/* KEY_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_KEY_CHANGE);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "key_change", "This control changes the value of the named key, obtaining the new key value from the content of file_name.");

	meta_add_parm(meta, "key_name", AMP_TYPE_STR);
	meta_add_parm(meta, "key_value", AMP_TYPE_BYTESTR);

	/* KEY_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_KEY_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "key_del", "This control deletes the key identified by name.");

	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* LTP_RX_RULE_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_ADD);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_rx_rule_add", "This control adds a rule specifying the manner in which LTP segment authentication will be applied to LTP segmentsrecieved from the indicated LTP engine. A segment from the indicated LTP engine will only be deemed authentic if it contains an authentication extension computed via the ciphersuite identified by ciphersuite_nbr using the applicable key value. If ciphersuite_nbr is 255 then the applicable key value is a hard-coded constant and key_name must be omitted; otherwise key_nameis required and the applicable key value is the current value of the key named key_name in the local security policy database. Valid values of ciphersuite_nbr are: 0: HMAC-SHA1-80 1: RSA-SHA256 255: NULL");

	meta_add_parm(meta, "ltp_engine_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "ciphersuite_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* LTP_RX_RULE_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_CHANGE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_rx_rule_change", "This control changes the parameters of the LTP segment authentication rule for the indicated LTP engine.");

	meta_add_parm(meta, "ltp_engine_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "ciphersuite_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* LTP_RX_RULE_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_rx_rule_del", "This control deletes the LTP segment authentication rule for the indicated LTP engine.");

	meta_add_parm(meta, "ltp_engine_id", AMP_TYPE_UINT);

	/* LTP_TX_RULE_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_ADD);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_tx_rule_add", "This control adds a rule specifying the manner in which LTP segments transmitted to the indicated LTP engine mustbe signed. Signing a segment destined for the indicated LTP engineentails computing an authentication extension via the ciphersuite identified by ciphersuite_nbr using the applicable key value. If ciphersuite_nbr is 255 then the applicable key value is a hard-coded constant and key_name must be omitted; otherwise key_nameis required and the applicable key value is the current value of the key named key_name in the local security policy database.Valid values of ciphersuite_nbr are: 0:HMAC_SHA1-80 1: RSA_SHA256 255: NULL");

	meta_add_parm(meta, "ltp_engine_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "ciphersuite_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* LTP_TX_RULE_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_CHANGE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_tx_rule_change", "This control changes the parameters of the LTP segment signing rule for the indicated LTP engine.");

	meta_add_parm(meta, "ltp_engine_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "ciphersuite_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* LTP_TX_RULE_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_tx_rule_del", "This control deletes the LTP segment signing rule forthe indicated LTP engine.");

	meta_add_parm(meta, "ltp_engine_id", AMP_TYPE_UINT);

	/* LIST_KEYS */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LIST_KEYS);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "list_keys", "This control lists the names of keys available in the key policy database.");


	/* LIST_LTP_RX_RULES */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LIST_LTP_RX_RULES);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "list_ltp_rx_rules", "This control lists all LTP segment authentication rules in the security policy database.");


	/* LIST_LTP_TX_RULES */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LIST_LTP_TX_RULES);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONSECADMIN, "list_ltp_tx_rules", "This control lists all LTP segment signing rules in the security policy database.");

}

void dtn_ion_ionsecadmin_init_mac()
{

}

void dtn_ion_ionsecadmin_init_rpttpl()
{

}

void dtn_ion_ionsecadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* LTP_RX_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionsecadmin_idx[ADM_TBLT_IDX], DTN_ION_IONSECADMIN_TBLT_LTP_RX_RULES), NULL);
	tblt_add_col(def, AMP_TYPE_UINT, "ltp_engine_id");
	tblt_add_col(def, AMP_TYPE_UINT, "ciphersuite_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "key_name");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_rx_rules", "This table lists all LTP segment authentication rulesin the security policy database.");

	/* LTP_TX_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionsecadmin_idx[ADM_TBLT_IDX], DTN_ION_IONSECADMIN_TBLT_LTP_TX_RULES), NULL);
	tblt_add_col(def, AMP_TYPE_UINT, "ltp_engine_id");
	tblt_add_col(def, AMP_TYPE_UINT, "ciphersuite_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "key_name");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_IONSECADMIN, "ltp_tx_rules", "This table lists all LTP segment signing rules in the security policy database.");
}

#endif // _HAVE_DTN_ION_IONSECADMIN_ADM_
