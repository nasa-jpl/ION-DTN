/****************************************************************************
 **
 ** File Name: adm_ltp_agent.h
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
 **  2018-03-16  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_LTP_AGENT_H_
#define ADM_LTP_AGENT_H_
#define _HAVE_LTP_AGENT_ADM_
#ifdef _HAVE_LTP_AGENT_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"


/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:ltp_agent
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define LTP_AGENT_ADM_META_NN_IDX 20
#define LTP_AGENT_ADM_META_NN_STR "20"

#define LTP_AGENT_ADM_EDD_NN_IDX 21
#define LTP_AGENT_ADM_EDD_NN_STR "21"

#define LTP_AGENT_ADM_VAR_NN_IDX 22
#define LTP_AGENT_ADM_VAR_NN_STR "22"

#define LTP_AGENT_ADM_RPT_NN_IDX 23
#define LTP_AGENT_ADM_RPT_NN_STR "23"

#define LTP_AGENT_ADM_CTRL_NN_IDX 24
#define LTP_AGENT_ADM_CTRL_NN_STR "24"

#define LTP_AGENT_ADM_CONST_NN_IDX 25
#define LTP_AGENT_ADM_CONST_NN_STR "25"

#define LTP_AGENT_ADM_MACRO_NN_IDX 26
#define LTP_AGENT_ADM_MACRO_NN_STR "26"

#define LTP_AGENT_ADM_OP_NN_IDX 27
#define LTP_AGENT_ADM_OP_NN_STR "27"

#define LTP_AGENT_ADM_TBL_NN_IDX 28
#define LTP_AGENT_ADM_TBL_NN_STR "28"

#define LTP_AGENT_ADM_ROOT_NN_IDX 29
#define LTP_AGENT_ADM_ROOT_NN_STR "29"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x87140100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x87140101  |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x87140102  |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x87140103  |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_LTP_AGENT_META_NAME_MID 0x87140100
// "namespace"
#define ADM_LTP_AGENT_META_NAMESPACE_MID 0x87140101
// "version"
#define ADM_LTP_AGENT_META_VERSION_MID 0x87140102
// "organization"
#define ADM_LTP_AGENT_META_ORGANIZATION_MID 0x87140103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_remote_engine_nbr       |0xc0150100  |The remote engine number of this span.            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_cur_expt_sess           |0xc0150101  |Expected sessions on this span.                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_cur_out_seg             |0xc0150102  |The current number of outbound segments for this s|             |
   |                             |            |pan.                                              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_cur_imp_sess            |0xc0150103  |The current number of import segments for this spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_cur_in_seg              |0xc0150104  |The current number of inbound segments for this sp|             |
   |                             |            |an.                                               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_reset_time              |0xc0150105  |The last time the span counters were reset.       |UVAST        |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_seg_q_cnt           |0xc0150106  |The output segment queued count for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_seg_q_bytes         |0xc0150107  |The output segment queued bytes for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_seg_pop_cnt         |0xc0150108  |The output segment popped count for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_seg_pop_bytes       |0xc0150109  |The output segment popped bytes for the span.     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_ckpt_xmit_cnt       |0xc015010a  |The output checkpoint transmit count for the span.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_pos_ack_rx_cnt      |0xc015010b  |The output positive acknowledgement received count|             |
   |                             |            | for the span.                                    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_neg_ack_rx_cnt      |0xc015010c  |The output negative acknowledgement received count|             |
   |                             |            | for the span.                                    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_cancel_rx_cnt       |0xc015010d  |The output cancelled received count for the span. |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_ckpt_rexmit_cnt     |0xc015010e  |The output checkpoint retransmit count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_cancel_xmit_cnt     |0xc015010f  |The output cancel retransmit count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_out_complete_cnt        |0xc0150110  |The output completed count for the span.          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_red_cnt       |0xc0150111  |The input segment received red count for the span.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_red_bytes     |0xc0150112  |The input segment received red bytes for the span.|UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_green_cnt     |0xc0150113  |The input segment received green count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_green_bytes   |0xc0150114  |The input segment received green bytes for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_redundant_cnt |0xc0150115  |The input segment received redundant count for the|             |
   |                             |            | span.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_redundant_byte|0xc0150116  |The input segment received redundant bytes for the|             |
   |                             |            | span.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_mal_cnt       |0xc0150117  |The input segment malformed count for the span.   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_mal_bytes     |0xc0150118  |The input segment malformed bytes for the span.   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_unk_sender_cnt|0xc0150119  |The input segment unknown sender count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_unk_sender_byt|0xc015011a  |The input segment unknown sender bytes for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_unk_client_cnt|0xc015011b  |The input segment unknown client count for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_rx_unk_client_byt|0xc015011c  |The input segment unknown client bytes for the spa|             |
   |                             |            |n.                                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_stray_cnt        |0xc015011d  |The input segment stray count for the span.       |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_stray_bytes      |0xc015011e  |The input segment stray bytes for the span.       |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_miscolor_cnt     |0xc015011f  |The input segment miscolored count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_miscolor_bytes   |0xc0150120  |The input segment miscolored bytes for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_closed_cnt       |0xc0150121  |The input segment closed count for the span.      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_seg_closed_bytes     |0xc0150122  |The input segment closed bytes for the span.      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_ckpt_rx_cnt          |0xc0150123  |The input checkpoint receive count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_pos_ack_tx_cnt       |0xc0150124  |The input positive acknolwedgement transmitted cou|             |
   |                             |            |nt for the span.                                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_neg_ack_tx_cnt       |0xc0150125  |The input negative acknolwedgement transmitted cou|             |
   |                             |            |nt for the span.                                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_cancel_tx_cnt        |0xc0150126  |The input cancel transmitted count for the span.  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_ack_retx_cnt         |0xc0150127  |The input acknolwedgement retransmit count for the|             |
   |                             |            | span.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_cancel_rx_cnt        |0xc0150128  |The input cancel receive count for the span.      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |span_in_complete_cnt         |0xc0150129  |The input completed count for the span.           |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR_MID 0xc0150100
#define ADM_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS_MID 0xc0150101
#define ADM_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG_MID 0xc0150102
#define ADM_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS_MID 0xc0150103
#define ADM_LTP_AGENT_EDD_SPAN_CUR_IN_SEG_MID 0xc0150104
#define ADM_LTP_AGENT_EDD_SPAN_RESET_TIME_MID 0xc0150105
#define ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT_MID 0xc0150106
#define ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES_MID 0xc0150107
#define ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT_MID 0xc0150108
#define ADM_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES_MID 0xc0150109
#define ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT_MID 0xc015010a
#define ADM_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT_MID 0xc015010b
#define ADM_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT_MID 0xc015010c
#define ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT_MID 0xc015010d
#define ADM_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT_MID 0xc015010e
#define ADM_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT_MID 0xc015010f
#define ADM_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT_MID 0xc0150110
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT_MID 0xc0150111
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES_MID 0xc0150112
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT_MID 0xc0150113
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES_MID 0xc0150114
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT_MID 0xc0150115
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES_MID 0xc0150116
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT_MID 0xc0150117
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES_MID 0xc0150118
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT_MID 0xc0150119
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES_MID 0xc015011a
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT_MID 0xc015011b
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES_MID 0xc015011c
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT_MID 0xc015011d
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES_MID 0xc015011e
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT_MID 0xc015011f
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES_MID 0xc0150120
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT_MID 0xc0150121
#define ADM_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES_MID 0xc0150122
#define ADM_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT_MID 0xc0150123
#define ADM_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT_MID 0xc0150124
#define ADM_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT_MID 0xc0150125
#define ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT_MID 0xc0150126
#define ADM_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT_MID 0xc0150127
#define ADM_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT_MID 0xc0150128
#define ADM_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT_MID 0xc0150129


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpointReport               |0xc2170100  |This is all known endpoint information            |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_LTP_AGENT_RPT_ENDPOINTREPORT_MID 0xc2170100


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |reset                        |0xc3180100  |Resets the counters associated with the engine and|             |
   |                             |            | updates the last reset time for the span to be th|             |
   |                             |            |e time when this control was run.                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |list_engines                 |0x83180101  |Lists all remote engine IDs.                      |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_LTP_AGENT_CTRL_RESET_MID 0xc3180100
#define ADM_LTP_AGENT_CTRL_LIST_ENGINES_MID 0x83180101


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    LTP_AGENT OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_ltp_agent_init();
void adm_ltp_agent_init_edd();
void adm_ltp_agent_init_variables();
void adm_ltp_agent_init_controls();
void adm_ltp_agent_init_constants();
void adm_ltp_agent_init_macros();
void adm_ltp_agent_init_metadata();
void adm_ltp_agent_init_ops();
void adm_ltp_agent_init_reports();
#endif /* _HAVE_LTP_AGENT_ADM_ */
#endif //ADM_LTP_AGENT_H_