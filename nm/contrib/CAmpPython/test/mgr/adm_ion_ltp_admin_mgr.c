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
 **  2018-03-16  AUTO             Auto-generated c file 
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
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"


#define _HAVE_ION_LTP_ADMIN_ADM_
#ifdef _HAVE_ION_LTP_ADMIN_ADM_
void adm_ion_ltp_admin_init()
{

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
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_EDD_VERSION_MID), AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("VERSION", "This is the version of ION that is currently installed.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_EDD_VERSION_MID);


}

void adm_ion_ltp_admin_init_variables()
{

}

void adm_ion_ltp_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_INIT_MID), NULL);
	names_add_name("INIT", "Until this control is executed, LTP is not in operation on the local ION node and most ltpadmin controls will fail. The control uses estMaxExportSessions to configure the hashtable it will use to manage access to export transmission sessions that are currently in progress. For optimum performance, estMaxExportSessions should normally equal or exceed the summation of maxExportSessions over all spans as discussed below. Appropriate values for the parameters configuring each \"span\" of potential LTP data exchange between the local LTP and neighboring engines are non-trivial to determine.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_INIT_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_INIT_MID, "est_max_export_sessions", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_HEAP_MID), NULL);
	names_add_name("MANAGE_HEAP", "This control declares the maximum number of bytes of SDR heap space that will be occupied by the acquisition of any single LTP block.  All data acquired in excess of this limit will be written to a temporary file pending extraction and dispatching of the acquired block. Default is the minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimum SDR heap space occupancy in the event that all acquisition is into a file.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_MANAGE_HEAP_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_HEAP_MID, "max_database_heap_per_block", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_MAX_BER_MID), NULL);
	names_add_name("MANAGE_MAX_BER", "This control sets the expected maximum bit error rate(BER) that LTP should provide for in computing the maximum number of transmission efforts to initiate in the transmission of a given block.(Note that this computation is also sensitive to data segment size and to the size of the block that is to be transmitted.) The default value is .0001 (10^-4).", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_MANAGE_MAX_BER_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_MAX_BER_MID, "max_expected_bit_error_rate", AMP_TYPE_REAL32);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_OWN_QUEUE_TIME_MID), NULL);
	names_add_name("MANAGE_OWN_QUEUE_TIME", "This control sets the number of seconds of predicted additional latency attributable to processing delay within the local engine itself that should be included whenever LTP computes the nominal round-trip time for an exchange of data with any remote engine. The default value is 1.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_MANAGE_OWN_QUEUE_TIME_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_OWN_QUEUE_TIME_MID, "own_queing_latency", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_MANAGE_SCREENING_MID), NULL);
	names_add_name("MANAGE_SCREENING", "This control enables or disables the screening of received LTP segments per the periods of scheduled reception in the node's contact graph.  By default, screening is disabled. When screening is enabled, such segments are silently discarded.  Note that when screening is enabled the ranges declared in the contact graph must be accurate and clocks must be synchronized; otherwise, segments will be arriving at times other than the scheduled contact intervals and will be discarded.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_MANAGE_SCREENING_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_MANAGE_SCREENING_MID, "new_state", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_SPAN_ADD_MID), NULL);
	names_add_name("SPAN_ADD", "This control declares that a span of potential LTP data interchange exists between the local LTP engine and the indicated (neighboring) LTP engine.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_SPAN_ADD_MID);
	UI_ADD_PARMSPEC_8(ADM_ION_LTP_ADMIN_CTRL_SPAN_ADD_MID, "peer_engine_number", AMP_TYPE_UINT, "max_export_sessions", AMP_TYPE_UINT, "max_import_sessions", AMP_TYPE_UINT, "max_segment_size", AMP_TYPE_UINT, "aggregtion_size_limit", AMP_TYPE_UINT, "aggregation_time_limit", AMP_TYPE_UINT, "lso_control", AMP_TYPE_STR, "queuing_latency", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_SPAN_CHANGE_MID), NULL);
	names_add_name("SPAN_CHANGE", "This control sets the indicated span's configuration parameters to the values provided as arguments", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_SPAN_CHANGE_MID);
	UI_ADD_PARMSPEC_8(ADM_ION_LTP_ADMIN_CTRL_SPAN_CHANGE_MID, "peer_engine_number", AMP_TYPE_UINT, "max_export_sessions", AMP_TYPE_UINT, "max_import_sessions", AMP_TYPE_UINT, "max_segment_size", AMP_TYPE_UINT, "aggregtion_size_limit", AMP_TYPE_UINT, "aggregation_time_limit", AMP_TYPE_UINT, "lso_control", AMP_TYPE_STR, "queuing_latency", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_SPAN_DEL_MID), NULL);
	names_add_name("SPAN_DEL", "This control deletes the span identified by peerEngineNumber. The control will fail if any outbound segments for this span are pending transmission or any inbound blocks from the peer engine are incomplete.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_SPAN_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_LTP_ADMIN_CTRL_SPAN_DEL_MID, "peer_engine_number", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_STOP_MID), NULL);
	names_add_name("STOP", "This control stops all link service input and output tasks for the local LTP engine.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_STOP_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_LTP_ADMIN_CTRL_WATCH_SET_MID), NULL);
	names_add_name("WATCH_SET", "This control enables and disables production of a continuous stream of user- selected LTP activity indication characters. Activity parameter of \"1\" selects all LTP activity indication characters; \"0\" de-selects all LTP activity indication characters; any other activitySpec such as \"df{]\" selects all activity indication characters in the string, de-selecting all others. LTP will print each selected activity indication character to stdout every time a processing event of the associated type occurs:\
 d    bundle appended to block for next session\
 e    segment of block is queued for transmission\
 f    block has been fully segmented for transmission\
 g    segment popped from transmission queue\
 h    positive ACK received for block, session ended\
 s    segment received\
 t    block has been fully received\
 @    negative ACK received for block, segments retransmitted\
 =    unacknowledged checkpoint was retransmitted\
 +    unacknowledged report segment was retransmitted\
 {    export session canceled locally (by sender)\
 }    import session canceled by remote sender\
 [    import session canceled locally (by receiver)\
 ]    export session canceled by remote receiver", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_CTRL_WATCH_SET_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_LTP_ADMIN_CTRL_WATCH_SET_MID, "status", AMP_TYPE_UINT, "activity", AMP_TYPE_UINT);


}

void adm_ion_ltp_admin_init_constants()
{

}

void adm_ion_ltp_admin_init_macros()
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
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_META_NAME_MID);
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_META_NAMESPACE_MID);
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_META_VERSION_MID);
	adm_add_edd(mid_from_value(ADM_ION_LTP_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM.", ADM_ION_LTP_ADMIN, ADM_ION_LTP_ADMIN_META_ORGANIZATION_MID);

}

void adm_ion_ltp_admin_init_ops()
{

}

void adm_ion_ltp_admin_init_reports()
{

	Lyst rpt                = NULL;
	mid_t *cur_mid          = NULL;
	rpttpl_item_t *cur_item = NULL;
	uint32_t used           = 0;
	
	
}

#endif // _HAVE_ION_LTP_ADMIN_ADM_
