/****************************************************************************
 **
 ** File Name: adm_ltp_agent_impl.c
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
 **  2018-01-06  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_ltp_agent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ltp_agent_setup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void adm_ltp_agent_cleanup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


value_t adm_ltp_agent_meta_name(tdc_t params)
{
	return val_from_string("adm_ltp_agent");
}


value_t adm_ltp_agent_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:ltp_agent");
}


value_t adm_ltp_agent_meta_version(tdc_t params)
{
	return val_from_string("V0.0");
}


value_t adm_ltp_agent_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/* Collect Functions */
/*
 * The remote engine number of this span.
 */
value_t adm_ltp_agent_get_span_remote_engine_nbr(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_remote_engine_nbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_remote_engine_nbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Expected sessions on this span.
 */
value_t adm_ltp_agent_get_span_cur_expt_sess(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_expt_sess BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_cur_expt_sess BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The current number of outbound segments for this span.
 */
value_t adm_ltp_agent_get_span_cur_out_seg(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_out_seg BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_cur_out_seg BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The current number of import segments for this span.
 */
value_t adm_ltp_agent_get_span_cur_imp_sess(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_imp_sess BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_cur_imp_sess BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The current number of inbound segments for this span.
 */
value_t adm_ltp_agent_get_span_cur_in_seg(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_in_seg BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_cur_in_seg BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The last time the span counters were reset.
 */
value_t adm_ltp_agent_get_span_reset_time(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_reset_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_reset_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output segment queued count for the span.
 */
value_t adm_ltp_agent_get_span_out_seg_q_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_q_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_seg_q_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output segment queued bytes for the span.
 */
value_t adm_ltp_agent_get_span_out_seg_q_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_q_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_seg_q_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output segment popped count for the span.
 */
value_t adm_ltp_agent_get_span_out_seg_pop_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_pop_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_seg_pop_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output segment popped bytes for the span.
 */
value_t adm_ltp_agent_get_span_out_seg_pop_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_pop_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_seg_pop_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output checkpoint transmit count for the span.
 */
value_t adm_ltp_agent_get_span_out_ckpt_xmit_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_ckpt_xmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_ckpt_xmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output positive acknowledgement received count for the span.
 */
value_t adm_ltp_agent_get_span_out_pos_ack_rx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_pos_ack_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_pos_ack_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output negative acknowledgement received count for the span.
 */
value_t adm_ltp_agent_get_span_out_neg_ack_rx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_neg_ack_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_neg_ack_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output cancelled received count for the span.
 */
value_t adm_ltp_agent_get_span_out_cancel_rx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_cancel_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_cancel_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output checkpoint retransmit count for the span.
 */
value_t adm_ltp_agent_get_span_out_ckpt_rexmit_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_ckpt_rexmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_ckpt_rexmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output cancel retransmit count for the span.
 */
value_t adm_ltp_agent_get_span_out_cancel_xmit_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_cancel_xmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_cancel_xmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The output completed count for the span.
 */
value_t adm_ltp_agent_get_span_out_complete_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_complete_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_out_complete_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment received red count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_red_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_red_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_red_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment received red bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_red_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_red_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_red_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment received green count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_green_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_green_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_green_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment received green bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_green_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_green_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_green_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment received redundant count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_redundant_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_redundant_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_redundant_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment received redundant bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_redundant_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_redundant_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_redundant_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment malformed count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_mal_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_mal_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_mal_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment malformed bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_mal_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_mal_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_mal_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment unknown sender count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_unk_sender_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_sender_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_unk_sender_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment unknown sender bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_unk_sender_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_sender_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_unk_sender_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment unknown client count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_unk_client_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_client_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_unk_client_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment unknown client bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_rx_unk_client_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_client_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_rx_unk_client_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment stray count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_stray_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_stray_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_stray_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment stray bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_stray_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_stray_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_stray_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment miscolored count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_miscolor_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_miscolor_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_miscolor_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment miscolored bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_miscolor_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_miscolor_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_miscolor_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment closed count for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_closed_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_closed_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_closed_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input segment closed bytes for the span.
 */
value_t adm_ltp_agent_get_span_in_seg_closed_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_closed_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_seg_closed_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input checkpoint receive count for the span.
 */
value_t adm_ltp_agent_get_span_in_ckpt_rx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_ckpt_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_ckpt_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input positive acknolwedgement transmitted count for the span.
 */
value_t adm_ltp_agent_get_span_in_pos_ack_tx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_pos_ack_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_pos_ack_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input negative acknolwedgement transmitted count for the span.
 */
value_t adm_ltp_agent_get_span_in_neg_ack_tx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_neg_ack_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_neg_ack_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input cancel transmitted count for the span.
 */
value_t adm_ltp_agent_get_span_in_cancel_tx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_cancel_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_cancel_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input acknolwedgement retransmit count for the span.
 */
value_t adm_ltp_agent_get_span_in_ack_retx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_ack_retx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_ack_retx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input cancel receive count for the span.
 */
value_t adm_ltp_agent_get_span_in_cancel_rx_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_cancel_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_cancel_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The input completed count for the span.
 */
value_t adm_ltp_agent_get_span_in_complete_cnt(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_complete_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_span_in_complete_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * Resets the counters associated with the engine and updates the last reset time for the span to be th
 * e time when this control was run.
 */
tdc_t* adm_ltp_agent_ctrl_reset(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_reset BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_reset BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Lists all remote engine IDs.
 */
tdc_t* adm_ltp_agent_ctrl_list_engines(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_list_engines BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_engines BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
