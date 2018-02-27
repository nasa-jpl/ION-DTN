/****************************************************************************
 **
 ** File Name: adm_ion_ltp_admin_agent.c
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
 **  2018-02-07  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ion_ltp_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_ion_ltp_admin_impl.h"
#include "rda.h"

#define _HAVE_ION_LTP_ADMIN_ADM_
#ifdef _HAVE_ION_LTP_ADMIN_ADM_

void adm_ion_ltp_admin_init()
{

	adm_ion_ltp_admin_setup();
	adm_ion_ltp_admin_init_edd();
	adm_ion_ltp_admin_init_variables();
	adm_ion_ltp_admin_init_controls();
	adm_ion_ltp_admin_init_constants();
	adm_ion_ltp_admin_init_macros();
	adm_ion_ltp_admin_init_metadata();
	adm_ion_ltp_admin_init_ops();
	adm_ion_ltp_admin_init_reports();

}

void adm_ion_ltp_admin_init_edd()
{
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_EDD_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_ltp_admin_get_version, NULL, NULL);


}

void adm_ion_ltp_admin_init_variables()
{

	uint32_t used  = 0;
	mid_t *cur_mid = NULL;
	expr_t *expr   = NULL;
	Lyst def       = NULL;


}

void adm_ion_ltp_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_INIT_MID), adm_ion_ltp_admin_ctrl_init);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_INIT_MID, "est_max_export_sessions", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_HEAP_MID), adm_ion_ltp_admin_ctrl_manage_heap);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_HEAP_MID, "max_database_heap_per_block", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_MAX_BER_MID), adm_ion_ltp_admin_ctrl_manage_max_ber);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_MAX_BER_MID, "max_expected_bit_error_rate", AMP_TYPE_REAL32);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_OWN_QUEUE_TIME_MID), adm_ion_ltp_admin_ctrl_manage_own_queue_time);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_OWN_QUEUE_TIME_MID, "own_queing_latency", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_SCREENING_MID), adm_ion_ltp_admin_ctrl_manage_screening);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_SCREENING_MID, "new_state", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_SPAN_ADD_MID), adm_ion_ltp_admin_ctrl_span_add);
	UI_ADD_PARMSPEC_8(ADM_ION_LTP_ADMIN_CTRL_SPAN_ADD_MID, "peer_engine_number", AMP_TYPE_UINT, "max_export_sessions", AMP_TYPE_UINT, "max_import_sessions", AMP_TYPE_UINT, "max_segment_size", AMP_TYPE_UINT, "aggregtion_size_limit", AMP_TYPE_UINT, "aggregation_time_limit", AMP_TYPE_UINT, "lso_control", AMP_TYPE_STR, "queuing_latency", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_SPAN_CHANGE_MID), adm_ion_ltp_admin_ctrl_span_change);
	UI_ADD_PARMSPEC_8(ADM_ION_LTP_ADMIN_CTRL_SPAN_CHANGE_MID, "peer_engine_number", AMP_TYPE_UINT, "max_export_sessions", AMP_TYPE_UINT, "max_import_sessions", AMP_TYPE_UINT, "max_segment_size", AMP_TYPE_UINT, "aggregtion_size_limit", AMP_TYPE_UINT, "aggregation_time_limit", AMP_TYPE_UINT, "lso_control", AMP_TYPE_STR, "queuing_latency", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_SPAN_DEL_MID), adm_ion_ltp_admin_ctrl_span_del);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_SPAN_DEL_MID, "peer_engine_number", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_STOP_MID), adm_ion_ltp_admin_ctrl_stop);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_WATCH_SET_MID), adm_ion_ltp_admin_ctrl_watch_set);
	UI_ADD_PARMSPEC_2(ADM_ION_LTP_ADMIN_CTRL_WATCH_SET_MID, "status", AMP_TYPE_UINT, "activity", AMP_TYPE_UINT);


}

void adm_ion_ltp_admin_init_constants()
{

}

void adm_ion_ltp_admin_init_macros()
{

}

void adm_ion_ltp_admin_init_ops()
{

}

void adm_ion_ltp_admin_init_metadata()
{

	/* Step 1: Register Nicknames */
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_META_NN_IDX, ION_LTP_ADMIN_ADM_META_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_EDD_NN_IDX, ION_LTP_ADMIN_ADM_EDD_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_VAR_NN_IDX, ION_LTP_ADMIN_ADM_VAR_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_RPT_NN_IDX, ION_LTP_ADMIN_ADM_RPT_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_CTRL_NN_IDX, ION_LTP_ADMIN_ADM_CTRL_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_CONST_NN_IDX, ION_LTP_ADMIN_ADM_CONST_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_MACRO_NN_IDX, ION_LTP_ADMIN_ADM_MACRO_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_OP_NN_IDX, ION_LTP_ADMIN_ADM_OP_NN_STR, "ION_LTP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_LTP_ADMIN_ADM_ROOT_NN_IDX, ION_LTP_ADMIN_ADM_ROOT_NN_STR, "ION_LTP_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, adm_ion_ltp_admin_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_ion_ltp_admin_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, adm_ion_ltp_admin_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_ion_ltp_admin_meta_organization, adm_print_string, adm_size_string);

}

void adm_ion_ltp_admin_init_reports()
{

	Lyst rpt                = NULL;
	mid_t *cur_mid          = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used           = 0;
	
	
}

#endif // _HAVE_ION_LTP_ADMIN_ADM_
