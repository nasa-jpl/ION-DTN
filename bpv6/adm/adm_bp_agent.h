/****************************************************************************
 **
 ** File Name: adm_bp_agent.h
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


#ifndef ADM_BP_AGENT_H_
#define ADM_BP_AGENT_H_
#define _HAVE_DTN_BP_AGENT_ADM_
#ifdef _HAVE_DTN_BP_AGENT_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/bp_agent
 */
#define ADM_ENUM_DTN_BP_AGENT 2
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_BP_AGENT META-DATA DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |bp_agent                |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |DTN/bp_agent            |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM                |STR    |v0.1                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_BP_AGENT_META_NAME 0x00
// "namespace"
#define DTN_BP_AGENT_META_NAMESPACE 0x01
// "version"
#define DTN_BP_AGENT_META_VERSION 0x02
// "organization"
#define DTN_BP_AGENT_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                      DTN_BP_AGENT EXTERNALLY DEFINED DATA DEFINITIONS                       +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |bp_node_id           |The node administration endpoint      |STR    |
 * +---------------------+--------------------------------------+-------+
 * |bp_node_version      |The latest version of the BP supported|       |
 * |                     | by this node                         |STR    |
 * +---------------------+--------------------------------------+-------+
 * |available_storage    |Bytes available for bundle storage    |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |last_reset_time      |The last time that BP counters were re|       |
 * |                     |set, either due to execution of a rese|       |
 * |                     |t control or a restart of the node its|       |
 * |                     |elf                                   |UVAST  |
 * +---------------------+--------------------------------------+-------+
 * |num_registrations    |number of registrations               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_pend_fwd         |number of bundles pending forwarding  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_pend_dis         |number of bundles awaiting dispatch   |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_in_cust          |number of bundles                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_pend_reassembly  |number of bundles pending reassembly  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |bundles_by_priority  |number of bundles for the given priori|       |
 * |                     |ty. Priority is given as a priority ma|       |
 * |                     |sk where Bulk=0x1, normal=0x2, express|       |
 * |                     |=0x4. Any bundles matching any of the |       |
 * |                     |masked priorities will be included in |       |
 * |                     |the returned count                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |bytes_by_priority    |number of bytes of the given priority.|       |
 * |                     | Priority is given as a priority mask |       |
 * |                     |where bulk=0x1, normal=0x2, express=0x|       |
 * |                     |4. Any bundles matching any of the mas|       |
 * |                     |ked priorities will be included in the|       |
 * |                     | returned count.                      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |src_bundles_by_priori|number of bundles sourced by this node|       |
 * |                     | of the given priority. Priority is gi|       |
 * |                     |ven as a priority mask where bulk=0x1,|       |
 * |                     | normal=0x2, express=0x4. Any bundles |       |
 * |                     |sourced by this node and matching any |       |
 * |                     |of the masked priorities will be inclu|       |
 * |                     |ded in the returned count.            |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |src_bytes_by_priority|number of bytes sourced by this node o|       |
 * |                     |f the given priority. Priority is give|       |
 * |                     |n as a priority mask where bulk=0x1, n|       |
 * |                     |ormal=0x2, express=0x4. Any bundles so|       |
 * |                     |urced by this node and matching any of|       |
 * |                     | the masked priorities will be include|       |
 * |                     |d in the returned count               |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fragmented_bundle|number of fragmented bundles          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_fragments_produce|number of bundles with fragmentary pay|       |
 * |                     |loads produced by this node           |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_failed_by_reason |number of bundles failed for any of th|       |
 * |                     |e given reasons. (noInfo=0x1, Expired=|       |
 * |                     |0x2, UniFwd=0x4, Cancelled=0x8, NoStor|       |
 * |                     |age=0x10, BadEID=0x20, NoRoute=0x40, N|       |
 * |                     |oContact=0x80, BadBlock=0x100)        |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |num_bundles_deleted  |number of bundles deleted by this node|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |failed_custody_bundle|number of bundle fails at this node   |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |failed_custody_bytes |number bytes of fails at this node    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |failed_forward_bundle|number bundles not forwarded by this n|       |
 * |                     |ode                                   |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |failed_forward_bytes |number of bytes not forwaded by this n|       |
 * |                     |ode                                   |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |abandoned_bundles    |number of bundles abandoned by this no|       |
 * |                     |de                                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |abandoned_bytes      |number of bytes abandoned by this node|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |discarded_bundles    |number of bundles discarded by this no|       |
 * |                     |de                                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |discarded_bytes      |number of bytes discarded by this node|UINT   |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_names       |CSV list of endpoint names for this no|       |
 * |                     |de                                    |STR    |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_active      |is the given endpoint active? (0=no)  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_singleton   |is the given endpoint singleton? (0=no|       |
 * |                     |)                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_policy      |Does the endpoint abandon on fail (0=n|       |
 * |                     |o)                                    |UINT   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BP_AGENT_EDD_BP_NODE_ID 0x00
#define DTN_BP_AGENT_EDD_BP_NODE_VERSION 0x01
#define DTN_BP_AGENT_EDD_AVAILABLE_STORAGE 0x02
#define DTN_BP_AGENT_EDD_LAST_RESET_TIME 0x03
#define DTN_BP_AGENT_EDD_NUM_REGISTRATIONS 0x04
#define DTN_BP_AGENT_EDD_NUM_PEND_FWD 0x05
#define DTN_BP_AGENT_EDD_NUM_PEND_DIS 0x06
#define DTN_BP_AGENT_EDD_NUM_IN_CUST 0x07
#define DTN_BP_AGENT_EDD_NUM_PEND_REASSEMBLY 0x08
#define DTN_BP_AGENT_EDD_BUNDLES_BY_PRIORITY 0x09
#define DTN_BP_AGENT_EDD_BYTES_BY_PRIORITY 0x0a
#define DTN_BP_AGENT_EDD_SRC_BUNDLES_BY_PRIORITY 0x0b
#define DTN_BP_AGENT_EDD_SRC_BYTES_BY_PRIORITY 0x0c
#define DTN_BP_AGENT_EDD_NUM_FRAGMENTED_BUNDLES 0x0d
#define DTN_BP_AGENT_EDD_NUM_FRAGMENTS_PRODUCED 0x0e
#define DTN_BP_AGENT_EDD_NUM_FAILED_BY_REASON 0x0f
#define DTN_BP_AGENT_EDD_NUM_BUNDLES_DELETED 0x10
#define DTN_BP_AGENT_EDD_FAILED_CUSTODY_BUNDLES 0x11
#define DTN_BP_AGENT_EDD_FAILED_CUSTODY_BYTES 0x12
#define DTN_BP_AGENT_EDD_FAILED_FORWARD_BUNDLES 0x13
#define DTN_BP_AGENT_EDD_FAILED_FORWARD_BYTES 0x14
#define DTN_BP_AGENT_EDD_ABANDONED_BUNDLES 0x15
#define DTN_BP_AGENT_EDD_ABANDONED_BYTES 0x16
#define DTN_BP_AGENT_EDD_DISCARDED_BUNDLES 0x17
#define DTN_BP_AGENT_EDD_DISCARDED_BYTES 0x18
#define DTN_BP_AGENT_EDD_ENDPOINT_NAMES 0x19
#define DTN_BP_AGENT_EDD_ENDPOINT_ACTIVE 0x1a
#define DTN_BP_AGENT_EDD_ENDPOINT_SINGLETON 0x1b
#define DTN_BP_AGENT_EDD_ENDPOINT_POLICY 0x1c


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_BP_AGENT VARIABLE DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BP_AGENT REPORT DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |full_report          |This is all known meta-data, EDD, and |       |
 * |                     |VAR values known by the agent.        |TNVC   |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_report      |This is all known endpoint information|TNVC   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BP_AGENT_RPTTPL_FULL_REPORT 0x00
#define DTN_BP_AGENT_RPTTPL_ENDPOINT_REPORT 0x01


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BP_AGENT TABLE DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_BP_AGENT CONTROL DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |reset_all_counts     |This control causes the Agent to reset|       |
 * |                     | all counts associated with bundle or |       |
 * |                     |byte statistics and to set the last re|       |
 * |                     |set time of the BP primitive data to t|       |
 * |                     |he time when the control was run      |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_BP_AGENT_CTRL_RESET_ALL_COUNTS 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_BP_AGENT CONSTANT DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                               DTN_BP_AGENT MACRO DEFINITIONS                                +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_BP_AGENT OPERATOR DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_bp_agent_init();
void dtn_bp_agent_init_meta();
void dtn_bp_agent_init_cnst();
void dtn_bp_agent_init_edd();
void dtn_bp_agent_init_op();
void dtn_bp_agent_init_var();
void dtn_bp_agent_init_ctrl();
void dtn_bp_agent_init_mac();
void dtn_bp_agent_init_rpttpl();
void dtn_bp_agent_init_tblt();
#endif /* _HAVE_DTN_BP_AGENT_ADM_ */
#endif //ADM_BP_AGENT_H_