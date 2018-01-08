/****************************************************************************
 **
 ** File Name: adm_ltp_agent_mgr.c
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
#include "../shared/adm/adm_ltp_agent.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"

#define _HAVE_LTP_AGENT_ADM_
#ifdef _HAVE_LTP_AGENT_ADM_

void adm_ltp_agent_init()
{
	adm_ltp_agent_init_edd();
	adm_ltp_agent_init_variables();
	adm_ltp_agent_init_controls();
	adm_ltp_agent_init_constants();
	adm_ltp_agent_init_macros();
	adm_ltp_agent_init_metadata();
	adm_ltp_agent_init_ops();
	adm_ltp_agent_init_reports();
}


void adm_ltp_agent_init_edd()
{
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_REMOTE_ENGINE_NBR", "The remote engine number of this span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_CUR_EXPT_SESS", "Expected sessions on this span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_CUR_OUT_SEG", "The current number of outbound segments for this span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_CUR_IMP_SESS", "The current number of import segments for this span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IN_SEG_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_CUR_IN_SEG", "The current number of inbound segments for this span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_CUR_IN_SEG_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_RESET_TIME_MID), AMP_TYPE_UVAST, 0, NULL, NULL, NULL);
	names_add_name("SPAN_RESET_TIME", "The last time the span counters were reset.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_RESET_TIME_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_SEG_Q_CNT", "The output segment queued count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_SEG_Q_BYTES", "The output segment queued bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_SEG_POP_CNT", "The output segment popped count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_SEG_POP_BYTES", "The output segment popped bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_CKPT_XMIT_CNT", "The output checkpoint transmit count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_POS_ACK_RX_CNT", "The output positive acknowledgement received count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_NEG_ACK_RX_CNT", "The output negative acknowledgement received count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_CANCEL_RX_CNT", "The output cancelled received count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_CKPT_REXMIT_CNT", "The output checkpoint retransmit count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_CANCEL_XMIT_CNT", "The output cancel retransmit count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_OUT_COMPLETE_CNT", "The output completed count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_RED_CNT", "The input segment received red count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_RED_BYTES", "The input segment received red bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_GREEN_CNT", "The input segment received green count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_GREEN_BYTES", "The input segment received green bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_REDUNDANT_CNT", "The input segment received redundant count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_REDUNDANT_BYTES", "The input segment received redundant bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_MAL_CNT", "The input segment malformed count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_MAL_BYTES", "The input segment malformed bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_UNK_SENDER_CNT", "The input segment unknown sender count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_UNK_SENDER_BYTES", "The input segment unknown sender bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_UNK_CLIENT_CNT", "The input segment unknown client count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_RX_UNK_CLIENT_BYTES", "The input segment unknown client bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_STRAY_CNT", "The input segment stray count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_STRAY_BYTES", "The input segment stray bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_MISCOLOR_CNT", "The input segment miscolored count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_MISCOLOR_BYTES", "The input segment miscolored bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_CLOSED_CNT", "The input segment closed count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_SEG_CLOSED_BYTES", "The input segment closed bytes for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_CKPT_RX_CNT", "The input checkpoint receive count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_POS_ACK_TX_CNT", "The input positive acknolwedgement transmitted count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_NEG_ACK_TX_CNT", "The input negative acknolwedgement transmitted count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_CANCEL_TX_CNT", "The input cancel transmitted count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_ACK_RETX_CNT", "The input acknolwedgement retransmit count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_CANCEL_RX_CNT", "The input cancel receive count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT_MID);

	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT_MID), AMP_TYPE_UINT, 0, NULL, NULL, NULL);
	names_add_name("SPAN_IN_COMPLETE_CNT", "The input completed count for the span.", ADM_LTP_AGENT, ADM_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT_MID);

}


void adm_ltp_agent_init_variables()
{
}


void adm_ltp_agent_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_LTP_AGENT_CTRL_RESET_MID), NULL);
	names_add_name("RESET", "Resets the counters associated with the engine and updates the last reset time for the span to be the time when this control was run.", ADM_LTP_AGENT, ADM_LTP_AGENT_CTRL_RESET_MID);
	UI_ADD_PARMSPEC_1(ADM_LTP_AGENT_CTRL_RESET_MID, "ltp_span", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_LTP_AGENT_CTRL_LIST_ENGINES_MID), NULL);
	names_add_name("LIST_ENGINES", "Lists all remote engine IDs.", ADM_LTP_AGENT, ADM_LTP_AGENT_CTRL_LIST_ENGINES_MID);

}


void adm_ltp_agent_init_constants()
{
}


void adm_ltp_agent_init_macros()
{
}


void adm_ltp_agent_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(LTP_AGENT_ADM_META_NN_IDX, LTP_AGENT_ADM_META_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_EDD_NN_IDX, LTP_AGENT_ADM_EDD_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_VAR_NN_IDX, LTP_AGENT_ADM_VAR_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_RPT_NN_IDX, LTP_AGENT_ADM_RPT_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_CTRL_NN_IDX, LTP_AGENT_ADM_CTRL_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_CONST_NN_IDX, LTP_AGENT_ADM_CONST_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_MACRO_NN_IDX, LTP_AGENT_ADM_MACRO_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_OP_NN_IDX, LTP_AGENT_ADM_OP_NN_STR, "LTP_AGENT", "2017-08-17");
	oid_nn_add_parm(LTP_AGENT_ADM_ROOT_NN_IDX, LTP_AGENT_ADM_ROOT_NN_STR, "LTP_AGENT", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_NAME_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_LTP_AGENT, ADM_LTP_AGENT_META_NAME_MID);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_NAMESPACE_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM.", ADM_LTP_AGENT, ADM_LTP_AGENT_META_NAMESPACE_MID);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_VERSION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM.", ADM_LTP_AGENT, ADM_LTP_AGENT_META_VERSION_MID);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM.", ADM_LTP_AGENT, ADM_LTP_AGENT_META_ORGANIZATION_MID);
}


void adm_ltp_agent_init_ops()
{
}


void adm_ltp_agent_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = NULL;
	rpt = lyst_create();
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IN_SEG_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_RESET_TIME_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT_MID));
	adm_add_rpt(mid_from_value(ADM_LTP_AGENT_RPT_ENDPOINTREPORT_MID), rpt);
	midcol_destroy(&rpt);
	names_add_name("ENDPOINTREPORT", "This is all known endpoint information", ADM_LTP_AGENT, ADM_LTP_AGENT_RPT_ENDPOINTREPORT_MID);

}

#endif // _HAVE_LTP_AGENT_ADM_
