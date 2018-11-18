<<<<<<< local
/*****************************************************************************
=======
/****************************************************************************
>>>>>>> other
 **
 ** File Name: adm_sbsp.h
 **
<<<<<<< local
 ** Description: This implements the public portions of a AMP SBSP ADM.
=======
 ** Description: TODO
>>>>>>> other
 **
<<<<<<< local
 ** Notes:
=======
 ** Notes: TODO
>>>>>>> other
 **
<<<<<<< local
 ** Assumptions:
=======
 ** Assumptions: TODO
>>>>>>> other
 **
<<<<<<< local
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
=======
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2018-11-15  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

>>>>>>> other

#ifndef ADM_SBSP_H_
#define ADM_SBSP_H_
<<<<<<< local

#define _HAVE_SBSP_ADM_

#ifdef _HAVE_SBSP_ADM_

#include "lyst.h"
#include "sbsp_instr.h"

=======
#define _HAVE_DTN_SBSP_ADM_
#ifdef _HAVE_DTN_SBSP_ADM_
>>>>>>> other

#include "../utils/nm_types.h"
<<<<<<< local
#include "../adm/adm.h"
=======
#include "adm.h"
>>>>>>> other

<<<<<<< local
=======
extern vec_idx_t g_dtn_sbsp_idx[11];
>>>>>>> other


/*
<<<<<<< local
 * +--------------------------------------------------------------------------+
 * |				     ADM TEMPLATE DOCUMENTATION  						  +
 * +--------------------------------------------------------------------------+
=======
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        ADM TEMPLATE DOCUMENTATION                                        +
 * +-----------------------------------------------------------------------------------------------------------+
>>>>>>> other
 *
<<<<<<< local
 * ADM ROOT STRING    : iso.identified-organization.dod.internet.mgmt.amp.sbsp
 * ADM ROOT ID STRING : 1.3.6.1.2.3.9
 * ADM ROOT OID       : 2B 06 01 02 03 09
 * ADM NICKNAMES      : 0 -> 0x2B0601020309
 *
 *
 *                             SBSP ADM ROOT
 *                             (1.3.6.1.2.3.9)
 *                                   |
 *                                   |
 *   Meta-   Atomic  Computed        |
 *   Data    Data      Data    Rpts  |  Ctrls  Literals  Macros   Ops
 *    (.0)   (.1)      (.2)    (.3)  |  (.4)    (.5)      (.6)    (.7)
 *      +-------+---------+------+------+--------+----------+---------+
 *
=======
 * ADM ROOT STRING:DTN/sbsp
 */
#define ADM_ENUM_DTN_SBSP 10
/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        AGENT NICKNAME DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
>>>>>>> other
 */

<<<<<<< local
=======
/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      DTN_SBSP META-DATA DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |name                 |448018d200    |The human-readable name of the ADM.   |STR    |sbsp                    |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |namespace            |448018d201    |The namespace of the ADM.             |STR    |DTN/sbsp                |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |version              |448018d202    |The version of the ADM.               |STR    |v1.0                    |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |organization         |448018d203    |The name of the issuing organization o|       |                        |
 * |                     |              |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_SBSP_META_NAME 0x00
// "namespace"
#define DTN_SBSP_META_NAMESPACE 0x01
// "version"
#define DTN_SBSP_META_VERSION 0x02
// "organization"
#define DTN_SBSP_META_ORGANIZATION 0x03

>>>>>>> other

/*
<<<<<<< local
 * +--------------------------------------------------------------------------+
 * |					    AGENT NICKNAME DEFINITIONS  					  +
 * +--------------------------------------------------------------------------+
 *
 * 40 -> 0x2B060102030900
 * 41 -> 0x2B060102030901
 * 42 -> 0x2B060102030902
 * 43 -> 0x2B060102030903
 * 44 -> 0x2B060102030904
 * 45 -> 0x2B060102030905
 * 46 -> 0x2B060102030906
 * 47 -> 0x2B060102030907
 * 49 -> 0x2B0601020309
=======
 * +-----------------------------------------------------------------------------------------------------------+
 * |                               DTN_SBSP EXTERNALLY DEFINED DATA DEFINITIONS                               +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bcb_blk  |448218ca00    |Total successfully Tx Bundle Confident|       |
 * |                     |              |iality blocks                         |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_blk   |448218ca01    |Total unsuccessfully Tx Block Confiden|       |
 * |                     |              |tiality Block (BCB) blocks            |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bcb_blk  |448218ca02    |Total successfully Rx BCB blocks      |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_blk   |448218ca03    |Total unsuccessfully Rx BCB blocks    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_bl|448218ca04    |Total missing-on-RX BCB blocks        |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bcb_blks     |448218ca05    |Total forward BCB blocks              |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bcb_bytes|448218ca06    |Total successfully Tx BCB bytes       |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_bytes |448218ca07    |Total unsuccessfully Tx BCB bytes     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_blks  |448218ca08    |Total unsuccessfully Tx BCB blocks    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bcb_bytes|448218ca09    |Total successfully Rx BCB bytes       |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_bytes |448218ca0a    |Total unsuccessfully Rx BCB bytes     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_by|448218ca0b    |Total missing-on-Rx BCB bytes         |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bcb_bytes    |448218ca0c    |Total forwarded BCB bytes             |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bib_blks |448218ca0d    |Total successfully Tx Block Integrity |       |
 * |                     |              |Block (BIB) blocks                    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bib_blks  |448218ca0e    |Total unsuccessfully Tx BIB blocks    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bib_blks |448218ca0f    |Total successfully Rx BIB blocks      |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bib_blks  |448218ca10    |Total unsuccessfully Rx BIB blocks    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_miss_rx_bib_blks |448218ca11    |Total missing-on-Rx BIB blocks        |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bib_blks     |448218ca12    |Total forwarded BIB blocks            |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bib_bytes|448218ca13    |Total successfully Tx BIB bytes       |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bib_bytes |448218ca14    |Total unsuccessfully Tx BIB bytes     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bib_bytes|448218ca15    |Total successfully Rx BIB bytes       |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bib_bytes |448218ca16    |Total unsuccessfully Rx BIB bytes     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_miss_rx_bib_bytes|448218ca17    |Total missing-on-Rx BIB bytes         |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bib_bytes    |458218ca1818  |Total forwarded BIB bytes             |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |last_update          |458218ca1819  |Last sbsp update                      |TV     |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_known_keys       |458218ca181a  |Number of known keys                  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |key_names            |458218ca181b  |Known key names                       |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 * |ciphersuite_names    |458218ca181c  |Known ciphersuite names               |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 * |rule_source          |458218ca181d  |Known rule sources                    |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bcb_blks_|45c218ca181e  |Number of successfully Tx BCB blocks f|       |
 * |                     |              |rom SRC                               |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_blks_s|45c218ca181f  |Number of failed TX BCB blocks from SR|       |
 * |                     |              |C                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bcb_blks_|45c218ca1820  |Number of successfully Rx BCB blocks f|       |
 * |                     |              |rom SRC                               |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_blks_s|45c218ca1821  |Number of failed RX BCB blocks from SR|       |
 * |                     |              |C                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_bl|45c218ca1822  |Number of missing-onRX BCB blocks from|       |
 * |                     |              | SRC                                  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bcb_blks_src |45c218ca1823  |Number of forwarded BCB blocks from SR|       |
 * |                     |              |C                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bcb_bytes|45c218ca1824  |Number of successfully Tx bcb bytes fr|       |
 * |                     |              |om SRC                                |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bcb_bytes_|45c218ca1825  |Number of failed Tx bcb bytes from SRC|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bcb_bytes|45c218ca1826  |Number of successfully Rx bcb bytes fr|       |
 * |                     |              |om SRC                                |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bcb_bytes_|45c218ca1827  |Number of failed Rx bcb bytes from SRC|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_missing_rx_bcb_by|45c218ca1828  |Number of missing-on-Rx bcb bytes from|       |
 * |                     |              | SRC                                  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bcb_bytes_src|45c218ca1829  |Number of forwarded bcb bytes from SRC|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bib_blks_|45c218ca182a  |Number of successfully Tx BIB blocks f|       |
 * |                     |              |rom SRC                               |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bib_blks_s|45c218ca182b  |Number of failed Tx BIB blocks from SR|       |
 * |                     |              |C                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bib_blks_|45c218ca182c  |Number of successfully Rx BIB blocks f|       |
 * |                     |              |rom SRC                               |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bib_blks_s|45c218ca182d  |Number of failed Rx BIB blocks from SR|       |
 * |                     |              |C                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_miss_rx_bib_blks_|45c218ca182e  |Number of missing-on-Rx BIB blocks fro|       |
 * |                     |              |m SRC                                 |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bib_blks_src |45c218ca182f  |Number of forwarded BIB blocks from SR|       |
 * |                     |              |C                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_tx_bib_bytes|45c218ca1830  |Number of successfully Tx BIB bytes fr|       |
 * |                     |              |om SRC                                |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_tx_bib_bytes_|45c218ca1831  |Number of failed Tx BIB bytes from SRC|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_good_rx_bib_bytes|45c218ca1832  |Number of successfully Rx BIB bytes fr|       |
 * |                     |              |om SRC                                |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bad_rx_bib_bytes_|45c218ca1833  |Number of failed Rx BIB bytes from SRC|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_missing_rx_bib_by|45c218ca1834  |Number of missing-on-Rx BIB bytes from|       |
 * |                     |              | SRC                                  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fwd_bib_bytes_src|45c218ca1835  |Number of forwarded BIB bytes from SRC|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |last_update_src      |45c218ca1836  |Last sbsp update from SRC             |TV     |
 * +---------------------+--------------+--------------------------------------+-------+
 * |last_reset           |45c218ca1837  |Last reset                            |TV     |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_SBSP_EDD_NUM_GOOD_TX_BCB_BLK 0x00
#define DTN_SBSP_EDD_NUM_BAD_TX_BCB_BLK 0x01
#define DTN_SBSP_EDD_NUM_GOOD_RX_BCB_BLK 0x02
#define DTN_SBSP_EDD_NUM_BAD_RX_BCB_BLK 0x03
#define DTN_SBSP_EDD_NUM_MISSING_RX_BCB_BLKS 0x04
#define DTN_SBSP_EDD_NUM_FWD_BCB_BLKS 0x05
#define DTN_SBSP_EDD_NUM_GOOD_TX_BCB_BYTES 0x06
#define DTN_SBSP_EDD_NUM_BAD_TX_BCB_BYTES 0x07
#define DTN_SBSP_EDD_NUM_BAD_TX_BCB_BLKS 0x08
#define DTN_SBSP_EDD_NUM_GOOD_RX_BCB_BYTES 0x09
#define DTN_SBSP_EDD_NUM_BAD_RX_BCB_BYTES 0x0a
#define DTN_SBSP_EDD_NUM_MISSING_RX_BCB_BYTES 0x0b
#define DTN_SBSP_EDD_NUM_FWD_BCB_BYTES 0x0c
#define DTN_SBSP_EDD_NUM_GOOD_TX_BIB_BLKS 0x0d
#define DTN_SBSP_EDD_NUM_BAD_TX_BIB_BLKS 0x0e
#define DTN_SBSP_EDD_NUM_GOOD_RX_BIB_BLKS 0x0f
#define DTN_SBSP_EDD_NUM_BAD_RX_BIB_BLKS 0x10
#define DTN_SBSP_EDD_NUM_MISS_RX_BIB_BLKS 0x11
#define DTN_SBSP_EDD_NUM_FWD_BIB_BLKS 0x12
#define DTN_SBSP_EDD_NUM_GOOD_TX_BIB_BYTES 0x13
#define DTN_SBSP_EDD_NUM_BAD_TX_BIB_BYTES 0x14
#define DTN_SBSP_EDD_NUM_GOOD_RX_BIB_BYTES 0x15
#define DTN_SBSP_EDD_NUM_BAD_RX_BIB_BYTES 0x16
#define DTN_SBSP_EDD_NUM_MISS_RX_BIB_BYTES 0x17
#define DTN_SBSP_EDD_NUM_FWD_BIB_BYTES 0x18
#define DTN_SBSP_EDD_LAST_UPDATE 0x19
#define DTN_SBSP_EDD_NUM_KNOWN_KEYS 0x1a
#define DTN_SBSP_EDD_KEY_NAMES 0x1b
#define DTN_SBSP_EDD_CIPHERSUITE_NAMES 0x1c
#define DTN_SBSP_EDD_RULE_SOURCE 0x1d
#define DTN_SBSP_EDD_NUM_GOOD_TX_BCB_BLKS_SRC 0x1e
#define DTN_SBSP_EDD_NUM_BAD_TX_BCB_BLKS_SRC 0x1f
#define DTN_SBSP_EDD_NUM_GOOD_RX_BCB_BLKS_SRC 0x20
#define DTN_SBSP_EDD_NUM_BAD_RX_BCB_BLKS_SRC 0x21
#define DTN_SBSP_EDD_NUM_MISSING_RX_BCB_BLKS_SRC 0x22
#define DTN_SBSP_EDD_NUM_FWD_BCB_BLKS_SRC 0x23
#define DTN_SBSP_EDD_NUM_GOOD_TX_BCB_BYTES_SRC 0x24
#define DTN_SBSP_EDD_NUM_BAD_TX_BCB_BYTES_SRC 0x25
#define DTN_SBSP_EDD_NUM_GOOD_RX_BCB_BYTES_SRC 0x26
#define DTN_SBSP_EDD_NUM_BAD_RX_BCB_BYTES_SRC 0x27
#define DTN_SBSP_EDD_NUM_MISSING_RX_BCB_BYTES_SRC 0x28
#define DTN_SBSP_EDD_NUM_FWD_BCB_BYTES_SRC 0x29
#define DTN_SBSP_EDD_NUM_GOOD_TX_BIB_BLKS_SRC 0x2a
#define DTN_SBSP_EDD_NUM_BAD_TX_BIB_BLKS_SRC 0x2b
#define DTN_SBSP_EDD_NUM_GOOD_RX_BIB_BLKS_SRC 0x2c
#define DTN_SBSP_EDD_NUM_BAD_RX_BIB_BLKS_SRC 0x2d
#define DTN_SBSP_EDD_NUM_MISS_RX_BIB_BLKS_SRC 0x2e
#define DTN_SBSP_EDD_NUM_FWD_BIB_BLKS_SRC 0x2f
#define DTN_SBSP_EDD_NUM_GOOD_TX_BIB_BYTES_SRC 0x30
#define DTN_SBSP_EDD_NUM_BAD_TX_BIB_BYTES_SRC 0x31
#define DTN_SBSP_EDD_NUM_GOOD_RX_BIB_BYTES_SRC 0x32
#define DTN_SBSP_EDD_NUM_BAD_RX_BIB_BYTES_SRC 0x33
#define DTN_SBSP_EDD_NUM_MISSING_RX_BIB_BYTES_SRC 0x34
#define DTN_SBSP_EDD_NUM_FWD_BIB_BYTES_SRC 0x35
#define DTN_SBSP_EDD_LAST_UPDATE_SRC 0x36
#define DTN_SBSP_EDD_LAST_RESET 0x37


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                       DTN_SBSP VARIABLE DEFINITIONS                                       +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |total_bad_tx_blks    |448c18d100    |This is the number of failed TX blocks|       |
 * |                     |              | (# failed BIB + # failed bcb).       |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_SBSP_VAR_TOTAL_BAD_TX_BLKS 0x00


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        DTN_SBSP REPORT DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |full_report          |448718cd00    |all known meta-data, externally define|       |
 * |                     |              |d data, and variables                 |TNVC   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |source_report        |44c718cd01    |security info by source               |TNVC   |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_SBSP_RPTTPL_FULL_REPORT 0x00
#define DTN_SBSP_RPTTPL_SOURCE_REPORT 0x01


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        DTN_SBSP TABLE DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |bib_rules            |448a18cf00    |BIB Rules.                            |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |bcb_rules            |448a18cf01    |BCB Rules.                            |       |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_SBSP_TBLT_BIB_RULES 0x00
#define DTN_SBSP_TBLT_BCB_RULES 0x01


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                       DTN_SBSP CONTROL DEFINITIONS                                       +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |rst_all_cnts         |448118c900    |This control causes the Agent to reset|       |
 * |                     |              | all counts associated with block or b|       |
 * |                     |              |yte statistics and to set the Last Res|       |
 * |                     |              |et Time of the sbsp EDD data to the ti|       |
 * |                     |              |me when the control was run.          |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |rst_src_cnts         |44c118c901    |This control causes the Agent to reset|       |
 * |                     |              | all counts (blocks and bytes) associa|       |
 * |                     |              |ted with a given bundle source and set|       |
 * |                     |              | the Last Reset Time of the source sta|       |
 * |                     |              |tistics to the time when the control w|       |
 * |                     |              |as run.                               |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |delete_key           |44c118c902    |This control deletes a key from the sb|       |
 * |                     |              |sp system.                            |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |add_key              |44c118c903    |This control adds a key to the sbsp sy|       |
 * |                     |              |stem.                                 |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |add_bib_rule         |44c118c904    |This control configures policy on the |       |
 * |                     |              |sbsp protocol implementation that desc|       |
 * |                     |              |ribes how BIB blocks should be applied|       |
 * |                     |              | to bundles in the system. This policy|       |
 * |                     |              | is captured as a rule which states wh|       |
 * |                     |              |en transmitting a bundle from the give|       |
 * |                     |              |n source endpoint ID to the given dest|       |
 * |                     |              |ination endpoint ID, blocks of type ta|       |
 * |                     |              |rget should have a BIB added to them u|       |
 * |                     |              |sing the given ciphersuite and the giv|       |
 * |                     |              |en key.                               |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |del_bib_rule         |44c118c905    |This control removes any configured po|       |
 * |                     |              |licy on the sbsp protocol implementati|       |
 * |                     |              |on that describes how BIB blocks shoul|       |
 * |                     |              |d be applied to bundles in the system.|       |
 * |                     |              | A BIB policy is uniquely identified b|       |
 * |                     |              |y a source endpoint Id, a destination |       |
 * |                     |              |Id, and a target block type.          |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |add_bcb_rule         |44c118c906    |This control configures policy on the |       |
 * |                     |              |sbsp protocol implementation that desc|       |
 * |                     |              |ribes how BCB blocks should be applied|       |
 * |                     |              | to bundles in the system. This policy|       |
 * |                     |              | is captured as a rule which states wh|       |
 * |                     |              |en transmitting a bundle from the give|       |
 * |                     |              |n source endpoint id to the given dest|       |
 * |                     |              |ination endpoint id, blocks of type ta|       |
 * |                     |              |rget should have a bcb added to them u|       |
 * |                     |              |sing the given ciphersuite and the giv|       |
 * |                     |              |en key.                               |       |
 * +---------------------+--------------+--------------------------------------+-------+
 * |del_bcb_rule         |44c118c907    |This control removes any configured po|       |
 * |                     |              |licy on the sbsp protocol implementati|       |
 * |                     |              |on that describes how BCB blocks shoul|       |
 * |                     |              |d be applied to bundles in the system.|       |
 * |                     |              | A bcb policy is uniquely identified b|       |
 * |                     |              |y a source endpoint id, a destination |       |
 * |                     |              |endpoint id, and a target block type. |       |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_SBSP_CTRL_RST_ALL_CNTS 0x00
#define DTN_SBSP_CTRL_RST_SRC_CNTS 0x01
#define DTN_SBSP_CTRL_DELETE_KEY 0x02
#define DTN_SBSP_CTRL_ADD_KEY 0x03
#define DTN_SBSP_CTRL_ADD_BIB_RULE 0x04
#define DTN_SBSP_CTRL_DEL_BIB_RULE 0x05
#define DTN_SBSP_CTRL_ADD_BCB_RULE 0x06
#define DTN_SBSP_CTRL_DEL_BCB_RULE 0x07


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                       DTN_SBSP CONSTANT DEFINITIONS                                       +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
>>>>>>> other
 */


<<<<<<< local
#define SBSP_ADM_MD_NN_IDX 40
#define SBSP_ADM_MD_NN_STR "2B060102030900"

#define SBSP_ADM_AD_NN_IDX 41
#define SBSP_ADM_AD_NN_STR "2B060102030901"

#define SBSP_ADM_CD_NN_IDX 42
#define SBSP_ADM_CD_NN_STR "2B060102030902"

#define SBSP_ADM_RPT_NN_IDX 43
#define SBSP_ADM_RPT_NN_STR "2B060102030903"

#define SBSP_ADM_CTRL_NN_IDX 44
#define SBSP_ADM_CTRL_NN_STR "2B060102030904"

#define SBSP_ADM_LTRL_NN_IDX 45
#define SBSP_ADM_LTRL_NN_STR "2B060102030905"

#define SBSP_ADM_MAC_NN_IDX 46
#define SBSP_ADM_MAC_NN_STR "2B060102030906"

#define SBSP_ADM_OP_NN_IDX 47
#define SBSP_ADM_OP_NN_STR "2B060102030907"

#define SBSP_ADM_ROOT_NN_IDX 49
#define SBSP_ADM_ROOT_NN_STR "2B0601020309"
=======
/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        DTN_SBSP MACRO DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */
>>>>>>> other


/*
<<<<<<< local
 * +--------------------------------------------------------------------------+
 * |					  SBSP META-DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |     Name         | 80280100 |  [40].0  |   ADM Name     |   STR    |
   +------------------+----------+---------+----------------+----------+
   |     Version      | 80280101 |  [40].1  |  ADM Version   |   STR    |
   +------------------+----------+---------+----------------+----------+
=======
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                       DTN_SBSP OPERATOR DEFINITIONS                                       +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
>>>>>>> other
 */

<<<<<<< local
// "SBSP ADM"
#define ADM_SBSP_MD_NAME_MID	"80280100"

// "2016_05_16"
#define ADM_SBSP_MD_VER_MID    "80280101"




/*
 * +--------------------------------------------------------------------------+
 * |					  SBSP ATOMIC DATA DEFINITIONS  					  +
 * +--------------------------------------------------------------------------+

   +-------------------------------------+------------+-----------+-----------+
   |       Name                          |    MID     |    OID    |    Type   |
   +-------------------------------------+------------+-----------+-----------+
   | Total Successfully Tx BCB blocks    |  80290100  |  [41].0   |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Tx BCB blocks  |  80290101  |  [41].1   |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Rx BCB blocks    |  80290102  |  [41].2   |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Rx BCB blocks  |  80290103  |  [41].3   |    UINT   |
   |                                     |            |           |           |
   | Total Missing-on-Rx BCB blocks      |  80290104  |  [41].4   |    UINT   |
   |                                     |            |           |           |
   | Total Forwarded BCB blocks          |  80290105  |  [41].5   |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Tx BCB bytes     |  80290106  |  [41].6   |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Tx BCB bytes   |  80290107  |  [41].7   |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Rx BCB bytes     |  80290108  |  [41].8   |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Rx BCB bytes   |  80290109  |  [41].9   |    UINT   |
   |                                     |            |           |           |
   | Total Missing-on-Rx BCB bytes       |  8029010A  |  [41].A   |    UINT   |
   |                                     |            |           |           |
   | Total Forwarded BCB bytes           |  8029010B  |  [41].B   |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Tx BIB blocks    |  8029010C  |  [41].C   |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Tx BIB blocks  |  8029010D  |  [41].D   |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Rx BIB blocks    |  8029010E  |  [41].E   |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Rx BIB blocks  |  8029010F  |  [41].F   |    UINT   |
   |                                     |            |           |           |
   | Total Missing-on-Rx BIB blocks      |  80290110  |  [41].10  |    UINT   |
   |                                     |            |           |           |
   | Total Forwarded BIB blocks          |  80290111  |  [41].11  |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Tx BIB bytes     |  80290112  |  [41].12  |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Tx BIB bytes   |  80290113  |  [41].13  |    UINT   |
   |                                     |            |           |           |
   | Total Successfully Rx BIB bytes     |  80290114  |  [41].14  |    UINT   |
   |                                     |            |           |           |
   | Total Unsuccessfully Rx BIB bytes   |  80290115  |  [41].15  |    UINT   |
   |                                     |            |           |           |
   | Total Missing-on-Rx BIB bytes       |  80290116  |  [41].16  |    UINT   |
   |                                     |            |           |           |
   | Total Forwarded BIB bytes           |  80290117  |  [41].17  |    UINT   |
   |                                     |            |           |           |
   | Last SBSP Update                   |  80290118  |  [41].18  |    TS     |
   |                                     |            |           |           |
   | Number of Known Keys                |  80290119  |  [41].19  |    UINT   |
   |                                     |            |           |           |
   | Known Key Names                     |  8029011A  |  [41].1A  |    UINT   |
   |                                     |            |           |           |
   | Known Ciphersuite Names             |  8029011B  |  [41].1B  |    BLOB   |
   |                                     |            |           |           |
   | Known Rule Sources                  |  8029011C  |  [41].1C  |    BLOB   |
   |                                     |            |           |           |
   | Successfully Tx BCB blocks from SRC |  C029011D  |  [41].1D  |    UINT   |
   |                                     |            |           |           |
   | Failed Tx BCB blocks from SRC       |  C029011E  |  [41].1E  |    UINT   |
   |                                     |            |           |           |
   | Successfully Rx BCB blocks from SRC |  C029011F  |  [41].1F  |    UINT   |
   |                                     |            |           |           |
   | Failed Rx BCB blocks from SRC       |  C0290120  |  [41].20  |    UINT   |
   |                                     |            |           |           |
   | Missing-on-Rx BCB blocks from SRC   |  C0290121  |  [41].21  |    UINT   |
   |                                     |            |           |           |
   | Forwarded BCB blocks from SRC       |  C0290122  |  [41].22  |    UINT   |
   |                                     |            |           |           |
   | Successfully Tx BCB bytes from SRC  |  C0290123  |  [41].23  |    UINT   |
   |                                     |            |           |           |
   | Failed Tx BCB bytes from SRC        |  C0290124  |  [41].24  |    UINT   |
   |                                     |            |           |           |
   | Successfully Rx BCB bytes from SRC  |  C0290125  |  [41].25  |    UINT   |
   |                                     |            |           |           |
   | Failed Rx BCB bytes from SRC        |  C0290126  |  [41].26  |    UINT   |
   |                                     |            |           |           |
   | Missing-on-Rx BCB bytes from SRC    |  C0290127  |  [41].27  |    UINT   |
   |                                     |            |           |           |
   | Forwarded BCB bytes from SRC        |  C0290128  |  [41].28  |    UINT   |
   |                                     |            |           |           |
   | Successfully Tx BIB blocks from SRC |  C0290129  |  [41].29  |    UINT   |
   |                                     |            |           |           |
   | Failed Tx BIB blocks from SRC       |  C029012A  |  [41].2A  |    UINT   |
   |                                     |            |           |           |
   | Successfully Rx BIB blocks from SRC |  C029012B  |  [41].2B  |    UINT   |
   |                                     |            |           |           |
   | Failed Rx BIB blocks from SRC       |  C029012C  |  [41].2C  |    UINT   |
   |                                     |            |           |           |
   | Missing-on-Rx BIB blocks from SRC   |  C029012D  |  [41].2D  |    UINT   |
   |                                     |            |           |           |
   | Forwarded BIB blocks from SRC       |  C029012E  |  [41].2E  |    UINT   |
   |                                     |            |           |           |
   | Successfully Tx BIB bytes from SRC  |  C029012F  |  [41].2F  |    UINT   |
   |                                     |            |           |           |
   | Failed Tx BIB bytes from SRC        |  C0290130  |  [41].30  |    UINT   |
   |                                     |            |           |           |
   | Successfully Rx BIB bytes from SRC  |  C0290131  |  [41].31  |    UINT   |
   |                                     |            |           |           |
   | Failed Rx BIB bytes from SRC        |  C0290132  |  [41].32  |    UINT   |
   |                                     |            |           |           |
   | Missing-on-Rx BIB bytes from SRC    |  C0290133  |  [41].33  |    UINT   |
   |                                     |            |           |           |
   | Forwarded BIB bytes from SRC        |  C0290134  |  [41].34  |    UINT   |
   |                                     |            |           |           |
   | Last SBSP Update from SRC          |  C0290135  |  [41].35  |    TS     |
   |                                     |            |           |           |
   | Last Reset                          |  C0290136  |  [41].36  |    TS     |
   |                                     |            |           |           |
   +-------------------------------------+------------+-----------+-----------+

 */


//EJB:
//Make an IDX per AD. Pass the IDX as first argument.

#define ADM_SBSP_AD_TOT_GOOD_TX_BCB_BLKS "80290100"
#define ADM_SBSP_AD_TOT_FAIL_TX_BCB_BLKS "80290101"
#define ADM_SBSP_AD_TOT_GOOD_RX_BCB_BLKS "80290102"
#define ADM_SBSP_AD_TOT_FAIL_RX_BCB_BLKS "80290103"
#define ADM_SBSP_AD_TOT_MISSING_BCB_BLKS "80290104"
#define ADM_SBSP_AD_TOT_FORWARD_BCB_BLKS "80290105"

#define ADM_SBSP_AD_TOT_GOOD_TX_BCB_BYTES "80290106"
#define ADM_SBSP_AD_TOT_FAIL_TX_BCB_BYTES "80290107"
#define ADM_SBSP_AD_TOT_GOOD_RX_BCB_BYTES "80290108"
#define ADM_SBSP_AD_TOT_FAIL_RX_BCB_BYTES "80290109"
#define ADM_SBSP_AD_TOT_MISSING_BCB_BYTES "8029010A"
#define ADM_SBSP_AD_TOT_FORWARD_BCB_BYTES "8029010B"

#define ADM_SBSP_AD_TOT_GOOD_TX_BIB_BLKS "8029010C"
#define ADM_SBSP_AD_TOT_FAIL_TX_BIB_BLKS "8029010D"
#define ADM_SBSP_AD_TOT_GOOD_RX_BIB_BLKS "8029010E"
#define ADM_SBSP_AD_TOT_FAIL_RX_BIB_BLKS "8029010F"
#define ADM_SBSP_AD_TOT_MISSING_BIB_BLKS "80290110"
#define ADM_SBSP_AD_TOT_FORWARD_BIB_BLKS "80290111"

#define ADM_SBSP_AD_TOT_GOOD_TX_BIB_BYTES "80290112"
#define ADM_SBSP_AD_TOT_FAIL_TX_BIB_BYTES "80290113"
#define ADM_SBSP_AD_TOT_GOOD_RX_BIB_BYTES "80290114"
#define ADM_SBSP_AD_TOT_FAIL_RX_BIB_BYTES "80290115"
#define ADM_SBSP_AD_TOT_MISSING_BIB_BYTES "80290116"
#define ADM_SBSP_AD_TOT_FORWARD_BIB_BYTES "80290117"

#define ADM_SBSP_AD_LAST_UPDATE "80290118"
#define ADM_SBSP_AD_NUM_KEYS "80290119"
#define ADM_SBSP_AD_KEYS "8029011A"
#define ADM_SBSP_AD_CIPHS "8029011B"
#define ADM_SBSP_AD_SRCS "8029011C"


#define ADM_SBSP_AD_SRC_GOOD_TX_BCB_BLKS "C029011D"
#define ADM_SBSP_AD_SRC_FAIL_TX_BCB_BLKS "C029011E"
#define ADM_SBSP_AD_SRC_GOOD_RX_BCB_BLKS "C029011F"
#define ADM_SBSP_AD_SRC_FAIL_RX_BCB_BLKS "C0290120"
#define ADM_SBSP_AD_SRC_MISSING_BCB_BLKS "C0290121"
#define ADM_SBSP_AD_SRC_FORWARD_BCB_BLKS "C0290122"

#define ADM_SBSP_AD_SRC_GOOD_TX_BCB_BYTES "C0290123"
#define ADM_SBSP_AD_SRC_FAIL_TX_BCB_BYTES "C0290124"
#define ADM_SBSP_AD_SRC_GOOD_RX_BCB_BYTES "C0290125"
#define ADM_SBSP_AD_SRC_FAIL_RX_BCB_BYTES "C0290126"
#define ADM_SBSP_AD_SRC_MISSING_BCB_BYTES "C0290127"
#define ADM_SBSP_AD_SRC_FORWARD_BCB_BYTES "C0290128"

#define ADM_SBSP_AD_SRC_GOOD_TX_BIB_BLKS "C0290129"
#define ADM_SBSP_AD_SRC_FAIL_TX_BIB_BLKS "C029012A"
#define ADM_SBSP_AD_SRC_GOOD_RX_BIB_BLKS "C029012B"
#define ADM_SBSP_AD_SRC_FAIL_RX_BIB_BLKS "C029012C"
#define ADM_SBSP_AD_SRC_MISSING_BIB_BLKS "C029012D"
#define ADM_SBSP_AD_SRC_FORWARD_BIB_BLKS "C029012E"

#define ADM_SBSP_AD_SRC_GOOD_TX_BIB_BYTES "C029012F"
#define ADM_SBSP_AD_SRC_FAIL_TX_BIB_BYTES "C0290130"
#define ADM_SBSP_AD_SRC_GOOD_RX_BIB_BYTES "C0290131"
#define ADM_SBSP_AD_SRC_FAIL_RX_BIB_BYTES "C0290132"
#define ADM_SBSP_AD_SRC_MISSING_BIB_BYTES "C0290133"
#define ADM_SBSP_AD_SRC_FORWARD_BIB_BYTES "C0290134"

#define ADM_SBSP_AD_SRC_LAST_UPDATE "C0290135"

#define ADM_SBSP_AD_LAST_RESET "C0290136"

/*
 *
 * +--------------------------------------------------------------------------+
 * |				    SBSP COMPUTED DATA DEFINITIONS 					  +
 * +--------------------------------------------------------------------------+

   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |                  |          | [42].0  |                |          |
   +------------------+----------+---------+----------------+----------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |				    	SBSP REPORT DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------------+----------+---------+------------------+--------------+
   |    Name    |   MID    |   OID   |   Description    |     Type     |
   +------------+----------+---------+------------------+--------------+
   | FullReport | 822B0100 | [43].0  |  Report of all   |      DC      |
   |            |          |         |   atomic data    |              |
   |            |          |         |      items       |              |
   +------------+----------+---------+------------------+--------------+
   | SRC Report | C22B0101 | [43].1  |  Report of all   |      DC      |
   |            |          |         |   atomic data    |              |
   |            |          |         |   from SRC       |              |
   +------------+----------+---------+------------------+--------------+

 */

#define ADM_SBSP_RPT_FULL_MID  "822B0100"
#define ADM_SBSP_RPT_SRC_MID  "C22B0101"



/*
 * +--------------------------------------------------------------------------+
 * |				    SBSP CONTROL DEFINITIONS CONSTANTS  				  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+----------------------------------+
   |      Name      |    MID    |   OID    |           Description            |
   +----------------+-----------+----------+----------------------------------+
   |    Reset All   |  832C0100 |  [44].0  | ResetAll()                       |
   |                |           |          |                                  |
   |    Reset SRC   |  C32C0101 |  [44].1  | ResetSrc(STR src)                |
   |                |           |          |                                  |
   |  DeleteKey     |  C32C0102 |  [44].2  | DeleteKey(STR keyname)           |
   |                |           |          |                                  |
   |  Add Key       |  C32C0103 |  [44].3  | AddKey(STR keyname, BLOB key)    |
   |                |           |          |                                  |
   | AddBibRule     |  C32C0104 |  [44].4  | AddBibRule(STR src, STR dest,    |
   |                |           |          |            INT tgt, STR cs,      |
   |                |           |          |            STR key)              |
   |                |           |          |                                  |
   | RemoveBibRule  |  C32C0105 |  [44].5  | RemoveBibRule(STR src, STR dest, |
   |                |           |          |              INT tgt)            |
   |                |           |          |                                  |
   | ListBibRules   |  832C0106 |  [44].6  | ListBibRules()                   |
   |                |           |          |                                  |
   | AddBcbRule     |  C32C0107 |  [44].7  | AddBcbRule(STR src, STR dst,     |
   |                |           |          |            INT tgt, STR cs,      |
   |                |           |          |            STR key)              |
   |                |           |          |                                  |
   | RemoveBcbRule  |  C32C0108 |  [44].8  | RemoveBcbRule(STR src, STR dest, |
   |                |           |          |               INT tgt)           |
   |                |           |          |                                  |
   | ListBcbRule    |  832C0109 |  [44].9  | ListBcbRules()                   |
   +----------------+-----------+----------+----------------------------------+
 */

#define ADM_SBSP_CTL_RESET_ALL_MID     "832C0100"
#define ADM_SBSP_CTL_RESET_SRC_MID     "C32C0101"
#define ADM_SBSP_CTL_DEL_KEY_MID       "C32C0102"
#define ADM_SBSP_CTL_ADD_KEY_MID       "C32C0103"
#define ADM_SBSP_CTL_ADD_BIB_RULE_MID  "C32C0104"
#define ADM_SBSP_CTL_DEL_BIB_RULE_MID  "C32C0105"
#define ADM_SBSP_CTL_LIST_BIB_RULE_MID "832C0106"
#define ADM_SBSP_CTL_ADD_BCB_RULE_MID  "C32C0107"
#define ADM_SBSP_CTL_DEL_BCB_RULE_MID  "C32C0108"
#define ADM_SBSP_CTL_LIST_BCB_RULE_MID "832C0109"
#define ADM_SBSP_CTL_UP_BIB_RULE_MID   "C32C010A"
#define ADM_SBSP_CTL_UP_BCB_RULE_MID   "C32C010B"


/*
 * +--------------------------------------------------------------------------+
 * |					  SBSP LITERAL DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------------+
   |      Name      |    MID    |   OID    |           Description           |
   +----------------+-----------+----------+---------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |					    SBSP MACRO DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------+
   |      Name      |    MID    |   OID    |        Description        |
   +----------------+-----------+----------+---------------------------+
*/

/*
 * +--------------------------------------------------------------------------+
 * |				    	SBSP OPERATOR DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------------+-----------+----------+-------------------------------+
   |    Name    |    MID    |   OID    |          Description          |
   +------------+-----------+----------+-------------------------------+
*/



=======
>>>>>>> other
/* Initialization functions. */
<<<<<<< local
void adm_sbsp_init();
void adm_sbsp_init_atomic();
void adm_sbsp_init_computed();
void adm_sbsp_init_controls();
void adm_sbsp_init_literals();
void adm_sbsp_init_macros();
void adm_sbsp_init_metadata();
void adm_sbsp_init_ops();
void adm_sbsp_init_reports();

#endif /* _HAVE_SBSP_ADM_ */
#endif //ADM_SBSP_H_
=======
void dtn_sbsp_init();
void dtn_sbsp_init_meta();
void dtn_sbsp_init_cnst();
void dtn_sbsp_init_edd();
void dtn_sbsp_init_op();
void dtn_sbsp_init_var();
void dtn_sbsp_init_ctrl();
void dtn_sbsp_init_mac();
void dtn_sbsp_init_rpttpl();
void dtn_sbsp_init_tblt();
#endif /* _HAVE_DTN_SBSP_ADM_ */
#endif //ADM_SBSP_H_>>>>>>> other
