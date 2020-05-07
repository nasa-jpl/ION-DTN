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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_LTP_AGENT_H_
#define ADM_LTP_AGENT_H_
#define _HAVE_DTN_LTP_AGENT_ADM_
#ifdef _HAVE_DTN_LTP_AGENT_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ltp_agent
 */
#define ADM_ENUM_DTN_LTP_AGENT 3
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_LTP_AGENT META-DATA DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |ltp_agent               |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |DTN/ltp_agent           |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM.               |STR    |v0.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_LTP_AGENT_META_NAME 0x00
// "namespace"
#define DTN_LTP_AGENT_META_NAMESPACE 0x01
// "version"
#define DTN_LTP_AGENT_META_VERSION 0x02
// "organization"
#define DTN_LTP_AGENT_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                      DTN_LTP_AGENT EXTERNALLY DEFINED DATA DEFINITIONS                      +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |span_remote_engine_nb|The remote engine number of this span.|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_cur_expt_sess   |Expected sessions on this span.       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_cur_out_seg     |The current number of outbound segment|       |
 * |                     |s for this span.                      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_cur_imp_sess    |The current number of import segments |       |
 * |                     |for this span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_cur_in_seg      |The current number of inbound segments|       |
 * |                     | for this span.                       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_reset_time      |The last time the span counters were r|       |
 * |                     |eset.                                 |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |span_out_seg_q_cnt   |The output segment queued count for th|       |
 * |                     |e span.                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_seg_q_bytes |The output segment queued bytes for th|       |
 * |                     |e span.                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_seg_pop_cnt |The output segment popped count for th|       |
 * |                     |e span.                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_seg_pop_byte|The output segment popped bytes for th|       |
 * |                     |e span.                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_ckpt_xmit_cn|The output checkpoint transmit count f|       |
 * |                     |or the span.                          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_pos_ack_rx_c|The output positive acknowledgement re|       |
 * |                     |ceived count for the span.            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_neg_ack_rx_c|The output negative acknowledgement re|       |
 * |                     |ceived count for the span.            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_cancel_rx_cn|The output cancelled received count fo|       |
 * |                     |r the span.                           |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_ckpt_rexmit_|The output checkpoint retransmit count|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_cancel_xmit_|The output cancel retransmit count for|       |
 * |                     | the span.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_out_complete_cnt|The output completed count for the spa|       |
 * |                     |n.                                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_red_cn|The input segment received red count f|       |
 * |                     |or the span.                          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_red_by|The input segment received red bytes f|       |
 * |                     |or the span.                          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_green_|The input segment received green count|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_green_|The input segment received green bytes|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_redund|The input segment received redundant c|       |
 * |                     |ount for the span.                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_redund|The input segment received redundant b|       |
 * |                     |ytes for the span.                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_mal_cn|The input segment malformed count for |       |
 * |                     |the span.                             |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_mal_by|The input segment malformed bytes for |       |
 * |                     |the span.                             |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_unk_se|The input segment unknown sender count|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_unk_se|The input segment unknown sender bytes|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_unk_cl|The input segment unknown client count|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_rx_unk_cl|The input segment unknown client bytes|       |
 * |                     | for the span.                        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_stray_cnt|The input segment stray count for the |       |
 * |                     |span.                                 |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_stray_byt|The input segment stray bytes for the |       |
 * |                     |span.                                 |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_miscolor_|The input segment miscolored count for|       |
 * |                     | the span.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_miscolor_|The input segment miscolored bytes for|       |
 * |                     | the span.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_closed_cn|The input segment closed count for the|       |
 * |                     | span.                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_seg_closed_by|The input segment closed bytes for the|       |
 * |                     | span.                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_ckpt_rx_cnt  |The input checkpoint receive count for|       |
 * |                     | the span.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_pos_ack_tx_cn|The input positive acknolwedgement tra|       |
 * |                     |nsmitted count for the span.          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_neg_ack_tx_cn|The input negative acknolwedgement tra|       |
 * |                     |nsmitted count for the span.          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_cancel_tx_cnt|The input cancel transmitted count for|       |
 * |                     | the span.                            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_ack_retx_cnt |The input acknolwedgement retransmit c|       |
 * |                     |ount for the span.                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_cancel_rx_cnt|The input cancel receive count for the|       |
 * |                     | span.                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |span_in_complete_cnt |The input completed count for the span|       |
 * |                     |.                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_LTP_AGENT_EDD_SPAN_REMOTE_ENGINE_NBR 0x00
#define DTN_LTP_AGENT_EDD_SPAN_CUR_EXPT_SESS 0x01
#define DTN_LTP_AGENT_EDD_SPAN_CUR_OUT_SEG 0x02
#define DTN_LTP_AGENT_EDD_SPAN_CUR_IMP_SESS 0x03
#define DTN_LTP_AGENT_EDD_SPAN_CUR_IN_SEG 0x04
#define DTN_LTP_AGENT_EDD_SPAN_RESET_TIME 0x05
#define DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_CNT 0x06
#define DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_Q_BYTES 0x07
#define DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_CNT 0x08
#define DTN_LTP_AGENT_EDD_SPAN_OUT_SEG_POP_BYTES 0x09
#define DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_XMIT_CNT 0x0a
#define DTN_LTP_AGENT_EDD_SPAN_OUT_POS_ACK_RX_CNT 0x0b
#define DTN_LTP_AGENT_EDD_SPAN_OUT_NEG_ACK_RX_CNT 0x0c
#define DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_RX_CNT 0x0d
#define DTN_LTP_AGENT_EDD_SPAN_OUT_CKPT_REXMIT_CNT 0x0e
#define DTN_LTP_AGENT_EDD_SPAN_OUT_CANCEL_XMIT_CNT 0x0f
#define DTN_LTP_AGENT_EDD_SPAN_OUT_COMPLETE_CNT 0x10
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_CNT 0x11
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_RED_BYTES 0x12
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_CNT 0x13
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_GREEN_BYTES 0x14
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_CNT 0x15
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_REDUNDANT_BYTES 0x16
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_CNT 0x17
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_MAL_BYTES 0x18
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_CNT 0x19
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_SENDER_BYTES 0x1a
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_CNT 0x1b
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_RX_UNK_CLIENT_BYTES 0x1c
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_CNT 0x1d
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_STRAY_BYTES 0x1e
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_CNT 0x1f
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_MISCOLOR_BYTES 0x20
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_CNT 0x21
#define DTN_LTP_AGENT_EDD_SPAN_IN_SEG_CLOSED_BYTES 0x22
#define DTN_LTP_AGENT_EDD_SPAN_IN_CKPT_RX_CNT 0x23
#define DTN_LTP_AGENT_EDD_SPAN_IN_POS_ACK_TX_CNT 0x24
#define DTN_LTP_AGENT_EDD_SPAN_IN_NEG_ACK_TX_CNT 0x25
#define DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_TX_CNT 0x26
#define DTN_LTP_AGENT_EDD_SPAN_IN_ACK_RETX_CNT 0x27
#define DTN_LTP_AGENT_EDD_SPAN_IN_CANCEL_RX_CNT 0x28
#define DTN_LTP_AGENT_EDD_SPAN_IN_COMPLETE_CNT 0x29


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_LTP_AGENT VARIABLE DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_LTP_AGENT REPORT DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |endpointReport       |This is all known endpoint information|TNVC   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_LTP_AGENT_RPTTPL_ENDPOINTREPORT 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_LTP_AGENT TABLE DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |engines              |This table lists all known remote engi|       |
 * |                     |ne ids.                               |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_LTP_AGENT_TBLT_ENGINES 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_LTP_AGENT CONTROL DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |reset                |Resets the counters associated with th|       |
 * |                     |e engine and updates the last reset ti|       |
 * |                     |me for the span to be the time when th|       |
 * |                     |is control was run.                   |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_LTP_AGENT_CTRL_RESET 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_LTP_AGENT CONSTANT DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_LTP_AGENT MACRO DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_LTP_AGENT OPERATOR DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_ltp_agent_init();
void dtn_ltp_agent_init_meta();
void dtn_ltp_agent_init_cnst();
void dtn_ltp_agent_init_edd();
void dtn_ltp_agent_init_op();
void dtn_ltp_agent_init_var();
void dtn_ltp_agent_init_ctrl();
void dtn_ltp_agent_init_mac();
void dtn_ltp_agent_init_rpttpl();
void dtn_ltp_agent_init_tblt();
#endif /* _HAVE_DTN_LTP_AGENT_ADM_ */
#endif //ADM_LTP_AGENT_H_