/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin_agent.c
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
 **  2018-01-06  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ion_ipn_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_ion_ipn_admin_impl.h"
#include "rda.h"

#define _HAVE_ION_IPN_ADMIN_ADM_
#ifdef _HAVE_ION_IPN_ADMIN_ADM_

void adm_ion_ipn_admin_init()
{
	adm_ion_ipn_admin_setup();
	adm_ion_ipn_admin_init_edd();
	adm_ion_ipn_admin_init_variables();
	adm_ion_ipn_admin_init_controls();
	adm_ion_ipn_admin_init_constants();
	adm_ion_ipn_admin_init_macros();
	adm_ion_ipn_admin_init_metadata();
	adm_ion_ipn_admin_init_ops();
	adm_ion_ipn_admin_init_reports();
}

void adm_ion_ipn_admin_init_edd()
{
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_EDD_ION_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_ipn_admin_get_ion_version, NULL, NULL);

}

void adm_ion_ipn_admin_init_variables()
{
}

void adm_ion_ipn_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_ADD_MID),adm_ion_ipn_admin_ctrl_exit_add);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_CHANGE_MID),adm_ion_ipn_admin_ctrl_exit_change);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_DEL_MID),adm_ion_ipn_admin_ctrl_exit_del);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_ADD_MID),adm_ion_ipn_admin_ctrl_exit_rule_add);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_CHANGE_MID),adm_ion_ipn_admin_ctrl_exit_rule_change);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_DEL_MID),adm_ion_ipn_admin_ctrl_exit_rule_del);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_ADD_MID),adm_ion_ipn_admin_ctrl_plan_add);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_CHANGE_MID),adm_ion_ipn_admin_ctrl_plan_change);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_DEL_MID),adm_ion_ipn_admin_ctrl_plan_del);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_ADD_MID),adm_ion_ipn_admin_ctrl_plan_rule_add);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_CHANGE_MID),adm_ion_ipn_admin_ctrl_plan_rule_change);
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_DEL_MID),adm_ion_ipn_admin_ctrl_plan_rule_del);
}

void adm_ion_ipn_admin_init_constants()
{
}

void adm_ion_ipn_admin_init_macros()
{
}

void adm_ion_ipn_admin_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_META_NN_IDX, ION_IPN_ADMIN_ADM_META_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_EDD_NN_IDX, ION_IPN_ADMIN_ADM_EDD_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_VAR_NN_IDX, ION_IPN_ADMIN_ADM_VAR_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_RPT_NN_IDX, ION_IPN_ADMIN_ADM_RPT_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_CTRL_NN_IDX, ION_IPN_ADMIN_ADM_CTRL_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_CONST_NN_IDX, ION_IPN_ADMIN_ADM_CONST_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_MACRO_NN_IDX, ION_IPN_ADMIN_ADM_MACRO_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_OP_NN_IDX, ION_IPN_ADMIN_ADM_OP_NN_STR, "ION_IPN_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_IPN_ADMIN_ADM_ROOT_NN_IDX, ION_IPN_ADMIN_ADM_ROOT_NN_STR, "ION_IPN_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, adm_ion_ipn_admin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_ion_ipn_admin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_ipn_admin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_ion_ipn_admin_meta_organization, adm_print_string, adm_size_string);
}

void adm_ion_ipn_admin_init_ops()
{
}

void adm_ion_ipn_admin_init_reports()
{
	uint32_t used= 0;

}

#endif // _HAVE_ION_IPN_ADMIN_ADM_
