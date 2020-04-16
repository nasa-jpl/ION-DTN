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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/


#ifndef ADM_ION_BP_ADMIN_H_
#define ADM_ION_BP_ADMIN_H_
#define _HAVE_DTN_ION_BPADMIN_ADM_
#ifdef _HAVE_DTN_ION_BPADMIN_ADM_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/ION/bpadmin
 */
#define ADM_ENUM_DTN_ION_BPADMIN 5
/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 AGENT NICKNAME DEFINITIONS                                  +
 * +---------------------------------------------------------------------------------------------+
 */

/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_BPADMIN META-DATA DEFINITIONS                            +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |ion_bp_admin            |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |namespace            |The namespace of the ADM              |STR    |DTN/ION/bpadmin         |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |version              |The version of the ADM                |STR    |v0.0                    |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |organization         |The name of the issuing organization o|       |                        |
 * |                     |f the ADM                             |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_ION_BPADMIN_META_NAME 0x00
// "namespace"
#define DTN_ION_BPADMIN_META_NAMESPACE 0x01
// "version"
#define DTN_ION_BPADMIN_META_VERSION 0x02
// "organization"
#define DTN_ION_BPADMIN_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                     DTN_ION_BPADMIN EXTERNALLY DEFINED DATA DEFINITIONS                     +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |bp_version           |Version of installed ION BP Admin util|       |
 * |                     |ity.                                  |STR    |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_BPADMIN_EDD_BP_VERSION 0x00


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_BPADMIN VARIABLE DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_BPADMIN REPORT DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_ION_BPADMIN TABLE DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |endpoints            |Local endpoints, regardless of scheme |       |
 * |                     |name.                                 |       |
 * +---------------------+--------------------------------------+-------+
 * |inducts              |Inducts established locally for the in|       |
 * |                     |dicated CL protocol.                  |       |
 * +---------------------+--------------------------------------+-------+
 * |outducts             |If protocolName is specified, this tab|       |
 * |                     |le lists all outducts established loca|       |
 * |                     |lly for the indicated CL protocol. Oth|       |
 * |                     |erwise, it lists all locally establish|       |
 * |                     |ed outducts, regardless of their proto|       |
 * |                     |col.                                  |       |
 * +---------------------+--------------------------------------+-------+
 * |protocols            |Convergence layer protocols that can c|       |
 * |                     |urrently be utilized at the local node|       |
 * |                     |.                                     |       |
 * +---------------------+--------------------------------------+-------+
 * |schemes              |Declared endpoint naming schemes.     |       |
 * +---------------------+--------------------------------------+-------+
 * |egress_plans         |Egress plans.                         |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_BPADMIN_TBLT_ENDPOINTS 0x00
#define DTN_ION_BPADMIN_TBLT_INDUCTS 0x01
#define DTN_ION_BPADMIN_TBLT_OUTDUCTS 0x02
#define DTN_ION_BPADMIN_TBLT_PROTOCOLS 0x03
#define DTN_ION_BPADMIN_TBLT_SCHEMES 0x04
#define DTN_ION_BPADMIN_TBLT_EGRESS_PLANS 0x05


/*
 * +---------------------------------------------------------------------------------------------+
 * |                             DTN_ION_BPADMIN CONTROL DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_add         |Establish DTN endpoint named endpointI|       |
 * |                     |d on the local node. The remaining par|       |
 * |                     |ameters indicate what is to be done wh|       |
 * |                     |en bundles destined for this endpoint |       |
 * |                     |arrive at a time when no application h|       |
 * |                     |as the endpoint open for bundle recept|       |
 * |                     |ion. If type is 'x', then such bundles|       |
 * |                     | are to be discarded silently and imme|       |
 * |                     |diately. If type is 'q', then such bun|       |
 * |                     |dles are to be enqueued for later deli|       |
 * |                     |very and, if recvScript is provided, r|       |
 * |                     |ecvScript is to be executed.          |       |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_change      |Change the action taken when bundles d|       |
 * |                     |estined for this endpoint arrive at a |       |
 * |                     |time when no application has the endpo|       |
 * |                     |int open for bundle reception.        |       |
 * +---------------------+--------------------------------------+-------+
 * |endpoint_del         |Delete the endpoint identified by endp|       |
 * |                     |ointId. The control will fail if any b|       |
 * |                     |undles are currently pending delivery |       |
 * |                     |to this endpoint.                     |       |
 * +---------------------+--------------------------------------+-------+
 * |induct_add           |Establish a duct for reception of bund|       |
 * |                     |les via the indicated CL protocol. The|       |
 * |                     | duct's data acquisition structure is |       |
 * |                     |used and populated by the induct task |       |
 * |                     |whose operation is initiated by cliCon|       |
 * |                     |trol at the time the duct is started. |       |
 * +---------------------+--------------------------------------+-------+
 * |induct_change        |Change the control used to initiate op|       |
 * |                     |eration of the induct task for the ind|       |
 * |                     |icated duct.                          |       |
 * +---------------------+--------------------------------------+-------+
 * |induct_del           |Delete the induct identified by protoc|       |
 * |                     |olName and ductName. The control will |       |
 * |                     |fail if any bundles are currently pend|       |
 * |                     |ing acquisition via this induct.      |       |
 * +---------------------+--------------------------------------+-------+
 * |induct_start         |Start the indicated induct task as def|       |
 * |                     |ined for the indicated CL protocol on |       |
 * |                     |the local node.                       |       |
 * +---------------------+--------------------------------------+-------+
 * |induct_stop          |Stop the indicated induct task as defi|       |
 * |                     |ned for the indicated CL protocol on t|       |
 * |                     |he local node.                        |       |
 * +---------------------+--------------------------------------+-------+
 * |manage_heap_max      |Declare the maximum number of bytes of|       |
 * |                     | SDR heap space that will be occupied |       |
 * |                     |by any single bundle acquisition activ|       |
 * |                     |ity (nominally the acquisition of a si|       |
 * |                     |ngle bundle, but this is at the discre|       |
 * |                     |tion of the convergence-layer input ta|       |
 * |                     |sk). All data acquired in excess of th|       |
 * |                     |is limit will be written to a temporar|       |
 * |                     |y file pending extraction and dispatch|       |
 * |                     |ing of the acquired bundle or bundles.|       |
 * |                     | The default is the minimum allowed va|       |
 * |                     |lue (560 bytes), which is the approxim|       |
 * |                     |ate size of a ZCO file reference objec|       |
 * |                     |t; this is the minimum SDR heap space |       |
 * |                     |occupancy in the event that all acquis|       |
 * |                     |ition is into a file.                 |       |
 * +---------------------+--------------------------------------+-------+
 * |outduct_add          |Establish a duct for transmission of b|       |
 * |                     |undles via the indicated CL protocol. |       |
 * |                     |the duct's data transmission structure|       |
 * |                     | is serviced by the outduct task whose|       |
 * |                     | operation is initiated by CLOcommand |       |
 * |                     |at the time the duct is started. A val|       |
 * |                     |ue of zero for maxPayloadLength indica|       |
 * |                     |tes that bundles of any size can be ac|       |
 * |                     |comodated; this is the default.       |       |
 * +---------------------+--------------------------------------+-------+
 * |outduct_change       |Set new values for the indicated duct'|       |
 * |                     |s payload size limit and the control t|       |
 * |                     |hat is used to initiate operation of t|       |
 * |                     |he outduct task for this duct.        |       |
 * +---------------------+--------------------------------------+-------+
 * |outduct_del          |Delete the outduct identified by proto|       |
 * |                     |colName and ductName. The control will|       |
 * |                     | fail if any bundles are currently pen|       |
 * |                     |ding transmission via this outduct.   |       |
 * +---------------------+--------------------------------------+-------+
 * |outduct_start        |Start the indicated outduct task as de|       |
 * |                     |fined for the indicated CL protocol on|       |
 * |                     | the local node.                      |       |
 * +---------------------+--------------------------------------+-------+
 * |egress_plan_block    |Disable transmission of bundles queued|       |
 * |                     | for transmission to the indicated nod|       |
 * |                     |e and reforwards all non-critical bund|       |
 * |                     |les currently queued for transmission |       |
 * |                     |to this node. This may result in some |       |
 * |                     |or all of these bundles being enqueued|       |
 * |                     | for transmission to the psuedo-node l|       |
 * |                     |imbo.                                 |       |
 * +---------------------+--------------------------------------+-------+
 * |egress_plan_unblock  |Re-enable transmission of bundles to t|       |
 * |                     |he indicated node and reforwards all b|       |
 * |                     |undles in limbo in the hope that the u|       |
 * |                     |nblocking of this egress plan will ena|       |
 * |                     |ble some of them to be transmitted.   |       |
 * +---------------------+--------------------------------------+-------+
 * |outduct_stop         |Stop the indicated outduct task as def|       |
 * |                     |ined for the indicated CL protocol on |       |
 * |                     |the local node.                       |       |
 * +---------------------+--------------------------------------+-------+
 * |protocol_add         |Establish access to the named converge|       |
 * |                     |nce layer protocol at the local node. |       |
 * |                     |The payloadBytesPerFrame and overheadB|       |
 * |                     |ytesPerFrame arguments are used in cal|       |
 * |                     |culating the estimated transmission ca|       |
 * |                     |pacity consumption of each bundle, to |       |
 * |                     |aid in route computation and congestin|       |
 * |                     |g forecasting. The optional nominalDat|       |
 * |                     |aRate argument overrides the hard code|       |
 * |                     |d default continuous data rate for the|       |
 * |                     | indicated protocol for purposes of ra|       |
 * |                     |te control. For all promiscuous protot|       |
 * |                     |ocols-that is, protocols whose outduct|       |
 * |                     |s are not specifically dedicated to tr|       |
 * |                     |ansmission to a single identified conv|       |
 * |                     |ergence-layer protocol endpoint- the p|       |
 * |                     |rotocol's applicable nominal continuou|       |
 * |                     |s data rate is the data rate that is a|       |
 * |                     |lways used for rate control over links|       |
 * |                     | served by that protocol; data rates a|       |
 * |                     |re not extracted from contact graph in|       |
 * |                     |formation. This is because only the in|       |
 * |                     |duct and outduct throttles for non-pro|       |
 * |                     |miscuous protocols (LTP, TCP) can be d|       |
 * |                     |ynamically adjusted in response to cha|       |
 * |                     |nges in data rate between the local no|       |
 * |                     |de and its neighbors, as enacted per t|       |
 * |                     |he contact plan. Even for an outduct o|       |
 * |                     |f a non-promiscuous protocol the nomin|       |
 * |                     |al data rate may be the authority for |       |
 * |                     |rate control, in the event that the co|       |
 * |                     |ntact plan lacks identified contacts w|       |
 * |                     |ith the node to which the outduct is m|       |
 * |                     |apped.                                |       |
 * +---------------------+--------------------------------------+-------+
 * |protocol_del         |Delete the convergence layer protocol |       |
 * |                     |identified by protocolName. The contro|       |
 * |                     |l will fail if any ducts are still loc|       |
 * |                     |ally declared for this protocol.      |       |
 * +---------------------+--------------------------------------+-------+
 * |protocol_start       |Start all induct and outduct tasks for|       |
 * |                     | inducts and outducts that have been d|       |
 * |                     |efined for the indicated CL protocol o|       |
 * |                     |n the local node.                     |       |
 * +---------------------+--------------------------------------+-------+
 * |protocol_stop        |Stop all induct and outduct tasks for |       |
 * |                     |inducts and outducts that have been de|       |
 * |                     |fined for the indicated CL protocol on|       |
 * |                     | the local node.                      |       |
 * +---------------------+--------------------------------------+-------+
 * |scheme_add           |Declares an endpoint naming scheme for|       |
 * |                     | use in endpoint IDs, which are struct|       |
 * |                     |ured as URIs: schemeName:schemeSpecifi|       |
 * |                     |cPart. forwarderControl will be execut|       |
 * |                     |ed when the scheme is started on this |       |
 * |                     |node, to initiate operation of a forwa|       |
 * |                     |rding daemon for this scheme. adminApp|       |
 * |                     |Control will also be executed when the|       |
 * |                     | scheme is started on this node, to in|       |
 * |                     |itiate operation of a daemon that open|       |
 * |                     |s a custodian endpoint identified with|       |
 * |                     |in this scheme so that it can recieve |       |
 * |                     |and process custody signals and bundle|       |
 * |                     | status reports.                      |       |
 * +---------------------+--------------------------------------+-------+
 * |scheme_change        |Set the indicated scheme's forwarderCo|       |
 * |                     |ntrol and adminAppControl to the strin|       |
 * |                     |gs provided as arguments.             |       |
 * +---------------------+--------------------------------------+-------+
 * |scheme_del           |Delete the scheme identified by scheme|       |
 * |                     |Name. The control will fail if any bun|       |
 * |                     |dles identified in this scheme are pen|       |
 * |                     |ding forwarding, transmission, or deli|       |
 * |                     |very.                                 |       |
 * +---------------------+--------------------------------------+-------+
 * |scheme_start         |Start the forwarder and administrative|       |
 * |                     | endpoint tasks for the indicated sche|       |
 * |                     |me task on the local node.            |       |
 * +---------------------+--------------------------------------+-------+
 * |scheme_stop          |Stop the forwarder and administrative |       |
 * |                     |endpoint tasks for the indicated schem|       |
 * |                     |e task on the local node.             |       |
 * +---------------------+--------------------------------------+-------+
 * |watch                |Enable/Disable production of a continu|       |
 * |                     |ous stream of user selected Bundle Pro|       |
 * |                     |tocol activity indication characters. |       |
 * |                     |A watch parameter of 1 selects all BP |       |
 * |                     |activity indication characters, 0 dese|       |
 * |                     |lects allBP activity indication charac|       |
 * |                     |ters; any other activitySpec such as a|       |
 * |                     |cz~ selects all activity indication ch|       |
 * |                     |aracters in the string, deselecting al|       |
 * |                     |l others. BP will print each selected |       |
 * |                     |activity indication character to stdou|       |
 * |                     |t every time a processing event of the|       |
 * |                     | associated type occurs: a new bundle |       |
 * |                     |is queued for forwarding, b bundle is |       |
 * |                     |queued for transmission, c bundle is p|       |
 * |                     |opped from its transmission queue, m c|       |
 * |                     |ustody acceptance signal is recieved, |       |
 * |                     |w custody of bundle is accepted, x cus|       |
 * |                     |tody of bundle is refused, y bundle is|       |
 * |                     | accepted upon arrival, z bundle is qu|       |
 * |                     |eued for delivery to an application, ~|       |
 * |                     | bundle is abandoned (discarded) on at|       |
 * |                     |tempt to forward it, ! bundle is destr|       |
 * |                     |oyed due to TTL expiration, &amp; cust|       |
 * |                     |ody refusal signal is recieved, # bund|       |
 * |                     |le is queued for re-forwarding due to |       |
 * |                     |CL protocol failures, j bundle is plac|       |
 * |                     |ed in 'limbo' for possible future refo|       |
 * |                     |rwarding, k bundle is removed from 'li|       |
 * |                     |mbo' and queued for reforwarding, $ bu|       |
 * |                     |ndle's custodial retransmission timeou|       |
 * |                     |t interval expired.                   |       |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_ION_BPADMIN_CTRL_ENDPOINT_ADD 0x00
#define DTN_ION_BPADMIN_CTRL_ENDPOINT_CHANGE 0x01
#define DTN_ION_BPADMIN_CTRL_ENDPOINT_DEL 0x02
#define DTN_ION_BPADMIN_CTRL_INDUCT_ADD 0x03
#define DTN_ION_BPADMIN_CTRL_INDUCT_CHANGE 0x04
#define DTN_ION_BPADMIN_CTRL_INDUCT_DEL 0x05
#define DTN_ION_BPADMIN_CTRL_INDUCT_START 0x06
#define DTN_ION_BPADMIN_CTRL_INDUCT_STOP 0x07
#define DTN_ION_BPADMIN_CTRL_MANAGE_HEAP_MAX 0x08
#define DTN_ION_BPADMIN_CTRL_OUTDUCT_ADD 0x09
#define DTN_ION_BPADMIN_CTRL_OUTDUCT_CHANGE 0x0a
#define DTN_ION_BPADMIN_CTRL_OUTDUCT_DEL 0x0b
#define DTN_ION_BPADMIN_CTRL_OUTDUCT_START 0x0c
#define DTN_ION_BPADMIN_CTRL_EGRESS_PLAN_BLOCK 0x0d
#define DTN_ION_BPADMIN_CTRL_EGRESS_PLAN_UNBLOCK 0x0e
#define DTN_ION_BPADMIN_CTRL_OUTDUCT_STOP 0x0f
#define DTN_ION_BPADMIN_CTRL_PROTOCOL_ADD 0x10
#define DTN_ION_BPADMIN_CTRL_PROTOCOL_DEL 0x11
#define DTN_ION_BPADMIN_CTRL_PROTOCOL_START 0x12
#define DTN_ION_BPADMIN_CTRL_PROTOCOL_STOP 0x13
#define DTN_ION_BPADMIN_CTRL_SCHEME_ADD 0x14
#define DTN_ION_BPADMIN_CTRL_SCHEME_CHANGE 0x15
#define DTN_ION_BPADMIN_CTRL_SCHEME_DEL 0x16
#define DTN_ION_BPADMIN_CTRL_SCHEME_START 0x17
#define DTN_ION_BPADMIN_CTRL_SCHEME_STOP 0x18
#define DTN_ION_BPADMIN_CTRL_WATCH 0x19


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_BPADMIN CONSTANT DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_ION_BPADMIN MACRO DEFINITIONS                              +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */


/*
 * +---------------------------------------------------------------------------------------------+
 * |                            DTN_ION_BPADMIN OPERATOR DEFINITIONS                             +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 */

/* Initialization functions. */
void dtn_ion_bpadmin_init();
void dtn_ion_bpadmin_init_meta();
void dtn_ion_bpadmin_init_cnst();
void dtn_ion_bpadmin_init_edd();
void dtn_ion_bpadmin_init_op();
void dtn_ion_bpadmin_init_var();
void dtn_ion_bpadmin_init_ctrl();
void dtn_ion_bpadmin_init_mac();
void dtn_ion_bpadmin_init_rpttpl();
void dtn_ion_bpadmin_init_tblt();
#endif /* _HAVE_DTN_ION_BPADMIN_ADM_ */
#endif //ADM_ION_BP_ADMIN_H_