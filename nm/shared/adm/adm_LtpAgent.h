/******************************************************************************
 **
 ** File Name: ./shared/adm_ltpAgent.h
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
 **
 **  2017-09-24  AUTO           Auto generated header file 
 **
*********************************************************************************/
#ifndef ADM_LTPAGENT_H_
#define ADM_LTPAGENT_H_
#define _HAVE_LTPAGENT_ADM_
#ifdef _HAVE_LTPAGENT_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                                          +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:ltpAgent
 * ADM NICKNAMES:
 *
 *
 *					LTPAGENT ADM ROOT
 *								    |
 *								    |
 * Meta-						    |
 * Data	EDDs	VARs	Rpts	Ctrls	Constants	Macros	Ops   Tbls
 *  _0	 _1	  _2	  _3	   _4 	  _5       _6     _7    _8
 *  +------+-----+-----+------+-------+--------+-------+-------+
 *
 */
/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                                        +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * META-> 14
 * EDD -> 15
 * VAR -> 16
 * RPT -> 17
 * CTRL -> 18
 * CONST -> 19
 * MACRO -> 1a
 * OP -> 1b
 * TBL -> 1c
 * ROOT -> 1d

 */
#define LTPAGENT_ADM_META_NN_IDX 0x14
#define LTPAGENT_ADM_META_NN_STR "14"

#define LTPAGENT_ADM_EDD_NN_IDX 0x15
#define LTPAGENT_ADM_EDD_NN_STR "15"

#define LTPAGENT_ADM_VAR_NN_IDX 0x16
#define LTPAGENT_ADM_VAR_NN_STR "16"

#define LTPAGENT_ADM_RPT_NN_IDX 0x17
#define LTPAGENT_ADM_RPT_NN_STR "17"

#define LTPAGENT_ADM_CTRL_NN_IDX 0x18
#define LTPAGENT_ADM_CTRL_NN_STR "18"

#define LTPAGENT_ADM_CONST_NN_IDX 0x19
#define LTPAGENT_ADM_CONST_NN_STR "19"

#define LTPAGENT_ADM_MACRO_NN_IDX 0x1A
#define LTPAGENT_ADM_MACRO_NN_STR "1A"

#define LTPAGENT_ADM_OP_NN_IDX 0x1B
#define LTPAGENT_ADM_OP_NN_STR "1B"

#define LTPAGENT_ADM_TBL_NN_IDX 0x1C
#define LTPAGENT_ADM_TBL_NN_STR "1C"

#define LTPAGENT_ADM_ROOT_NN_IDX 0x1D
#define LTPAGENT_ADM_ROOT_NN_STR "1D"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTPAGENT META-DATA DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |Name                         |86140100    |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |NameSpace                    |86140101    |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |Version                      |86140102    |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |Organization                 |86140103    |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "Name"
#define ADM_LTPAGENT_META_NAME_MID "86140100"
// "NameSpace"
#define ADM_LTPAGENT_META_NAMESPACE_MID "86140101"
// "Version"
#define ADM_LTPAGENT_META_VERSION_MID "86140102"
// "Organization"
#define ADM_LTPAGENT_META_ORGANIZATION_MID "86140103"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                 LTPAGENT EXTERNALLY DEFINED DATA DEFINITIONS                                               +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanRemoteEngineNbr          |c0150100    |The remote engine number of this LTP span.        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanCurExptSess              |c0150101    |The remote engine number of this LTP engines.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanCurOutSeg                |c0150102    |The current number of outbound segments for this s|             |
   |                             |            |pan.                                              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanCurImpSess               |c0150103    |The current number of import sessions for this spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanCurInSeg                 |c0150104    |The current number of inbound segments for this sp|             |
   |                             |            |an.                                               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanResetTime                |c0150105    |The last time the span counters were reset.       |UVAST        |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutSegQCnt               |c0150106    |The output segment queued count for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutSegQBytes             |c0150107    |The output segment queued bytes for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutSegPopCnt             |c0150108    |The output segment popped count for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutSegPopBytes           |c0150109    |The output segment popped bytes for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutCkptXmitCnt           |c015010a    |The output checkpoint transmit count for the span.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutPosAckRxCnt           |c015010b    |The output positive acknolwedgement received count|             |
   |                             |            | for the span.                                    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutNegAckRxCnt           |c015010c    |The output negative acknolwedgement received count|             |
   |                             |            | for the span.                                    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutCancelRxCnt           |c015010d    |The output cancelled received count for the span. |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutCkptReXmitCnt         |c015010e    |The output checkpoint retransmit count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutCancelXmitCnt         |c015010f    |The output cancel retransmit count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanOutCompleteCnt           |c0150110    |The output completed count for the span.          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxRedCnt            |c0150111    |The input segment received red count for the span.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxRedBytes          |c0150112    |The input segment received red bytes for the span.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxGreenCnt          |c0150113    |The input segment received green count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxGreenBytes        |c0150114    |The input segment received green bytes for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxRedundantCnt      |c0150115    |The input segment received redundant count for the|             |
   |                             |            | span.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxRedundantBytes    |c0150116    |The input segment received redundant bytes for the|             |
   |                             |            | span.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxMalCnt            |c0150117    |The input segment malformed count for the span.   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxMalBytes          |c0150118    |The input segment malformed bytes for the span.   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxUnkSendCnt        |c0150119    |The input segment unknown sender count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxUnkSendBytes      |c015011a    |The input segment unknown sender bytes for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxUnkClientCnt      |c015011b    |The input segment unknown client count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegRxUnkClientBytes    |c015011c    |The input segment unknown client bytes for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegStrayCnt            |c015011d    |The input segment stray count for the span.       |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegStrayBytes          |c015011e    |The input segment stray bytes for the span.       |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegMiscolorCnt         |c015011f    |The input segment miscolored count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegMiscolorBytes       |c0150120    |The input segment miscolored bytes for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegClosedCnt           |c0150121    |The input segment closed count for the span.      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInSegClosedBytes         |c0150122    |The input segment closed bytes for the span.      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInCkptRxCnt              |c0150123    |The input checkpoint receive count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInPosAckTxCnt            |c0150124    |The input positive acknolwedgement transmitted cou|             |
   |                             |            |nt for the span.                                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInNegAckTxCnt            |c0150125    |The input negative acknowledgement transmitted cou|             |
   |                             |            |nt for the span.                                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInCancelTxCnt            |c0150126    |The input cancel transmited count for the span.   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInAckReTxCnt             |c0150127    |The input acknolwedgement retransmit count for the|             |
   |                             |            | span.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInCancelRxCnt            |c0150128    |The input cancel receive count for the span.      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |spanInCompleteCnt            |c0150129    |The input completed count for the span.           |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_LTPAGENT_EDD_SPANREMOTEENGINENBR_MID "c0150100"
#define ADM_LTPAGENT_EDD_SPANCUREXPTSESS_MID "c0150101"
#define ADM_LTPAGENT_EDD_SPANCUROUTSEG_MID "c0150102"
#define ADM_LTPAGENT_EDD_SPANCURIMPSESS_MID "c0150103"
#define ADM_LTPAGENT_EDD_SPANCURINSEG_MID "c0150104"
#define ADM_LTPAGENT_EDD_SPANRESETTIME_MID "c0150105"
#define ADM_LTPAGENT_EDD_SPANOUTSEGQCNT_MID "c0150106"
#define ADM_LTPAGENT_EDD_SPANOUTSEGQBYTES_MID "c0150107"
#define ADM_LTPAGENT_EDD_SPANOUTSEGPOPCNT_MID "c0150108"
#define ADM_LTPAGENT_EDD_SPANOUTSEGPOPBYTES_MID "c0150109"
#define ADM_LTPAGENT_EDD_SPANOUTCKPTXMITCNT_MID "c015010a"
#define ADM_LTPAGENT_EDD_SPANOUTPOSACKRXCNT_MID "c015010b"
#define ADM_LTPAGENT_EDD_SPANOUTNEGACKRXCNT_MID "c015010c"
#define ADM_LTPAGENT_EDD_SPANOUTCANCELRXCNT_MID "c015010d"
#define ADM_LTPAGENT_EDD_SPANOUTCKPTREXMITCNT_MID "c015010e"
#define ADM_LTPAGENT_EDD_SPANOUTCANCELXMITCNT_MID "c015010f"
#define ADM_LTPAGENT_EDD_SPANOUTCOMPLETECNT_MID "c0150110"
#define ADM_LTPAGENT_EDD_SPANINSEGRXREDCNT_MID "c0150111"
#define ADM_LTPAGENT_EDD_SPANINSEGRXREDBYTES_MID "c0150112"
#define ADM_LTPAGENT_EDD_SPANINSEGRXGREENCNT_MID "c0150113"
#define ADM_LTPAGENT_EDD_SPANINSEGRXGREENBYTES_MID "c0150114"
#define ADM_LTPAGENT_EDD_SPANINSEGRXREDUNDANTCNT_MID "c0150115"
#define ADM_LTPAGENT_EDD_SPANINSEGRXREDUNDANTBYTES_MID "c0150116"
#define ADM_LTPAGENT_EDD_SPANINSEGRXMALCNT_MID "c0150117"
#define ADM_LTPAGENT_EDD_SPANINSEGRXMALBYTES_MID "c0150118"
#define ADM_LTPAGENT_EDD_SPANINSEGRXUNKSENDCNT_MID "c0150119"
#define ADM_LTPAGENT_EDD_SPANINSEGRXUNKSENDBYTES_MID "c015011a"
#define ADM_LTPAGENT_EDD_SPANINSEGRXUNKCLIENTCNT_MID "c015011b"
#define ADM_LTPAGENT_EDD_SPANINSEGRXUNKCLIENTBYTES_MID "c015011c"
#define ADM_LTPAGENT_EDD_SPANINSEGSTRAYCNT_MID "c015011d"
#define ADM_LTPAGENT_EDD_SPANINSEGSTRAYBYTES_MID "c015011e"
#define ADM_LTPAGENT_EDD_SPANINSEGMISCOLORCNT_MID "c015011f"
#define ADM_LTPAGENT_EDD_SPANINSEGMISCOLORBYTES_MID "c0150120"
#define ADM_LTPAGENT_EDD_SPANINSEGCLOSEDCNT_MID "c0150121"
#define ADM_LTPAGENT_EDD_SPANINSEGCLOSEDBYTES_MID "c0150122"
#define ADM_LTPAGENT_EDD_SPANINCKPTRXCNT_MID "c0150123"
#define ADM_LTPAGENT_EDD_SPANINPOSACKTXCNT_MID "c0150124"
#define ADM_LTPAGENT_EDD_SPANINNEGACKTXCNT_MID "c0150125"
#define ADM_LTPAGENT_EDD_SPANINCANCELTXCNT_MID "c0150126"
#define ADM_LTPAGENT_EDD_SPANINACKRETXCNT_MID "c0150127"
#define ADM_LTPAGENT_EDD_SPANINCANCELRXCNT_MID "c0150128"
#define ADM_LTPAGENT_EDD_SPANINCOMPLETECNT_MID "c0150129"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                  LTPAGENT VARIABLE DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                LTPAGENT REPORT DEFINITIONS                                                           +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |fullSpanReport               |c2170100    |All data associated with a given span.            |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_LTPAGENT_RPT_FULLSPANREPORT_MID "c2170100"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                    LTPAGENT CONTROL DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |reset                        |c3170100    |Resets the counters associated with the engine and|             |
   |                             |            | updates the last reset time for the span to be th|             |
   |                             |            |e time when this control was run.                 |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |listEngines                  |83170101    |Lists all remote engine IDs.                      |             |
   +-----------------------------+------------+----------------------------------------------------------------+
 */

#define ADM_LTPAGENT_CTRL_RESET_MID "c3170100"
#define ADM_LTPAGENT_CTRL_LISTENGINES_MID "83170101"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                LTPAGENT CONSTANT DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                LTPAGENT MACRO DEFINITIONS                                                            +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |							      LTPAGENT OPERATOR DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_LtpAgent_init();
void adm_LtpAgent_init_edd();
void adm_LtpAgent_init_variables();
void adm_LtpAgent_init_controls();
void adm_LtpAgent_init_constants();
void adm_LtpAgent_init_macros();
void adm_LtpAgent_init_metadata();
void adm_LtpAgent_init_ops();
void adm_LtpAgent_init_reports();
#endif /* _HAVE_LTPAGENT_ADM_ */
#endif //ADM_LTPAGENT_H_
