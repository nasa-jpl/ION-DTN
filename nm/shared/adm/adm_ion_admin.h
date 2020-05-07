/****************************************************************************
 **
 ** File Name: adm_ion_admin.h
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


#ifndef ADM_ION_ADMIN_H_
#define ADM_ION_ADMIN_H_
#define _HAVE_DTN_ION_IONADMIN_ADM_
#ifdef _HAVE_DTN_ION_IONADMIN_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ION/ionadmin
 */
#define ADM_ENUM_DTN_ION_IONADMIN 7
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                           DTN_ION_IONADMIN META-DATA DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |ion_admin               |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM.             |STR    |DTN/ION/ionadmin        |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM.               |STR    |v0.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM.                            |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_ION_IONADMIN_META_NAME 0x00
// "namespace"
#define DTN_ION_IONADMIN_META_NAMESPACE 0x01
// "version"
#define DTN_ION_IONADMIN_META_VERSION 0x02
// "organization"
#define DTN_ION_IONADMIN_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                    DTN_ION_IONADMIN EXTERNALLY DEFINED DATA DEFINITIONS                     +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |clock_error          |This is how accurate the ION Agent's c|       |
 * |                     |lock is described as number of seconds|       |
 * |                     |, an absolute value.                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |clock_sync           |This is whether or not the the compute|       |
 * |                     |r on which the local ION node is runni|       |
 * |                     |ng has a synchronized clock.          |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |congestion_alarm_cont|This is whether or not the node has a |       |
 * |                     |control that will set off alarm if it |       |
 * |                     |will become congested at some future t|       |
 * |                     |ime.                                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |congestion_end_time_f|This is the time horizon beyond which |       |
 * |                     |we don't attempt to forecast congestio|       |
 * |                     |n                                     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |consumption_rate     |This is the mean rate of continuous da|       |
 * |                     |ta delivery to local BP applications. |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |inbound_file_system_o|This is the maximum number of megabyte|       |
 * |                     |s of storage space in ION's local file|       |
 * |                     | system that can be used for the stora|       |
 * |                     |ge of inbound zero-copy objects. The d|       |
 * |                     |efault heap limit is 1 Terabyte.      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |inbound_heap_occupanc|This is the maximum number of megabyte|       |
 * |                     |s of storage space in ION's SDR non-vo|       |
 * |                     |latile heap that can be used for the s|       |
 * |                     |torage of inbound zero-copy objects. T|       |
 * |                     |he default heap limit is 20% of the SD|       |
 * |                     |R data space's total heap size.       |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |number               |This is a CBHE node number which uniqu|       |
 * |                     |ely identifies the node in the delay-t|       |
 * |                     |olerant network.                      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |outbound_file_system_|This is the maximum number of megabyte|       |
 * |                     |s of storage space in ION's local file|       |
 * |                     | system that can be used for the stora|       |
 * |                     |ge of outbound zero-copy objects. The |       |
 * |                     |default heap limit is 1 Terabyte.     |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |outbound_heap_occupan|This is the maximum number of megabyte|       |
 * |                     |s of storage space in ION's SDR non-vo|       |
 * |                     |latile heap that can be used for the s|       |
 * |                     |torage of outbound zero-copy objects. |       |
 * |                     |The default heap limit is 20% of the S|       |
 * |                     |DR data space's total heap size.      |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |production_rate      |This is the rate of local data product|       |
 * |                     |ion.                                  |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |ref_time             |This is the reference time that will b|       |
 * |                     |e used for interpreting relative time |       |
 * |                     |values from now until the next revisio|       |
 * |                     |n of reference time.                  |TV     |
 * +---------------------+--------------------------------------+-------+
 * |time_delta           |The time delta is used to compensate f|       |
 * |                     |or error (drift) in clocks, particular|       |
 * |                     |ly spacecraft clocks. The hardware clo|       |
 * |                     |ck on a spacecraft might gain or lose |       |
 * |                     |a few seconds every month, to the poin|       |
 * |                     |t at which its understanding of the cu|       |
 * |                     |rrent time - as reported out by the op|       |
 * |                     |erating system - might differ signific|       |
 * |                     |antly from the actual value of Unix Ep|       |
 * |                     |och time as reported by authoritative |       |
 * |                     |clocks on Earth. To compensate for thi|       |
 * |                     |s difference without correcting the cl|       |
 * |                     |ock itself (which can be difficult and|       |
 * |                     | dangerous), ION simply adds the time |       |
 * |                     |delta to the Epoch time reported by th|       |
 * |                     |e operating system.                   |UINT   |
 * +---------------------+--------------------------------------+-------+
 * |version              |This is the version of ION that is cur|       |
 * |                     |rently installed.                     |STR    |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IONADMIN_EDD_CLOCK_ERROR 0x00
#define DTN_ION_IONADMIN_EDD_CLOCK_SYNC 0x01
#define DTN_ION_IONADMIN_EDD_CONGESTION_ALARM_CONTROL 0x02
#define DTN_ION_IONADMIN_EDD_CONGESTION_END_TIME_FORECASTS 0x03
#define DTN_ION_IONADMIN_EDD_CONSUMPTION_RATE 0x04
#define DTN_ION_IONADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT 0x05
#define DTN_ION_IONADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT 0x06
#define DTN_ION_IONADMIN_EDD_NUMBER 0x07
#define DTN_ION_IONADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT 0x08
#define DTN_ION_IONADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT 0x09
#define DTN_ION_IONADMIN_EDD_PRODUCTION_RATE 0x0a
#define DTN_ION_IONADMIN_EDD_REF_TIME 0x0b
#define DTN_ION_IONADMIN_EDD_TIME_DELTA 0x0c
#define DTN_ION_IONADMIN_EDD_VERSION 0x0d


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IONADMIN VARIABLE DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_IONADMIN REPORT DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_IONADMIN TABLE DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |contacts             |This table shows all scheduled periods|       |
 * |                     | of data transmission.                |       |
 * +---------------------+--------------------------------------+-------+
 * |ranges               |This table shows all predicted periods|       |
 * |                     | of constant distance between nodes.  |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IONADMIN_TBLT_CONTACTS 0x00
#define DTN_ION_IONADMIN_TBLT_RANGES 0x01


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IONADMIN CONTROL DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |node_init            |Until this control is executed, the lo|       |
 * |                     |cal ION node does not exist and most i|       |
 * |                     |onadmin controls will fail. The contro|       |
 * |                     |l configures the local node to be iden|       |
 * |                     |tified by node_number, a CBHE node num|       |
 * |                     |ber which uniquely identifies the node|       |
 * |                     | in the delay-tolerant network.  It al|       |
 * |                     |so configures ION's data space (SDR) a|       |
 * |                     |nd shared working-memory region.  For |       |
 * |                     |this purpose it uses a set of default |       |
 * |                     |settings if no argument follows node_n|       |
 * |                     |umber or if the argument following nod|       |
 * |                     |e_number is ''; otherwise it uses the |       |
 * |                     |configuration settings found in a conf|       |
 * |                     |iguration file.  If configuration file|       |
 * |                     | name is provided, then the configurat|       |
 * |                     |ion file's name is implicitly 'hostnam|       |
 * |                     |e.ionconfig'; otherwise, ion_config_fi|       |
 * |                     |lename is taken to be the explicit con|       |
 * |                     |figuration file name.                 |       |
 * +---------------------+--------------------------------------+-------+
 * |node_clock_error_set |This management control sets ION's und|       |
 * |                     |erstanding of the accuracy of the sche|       |
 * |                     |duled start and stop times of planned |       |
 * |                     |contacts, in seconds.  The default val|       |
 * |                     |ue is 1.                              |       |
 * +---------------------+--------------------------------------+-------+
 * |node_clock_sync_set  |This management control reports whethe|       |
 * |                     |r or not the computer on which the loc|       |
 * |                     |al ION node is running has a synchroni|       |
 * |                     |zed clock.                            |       |
 * +---------------------+--------------------------------------+-------+
 * |node_congestion_alarm|This management control establishes a |       |
 * |                     |control which will automatically be ex|       |
 * |                     |ecuted whenever ionadmin predicts that|       |
 * |                     | the node will become congested at som|       |
 * |                     |e future time.                        |       |
 * +---------------------+--------------------------------------+-------+
 * |node_congestion_end_t|This management control sets the end t|       |
 * |                     |ime for computed congestion forecasts.|       |
 * |                     | Setting congestion forecast horizon t|       |
 * |                     |o zero sets the congestion forecast en|       |
 * |                     |d time to infinite time in the future:|       |
 * |                     | if there is any predicted net growth |       |
 * |                     |in bundle storage space occupancy at a|       |
 * |                     |ll, following the end of the last sche|       |
 * |                     |duled contact, then eventual congestio|       |
 * |                     |n will be predicted. The default value|       |
 * |                     | is zero, i.e., no end time.          |       |
 * +---------------------+--------------------------------------+-------+
 * |node_consumption_rate|This management control sets ION's exp|       |
 * |                     |ectation of the mean rate of continuou|       |
 * |                     |s data delivery to local BP applicatio|       |
 * |                     |ns throughout the period of time over |       |
 * |                     |which congestion forecasts are compute|       |
 * |                     |d. For nodes that function only as rou|       |
 * |                     |ters this variable will normally be ze|       |
 * |                     |ro. A value of -1, which is the defaul|       |
 * |                     |t, indicates that the rate of local da|       |
 * |                     |ta consumption is unknown; in that cas|       |
 * |                     |e local data consumption is not consid|       |
 * |                     |ered in the computation of congestion |       |
 * |                     |forecasts.                            |       |
 * +---------------------+--------------------------------------+-------+
 * |node_contact_add     |This control schedules a period of dat|       |
 * |                     |a transmission from source_node to des|       |
 * |                     |t_node. The period of transmission wil|       |
 * |                     |l begin at start_time and end at stop_|       |
 * |                     |time, and the rate of data transmissio|       |
 * |                     |n will be xmit_data_rate bytes/second.|       |
 * |                     | Our confidence in the contact default|       |
 * |                     |s to 1.0, indicating that the contact |       |
 * |                     |is scheduled - not that non-occurrence|       |
 * |                     | of the contact is impossible, just th|       |
 * |                     |at occurrence of the contact is planne|       |
 * |                     |d and scheduled rather than merely imp|       |
 * |                     |uted from past node behavior. In the l|       |
 * |                     |atter case, confidence indicates our e|       |
 * |                     |stimation of the likelihood of this po|       |
 * |                     |tential contact.                      |       |
 * +---------------------+--------------------------------------+-------+
 * |node_contact_del     |This control deletes the scheduled per|       |
 * |                     |iod of data transmission from source_n|       |
 * |                     |ode to dest_node starting at start_tim|       |
 * |                     |e. To delete all contacts between some|       |
 * |                     | pair of nodes, use '*' as start_time.|       |
 * +---------------------+--------------------------------------+-------+
 * |node_inbound_heap_occ|This management control sets the maxim|       |
 * |                     |um number of megabytes of storage spac|       |
 * |                     |e in ION's SDR non-volatile heap that |       |
 * |                     |can be used for the storage of inbound|       |
 * |                     | zero-copy objects. A value of -1 for |       |
 * |                     |either limit signifies 'leave unchange|       |
 * |                     |d'. The default heap limit is 30% of t|       |
 * |                     |he SDR data space's total heap size.  |       |
 * +---------------------+--------------------------------------+-------+
 * |node_outbound_heap_oc|This management control sets the maxim|       |
 * |                     |um number of megabytes of storage spac|       |
 * |                     |e in ION's SDR non-volatile heap that |       |
 * |                     |can be used for the storage of outboun|       |
 * |                     |d zero-copy objects.  A value of  -1 f|       |
 * |                     |or either limit signifies 'leave uncha|       |
 * |                     |nged'. The default heap  limit is 30% |       |
 * |                     |of the SDR data space's total heap siz|       |
 * |                     |e.                                    |       |
 * +---------------------+--------------------------------------+-------+
 * |node_production_rate_|This management control sets ION's exp|       |
 * |                     |ectation of the mean rate of continuou|       |
 * |                     |s data origination by local BP applica|       |
 * |                     |tions throughout the period of time ov|       |
 * |                     |er which congestion forecasts are comp|       |
 * |                     |uted. For nodes that function only as |       |
 * |                     |routers this variable will normally be|       |
 * |                     | zero. A value of -1, which is the def|       |
 * |                     |ault, indicates that the rate of local|       |
 * |                     | data production is unknown; in that c|       |
 * |                     |ase local data production is not consi|       |
 * |                     |dered in the computation of congestion|       |
 * |                     | forecasts.                           |       |
 * +---------------------+--------------------------------------+-------+
 * |node_range_add       |This control predicts a period of time|       |
 * |                     | during which the distance from node t|       |
 * |                     |o other_node will be constant to withi|       |
 * |                     |n one light second. The period will be|       |
 * |                     |gin at start_time and end at stop_time|       |
 * |                     |, and the distance between the nodes d|       |
 * |                     |uring that time will be distance light|       |
 * |                     | seconds.                             |       |
 * +---------------------+--------------------------------------+-------+
 * |node_range_del       |This control deletes the predicted per|       |
 * |                     |iod of constant distance between node |       |
 * |                     |and other_node starting at start_time.|       |
 * |                     | To delete all ranges between some pai|       |
 * |                     |r of nodes, use '*' as start_time.    |       |
 * +---------------------+--------------------------------------+-------+
 * |node_ref_time_set    |This is used to set the reference time|       |
 * |                     | that will be used for interpreting re|       |
 * |                     |lative time values from now until the |       |
 * |                     |next revision of reference time. Note |       |
 * |                     |that the new reference time can be a r|       |
 * |                     |elative time, i.e., an offset beyond t|       |
 * |                     |he current reference time.            |       |
 * +---------------------+--------------------------------------+-------+
 * |node_time_delta_set  |This management control sets ION's und|       |
 * |                     |erstanding of the current difference b|       |
 * |                     |etween correct time and the Unix Epoch|       |
 * |                     | time values reported by the clock for|       |
 * |                     | the local ION node's computer. This d|       |
 * |                     |elta is automatically applied to local|       |
 * |                     |ly obtained time values whenever ION n|       |
 * |                     |eeds to know the current time.        |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_IONADMIN_CTRL_NODE_INIT 0x00
#define DTN_ION_IONADMIN_CTRL_NODE_CLOCK_ERROR_SET 0x01
#define DTN_ION_IONADMIN_CTRL_NODE_CLOCK_SYNC_SET 0x02
#define DTN_ION_IONADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET 0x03
#define DTN_ION_IONADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET 0x04
#define DTN_ION_IONADMIN_CTRL_NODE_CONSUMPTION_RATE_SET 0x05
#define DTN_ION_IONADMIN_CTRL_NODE_CONTACT_ADD 0x06
#define DTN_ION_IONADMIN_CTRL_NODE_CONTACT_DEL 0x07
#define DTN_ION_IONADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET 0x08
#define DTN_ION_IONADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET 0x09
#define DTN_ION_IONADMIN_CTRL_NODE_PRODUCTION_RATE_SET 0x0a
#define DTN_ION_IONADMIN_CTRL_NODE_RANGE_ADD 0x0b
#define DTN_ION_IONADMIN_CTRL_NODE_RANGE_DEL 0x0c
#define DTN_ION_IONADMIN_CTRL_NODE_REF_TIME_SET 0x0d
#define DTN_ION_IONADMIN_CTRL_NODE_TIME_DELTA_SET 0x0e


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IONADMIN CONSTANT DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_IONADMIN MACRO DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_IONADMIN OPERATOR DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_ion_ionadmin_init();
void dtn_ion_ionadmin_init_meta();
void dtn_ion_ionadmin_init_cnst();
void dtn_ion_ionadmin_init_edd();
void dtn_ion_ionadmin_init_op();
void dtn_ion_ionadmin_init_var();
void dtn_ion_ionadmin_init_ctrl();
void dtn_ion_ionadmin_init_mac();
void dtn_ion_ionadmin_init_rpttpl();
void dtn_ion_ionadmin_init_tblt();
#endif /* _HAVE_DTN_ION_IONADMIN_ADM_ */
#endif //ADM_ION_ADMIN_H_