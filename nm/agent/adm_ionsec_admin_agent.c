/****************************************************************************
 **
 ** File Name: adm_ionsec_admin_agent.c
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
#include "adm_ionsec_admin_impl.h"
#include "agent/rda.h"



#define _HAVE_DTN_ION_IONSECADMIN_ADM_
#ifdef _HAVE_DTN_ION_IONSECADMIN_ADM_

static vec_idx_t g_dtn_ion_ionsecadmin_idx[11];

void dtn_ion_ionsecadmin_init()
{
	adm_add_adm_info("dtn_ion_ionsecadmin", ADM_ENUM_DTN_ION_IONSECADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONSECADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_ionsecadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONSECADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_ionsecadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONSECADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_ionsecadmin_setup();
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

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_NAME), dtn_ion_ionsecadmin_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_NAMESPACE), dtn_ion_ionsecadmin_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_VERSION), dtn_ion_ionsecadmin_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionsecadmin_idx[ADM_META_IDX], DTN_ION_IONSECADMIN_META_ORGANIZATION), dtn_ion_ionsecadmin_meta_organization);
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

	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_KEY_ADD, 2, dtn_ion_ionsecadmin_ctrl_key_add);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_KEY_CHANGE, 2, dtn_ion_ionsecadmin_ctrl_key_change);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_KEY_DEL, 1, dtn_ion_ionsecadmin_ctrl_key_del);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_ADD, 3, dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_add);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_CHANGE, 3, dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_change);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_RX_RULE_DEL, 1, dtn_ion_ionsecadmin_ctrl_ltp_rx_rule_del);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_ADD, 3, dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_add);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_CHANGE, 3, dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_change);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LTP_TX_RULE_DEL, 1, dtn_ion_ionsecadmin_ctrl_ltp_tx_rule_del);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LIST_KEYS, 0, dtn_ion_ionsecadmin_ctrl_list_keys);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LIST_LTP_RX_RULES, 0, dtn_ion_ionsecadmin_ctrl_list_ltp_rx_rules);
	adm_add_ctrldef(g_dtn_ion_ionsecadmin_idx[ADM_CTRL_IDX], DTN_ION_IONSECADMIN_CTRL_LIST_LTP_TX_RULES, 0, dtn_ion_ionsecadmin_ctrl_list_ltp_tx_rules);
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

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionsecadmin_idx[ADM_TBLT_IDX], DTN_ION_IONSECADMIN_TBLT_LTP_RX_RULES), dtn_ion_ionsecadmin_tblt_ltp_rx_rules);
	tblt_add_col(def, AMP_TYPE_UINT, "ltp_engine_id");
	tblt_add_col(def, AMP_TYPE_UINT, "ciphersuite_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "key_name");
	adm_add_tblt(def);

	/* LTP_TX_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionsecadmin_idx[ADM_TBLT_IDX], DTN_ION_IONSECADMIN_TBLT_LTP_TX_RULES), dtn_ion_ionsecadmin_tblt_ltp_tx_rules);
	tblt_add_col(def, AMP_TYPE_UINT, "ltp_engine_id");
	tblt_add_col(def, AMP_TYPE_UINT, "ciphersuite_nbr");
	tblt_add_col(def, AMP_TYPE_STR, "key_name");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_ION_IONSECADMIN_ADM_
