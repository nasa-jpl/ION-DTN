/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: adm_bp.h
 **
 ** Description: This file contains the definitions of the Bundle Protocol
 **              ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **      1. We current use a non-official OID root tree for DTN Bundle Protocol
 **         identifiers.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef ADM_BP_H_
#define ADM_BP_H_

#include "lyst.h"
#include "bpnm.h"

#include "../utils/nm_types.h"


#include "../adm/adm.h"


/*
 * +--------------------------------------------------------------------------+
 * |				     ADM TEMPLATE DOCUMENTATION  						  +
 * +--------------------------------------------------------------------------+
 *
 * ADM ROOT STRING    : iso.identified-organization.dod.internet.mgmt.dtnmp.bp
 * ADM ROOT ID STRING : 1.3.6.1.2.3.1
 * ADM ROOT OID       : 2B 06 01 02 03 01
 * ADM NICKNAMES      : 0 -> 0x2B0601020301
 *
 *
 *                             AGENT ADM ROOT
 *                             (1.3.6.1.2.3.1)
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
 * |					      BP NICKNAME DEFINITIONS    					  +
 * +--------------------------------------------------------------------------+
 *
 * 10 -> 0x2B060102030100
 * 11 -> 0x2B060102030101
 * 12 -> 0x2B060102030102
 * 13 -> 0x2B060102030103
 * 14 -> 0x2B060102030104
 * 15 -> 0x2B060102030105
 * 16 -> 0x2B060102030106
 * 17 -> 0x2B060102030107
 * 18 -> 0x2B0601020301
 */

#define BP_ADM_MD_NN_IDX 10
#define BP_ADM_MD_NN_STR "2B060102030100"

#define BP_ADM_AD_NN_IDX 11
#define BP_ADM_AD_NN_STR "2B060102030101"

#define BP_ADM_CD_NN_IDX 12
#define BP_ADM_CD_NN_STR "2B060102030102"

#define BP_ADM_RPT_NN_IDX 13
#define BP_ADM_RPT_NN_STR "2B060102030103"

#define BP_ADM_CTRL_NN_IDX 14
#define BP_ADM_CTRL_NN_STR "2B060102030104"

#define BP_ADM_LTRL_NN_IDX 15
#define BP_ADM_LTRL_NN_STR "2B060102030105"

#define BP_ADM_MAC_NN_IDX 16
#define BP_ADM_MAC_NN_STR "2B060102030106"

#define BP_ADM_OP_NN_IDX 17
#define BP_ADM_OP_NN_STR "2B060102030107"

#define BP_ADM_ROOT_NN_IDX 18
#define BP_ADM_ROOT_NN_STR "2B0601020301"




/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT META-DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |     Name         | 800A0100 |  [10].0 |   ADM Name     |   STR    |
   +------------------+----------+---------+----------------+----------+
   |     Version      | 800A0101 |  [10].1 |  ADM Version   |   STR    |
   +------------------+----------+---------+----------------+----------+
 */

// "BP ADM"
#define ADM_BP_MD_NAME_MID	"800A0100"

// "2014_12_31"
#define ADM_BP_MD_VER_MID    "800A0101"



/*
 * +--------------------------------------------------------------------------+
 * |					  AGENT ATOMIC DATA DEFINITIONS  					  +
 * +--------------------------------------------------------------------------+

  See the BP MIB for data descriptions.

   +------------------------+----------+---------+----------+----------+
   |       Name             |   MID    |   OID   |  Descr   |   Type   |
   +------------------------+----------+---------+----------+----------+
   |   BP Node ID           | 800B0100 | [11].0  |          |   STR    |
   |                        |          |         |          |          |
   | BP Node Version        | 800B0101 | [11].1  |          |   STR    |
   |                        |          |         |          |          |
   | Avail Storage          | 800B0102 | [11].2  |          |  UVAST   |
   |                        |          |         |          |          |
   | Last Reset Time        | 800B0103 | [11].3  |          |  UVAST   |
   |                        |          |         |          |          |
   | # Registrations        | 800B0104 | [11].4  |          |   UINT   |
   |                        |          |         |          |          |
   | # Pending Fwds         | 800B0105 | [11].5  |          |   UINT   |
   |                        |          |         |          |          |
   | # Dispatch Pending     | 800B0106 | [11].6  |          |   UINT   |
   |                        |          |         |          |          |
   | # Bundles in Custody   | 800B0107 | [11].7  |          |   UINT   |
   |                        |          |         |          |          |
   | # Pending Reassembly   | 800B0108 | [11].8  |          |   UINT   |
   |                        |          |         |          |          |
   | Bulk Priority Bundles  | 800B0109 | [11].9  |          |   UINT   |
   |                        |          |         |          |          |
   |Normal Priority Bundles | 800B010A | [11].A  |          |   UINT   |
   |                        |          |         |          |          |
   |Express Priority Bundles| 800B010B | [11].B  |          |   UINT   |
   |                        |          |         |          |          |
   | Bulk Priority Bytes    | 800B010C | [11].C  |          |   UINT   |
   |                        |          |         |          |          |
   | Normal Pririty Bytes   | 800B010D | [11].D  |          |   UINT   |
   |                        |          |         |          |          |
   | Express Priority Bytes | 800B010E | [11].E  |          |   UINT   |
   |                        |          |         |          |          |
   | # Bulk Src Count       | 800B010F | [11].F  |          |   UINT   |
   |                        |          |         |          |          |
   | # Norm Src Count       | 800B0110 | [11].10 |          |   UINT   |
   |                        |          |         |          |          |
   | # Express Src Count    | 800B0111 | [11].11 |          |   UINT   |
   |                        |          |         |          |          |
   | Bulk Src Bytes         | 800B0112 | [11].12 |          |   UINT   |
   |                        |          |         |          |          |
   | Norm Src Bytes         | 800B0113 | [11].13 |          |   UINT   |
   |                        |          |         |          |          |
   | Express Src Bytes      | 800B0114 | [11].14 |          |   UINT   |
   |                        |          |         |          |          |
   | # Fragmented Bundles   | 800B0115 | [11].15 |          |   UINT   |
   |                        |          |         |          |          |
   | # Fragments Produced   | 800B0116 | [11].16 |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted For No Info    | 800B0117 | [11].17 |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted For Expired    | 800B0118 | [11].18 |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted for UniFwd     | 800B0119 | [11].19 |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted for Cancelled  | 800B011A | [11].1A |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted for No Strg    | 800B011B | [11].1B |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted for Bad EID    | 800B011C | [11].1C |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted No Route       | 800B011D | [11].1D |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted No Contact     | 800B011E | [11].1E |          |   UINT   |
   |                        |          |         |          |          |
   | Deleted Bad Block      | 800B011F | [11].1F |          |   UINT   |
   |                        |          |         |          |          |
   | Bundles Deleted        | 800B0120 | [11].20 |          |   UINT   |
   |                        |          |         |          |          |
   | Fail Cstody Xfer       | 800B0121 | [11].21 |          |   UINT   |
   |                        |          |         |          |          |
   | Fail Cstody Xfer Bytes | 800B0122 | [11].22 |          |   UINT   |
   |                        |          |         |          |          |
   | Fail Forward           | 800B0123 | [11].23 |          |   UINT   |
   |                        |          |         |          |          |
   | Fail Fwd Bytes         | 800B0124 | [11].24 |          |   UINT   |
   |                        |          |         |          |          |
   | Abandoned              | 800B0125 | [11].25 |          |   UINT   |
   |                        |          |         |          |          |
   | Abandoned Bytes        | 800B0126 | [11].26 |          |   UINT   |
   |                        |          |         |          |          |
   | Discard Count          | 800B0127 | [11].27 |          |   UINT   |
   |                        |          |         |          |          |
   | Discard Bytes          | 800B0128 | [11].28 |          |   UINT   |
   |                        |          |         |          |          |
   | Endpoint Names         | 800B0129 | [11].29 |          |   STR    |
   |                        |          |         |          |          |
   +------------------------+----------+---------+----------+----------+
   |	   	   	   	      Endpoint-Specific Information                |
   +------------------------+----------+---------+----------+----------+
   |                        |          |         |          |          |
   | Endpoint Name          | C00B012A | [11].2A |          |   STR    |
   |                        |          |         |          |          |
   | Endpoint Active        | C00B012B | [11].2B |          |   UINT   |
   |                        |          |         |          |          |
   | Endpoint Singleton     | C00B012C | [11].2C |          |   UINT   |
   |                        |          |         |          |          |
   | EP Abandon ON Del Fail | C00B012D | [11].2D |          |   UINT   |
   |                        |          |         |          |          |
   +------------------------+----------+---------+----------+----------+
 */


#define ADM_BP_AD_NODE_ID_MID                    "800B0100"
#define ADM_BP_AD_NODE_VER_MID                   "800B0101"
#define ADM_BP_AD_AVAIL_STOR_MID                 "800B0102"
#define ADM_BP_AD_RESET_TIME_MID                 "800B0103"
#define ADM_BP_AD_NUM_REG_MID                    "800B0104"
#define ADM_BP_AD_BNDL_CUR_FWD_PEND_CNT_MID      "800B0105"
#define ADM_BP_AD_BNDL_CUR_DISPATCH_PEND_CNT_MID "800B0106"
#define ADM_BP_AD_BNDL_CUR_IN_CUSTODY_CNT_MID    "800B0107"
#define ADM_BP_AD_BNDL_CUR_REASSMBL_PEND_CNT_MID "800B0108"
#define ADM_BP_AD_BNDL_CUR_BULK_RES_CNT_MID      "800B0109"
#define ADM_BP_AD_BNDL_CUR_NORM_RES_CNT_MID 	 "800B010A"
#define ADM_BP_AD_BNDL_CUR_EXP_RES_CNT_MID 		 "800B010B"
#define ADM_BP_AD_BNDL_CUR_BULK_RES_BYTES_MID 	 "800B010C"
#define ADM_BP_AD_BNDL_CUR_NORM_BYTES_MID 		 "800B010D"
#define ADM_BP_AD_BNDL_CUR_EXP_BYTES_MID 		 "800B010E"
#define ADM_BP_AD_BNDL_BULK_SRC_CNT_MID 		 "800B010F"
#define ADM_BP_AD_BNDL_NORM_SRC_CNT_MID 		 "800B0110"
#define ADM_BP_AD_BNDL_EXP_SRC_CNT_MID 			 "800B0111"
#define ADM_BP_AD_BNDL_BULK_SRC_BYTES_MID 		 "800B0112"
#define ADM_BP_AD_BNDL_NORM_SRC_BYTES_MID 		 "800B0113"
#define ADM_BP_AD_BNDL_EXP_SRC_BYTES_MID 		 "800B0114"
#define ADM_BP_AD_BNDL_FRAGMENTED_CNT_MID 		 "800B0115"
#define ADM_BP_AD_BNDL_FRAG_PRODUCED_MID 		 "800B0116"
#define ADM_BP_AD_RPT_NOINFO_DEL_CNT_MID 		 "800B0117"
#define ADM_BP_AD_RPT_EXPIRED_DEL_CNT_MID 		 "800B0118"
#define ADM_BP_AD_RPT_UNI_FWD_DEL_CNT_MID 		 "800B0119"
#define ADM_BP_AD_RPT_CANCEL_DEL_CNT_MID 		 "800B011A"
#define ADM_BP_AD_RPT_NO_STRG_DEL_CNT_MID 		 "800B011B"
#define ADM_BP_AD_RPT_BAD_EID_DEL_CNT_MID 		 "800B011C"
#define ADM_BP_AD_RPT_NO_ROUTE_DEL_CNT_MID 		 "800B011D"
#define ADM_BP_AD_RPT_NO_CONTACT_DEL_CNT_MID 	 "800B011E"
#define ADM_BP_AD_RPT_BAD_BLOCK_DEL_CNT_MID 	 "800B011F"
#define ADM_BP_AD_RPT_BUNDLES_DEL_CNT_MID 		 "800B0120"
#define ADM_BP_AD_RPT_FAIL_CUST_XFER_CNT_MID 	 "800B0121"
#define ADM_BP_AD_RPT_FAIL_CUST_XFER_BYTES_MID   "800B0122"
#define ADM_BP_AD_RPT_FAIL_FWD_CNT_MID 			 "800B0123"
#define ADM_BP_AD_RPT_FAIL_FWD_BYTES_MID 		 "800B0124"
#define ADM_BP_AD_RPT_ABANDONED_CNT_MID 		 "800B0125"
#define ADM_BP_AD_RPT_ABANDONED_BYTES_MID 		 "800B0126"
#define ADM_BP_AD_RPT_DISCARD_CNT_MID 			 "800B0127"
#define ADM_BP_AD_RPT_DISCARD_BYTES_MID 		 "800B0128"
#define ADM_BP_AD_ENDPT_NAMES_MID 		         "800B0129"
#define ADM_BP_AD_ENDPT_NAME_MID 				 "C00B012A"
#define ADM_BP_AD_ENDPT_ACTIVE_MID 				 "C00B012B"
#define ADM_BP_AD_ENDPT_SINGLETON_MID 			 "C00B012C"
#define ADM_BP_AD_ENDPT_ABANDON_ON_DEL_FAIL_MID  "C00B012D"


/*
 * +--------------------------------------------------------------------------+
 * |				    BP COMPUTED DATA DEFINITIONS 					  +
 * +--------------------------------------------------------------------------+

   +------------------+----------+---------+----------------+----------+
   |       Name       |   MID    |   OID   |  Description   |   Type   |
   +------------------+----------+---------+----------------+----------+
   |                  |          | [12].X  |                |          |
   +------------------+----------+---------+----------------+----------+
 */



/*
 * +--------------------------------------------------------------------------+
 * |				    	  BP REPORT DEFINITIONS			     			  +
 * +--------------------------------------------------------------------------+

   +------------+----------+---------+------------------+--------------+
   |    Name    |   MID    |   OID   |   Description    |     Type     |
   +------------+----------+---------+------------------+--------------+
   | BundleRpt  | 820D0100 | [13].0  |  Report of all   |      DC      |
   |            |          |         |   Bundle data    |              |
   |            |          |         |                  |              |
   | EndpointRpt| 820D0101 | [13].1  | All Endpoint Info|      DC      |
   +------------+----------+---------+------------------+--------------+
 */

#define ADM_BP_RPT_FULL_MID   "820D0100"
#define ADM_BP_ENDPT_FULL_MID "C20D0101"


/*
 * +--------------------------------------------------------------------------+
 * |				    BP CONTROL DEFINITIONS CONSTANTS  				      +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------+
   |      Name      |    MID    |   OID    |        Description        |
   +----------------+-----------+----------+---------------------------+
   | Reset BP Counts| 830E0100  | [14].0   | Resets all BP counters.   |
   +----------------+-----------+----------+---------------------------+
*/

#define ADM_BP_CTL_RESET_BP_COUNTS "830E0100"


/*
 * +--------------------------------------------------------------------------+
 * |					  BP LITERAL DEFINTIONS  						      +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------------+
   |      Name      |    MID    |   OID    |           Description           |
   +----------------+-----------+----------+---------------------------------+
   |                |           |  [15].X  |                                 |
   +----------------+-----------+----------+---------------------------------+
*/


/*
 * +--------------------------------------------------------------------------+
 * |					  BP MACRO DEFINTIONS  						          +
 * +--------------------------------------------------------------------------+

   +----------------+-----------+----------+---------------------------+
   |      Name      |    MID    |   OID    |        Description        |
   +----------------+-----------+----------+---------------------------+
   |                |           |  [16].0  |                           |
   +----------------+-----------+----------+---------------------------+

 */


/*
 * +--------------------------------------------------------------------------+
 * |				    	BP OPERATOR DEFINITIONS						      +
 * +--------------------------------------------------------------------------+

   +------------+-----------+----------+-------------------------------+
   |    Name    |    MID    |   OID    |          Description          |
   +------------+-----------+----------+-------------------------------+
   |            |           | [17].0   |                               |
   +------------+-----------+----------+-------------------------------+
*/




/*
 * [3] arrays ar eby classes of service.
 * 0 - BULK
 * 1 - NORM
 * 2 - EXP
 */


/*
 * +--------------------------------------------------------------------------+
 * |					        FUNCTION PROTOTYPES  						  +
 * +--------------------------------------------------------------------------+
 */

 void adm_bp_init();
 void adm_bp_init_atomic();
 void adm_bp_init_computed();
 void adm_bp_init_controls();
 void adm_bp_init_literals();
 void adm_bp_init_macros();
 void adm_bp_init_metadata();
 void adm_bp_init_names();
 void adm_bp_init_ops();
 void adm_bp_init_reports();


/* Custom Size Functions. */
uint32_t bp_size_node_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_endpoint_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_id(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_version(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_restart_time(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_num_reg(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_endpoint_name(uint8_t* buffer, uint64_t buffer_len);


#endif //ADM_BP_H_
