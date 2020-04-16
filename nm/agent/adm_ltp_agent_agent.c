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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_ltp_agent.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_ltp_agent_impl.h"
#include "agent/rda.h"



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


	dtn_ltp_agent_setup();
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

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_NAME), dtn_ltp_agent_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_NAMESPACE), dtn_ltp_agent_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_VERSION), dtn_ltp_agent_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ltp_agent_idx[ADM_META_IDX], DTN_LTP_AGENT_META_ORGANIZATION), dtn_ltp_agent_meta_organization);
}

void dtn_ltp_agent_init_cnst()
{

}

void dtn_ltp_agent_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR), dtn_ltp_agent_get_span_remote_engine_nbr);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS), dtn_ltp_agent_get_span_cur_expt_sess);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG), dtn_ltp_agent_get_span_cur_out_seg);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS), dtn_ltp_agent_get_span_cur_imp_sess);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_CUR_IN_SEG), dtn_ltp_agent_get_span_cur_in_seg);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_RESET_TIME), dtn_ltp_agent_get_span_reset_time);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT), dtn_ltp_agent_get_span_out_seg_q_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES), dtn_ltp_agent_get_span_out_seg_q_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT), dtn_ltp_agent_get_span_out_seg_pop_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES), dtn_ltp_agent_get_span_out_seg_pop_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT), dtn_ltp_agent_get_span_out_ckpt_xmit_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT), dtn_ltp_agent_get_span_out_pos_ack_rx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT), dtn_ltp_agent_get_span_out_neg_ack_rx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT), dtn_ltp_agent_get_span_out_cancel_rx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT), dtn_ltp_agent_get_span_out_ckpt_rexmit_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT), dtn_ltp_agent_get_span_out_cancel_xmit_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT), dtn_ltp_agent_get_span_out_complete_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT), dtn_ltp_agent_get_span_in_seg_rx_red_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES), dtn_ltp_agent_get_span_in_seg_rx_red_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT), dtn_ltp_agent_get_span_in_seg_rx_green_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES), dtn_ltp_agent_get_span_in_seg_rx_green_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT), dtn_ltp_agent_get_span_in_seg_rx_redundant_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES), dtn_ltp_agent_get_span_in_seg_rx_redundant_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT), dtn_ltp_agent_get_span_in_seg_rx_mal_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES), dtn_ltp_agent_get_span_in_seg_rx_mal_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT), dtn_ltp_agent_get_span_in_seg_rx_unk_sender_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES), dtn_ltp_agent_get_span_in_seg_rx_unk_sender_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT), dtn_ltp_agent_get_span_in_seg_rx_unk_client_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES), dtn_ltp_agent_get_span_in_seg_rx_unk_client_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT), dtn_ltp_agent_get_span_in_seg_stray_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES), dtn_ltp_agent_get_span_in_seg_stray_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT), dtn_ltp_agent_get_span_in_seg_miscolor_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES), dtn_ltp_agent_get_span_in_seg_miscolor_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT), dtn_ltp_agent_get_span_in_seg_closed_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES), dtn_ltp_agent_get_span_in_seg_closed_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT), dtn_ltp_agent_get_span_in_ckpt_rx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT), dtn_ltp_agent_get_span_in_pos_ack_tx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT), dtn_ltp_agent_get_span_in_neg_ack_tx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT), dtn_ltp_agent_get_span_in_cancel_tx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT), dtn_ltp_agent_get_span_in_ack_retx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT), dtn_ltp_agent_get_span_in_cancel_rx_cnt);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_ltp_agent_idx[ADM_EDD_IDX], DTN_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT), dtn_ltp_agent_get_span_in_complete_cnt);
}

void dtn_ltp_agent_init_op()
{

}

void dtn_ltp_agent_init_var()
{

}

void dtn_ltp_agent_init_ctrl()
{

	adm_add_ctrldef(g_dtn_ltp_agent_idx[ADM_CTRL_IDX], DTN_LTP_AGENT_CTRL_RESET, 1, dtn_ltp_agent_ctrl_reset);
}

void dtn_ltp_agent_init_mac()
{

}

void dtn_ltp_agent_init_rpttpl()
{

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
}

void dtn_ltp_agent_init_tblt()
{

	tblt_t *def = NULL;

	/* ENGINES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ltp_agent_idx[ADM_TBLT_IDX], DTN_LTP_AGENT_TBLT_ENGINES), dtn_ltp_agent_tblt_engines);
	tblt_add_col(def, AMP_TYPE_UVAST, "peer_engine_nbr");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_LTP_AGENT_ADM_
