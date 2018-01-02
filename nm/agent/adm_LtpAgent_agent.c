/****************************************************************************
 **
 ** File Name: ./agent/adm_ltpAgent_agent.c
 **
 ** Description: 
 **
 ** Notes: 
 **
 ** Assumptions: 
 **
 ** Modification History: 
 **  MM/DD/YY  AUTHOR           DESCRIPTION
 **  --------  --------------   ------------------------------------------------
 **  2017-09-24  AUTO           Auto generated c file 
 **
****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "../shared/adm/adm_LtpAgent.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_LtpAgent_impl.h"
#include "rda.h"
#define _HAVE_LTPAGENT_ADM_
#ifdef _HAVE_LTPAGENT_ADM_
void adm_LtpAgent_init()
{
	adm_LtpAgent_init_edd();
	adm_LtpAgent_init_variables();
	adm_LtpAgent_init_controls();
	adm_LtpAgent_init_constants();
	adm_LtpAgent_init_macros();
	adm_LtpAgent_init_metadata();
	adm_LtpAgent_init_ops();
	adm_LtpAgent_init_reports();
}

void adm_LtpAgent_init_edd()
{

	adm_add_edd(ADM_LTPAGENT_EDD_SPANREMOTEENGINENBR_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanRemoteEngineNbr, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANCUREXPTSESS_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanCurExptSess, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANCUROUTSEG_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanCurOutSeg, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANCURIMPSESS_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanCurImpSess, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANCURINSEG_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanCurInSeg, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANRESETTIME_MID, AMP_TYPE_UVAST, 0, adm_LtpAgent_get_spanResetTime, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTSEGQCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutSegQCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTSEGQBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutSegQBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTSEGPOPCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutSegPopCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTSEGPOPBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutSegPopBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTCKPTXMITCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutCkptXmitCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTPOSACKRXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutPosAckRxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTNEGACKRXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutNegAckRxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTCANCELRXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutCancelRxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTCKPTREXMITCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutCkptReXmitCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTCANCELXMITCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutCancelXmitCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANOUTCOMPLETECNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanOutCompleteCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXREDCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxRedCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXREDBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxRedBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXGREENCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxGreenCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXGREENBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxGreenBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXREDUNDANTCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxRedundantCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXREDUNDANTBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxRedundantBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXMALCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxMalCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXMALBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxMalBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXUNKSENDCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxUnkSendCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXUNKSENDBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxUnkSendBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXUNKCLIENTCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxUnkClientCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGRXUNKCLIENTBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegRxUnkClientBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGSTRAYCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegStrayCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGSTRAYBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegStrayBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGMISCOLORCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegMiscolorCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGMISCOLORBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegMiscolorBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGCLOSEDCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegClosedCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINSEGCLOSEDBYTES_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInSegClosedBytes, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINCKPTRXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInCkptRxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINPOSACKTXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInPosAckTxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINNEGACKTXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInNegAckTxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINCANCELTXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInCancelTxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINACKRETXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInAckReTxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINCANCELRXCNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInCancelRxCnt, NULL, NULL);
	adm_add_edd(ADM_LTPAGENT_EDD_SPANINCOMPLETECNT_MID, AMP_TYPE_UINT, 0, adm_LtpAgent_get_spanInCompleteCnt, NULL, NULL);
}

void adm_LtpAgent_init_variables()
{
}

void adm_LtpAgent_init_controls()
{
	adm_add_ctrl(ADM_LTPAGENT_CTRL_RESET_MID, adm_LtpAgent_ctrl_reset);
	adm_add_ctrl(ADM_LTPAGENT_CTRL_LISTENGINES_MID, adm_LtpAgent_ctrl_listEngines);

}

void adm_LtpAgent_init_constants()
{
}

void adm_LtpAgent_init_macros()
{
}

void adm_LtpAgent_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(LTPAGENT_ADM_META_NN_IDX, LTPAGENT_ADM_META_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_EDD_NN_IDX, LTPAGENT_ADM_EDD_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_VAR_NN_IDX, LTPAGENT_ADM_VAR_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_RPT_NN_IDX, LTPAGENT_ADM_RPT_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_CTRL_NN_IDX, LTPAGENT_ADM_CTRL_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_CONST_NN_IDX, LTPAGENT_ADM_CONST_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_MACRO_NN_IDX, LTPAGENT_ADM_MACRO_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_OP_NN_IDX, LTPAGENT_ADM_OP_NN_STR, "LTPAGENT", "2017-08-17");
	oid_nn_add_parm(LTPAGENT_ADM_ROOT_NN_IDX, LTPAGENT_ADM_ROOT_NN_STR, "LTPAGENT", "2017-08-17");
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_LTPAGENT_META_NAME_MID, AMP_TYPE_STRING, 0, adm_LtpAgent_meta_Name, adm_print_string, adm_size_string);
	adm_add_edd(ADM_LTPAGENT_META_NAMESPACE_MID, AMP_TYPE_STRING, 0, adm_LtpAgent_meta_NameSpace, adm_print_string, adm_size_string);
	adm_add_edd(ADM_LTPAGENT_META_VERSION_MID, AMP_TYPE_STRING, 0, adm_LtpAgent_meta_Version, adm_print_string, adm_size_string);
	adm_add_edd(ADM_LTPAGENT_META_ORGANIZATION_MID, AMP_TYPE_STRING, 0, adm_LtpAgent_meta_Organization, adm_print_string, adm_size_string);
}

void adm_LtpAgent_init_ops()
{
}

void adm_LtpAgent_init_reports()
{
	uint32_t used= 0;

}

#endif // _HAVE_LTPAGENT_ADM_
