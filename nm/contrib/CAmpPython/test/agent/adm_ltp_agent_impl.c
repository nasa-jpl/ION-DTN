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
 **  2018-03-16  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/primitives/table.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "adm_ltp_agent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */

int8_t get_span(tdc_t params, NmltpSpan *stats)
{
  int8_t success = 0;
  uint32_t id = adm_extract_uint(params, 0, &success);

  if(success == 1)
  {
	  ltpnm_span_get(id, stats, (int *) &success);
  }

  return success;
}

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
	return val_from_str("adm_ltp_agent");
}


value_t adm_ltp_agent_meta_namespace(tdc_t params)
{
	return val_from_str("arn:DTN:ltp_agent");
}


value_t adm_ltp_agent_meta_version(tdc_t params)
{
	return val_from_str("V0.0");
}


value_t adm_ltp_agent_meta_organization(tdc_t params)
{
	return val_from_str("JHUAPL");
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
	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.remoteEngineNbr;
	}

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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.currentExportSessions;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.currentOutboundSegments;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.currentImportSessions;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.currentInboundSegments;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.lastResetTime;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputSegQueuedCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputSegQueuedBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputSegPoppedCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputSegPoppedBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputCkptXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputPosAckRecvCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputNegAckRecvCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputCancelRecvCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputCkptReXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputCancelXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.outputCompleteCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegRecvRedCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegRecvRedBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegRecvGreenCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegRecvGreenBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegRedundantCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegRedundantBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegMalformedCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegMalformedBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegUnkSenderCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegUnkSenderBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegUnkClientCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegUnkClientBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegStrayCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegStrayBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegMiscolorCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegMiscolorBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegClosedCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputSegClosedBytes;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputCkptRecvCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputPosAckXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputNegAckXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputCancelXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputAckReXmitCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputCancelRecvCount;
	}
	
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

	NmltpSpan stats;

	if(get_span(params, &stats) != 1)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = stats.inputCompleteCount;
	}
	
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

	int8_t success = 0;
	uint32_t id = adm_extract_uint(params, 0, &success);

	if(success == 1)
	{
		ltpnm_span_reset(id, (int*) &success);
		if(success == 1)
		{
			*status = CTRL_SUCCESS;
		}
	}

	
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
	
	table_t *table = NULL;
	uint8_t *data = NULL;
	uint32_t len = 0;
	unsigned int ids[20];
	int numIds = 20;
	int i = 0;
	Lyst cur_row = NULL;


	if((table = table_create(NULL, NULL)) == NULL)
	{
		AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines","Cannot allocate table.", NULL);
		return NULL;
	}

	if(table_add_col(table, "RemoteEngineId", AMP_TYPE_UINT) == ERROR)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines","Cannot add columns.", NULL);
		return NULL;
	}

	ltpnm_spanEngineIds_get(ids, &numIds);

	for(i = 0; i < numIds; i++)
	{
		if((cur_row = lyst_create()) != NULL)
		{
			uint32_t value = ids[i];
			if(dc_add(cur_row, (uint8_t*) &value, sizeof(uint32_t)) == ERROR)
			{
				dc_destroy(&cur_row);
				table_destroy(table, 1);

				AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines", "Error extracting id", NULL);
				return NULL;
			}
			else
			{
				table_add_row(table, cur_row);
			}

		}
	}

	/* Step 2: Allocate the return value. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines","Can't make TDC.", NULL);
		return NULL;
	}

	/* Step 3: Populate the result. */
	if((data = table_serialize(table, &len)) == NULL)
	{
		table_destroy(table, 1);
		tdc_destroy(&result);

		AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines","Can't serialize table.", NULL);
		return NULL;
	}

	table_destroy(table, 1);

	tdc_insert(result, AMP_TYPE_TABLE, data, len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_engines BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
