/****************************************************************************
 **
 ** File Name: adm_bp.h
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


#ifndef ADM_BP_H_
#define ADM_BP_H_
#define _HAVE_BP_ADM_
#ifdef _HAVE_BP_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"


/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:bp
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define BP_ADM_META_NN_IDX 10
#define BP_ADM_META_NN_STR "10"

#define BP_ADM_EDD_NN_IDX 11
#define BP_ADM_EDD_NN_STR "11"

#define BP_ADM_VAR_NN_IDX 12
#define BP_ADM_VAR_NN_STR "12"

#define BP_ADM_RPT_NN_IDX 13
#define BP_ADM_RPT_NN_STR "13"

#define BP_ADM_CTRL_NN_IDX 14
#define BP_ADM_CTRL_NN_STR "14"

#define BP_ADM_CONST_NN_IDX 15
#define BP_ADM_CONST_NN_STR "15"

#define BP_ADM_MACRO_NN_IDX 16
#define BP_ADM_MACRO_NN_STR "16"

#define BP_ADM_OP_NN_IDX 17
#define BP_ADM_OP_NN_STR "17"

#define BP_ADM_TBL_NN_IDX 18
#define BP_ADM_TBL_NN_STR "18"

#define BP_ADM_ROOT_NN_IDX 19
#define BP_ADM_ROOT_NN_STR "19"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x870a0100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x870a0101  |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x870a0102  |The version of the ADM                            |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x870a0103  |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_BP_META_NAME_MID 0x870a0100
// "namespace"
#define ADM_BP_META_NAMESPACE_MID 0x870a0101
// "version"
#define ADM_BP_META_VERSION_MID 0x870a0102
// "organization"
#define ADM_BP_META_ORGANIZATION_MID 0x870a0103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |bp_node_id                   |0x800b0100  |The node administration endpoint                  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |bp_node_version              |0x800b0101  |The latest version of the BP supported by this nod|             |
   |                             |            |e                                                 |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |available_storage            |0x800b0102  |Bytes available for bundle storage                |UVAST        |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |last_reset_time              |0x800b0103  |The last time that BP counters were reset, either |             |
   |                             |            |due to execution of a reset control or a restart o|             |
   |                             |            |f the node itself                                 |UVAST        |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_registrations            |0x800b0104  |number of registrations                           |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_pend_fwd                 |0x800b0105  |number of bundles pending forwarding              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_pend_dis                 |0x800b0106  |number of bundles awaiting dispatch               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_in_cust                  |0x800b0107  |number of bundles                                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_pend_reassembly          |0x800b0108  |number of bundles pending reassembly              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |bundles_by_priority          |0xc00b0109  |number of bundles for the given priority. Priority|             |
   |                             |            | is given as a priority mask where Bulk=0x1, norma|             |
   |                             |            |l=0x2, express=0x4. Any bundles matching any of th|             |
   |                             |            |e masked priorities will be included in the return|             |
   |                             |            |ed count                                          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |bytes_by_priority            |0xc00b010a  |number of bytes of the given priority. Priority is|             |
   |                             |            | given as a priority mask where bulk=0x1, normal=0|             |
   |                             |            |x2, express=0x4. Any bundles matching any of the m|             |
   |                             |            |asked priorities will be included in the returned |             |
   |                             |            |count.                                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |src_bundles_by_priority      |0xc00b010b  |number of bundles sourced by this node of the give|             |
   |                             |            |n priority. Priority is given as a priority mask w|             |
   |                             |            |here bulk=0x1, normal=0x2, express=0x4. Any bundle|             |
   |                             |            |s sourced by this node and matching any of the mas|             |
   |                             |            |ked priorities will be included in the returned co|             |
   |                             |            |unt.                                              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |src_bytes_by_priority        |0xc00b010c  |number of bytes sourced by this node of the given |             |
   |                             |            |priority. Priority is given as a priority mask whe|             |
   |                             |            |re bulk=0x1, normal=0x2, express=0x4. Any bundles |             |
   |                             |            |sourced by this node and matching any of the maske|             |
   |                             |            |d priorities will be included in the returned coun|             |
   |                             |            |t                                                 |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fragmented_bundles       |0x800b010d  |number of fragmented bundles                      |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_fragments_produced       |0x800b010e  |number of bundles with fragmentary payloads produc|             |
   |                             |            |ed by this node                                   |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_failed_by_reason         |0xc00b010f  |number of bundles failed for any of the given reas|             |
   |                             |            |ons. (noInfo=0x1, Expired=0x2, UniFwd=0x4, Cancell|             |
   |                             |            |ed=0x8, NoStorage=0x10, BadEID=0x20, NoRoute=0x40,|             |
   |                             |            | NoContact=0x80, BadBlock=0x100)                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |num_bundles_deleted          |0x800b0110  |number of bundles deleted by this node            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |failed_custody_bundles       |0x800b0111  |number of bundle fails at this node               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |failed_custody_bytes         |0x800b0112  |number bytes of fails at this node                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |failed_forward_bundles       |0x800b0113  |number bundles not forwarded by this node         |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |failed_forward_bytes         |0x800b0114  |number of bytes not forwaded by this node         |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |abandoned_bundles            |0x800b0115  |number of bundles abandoned by this node          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |abandoned_bytes              |0x800b0116  |number of bytes abandoned by this node            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |discarded_bundles            |0x800b0117  |number of bundles discarded by this node          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |discarded_bytes              |0x800b0118  |number of bytes discarded by this node            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpoint_names               |0x800b0119  |CSV list of endpoint names for this node          |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpoint_active              |0xc00b011a  |is the given endpoint active? (0=no)              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpoint_singleton           |0xc00b011b  |is the given endpoint singleton? (0=no)           |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpoint_policy              |0xc00b011c  |Does the endpoint abandon on fail (0=no)          |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BP_EDD_BP_NODE_ID_MID 0x800b0100
#define ADM_BP_EDD_BP_NODE_VERSION_MID 0x800b0101
#define ADM_BP_EDD_AVAILABLE_STORAGE_MID 0x800b0102
#define ADM_BP_EDD_LAST_RESET_TIME_MID 0x800b0103
#define ADM_BP_EDD_NUM_REGISTRATIONS_MID 0x800b0104
#define ADM_BP_EDD_NUM_PEND_FWD_MID 0x800b0105
#define ADM_BP_EDD_NUM_PEND_DIS_MID 0x800b0106
#define ADM_BP_EDD_NUM_IN_CUST_MID 0x800b0107
#define ADM_BP_EDD_NUM_PEND_REASSEMBLY_MID 0x800b0108
#define ADM_BP_EDD_BUNDLES_BY_PRIORITY_MID 0xc00b0109
#define ADM_BP_EDD_BYTES_BY_PRIORITY_MID 0xc00b010a
#define ADM_BP_EDD_SRC_BUNDLES_BY_PRIORITY_MID 0xc00b010b
#define ADM_BP_EDD_SRC_BYTES_BY_PRIORITY_MID 0xc00b010c
#define ADM_BP_EDD_NUM_FRAGMENTED_BUNDLES_MID 0x800b010d
#define ADM_BP_EDD_NUM_FRAGMENTS_PRODUCED_MID 0x800b010e
#define ADM_BP_EDD_NUM_FAILED_BY_REASON_MID 0xc00b010f
#define ADM_BP_EDD_NUM_BUNDLES_DELETED_MID 0x800b0110
#define ADM_BP_EDD_FAILED_CUSTODY_BUNDLES_MID 0x800b0111
#define ADM_BP_EDD_FAILED_CUSTODY_BYTES_MID 0x800b0112
#define ADM_BP_EDD_FAILED_FORWARD_BUNDLES_MID 0x800b0113
#define ADM_BP_EDD_FAILED_FORWARD_BYTES_MID 0x800b0114
#define ADM_BP_EDD_ABANDONED_BUNDLES_MID 0x800b0115
#define ADM_BP_EDD_ABANDONED_BYTES_MID 0x800b0116
#define ADM_BP_EDD_DISCARDED_BUNDLES_MID 0x800b0117
#define ADM_BP_EDD_DISCARDED_BYTES_MID 0x800b0118
#define ADM_BP_EDD_ENDPOINT_NAMES_MID 0x800b0119
#define ADM_BP_EDD_ENDPOINT_ACTIVE_MID 0xc00b011a
#define ADM_BP_EDD_ENDPOINT_SINGLETON_MID 0xc00b011b
#define ADM_BP_EDD_ENDPOINT_POLICY_MID 0xc00b011c


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |full_report                  |0x820d0100  |This is all known meta-data, EDD, and VAR values k|             |
   |                             |            |nown by the agent.                                |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpoint_report              |0xc20d0101  |This is all known endpoint information            |?            |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BP_RPT_FULL_REPORT_MID 0x820d0100
#define ADM_BP_RPT_ENDPOINT_REPORT_MID 0xc20d0101


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |reset_all_counts             |0x830e0100  |This control causes the Agent to reset all counts |             |
   |                             |            |associated with bundle or byte statistics and to s|             |
   |                             |            |et the last reset time of the BP primitive data to|             |
   |                             |            | the time when the control was run                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_BP_CTRL_RESET_ALL_COUNTS_MID 0x830e0100


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    BP OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_bp_init();
void adm_bp_init_edd();
void adm_bp_init_variables();
void adm_bp_init_controls();
void adm_bp_init_constants();
void adm_bp_init_macros();
void adm_bp_init_metadata();
void adm_bp_init_ops();
void adm_bp_init_reports();
#endif /* _HAVE_BP_ADM_ */
#endif //ADM_BP_H_