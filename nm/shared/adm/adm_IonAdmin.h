/******************************************************************************
 **
 ** File Name: ./shared/adm_IonAdmin.h
 **
 ** Description: 
 **
 ** Notes: 
 **
 ** Assumptions: 
 **
 ** Modification History: 
 **  MM/DD/YY  AUTHOR           DESCRIPTION
 **  --------  --------------   ------------------------------------------------
 **
 **  2017-11-11  AUTO           Auto generated header file 
 **
*********************************************************************************/
#ifndef ADM_IONADMIN_H_
#define ADM_IONADMIN_H_
#define _HAVE_IONADMIN_ADM_
#ifdef _HAVE_IONADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                                          +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:IonAdmin
 * ADM NICKNAMES:
 *
 *
 *					IONADMIN ADM ROOT
 *								    |
 *								    |
 * Meta-						    |
 * Data	EDDs	VARs	Rpts	Ctrls	Constants	Macros	Ops   Tbls
 *  _0	 _1	  _2	  _3	   _4 	  _5       _6     _7    _8
 *  +------+-----+-----+------+-------+--------+-------+-------+
 *
 */
/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                                        +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * META-> 30
 * EDD -> 31
 * VAR -> 32
 * RPT -> 33
 * CTRL -> 34
 * CONST -> 35
 * MACRO -> 36
 * OP -> 37
 * TBL -> 38
 * ROOT -> 39

 */
#define IONADMIN_ADM_META_NN_IDX 30
#define IONADMIN_ADM_META_NN_STR "30"

#define IONADMIN_ADM_EDD_NN_IDX 31
#define IONADMIN_ADM_EDD_NN_STR "31"

#define IONADMIN_ADM_VAR_NN_IDX 32
#define IONADMIN_ADM_VAR_NN_STR "32"

#define IONADMIN_ADM_RPT_NN_IDX 33
#define IONADMIN_ADM_RPT_NN_STR "33"

#define IONADMIN_ADM_CTRL_NN_IDX 34
#define IONADMIN_ADM_CTRL_NN_STR "34"

#define IONADMIN_ADM_CONST_NN_IDX 35
#define IONADMIN_ADM_CONST_NN_STR "35"

#define IONADMIN_ADM_MACRO_NN_IDX 36
#define IONADMIN_ADM_MACRO_NN_STR "36"

#define IONADMIN_ADM_OP_NN_IDX 37
#define IONADMIN_ADM_OP_NN_STR "37"

#define IONADMIN_ADM_TBL_NN_IDX 38
#define IONADMIN_ADM_TBL_NN_STR "38"

#define IONADMIN_ADM_ROOT_NN_IDX 39
#define IONADMIN_ADM_ROOT_NN_STR "39"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONADMIN META-DATA DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |Name                         |861e0100    |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |NameSpace                    |861e0101    |The namespace of the ADM.                         |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |Version                      |861e0102    |The version of the ADM.                           |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |Organization                 |861e0103    |The name of the issuing organization of the ADM.  |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "Name"
#define ADM_IONADMIN_META_NAME_MID "861e0100"
// "NameSpace"
#define ADM_IONADMIN_META_NAMESPACE_MID "861e0101"
// "Version"
#define ADM_IONADMIN_META_VERSION_MID "861e0102"
// "Organization"
#define ADM_IONADMIN_META_ORGANIZATION_MID "861e0103"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                 IONADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |clockError                   |801f0100    |This is how accurate the ION Agent's clock is.    |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |clockSync                    |801f0101    |This is whether or not the the computer on which t|             |
   |                             |            |he local ION node is running has a synchronized cl|             |
   |                             |            |ock.                                              |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |congestionAlarmControl       |801f0102    |This is whether or not the node has a control that|             |
   |                             |            | will set off alarm if it will become congested at|             |
   |                             |            | some future time.                                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |congestionEndTimeForecasts   |801f0103    |This is when the node will be predicted to be no l|             |
   |                             |            |onger congested.                                  |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |consumptionRate              |801f0104    |This is the mean rate of continuous data delivery |             |
   |                             |            |to local BP applications.                         |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inboundFileSystemOccupancyLim|801f0105    |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's local file system that can be used|             |
   |                             |            | for the storage of inbound zero-copy objects. The|             |
   |                             |            | default heap limit is 1 Terabyte.                |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inboundHeapOccupancyLimit    |801f0106    |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's SDR non-volatile heap that can be |             |
   |                             |            |used for the storage of inbound zero-copy objects.|             |
   |                             |            | The default heap limit is 30% of the SDR data spa|             |
   |                             |            |ce's total heap size.                             |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |number                       |801f0107    |This is a CBHE node number which uniquely identifi|             |
   |                             |            |es the node in the delay-tolerant network.        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outboundFileSystemOccupancyLi|801f0108    |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's local file system that can be used|             |
   |                             |            | for the storage of outbound zero-copy objects. Th|             |
   |                             |            |e default heap limit is 1 Terabyte.               |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outboundHeapOccupancyLimit   |801f0109    |This is the maximum number of megabytes of storage|             |
   |                             |            | space in ION's SDR non-volatile heap that can be |             |
   |                             |            |used for the storage of outbound zero-copy objects|             |
   |                             |            |. The default heap limit is 30% of the SDR data sp|             |
   |                             |            |ace's total heap size.                            |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |productionRate               |801f010a    |This is the rate of local data production.        |UINT         |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |refTime                      |801f010b    |This is the reference time that will be used for i|             |
   |                             |            |nterpreting relative time values from now until th|             |
   |                             |            |e next revision of reference time.                |TS           |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |utcDelta                     |801f010c    |The UTC delta is used to compensate for error (dri|             |
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
   |version                      |801f010d    |This is the version of ION that is currently insta|             |
   |                             |            |lled.                                             |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_IONADMIN_EDD_CLOCKERROR_MID "801f0100"
#define ADM_IONADMIN_EDD_CLOCKSYNC_MID "801f0101"
#define ADM_IONADMIN_EDD_CONGESTIONALARMCONTROL_MID "801f0102"
#define ADM_IONADMIN_EDD_CONGESTIONENDTIMEFORECASTS_MID "801f0103"
#define ADM_IONADMIN_EDD_CONSUMPTIONRATE_MID "801f0104"
#define ADM_IONADMIN_EDD_INBOUNDFILESYSTEMOCCUPANCYLIMIT_MID "801f0105"
#define ADM_IONADMIN_EDD_INBOUNDHEAPOCCUPANCYLIMIT_MID "801f0106"
#define ADM_IONADMIN_EDD_NUMBER_MID "801f0107"
#define ADM_IONADMIN_EDD_OUTBOUNDFILESYSTEMOCCUPANCYLIMIT_MID "801f0108"
#define ADM_IONADMIN_EDD_OUTBOUNDHEAPOCCUPANCYLIMIT_MID "801f0109"
#define ADM_IONADMIN_EDD_PRODUCTIONRATE_MID "801f010a"
#define ADM_IONADMIN_EDD_REFTIME_MID "801f010b"
#define ADM_IONADMIN_EDD_UTCDELTA_MID "801f010c"
#define ADM_IONADMIN_EDD_VERSION_MID "801f010d"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                  IONADMIN VARIABLE DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                IONADMIN REPORT DEFINITIONS                                                           +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                    IONADMIN CONTROL DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |nodeInit                     |c3220108    |Until this control is executed, the local ION node|             |
   |                             |            | does not exist and most ionadmin controls will fa|             |
   |                             |            |il. The control configures the local node to be id|             |
   |                             |            |entified by nodeNumber, a CBHE node number which u|             |
   |                             |            |niquely identifies the node in the delay-tolerant |             |
   |                             |            |network.  It also configures ION's data space (SDR|             |
   |                             |            |) and shared working-memory region.  For this purp|             |
   |                             |            |ose it uses a set of default settings if no argume|             |
   |                             |            |nt follows nodeNumber or if the argument following|             |
   |                             |            | nodeNumber is ''; otherwise it uses the configura|             |
   |                             |            |tion settings found in a configuration file.  If c|             |
   |                             |            |onfiguration file name '.' is provided, then the c|             |
   |                             |            |onfiguration file's name is implicitly 'hostname.i|             |
   |                             |            |onconfig'; otherwise, ionConfigFileName is taken t|             |
   |                             |            |o be the explicit configuration file name.        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeClockErrorSet            |c3220100    |This management control sets ION's understanding o|             |
   |                             |            |f the accuracy of the scheduled start and stop tim|             |
   |                             |            |es of planned contacts, in seconds.  The default v|             |
   |                             |            |alue is 1.                                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeClockSyncSet             |c3220101    |This management control reports whether or not the|             |
   |                             |            | computer on which the local ION node is running h|             |
   |                             |            |as a synchronized clock.                          |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeCongestionAlarmControlSet|c3220102    |This management control establishes a control whic|             |
   |                             |            |h will automatically be executed whenever ionadmin|             |
   |                             |            | predicts that the node will become congested at s|             |
   |                             |            |ome future time.                                  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeCongestionEndTimeForecast|c3220103    |This management control sets the end time for comp|             |
   |                             |            |uted congestion forecasts. Setting congestion fore|             |
   |                             |            |cast horizon to zero sets the congestion forecast |             |
   |                             |            |end time to infinite time in the future: if there |             |
   |                             |            |is any predicted net growth in bundle storage spac|             |
   |                             |            |e occupancy at all, following the end of the last |             |
   |                             |            |scheduled contact, then eventual congestion will b|             |
   |                             |            |e predicted. The default value is zero, i.e., no e|             |
   |                             |            |nd time.                                          |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeConsumptionRateSet       |c3220104    |This management control sets ION's expectation of |             |
   |                             |            |the mean rate of continuous data delivery to local|             |
   |                             |            | BP applications throughout the period of time ove|             |
   |                             |            |r which congestion forecasts are computed. For nod|             |
   |                             |            |es that function only as routers this variable wil|             |
   |                             |            |l normally be zero. A value of -1, which is the de|             |
   |                             |            |fault, indicates that the rate of local data consu|             |
   |                             |            |mption is unknown; in that case local data consump|             |
   |                             |            |tion is not considered in the computation of conge|             |
   |                             |            |stion forecasts.                                  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeContactAdd               |c3220105    |This control schedules a period of data transmissi|             |
   |                             |            |on from sourceNode to destNode. The period of tran|             |
   |                             |            |smission will begin at startTime and end at stopTi|             |
   |                             |            |me, and the rate of data transmission will be xmit|             |
   |                             |            |DataRate bytes/second. Our confidence in the conta|             |
   |                             |            |ct defaults to 1.0, indicating that the contact is|             |
   |                             |            | scheduled - not that non-occurrence of the contac|             |
   |                             |            |t is impossible, just that occurrence of the conta|             |
   |                             |            |ct is planned and scheduled rather than merely imp|             |
   |                             |            |uted from past node behavior. In the latter case, |             |
   |                             |            |confidence indicates our estimation of the likelih|             |
   |                             |            |ood of this potential contact.                    |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeContactDel               |c3220106    |This control deletes the scheduled period of data |             |
   |                             |            |transmission from sourceNode to destNode starting |             |
   |                             |            |at startTime. To delete all contacts between some |             |
   |                             |            |pair of nodes, use '*' as startTime.              |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeInboundHeapOccupancyLimit|c3220107    |This management control sets the maximum number of|             |
   |                             |            | megabytes of storage space in ION's SDR non-volat|             |
   |                             |            |ile heap that can be used for the storage of inbou|             |
   |                             |            |nd zero-copy objects. A value of -1 for either lim|             |
   |                             |            |it signifies 'leave unchanged'. The default heap l|             |
   |                             |            |imit is 30% of the SDR data space's total heap siz|             |
   |                             |            |e.                                                |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeOutboundHeapOccupancyLimi|c3220109    |This management control sets the maximum number of|             |
   |                             |            | megabytes of storage space in ION's SDR non-volat|             |
   |                             |            |ile heap that can be used for the storage of outbo|             |
   |                             |            |und zero-copy objects.  A value of -1 for either l|             |
   |                             |            |imit signifies 'leave unchanged'. The default heap|             |
   |                             |            | limit is 30% of the SDR data space's total heap s|             |
   |                             |            |ize.                                              |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeProductionRateSet        |c322010a    |This management control sets ION's expectation of |             |
   |                             |            |the mean rate of continuous data origination by lo|             |
   |                             |            |cal BP applications throughout the period of time |             |
   |                             |            |over which congestion forecasts are computed. For |             |
   |                             |            |nodes that function only as routers this variable |             |
   |                             |            |will normally be zero. A value of -1, which is the|             |
   |                             |            | default, indicates that the rate of local data pr|             |
   |                             |            |oduction is unknown; in that case local data produ|             |
   |                             |            |ction is not considered in the computation of cong|             |
   |                             |            |estion forecasts.                                 |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeRangeAdd                 |c322010b    |This control predicts a period of time during whic|             |
   |                             |            |h the distance from node to otherNode will be cons|             |
   |                             |            |tant to within one light second. The period will b|             |
   |                             |            |egin at startTime and end at stopTime, and the dis|             |
   |                             |            |tance between the nodes during that time will be d|             |
   |                             |            |istance light seconds.                            |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeRangeDel                 |c322010c    |This control deletes the predicted period of const|             |
   |                             |            |ant distance between node and otherNode starting a|             |
   |                             |            |t startTime. To delete all ranges between some pai|             |
   |                             |            |r of nodes, use '*' as startTime.                 |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeRefTimeSet               |c322010d    |This is used to set the reference time that will b|             |
   |                             |            |e used for interpreting relative time values from |             |
   |                             |            |now until the next revision of reference time. sNo|             |
   |                             |            |te that the new reference time can be a relative t|             |
   |                             |            |ime, i.e., an offset beyond the current reference |             |
   |                             |            |time.                                             |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |nodeUTCDeltaSet              |c322010e    |This management control sets ION's understanding o|             |
   |                             |            |f the current difference between correct UTC time |             |
   |                             |            |and the time values reported by the clock for the |             |
   |                             |            |local ION node's computer. This delta is automatic|             |
   |                             |            |ally applied to locally obtained time values whene|             |
   |                             |            |ver ION needs to know the current time.           |             |
   +-----------------------------+------------+----------------------------------------------------------------+
 */
#define ADM_IONADMIN_CTRL_NODEINIT_MID "c3220108"
#define ADM_IONADMIN_CTRL_NODECLOCKERRORSET_MID "c3220100"
#define ADM_IONADMIN_CTRL_NODECLOCKSYNCSET_MID "c3220101"
#define ADM_IONADMIN_CTRL_NODECONGESTIONALARMCONTROLSET_MID "c3220102"
#define ADM_IONADMIN_CTRL_NODECONGESTIONENDTIMEFORECASTSSET_MID "c3220103"
#define ADM_IONADMIN_CTRL_NODECONSUMPTIONRATESET_MID "c3220104"
#define ADM_IONADMIN_CTRL_NODECONTACTADD_MID "c3220105"
#define ADM_IONADMIN_CTRL_NODECONTACTDEL_MID "c3220106"
#define ADM_IONADMIN_CTRL_NODEINBOUNDHEAPOCCUPANCYLIMITSET_MID "c3220107"
#define ADM_IONADMIN_CTRL_NODEOUTBOUNDHEAPOCCUPANCYLIMITSET_MID "c3220109"
#define ADM_IONADMIN_CTRL_NODEPRODUCTIONRATESET_MID "c322010a"
#define ADM_IONADMIN_CTRL_NODERANGEADD_MID "c322010b"
#define ADM_IONADMIN_CTRL_NODERANGEDEL_MID "c322010c"
#define ADM_IONADMIN_CTRL_NODEREFTIMESET_MID "c322010d"
#define ADM_IONADMIN_CTRL_NODEUTCDELTASET_MID "c322010e"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                IONADMIN CONSTANT DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                IONADMIN MACRO DEFINITIONS                                                            +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |							      IONADMIN OPERATOR DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_IonAdmin_init();
void adm_IonAdmin_init_edd();
void adm_IonAdmin_init_variables();
void adm_IonAdmin_init_controls();
void adm_IonAdmin_init_constants();
void adm_IonAdmin_init_macros();
void adm_IonAdmin_init_metadata();
void adm_IonAdmin_init_ops();
void adm_IonAdmin_init_reports();
#endif /* _HAVE_IONADMIN_ADM_ */
#endif //ADM_IONADMIN_H_