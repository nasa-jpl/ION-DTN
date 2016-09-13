/*****************************************************************************
 **
 ** File Name: adm_bpsec.h
 **
 ** Description: This implements the public portions of a AMP BPSEC ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef ADM_BPSEC_H_
#define ADM_BPSEC_H_

#define _HAVE_BPSEC_ADM_

#ifdef _HAVE_BPSEC_ADM_

#include "lyst.h"
#include "bpsec_instr.h"


#include "../utils/nm_types.h"
#include "../adm/adm.h"



/*
 * +--------------------------------------------------------------------------+
 * |				     ADM TEMPLATE DOCUMENTATION  						  +
 * +--------------------------------------------------------------------------+
 *
 * ADM ROOT STRING    : iso.identified-organization.dod.internet.mgmt.amp.bpsec
 * ADM ROOT ID STRING : 1.3.6.1.2.3.9
 * ADM ROOT OID       : 2B 06 01 02 03 09
 * ADM NICKNAMES      : 0 -> 0x2B0601020309
 *
 *
 *                             BPSEC ADM ROOT
 *                             (1.3.6.1.2.3.9)
 *                                   |
 *                                   |
 *   Meta-   Atomic  Computed        |
 *   Data    Data      Data    Rpts  |  Ctrls  Literals  Macros   Ops
 *    (.0)   (.1)      (.2)    (.3)  |  (.4)    (.5)      (.6)    (.7)
 *      +-------+---------+------+------+--------+----------+---------+
 *
 */


/*
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
 */


#define BPSEC_ADM_MD_NN_IDX 40
#define BPSEC_ADM_MD_NN_STR "2B060102030900"

#define BPSEC_ADM_AD_NN_IDX 41
#define BPSEC_ADM_AD_NN_STR "2B060102030901"

#define BPSEC_ADM_CD_NN_IDX 42
#define BPSEC_ADM_CD_NN_STR "2B060102030902"

#define BPSEC_ADM_RPT_NN_IDX 43
#define BPSEC_ADM_RPT_NN_STR "2B060102030903"

#define BPSEC_ADM_CTRL_NN_IDX 44
#define BPSEC_ADM_CTRL_NN_STR "2B060102030904"

#define BPSEC_ADM_LTRL_NN_IDX 45
#define BPSEC_ADM_LTRL_NN_STR "2B060102030905"

#define BPSEC_ADM_MAC_NN_IDX 46
#define BPSEC_ADM_MAC_NN_STR "2B060102030906"

#define BPSEC_ADM_OP_NN_IDX 47
#define BPSEC_ADM_OP_NN_STR "2B060102030907"

#define BPSEC_ADM_ROOT_NN_IDX 49
#define BPSEC_ADM_ROOT_NN_STR "2B0601020309"


/*
 * +--------------------------------------------------------------------------+
 * |					  BPSEC META-DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |     Name         | 80280100 |  [40].0  |   ADM Name     |   STR    |
   +------------------+----------+---------+----------------+----------+
   |     Version      | 80280101 |  [40].1  |  ADM Version   |   STR    |
   +------------------+----------+---------+----------------+----------+
 */

// "BPSEC ADM"
#define ADM_BPSEC_MD_NAME_MID	"80280100"

// "2016_05_16"
#define ADM_BPSEC_MD_VER_MID    "80280101"




/*
 * +--------------------------------------------------------------------------+
 * |					  BPSEC ATOMIC DATA DEFINITIONS  					  +
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
   | Last BPSEC Update                   |  80290118  |  [41].18  |    TS     |
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
   | Last BPSEC Update from SRC          |  C0290135  |  [41].35  |    TS     |
   |                                     |            |           |           |
   | Last Reset                          |  C0290136  |  [41].36  |    TS     |
   |                                     |            |           |           |
   +-------------------------------------+------------+-----------+-----------+

 */


//EJB:
//Make an IDX per AD. Pass the IDX as first argument.

#define ADM_BPSEC_AD_TOT_GOOD_TX_BCB_BLKS "80290100"
#define ADM_BPSEC_AD_TOT_FAIL_TX_BCB_BLKS "80290101"
#define ADM_BPSEC_AD_TOT_GOOD_RX_BCB_BLKS "80290102"
#define ADM_BPSEC_AD_TOT_FAIL_RX_BCB_BLKS "80290103"
#define ADM_BPSEC_AD_TOT_MISSING_BCB_BLKS "80290104"
#define ADM_BPSEC_AD_TOT_FORWARD_BCB_BLKS "80290105"

#define ADM_BPSEC_AD_TOT_GOOD_TX_BCB_BYTES "80290106"
#define ADM_BPSEC_AD_TOT_FAIL_TX_BCB_BYTES "80290107"
#define ADM_BPSEC_AD_TOT_GOOD_RX_BCB_BYTES "80290108"
#define ADM_BPSEC_AD_TOT_FAIL_RX_BCB_BYTES "80290109"
#define ADM_BPSEC_AD_TOT_MISSING_BCB_BYTES "8029010A"
#define ADM_BPSEC_AD_TOT_FORWARD_BCB_BYTES "8029010B"

#define ADM_BPSEC_AD_TOT_GOOD_TX_BIB_BLKS "8029010C"
#define ADM_BPSEC_AD_TOT_FAIL_TX_BIB_BLKS "8029010D"
#define ADM_BPSEC_AD_TOT_GOOD_RX_BIB_BLKS "8029010E"
#define ADM_BPSEC_AD_TOT_FAIL_RX_BIB_BLKS "8029010F"
#define ADM_BPSEC_AD_TOT_MISSING_BIB_BLKS "80290110"
#define ADM_BPSEC_AD_TOT_FORWARD_BIB_BLKS "80290111"

#define ADM_BPSEC_AD_TOT_GOOD_TX_BIB_BYTES "80290112"
#define ADM_BPSEC_AD_TOT_FAIL_TX_BIB_BYTES "80290113"
#define ADM_BPSEC_AD_TOT_GOOD_RX_BIB_BYTES "80290114"
#define ADM_BPSEC_AD_TOT_FAIL_RX_BIB_BYTES "80290115"
#define ADM_BPSEC_AD_TOT_MISSING_BIB_BYTES "80290116"
#define ADM_BPSEC_AD_TOT_FORWARD_BIB_BYTES "80290117"

#define ADM_BPSEC_AD_LAST_UPDATE "80290118"
#define ADM_BPSEC_AD_NUM_KEYS "80290119"
#define ADM_BPSEC_AD_KEYS "8029011A"
#define ADM_BPSEC_AD_CIPHS "8029011B"
#define ADM_BPSEC_AD_SRCS "8029011C"


#define ADM_BPSEC_AD_SRC_GOOD_TX_BCB_BLKS "C029011D"
#define ADM_BPSEC_AD_SRC_FAIL_TX_BCB_BLKS "C029011E"
#define ADM_BPSEC_AD_SRC_GOOD_RX_BCB_BLKS "C029011F"
#define ADM_BPSEC_AD_SRC_FAIL_RX_BCB_BLKS "C0290120"
#define ADM_BPSEC_AD_SRC_MISSING_BCB_BLKS "C0290121"
#define ADM_BPSEC_AD_SRC_FORWARD_BCB_BLKS "C0290122"

#define ADM_BPSEC_AD_SRC_GOOD_TX_BCB_BYTES "C0290123"
#define ADM_BPSEC_AD_SRC_FAIL_TX_BCB_BYTES "C0290124"
#define ADM_BPSEC_AD_SRC_GOOD_RX_BCB_BYTES "C0290125"
#define ADM_BPSEC_AD_SRC_FAIL_RX_BCB_BYTES "C0290126"
#define ADM_BPSEC_AD_SRC_MISSING_BCB_BYTES "C0290127"
#define ADM_BPSEC_AD_SRC_FORWARD_BCB_BYTES "C0290128"

#define ADM_BPSEC_AD_SRC_GOOD_TX_BIB_BLKS "C0290129"
#define ADM_BPSEC_AD_SRC_FAIL_TX_BIB_BLKS "C029012A"
#define ADM_BPSEC_AD_SRC_GOOD_RX_BIB_BLKS "C029012B"
#define ADM_BPSEC_AD_SRC_FAIL_RX_BIB_BLKS "C029012C"
#define ADM_BPSEC_AD_SRC_MISSING_BIB_BLKS "C029012D"
#define ADM_BPSEC_AD_SRC_FORWARD_BIB_BLKS "C029012E"

#define ADM_BPSEC_AD_SRC_GOOD_TX_BIB_BYTES "C029012F"
#define ADM_BPSEC_AD_SRC_FAIL_TX_BIB_BYTES "C0290130"
#define ADM_BPSEC_AD_SRC_GOOD_RX_BIB_BYTES "C0290131"
#define ADM_BPSEC_AD_SRC_FAIL_RX_BIB_BYTES "C0290132"
#define ADM_BPSEC_AD_SRC_MISSING_BIB_BYTES "C0290133"
#define ADM_BPSEC_AD_SRC_FORWARD_BIB_BYTES "C0290134"

#define ADM_BPSEC_AD_SRC_LAST_UPDATE "C0290135"

#define ADM_BPSEC_AD_LAST_RESET "C0290136"

/*
 *
 * +--------------------------------------------------------------------------+
 * |				    BPSEC COMPUTED DATA DEFINITIONS 					  +
 * +--------------------------------------------------------------------------+

   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |                  |          | [42].0  |                |          |
   +------------------+----------+---------+----------------+----------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |				    	BPSEC REPORT DEFINITIONS						  +
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

#define ADM_BPSEC_RPT_FULL_MID  "822B0100"
#define ADM_BPSEC_RPT_SRC_MID  "C22B0101"



/*
 * +--------------------------------------------------------------------------+
 * |				    BPSEC CONTROL DEFINITIONS CONSTANTS  				  +
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

#define ADM_BPSEC_CTL_RESET_ALL_MID     "832C0100"
#define ADM_BPSEC_CTL_RESET_SRC_MID     "C32C0101"
#define ADM_BPSEC_CTL_DEL_KEY_MID       "C32C0102"
#define ADM_BPSEC_CTL_ADD_KEY_MID       "C32C0103"
#define ADM_BPSEC_CTL_ADD_BIB_RULE_MID  "C32C0104"
#define ADM_BPSEC_CTL_DEL_BIB_RULE_MID  "C32C0105"
#define ADM_BPSEC_CTL_LIST_BIB_RULE_MID "832C0106"
#define ADM_BPSEC_CTL_ADD_BCB_RULE_MID  "C32C0107"
#define ADM_BPSEC_CTL_DEL_BCB_RULE_MID  "C32C0108"
#define ADM_BPSEC_CTL_LIST_BCB_RULE_MID "832C0109"
#define ADM_BPSEC_CTL_UP_BIB_RULE_MID   "C32C010A"
#define ADM_BPSEC_CTL_UP_BCB_RULE_MID   "C32C010B"


/*
 * +--------------------------------------------------------------------------+
 * |					  BPSEC LITERAL DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------------+
   |      Name      |    MID    |   OID    |           Description           |
   +----------------+-----------+----------+---------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |					    BPSEC MACRO DEFINTIONS  						  +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------+
   |      Name      |    MID    |   OID    |        Description        |
   +----------------+-----------+----------+---------------------------+
*/

/*
 * +--------------------------------------------------------------------------+
 * |				    	BPSEC OPERATOR DEFINITIONS						  +
 * +--------------------------------------------------------------------------+

   +------------+-----------+----------+-------------------------------+
   |    Name    |    MID    |   OID    |          Description          |
   +------------+-----------+----------+-------------------------------+
*/



/* Initialization functions. */
void adm_bpsec_init();
void adm_bpsec_init_atomic();
void adm_bpsec_init_computed();
void adm_bpsec_init_controls();
void adm_bpsec_init_literals();
void adm_bpsec_init_macros();
void adm_bpsec_init_metadata();
void adm_bpsec_init_ops();
void adm_bpsec_init_reports();

#endif /* _HAVE_BPSEC_ADM_ */
#endif //ADM_BPSEC_H_
