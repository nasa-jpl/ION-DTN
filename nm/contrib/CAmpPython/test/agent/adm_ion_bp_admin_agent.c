/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin_agent.c
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
#include "../shared/adm/adm_ion_bp_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_ion_bp_admin_impl.h"
#include "rda.h"

#define _HAVE_ION_BP_ADMIN_ADM_
#ifdef _HAVE_ION_BP_ADMIN_ADM_

void adm_ion_bp_admin_init()
{

	adm_ion_bp_admin_setup();
	adm_ion_bp_admin_init_edd();
	adm_ion_bp_admin_init_variables();
	adm_ion_bp_admin_init_controls();
	adm_ion_bp_admin_init_constants();
	adm_ion_bp_admin_init_macros();
	adm_ion_bp_admin_init_metadata();
	adm_ion_bp_admin_init_ops();
	adm_ion_bp_admin_init_reports();

}

void adm_ion_bp_admin_init_edd()
{
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_EDD_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_bp_admin_get_version, NULL, NULL);

}

void adm_ion_bp_admin_init_variables()
{

	uint32_t used  = 0;
	mid_t *cur_mid = NULL;
	expr_t *expr   = NULL;
	Lyst def       = NULL;


}

void adm_ion_bp_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_ADD_MID), adm_ion_bp_admin_ctrl_endpoint_add);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_CHANGE_MID), adm_ion_bp_admin_ctrl_endpoint_change);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_DEL_MID), adm_ion_bp_admin_ctrl_endpoint_del);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_ADD_MID), adm_ion_bp_admin_ctrl_induct_add);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_CHANGE_MID), adm_ion_bp_admin_ctrl_induct_change);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_DEL_MID), adm_ion_bp_admin_ctrl_induct_del);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_START_MID), adm_ion_bp_admin_ctrl_induct_start);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_STOP_MID), adm_ion_bp_admin_ctrl_induct_stop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INIT_MID), adm_ion_bp_admin_ctrl_init);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_MANAGE_HEAP_MAX_MID), adm_ion_bp_admin_ctrl_manage_heap_max);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_ADD_MID), adm_ion_bp_admin_ctrl_outduct_add);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_CHANGE_MID), adm_ion_bp_admin_ctrl_outduct_change);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_DEL_MID), adm_ion_bp_admin_ctrl_outduct_del);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_START_MID), adm_ion_bp_admin_ctrl_outduct_start);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_EGRESS_PLAN_BLOCK_MID), adm_ion_bp_admin_ctrl_egress_plan_block);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_EGRESS_PLAN_UNBLOCK_MID), adm_ion_bp_admin_ctrl_egress_plan_unblock);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_STOP_MID), adm_ion_bp_admin_ctrl_outduct_stop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_ADD_MID), adm_ion_bp_admin_ctrl_protocol_add);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_DEL_MID), adm_ion_bp_admin_ctrl_protocol_del);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_START_MID), adm_ion_bp_admin_ctrl_protocol_start);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_STOP_MID), adm_ion_bp_admin_ctrl_protocol_stop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_ADD_MID), adm_ion_bp_admin_ctrl_scheme_add);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_CHANGE_MID), adm_ion_bp_admin_ctrl_scheme_change);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_DEL_MID), adm_ion_bp_admin_ctrl_scheme_del);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_START_MID), adm_ion_bp_admin_ctrl_scheme_start);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_STOP_MID), adm_ion_bp_admin_ctrl_scheme_stop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_START_MID), adm_ion_bp_admin_ctrl_start);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_STOP_MID), adm_ion_bp_admin_ctrl_stop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_WATCH_MID), adm_ion_bp_admin_ctrl_watch);

}

void adm_ion_bp_admin_init_constants()
{

}

void adm_ion_bp_admin_init_macros()
{

}

void adm_ion_bp_admin_init_ops()
{

}

void adm_ion_bp_admin_init_metadata()
{

	/* Step 1: Register Nicknames */
	oid_nn_add_parm(ION_BP_ADMIN_ADM_META_NN_IDX, ION_BP_ADMIN_ADM_META_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_EDD_NN_IDX, ION_BP_ADMIN_ADM_EDD_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_VAR_NN_IDX, ION_BP_ADMIN_ADM_VAR_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_RPT_NN_IDX, ION_BP_ADMIN_ADM_RPT_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_CTRL_NN_IDX, ION_BP_ADMIN_ADM_CTRL_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_CONST_NN_IDX, ION_BP_ADMIN_ADM_CONST_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_MACRO_NN_IDX, ION_BP_ADMIN_ADM_MACRO_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_OP_NN_IDX, ION_BP_ADMIN_ADM_OP_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_ROOT_NN_IDX, ION_BP_ADMIN_ADM_ROOT_NN_STR, "ION_BP_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_organization, adm_print_string, adm_size_string);

}

void adm_ion_bp_admin_init_reports()
{

	Lyst rpt                = NULL;
	mid_t *cur_mid          = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used           = 0;
	
	
}

#endif // _HAVE_ION_BP_ADMIN_ADM_
