/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin_mgr.c
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
 **  2018-01-08  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_ion_bp_admin.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "nm_mgr_names.h"
#include "nm_mgr_ui.h"

#define _HAVE_ION_BP_ADMIN_ADM_
#ifdef _HAVE_ION_BP_ADMIN_ADM_

void adm_ion_bp_admin_init()
{
	adm_ion_bp_admin_init_edd();
	adm_ion_bp_admin_init_variables();
	adm_ion_bp_admin_init_controls();
	adm_ion_bp_admin_init_constants();
	adm_ion_bp_admin_init_macros();
	adm_ion_bp_admin_init_metadata();
	adm_ion_bp_admin_init_ops();
	adm_ion_bp_admin_init_reports();
}


void adm_ion_bp_admin_init_edd()
{
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_EDD_VERSION_MID), AMP_TYPE_STR, 0, NULL, NULL, NULL);
	names_add_name("VERSION", "Version of installed ION BP Admin utility.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_EDD_VERSION_MID);

}


void adm_ion_bp_admin_init_variables()
{
}


void adm_ion_bp_admin_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_ADD_MID), NULL);
	names_add_name("ENDPOINT_ADD", "Establish DTN endpoint named endpointId on the local node. The remaining parameters indicate what is to be done when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception. If type is 'x', then such bundles are to be discarded silently and immediately. If type is 'q', then such bundles are to be enqueued for later delivery and, if recvScript is provided, recvScript is to be executed.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_ENDPOINT_ADD_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_ADD_MID, "endpoint_id", AMP_TYPE_STR, "type", AMP_TYPE_UINT, "rcv_script", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_CHANGE_MID), NULL);
	names_add_name("ENDPOINT_CHANGE", "Change the action taken when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_ENDPOINT_CHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_CHANGE_MID, "endpoint_id", AMP_TYPE_STR, "type", AMP_TYPE_UINT, "rcv_script", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_DEL_MID), NULL);
	names_add_name("ENDPOINT_DEL", "Delete the endpoint identified by endpointId. The control will fail if any bundles are currently pending delivery to this endpoint.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_ENDPOINT_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_ENDPOINT_DEL_MID, "endpoint_id", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_ADD_MID), NULL);
	names_add_name("INDUCT_ADD", "Establish a duct for reception of bundles via the indicated CL protocol. The duct's data acquisition structure is used and populated by the induct task whose operation is initiated by cliControl at the time the duct is started.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_INDUCT_ADD_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_BP_ADMIN_CTRL_INDUCT_ADD_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR, "cli_control", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_CHANGE_MID), NULL);
	names_add_name("INDUCT_CHANGE", "Change the control used to initiate operation of the induct task for the indicated duct.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_INDUCT_CHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_BP_ADMIN_CTRL_INDUCT_CHANGE_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR, "cli_control", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_DEL_MID), NULL);
	names_add_name("INDUCT_DEL", "Delete the induct identified by protocolName and ductName. The control will fail if any bundles are currently pending acquisition via this induct.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_INDUCT_DEL_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_INDUCT_DEL_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_START_MID), NULL);
	names_add_name("INDUCT_START", "Start the indicated induct task as defined for the indicated CL protocol on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_INDUCT_START_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_INDUCT_START_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INDUCT_STOP_MID), NULL);
	names_add_name("INDUCT_STOP", "Stop the indicated induct task as defined for the indicated CL protocol on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_INDUCT_STOP_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_INDUCT_STOP_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_INIT_MID), NULL);
	names_add_name("INIT", "Until this control is executed, Bundle Protocol is not in operation on the local ION node and most bpadmin controls will fail.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_INIT_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_MANAGE_HEAP_MAX_MID), NULL);
	names_add_name("MANAGE_HEAP_MAX", "Declare the maximum number of bytes of SDR heap space that will be occupied by any single bundle acquisition activity (nominally the acquisition of a single bundle, but this is at the discretion of the convergence-layer input task). All data acquired in excess of this limit will be written to a temporary file pending extraction and dispatching of the acquired bundle or bundles. The default is the minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimum SDR heap space occupancy in the event that all acquisition is into a file.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_MANAGE_HEAP_MAX_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_MANAGE_HEAP_MAX_MID, "max_database_heap_per_acquisition", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_ADD_MID), NULL);
	names_add_name("OUTDUCT_ADD", "Establish a duct for transmission of bundles via the indicated CL protocol. the duct's data transmission structure is serviced by the outduct task whose operation is initiated by CLOcommand at the time the duct is started. A value of zero for maxPayloadLength indicates that bundles of any size can be accomodated;this is the default.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_ADD_MID);
	UI_ADD_PARMSPEC_4(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_ADD_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR, "clo_command", AMP_TYPE_STR, "max_payload_length", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_CHANGE_MID), NULL);
	names_add_name("OUTDUCT_CHANGE", "Set new values for the indicated duct's payload size limit and the control that is used to initiate operation of the outduct task for this duct.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_CHANGE_MID);
	UI_ADD_PARMSPEC_4(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_CHANGE_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR, "clo_control", AMP_TYPE_STR, "max_payload_length", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_DEL_MID), NULL);
	names_add_name("OUTDUCT_DEL", "Delete the outduct identified by protocolName and ductName. The control will fail if any bundles are currently pending transmission via this outduct.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_DEL_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_DEL_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_START_MID), NULL);
	names_add_name("OUTDUCT_START", "Start the indicated outduct task as defined for the indicated CL protocol on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_START_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_START_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_BLOCK_MID), NULL);
	names_add_name("OUTDUCT_BLOCK", "Disable transmission of bundles queued for transmission to the indicated node and reforwards all non-critical bundles currently queued for transmission to this node. This may result in some or all of these bundles being enqueued for transmission to the psuedo-node limbo.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_BLOCK_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_BLOCK_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_UNBLOCK_MID), NULL);
	names_add_name("OUTDUCT_UNBLOCK", "Re-enable transmission of bundles to the indicated node and reforwards all bundles in limbo in the hope that the unblocking of this egress plan will enable some of them to be transmitted.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_UNBLOCK_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_UNBLOCK_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_STOP_MID), NULL);
	names_add_name("OUTDUCT_STOP", "Stop the indicated outduct task as defined for the indicated CL protocol on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_OUTDUCT_STOP_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_OUTDUCT_STOP_MID, "protocol_name", AMP_TYPE_STR, "duct_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_ADD_MID), NULL);
	names_add_name("PROTOCOL_ADD", "Establish access to the named convergence layer protocol at the local node. The payloadBytesPerFrame and overheadBytesPerFrame arguments are used in calculating the estimated transmission capacity consumption of each bundle, to aid in route computation and congesting forecasting. The optional nominalDataRate argument overrides the hard coded default continuous data rate for the indicated protocol for purposes of rate control. For all promiscuous prototocols-that is, protocols whose outducts are not specifically dedicated to transmission to a single identified convergence-layer protocol endpoint- the protocol's applicable nominal continuous data rate is the data rate that is always used for rate control over links served by that protocol; data rates are not extracted from contact graph information. This is because only the induct and outduct throttles for non-promiscuous protocols (LTP, TCP) can be dynamically adjusted in response to changes in data rate between the local node and its neighbors, as enacted per the contact plan. Even for an outduct of a non-promiscuous protocol the nominal data rate may be the authority for rate control, in the event that the contact plan lacks identified contacts with the node to which the outduct is mapped.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_PROTOCOL_ADD_MID);
	UI_ADD_PARMSPEC_4(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_ADD_MID, "protocol_name", AMP_TYPE_STR, "payload_bytes_per_frame", AMP_TYPE_UINT, "overhead_bytes_per_frame", AMP_TYPE_UINT, "nominal_data_rate", AMP_TYPE_UINT);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_DEL_MID), NULL);
	names_add_name("PROTOCOL_DEL", "Delete the convergence layer protocol identified by protocolName. The control will fail if any ducts are still locally declared for this protocol.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_PROTOCOL_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_DEL_MID, "protocol_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_START_MID), NULL);
	names_add_name("PROTOCOL_START", "Start all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_PROTOCOL_START_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_START_MID, "protocol_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_STOP_MID), NULL);
	names_add_name("PROTOCOL_STOP", "Stop all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_PROTOCOL_STOP_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_PROTOCOL_STOP_MID, "protocol_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_ADD_MID), NULL);
	names_add_name("SCHEME_ADD", "Declares an endpoint naming scheme for use in endpoint IDs, which are structured as URIs: schemeName:schemeSpecificPart. forwarderControl will be executed when the scheme is started on this node, to initiate operation of a forwarding daemon for this scheme. adminAppControl will also be executed when the scheme is started on this node, to initiate operation of a daemon that opens a custodian endpoint identified within this scheme so that it can recieve and process custody signals and bundle status reports.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_SCHEME_ADD_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_BP_ADMIN_CTRL_SCHEME_ADD_MID, "scheme_name", AMP_TYPE_STR, "forwarder_control", AMP_TYPE_STR, "admin_app_control", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_CHANGE_MID), NULL);
	names_add_name("SCHEME_CHANGE", "Set the indicated scheme's forwarderControl and adminAppControl to the strings provided as arguments.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_SCHEME_CHANGE_MID);
	UI_ADD_PARMSPEC_3(ADM_ION_BP_ADMIN_CTRL_SCHEME_CHANGE_MID, "scheme_name", AMP_TYPE_STR, "forwarder_control", AMP_TYPE_STR, "admin_app_control", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_DEL_MID), NULL);
	names_add_name("SCHEME_DEL", "Delete the scheme identified by schemeName. The control will fail if any bundles identified in this scheme are pending forwarding, transmission, or delivery.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_SCHEME_DEL_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_SCHEME_DEL_MID, "scheme_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_START_MID), NULL);
	names_add_name("SCHEME_START", "Start the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_SCHEME_START_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_SCHEME_START_MID, "scheme_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_SCHEME_STOP_MID), NULL);
	names_add_name("SCHEME_STOP", "Stop the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_SCHEME_STOP_MID);
	UI_ADD_PARMSPEC_1(ADM_ION_BP_ADMIN_CTRL_SCHEME_STOP_MID, "scheme_name", AMP_TYPE_STR);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_START_MID), NULL);
	names_add_name("START", "Start all schemes and all protocols on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_START_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_STOP_MID), NULL);
	names_add_name("STOP", "Stop all schemes and all protocols on the local node.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_STOP_MID);

	adm_add_ctrl(mid_from_value(ADM_ION_BP_ADMIN_CTRL_WATCH_MID), NULL);
	names_add_name("WATCH", "Enable/Disable production of a continuous stream of user selected Bundle Protocol activity indication characters. A watch parameter of 1 selects all BP activity indication characters, 0 deselects allBP activity indication characters; any other activitySpec such as acz~ selects all activity indication characters in the string, deselecting all others. BP will print each selected activity indication character to stdout every time a processing event of the associated type occurs: a new bundle is queued for forwarding, b bundle is queued for transmission, c bundle is popped from its transmission queue, m custody acceptance signal is recieved, w custody of bundle is accepted, x custody of bundle is refused, y bundle is accepted upon arrival, z bundle is queued for delivery to an application, ~ bundle is abandoned (discarded) on attempt to forward it, ! bundle is destroyed due to TTL expiration, & custody refusal signal is recieved, # bundle is queued for re-forwarding due to CL protocol failures, j bundle is placed in 'limbo' for possible future reforwarding, k bundle is removed from 'limbo' and queued for reforwarding, $ bundle's custodial retransmission timeout interval expired. ", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_CTRL_WATCH_MID);
	UI_ADD_PARMSPEC_2(ADM_ION_BP_ADMIN_CTRL_WATCH_MID, "status", AMP_TYPE_UINT, "activity_spec", AMP_TYPE_UINT);

}


void adm_ion_bp_admin_init_constants()
{
}


void adm_ion_bp_admin_init_macros()
{
}


void adm_ion_bp_admin_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(ION_BP_ADMIN_ADM_META_NN_IDX, ION_BP_ADMIN_ADM_META_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_EDD_NN_IDX, ION_BP_ADMIN_ADM_EDD_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_VAR_NN_IDX, ION_BP_ADMIN_ADM_VAR_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_RPT_NN_IDX, ION_BP_ADMIN_ADM_RPT_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_CTRL_NN_IDX, ION_BP_ADMIN_ADM_CTRL_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_CONST_NN_IDX, ION_BP_ADMIN_ADM_CONST_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_MACRO_NN_IDX, ION_BP_ADMIN_ADM_MACRO_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_OP_NN_IDX, ION_BP_ADMIN_ADM_OP_NN_STR, "ION_BP_ADMIN", "2017-08-17");
	oid_nn_add_parm(ION_BP_ADMIN_ADM_ROOT_NN_IDX, ION_BP_ADMIN_ADM_ROOT_NN_STR, "ION_BP_ADMIN", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_NAME_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAME", "The human-readable name of the ADM.", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_META_NAME_MID);
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_NAMESPACE_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("NAMESPACE", "The namespace of the ADM", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_META_NAMESPACE_MID);
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_VERSION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("VERSION", "The version of the ADM", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_META_VERSION_MID);
	adm_add_edd(mid_from_value(ADM_ION_BP_ADMIN_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, NULL, adm_print_string, adm_size_string);
	names_add_name("ORGANIZATION", "The name of the issuing organization of the ADM", ADM_ION_BP_ADMIN, ADM_ION_BP_ADMIN_META_ORGANIZATION_MID);
}


void adm_ion_bp_admin_init_ops()
{
}


void adm_ion_bp_admin_init_reports()
{
	uint32_t used= 0;
	Lyst rpt = NULL;
}

#endif // _HAVE_ION_BP_ADMIN_ADM_
