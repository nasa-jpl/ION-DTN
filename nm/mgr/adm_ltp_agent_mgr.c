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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_ltp_agent.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "metadata.h"
#include "nm_mgr_ui.h"




#define _HAVE_DTN_LTP_AGENT_ADM_
#ifdef _HAVE_DTN_LTP_AGENT_ADM_
static vec_idx_t g_dtn_ltp_agent_idx[11];

void dtn_ltp_agent_init()
{
	adm_add_adm_info("dtn_ltp_agent", ADM_ENUM_DTN_LTP_AGENT);

	VDB_ADD_NN(((ADM_ENUM_DTN_LTP_AGENT * 20) + ADM_META_IDX), &(g_dtn_ltp_agent_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_LTP_AGENT * 20) + ADM_TBLT_IDX), &(g_dtn_ltp_agent_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_LTP_AGENT * 20) + ADM_RPTT_IDX), &(g_dtn_ltp_agent_idx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_LTP_AGENT * 20) + ADM_EDD_IDX), &(g_dtn_ltp_agent_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_LTP_AGENT * 20) + ADM_CTRL_IDX), &(g_dtn_ltp_agent_idx[ADM_CTRL_IDX]));


	dtn_ltp_agent_init_meta();
	dtn_ltp_agent_init_cnst();
	dtn_ltp_agent_init_edd();
	dtn_ltp_agent_init_op();
	dtn_ltp_agent_init_var();
	dtn_ltp_agent_init_ctrl();
	dtn_ltp_agent_init_mac();
	dtn_ltp_agent_init_rpttpl();
	dtn_ltp_agent_init_tblt();
}

void dtn_ltp_agent_init_meta()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_LTP_AGENT, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_LTP_AGENT, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_LTP_AGENT, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_LTP_AGENT, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_ltp_agent_init_cnst()
{

}

void dtn_ltp_agent_init_edd()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_remote_engine_nbr", "The remote engine number of this span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_cur_expt_sess", "Expected sessions on this span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_cur_out_seg", "The current number of outbound segments for this span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_cur_imp_sess", "The current number of import segments for this span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_IN_SEG);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_cur_in_seg", "The current number of inbound segments for this span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_RESET_TIME);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_LTP_AGENT, "span_reset_time", "The last time the span counters were reset.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_seg_q_cnt", "The output segment queued count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_seg_q_bytes", "The output segment queued bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_seg_pop_cnt", "The output segment popped count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_seg_pop_bytes", "The output segment popped bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_ckpt_xmit_cnt", "The output checkpoint transmit count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_pos_ack_rx_cnt", "The output positive acknowledgement received count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_neg_ack_rx_cnt", "The output negative acknowledgement received count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_cancel_rx_cnt", "The output cancelled received count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_ckpt_rexmit_cnt", "The output checkpoint retransmit count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_cancel_xmit_cnt", "The output cancel retransmit count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_out_complete_cnt", "The output completed count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_red_cnt", "The input segment received red count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_red_bytes", "The input segment received red bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_green_cnt", "The input segment received green count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_green_bytes", "The input segment received green bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_redundant_cnt", "The input segment received redundant count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_redundant_bytes", "The input segment received redundant bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_mal_cnt", "The input segment malformed count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_mal_bytes", "The input segment malformed bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_unk_sender_cnt", "The input segment unknown sender count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_unk_sender_bytes", "The input segment unknown sender bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_unk_client_cnt", "The input segment unknown client count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_rx_unk_client_bytes", "The input segment unknown client bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_stray_cnt", "The input segment stray count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_stray_bytes", "The input segment stray bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_miscolor_cnt", "The input segment miscolored count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_miscolor_bytes", "The input segment miscolored bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_closed_cnt", "The input segment closed count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_seg_closed_bytes", "The input segment closed bytes for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_ckpt_rx_cnt", "The input checkpoint receive count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_pos_ack_tx_cnt", "The input positive acknolwedgement transmitted count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_neg_ack_tx_cnt", "The input negative acknolwedgement transmitted count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_cancel_tx_cnt", "The input cancel transmitted count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_ack_retx_cnt", "The input acknolwedgement retransmit count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_cancel_rx_cnt", "The input cancel receive count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_LTP_AGENT, "span_in_complete_cnt", "The input completed count for the span.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
}

void dtn_ltp_agent_init_op()
{

}

void dtn_ltp_agent_init_var()
{

}

void dtn_ltp_agent_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* RESET */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ltp_agent_idx[ADM_CTRL_IDX], DTN_LTP_AGENT_CTRL_RESET);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_LTP_AGENT, "reset", "Resets the counters associated with the engine and updates the last reset time for the span to be the time when this control was run.");

	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
}

void dtn_ltp_agent_init_mac()
{

}

void dtn_ltp_agent_init_rpttpl()
{

	metadata_t *meta = NULL;

	rpttpl_t *def = NULL;

	/* ENDPOINTREPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 1, g_dtn_ltp_agent_idx[ADM_RPTT_IDX], DTN_LTP_AGENT_RPTTPL_ENDPOINTREPORT));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_IN_SEG, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_RESET_TIME, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT, tnv_from_map(AMP_TYPE_UINT, 0)));
	adm_add_rpttpl(def);
	meta = meta_add_rpttpl(def->id, ADM_ENUM_DTN_LTP_AGENT, "endpointReport", "This is all known endpoint information");
	meta_add_parm(meta, "ltp_span", AMP_TYPE_UINT);
}

void dtn_ltp_agent_init_tblt()
{

	tblt_t *def = NULL;

	/* ENGINES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ltp_agent_idx[ADM_TBLT_IDX], DTN_LTP_AGENT_TBLT_ENGINES), NULL);
	tblt_add_col(def, AMP_TYPE_UVAST, "peer_engine_nbr");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_LTP_AGENT, "engines", "This table lists all known remote engine ids.");
}

#endif // _HAVE_DTN_LTP_AGENT_ADM_
