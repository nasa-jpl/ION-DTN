/****************************************************************************
 **
 ** File Name: adm_ltp_agent_impl.h
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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_LTP_AGENT_IMPL_H_
#define ADM_LTP_AGENT_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#include "ltpnm.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */
/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ltp_agent_setup();
void dtn_ltp_agent_cleanup();


/* Metadata Functions */
tnv_t *dtn_ltp_agent_meta_name(tnvc_t *parms);
tnv_t *dtn_ltp_agent_meta_namespace(tnvc_t *parms);
tnv_t *dtn_ltp_agent_meta_version(tnvc_t *parms);
tnv_t *dtn_ltp_agent_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_ltp_agent_get_span_remote_engine_nbr(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_cur_expt_sess(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_cur_out_seg(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_cur_imp_sess(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_cur_in_seg(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_reset_time(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_seg_q_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_seg_q_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_seg_pop_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_seg_pop_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_ckpt_xmit_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_pos_ack_rx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_neg_ack_rx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_cancel_rx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_ckpt_rexmit_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_cancel_xmit_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_out_complete_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_red_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_red_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_green_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_green_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_redundant_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_redundant_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_mal_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_mal_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_sender_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_sender_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_client_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_client_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_stray_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_stray_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_miscolor_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_miscolor_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_closed_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_seg_closed_bytes(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_ckpt_rx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_pos_ack_tx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_neg_ack_tx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_cancel_tx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_ack_retx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_cancel_rx_cnt(tnvc_t *parms);
tnv_t *dtn_ltp_agent_get_span_in_complete_cnt(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_ltp_agent_ctrl_reset(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_ltp_agent_tblt_engines(ari_t *id);

#endif //ADM_LTP_AGENT_IMPL_H_
