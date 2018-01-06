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
 **  2018-01-05  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_ADMIN_H_
#define ADM_ION_ADMIN_H_
#define _HAVE_ION_ADMIN_ADM_
#ifdef _HAVE_ION_ADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:ion_admin
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define ION_ADMIN_ADM_META_NN_IDX 30
#define ION_ADMIN_ADM_META_NN_STR "30"

#define ION_ADMIN_ADM_EDD_NN_IDX 31
#define ION_ADMIN_ADM_EDD_NN_STR "31"

#define ION_ADMIN_ADM_VAR_NN_IDX 32
#define ION_ADMIN_ADM_VAR_NN_STR "32"

#define ION_ADMIN_ADM_RPT_NN_IDX 33
#define ION_ADMIN_ADM_RPT_NN_STR "33"

#define ION_ADMIN_ADM_CTRL_NN_IDX 34
#define ION_ADMIN_ADM_CTRL_NN_STR "34"

#define ION_ADMIN_ADM_CONST_NN_IDX 35
#define ION_ADMIN_ADM_CONST_NN_STR "35"

#define ION_ADMIN_ADM_MACRO_NN_IDX 36
#define ION_ADMIN_ADM_MACRO_NN_STR "36"

#define ION_ADMIN_ADM_OP_NN_IDX 37
#define ION_ADMIN_ADM_OP_NN_STR "37"

#define ION_ADMIN_ADM_TBL_NN_IDX 38
#define ION_ADMIN_ADM_TBL_NN_STR "38"

#define ION_ADMIN_ADM_ROOT_NN_IDX 39
#define ION_ADMIN_ADM_ROOT_NN_STR "39"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x871e0100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x871e0101  |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x871e0102  |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x871e0103  |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_ION_ADMIN_META_NAME_MID 0x871e0100
// "namespace"
#define ADM_ION_ADMIN_META_NAMESPACE_MID 0x871e0101
// "version"
#define ADM_ION_ADMIN_META_VERSION_MID 0x871e0102
// "organization"
#define ADM_ION_ADMIN_META_ORGANIZATION_MID 0x871e0103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |clock_error                  |0x801f0100  |This is how accurate the ION Agent's clock is.    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |clock_sync                   |0x801f0101  |This is whether or not the the computer on which t|             |
   |                             |            |he local ION node is running has a synchronized cl|             |
   |                             |            |ock.                                              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |congestion_alarm_control     |0x801f0102  |This is whether or not the node has a control that|             |
   |                             |            | will set off alarm if it will become congested at|             |
   |                             |            | some future time.                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |congestion_end_time_forecasts|0x801f0103  |This is when the node will be predicted to be no l|             |
   |                             |            |onger congested.                                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |consumption_rate             |0x801f0104  |This is the mean rate of continuous data delivery |             |
   |                             |            |to local BP applications.                         |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inbound_file_system_occupancy|0x801f0105  |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's local file system that can be used|             |
   |                             |            | for the storage of inbound zero-copy objects. The|             |
   |                             |            | default heap limit is 1 Terabyte.                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inbound_heap_occupancy_limit |0x801f0106  |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's SDR non-volatile heap that can be |             |
   |                             |            |used for the storage of inbound zero-copy objects.|             |
   |                             |            | The default heap limit is 30% of the SDR data spa|             |
   |                             |            |ce's total heap size.                             |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |number                       |0x801f0107  |This is a CBHE node number which uniquely identifi|             |
   |                             |            |es the node in the delay-tolerant network.        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outbound_file_system_occupanc|0x801f0108  |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's local file system that can be used|             |
   |                             |            | for the storage of outbound zero-copy objects. Th|             |
   |                             |            |e default heap limit is 1 Terabyte.               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outbound_heap_occupancy_limit|0x801f0109  |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's SDR non-volatile heap that can be |             |
   |                             |            |used for the storage of outbound zero-copy objects|             |
   |                             |            |. The default heap limit is 30% of the SDR data sp|             |
   |                             |            |ace's total heap size.                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |production_rate              |0x801f010a  |This is the rate of local data production.        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |ref_time                     |0x801f010b  |This is the reference time that will be used for i|             |
   |                             |            |nterpreting relative time values from now until th|             |
   |                             |            |e next revision of reference time.                |TS           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |utc_delta                    |0x801f010c  |The UTC delta is used to compensate for error (dri|             |
   |                             |            |ft) in clocks, particularly spacecraft clocks. The|             |
   |                             |            | hardware clock on a spacecraft might gain or lose|             |
   |                             |            | a few seconds every month, to the point at which |             |
   |                             |            |its understanding of the current time - as reporte|             |
   |                             |            |d out by the operating system - might differ signi|             |
   |                             |            |ficantly from the actual value of UTC as reported |             |
   |                             |            |by authoritative clocks on Earth. To compensate fo|             |
   |                             |            |r this difference without correcting the clock its|             |
   |                             |            |elf (which can be difficult and dangerous), ION si|             |
   |                             |            |mply adds the UTC delta to the UTC reported by the|             |
   |                             |            | operating system.                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x801f010d  |This is the version of ION that is currently insta|             |
   |                             |            |lled.                                             |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_ADMIN_EDD_CLOCK_ERROR_MID 0x801f0100
#define ADM_ION_ADMIN_EDD_CLOCK_SYNC_MID 0x801f0101
#define ADM_ION_ADMIN_EDD_CONGESTION_ALARM_CONTROL_MID 0x801f0102
#define ADM_ION_ADMIN_EDD_CONGESTION_END_TIME_FORECASTS_MID 0x801f0103
#define ADM_ION_ADMIN_EDD_CONSUMPTION_RATE_MID 0x801f0104
#define ADM_ION_ADMIN_EDD_INBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID 0x801f0105
#define ADM_ION_ADMIN_EDD_INBOUND_HEAP_OCCUPANCY_LIMIT_MID 0x801f0106
#define ADM_ION_ADMIN_EDD_NUMBER_MID 0x801f0107
#define ADM_ION_ADMIN_EDD_OUTBOUND_FILE_SYSTEM_OCCUPANCY_LIMIT_MID 0x801f0108
#define ADM_ION_ADMIN_EDD_OUTBOUND_HEAP_OCCUPANCY_LIMIT_MID 0x801f0109
#define ADM_ION_ADMIN_EDD_PRODUCTION_RATE_MID 0x801f010a
#define ADM_ION_ADMIN_EDD_REF_TIME_MID 0x801f010b
#define ADM_ION_ADMIN_EDD_UTC_DELTA_MID 0x801f010c
#define ADM_ION_ADMIN_EDD_VERSION_MID 0x801f010d


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_init                    |0x83220100  |Until this control is executed, the local ION node|             |
   |                             |            | does not exist and most ionadmin controls will fa|             |
   |                             |            |il. The control configures the local node to be id|             |
   |                             |            |entified by node_number, a CBHE node number which |             |
   |                             |            |uniquely identifies the node in the delay-tolerant|             |
   |                             |            | network.  It also configures ION's data space (SD|             |
   |                             |            |R) and shared working-memory region.  For this pur|             |
   |                             |            |pose it uses a set of default settings if no argum|             |
   |                             |            |ent follows node_number or if the argument followi|             |
   |                             |            |ng node_number is ''; otherwise it uses the config|             |
   |                             |            |uration settings found in a configuration file.  I|             |
   |                             |            |f configuration file name is provided, then the co|             |
   |                             |            |nfiguration file's name is implicitly 'hostname.io|             |
   |                             |            |nconfig'; otherwise, ion_config_filename is taken |             |
   |                             |            |to be the explicit configuration file name.       |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_clock_error_set         |0x83220101  |This management control sets ION's understanding o|             |
   |                             |            |f the accuracy of the scheduled start and stop tim|             |
   |                             |            |es of planned contacts, in seconds.  The default v|             |
   |                             |            |alue is 1.                                        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_clock_sync_set          |0x83220102  |This management control reports whether or not the|             |
   |                             |            | computer on which the local ION node is running h|             |
   |                             |            |as a synchronized clock.                          |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_congestion_alarm_control|0x83220103  |This management control establishes a control whic|             |
   |                             |            |h will automatically be executed whenever ionadmin|             |
   |                             |            | predicts that the node will become congested at s|             |
   |                             |            |ome future time.                                  |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_congestion_end_time_fore|0x83220104  |This management control sets the end time for comp|             |
   |                             |            |uted congestion forecasts. Setting congestion fore|             |
   |                             |            |cast horizon to zero sets the congestion forecast |             |
   |                             |            |end time to infinite time in the future: if there |             |
   |                             |            |is any predicted net growth in bundle storage spac|             |
   |                             |            |e occupancy at all, following the end of the last |             |
   |                             |            |scheduled contact, then eventual congestion will b|             |
   |                             |            |e predicted. The default value is zero, i.e., no e|             |
   |                             |            |nd time.                                          |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_consumption_rate_set    |0x83220105  |This management control sets ION's expectation of |             |
   |                             |            |the mean rate of continuous data delivery to local|             |
   |                             |            | BP applications throughout the period of time ove|             |
   |                             |            |r which congestion forecasts are computed. For nod|             |
   |                             |            |es that function only as routers this variable wil|             |
   |                             |            |l normally be zero. A value of -1, which is the de|             |
   |                             |            |fault, indicates that the rate of local data consu|             |
   |                             |            |mption is unknown; in that case local data consump|             |
   |                             |            |tion is not considered in the computation of conge|             |
   |                             |            |stion forecasts.                                  |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_contact_add             |0x83220106  |This control schedules a period of data transmissi|             |
   |                             |            |on from source_node to dest_node. The period of tr|             |
   |                             |            |ansmission will begin at start_time and end at sto|             |
   |                             |            |p_time, and the rate of data transmission will be |             |
   |                             |            |xmit_data_rate bytes/second. Our confidence in the|             |
   |                             |            | contact defaults to 1.0, indicating that the cont|             |
   |                             |            |act is scheduled - not that non-occurrence of the |             |
   |                             |            |contact is impossible, just that occurrence of the|             |
   |                             |            | contact is planned and scheduled rather than mere|             |
   |                             |            |ly imputed from past node behavior. In the latter |             |
   |                             |            |case, confidence indicates our estimation of the l|             |
   |                             |            |ikelihood of this potential contact.              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_contact_del             |0x83220107  |This control deletes the scheduled period of data |             |
   |                             |            |transmission from source_node to dest_node startin|             |
   |                             |            |g at start_time. To delete all contacts between so|             |
   |                             |            |me pair of nodes, use '*' as start_time.          |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_inbound_heap_occupancy_l|0x83220108  |This management control sets the maximum number of|             |
   |                             |            | megabytes of storage space in ION's SDR non-volat|             |
   |                             |            |ile heap that can be used for the storage of inbou|             |
   |                             |            |nd zero-copy objects. A value of -1 for either lim|             |
   |                             |            |it signifies 'leave unchanged'. The default heap l|             |
   |                             |            |imit is 30% of the SDR data space's total heap siz|             |
   |                             |            |e.                                                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_outbound_heap_occupancy_|0x83220109  |This management control sets the maximum number of|             |
   |                             |            | megabytes of storage space in ION's SDR non-volat|             |
   |                             |            |ile heap that can be used for the storage of outbo|             |
   |                             |            |und zero-copy objects.  A value of -1 for either l|             |
   |                             |            |imit signifies 'leave unchanged'. The default heap|             |
   |                             |            | limit is 30% of the SDR data space's total heap s|             |
   |                             |            |ize.                                              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_production_rate_set     |0x8322010a  |This management control sets ION's expectation of |             |
   |                             |            |the mean rate of continuous data origination by lo|             |
   |                             |            |cal BP applications throughout the period of time |             |
   |                             |            |over which congestion forecasts are computed. For |             |
   |                             |            |nodes that function only as routers this variable |             |
   |                             |            |will normally be zero. A value of -1, which is the|             |
   |                             |            | default, indicates that the rate of local data pr|             |
   |                             |            |oduction is unknown; in that case local data produ|             |
   |                             |            |ction is not considered in the computation of cong|             |
   |                             |            |estion forecasts.                                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_range_add               |0x8322010b  |This control predicts a period of time during whic|             |
   |                             |            |h the distance from node to other_node will be con|             |
   |                             |            |stant to within one light second. The period will |             |
   |                             |            |begin at start_time and end at stop_time, and the |             |
   |                             |            |distance between the nodes during that time will b|             |
   |                             |            |e distance light seconds.                         |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_range_del               |0x8322010c  |This control deletes the predicted period of const|             |
   |                             |            |ant distance between node and other_node starting |             |
   |                             |            |at start_time. To delete all ranges between some p|             |
   |                             |            |air of nodes, use '*' as start_time.              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_ref_time_set            |0x8322010d  |This is used to set the reference time that will b|             |
   |                             |            |e used for interpreting relative time values from |             |
   |                             |            |now until the next revision of reference time. Not|             |
   |                             |            |e that the new reference time can be a relative ti|             |
   |                             |            |me, i.e., an offset beyond the current reference t|             |
   |                             |            |ime.                                              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |node_utc_delta_set           |0x8322010e  |This management control sets ION's understanding o|             |
   |                             |            |f the current difference between correct UTC time |             |
   |                             |            |and the time values reported by the clock for the |             |
   |                             |            |local ION node's computer. This delta is automatic|             |
   |                             |            |ally applied to locally obtained time values whene|             |
   |                             |            |ver ION needs to know the current time.           |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_ADMIN_CTRL_NODE_INIT_MID 0x83220100
#define ADM_ION_ADMIN_CTRL_NODE_CLOCK_ERROR_SET_MID 0x83220101
#define ADM_ION_ADMIN_CTRL_NODE_CLOCK_SYNC_SET_MID 0x83220102
#define ADM_ION_ADMIN_CTRL_NODE_CONGESTION_ALARM_CONTROL_SET_MID 0x83220103
#define ADM_ION_ADMIN_CTRL_NODE_CONGESTION_END_TIME_FORECASTS_SET_MID 0x83220104
#define ADM_ION_ADMIN_CTRL_NODE_CONSUMPTION_RATE_SET_MID 0x83220105
#define ADM_ION_ADMIN_CTRL_NODE_CONTACT_ADD_MID 0x83220106
#define ADM_ION_ADMIN_CTRL_NODE_CONTACT_DEL_MID 0x83220107
#define ADM_ION_ADMIN_CTRL_NODE_INBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID 0x83220108
#define ADM_ION_ADMIN_CTRL_NODE_OUTBOUND_HEAP_OCCUPANCY_LIMIT_SET_MID 0x83220109
#define ADM_ION_ADMIN_CTRL_NODE_PRODUCTION_RATE_SET_MID 0x8322010a
#define ADM_ION_ADMIN_CTRL_NODE_RANGE_ADD_MID 0x8322010b
#define ADM_ION_ADMIN_CTRL_NODE_RANGE_DEL_MID 0x8322010c
#define ADM_ION_ADMIN_CTRL_NODE_REF_TIME_SET_MID 0x8322010d
#define ADM_ION_ADMIN_CTRL_NODE_UTC_DELTA_SET_MID 0x8322010e


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_ADMIN OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_ion_admin_init();
void adm_ion_admin_init_edd();
void adm_ion_admin_init_variables();
void adm_ion_admin_init_controls();
void adm_ion_admin_init_constants();
void adm_ion_admin_init_macros();
void adm_ion_admin_init_metadata();
void adm_ion_admin_init_ops();
void adm_ion_admin_init_reports();
#endif /* _HAVE_ION_ADMIN_ADM_ */
#endif //ADM_ION_ADMIN_H_