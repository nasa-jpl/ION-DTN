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
 **  2018-11-09  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_BP_AGENT_H_
#define ADM_BP_AGENT_H_
#define _HAVE_DTN_BP_AGENT_ADM_
#ifdef _HAVE_DTN_BP_AGENT_ADM_

#include "../utils/nm_types.h"
#include "adm.h"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        ADM TEMPLATE DOCUMENTATION                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/bp_agent
 */
#define ADM_ENUM_DTN_BP_AGENT 2
/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                        AGENT NICKNAME DEFINITIONS                                        +
 * +-----------------------------------------------------------------------------------------------------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                    DTN_BP_AGENT META-DATA DEFINITIONS                                    +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |name                 |4480183200    |The human-readable name of the ADM.   |STR    |bp_agent                |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |namespace            |4480183201    |The namespace of the ADM.             |STR    |DTN/bp_agent            |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |version              |4480183202    |The version of the ADM                |STR    |v0.1                    |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 * |organization         |4480183203    |The name of the issuing organization o|       |                        |
 * |                     |              |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
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
 * +-----------------------------------------------------------------------------------------------------------+
 * |                             DTN_BP_AGENT EXTERNALLY DEFINED DATA DEFINITIONS                             +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |bp_node_id           |4482182a00    |The node administration endpoint      |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 * |bp_node_version      |4482182a01    |The latest version of the BP supported|       |
 * |                     |              | by this node                         |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 * |available_storage    |4482182a02    |Bytes available for bundle storage    |UVAST  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |last_reset_time      |4482182a03    |The last time that BP counters were re|       |
 * |                     |              |set, either due to execution of a rese|       |
 * |                     |              |t control or a restart of the node its|       |
 * |                     |              |elf                                   |UVAST  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_registrations    |4482182a04    |number of registrations               |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_pend_fwd         |4482182a05    |number of bundles pending forwarding  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_pend_dis         |4482182a06    |number of bundles awaiting dispatch   |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_in_cust          |4482182a07    |number of bundles                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_pend_reassembly  |4482182a08    |number of bundles pending reassembly  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |bundles_by_priority  |44c2182a09    |number of bundles for the given priori|       |
 * |                     |              |ty. Priority is given as a priority ma|       |
 * |                     |              |sk where Bulk=0x1, normal=0x2, express|       |
 * |                     |              |=0x4. Any bundles matching any of the |       |
 * |                     |              |masked priorities will be included in |       |
 * |                     |              |the returned count                    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |bytes_by_priority    |44c2182a0a    |number of bytes of the given priority.|       |
 * |                     |              | Priority is given as a priority mask |       |
 * |                     |              |where bulk=0x1, normal=0x2, express=0x|       |
 * |                     |              |4. Any bundles matching any of the mas|       |
 * |                     |              |ked priorities will be included in the|       |
 * |                     |              | returned count.                      |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |src_bundles_by_priori|44c2182a0b    |number of bundles sourced by this node|       |
 * |                     |              | of the given priority. Priority is gi|       |
 * |                     |              |ven as a priority mask where bulk=0x1,|       |
 * |                     |              | normal=0x2, express=0x4. Any bundles |       |
 * |                     |              |sourced by this node and matching any |       |
 * |                     |              |of the masked priorities will be inclu|       |
 * |                     |              |ded in the returned count.            |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |src_bytes_by_priority|44c2182a0c    |number of bytes sourced by this node o|       |
 * |                     |              |f the given priority. Priority is give|       |
 * |                     |              |n as a priority mask where bulk=0x1, n|       |
 * |                     |              |ormal=0x2, express=0x4. Any bundles so|       |
 * |                     |              |urced by this node and matching any of|       |
 * |                     |              | the masked priorities will be include|       |
 * |                     |              |d in the returned count               |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fragmented_bundle|4482182a0d    |number of fragmented bundles          |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_fragments_produce|4482182a0e    |number of bundles with fragmentary pay|       |
 * |                     |              |loads produced by this node           |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_failed_by_reason |44c2182a0f    |number of bundles failed for any of th|       |
 * |                     |              |e given reasons. (noInfo=0x1, Expired=|       |
 * |                     |              |0x2, UniFwd=0x4, Cancelled=0x8, NoStor|       |
 * |                     |              |age=0x10, BadEID=0x20, NoRoute=0x40, N|       |
 * |                     |              |oContact=0x80, BadBlock=0x100)        |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |num_bundles_deleted  |4482182a10    |number of bundles deleted by this node|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |failed_custody_bundle|4482182a11    |number of bundle fails at this node   |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |failed_custody_bytes |4482182a12    |number bytes of fails at this node    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |failed_forward_bundle|4482182a13    |number bundles not forwarded by this n|       |
 * |                     |              |ode                                   |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |failed_forward_bytes |4482182a14    |number of bytes not forwaded by this n|       |
 * |                     |              |ode                                   |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |abandoned_bundles    |4482182a15    |number of bundles abandoned by this no|       |
 * |                     |              |de                                    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |abandoned_bytes      |4482182a16    |number of bytes abandoned by this node|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |discarded_bundles    |4482182a17    |number of bundles discarded by this no|       |
 * |                     |              |de                                    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |discarded_bytes      |4582182a1818  |number of bytes discarded by this node|UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |endpoint_names       |4582182a1819  |CSV list of endpoint names for this no|       |
 * |                     |              |de                                    |STR    |
 * +---------------------+--------------+--------------------------------------+-------+
 * |endpoint_active      |45c2182a181a  |is the given endpoint active? (0=no)  |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |endpoint_singleton   |45c2182a181b  |is the given endpoint singleton? (0=no|       |
 * |                     |              |)                                     |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |endpoint_policy      |45c2182a181c  |Does the endpoint abandon on fail (0=n|       |
 * |                     |              |o)                                    |UINT   |
 * +---------------------+--------------+--------------------------------------+-------+
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
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                     DTN_BP_AGENT VARIABLE DEFINITIONS                                     +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      DTN_BP_AGENT REPORT DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |full_report          |4487182d00    |This is all known meta-data, EDD, and |       |
 * |                     |              |VAR values known by the agent.        |TNVC   |
 * +---------------------+--------------+--------------------------------------+-------+
 * |endpoint_report      |44c7182d01    |This is all known endpoint information|TNVC   |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_BP_AGENT_RPTTPL_FULL_REPORT 0x00
#define DTN_BP_AGENT_RPTTPL_ENDPOINT_REPORT 0x01


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      DTN_BP_AGENT TABLE DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                     DTN_BP_AGENT CONTROL DEFINITIONS                                     +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 * |reset_all_counts     |4481182900    |This control causes the Agent to reset|       |
 * |                     |              | all counts associated with bundle or |       |
 * |                     |              |byte statistics and to set the last re|       |
 * |                     |              |set time of the BP primitive data to t|       |
 * |                     |              |he time when the control was run      |       |
 * +---------------------+--------------+--------------------------------------+-------+
 */
#define DTN_BP_AGENT_CTRL_RESET_ALL_COUNTS 0x00


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                     DTN_BP_AGENT CONSTANT DEFINITIONS                                     +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------+--------------------------------------+-------+------------------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                      DTN_BP_AGENT MACRO DEFINITIONS                                      +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |                                     DTN_BP_AGENT OPERATOR DEFINITIONS                                     +
 * +-----------------------------------------------------------------------------------------------------------+
 * |        NAME         |     ARI      |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_bp_agent_init();
void dtn_bp_agent_init_meta();
void dtn_bp_agent_init_rpttpl();
void dtn_bp_agent_init_edd();
void dtn_bp_agent_init_ctrl();
#endif /* _HAVE_DTN_BP_AGENT_ADM_ */
#endif //ADM_BP_AGENT_H_