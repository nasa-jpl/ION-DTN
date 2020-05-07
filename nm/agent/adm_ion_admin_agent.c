/****************************************************************************
 **
 ** File Name: adm_ion_admin_agent.c
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
#include "adm_ion_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_ion_admin_impl.h"
#include "agent/rda.h"



#define _HAVE_DTN_ION_IONADMIN_ADM_
#ifdef _HAVE_DTN_ION_IONADMIN_ADM_

static vec_idx_t g_dtn_ion_ionadmin_idx[11];

void dtn_ion_ionadmin_init()
{
	adm_add_adm_info("dtn_ion_ionadmin", ADM_ENUM_DTN_ION_IONADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_ionadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_ionadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONADMIN * 20) + ADM_EDD_IDX), &(g_dtn_ion_ionadmin_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_IONADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_ionadmin_setup();
	dtn_ion_ionadmin_init_meta();
	dtn_ion_ionadmin_init_cnst();
	dtn_ion_ionadmin_init_edd();
	dtn_ion_ionadmin_init_op();
	dtn_ion_ionadmin_init_var();
	dtn_ion_ionadmin_init_ctrl();
	dtn_ion_ionadmin_init_mac();
	dtn_ion_ionadmin_init_rpttpl();
	dtn_ion_ionadmin_init_tblt();
}

void dtn_ion_ionadmin_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_NAME), dtn_ion_ionadmin_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_NAMESPACE), dtn_ion_ionadmin_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_VERSION), dtn_ion_ionadmin_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_ORGANIZATION), dtn_ion_ionadmin_meta_organization);
}

void dtn_ion_ionadmin_init_cnst()
{

}

void dtn_ion_ionadmin_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CLOCK_ERROR), dtn_ion_ionadmin_get_clock_error);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CLOCK_SYNC), dtn_ion_ionadmin_get_clock_sync);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CONGESTION_ALARM_CONTROL), dtn_ion_ionadmin_get_congestion_alarm_control);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CONGESTION_END_TIME_FORECASTS), dtn_ion_ionadmin_get_congestion_end_time_forecasts);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CONSUMPTION_RATE), dtn_ion_ionadmin_get_consumption_rate);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT), dtn_ion_ionadmin_get_inbound_file_system_occupancy_limit);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT), dtn_ion_ionadmin_get_inbound_heap_occupancy_limit);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_NUMBER), dtn_ion_ionadmin_get_number);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT), dtn_ion_ionadmin_get_outbound_file_system_occupancy_limit);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT), dtn_ion_ionadmin_get_outbound_heap_occupancy_limit);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_PRODUCTION_RATE), dtn_ion_ionadmin_get_production_rate);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_REF_TIME), dtn_ion_ionadmin_get_ref_time);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_TIME_DELTA), dtn_ion_ionadmin_get_time_delta);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_VERSION), dtn_ion_ionadmin_get_version);
}

void dtn_ion_ionadmin_init_op()
{

}

void dtn_ion_ionadmin_init_var()
{

}

void dtn_ion_ionadmin_init_ctrl()
{

	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_INIT, 2, dtn_ion_ionadmin_ctrl_node_init);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CLOCK_ERROR_SET, 1, dtn_ion_ionadmin_ctrl_node_clock_error_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CLOCK_SYNC_SET, 1, dtn_ion_ionadmin_ctrl_node_clock_sync_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET, 1, dtn_ion_ionadmin_ctrl_node_congestion_alarm_control_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET, 1, dtn_ion_ionadmin_ctrl_node_congestion_end_time_forecasts_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONSUMPTION_RATE_SET, 1, dtn_ion_ionadmin_ctrl_node_consumption_rate_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONTACT_ADD, 6, dtn_ion_ionadmin_ctrl_node_contact_add);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONTACT_DEL, 3, dtn_ion_ionadmin_ctrl_node_contact_del);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET, 2, dtn_ion_ionadmin_ctrl_node_inbound_heap_occupancy_limit_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET, 2, dtn_ion_ionadmin_ctrl_node_outbound_heap_occupancy_limit_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_PRODUCTION_RATE_SET, 1, dtn_ion_ionadmin_ctrl_node_production_rate_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_RANGE_ADD, 5, dtn_ion_ionadmin_ctrl_node_range_add);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_RANGE_DEL, 3, dtn_ion_ionadmin_ctrl_node_range_del);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_REF_TIME_SET, 1, dtn_ion_ionadmin_ctrl_node_ref_time_set);
	adm_add_ctrldef(g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_TIME_DELTA_SET, 1, dtn_ion_ionadmin_ctrl_node_time_delta_set);
}

void dtn_ion_ionadmin_init_mac()
{

}

void dtn_ion_ionadmin_init_rpttpl()
{

}

void dtn_ion_ionadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* CONTACTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionadmin_idx[ADM_TBLT_IDX], DTN_ION_IONADMIN_TBLT_CONTACTS), dtn_ion_ionadmin_tblt_contacts);
	tblt_add_col(def, AMP_TYPE_TV, "start_time");
	tblt_add_col(def, AMP_TYPE_TV, "stop_time");
	tblt_add_col(def, AMP_TYPE_UINT, "source_node");
	tblt_add_col(def, AMP_TYPE_UINT, "dest_node");
	tblt_add_col(def, AMP_TYPE_UVAST, "xmit_data");
	tblt_add_col(def, AMP_TYPE_UVAST, "confidence");
	adm_add_tblt(def);

	/* RANGES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionadmin_idx[ADM_TBLT_IDX], DTN_ION_IONADMIN_TBLT_RANGES), dtn_ion_ionadmin_tblt_ranges);
	tblt_add_col(def, AMP_TYPE_TV, "start");
	tblt_add_col(def, AMP_TYPE_TV, "stop");
	tblt_add_col(def, AMP_TYPE_UINT, "node");
	tblt_add_col(def, AMP_TYPE_UINT, "other_node");
	tblt_add_col(def, AMP_TYPE_UINT, "distance");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_ION_IONADMIN_ADM_
