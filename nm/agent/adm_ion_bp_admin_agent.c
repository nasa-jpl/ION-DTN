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
 **  2018-01-05  AUTO             Auto-generated c file 
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
	adm_add_edd(ADM_ION_BP_ADMIN_EDD_VERSION_MID, AMP_TYPE_STR, 0, adm_ion_bp_admin_get_version, NULL, NULL);

}

void adm_ion_bp_admin_init_variables()
{
}

void adm_ion_bp_admin_init_controls()
{
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_ENDPOINTADD,adm_ion_bp_admin_ctrl_endpointAdd);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_ENDPOINTCHANGE,adm_ion_bp_admin_ctrl_endpointChange);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_ENDPOINTDEL,adm_ion_bp_admin_ctrl_endpointDel);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_INDUCTADD,adm_ion_bp_admin_ctrl_inductAdd);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_INDUCTCHANGE,adm_ion_bp_admin_ctrl_inductChange);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_INDUCTDEL,adm_ion_bp_admin_ctrl_inductDel);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_INDUCTSTART,adm_ion_bp_admin_ctrl_inductStart);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_INDUCTSTOP,adm_ion_bp_admin_ctrl_inductStop);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_INIT,adm_ion_bp_admin_ctrl_init);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_MANAGEHEAPMAX,adm_ion_bp_admin_ctrl_manageHeapMax);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTADD,adm_ion_bp_admin_ctrl_outductAdd);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTCHANGE,adm_ion_bp_admin_ctrl_outductChange);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTDEL,adm_ion_bp_admin_ctrl_outductDel);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTSTART,adm_ion_bp_admin_ctrl_outductStart);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTBLOCK,adm_ion_bp_admin_ctrl_outductBlock);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTUNBLOCK,adm_ion_bp_admin_ctrl_outductUnblock);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_OUTDUCTSTOP,adm_ion_bp_admin_ctrl_outductStop);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_PROTOCOLADD,adm_ion_bp_admin_ctrl_protocolAdd);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_PROTOCOLDEL,adm_ion_bp_admin_ctrl_protocolDel);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_PROTOCOLSTART,adm_ion_bp_admin_ctrl_protocolStart);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_PROTOCOLSTOP,adm_ion_bp_admin_ctrl_protocolStop);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_SCHEMEADD,adm_ion_bp_admin_ctrl_schemeAdd);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_SCHEMECHANGE,adm_ion_bp_admin_ctrl_schemeChange);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_SCHEMEDEL,adm_ion_bp_admin_ctrl_schemeDel);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_SCHEMESTART,adm_ion_bp_admin_ctrl_schemeStart);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_SCHEMESTOP,adm_ion_bp_admin_ctrl_schemeStop);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_START,adm_ion_bp_admin_ctrl_start);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_STOP,adm_ion_bp_admin_ctrl_stop);
	adm_add_ctrl(ADM_ION_BP_ADMIN_CTRL_WATCH,adm_ion_bp_admin_ctrl_watch);
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
	adm_add_edd(ADM_ION_BP_ADMIN_META_NAME_MID, AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(ADM_ION_BP_ADMIN_META_NAMESPACE_MID, AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(ADM_ION_BP_ADMIN_META_VERSION_MID, AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(ADM_ION_BP_ADMIN_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, adm_ion_bp_admin_meta_organization, adm_print_string, adm_size_string);
}

void adm_ion_bp_admin_init_ops()
{
}

void adm_ion_bp_admin_init_reports()
{
	uint32_t used= 0;

}

#endif // _HAVE_ION_BP_ADMIN_ADM_
