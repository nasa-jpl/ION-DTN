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
 **  2018-02-07  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_LTP_AGENT_IMPL_H_
#define ADM_LTP_AGENT_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#include "ltpnm.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"

/*   START typeENUM */
/*   STOP typeENUM  */

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ltp_agent_setup();
void adm_ltp_agent_cleanup();

/* Metadata Functions */
value_t adm_ltp_agent_meta_name(tdc_t params);
value_t adm_ltp_agent_meta_namespace(tdc_t params);

value_t adm_ltp_agent_meta_version(tdc_t params);

value_t adm_ltp_agent_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_ltp_agent_get_span_remote_engine_nbr(tdc_t params);
value_t adm_ltp_agent_get_span_cur_expt_sess(tdc_t params);
value_t adm_ltp_agent_get_span_cur_out_seg(tdc_t params);
value_t adm_ltp_agent_get_span_cur_imp_sess(tdc_t params);
value_t adm_ltp_agent_get_span_cur_in_seg(tdc_t params);
value_t adm_ltp_agent_get_span_reset_time(tdc_t params);
value_t adm_ltp_agent_get_span_out_seg_q_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_seg_q_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_out_seg_pop_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_seg_pop_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_out_ckpt_xmit_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_pos_ack_rx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_neg_ack_rx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_cancel_rx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_ckpt_rexmit_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_cancel_xmit_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_out_complete_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_red_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_red_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_green_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_green_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_redundant_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_redundant_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_mal_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_mal_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_unk_sender_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_unk_sender_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_unk_client_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_rx_unk_client_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_stray_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_stray_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_miscolor_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_miscolor_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_closed_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_seg_closed_bytes(tdc_t params);
value_t adm_ltp_agent_get_span_in_ckpt_rx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_pos_ack_tx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_neg_ack_tx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_cancel_tx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_ack_retx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_cancel_rx_cnt(tdc_t params);
value_t adm_ltp_agent_get_span_in_complete_cnt(tdc_t params);


/* Control Functions */
tdc_t* adm_ltp_agent_ctrl_reset(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_ltp_agent_ctrl_list_engines(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_LTP_AGENT_IMPL_H_
