/*****************************************************************************
 **
 ** File Name: ./agent/adm_ltpAgent_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  2017-09-24  AUTO           Auto generated c file 
 *****************************************************************************/

#include "adm_LtpAgent_impl.h"

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/primitives/table.h"
/*   STOP CUSTOM INCLUDES HERE   */


/*   START CUSTOM FUNCTIONS HERE */

int8_t adm_ltpAgent_getspan(tdc_t params, NmltpSpan *stats)
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

/* Metadata Functions */

value_t adm_LtpAgent_meta_Name(tdc_t params)
{
	return val_from_string("adm_ltpAgent");
}

value_t adm_LtpAgent_meta_NameSpace(tdc_t params)
{
	return val_from_string("arn:ltpAgent");
}


value_t adm_LtpAgent_meta_Version(tdc_t params)
{
	return val_from_string("V0.1");
}


value_t adm_LtpAgent_meta_Organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}



/* Collect Functions */
// The remote engine number of this LTP span.
value_t adm_LtpAgent_get_spanRemoteEngineNbr(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanRemoteEngineNbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanRemoteEngineNbr BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The remote engine number of this LTP engines.
value_t adm_LtpAgent_get_spanCurExptSess(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanCurExptSess BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanCurExptSess BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The current number of outbound segments for this span.
value_t adm_LtpAgent_get_spanCurOutSeg(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanCurOutSeg BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanCurOutSeg BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The current number of import sessions for this span.
value_t adm_LtpAgent_get_spanCurImpSess(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanCurImpSess BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanCurImpSess BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The current number of inbound segments for this span.
value_t adm_LtpAgent_get_spanCurInSeg(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanCurInSeg BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanCurInSeg BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The last time the span counters were reset.
value_t adm_LtpAgent_get_spanResetTime(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanResetTime BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanResetTime BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output segment queued count for the span.
value_t adm_LtpAgent_get_spanOutSegQCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegQCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegQCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output segment queued bytes for the span.
value_t adm_LtpAgent_get_spanOutSegQBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegQBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegQBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output segment popped count for the span.
value_t adm_LtpAgent_get_spanOutSegPopCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegPopCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegPopCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output segment popped bytes for the span.
value_t adm_LtpAgent_get_spanOutSegPopBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegPopBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutSegPopBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output checkpoint transmit count for the span.
value_t adm_LtpAgent_get_spanOutCkptXmitCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutCkptXmitCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutCkptXmitCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output positive acknolwedgement received count for the span.
value_t adm_LtpAgent_get_spanOutPosAckRxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutPosAckRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutPosAckRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output negative acknolwedgement received count for the span.
value_t adm_LtpAgent_get_spanOutNegAckRxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutNegAckRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutNegAckRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output cancelled received count for the span.
value_t adm_LtpAgent_get_spanOutCancelRxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutCancelRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutCancelRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output checkpoint retransmit count for the span.
value_t adm_LtpAgent_get_spanOutCkptReXmitCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutCkptReXmitCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutCkptReXmitCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output cancel retransmit count for the span.
value_t adm_LtpAgent_get_spanOutCancelXmitCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutCancelXmitCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutCancelXmitCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The output completed count for the span.
value_t adm_LtpAgent_get_spanOutCompleteCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanOutCompleteCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanOutCompleteCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment received red count for the span.
value_t adm_LtpAgent_get_spanInSegRxRedCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment received red bytes for the span.
value_t adm_LtpAgent_get_spanInSegRxRedBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment received green count for the span.
value_t adm_LtpAgent_get_spanInSegRxGreenCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxGreenCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxGreenCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment received green bytes for the span.
value_t adm_LtpAgent_get_spanInSegRxGreenBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxGreenBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxGreenBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment received redundant count for the span.
value_t adm_LtpAgent_get_spanInSegRxRedundantCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedundantCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedundantCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment received redundant bytes for the span.
value_t adm_LtpAgent_get_spanInSegRxRedundantBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedundantBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxRedundantBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment malformed count for the span.
value_t adm_LtpAgent_get_spanInSegRxMalCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxMalCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxMalCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment malformed bytes for the span.
value_t adm_LtpAgent_get_spanInSegRxMalBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxMalBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxMalBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment unknown sender count for the span.
value_t adm_LtpAgent_get_spanInSegRxUnkSendCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkSendCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkSendCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment unknown sender bytes for the span.
value_t adm_LtpAgent_get_spanInSegRxUnkSendBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkSendBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkSendBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment unknown client count for the span.
value_t adm_LtpAgent_get_spanInSegRxUnkClientCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkClientCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkClientCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment unknown client bytes for the span.
value_t adm_LtpAgent_get_spanInSegRxUnkClientBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkClientBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegRxUnkClientBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment stray count for the span.
value_t adm_LtpAgent_get_spanInSegStrayCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegStrayCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegStrayCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment stray bytes for the span.
value_t adm_LtpAgent_get_spanInSegStrayBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegStrayBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegStrayBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment miscolored count for the span.
value_t adm_LtpAgent_get_spanInSegMiscolorCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegMiscolorCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegMiscolorCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment miscolored bytes for the span.
value_t adm_LtpAgent_get_spanInSegMiscolorBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegMiscolorBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegMiscolorBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment closed count for the span.
value_t adm_LtpAgent_get_spanInSegClosedCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegClosedCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegClosedCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input segment closed bytes for the span.
value_t adm_LtpAgent_get_spanInSegClosedBytes(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInSegClosedBytes BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInSegClosedBytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input checkpoint receive count for the span.
value_t adm_LtpAgent_get_spanInCkptRxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInCkptRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInCkptRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input positive acknolwedgement transmitted count for the span.
value_t adm_LtpAgent_get_spanInPosAckTxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInPosAckTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInPosAckTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input negative acknowledgement transmitted count for the span.
value_t adm_LtpAgent_get_spanInNegAckTxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInNegAckTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInNegAckTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input cancel transmited count for the span.
value_t adm_LtpAgent_get_spanInCancelTxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInCancelTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInCancelTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input acknolwedgement retransmit count for the span.
value_t adm_LtpAgent_get_spanInAckReTxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInAckReTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInAckReTxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input cancel receive count for the span.
value_t adm_LtpAgent_get_spanInCancelRxCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInCancelRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInCancelRxCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// The input completed count for the span.
value_t adm_LtpAgent_get_spanInCompleteCnt(tdc_t params)
{
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_get_spanInCompleteCnt BODY
	 * +-------------------------------------------------------------------------+
	 */

	value_t result;
	NmltpSpan stats;

	if(adm_ltpAgent_getspan(params, &stats) != 1)
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_get_spanInCompleteCnt BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */
// Resets the counters associated with the engine and updates the last reset time for the span to be the time when this control was run.
tdc_t *adm_LtpAgent_ctrl_reset(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ltpAgent_ctrl_reset BODY
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
	 * |STOP CUSTOM FUNCTION adm_ltpAgent_ctrl_reset BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

tdc_t* adm_LtpAgent_ctrl_listEngines(eid_t *def_mgr, tdc_t params, int8_t *status)
{

	tdc_t *retval = NULL;
	table_t *table = NULL;
	uint8_t *data = NULL;
	uint32_t len = 0;
	unsigned int ids[20];
	int numIds = 20;
	int i = 0;
	Lyst cur_row = NULL;

	*status = CTRL_FAILURE;

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
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines","Can't make TDC.", NULL);
		return NULL;
	}

	/* Step 3: Populate the retval. */
	if((data = table_serialize(table, &len)) == NULL)
	{
		table_destroy(table, 1);
		tdc_destroy(&retval);

		AMP_DEBUG_ERR("adm_LtpAgent_ctrl_listEngines","Can't serialize table.", NULL);
		return NULL;
	}

	table_destroy(table, 1);

	tdc_insert(retval, AMP_TYPE_TABLE, data, len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}


/* OP Functions */
