/******************************************************************************
 **
 ** File Name: ./shared/adm_IonBpAdmin.h
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
#ifndef ADM_IONBPADMIN_H_
#define ADM_IONBPADMIN_H_
#define _HAVE_IONBPADMIN_ADM_
#ifdef _HAVE_IONBPADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                                          +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:IonBpAdmin
 * ADM NICKNAMES:
 *
 *
 *					IONBPADMIN ADM ROOT
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
 * META-> 50
 * EDD -> 51
 * VAR -> 52
 * RPT -> 53
 * CTRL -> 54
 * CONST -> 55
 * MACRO -> 56
 * OP -> 57
 * TBL -> 58
 * ROOT -> 59

 */
#define IONBPADMIN_ADM_META_NN_IDX 50
#define IONBPADMIN_ADM_META_NN_STR "50"

#define IONBPADMIN_ADM_EDD_NN_IDX 51
#define IONBPADMIN_ADM_EDD_NN_STR "51"

#define IONBPADMIN_ADM_VAR_NN_IDX 52
#define IONBPADMIN_ADM_VAR_NN_STR "52"

#define IONBPADMIN_ADM_RPT_NN_IDX 53
#define IONBPADMIN_ADM_RPT_NN_STR "53"

#define IONBPADMIN_ADM_CTRL_NN_IDX 54
#define IONBPADMIN_ADM_CTRL_NN_STR "54"

#define IONBPADMIN_ADM_CONST_NN_IDX 55
#define IONBPADMIN_ADM_CONST_NN_STR "55"

#define IONBPADMIN_ADM_MACRO_NN_IDX 56
#define IONBPADMIN_ADM_MACRO_NN_STR "56"

#define IONBPADMIN_ADM_OP_NN_IDX 57
#define IONBPADMIN_ADM_OP_NN_STR "57"

#define IONBPADMIN_ADM_TBL_NN_IDX 58
#define IONBPADMIN_ADM_TBL_NN_STR "58"

#define IONBPADMIN_ADM_ROOT_NN_IDX 59
#define IONBPADMIN_ADM_ROOT_NN_STR "59"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    IONBPADMIN META-DATA DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |86320100    |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |86320101    |The namespace of the ADM                          |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |86320102    |The version of the ADM                            |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |86320103    |The name of the issuing organization of the ADM   |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_IONBPADMIN_META_NAME_MID "86320100"
// "namespace"
#define ADM_IONBPADMIN_META_NAMESPACE_MID "86320101"
// "version"
#define ADM_IONBPADMIN_META_VERSION_MID "86320102"
// "organization"
#define ADM_IONBPADMIN_META_ORGANIZATION_MID "86320103"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                 IONBPADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |80330100    |This is the version of ion that is currently insta|             |
   |                             |            |lled and the crypto suite that BP was compiled wit|             |
   |                             |            |h.                                                |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_IONBPADMIN_EDD_VERSION_MID "80330100"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                  IONBPADMIN VARIABLE DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                IONBPADMIN REPORT DEFINITIONS                                                           +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |			                    IONBPADMIN CONTROL DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpointAdd                  |c3360100    |This control establishes a DTN endpoint named endp|             |
   |                             |            |ointId on the local node. The remaining parameters|             |
   |                             |            | indicate what is to be done when bundles destined|             |
   |                             |            | for this endpoint arrive at a time when no applic|             |
   |                             |            |ation has the endpoint open for bundle reception. |             |
   |                             |            |If type is 0, then such bundles are to be discarde|             |
   |                             |            |d silently and immediately. If type is 1, then suc|             |
   |                             |            |h bundles are to be enqueued for later delivery an|             |
   |                             |            |d, if recvScript is provided, recvScript is to be |             |
   |                             |            |executed.                                         |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |endpointChange               |c3360101    |This control changes the action that is to be take|             |
   |                             |            |n when bundles destined for this endpoint arrive a|             |
   |                             |            |t a time when no application has the endpoint open|             |
   |                             |            | for bundle reception.                            |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |endpointDel                  |c3360102    |This control deletes the endpoint identified by en|             |
   |                             |            |dpointId. The control will fail if any bundles are|             |
   |                             |            | currently pending delivery to this endpoint.     |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |inductAdd                    |c3360103    |This control establishes a duct for reception of b|             |
   |                             |            |undles via the indicated CL protocol. The duct's d|             |
   |                             |            |ata acquisition structure is used and populated by|             |
   |                             |            | the induct task whose operation is initiated by c|             |
   |                             |            |liControl at the time the duct is started.        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |inductChange                 |c3360104    |This changes the control that is used to initiate |             |
   |                             |            |operation of the induct task for the indicated duc|             |
   |                             |            |t.                                                |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |inductDel                    |c3360105    |This control deletes the induct identified by prot|             |
   |                             |            |ocolName and ductName. The control will fail if an|             |
   |                             |            |y bundles are currently pending acquisition via th|             |
   |                             |            |is induct.                                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |inductStart                  |c3360106    |This control starts the indicated induct task as d|             |
   |                             |            |efined for the indicated CL protocol on the local |             |
   |                             |            |node.                                             |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |inductStop                   |c3360107    |This control stops the indicated induct task as de|             |
   |                             |            |fined for the indicated CL protocol on the local n|             |
   |                             |            |ode.                                              |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |manageHeapMax                |c3360108    |This control declares the maximum number of bytes |             |
   |                             |            |of SDR heap space that will be occupied by any sin|             |
   |                             |            |gle bundle acquisition activity (nominally the acq|             |
   |                             |            |uisition of a single bundle, but this is at the di|             |
   |                             |            |scretion of the convergence-layer input task). All|             |
   |                             |            | data acquired in excess of this limit will be wri|             |
   |                             |            |tten to a temporary file pending extraction and di|             |
   |                             |            |spatching of the acquired bundle or bundles. The d|             |
   |                             |            |efault is the minimum allowed value (560 bytes), w|             |
   |                             |            |hich is the approximate size of a ZCO file referen|             |
   |                             |            |ce object; this is the minimum SDR heap space occu|             |
   |                             |            |pancy in the event that all acquisition is into a |             |
   |                             |            |file.                                             |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |outductAdd                   |c3360109    |This control establishes a duct for transmission o|             |
   |                             |            |f bundles via the indicated CL protocol. the duct'|             |
   |                             |            |s data transmission structure is serviced by the o|             |
   |                             |            |utduct task whose operation is initiated by CLOcom|             |
   |                             |            |mand at the time the duct is started. A value of z|             |
   |                             |            |ero for maxPayloadLength indicates that bundles of|             |
   |                             |            | any size can be accomodated;this is the default. |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |outductChange                |c336010a    |This control sets new values for the indicated duc|             |
   |                             |            |t's payload size limit and the control that is use|             |
   |                             |            |d to initiate operation of the outduct task for th|             |
   |                             |            |is duct.                                          |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |outductDel                   |c336010b    |This control deletes the outduct identified by pro|             |
   |                             |            |tocolName and ductName. The control will fail if a|             |
   |                             |            |ny bundles are currently pending transmission via |             |
   |                             |            |this outduct.                                     |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |outductStart                 |c336010c    |This control starts the indicated outduct task as |             |
   |                             |            |defined for the indicated CL protocol on the local|             |
   |                             |            | node.                                            |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |outductStop                  |c336010d    |This control stops the indicated outduct task as d|             |
   |                             |            |efined for the indicated CL protocol on the local |             |
   |                             |            |node.                                             |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |protocolAdd                  |c336010e    |This control establishes access to the named conve|             |
   |                             |            |rgence layer protocol at the local node. The paylo|             |
   |                             |            |adBytesPerFrame and overheadBytesPerFrame argument|             |
   |                             |            |s are used in calculating the estimated transmissi|             |
   |                             |            |on capacity consumption of each bundle, to aid in |             |
   |                             |            |route computation and congesting forecasting. The |             |
   |                             |            |optional nominalDataRate argument overrides the ha|             |
   |                             |            |rd coded default continuous data rate for the indi|             |
   |                             |            |cated protocol for purposes of rate control. For a|             |
   |                             |            |ll promiscuous prototocols-that is, protocols whos|             |
   |                             |            |e outducts are not specifically dedicated to trans|             |
   |                             |            |mission to a single identified convergence-layer p|             |
   |                             |            |rotocol endpoint- the protocol's applicable nomina|             |
   |                             |            |l continuous data rate is the data rate that is al|             |
   |                             |            |ways used for rate control over links served by th|             |
   |                             |            |at protocol; data rates are not extracted from con|             |
   |                             |            |tact graph information. This is because only the i|             |
   |                             |            |nduct and outduct throttles for non-promiscuous pr|             |
   |                             |            |otocols (LTP, TCP) can be dynamically adjusted in |             |
   |                             |            |response to changes in data rate between the local|             |
   |                             |            | node and its neighbors, as enacted per the contac|             |
   |                             |            |t plan. Even for an outduct of a non-promiscuous p|             |
   |                             |            |rotocol the nominal data rate may be the authority|             |
   |                             |            | for rate control, in the event that the contact p|             |
   |                             |            |lan lacks identified contacts with the node to whi|             |
   |                             |            |ch the outduct is mapped.                         |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |protocolDel                  |c336010f    |This control deletes the convergence layer protoco|             |
   |                             |            |l identified by protocolName. The control will fai|             |
   |                             |            |l if any ducts are still locally declared for this|             |
   |                             |            | protocol.                                        |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |protocolStart                |c3360110    |This control starts all induct and outduct tasks f|             |
   |                             |            |or inducts and outducts that have been defined for|             |
   |                             |            | the indicated CL protocol on the local node.     |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |protocolStop                 |c3360111    |This control stops all induct and outduct tasks fo|             |
   |                             |            |r inducts and outducts that have been defined for |             |
   |                             |            |the indicated CL protocol on the local node.      |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |schemeAdd                    |c3360112    |This control declares an endpoint naming scheme fo|             |
   |                             |            |r use in endpoint IDs, which are structured as URI|             |
   |                             |            |s: schemeName:schemeSpecificPart. forwarderControl|             |
   |                             |            | will be executed when the scheme is started on th|             |
   |                             |            |is node, to initiate operation of a forwarding dae|             |
   |                             |            |mon for this scheme. adminAppControl will also be |             |
   |                             |            |executed when the scheme is started on this node, |             |
   |                             |            |to initiate operation of a daemon that opens a cus|             |
   |                             |            |todian endpoint identified within this scheme so t|             |
   |                             |            |hat it can recieve and process custody signals and|             |
   |                             |            | bundle status reports.                           |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |schemeChange                 |c3360113    |This control sets the indicated scheme's forwarder|             |
   |                             |            |Control and adminAppControl to the strings provide|             |
   |                             |            |d as arguments.                                   |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |schemeDel                    |c3360114    |This control deletes the scheme identified by sche|             |
   |                             |            |meName. The control will fail if any bundles ident|             |
   |                             |            |ified in this scheme are pending forwarding, trans|             |
   |                             |            |mission, or delivery.                             |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |schemeStart                  |c3360115    |This control starts the forwarder and administrati|             |
   |                             |            |ve endpoint tasks for the indicated scheme task on|             |
   |                             |            | the local node.                                  |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |schemeStop                   |c3360116    |This control stops the forwarder and administrativ|             |
   |                             |            |e endpoint tasks for the indicated scheme task on |             |
   |                             |            |the local node.                                   |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanAdd                |c3360117    |This command establishes an egress plan governing |             |
   |                             |            |transmission to the neighboring node[s] identified|             |
   |                             |            | by endpoint_name.  The plan is functionally enact|             |
   |                             |            |ed by a bpclm(1) daemon dedicated to managing bund|             |
   |                             |            |les queued for transmission to the indicated neigh|             |
   |                             |            |boring node[s].                                   |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanUpdate             |c3360118    |This command sets a new value for the indicated pl|             |
   |                             |            |an's transmission rate.                           |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanDelete             |c3360119    |This command deletes the outduct identified by end|             |
   |                             |            |point_name.  The command will fail if any bundles |             |
   |                             |            |are currently pending transmission per this plan. |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanStart              |c336011a    |This command starts the bpclm task for the indicat|             |
   |                             |            |ed egress plan.                                   |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanStop               |c336011b    |This command stops the bpclm task for the indicate|             |
   |                             |            |d egress plan.                                    |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanBlock              |c336011c    |This command disables transmission of bundles queu|             |
   |                             |            |ed for transmission to the indicated node and refo|             |
   |                             |            |rwards all non-critical bundles currently queued f|             |
   |                             |            |or transmission to this node.  This may result in |             |
   |                             |            |some or all of these bundles being enqueued for tr|             |
   |                             |            |ansmission (actually just retention) to the pseudo|             |
   |                             |            |-node limbo                                       |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanUnblock            |c336011d    |This command re-enables transmission of bundles to|             |
   |                             |            | the indicated node and reforwards all bundles in |             |
   |                             |            |limbo in the hope that the unblocking of this egre|             |
   |                             |            |ss plan will enable some of them to be transmitted|             |
   |                             |            |.                                                 |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanGateway            |c336011e    |This command declares the name of the endpoint to |             |
   |                             |            |which bundles queued for transmission to the node[|             |
   |                             |            |s] identified byendpoint_name must be re-routed.  |             |
   |                             |            |Declaring gateway_endpoint_name to be the zero-len|             |
   |                             |            |gth string '' disables re-routing: bundles will in|             |
   |                             |            |stead be transmitted using the plan's attached con|             |
   |                             |            |vergence-layer protocol outduct[s].               |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanOutductAttach      |c336011f    |This command declares that the indicated convergen|             |
   |                             |            |ce-layer protocol outduct is now a viable device f|             |
   |                             |            |or transmitting bundles to the node[s] identified |             |
   |                             |            |by endpoint_name.                                 |             |
   +-----------------------------+------------+----------------------------------------------------------------+
   |egressPlanOutductDetatch     |c3360120    |This command declares that the indicated convergen|             |
   |                             |            |ce-layer protocol outduct is no longer a viable de|             |
   |                             |            |vice for transmitting bundles to the node[s] ident|             |
   |                             |            |ified by endpoint_name.                           |             |
   +-----------------------------+------------+----------------------------------------------------------------+
 */
#define ADM_IONBPADMIN_CTRL_ENDPOINTADD_MID "c3360100"
#define ADM_IONBPADMIN_CTRL_ENDPOINTCHANGE_MID "c3360101"
#define ADM_IONBPADMIN_CTRL_ENDPOINTDEL_MID "c3360102"
#define ADM_IONBPADMIN_CTRL_INDUCTADD_MID "c3360103"
#define ADM_IONBPADMIN_CTRL_INDUCTCHANGE_MID "c3360104"
#define ADM_IONBPADMIN_CTRL_INDUCTDEL_MID "c3360105"
#define ADM_IONBPADMIN_CTRL_INDUCTSTART_MID "c3360106"
#define ADM_IONBPADMIN_CTRL_INDUCTSTOP_MID "c3360107"
#define ADM_IONBPADMIN_CTRL_MANAGEHEAPMAX_MID "c3360108"
#define ADM_IONBPADMIN_CTRL_OUTDUCTADD_MID "c3360109"
#define ADM_IONBPADMIN_CTRL_OUTDUCTCHANGE_MID "c336010a"
#define ADM_IONBPADMIN_CTRL_OUTDUCTDEL_MID "c336010b"
#define ADM_IONBPADMIN_CTRL_OUTDUCTSTART_MID "c336010c"
#define ADM_IONBPADMIN_CTRL_OUTDUCTSTOP_MID "c336010d"
#define ADM_IONBPADMIN_CTRL_PROTOCOLADD_MID "c336010e"
#define ADM_IONBPADMIN_CTRL_PROTOCOLDEL_MID "c336010f"
#define ADM_IONBPADMIN_CTRL_PROTOCOLSTART_MID "c3360110"
#define ADM_IONBPADMIN_CTRL_PROTOCOLSTOP_MID "c3360111"
#define ADM_IONBPADMIN_CTRL_SCHEMEADD_MID "c3360112"
#define ADM_IONBPADMIN_CTRL_SCHEMECHANGE_MID "c3360113"
#define ADM_IONBPADMIN_CTRL_SCHEMEDEL_MID "c3360114"
#define ADM_IONBPADMIN_CTRL_SCHEMESTART_MID "c3360115"
#define ADM_IONBPADMIN_CTRL_SCHEMESTOP_MID "c3360116"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANADD_MID "c3360117"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANUPDATE_MID "c3360118"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANDELETE_MID "c3360119"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANSTART_MID "c336011a"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANSTOP_MID "c336011b"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANBLOCK_MID "c336011c"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANUNBLOCK_MID "c336011d"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANGATEWAY_MID "c336011e"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTATTACH_MID "c336011f"
#define ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTDETATCH_MID "c3360120"

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                IONBPADMIN CONSTANT DEFINITIONS                                                         +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |				                IONBPADMIN MACRO DEFINITIONS                                                            +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |							      IONBPADMIN OPERATOR DEFINITIONS                                                          +
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_IonBpAdmin_init();
void adm_IonBpAdmin_init_edd();
void adm_IonBpAdmin_init_variables();
void adm_IonBpAdmin_init_controls();
void adm_IonBpAdmin_init_constants();
void adm_IonBpAdmin_init_macros();
void adm_IonBpAdmin_init_metadata();
void adm_IonBpAdmin_init_ops();
void adm_IonBpAdmin_init_reports();
#endif /* _HAVE_IONBPADMIN_ADM_ */
#endif //ADM_IONBPADMIN_H_