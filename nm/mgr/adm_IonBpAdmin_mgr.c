/****************************************************************************
 **
 ** File Name: ./mgr/adm_IonBpAdmin_mgr.c
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
 **  2017-11-11  AUTO           Auto generated c file 
 **
****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_IonBpAdmin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"
#define _HAVE_IONBPADMIN_ADM_
#ifdef _HAVE_IONBPADMIN_ADM_
void adm_IonBpAdmin_init()
{
	adm_IonBpAdmin_init_edd();
	adm_IonBpAdmin_init_variables();
	adm_IonBpAdmin_init_controls();
	adm_IonBpAdmin_init_constants();
	adm_IonBpAdmin_init_macros();
	adm_IonBpAdmin_init_metadata();
	adm_IonBpAdmin_init_ops();
	adm_IonBpAdmin_init_reports();
}

void adm_IonBpAdmin_init_edd()
{
	adm_add_edd(ADM_IONBPADMIN_EDD_VERSION_MID, AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("ADM_IONBPADMIN_EDD_VERSION_MID", "This is the version of ion that is currently installed and the crypto suite that BP was compiled with.", ADM_IONBPADMIN, ADM_IONBPADMIN_EDD_VERSION_MID);

}

void adm_IonBpAdmin_init_variables()
{
}

void adm_IonBpAdmin_init_controls()
{
	adm_add_ctrl(ADM_IONBPADMIN_CTRL_ENDPOINTADD_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_ENDPOINTADD_MID", "This control establishes a DTN endpoint named endpointId on the local node. The remaining parameters indicate what is to be done when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception. If type is 0, then such bundles are to be discarded silently and immediately. If type is 1, then such bundles are to be enqueued for later delivery and, if recvScript is provided, recvScript is to be executed.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_ENDPOINTADD_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_ENDPOINTADD_MID, "endpointId", AMP_TYPE_STR, "rule", AMP_TYPE_UINT, "rcvScript", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_ENDPOINTCHANGE_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_ENDPOINTCHANGE_MID", "This control changes the action that is to be taken when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_ENDPOINTCHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_ENDPOINTCHANGE_MID, "endpointId", AMP_TYPE_STR, "type", AMP_TYPE_UINT, "rcvScript", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_ENDPOINTDEL_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_ENDPOINTDEL_MID", "This control deletes the endpoint identified by endpointId. The control will fail if any bundles are currently pending delivery to this endpoint.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_ENDPOINTDEL_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_ENDPOINTDEL_MID, "endpointId", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTADD_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_INDUCTADD_MID", "This control establishes a duct for reception of bundles via the indicated CL protocol. The duct's data acquisition structure is used and populated by the induct task whose operation is initiated by cliControl at the time the duct is started.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_INDUCTADD_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_INDUCTADD_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR, "cliControl", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTCHANGE_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_INDUCTCHANGE_MID", "This changes the control that is used to initiate operation of the induct task for the indicated duct.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_INDUCTCHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_INDUCTCHANGE_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR, "cliControl", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTDEL_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_INDUCTDEL_MID", "This control deletes the induct identified by protocolName and ductName. The control will fail if any bundles are currently pending acquisition via this induct.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_INDUCTDEL_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_INDUCTDEL_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTSTART_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_INDUCTSTART_MID", "This control starts the indicated induct task as defined for the indicated CL protocol on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_INDUCTSTART_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_INDUCTSTART_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_INDUCTSTOP_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_INDUCTSTOP_MID", "This control stops the indicated induct task as defined for the indicated CL protocol on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_INDUCTSTOP_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_INDUCTSTOP_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_MANAGEHEAPMAX_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_MANAGEHEAPMAX_MID", "This control declares the maximum number of bytes of SDR heap space that will be occupied by any single bundle acquisition activity (nominally the acquisition of a single bundle, but this is at the discretion of the convergence-layer input task). All data acquired in excess of this limit will be written to a temporary file pending extraction and dispatching of the acquired bundle or bundles. The default is the minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimum SDR heap space occupancy in the event that all acquisition is into a file.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_MANAGEHEAPMAX_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_MANAGEHEAPMAX_MID, "maxDatabaseHeapPerAcquisition", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTADD_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_OUTDUCTADD_MID", "This control establishes a duct for transmission of bundles via the indicated CL protocol. the duct's data transmission structure is serviced by the outduct task whose operation is initiated by CLOcommand at the time the duct is started. A value of zero for maxPayloadLength indicates that bundles of any size can be accomodated;this is the default.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_OUTDUCTADD_MID);
	UI_ADD_PARMSPEC_4(ADM_IONBPADMIN_CTRL_OUTDUCTADD_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR, "cloCommand", AMP_TYPE_STR, "maxPayloadLength", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTCHANGE_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_OUTDUCTCHANGE_MID", "This control sets new values for the indicated duct's payload size limit and the control that is used to initiate operation of the outduct task for this duct.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_OUTDUCTCHANGE_MID);
	UI_ADD_PARMSPEC_4(ADM_IONBPADMIN_CTRL_OUTDUCTCHANGE_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR, "cloControl", AMP_TYPE_STR, "maxPayloadLength", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTDEL_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_OUTDUCTDEL_MID", "This control deletes the outduct identified by protocolName and ductName. The control will fail if any bundles are currently pending transmission via this outduct.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_OUTDUCTDEL_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_OUTDUCTDEL_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTSTART_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_OUTDUCTSTART_MID", "This control starts the indicated outduct task as defined for the indicated CL protocol on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_OUTDUCTSTART_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_OUTDUCTSTART_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_OUTDUCTSTOP_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_OUTDUCTSTOP_MID", "This control stops the indicated outduct task as defined for the indicated CL protocol on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_OUTDUCTSTOP_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_OUTDUCTSTOP_MID, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLADD_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_PROTOCOLADD_MID", "This control establishes access to the named convergence layer protocol at the local node. The payloadBytesPerFrame and overheadBytesPerFrame arguments are used in calculating the estimated transmission capacity consumption of each bundle, to aid in route computation and congesting forecasting. The optional nominalDataRate argument overrides the hard coded default continuous data rate for the indicated protocol for purposes of rate control. For all promiscuous prototocols-that is, protocols whose outducts are not specifically dedicated to transmission to a single identified convergence-layer protocol endpoint- the protocol's applicable nominal continuous data rate is the data rate that is always used for rate control over links served by that protocol; data rates are not extracted from contact graph information. This is because only the induct and outduct throttles for non-promiscuous protocols (LTP, TCP) can be dynamically adjusted in response to changes in data rate between the local node and its neighbors, as enacted per the contact plan. Even for an outduct of a non-promiscuous protocol the nominal data rate may be the authority for rate control, in the event that the contact plan lacks identified contacts with the node to which the outduct is mapped.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_PROTOCOLADD_MID);
	UI_ADD_PARMSPEC_4(ADM_IONBPADMIN_CTRL_PROTOCOLADD_MID, "protocolName", AMP_TYPE_STR, "payloadBytesPerFrame", AMP_TYPE_UINT, "overheadBytesPerFrame", AMP_TYPE_UINT, "nominalDataRate", AMP_TYPE_UINT);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLDEL_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_PROTOCOLDEL_MID", "This control deletes the convergence layer protocol identified by protocolName. The control will fail if any ducts are still locally declared for this protocol.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_PROTOCOLDEL_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_PROTOCOLDEL_MID, "protocolName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLSTART_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_PROTOCOLSTART_MID", "This control starts all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_PROTOCOLSTART_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_PROTOCOLSTART_MID, "protocolName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_PROTOCOLSTOP_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_PROTOCOLSTOP_MID", "This control stops all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_PROTOCOLSTOP_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_PROTOCOLSTOP_MID, "protocolName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMEADD_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_SCHEMEADD_MID", "This control declares an endpoint naming scheme for use in endpoint IDs, which are structured as URIs: schemeName:schemeSpecificPart. forwarderControl will be executed when the scheme is started on this node, to initiate operation of a forwarding daemon for this scheme. adminAppControl will also be executed when the scheme is started on this node, to initiate operation of a daemon that opens a custodian endpoint identified within this scheme so that it can recieve and process custody signals and bundle status reports.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_SCHEMEADD_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_SCHEMEADD_MID, "schemeName", AMP_TYPE_STR, "forwarderControl", AMP_TYPE_STR, "adminAppControl", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMECHANGE_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_SCHEMECHANGE_MID", "This control sets the indicated scheme's forwarderControl and adminAppControl to the strings provided as arguments.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_SCHEMECHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_SCHEMECHANGE_MID, "schemeName", AMP_TYPE_STR, "forwarderControl", AMP_TYPE_STR, "adminAppControl", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMEDEL_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_SCHEMEDEL_MID", "This control deletes the scheme identified by schemeName. The control will fail if any bundles identified in this scheme are pending forwarding, transmission, or delivery.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_SCHEMEDEL_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_SCHEMEDEL_MID, "schemeName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMESTART_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_SCHEMESTART_MID", "This control starts the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_SCHEMESTART_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_SCHEMESTART_MID, "schemeName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_SCHEMESTOP_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_SCHEMESTOP_MID", "This control stops the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_SCHEMESTOP_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_SCHEMESTOP_MID, "schemeName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANADD_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANADD_MID", "This command establishes an egress plan governing transmission to the neighboring node[s] identified by endpoint_name.  The plan is functionally enacted by a bpclm(1) daemon dedicated to managing bundles queued for transmission to the indicated neighboring node[s].", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANADD_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_EGRESSPLANADD_MID, "endpointName", AMP_TYPE_STR, "xmitRate", AMP_TYPE_REAL32);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANUPDATE_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANUPDATE_MID", "This command sets a new value for the indicated plan's transmission rate.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANUPDATE_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_EGRESSPLANUPDATE_MID, "endpointName", AMP_TYPE_STR, "xmitRate", AMP_TYPE_REAL32);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANDELETE_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANDELETE_MID", "This command deletes the outduct identified by endpoint_name.  The command will fail if any bundles are currently pending transmission per this plan.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANDELETE_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_EGRESSPLANDELETE_MID, "endpointName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANSTART_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANSTART_MID", "This command starts the bpclm task for the indicated egress plan.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANSTART_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_EGRESSPLANSTART_MID, "endpointName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANSTOP_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANSTOP_MID", "This command stops the bpclm task for the indicated egress plan.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANSTOP_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_EGRESSPLANSTOP_MID, "endpointName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANBLOCK_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANBLOCK_MID", "This command disables transmission of bundles queued for transmission to the indicated node and reforwards all non-critical bundles currently queued for transmission to this node.  This may result in some or all of these bundles being enqueued for transmission (actually just retention) to the pseudo-node limbo", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANBLOCK_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_EGRESSPLANBLOCK_MID, "endpointName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANUNBLOCK_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANUNBLOCK_MID", "This command re-enables transmission of bundles to the indicated node and reforwards all bundles in limbo in the hope that the unblocking of this egress plan will enable some of them to be transmitted.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANUNBLOCK_MID);
	UI_ADD_PARMSPEC_1(ADM_IONBPADMIN_CTRL_EGRESSPLANUNBLOCK_MID, "endpointName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANGATEWAY_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANGATEWAY_MID", "This command declares the name of the endpoint to which bundles queued for transmission to the node[s] identified byendpoint_name must be re-routed.  Declaring gateway_endpoint_name to be the zero-length string '' disables re-routing: bundles will instead be transmitted using the plan's attached convergence-layer protocol outduct[s].", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANGATEWAY_MID);
	UI_ADD_PARMSPEC_2(ADM_IONBPADMIN_CTRL_EGRESSPLANGATEWAY_MID, "endpointName", AMP_TYPE_STR, "gatewayEndpointName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTATTACH_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTATTACH_MID", "This command declares that the indicated convergence-layer protocol outduct is now a viable device for transmitting bundles to the node[s] identified by endpoint_name.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTATTACH_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTATTACH_MID, "endpointName", AMP_TYPE_STR, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);

	adm_add_ctrl(ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTDETATCH_MID, NULL);
	names_add_name("ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTDETATCH_MID", "This command declares that the indicated convergence-layer protocol outduct is no longer a viable device for transmitting bundles to the node[s] identified by endpoint_name.", ADM_IONBPADMIN, ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTDETATCH_MID);
	UI_ADD_PARMSPEC_3(ADM_IONBPADMIN_CTRL_EGRESSPLANOUTDUCTDETATCH_MID, "endpointName", AMP_TYPE_STR, "protocolName", AMP_TYPE_STR, "ductName", AMP_TYPE_STR);


}

void adm_IonBpAdmin_init_constants()
{

}

void adm_IonBpAdmin_init_macros()
{

}

void adm_IonBpAdmin_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(IONBPADMIN_ADM_META_NN_IDX, IONBPADMIN_ADM_META_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_EDD_NN_IDX, IONBPADMIN_ADM_EDD_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_VAR_NN_IDX, IONBPADMIN_ADM_VAR_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_RPT_NN_IDX, IONBPADMIN_ADM_RPT_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_CTRL_NN_IDX, IONBPADMIN_ADM_CTRL_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_CONST_NN_IDX, IONBPADMIN_ADM_CONST_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_MACRO_NN_IDX, IONBPADMIN_ADM_MACRO_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_OP_NN_IDX, IONBPADMIN_ADM_OP_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_TBL_NN_IDX, IONBPADMIN_ADM_TBL_NN_STR, "IONBPADMIN", "2017-08-17");
	oid_nn_add_parm(IONBPADMIN_ADM_ROOT_NN_IDX, IONBPADMIN_ADM_ROOT_NN_STR, "IONBPADMIN", "2017-08-17");
	/* Step 2: Register Metadata Information. */
	adm_add_edd(ADM_IONBPADMIN_META_NAME_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONBPADMIN_META_NAME_MID", "The human-readable name of the ADM.", ADM_IONBPADMIN, ADM_IONBPADMIN_META_NAME_MID);
	adm_add_edd(ADM_IONBPADMIN_META_NAMESPACE_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONBPADMIN_META_NAMESPACE_MID", "The namespace of the ADM", ADM_IONBPADMIN, ADM_IONBPADMIN_META_NAMESPACE_MID);
	adm_add_edd(ADM_IONBPADMIN_META_VERSION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONBPADMIN_META_VERSION_MID", "The version of the ADM", ADM_IONBPADMIN, ADM_IONBPADMIN_META_VERSION_MID);
	adm_add_edd(ADM_IONBPADMIN_META_ORGANIZATION_MID, AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ADM_IONBPADMIN_META_ORGANIZATION_MID", "The name of the issuing organization of the ADM", ADM_IONBPADMIN, ADM_IONBPADMIN_META_ORGANIZATION_MID);

}

void adm_IonBpAdmin_init_ops()
{

}

void adm_IonBpAdmin_init_reports()
{
	uint32_t used= 0;
}

#endif // _HAVE_IONBPADMIN_ADM_
