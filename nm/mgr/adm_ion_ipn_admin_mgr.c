/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin_mgr.c
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


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ion_ipn_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"

#define _HAVE_ION_IPN_ADMIN_ADM_
#ifdef _HAVE_ION_IPN_ADMIN_ADM_

void adm_ion_ipn_admin_init()
{
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
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_EDD_ION_VERSION_MID), AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("ION_VERSION", "This is the version of ion is that currently installed.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_EDD_ION_VERSION_MID);

}


void adm_ion_ipn_admin_init_variables()
{
}


void adm_ion_ipn_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_ADD_MID), NULL);
	names_add_name("EXIT_ADD", "This control establishes an \"exit\" for static default routing.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_EXIT_ADD_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_CHANGE_MID), NULL);
	names_add_name("EXIT_CHANGE", "This control changes the gateway node number for the exit identified by firstNodeNbr and lastNodeNbr.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_EXIT_CHANGE_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_DEL_MID), NULL);
	names_add_name("EXIT_DEL", "This control deletes the exit identified by firstNodeNbr and lastNodeNbr.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_EXIT_DEL_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_ADD_MID), NULL);
	names_add_name("EXIT_RULE_ADD", "This control adds an exit rule.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_ADD_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_CHANGE_MID), NULL);
	names_add_name("EXIT_RULE_CHANGE", "This control changes an exit rule.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_CHANGE_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_DEL_MID), NULL);
	names_add_name("EXIT_RULE_DEL", "This control deletes an exit rule.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_EXIT_RULE_DEL_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_ADD_MID), NULL);
	names_add_name("PLAN_ADD", "This control establishes an egress plan for the bundles that must be transmitted to the neighboring node that is identified by it's nodeNbr.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_PLAN_ADD_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_CHANGE_MID), NULL);
	names_add_name("PLAN_CHANGE", "This control changes the duct expression for the indicated plan.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_PLAN_CHANGE_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_DEL_MID), NULL);
	names_add_name("PLAN_DEL", "This control deletes the egress plan for the node that is identified by it's nodeNbr.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_PLAN_DEL_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_ADD_MID), NULL);
	names_add_name("PLAN_RULE_ADD", "This control adds a plan rule.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_ADD_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_CHANGE_MID), NULL);
	names_add_name("PLAN_RULE_CHANGE", "This control changes the plan rule.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_CHANGE_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_DEL_MID), NULL);
	names_add_name("PLAN_RULE_DEL", "This control deletes a plan rule.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_CTRL_PLAN_RULE_DEL_MID);

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
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_META_NAME_MID);
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_META_NAMESPACE_MID);
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_META_VERSION_MID);
	adm_add_edd(mid_from_value(ADM_ION_IPN_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM", ADM_ION_IPN_ADMIN, ADM_ION_IPN_ADMIN_META_ORGANIZATION_MID);
}


void adm_ion_ipn_admin_init_ops()
{
}


void adm_ion_ipn_admin_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = NULL;
}

#endif // _HAVE_ION_IPN_ADMIN_ADM_
