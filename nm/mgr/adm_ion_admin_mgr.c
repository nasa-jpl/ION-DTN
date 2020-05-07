/****************************************************************************
 **
 ** File Name: adm_ion_admin_mgr.c
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
#include "metadata.h"
#include "nm_mgr_ui.h"




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

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONADMIN, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONADMIN, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONADMIN, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ionadmin_idx[ADM_META_IDX], DTN_ION_IONADMIN_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONADMIN, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_ion_ionadmin_init_cnst()
{

}

void dtn_ion_ionadmin_init_edd()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CLOCK_ERROR);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "clock_error", "This is how accurate the ION Agent's clock is described as number of seconds, an absolute value.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CLOCK_SYNC);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "clock_sync", "This is whether or not the the computer on which the local ION node is running has a synchronized clock.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CONGESTION_ALARM_CONTROL);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "congestion_alarm_control", "This is whether or not the node has a control that will set off alarm if it will become congested at some future time.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CONGESTION_END_TIME_FORECASTS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "congestion_end_time_forecasts", "This is the time horizon beyond which we don't attempt to forecast congestion");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_CONSUMPTION_RATE);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "consumption_rate", "This is the mean rate of continuous data delivery to local BP applications.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "inbound_file_system_occupancy_limit", "This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of inbound zero-copy objects. The default heap limit is 1 Terabyte.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "inbound_heap_occupancy_limit", "This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. The default heap limit is 20% of the SDR data space's total heap size.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_NUMBER);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "number", "This is a CBHE node number which uniquely identifies the node in the delay-tolerant network.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "outbound_file_system_occupancy_limit", "This is the maximum number of megabytes of storage space in ION's local file system that can be used for the storage of outbound zero-copy objects. The default heap limit is 1 Terabyte.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "outbound_heap_occupancy_limit", "This is the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects. The default heap limit is 20% of the SDR data space's total heap size.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_PRODUCTION_RATE);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "production_rate", "This is the rate of local data production.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_REF_TIME);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_TV, id, ADM_ENUM_DTN_ION_IONADMIN, "ref_time", "This is the reference time that will be used for interpreting relative time values from now until the next revision of reference time.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_TIME_DELTA);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_ION_IONADMIN, "time_delta", "The time delta is used to compensate for error (drift) in clocks, particularly spacecraft clocks. The hardware clock on a spacecraft might gain or lose a few seconds every month, to the point at which its understanding of the current time - as reported out by the operating system - might differ significantly from the actual value of Unix Epoch time as reported by authoritative clocks on Earth. To compensate for this difference without correcting the clock itself (which can be difficult and dangerous), ION simply adds the time delta to the Epoch time reported by the operating system.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ionadmin_idx[ADM_EDD_IDX], DTN_ION_IONADMIN_EDD_VERSION);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_IONADMIN, "version", "This is the version of ION that is currently installed.");

}

void dtn_ion_ionadmin_init_op()
{

}

void dtn_ion_ionadmin_init_var()
{

}

void dtn_ion_ionadmin_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* NODE_INIT */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_INIT);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_init", "Until this control is executed, the local ION node does not exist and most ionadmin controls will fail. The control configures the local node to be identified by node_number, a CBHE node number which uniquely identifies the node in the delay-tolerant network.  It also configures ION's data space (SDR) and shared working-memory region.  For this purpose it uses a set of default settings if no argument follows node_number or if the argument following node_number is ''; otherwise it uses the configuration settings found in a configuration file.  If configuration file name is provided, then the configuration file's name is implicitly 'hostname.ionconfig'; otherwise, ion_config_filename is taken to be the explicit configuration file name.");

	meta_add_parm(meta, "node_nbr", AMP_TYPE_UINT);
	meta_add_parm(meta, "config_file", AMP_TYPE_STR);

	/* NODE_CLOCK_ERROR_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CLOCK_ERROR_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_clock_error_set", "This management control sets ION's understanding of the accuracy of the scheduled start and stop times of planned contacts, in seconds.  The default value is 1.");

	meta_add_parm(meta, "known_maximum_clock_error", AMP_TYPE_UINT);

	/* NODE_CLOCK_SYNC_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CLOCK_SYNC_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_clock_sync_set", "This management control reports whether or not the computer on which the local ION node is running has a synchronized clock.");

	meta_add_parm(meta, "new_state", AMP_TYPE_BOOL);

	/* NODE_CONGESTION_ALARM_CONTROL_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_congestion_alarm_control_set", "This management control establishes a control which will automatically be executed whenever ionadmin predicts that the node will become congested at some future time.");

	meta_add_parm(meta, "congestion_alarm_control", AMP_TYPE_STR);

	/* NODE_CONGESTION_END_TIME_FORECASTS_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_congestion_end_time_forecasts_set", "This management control sets the end time for computed congestion forecasts. Setting congestion forecast horizon to zero sets the congestion forecast end time to infinite time in the future: if there is any predicted net growth in bundle storage space occupancy at all, following the end of the last scheduled contact, then eventual congestion will be predicted. The default value is zero, i.e., no end time.");

	meta_add_parm(meta, "end_time_for_congestion_forcasts", AMP_TYPE_UINT);

	/* NODE_CONSUMPTION_RATE_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONSUMPTION_RATE_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_consumption_rate_set", "This management control sets ION's expectation of the mean rate of continuous data delivery to local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data consumption is unknown; in that case local data consumption is not considered in the computation of congestion forecasts.");

	meta_add_parm(meta, "planned_data_consumption_rate", AMP_TYPE_UINT);

	/* NODE_CONTACT_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONTACT_ADD);
	adm_add_ctrldef_ari(id, 6, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_contact_add", "This control schedules a period of data transmission from source_node to dest_node. The period of transmission will begin at start_time and end at stop_time, and the rate of data transmission will be xmit_data_rate bytes/second. Our confidence in the contact defaults to 1.0, indicating that the contact is scheduled - not that non-occurrence of the contact is impossible, just that occurrence of the contact is planned and scheduled rather than merely imputed from past node behavior. In the latter case, confidence indicates our estimation of the likelihood of this potential contact.");

	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "stop", AMP_TYPE_TV);
	meta_add_parm(meta, "from_node_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "to_node_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "data_rate", AMP_TYPE_UVAST);
	meta_add_parm(meta, "prob", AMP_TYPE_UVAST);

	/* NODE_CONTACT_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_CONTACT_DEL);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_contact_del", "This control deletes the scheduled period of data transmission from source_node to dest_node starting at start_time. To delete all contacts between some pair of nodes, use '*' as start_time.");

	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "node_id", AMP_TYPE_UINT);
	meta_add_parm(meta, "dest", AMP_TYPE_STR);

	/* NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_inbound_heap_occupancy_limit_set", "This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of inbound zero-copy objects. A value of -1 for either limit signifies 'leave unchanged'. The default heap limit is 30% of the SDR data space's total heap size.");

	meta_add_parm(meta, "heap_occupancy_limit", AMP_TYPE_UINT);
	meta_add_parm(meta, "file_system_occupancy_limit", AMP_TYPE_UINT);

	/* NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_outbound_heap_occupancy_limit_set", "This management control sets the maximum number of megabytes of storage space in ION's SDR non-volatile heap that can be used for the storage of outbound zero-copy objects.  A value of  -1 for either limit signifies 'leave unchanged'. The default heap  limit is 30% of the SDR data space's total heap size.");

	meta_add_parm(meta, "heap_occupancy_limit", AMP_TYPE_UINT);
	meta_add_parm(meta, "file_system_occupancy_limit", AMP_TYPE_UINT);

	/* NODE_PRODUCTION_RATE_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_PRODUCTION_RATE_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_production_rate_set", "This management control sets ION's expectation of the mean rate of continuous data origination by local BP applications throughout the period of time over which congestion forecasts are computed. For nodes that function only as routers this variable will normally be zero. A value of -1, which is the default, indicates that the rate of local data production is unknown; in that case local data production is not considered in the computation of congestion forecasts.");

	meta_add_parm(meta, "planned_data_production_rate", AMP_TYPE_UINT);

	/* NODE_RANGE_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_RANGE_ADD);
	adm_add_ctrldef_ari(id, 5, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_range_add", "This control predicts a period of time during which the distance from node to other_node will be constant to within one light second. The period will begin at start_time and end at stop_time, and the distance between the nodes during that time will be distance light seconds.");

	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "stop", AMP_TYPE_TV);
	meta_add_parm(meta, "node", AMP_TYPE_UINT);
	meta_add_parm(meta, "other_node", AMP_TYPE_UINT);
	meta_add_parm(meta, "distance", AMP_TYPE_UINT);

	/* NODE_RANGE_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_RANGE_DEL);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_range_del", "This control deletes the predicted period of constant distance between node and other_node starting at start_time. To delete all ranges between some pair of nodes, use '*' as start_time.");

	meta_add_parm(meta, "start", AMP_TYPE_TV);
	meta_add_parm(meta, "node", AMP_TYPE_UINT);
	meta_add_parm(meta, "other_node", AMP_TYPE_UINT);

	/* NODE_REF_TIME_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_REF_TIME_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_ref_time_set", "This is used to set the reference time that will be used for interpreting relative time values from now until the next revision of reference time. Note that the new reference time can be a relative time, i.e., an offset beyond the current reference time.");

	meta_add_parm(meta, "time", AMP_TYPE_TV);

	/* NODE_TIME_DELTA_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ionadmin_idx[ADM_CTRL_IDX], DTN_ION_IONADMIN_CTRL_NODE_TIME_DELTA_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_IONADMIN, "node_time_delta_set", "This management control sets ION's understanding of the current difference between correct time and the Unix Epoch time values reported by the clock for the local ION node's computer. This delta is automatically applied to locally obtained time values whenever ION needs to know the current time.");

	meta_add_parm(meta, "local_time_sec_after_epoch", AMP_TYPE_UINT);
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

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionadmin_idx[ADM_TBLT_IDX], DTN_ION_IONADMIN_TBLT_CONTACTS), NULL);
	tblt_add_col(def, AMP_TYPE_TV, "start_time");
	tblt_add_col(def, AMP_TYPE_TV, "stop_time");
	tblt_add_col(def, AMP_TYPE_UINT, "source_node");
	tblt_add_col(def, AMP_TYPE_UINT, "dest_node");
	tblt_add_col(def, AMP_TYPE_UVAST, "xmit_data");
	tblt_add_col(def, AMP_TYPE_UVAST, "confidence");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_IONADMIN, "contacts", "This table shows all scheduled periods of data transmission.");

	/* RANGES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ionadmin_idx[ADM_TBLT_IDX], DTN_ION_IONADMIN_TBLT_RANGES), NULL);
	tblt_add_col(def, AMP_TYPE_TV, "start");
	tblt_add_col(def, AMP_TYPE_TV, "stop");
	tblt_add_col(def, AMP_TYPE_UINT, "node");
	tblt_add_col(def, AMP_TYPE_UINT, "other_node");
	tblt_add_col(def, AMP_TYPE_UINT, "distance");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_IONADMIN, "ranges", "This table shows all predicted periods of constant distance between nodes.");
}

#endif // _HAVE_DTN_ION_IONADMIN_ADM_
