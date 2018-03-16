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
 **  2018-03-16  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ionsec_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_ionsec_admin_impl.h"
#include "rda.h"

#define _HAVE_IONSEC_ADMIN_ADM_
#ifdef _HAVE_IONSEC_ADMIN_ADM_

void adm_ionsec_admin_init()
{

	adm_ionsec_admin_setup();
	adm_ionsec_admin_init_edd();
	adm_ionsec_admin_init_variables();
	adm_ionsec_admin_init_controls();
	adm_ionsec_admin_init_constants();
	adm_ionsec_admin_init_macros();
	adm_ionsec_admin_init_metadata();
	adm_ionsec_admin_init_ops();
	adm_ionsec_admin_init_reports();

}

void adm_ionsec_admin_init_edd()
{

}

void adm_ionsec_admin_init_variables()
{

	uint32_t used  = 0;
	mid_t *cur_mid = NULL;
	expr_t *expr   = NULL;
	Lyst def       = NULL;


}

void adm_ionsec_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_KEY_ADD_MID), adm_ionsec_admin_ctrl_key_add);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_KEY_CHANGE_MID), adm_ionsec_admin_ctrl_key_change);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_KEY_DEL_MID), adm_ionsec_admin_ctrl_key_del);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_ADD_MID), adm_ionsec_admin_ctrl_ltp_rx_rule_add);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_CHANGE_MID), adm_ionsec_admin_ctrl_ltp_rx_rule_change);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LTP_RX_RULE_DEL_MID), adm_ionsec_admin_ctrl_ltp_rx_rule_del);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_ADD_MID), adm_ionsec_admin_ctrl_ltp_tx_rule_add);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_CHANGE_MID), adm_ionsec_admin_ctrl_ltp_tx_rule_change);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LTP_TX_RULE_DEL_MID), adm_ionsec_admin_ctrl_ltp_tx_rule_del);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LIST_KEYS_MID), adm_ionsec_admin_ctrl_list_keys);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LIST_LTP_RX_RULES_MID), adm_ionsec_admin_ctrl_list_ltp_rx_rules);
	adm_add_ctrl(mid_from_value(ADM_IONSEC_ADMIN_CTRL_LIST_LTP_TX_RULES_MID), adm_ionsec_admin_ctrl_list_ltp_tx_rules);

}

void adm_ionsec_admin_init_constants()
{

}

void adm_ionsec_admin_init_macros()
{

}

void adm_ionsec_admin_init_ops()
{

}

void adm_ionsec_admin_init_metadata()
{

	/* Step 1: Register Nicknames */
	oid_nn_add_parm(IONSEC_ADMIN_ADM_META_NN_IDX, IONSEC_ADMIN_ADM_META_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_EDD_NN_IDX, IONSEC_ADMIN_ADM_EDD_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_VAR_NN_IDX, IONSEC_ADMIN_ADM_VAR_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_RPT_NN_IDX, IONSEC_ADMIN_ADM_RPT_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_CTRL_NN_IDX, IONSEC_ADMIN_ADM_CTRL_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_CONST_NN_IDX, IONSEC_ADMIN_ADM_CONST_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_MACRO_NN_IDX, IONSEC_ADMIN_ADM_MACRO_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_OP_NN_IDX, IONSEC_ADMIN_ADM_OP_NN_STR, "IONSEC_ADMIN", "2017-08-17");
	oid_nn_add_parm(IONSEC_ADMIN_ADM_ROOT_NN_IDX, IONSEC_ADMIN_ADM_ROOT_NN_STR, "IONSEC_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_IONSEC_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, adm_ionsec_admin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_IONSEC_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_ionsec_admin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_IONSEC_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, adm_ionsec_admin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_IONSEC_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_ionsec_admin_meta_organization, adm_print_string, adm_size_string);

}

void adm_ionsec_admin_init_reports()
{

	Lyst rpt                = NULL;
	mid_t *cur_mid          = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used           = 0;
	
	
}

#endif // _HAVE_IONSEC_ADMIN_ADM_
