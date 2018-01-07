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
 **  2018-01-06  AUTO             Auto-generated c file 
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
}

void adm_ion_bp_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINTADD_MID),adm_ion_bp_admin_ctrl_endpointadd);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINTCHANGE_MID),adm_ion_bp_admin_ctrl_endpointchange);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINTDEL_MID),adm_ion_bp_admin_ctrl_endpointdel);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCTADD_MID),adm_ion_bp_admin_ctrl_inductadd);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCTCHANGE_MID),adm_ion_bp_admin_ctrl_inductchange);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCTDEL_MID),adm_ion_bp_admin_ctrl_inductdel);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCTSTART_MID),adm_ion_bp_admin_ctrl_inductstart);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCTSTOP_MID),adm_ion_bp_admin_ctrl_inductstop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INIT_MID),adm_ion_bp_admin_ctrl_init);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_MANAGEHEAPMAX_MID),adm_ion_bp_admin_ctrl_manageheapmax);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTADD_MID),adm_ion_bp_admin_ctrl_outductadd);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTCHANGE_MID),adm_ion_bp_admin_ctrl_outductchange);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTDEL_MID),adm_ion_bp_admin_ctrl_outductdel);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTSTART_MID),adm_ion_bp_admin_ctrl_outductstart);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTBLOCK_MID),adm_ion_bp_admin_ctrl_outductblock);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTUNBLOCK_MID),adm_ion_bp_admin_ctrl_outductunblock);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCTSTOP_MID),adm_ion_bp_admin_ctrl_outductstop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOLADD_MID),adm_ion_bp_admin_ctrl_protocoladd);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOLDEL_MID),adm_ion_bp_admin_ctrl_protocoldel);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOLSTART_MID),adm_ion_bp_admin_ctrl_protocolstart);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOLSTOP_MID),adm_ion_bp_admin_ctrl_protocolstop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEMEADD_MID),adm_ion_bp_admin_ctrl_schemeadd);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEMECHANGE_MID),adm_ion_bp_admin_ctrl_schemechange);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEMEDEL_MID),adm_ion_bp_admin_ctrl_schemedel);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEMESTART_MID),adm_ion_bp_admin_ctrl_schemestart);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEMESTOP_MID),adm_ion_bp_admin_ctrl_schemestop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_START_MID),adm_ion_bp_admin_ctrl_start);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_STOP_MID),adm_ion_bp_admin_ctrl_stop);
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_WATCH_MID),adm_ion_bp_admin_ctrl_watch);
}

void adm_ion_bp_admin_init_constants()
{
}

void adm_ion_bp_admin_init_macros()
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

void adm_ion_bp_admin_init_ops()
{
}

void adm_ion_bp_admin_init_reports()
{
	uint32_t used= 0;

}

#endif // _HAVE_ION_BP_ADMIN_ADM_
