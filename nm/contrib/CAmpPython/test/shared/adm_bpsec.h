/****************************************************************************
 **
 ** File Name: adm_bpsec.h
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


#ifndef ADM_BPSEC_H_
#define ADM_BPSEC_H_
#define _HAVE_BPSEC_ADM_
#ifdef _HAVE_BPSEC_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"


/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:bpsec
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define BPSEC_ADM_META_NN_IDX 40
#define BPSEC_ADM_META_NN_STR "40"

#define BPSEC_ADM_EDD_NN_IDX 41
#define BPSEC_ADM_EDD_NN_STR "41"

#define BPSEC_ADM_VAR_NN_IDX 42
#define BPSEC_ADM_VAR_NN_STR "42"

#define BPSEC_ADM_RPT_NN_IDX 43
#define BPSEC_ADM_RPT_NN_STR "43"

#define BPSEC_ADM_CTRL_NN_IDX 44
#define BPSEC_ADM_CTRL_NN_STR "44"

#define BPSEC_ADM_CONST_NN_IDX 45
#define BPSEC_ADM_CONST_NN_STR "45"

#define BPSEC_ADM_MACRO_NN_IDX 46
#define BPSEC_ADM_MACRO_NN_STR "46"

#define BPSEC_ADM_OP_NN_IDX 47
#define BPSEC_ADM_OP_NN_STR "47"

#define BPSEC_ADM_TBL_NN_IDX 48
#define BPSEC_ADM_TBL_NN_STR "48"

#define BPSEC_ADM_ROOT_NN_IDX 49
#define BPSEC_ADM_ROOT_NN_STR "49"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x87280100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x87280101  |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x87280102  |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x87280103  |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_BPSEC_META_NAME_MID 0x87280100
// "namespace"
#define ADM_BPSEC_META_NAMESPACE_MID 0x87280101
// "version"
#define ADM_BPSEC_META_VERSION_MID 0x87280102
// "organization"
#define ADM_BPSEC_META_ORGANIZATION_MID 0x87280103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bcb_blk          |0x80290100  |Total successfully Tx BCB blocks                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bcb_blk           |0x80290101  |Total unsuccessfully Tx BCB blocks                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bcb_blk          |0x80290102  |Total successfully Rx BCB blocks                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bcb_blk           |0x80290103  |Total unsuccessfully Rx BCB blocks                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_missing_rx_bcb_blks      |0x80290104  |Total missing-on-RX BCB blocks                    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bcb_blks             |0x80290105  |Total forward BCB blocks                          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bcb_bytes        |0x80290106  |Total successfully Tx bcb bytes                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bcb_bytes         |0x80290107  |Total unsuccessfully Tx bcb bytes                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bcb_bytes        |0x80290108  |Total successfully Rx bcb bytes                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bcb_bytes         |0x80290109  |Total unsuccessfully Rx bcb bytes                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_missing_rx_bcb_bytes     |0x8029010a  |Total missing-on-Rx bcb bytes                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bcb_bytes            |0x8029010b  |Total forwarded bcb bytes                         |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bib_blks         |0x8029010c  |Total successfully Tx BIB blocks                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bib_blks          |0x8029010d  |Total unsuccessfully Tx BIB blocks                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bib_blks         |0x8029010e  |Total successfully Rx BIB blocks                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bib_blks          |0x8029010f  |Total unsuccessfully Rx BIB blocks                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_miss_rx_bib_blks         |0x80290110  |Total missing-on-Rx BIB blocks                    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bib_blks             |0x80290111  |Total forwarded BIB blocks                        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bib_bytes        |0x80290112  |Total successfully Tx BIB bytes                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bib_bytes         |0x80290113  |Total unsuccessfully Tx BIB bytes                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bib_bytes        |0x80290114  |Total successfully Rx BIB bytes                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bib_bytes         |0x80290115  |Total unsuccessfully Rx BIB bytes                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_miss_rx_bib_bytes        |0x80290116  |Total missing-on-Rx BIB bytes                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bib_bytes            |0x80290117  |Total forwarded BIB bytes                         |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |last_update                  |0x80290118  |Last BPSEC update                                 |TS           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_known_keys               |0x80290119  |Number of known keys                              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |key_names                    |0x8029011a  |Known key names                                   |STRING       |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ciphersuite_names            |0x8029011b  |Known ciphersuite names                           |STRING       |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |rule_source                  |0x8029011c  |Known rule sources                                |STRING       |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bcb_blks_src     |0xc029011d  |Successfully Tx BCB blocks from SRC               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bcb_blks_src      |0xc029011e  |Failed TX BCB blocks from SRC                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bcb_blks_src     |0xc029011f  |Successfully Rx BCB blocks from SRC               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bcb_blks_src      |0xc0290120  |Failed RX BCB blocks from SRC                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_missing_rx_bcb_blks_src  |0xc0290121  |Missing-onRX BCB blocks from SRC                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bcb_blks_src         |0xc0290122  |Forwarded BCB blocks from SRC                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bcb_bytes_src    |0xc0290123  |Successfullt Tx bcb bytes from SRC                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bcb_bytes_src     |0xc0290124  |Failed Tx bcb bytes from SRC                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bcb_bytes_src    |0xc0290125  |Successfully Rx bcb bytes from SRC                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bcb_bytes_src     |0xc0290126  |Failed Rx bcb bytes from SRC                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_missing_rx_bcb_bytes_src |0xc0290127  |Missin-on-Rx bcb bytes from SRC                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bcb_bytes_src        |0xc0290128  |Forwarded bcb bytes from SRC                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bib_blks_src     |0xc0290129  |Successfully Tx BIB blocks from SRC               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bib_blks_src      |0xc029012a  |Failed Tx BIB blocks from SRC                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bib_blks_src     |0xc029012b  |Successfully Rx BIB blocks from SRC               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bib_blks_src      |0xc029012c  |Failed Rx BIB blocks from SRC                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_miss_rx_bib_blks_src     |0xc029012d  |Missing-on-Rx BIB blocks from SRC                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bib_blks_src         |0xc029012e  |Forwarded BIB blocks from SRC                     |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_tx_bib_bytes_src    |0xc029012f  |Successfully Tx BIB bytes from SRC                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_tx_bib_bytes_src     |0xc0290130  |Failed Tx BIB bytes from SRC                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_good_rx_bib_bytes_src    |0xc0290131  |Successfully Rx BIB bytes from SRC                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bad_rx_bib_bytes_src     |0xc0290132  |Failed Rx BIB bytes from SRC                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_missing_rx_bib_bytes_src |0xc0290133  |Missing-on-Rx BIB bytes from SRC                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fwd_bib_bytes_src        |0xc0290134  |Forwarded BIB bytes from SRC                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |last_update_src              |0xc0290135  |Last BPSEC Update from SRC                        |TS           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |last_reset                   |0xc0290136  |Last reset                                        |TS           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK_MID 0x80290100
#define ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLK_MID 0x80290101
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK_MID 0x80290102
#define ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLK_MID 0x80290103
#define ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_MID 0x80290104
#define ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_MID 0x80290105
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_MID 0x80290106
#define ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_MID 0x80290107
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_MID 0x80290108
#define ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_MID 0x80290109
#define ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_MID 0x8029010a
#define ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_MID 0x8029010b
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_MID 0x8029010c
#define ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_MID 0x8029010d
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_MID 0x8029010e
#define ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_MID 0x8029010f
#define ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_MID 0x80290110
#define ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_MID 0x80290111
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_MID 0x80290112
#define ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_MID 0x80290113
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_MID 0x80290114
#define ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_MID 0x80290115
#define ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES_MID 0x80290116
#define ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_MID 0x80290117
#define ADM_BPSEC_EDD_LAST_UPDATE_MID 0x80290118
#define ADM_BPSEC_EDD_NUM_KNOWN_KEYS_MID 0x80290119
#define ADM_BPSEC_EDD_KEY_NAMES_MID 0x8029011a
#define ADM_BPSEC_EDD_CIPHERSUITE_NAMES_MID 0x8029011b
#define ADM_BPSEC_EDD_RULE_SOURCE_MID 0x8029011c
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID 0xc029011d
#define ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID 0xc029011e
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID 0xc029011f
#define ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID 0xc0290120
#define ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID 0xc0290121
#define ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID 0xc0290122
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID 0xc0290123
#define ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID 0xc0290124
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID 0xc0290125
#define ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID 0xc0290126
#define ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID 0xc0290127
#define ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID 0xc0290128
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID 0xc0290129
#define ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID 0xc029012a
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID 0xc029012b
#define ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID 0xc029012c
#define ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID 0xc029012d
#define ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID 0xc029012e
#define ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID 0xc029012f
#define ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID 0xc0290130
#define ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID 0xc0290131
#define ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID 0xc0290132
#define ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID 0xc0290133
#define ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID 0xc0290134
#define ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID 0xc0290135
#define ADM_BPSEC_EDD_LAST_RESET_MID 0xc0290136


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |total_bad_tx_blks            |0x812a0100  |This is the number of failed TX blocks (# failed B|             |
   |                             |            |IB + # failed bcb).                               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BPSEC_VAR_TOTAL_BAD_TX_BLKS_MID 0x812a0100


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |full_report                  |0xc22b0100  |all known meta-data, externally defined data, and |             |
   |                             |            |variables                                         |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |source_report                |0xc22b0101  |security info by source                           |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BPSEC_RPT_FULL_REPORT_MID 0xc22b0100
#define ADM_BPSEC_RPT_SOURCE_REPORT_MID 0xc22b0101


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |rst_all_cnts                 |0x832c0100  |This control causes the Agent to reset all counts |             |
   |                             |            |associated with block or byte statistics and to se|             |
   |                             |            |t the Last Reset Time of the BPsec EDD data to the|             |
   |                             |            | time when the control was run.                   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |rst_src_cnts                 |0xc32c0101  |This control causes the Agent to reset all counts |             |
   |                             |            |(blocks and bytes) associated with a given bundle |             |
   |                             |            |source and set the Last Reset Time of the source s|             |
   |                             |            |tatistics to the time when the control was run.   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |delete_key                   |0xc32c0102  |This control deletes a key from the BPsec system. |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |add_key                      |0xc32c0103  |This control adds a key to the BPsec system.      |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |add_bib_rule                 |0xc32c0104  |This control configures policy on the BPsec protoc|             |
   |                             |            |ol implementation that describes how BIB blocks sh|             |
   |                             |            |ould be applied to bundles in the system. This pol|             |
   |                             |            |icy is captured as a rule which states when transm|             |
   |                             |            |itting a bundle from the given source endpoint ID |             |
   |                             |            |to the given destination endpoint ID, blocks of ty|             |
   |                             |            |pe target should have a BIB added to them using th|             |
   |                             |            |e given ciphersuite and the given key.            |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |del_bib_rule                 |0xc32c0105  |This control removes any configured policy on the |             |
   |                             |            |BPsec protocol implementation that describes how B|             |
   |                             |            |IB blocks should be applied to bundles in the syst|             |
   |                             |            |em. A BIB policy is uniquely identified by a sourc|             |
   |                             |            |e endpoint Id, a destination Id, and a target bloc|             |
   |                             |            |k type.                                           |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |list_bib_rules               |0x832c0106  |This control returns a table describinng all of th|             |
   |                             |            |e BIB policy rules that are known to the BPsec imp|             |
   |                             |            |lementation.                                      |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |add_bcb_rule                 |0xc32c0107  |This control configures policy on the BPsec protoc|             |
   |                             |            |ol implementation that describes how BCB blocks sh|             |
   |                             |            |ould be applied to bundles in the system. This pol|             |
   |                             |            |icy is captured as a rule which states when transm|             |
   |                             |            |itting a bundle from the given source endpoint id |             |
   |                             |            |to the given destination endpoint id, blocks of ty|             |
   |                             |            |pe target should have a bcb added to them using th|             |
   |                             |            |e given ciphersuite and the given key.            |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |del_bcb_rule                 |0xc32c0108  |This control removes any configured policy on the |             |
   |                             |            |BPsec protocol implementation that describes how B|             |
   |                             |            |CB blocks should be applied to bundles in the syst|             |
   |                             |            |em. A bcb policy is uniquely identified by a sourc|             |
   |                             |            |e endpoint id, a destination endpoint id, and a ta|             |
   |                             |            |rget block type.                                  |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |list_bcb_rules               |0x832c0109  |This control returns a table describing all of the|             |
   |                             |            | bcb policy rules that are known to the BPsec impl|             |
   |                             |            |ementation                                        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BPSEC_CTRL_RST_ALL_CNTS_MID 0x832c0100
#define ADM_BPSEC_CTRL_RST_SRC_CNTS_MID 0xc32c0101
#define ADM_BPSEC_CTRL_DELETE_KEY_MID 0xc32c0102
#define ADM_BPSEC_CTRL_ADD_KEY_MID 0xc32c0103
#define ADM_BPSEC_CTRL_ADD_BIB_RULE_MID 0xc32c0104
#define ADM_BPSEC_CTRL_DEL_BIB_RULE_MID 0xc32c0105
#define ADM_BPSEC_CTRL_LIST_BIB_RULES_MID 0x832c0106
#define ADM_BPSEC_CTRL_ADD_BCB_RULE_MID 0xc32c0107
#define ADM_BPSEC_CTRL_DEL_BCB_RULE_MID 0xc32c0108
#define ADM_BPSEC_CTRL_LIST_BCB_RULES_MID 0x832c0109


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BPSEC OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_bpsec_init();
void adm_bpsec_init_edd();
void adm_bpsec_init_variables();
void adm_bpsec_init_controls();
void adm_bpsec_init_constants();
void adm_bpsec_init_macros();
void adm_bpsec_init_metadata();
void adm_bpsec_init_ops();
void adm_bpsec_init_reports();
#endif /* _HAVE_BPSEC_ADM_ */
#endif //ADM_BPSEC_H_