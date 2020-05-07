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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/primitives/table.h"
#include "ltpP.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_ltp_agent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */

int8_t get_span(tnvc_t *parms, NmltpSpan *stats)
{
  int success = 0;
  uint32_t id = adm_get_parm_uint(parms, 0, &success);

  if(success == 1)
  {
	  ltpnm_span_get(id, stats, (int *) &success);
  }

  return success;
}

/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ltp_agent_setup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	ltpAttach();

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void dtn_ltp_agent_cleanup()
{

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


tnv_t *dtn_ltp_agent_meta_name(tnvc_t *parms)
{
	return tnv_from_str("ltp_agent");
}


tnv_t *dtn_ltp_agent_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/ltp_agent");
}


tnv_t *dtn_ltp_agent_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.0");
}


tnv_t *dtn_ltp_agent_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * This table lists all known remote engine ids.
 */
tbl_t *dtn_ltp_agent_tblt_engines(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_engines BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr		sdr = getIonsdr();
	LtpVdb		*vdb = getLtpVdb();
	PsmPartition	ionwm = getIonwm();
	Object		ltpdbObj = getLtpDbObject();
	OBJ_POINTER(LtpDB, ltpdb);
	PsmAddress	elt;
	LtpVspan	*vspan;
	tnvc_t  *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));	/*	Just to lock memory.	*/
	GET_OBJ_POINTER(sdr, LtpDB, ltpdb, ltpdbObj);

	for (elt = sm_list_first(ionwm, vdb->spans); elt; elt = sm_list_next(ionwm, elt))
	{
		vspan = (LtpVspan *) psp(ionwm, sm_list_data(ionwm, elt));

		/* (UVAST) peer_engine_nbr */
		if((cur_row = tnvc_create(1)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uvast(vspan->engineId));
			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ltp_agent_tblt_engines", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_engines BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * The remote engine number of this span.
 */
tnv_t *dtn_ltp_agent_get_span_remote_engine_nbr(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_remote_engine_nbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.remoteEngineNbr);
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
tnv_t *dtn_ltp_agent_get_span_cur_expt_sess(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_expt_sess BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.currentExportSessions);
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
tnv_t *dtn_ltp_agent_get_span_cur_out_seg(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_out_seg BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.currentOutboundSegments);
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
tnv_t *dtn_ltp_agent_get_span_cur_imp_sess(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_imp_sess BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.currentImportSessions);
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
tnv_t *dtn_ltp_agent_get_span_cur_in_seg(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_cur_in_seg BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.currentInboundSegments);
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
tnv_t *dtn_ltp_agent_get_span_reset_time(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_reset_time BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.lastResetTime);
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
tnv_t *dtn_ltp_agent_get_span_out_seg_q_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_q_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputSegQueuedCount);
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
tnv_t *dtn_ltp_agent_get_span_out_seg_q_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_q_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputSegQueuedBytes);
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
tnv_t *dtn_ltp_agent_get_span_out_seg_pop_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_pop_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputSegPoppedCount);
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
tnv_t *dtn_ltp_agent_get_span_out_seg_pop_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_seg_pop_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputSegPoppedBytes);
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
tnv_t *dtn_ltp_agent_get_span_out_ckpt_xmit_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_ckpt_xmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputCkptXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_out_pos_ack_rx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_pos_ack_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputPosAckRecvCount);
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
tnv_t *dtn_ltp_agent_get_span_out_neg_ack_rx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_neg_ack_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputNegAckRecvCount);
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
tnv_t *dtn_ltp_agent_get_span_out_cancel_rx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_cancel_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputCancelRecvCount);
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
tnv_t *dtn_ltp_agent_get_span_out_ckpt_rexmit_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_ckpt_rexmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputCkptReXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_out_cancel_xmit_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_cancel_xmit_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputCancelXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_out_complete_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_out_complete_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.outputCompleteCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_red_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_red_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegRecvRedCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_red_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_red_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegRecvRedBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_green_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_green_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegRecvGreenCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_green_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_green_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegRecvGreenBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_redundant_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_redundant_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegRedundantCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_redundant_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_redundant_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegRedundantBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_mal_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_mal_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegMalformedCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_mal_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_mal_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegMalformedBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_sender_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_sender_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegUnkSenderCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_sender_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_sender_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegUnkSenderBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_client_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_client_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegUnkClientCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_rx_unk_client_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_rx_unk_client_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegUnkClientBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_stray_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_stray_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegStrayCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_stray_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_stray_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegStrayBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_miscolor_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_miscolor_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegMiscolorCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_miscolor_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_miscolor_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegMiscolorBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_closed_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_closed_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegClosedCount);
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
tnv_t *dtn_ltp_agent_get_span_in_seg_closed_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_seg_closed_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputSegClosedBytes);
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
tnv_t *dtn_ltp_agent_get_span_in_ckpt_rx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_ckpt_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputCkptRecvCount);
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
tnv_t *dtn_ltp_agent_get_span_in_pos_ack_tx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_pos_ack_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputPosAckXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_in_neg_ack_tx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_neg_ack_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputNegAckXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_in_cancel_tx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_cancel_tx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputCancelXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_in_ack_retx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_ack_retx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputAckReXmitCount);
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
tnv_t *dtn_ltp_agent_get_span_in_cancel_rx_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_cancel_rx_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputCancelRecvCount);
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
tnv_t *dtn_ltp_agent_get_span_in_complete_cnt(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_span_in_complete_cnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmltpSpan stats;

	if(get_span(parms, &stats) == 1)
	{
		result = tnv_from_uint(stats.inputCompleteCount);
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
tnv_t *dtn_ltp_agent_ctrl_reset(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_reset BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success = 0;
	uint32_t id = adm_get_parm_uint(parms, 0, &success);

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



/* OP Functions */
