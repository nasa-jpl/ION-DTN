/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin.h
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
 **  2018-01-06  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_BP_ADMIN_H_
#define ADM_ION_BP_ADMIN_H_
#define _HAVE_ION_BP_ADMIN_ADM_
#ifdef _HAVE_ION_BP_ADMIN_ADM_

#include "lyst.h"
#include "../utils/nm_types.h"
#include "adm.h"

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |			              ADM TEMPLATE DOCUMENTATION                                              +
 * +----------------------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:arn:DTN:ion_bp_admin
 */

/*
 * +----------------------------------------------------------------------------------------------------------+
 * |				             AGENT NICKNAME DEFINITIONS                                       +
 * +----------------------------------------------------------------------------------------------------------+
 */
#define ION_BP_ADMIN_ADM_META_NN_IDX 50
#define ION_BP_ADMIN_ADM_META_NN_STR "50"

#define ION_BP_ADMIN_ADM_EDD_NN_IDX 51
#define ION_BP_ADMIN_ADM_EDD_NN_STR "51"

#define ION_BP_ADMIN_ADM_VAR_NN_IDX 52
#define ION_BP_ADMIN_ADM_VAR_NN_STR "52"

#define ION_BP_ADMIN_ADM_RPT_NN_IDX 53
#define ION_BP_ADMIN_ADM_RPT_NN_STR "53"

#define ION_BP_ADMIN_ADM_CTRL_NN_IDX 54
#define ION_BP_ADMIN_ADM_CTRL_NN_STR "54"

#define ION_BP_ADMIN_ADM_CONST_NN_IDX 55
#define ION_BP_ADMIN_ADM_CONST_NN_STR "55"

#define ION_BP_ADMIN_ADM_MACRO_NN_IDX 56
#define ION_BP_ADMIN_ADM_MACRO_NN_STR "56"

#define ION_BP_ADMIN_ADM_OP_NN_IDX 57
#define ION_BP_ADMIN_ADM_OP_NN_STR "57"

#define ION_BP_ADMIN_ADM_TBL_NN_IDX 58
#define ION_BP_ADMIN_ADM_TBL_NN_STR "58"

#define ION_BP_ADMIN_ADM_ROOT_NN_IDX 59
#define ION_BP_ADMIN_ADM_ROOT_NN_STR "59"


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN META-DATA DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |name                         |0x87320100  |The human-readable name of the ADM.               |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |namespace                    |0x87320101  |The namespace of the ADM                          |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x87320102  |The version of the ADM                            |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |organization                 |0x87320103  |The name of the issuing organization of the ADM   |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
// "name"
#define ADM_ION_BP_ADMIN_META_NAME_MID 0x87320100
// "namespace"
#define ADM_ION_BP_ADMIN_META_NAMESPACE_MID 0x87320101
// "version"
#define ADM_ION_BP_ADMIN_META_VERSION_MID 0x87320102
// "organization"
#define ADM_ION_BP_ADMIN_META_ORGANIZATION_MID 0x87320103


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN EXTERNALLY DEFINED DATA DEFINITIONS                                               
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |version                      |0x80330100  |Version of installed ION BP Admin utility.        |STR          |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_BP_ADMIN_EDD_VERSION_MID 0x80330100


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN VARIABLE DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN REPORT DEFINITIONS                                                           
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN CONTROL DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpointAdd                  |0xc3360100  |Establish DTN endpoint named endpointId on the loc|             |
   |                             |            |al node. The remaining parameters indicate what is|             |
   |                             |            | to be done when bundles destined for this endpoin|             |
   |                             |            |t arrive at a time when no application has the end|             |
   |                             |            |point open for bundle reception. If type is 'x', t|             |
   |                             |            |hen such bundles are to be discarded silently and |             |
   |                             |            |immediately. If type is 'q', then such bundles are|             |
   |                             |            | to be enqueued for later delivery and, if recvScr|             |
   |                             |            |ipt is provided, recvScript is to be executed.    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpointChange               |0xc3360101  |Change the action taken when bundles destined for |             |
   |                             |            |this endpoint arrive at a time when no application|             |
   |                             |            | has the endpoint open for bundle reception.      |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |endpointDel                  |0xc3360102  |Delete the endpoint identified by endpointId. The |             |
   |                             |            |control will fail if any bundles are currently pen|             |
   |                             |            |ding delivery to this endpoint.                   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inductAdd                    |0xc3360103  |Establish a duct for reception of bundles via the |             |
   |                             |            |indicated CL protocol. The duct's data acquisition|             |
   |                             |            | structure is used and populated by the induct tas|             |
   |                             |            |k whose operation is initiated by cliControl at th|             |
   |                             |            |e time the duct is started.                       |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inductChange                 |0xc3360104  |Change the control used to initiate operation of t|             |
   |                             |            |he induct task for the indicated duct.            |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inductDel                    |0xc3360105  |Delete the induct identified by protocolName and d|             |
   |                             |            |uctName. The control will fail if any bundles are |             |
   |                             |            |currently pending acquisition via this induct.    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inductStart                  |0xc3360106  |Start the indicated induct task as defined for the|             |
   |                             |            | indicated CL protocol on the local node.         |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |inductStop                   |0xc3360107  |Stop the indicated induct task as defined for the |             |
   |                             |            |indicated CL protocol on the local node.          |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |init                         |0x83360108  |Until this control is executed, Bundle Protocol is|             |
   |                             |            | not in operation on the local ION node and most b|             |
   |                             |            |padmin controls will fail.                        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |manageHeapMax                |0xc3360109  |Declare the maximum number of bytes of SDR heap sp|             |
   |                             |            |ace that will be occupied by any single bundle acq|             |
   |                             |            |uisition activity (nominally the acquisition of a |             |
   |                             |            |single bundle, but this is at the discretion of th|             |
   |                             |            |e convergence-layer input task). All data acquired|             |
   |                             |            | in excess of this limit will be written to a temp|             |
   |                             |            |orary file pending extraction and dispatching of t|             |
   |                             |            |he acquired bundle or bundles. The default is the |             |
   |                             |            |minimum allowed value (560 bytes), which is the ap|             |
   |                             |            |proximate size of a ZCO file reference object; thi|             |
   |                             |            |s is the minimum SDR heap space occupancy in the e|             |
   |                             |            |vent that all acquisition is into a file.         |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductAdd                   |0xc336010a  |Establish a duct for transmission of bundles via t|             |
   |                             |            |he indicated CL protocol. the duct's data transmis|             |
   |                             |            |sion structure is serviced by the outduct task who|             |
   |                             |            |se operation is initiated by CLOcommand at the tim|             |
   |                             |            |e the duct is started. A value of zero for maxPayl|             |
   |                             |            |oadLength indicates that bundles of any size can b|             |
   |                             |            |e accomodated;this is the default.                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductChange                |0xc336010b  |Set new values for the indicated duct's payload si|             |
   |                             |            |ze limit and the control that is used to initiate |             |
   |                             |            |operation of the outduct task for this duct.      |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductDel                   |0xc336010c  |Delete the outduct identified by protocolName and |             |
   |                             |            |ductName. The control will fail if any bundles are|             |
   |                             |            | currently pending transmission via this outduct. |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductStart                 |0xc336010d  |Start the indicated outduct task as defined for th|             |
   |                             |            |e indicated CL protocol on the local node.        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductBlock                 |0xc336010e  |Disable transmission of bundles queued for transmi|             |
   |                             |            |ssion to the indicated node and reforwards all non|             |
   |                             |            |-critical bundles currently queued for transmissio|             |
   |                             |            |n to this node. This may result in some or all of |             |
   |                             |            |these bundles being enqueued for transmission to t|             |
   |                             |            |he psuedo-node limbo.                             |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductUnblock               |0xc336010f  |Re-enable transmission of bundles to the indicated|             |
   |                             |            | node and reforwards all bundles in limbo in the h|             |
   |                             |            |ope that the unblocking of this egress plan will e|             |
   |                             |            |nable some of them to be transmitted.             |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |outductStop                  |0xc3360110  |Stop the indicated outduct task as defined for the|             |
   |                             |            | indicated CL protocol on the local node.         |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |protocolAdd                  |0xc3360111  |Establish access to the named convergence layer pr|             |
   |                             |            |otocol at the local node. The payloadBytesPerFrame|             |
   |                             |            | and overheadBytesPerFrame arguments are used in c|             |
   |                             |            |alculating the estimated transmission capacity con|             |
   |                             |            |sumption of each bundle, to aid in route computati|             |
   |                             |            |on and congesting forecasting. The optional nomina|             |
   |                             |            |lDataRate argument overrides the hard coded defaul|             |
   |                             |            |t continuous data rate for the indicated protocol |             |
   |                             |            |for purposes of rate control. For all promiscuous |             |
   |                             |            |prototocols-that is, protocols whose outducts are |             |
   |                             |            |not specifically dedicated to transmission to a si|             |
   |                             |            |ngle identified convergence-layer protocol endpoin|             |
   |                             |            |t- the protocol's applicable nominal continuous da|             |
   |                             |            |ta rate is the data rate that is always used for r|             |
   |                             |            |ate control over links served by that protocol; da|             |
   |                             |            |ta rates are not extracted from contact graph info|             |
   |                             |            |rmation. This is because only the induct and outdu|             |
   |                             |            |ct throttles for non-promiscuous protocols (LTP, T|             |
   |                             |            |CP) can be dynamically adjusted in response to cha|             |
   |                             |            |nges in data rate between the local node and its n|             |
   |                             |            |eighbors, as enacted per the contact plan. Even fo|             |
   |                             |            |r an outduct of a non-promiscuous protocol the nom|             |
   |                             |            |inal data rate may be the authority for rate contr|             |
   |                             |            |ol, in the event that the contact plan lacks ident|             |
   |                             |            |ified contacts with the node to which the outduct |             |
   |                             |            |is mapped.                                        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |protocolDel                  |0xc3360112  |Delete the convergence layer protocol identified b|             |
   |                             |            |y protocolName. The control will fail if any ducts|             |
   |                             |            | are still locally declared for this protocol.    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |protocolStart                |0xc3360113  |Start all induct and outduct tasks for inducts and|             |
   |                             |            | outducts that have been defined for the indicated|             |
   |                             |            | CL protocol on the local node.                   |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |protocolStop                 |0xc3360114  |Stop all induct and outduct tasks for inducts and |             |
   |                             |            |outducts that have been defined for the indicated |             |
   |                             |            |CL protocol on the local node.                    |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |schemeAdd                    |0xc3360115  |Declares an endpoint naming scheme for use in endp|             |
   |                             |            |oint IDs, which are structured as URIs: schemeName|             |
   |                             |            |:schemeSpecificPart. forwarderControl will be exec|             |
   |                             |            |uted when the scheme is started on this node, to i|             |
   |                             |            |nitiate operation of a forwarding daemon for this |             |
   |                             |            |scheme. adminAppControl will also be executed when|             |
   |                             |            | the scheme is started on this node, to initiate o|             |
   |                             |            |peration of a daemon that opens a custodian endpoi|             |
   |                             |            |nt identified within this scheme so that it can re|             |
   |                             |            |cieve and process custody signals and bundle statu|             |
   |                             |            |s reports.                                        |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |schemeChange                 |0xc3360116  |Set the indicated scheme's forwarderControl and ad|             |
   |                             |            |minAppControl to the strings provided as arguments|             |
   |                             |            |.                                                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |schemeDel                    |0xc3360117  |Delete the scheme identified by schemeName. The co|             |
   |                             |            |ntrol will fail if any bundles identified in this |             |
   |                             |            |scheme are pending forwarding, transmission, or de|             |
   |                             |            |livery.                                           |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |schemeStart                  |0xc3360118  |Start the forwarder and administrative endpoint ta|             |
   |                             |            |sks for the indicated scheme task on the local nod|             |
   |                             |            |e.                                                |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |schemeStop                   |0xc3360119  |Stop the forwarder and administrative endpoint tas|             |
   |                             |            |ks for the indicated scheme task on the local node|             |
   |                             |            |.                                                 |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |start                        |0x8336011a  |Start all schemes and all protocols on the local n|             |
   |                             |            |ode.                                              |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |stop                         |0x8336011b  |Stop all schemes and all protocols on the local no|             |
   |                             |            |de.                                               |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |watch                        |0xc336011c  |Enable/Disable production of a continuous stream o|             |
   |                             |            |f user selected Bundle Protocol activity indicatio|             |
   |                             |            |n characters. A watch parameter of 1 selects all B|             |
   |                             |            |P activity indication characters, 0 deselects allB|             |
   |                             |            |P activity indication characters; any other activi|             |
   |                             |            |tySpec such as acz~ selects all activity indicatio|             |
   |                             |            |n characters in the string, deselecting all others|             |
   |                             |            |. BP will print each selected activity indication |             |
   |                             |            |character to stdout every time a processing event |             |
   |                             |            |of the associated type occurs: a new bundle is que|             |
   |                             |            |ued for forwarding, b bundle is queued for transmi|             |
   |                             |            |ssion, c bundle is popped from its transmission qu|             |
   |                             |            |eue, m custody acceptance signal is recieved, w cu|             |
   |                             |            |stody of bundle is accepted, x custody of bundle i|             |
   |                             |            |s refused, y bundle is accepted upon arrival, z bu|             |
   |                             |            |ndle is queued for delivery to an application, ~ b|             |
   |                             |            |undle is abandoned (discarded) on attempt to forwa|             |
   |                             |            |rd it, ! bundle is destroyed due to TTL expiration|             |
   |                             |            |, & custody refusal signal is recieved, # bundle i|             |
   |                             |            |s queued for re-forwarding due to CL protocol fail|             |
   |                             |            |ures, j bundle is placed in 'limbo' for possible f|             |
   |                             |            |uture reforwarding, k bundle is removed from 'limb|             |
   |                             |            |o' and queued for reforwarding, $ bundle's custodi|             |
   |                             |            |al retransmission timeout interval expired.       |             |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */
#define ADM_ION_BP_ADMIN_CTRL_ENDPOINTADD_MID 0xc3360100
#define ADM_ION_BP_ADMIN_CTRL_ENDPOINTCHANGE_MID 0xc3360101
#define ADM_ION_BP_ADMIN_CTRL_ENDPOINTDEL_MID 0xc3360102
#define ADM_ION_BP_ADMIN_CTRL_INDUCTADD_MID 0xc3360103
#define ADM_ION_BP_ADMIN_CTRL_INDUCTCHANGE_MID 0xc3360104
#define ADM_ION_BP_ADMIN_CTRL_INDUCTDEL_MID 0xc3360105
#define ADM_ION_BP_ADMIN_CTRL_INDUCTSTART_MID 0xc3360106
#define ADM_ION_BP_ADMIN_CTRL_INDUCTSTOP_MID 0xc3360107
#define ADM_ION_BP_ADMIN_CTRL_INIT_MID 0x83360108
#define ADM_ION_BP_ADMIN_CTRL_MANAGEHEAPMAX_MID 0xc3360109
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTADD_MID 0xc336010a
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTCHANGE_MID 0xc336010b
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTDEL_MID 0xc336010c
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTSTART_MID 0xc336010d
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTBLOCK_MID 0xc336010e
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTUNBLOCK_MID 0xc336010f
#define ADM_ION_BP_ADMIN_CTRL_OUTDUCTSTOP_MID 0xc3360110
#define ADM_ION_BP_ADMIN_CTRL_PROTOCOLADD_MID 0xc3360111
#define ADM_ION_BP_ADMIN_CTRL_PROTOCOLDEL_MID 0xc3360112
#define ADM_ION_BP_ADMIN_CTRL_PROTOCOLSTART_MID 0xc3360113
#define ADM_ION_BP_ADMIN_CTRL_PROTOCOLSTOP_MID 0xc3360114
#define ADM_ION_BP_ADMIN_CTRL_SCHEMEADD_MID 0xc3360115
#define ADM_ION_BP_ADMIN_CTRL_SCHEMECHANGE_MID 0xc3360116
#define ADM_ION_BP_ADMIN_CTRL_SCHEMEDEL_MID 0xc3360117
#define ADM_ION_BP_ADMIN_CTRL_SCHEMESTART_MID 0xc3360118
#define ADM_ION_BP_ADMIN_CTRL_SCHEMESTOP_MID 0xc3360119
#define ADM_ION_BP_ADMIN_CTRL_START_MID 0x8336011a
#define ADM_ION_BP_ADMIN_CTRL_STOP_MID 0x8336011b
#define ADM_ION_BP_ADMIN_CTRL_WATCH_MID 0xc336011c


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN CONSTANT DEFINITIONS                                                         
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN MACRO DEFINITIONS                                                            
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */


/*
 * +-----------------------------------------------------------------------------------------------------------+
 * |		                    ION_BP_ADMIN OPERATOR DEFINITIONS                                                          
 * +-----------------------------------------------------------------------------------------------------------+
   +-----------------------------+------------+--------------------------------------------------+-------------+
   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |
   +-----------------------------+------------+--------------------------------------------------+-------------+
 */

/* Initialization functions. */
void adm_ion_bp_admin_init();
void adm_ion_bp_admin_init_edd();
void adm_ion_bp_admin_init_variables();
void adm_ion_bp_admin_init_controls();
void adm_ion_bp_admin_init_constants();
void adm_ion_bp_admin_init_macros();
void adm_ion_bp_admin_init_metadata();
void adm_ion_bp_admin_init_ops();
void adm_ion_bp_admin_init_reports();
#endif /* _HAVE_ION_BP_ADMIN_ADM_ */
#endif //ADM_ION_BP_ADMIN_H_