/****************************************************************************
 **
 ** File Name: adm_ltp_agent_agent.c
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
#include "adm_ltp_agent_impl.h"
#include "rda.h"

#define _HAVE_LTP_AGENT_ADM_
#ifdef _HAVE_LTP_AGENT_ADM_

void adm_ltp_agent_init()
{
	adm_ltp_agent_setup();
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
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_remote_engine_nbr, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_cur_expt_sess, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_cur_out_seg, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_cur_imp_sess, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_CUR_IN_SEG_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_cur_in_seg, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_RESET_TIME_MID), AMP_TYPE_UVAST, 0, adm_ltp_agent_get_span_reset_time, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_seg_q_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_seg_q_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_seg_pop_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_seg_pop_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_ckpt_xmit_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_pos_ack_rx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_neg_ack_rx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_cancel_rx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_ckpt_rexmit_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_cancel_xmit_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_out_complete_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_red_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_red_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_green_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_green_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_redundant_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_redundant_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_mal_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_mal_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_unk_sender_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_unk_sender_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_unk_client_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_rx_unk_client_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_stray_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_stray_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_miscolor_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_miscolor_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_closed_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_seg_closed_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_ckpt_rx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_pos_ack_tx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_neg_ack_tx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_cancel_tx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_ack_retx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_cancel_rx_cnt, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT_MID), AMP_TYPE_UINT, 0, adm_ltp_agent_get_span_in_complete_cnt, NULL, NULL);

}

void adm_ltp_agent_init_variables()
{
}

void adm_ltp_agent_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_LTP_AGENT_CTRL_RESET_MID),adm_ltp_agent_ctrl_reset);
	adm_add_ctrl(mid_from_value(ADM_LTP_AGENT_CTRL_LIST_ENGINES_MID),adm_ltp_agent_ctrl_list_engines);
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
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_NAME_MID), AMP_TYPE_STR, 0, adm_ltp_agent_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_ltp_agent_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_VERSION_MID), AMP_TYPE_STR, 0, adm_ltp_agent_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_LTP_AGENT_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_ltp_agent_meta_organization, adm_print_string, adm_size_string);
}

void adm_ltp_agent_init_ops()
{
}

void adm_ltp_agent_init_reports()
{
	Lyst rpt = NULL;
	uint32_t used= 0;
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


}

#endif // _HAVE_LTP_AGENT_ADM_
