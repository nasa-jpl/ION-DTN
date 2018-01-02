/****************************************************************************
 **
 ** File Name: ./agent/adm_IonBpAdmin_agent.c
 **
 ** Description: 
 **
 ** Notes: 
 **
 ** Assumptions: 
 **
 ** Modification History: 
 **  MM/DD/YY  AUTHOR           DESCRIPTION
 **  --------  --------------   ------------------------------------------------
 **  2017-11-11  AUTO           Auto generated c file 
 **
****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_IonBpAdmin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_IonBpAdmin_impl.h"
#include "rda.h"

#define _HAVE_IONBPADMIN_ADM_
#ifdef _HAVE_IONBPADMIN_ADM_

void adm_IonBpAdmin_init()
{
	adm_IonBpAdmin_init_edd();
	adm_IonBpAdmin_init_variables();
	adm_IonBpAdmin_init_controls();
	adm_IonBpAdmin_init_constants();
	adm_IonBpAdmin_init_macros();
	adm_IonBpAdmin_init_metadata();
	adm_IonBpAdmin_init_ops();
	adm_IonBpAdmin_init_reports();
}

void adm_IonBpAdmin_init_edd()
{
	adm_add_edd(ADM_IONBPADMIN_EDD_VERSION_MID, AMP_TYPE_STR, 0, adm_IonBpAdmin_get_version, NULL, NULL);
}

void adm_IonBpAdmin_init_variables()
{
}

void adm_IonBpAdmin_init_controls()
{
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_ENDPOINTADD_MID, adm_IonBpAdmin_ctrl_endpointAdd);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_ENDPOINTCHANGE_MID, adm_IonBpAdmin_ctrl_endpointChange);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_ENDPOINTDEL_MID, adm_IonBpAdmin_ctrl_endpointDel);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTADD_MID, adm_IonBpAdmin_ctrl_inductAdd);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTCHANGE_MID, adm_IonBpAdmin_ctrl_inductChange);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTDEL_MID, adm_IonBpAdmin_ctrl_inductDel);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTSTART_MID, adm_IonBpAdmin_ctrl_inductStart);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTSTOP_MID, adm_IonBpAdmin_ctrl_inductStop);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_MANAGEHEAPMAX_MID, adm_IonBpAdmin_ctrl_manageHeapMax);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTADD_MID, adm_IonBpAdmin_ctrl_outductAdd);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTCHANGE_MID, adm_IonBpAdmin_ctrl_outductChange);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTDEL_MID, adm_IonBpAdmin_ctrl_outductDel);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTSTART_MID, adm_IonBpAdmin_ctrl_outductStart);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTSTOP_MID, adm_IonBpAdmin_ctrl_outductStop);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLADD_MID, adm_IonBpAdmin_ctrl_protocolAdd);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLDEL_MID, adm_IonBpAdmin_ctrl_protocolDel);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLSTART_MID, adm_IonBpAdmin_ctrl_protocolStart);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLSTOP_MID, adm_IonBpAdmin_ctrl_protocolStop);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMEADD_MID, adm_IonBpAdmin_ctrl_schemeAdd);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMECHANGE_MID, adm_IonBpAdmin_ctrl_schemeChange);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMEDEL_MID, adm_IonBpAdmin_ctrl_schemeDel);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMESTART_MID, adm_IonBpAdmin_ctrl_schemeStart);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMESTOP_MID, adm_IonBpAdmin_ctrl_schemeStop);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANADD_MID, adm_IonBpAdmin_ctrl_egressPlanAdd);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANUPDATE_MID, adm_IonBpAdmin_ctrl_egressPlanUpdate);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANDELETE_MID, adm_IonBpAdmin_ctrl_egressPlanDelete);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANSTART_MID, adm_IonBpAdmin_ctrl_egressPlanStart);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANSTOP_MID, adm_IonBpAdmin_ctrl_egressPlanStop);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANBLOCK_MID, adm_IonBpAdmin_ctrl_egressPlanBlock);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANUNBLOCK_MID, adm_IonBpAdmin_ctrl_egressPlanUnblock);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANGATEWAY_MID, adm_IonBpAdmin_ctrl_egressPlanGateway);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTATTACH_MID, adm_IonBpAdmin_ctrl_egressPlanOutductAttach);
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTDETATCH_MID, adm_IonBpAdmin_ctrl_egressPlanOutductDetatch);
}

void adm_IonBpAdmin_init_constants()
{
}

void adm_IonBpAdmin_init_macros()
{
}

void adm_IonBpAdmin_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(IONBPADMIN_ADM_META_NN_IDX, IONBPADMIN_ADM_META_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_EDD_NN_IDX, IONBPADMIN_ADM_EDD_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_VAR_NN_IDX, IONBPADMIN_ADM_VAR_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_RPT_NN_IDX, IONBPADMIN_ADM_RPT_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_CTRL_NN_IDX, IONBPADMIN_ADM_CTRL_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_CONST_NN_IDX, IONBPADMIN_ADM_CONST_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_MACRO_NN_IDX, IONBPADMIN_ADM_MACRO_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_OP_NN_IDX, IONBPADMIN_ADM_OP_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_ROOT_NN_IDX, IONBPADMIN_ADM_ROOT_NN_STR, "IONBPADMIN", "2017-08-17");
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_IONBPADMIN_META_NAME_MID, AMP_TYPE_STR, 0, adm_IonBpAdmin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(ADM_IONBPADMIN_META_NAMESPACE_MID, AMP_TYPE_STR, 0, adm_IonBpAdmin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(ADM_IONBPADMIN_META_VERSION_MID, AMP_TYPE_STR, 0, adm_IonBpAdmin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(ADM_IONBPADMIN_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, adm_IonBpAdmin_meta_organization, adm_print_string, adm_size_string);
}

void adm_IonBpAdmin_init_ops()
{
}

void adm_IonBpAdmin_init_reports()
{
	uint32_t used= 0;

}

#endif // _HAVE_IONBPADMIN_ADM_
