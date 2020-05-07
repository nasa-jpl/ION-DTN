/****************************************************************************
 **
 ** File Name: adm_ion_ltp_admin_mgr.c
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
#include "adm_ion_ltp_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "metadata.h"
#include "nm_mgr_ui.h"




#define _HAVE_DTN_ION_LTPADMIN_ADM_
#ifdef _HAVE_DTN_ION_LTPADMIN_ADM_
static vec_idx_t g_dtn_ion_ltpadmin_idx[11];

void dtn_ion_ltpadmin_init()
{
	adm_add_adm_info("dtn_ion_ltpadmin", ADM_ENUM_DTN_ION_LTPADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_EDD_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_LTPADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_ltpadmin_init_meta();
	dtn_ion_ltpadmin_init_cnst();
	dtn_ion_ltpadmin_init_edd();
	dtn_ion_ltpadmin_init_op();
	dtn_ion_ltpadmin_init_var();
	dtn_ion_ltpadmin_init_ctrl();
	dtn_ion_ltpadmin_init_mac();
	dtn_ion_ltpadmin_init_rpttpl();
	dtn_ion_ltpadmin_init_tblt();
}

void dtn_ion_ltpadmin_init_meta()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_LTPADMIN, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_LTPADMIN, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_LTPADMIN, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_ltpadmin_idx[ADM_META_IDX], DTN_ION_LTPADMIN_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_LTPADMIN, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_ion_ltpadmin_init_cnst()
{

}

void dtn_ion_ltpadmin_init_edd()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_ltpadmin_idx[ADM_EDD_IDX], DTN_ION_LTPADMIN_EDD_ION_VERSION);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_LTPADMIN, "ion_version", "This is the version of ION that is currently installed.");

}

void dtn_ion_ltpadmin_init_op()
{

}

void dtn_ion_ltpadmin_init_var()
{

}

void dtn_ion_ltpadmin_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* MANAGE_HEAP */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_HEAP);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "manage_heap", "This control declares the maximum number of bytes of SDR heap space that will be occupied by the acquisition of any single LTP block. All data acquired in excess of this limit will be written to a temporary file pending extraction and dispatching of the acquired block. Default is the minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimum SDR heap space occupancy in the event that all acquisition is into a file.");

	meta_add_parm(meta, "max_database_heap_per_block", AMP_TYPE_UINT);

	/* MANAGE_MAX_BER */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_MAX_BER);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "manage_max_ber", "This control sets the expected maximum bit error rate(BER) that LTP should provide for in computing the maximum number of transmission efforts to initiate in the transmission of a given block.(Note that this computation is also sensitive to data segment size and to the size of the block that is to be transmitted.) The default value is .0001 (10^-4).");

	meta_add_parm(meta, "max_expected_bit_error_rate", AMP_TYPE_REAL32);

	/* MANAGE_OWN_QUEUE_TIME */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_OWN_QUEUE_TIME);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "manage_own_queue_time", "This control sets the number of seconds of predicted additional latency attributable to processing delay within the local engine itself that should be included whenever LTP computes the nominal round-trip time for an exchange of data with any remote engine.The default value is 1.");

	meta_add_parm(meta, "own_queing_latency", AMP_TYPE_UINT);

	/* MANAGE_SCREENING */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_MANAGE_SCREENING);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "manage_screening", "This control enables or disables the screening of received LTP segments per the periods of scheduled reception in the node's contact graph. By default, screening is disabled. When screening is enabled, such segments are silently discarded. Note that when screening is enabled the ranges declared in the contact graph must be accurate and clocks must be synchronized; otherwise, segments will be arriving at times other than the scheduled contact intervals and will be discarded.");

	meta_add_parm(meta, "new_state", AMP_TYPE_UINT);

	/* SPAN_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_SPAN_ADD);
	adm_add_ctrldef_ari(id, 8, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "span_add", "This control declares that a span of potential LTP data interchange exists between the local LTP engine and the indicated (neighboring) LTP engine.");

	meta_add_parm(meta, "peer_engine_number", AMP_TYPE_UVAST);
	meta_add_parm(meta, "max_export_sessions", AMP_TYPE_UINT);
	meta_add_parm(meta, "max_import_sessions", AMP_TYPE_UINT);
	meta_add_parm(meta, "max_segment_size", AMP_TYPE_UINT);
	meta_add_parm(meta, "aggregtion_size_limit", AMP_TYPE_UINT);
	meta_add_parm(meta, "aggregation_time_limit", AMP_TYPE_UINT);
	meta_add_parm(meta, "lso_control", AMP_TYPE_STR);
	meta_add_parm(meta, "queuing_latency", AMP_TYPE_UINT);

	/* SPAN_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_SPAN_CHANGE);
	adm_add_ctrldef_ari(id, 8, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "span_change", "This control sets the indicated span's configuration parameters to the values provided as arguments");

	meta_add_parm(meta, "peer_engine_number", AMP_TYPE_UVAST);
	meta_add_parm(meta, "max_export_sessions", AMP_TYPE_UINT);
	meta_add_parm(meta, "max_import_sessions", AMP_TYPE_UINT);
	meta_add_parm(meta, "max_segment_size", AMP_TYPE_UINT);
	meta_add_parm(meta, "aggregtion_size_limit", AMP_TYPE_UINT);
	meta_add_parm(meta, "aggregation_time_limit", AMP_TYPE_UINT);
	meta_add_parm(meta, "lso_control", AMP_TYPE_STR);
	meta_add_parm(meta, "queuing_latency", AMP_TYPE_UINT);

	/* SPAN_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_SPAN_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "span_del", "This control deletes the span identified by peerEngineNumber. The control will fail if any outbound segments for this span are pending transmission or any inbound blocks from the peer engine are incomplete.");

	meta_add_parm(meta, "peer_engine_number", AMP_TYPE_UVAST);

	/* STOP */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_STOP);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "stop", "This control stops all link service input and output tasks for the local LTP engine.");


	/* WATCH_SET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_ltpadmin_idx[ADM_CTRL_IDX], DTN_ION_LTPADMIN_CTRL_WATCH_SET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_LTPADMIN, "watch_set", "This control enables and disables production of a continuous stream of user- selected LTP activity indication characters. Activity parameter of 1 selects all LTP activity indication characters; 0 de-selects all LTP activity indication characters; any other activitySpec such as df{] selects all activity indication characters in the string, de-selecting all others. LTP will print each selected activity indication character to stdout every time a processing event of the associated type occurs: d bundle appended to block for next session, e segment of block is queued for transmission, f block has been fully segmented for transmission, g segment popped from transmission queue, h positive ACK received for block and session ended, s segment received, t block has been fully received, @ negative ACK received for block and segments retransmitted, = unacknowledged checkpoint was retransmitted, + unacknowledged report segment was retransmitted, { export session canceled locally (by sender), } import session canceled by remote sender, [ import session canceled locally (by receiver), ] export session canceled by remote receiver");

	meta_add_parm(meta, "activity", AMP_TYPE_STR);
}

void dtn_ion_ltpadmin_init_mac()
{

}

void dtn_ion_ltpadmin_init_rpttpl()
{

}

void dtn_ion_ltpadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* SPANS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_ltpadmin_idx[ADM_TBLT_IDX], DTN_ION_LTPADMIN_TBLT_SPANS), NULL);
	tblt_add_col(def, AMP_TYPE_UVAST, "peer_engine_nbr");
	tblt_add_col(def, AMP_TYPE_UINT, "max_export_sessions");
	tblt_add_col(def, AMP_TYPE_UINT, "max_import_sessions");
	tblt_add_col(def, AMP_TYPE_UINT, "max_segment_size");
	tblt_add_col(def, AMP_TYPE_UINT, "aggregation_size_limit");
	tblt_add_col(def, AMP_TYPE_UINT, "aggregation_time_limit");
	tblt_add_col(def, AMP_TYPE_STR, "lso_control");
	tblt_add_col(def, AMP_TYPE_UINT, "queueing_latency");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_LTPADMIN, "spans", "This table lists all spans of potential LTP data interchange that exists between the local LTP engine and the indicated (neighboring) LTP engine.");
}

#endif // _HAVE_DTN_ION_LTPADMIN_ADM_
