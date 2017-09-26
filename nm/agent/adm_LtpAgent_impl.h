/*****************************************************************************
 **
 ** File Name: ./agent/adm_ltpAgent_impl.h
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
 **  2017-09-24  AUTO           Auto generated header file 
 *****************************************************************************/

#ifndef ADM_LTPAGENT_IMPL_H_
#define ADM_LTPAGENT_IMPL_H_

#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"

/*   START CUSTOM INCLUDES HERE  */
#include "ltpnm.h"

/*   STOP CUSTOM INCLUDES HERE   */


/******************
 * TODO: typeENUM *
 *****************/

void Name_adm_init_agent();

int8_t adm_ltpAgent_getspan(tdc_t params, NmltpSpan *stats);


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/* Metadata Functions */
value_t adm_LtpAgent_meta_Name(tdc_t params);
value_t adm_LtpAgent_meta_NameSpace(tdc_t params);

value_t adm_LtpAgent_meta_Version(tdc_t params);

value_t adm_LtpAgent_meta_Organization(tdc_t params);


/* Collect Functions */
value_t adm_LtpAgent_get_spanRemoteEngineNbr(tdc_t params);
value_t adm_LtpAgent_get_spanCurExptSess(tdc_t params);
value_t adm_LtpAgent_get_spanCurOutSeg(tdc_t params);
value_t adm_LtpAgent_get_spanCurImpSess(tdc_t params);
value_t adm_LtpAgent_get_spanCurInSeg(tdc_t params);
value_t adm_LtpAgent_get_spanResetTime(tdc_t params);
value_t adm_LtpAgent_get_spanOutSegQCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutSegQBytes(tdc_t params);
value_t adm_LtpAgent_get_spanOutSegPopCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutSegPopBytes(tdc_t params);
value_t adm_LtpAgent_get_spanOutCkptXmitCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutPosAckRxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutNegAckRxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutCancelRxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutCkptReXmitCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutCancelXmitCnt(tdc_t params);
value_t adm_LtpAgent_get_spanOutCompleteCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxRedCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxRedBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxGreenCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxGreenBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxRedundantCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxRedundantBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxMalCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxMalBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxUnkSendCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxUnkSendBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxUnkClientCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegRxUnkClientBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegStrayCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegStrayBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegMiscolorCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegMiscolorBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInSegClosedCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInSegClosedBytes(tdc_t params);
value_t adm_LtpAgent_get_spanInCkptRxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInPosAckTxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInNegAckTxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInCancelTxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInAckReTxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInCancelRxCnt(tdc_t params);
value_t adm_LtpAgent_get_spanInCompleteCnt(tdc_t params);


/* Control Functions */
tdc_t* adm_LtpAgent_ctrl_reset(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_LtpAgent_ctrl_listEngines(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_LTPAGENT_IMPL_H_
