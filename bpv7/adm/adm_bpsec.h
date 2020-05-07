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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_BPSEC_H_
#define ADM_BPSEC_H_
#define _HAVE_DTN_BPSEC_ADM_
#ifdef _HAVE_DTN_BPSEC_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/bpsec
 */
#define ADM_ENUM_DTN_BPSEC 10
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BPSEC META-DATA DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |bpsec                   |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |DTN/bpsec               |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM.               |STR    |v1.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_BPSEC_META_NAME 0x00
// "namespace"
#define DTN_BPSEC_META_NAMESPACE 0x01
// "version"
#define DTN_BPSEC_META_VERSION 0x02
// "organization"
#define DTN_BPSEC_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                        DTN_BPSEC EXTERNALLY DEFINED DATA DEFINITIONS                        +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bcb_blk  |Total successfully Tx Bundle Confident|       |
 * |                     |iality blocks                         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_blk   |Total unsuccessfully Tx Block Confiden|       |
 * |                     |tiality Block (BCB) blocks            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bcb_blk  |Total successfully Rx BCB blocks      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_blk   |Total unsuccessfully Rx BCB blocks    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_bl|Total missing-on-RX BCB blocks        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bcb_blks     |Total forward BCB blocks              |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bcb_bytes|Total successfully Tx BCB bytes       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_bytes |Total unsuccessfully Tx BCB bytes     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_blks  |Total unsuccessfully Tx BCB blocks    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bcb_bytes|Total successfully Rx BCB bytes       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_bytes |Total unsuccessfully Rx BCB bytes     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_by|Total missing-on-Rx BCB bytes         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bcb_bytes    |Total forwarded BCB bytes             |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bib_blks |Total successfully Tx Block Integrity |       |
 * |                     |Block (BIB) blocks                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bib_blks  |Total unsuccessfully Tx BIB blocks    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bib_blks |Total successfully Rx BIB blocks      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bib_blks  |Total unsuccessfully Rx BIB blocks    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_miss_rx_bib_blks |Total missing-on-Rx BIB blocks        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bib_blks     |Total forwarded BIB blocks            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bib_bytes|Total successfully Tx BIB bytes       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bib_bytes |Total unsuccessfully Tx BIB bytes     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bib_bytes|Total successfully Rx BIB bytes       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bib_bytes |Total unsuccessfully Rx BIB bytes     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_miss_rx_bib_bytes|Total missing-on-Rx BIB bytes         |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bib_bytes    |Total forwarded BIB bytes             |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |last_update          |Last BPSEC update                     |TV     |
 * +---------------------+--------------------------------------+-------+
 * |num_known_keys       |Number of known keys                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |key_names            |Known key names                       |STR    |
 * +---------------------+--------------------------------------+-------+
 * |ciphersuite_names    |Known ciphersuite names               |STR    |
 * +---------------------+--------------------------------------+-------+
 * |rule_source          |Known rule sources                    |STR    |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bcb_blks_|Number of successfully Tx BCB blocks f|       |
 * |                     |rom SRC                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_blks_s|Number of failed TX BCB blocks from SR|       |
 * |                     |C                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bcb_blks_|Number of successfully Rx BCB blocks f|       |
 * |                     |rom SRC                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_blks_s|Number of failed RX BCB blocks from SR|       |
 * |                     |C                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_bl|Number of missing-onRX BCB blocks from|       |
 * |                     | SRC                                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bcb_blks_src |Number of forwarded BCB blocks from SR|       |
 * |                     |C                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bcb_bytes|Number of successfully Tx bcb bytes fr|       |
 * |                     |om SRC                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_bytes_|Number of failed Tx bcb bytes from SRC|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bcb_bytes|Number of successfully Rx bcb bytes fr|       |
 * |                     |om SRC                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_bytes_|Number of failed Rx bcb bytes from SRC|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_by|Number of missing-on-Rx bcb bytes from|       |
 * |                     | SRC                                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bcb_bytes_src|Number of forwarded bcb bytes from SRC|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bib_blks_|Number of successfully Tx BIB blocks f|       |
 * |                     |rom SRC                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bib_blks_s|Number of failed Tx BIB blocks from SR|       |
 * |                     |C                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bib_blks_|Number of successfully Rx BIB blocks f|       |
 * |                     |rom SRC                               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bib_blks_s|Number of failed Rx BIB blocks from SR|       |
 * |                     |C                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_miss_rx_bib_blks_|Number of missing-on-Rx BIB blocks fro|       |
 * |                     |m SRC                                 |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bib_blks_src |Number of forwarded BIB blocks from SR|       |
 * |                     |C                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_tx_bib_bytes|Number of successfully Tx BIB bytes fr|       |
 * |                     |om SRC                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_tx_bib_bytes_|Number of failed Tx BIB bytes from SRC|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_good_rx_bib_bytes|Number of successfully Rx BIB bytes fr|       |
 * |                     |om SRC                                |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bad_rx_bib_bytes_|Number of failed Rx BIB bytes from SRC|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_missing_rx_bib_by|Number of missing-on-Rx BIB bytes from|       |
 * |                     | SRC                                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fwd_bib_bytes_src|Number of forwarded BIB bytes from SRC|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |last_update_src      |Last BPSEC update from SRC            |TV     |
 * +---------------------+--------------------------------------+-------+
 * |last_reset           |Last reset                            |TV     |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK 0x00
#define DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLK 0x01
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK 0x02
#define DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLK 0x03
#define DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS 0x04
#define DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS 0x05
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES 0x06
#define DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES 0x07
#define DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS 0x08
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES 0x09
#define DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES 0x0a
#define DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES 0x0b
#define DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES 0x0c
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS 0x0d
#define DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS 0x0e
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS 0x0f
#define DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS 0x10
#define DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS 0x11
#define DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS 0x12
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES 0x13
#define DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES 0x14
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES 0x15
#define DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES 0x16
#define DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES 0x17
#define DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES 0x18
#define DTN_BPSEC_EDD_LAST_UPDATE 0x19
#define DTN_BPSEC_EDD_NUM_KNOWN_KEYS 0x1a
#define DTN_BPSEC_EDD_KEY_NAMES 0x1b
#define DTN_BPSEC_EDD_CIPHERSUITE_NAMES 0x1c
#define DTN_BPSEC_EDD_RULE_SOURCE 0x1d
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC 0x1e
#define DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC 0x1f
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC 0x20
#define DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC 0x21
#define DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC 0x22
#define DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC 0x23
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC 0x24
#define DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC 0x25
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC 0x26
#define DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC 0x27
#define DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC 0x28
#define DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC 0x29
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC 0x2a
#define DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC 0x2b
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC 0x2c
#define DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC 0x2d
#define DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC 0x2e
#define DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC 0x2f
#define DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC 0x30
#define DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC 0x31
#define DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC 0x32
#define DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC 0x33
#define DTN_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC 0x34
#define DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC 0x35
#define DTN_BPSEC_EDD_LAST_UPDATE_SRC 0x36
#define DTN_BPSEC_EDD_LAST_RESET 0x37


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BPSEC VARIABLE DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |total_bad_tx_blks    |This is the number of failed TX blocks|       |
 * |                     | (# failed BIB + # failed bcb).       |UINT   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BPSEC_VAR_TOTAL_BAD_TX_BLKS 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                DTN_BPSEC REPORT DEFINITIONS                                 +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |full_report          |all known meta-data, externally define|       |
 * |                     |d data, and variables                 |TNVC   |
 * +---------------------+--------------------------------------+-------+
 * |source_report        |security info by source               |TNVC   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BPSEC_RPTTPL_FULL_REPORT 0x00
#define DTN_BPSEC_RPTTPL_SOURCE_REPORT 0x01


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 DTN_BPSEC TABLE DEFINITIONS                                 +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |keys                 |This table lists all keys in the secur|       |
 * |                     |ity policy database.                  |       |
 * +---------------------+--------------------------------------+-------+
 * |ciphersuites         |This table lists supported ciphersuite|       |
 * |                     |s.                                    |       |
 * +---------------------+--------------------------------------+-------+
 * |bib_rules            |BIB Rules.                            |       |
 * +---------------------+--------------------------------------+-------+
 * |bcb_rules            |BCB Rules.                            |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BPSEC_TBLT_KEYS 0x00
#define DTN_BPSEC_TBLT_CIPHERSUITES 0x01
#define DTN_BPSEC_TBLT_BIB_RULES 0x02
#define DTN_BPSEC_TBLT_BCB_RULES 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                DTN_BPSEC CONTROL DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |rst_all_cnts         |This control causes the Agent to reset|       |
 * |                     | all counts associated with block or b|       |
 * |                     |yte statistics and to set the Last Res|       |
 * |                     |et Time of the BPsec EDD data to the t|       |
 * |                     |ime when the control was run.         |       |
 * +---------------------+--------------------------------------+-------+
 * |rst_src_cnts         |This control causes the Agent to reset|       |
 * |                     | all counts (blocks and bytes) associa|       |
 * |                     |ted with a given bundle source and set|       |
 * |                     | the Last Reset Time of the source sta|       |
 * |                     |tistics to the time when the control w|       |
 * |                     |as run.                               |       |
 * +---------------------+--------------------------------------+-------+
 * |delete_key           |This control deletes a key from the BP|       |
 * |                     |sec system.                           |       |
 * +---------------------+--------------------------------------+-------+
 * |add_key              |This control adds a key to the BPsec s|       |
 * |                     |ystem.                                |       |
 * +---------------------+--------------------------------------+-------+
 * |add_bib_rule         |This control configures policy on the |       |
 * |                     |BPsec protocol implementation that des|       |
 * |                     |cribes how BIB blocks should be applie|       |
 * |                     |d to bundles in the system. This polic|       |
 * |                     |y is captured as a rule which states w|       |
 * |                     |hen transmitting a bundle from the giv|       |
 * |                     |en source endpoint ID to the given des|       |
 * |                     |tination endpoint ID, blocks of type t|       |
 * |                     |arget should have a BIB added to them |       |
 * |                     |using the given ciphersuite and the gi|       |
 * |                     |ven key.                              |       |
 * +---------------------+--------------------------------------+-------+
 * |del_bib_rule         |This control removes any configured po|       |
 * |                     |licy on the BPsec protocol implementat|       |
 * |                     |ion that describes how BIB blocks shou|       |
 * |                     |ld be applied to bundles in the system|       |
 * |                     |. A BIB policy is uniquely identified |       |
 * |                     |by a source endpoint Id, a destination|       |
 * |                     | Id, and a target block type.         |       |
 * +---------------------+--------------------------------------+-------+
 * |add_bcb_rule         |This control configures policy on the |       |
 * |                     |BPsec protocol implementation that des|       |
 * |                     |cribes how BCB blocks should be applie|       |
 * |                     |d to bundles in the system. This polic|       |
 * |                     |y is captured as a rule which states w|       |
 * |                     |hen transmitting a bundle from the giv|       |
 * |                     |en source endpoint id to the given des|       |
 * |                     |tination endpoint id, blocks of type t|       |
 * |                     |arget should have a bcb added to them |       |
 * |                     |using the given ciphersuite and the gi|       |
 * |                     |ven key.                              |       |
 * +---------------------+--------------------------------------+-------+
 * |del_bcb_rule         |This control removes any configured po|       |
 * |                     |licy on the BPsec protocol implementat|       |
 * |                     |ion that describes how BCB blocks shou|       |
 * |                     |ld be applied to bundles in the system|       |
 * |                     |. A bcb policy is uniquely identified |       |
 * |                     |by a source endpoint id, a destination|       |
 * |                     | endpoint id, and a target block type.|       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BPSEC_CTRL_RST_ALL_CNTS 0x00
#define DTN_BPSEC_CTRL_RST_SRC_CNTS 0x01
#define DTN_BPSEC_CTRL_DELETE_KEY 0x02
#define DTN_BPSEC_CTRL_ADD_KEY 0x03
#define DTN_BPSEC_CTRL_ADD_BIB_RULE 0x04
#define DTN_BPSEC_CTRL_DEL_BIB_RULE 0x05
#define DTN_BPSEC_CTRL_ADD_BCB_RULE 0x06
#define DTN_BPSEC_CTRL_DEL_BCB_RULE 0x07


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BPSEC CONSTANT DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 DTN_BPSEC MACRO DEFINITIONS                                 +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BPSEC OPERATOR DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_bpsec_init();
void dtn_bpsec_init_meta();
void dtn_bpsec_init_cnst();
void dtn_bpsec_init_edd();
void dtn_bpsec_init_op();
void dtn_bpsec_init_var();
void dtn_bpsec_init_ctrl();
void dtn_bpsec_init_mac();
void dtn_bpsec_init_rpttpl();
void dtn_bpsec_init_tblt();
#endif /* _HAVE_DTN_BPSEC_ADM_ */
#endif //ADM_BPSEC_H_