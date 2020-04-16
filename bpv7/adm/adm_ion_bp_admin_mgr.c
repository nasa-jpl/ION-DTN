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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_ion_bp_admin.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "metadata.h"
#include "nm_mgr_ui.h"




#define _HAVE_DTN_ION_BPADMIN_ADM_
#ifdef _HAVE_DTN_ION_BPADMIN_ADM_
static vec_idx_t g_dtn_ion_bpadmin_idx[11];

void dtn_ion_bpadmin_init()
{
	adm_add_adm_info("dtn_ion_bpadmin", ADM_ENUM_DTN_ION_BPADMIN);

	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_META_IDX), &(g_dtn_ion_bpadmin_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_TBLT_IDX), &(g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_EDD_IDX), &(g_dtn_ion_bpadmin_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_ION_BPADMIN * 20) + ADM_CTRL_IDX), &(g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX]));


	dtn_ion_bpadmin_init_meta();
	dtn_ion_bpadmin_init_cnst();
	dtn_ion_bpadmin_init_edd();
	dtn_ion_bpadmin_init_op();
	dtn_ion_bpadmin_init_var();
	dtn_ion_bpadmin_init_ctrl();
	dtn_ion_bpadmin_init_mac();
	dtn_ion_bpadmin_init_rpttpl();
	dtn_ion_bpadmin_init_tblt();
}

void dtn_ion_bpadmin_init_meta()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_BPADMIN, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_BPADMIN, "namespace", "The namespace of the ADM");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_BPADMIN, "version", "The version of the ADM");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_ion_bpadmin_idx[ADM_META_IDX], DTN_ION_BPADMIN_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_BPADMIN, "organization", "The name of the issuing organization of the ADM");

}

void dtn_ion_bpadmin_init_cnst()
{

}

void dtn_ion_bpadmin_init_edd()
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_ion_bpadmin_idx[ADM_EDD_IDX], DTN_ION_BPADMIN_EDD_BP_VERSION);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_ION_BPADMIN, "bp_version", "Version of installed ION BP Admin utility.");

}

void dtn_ion_bpadmin_init_op()
{

}

void dtn_ion_bpadmin_init_var()
{

}

void dtn_ion_bpadmin_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* ENDPOINT_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_ENDPOINT_ADD);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "endpoint_add", "Establish DTN endpoint named endpointId on the local node. The remaining parameters indicate what is to be done when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception. If type is 'x', then such bundles are to be discarded silently and immediately. If type is 'q', then such bundles are to be enqueued for later delivery and, if recvScript is provided, recvScript is to be executed.");

	meta_add_parm(meta, "endpoint_id", AMP_TYPE_STR);
	meta_add_parm(meta, "type", AMP_TYPE_UINT);
	meta_add_parm(meta, "rcv_script", AMP_TYPE_STR);

	/* ENDPOINT_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_ENDPOINT_CHANGE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "endpoint_change", "Change the action taken when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception.");

	meta_add_parm(meta, "endpoint_id", AMP_TYPE_STR);
	meta_add_parm(meta, "type", AMP_TYPE_UINT);
	meta_add_parm(meta, "rcv_script", AMP_TYPE_STR);

	/* ENDPOINT_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_ENDPOINT_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "endpoint_del", "Delete the endpoint identified by endpointId. The control will fail if any bundles are currently pending delivery to this endpoint.");

	meta_add_parm(meta, "endpoint_id", AMP_TYPE_STR);

	/* INDUCT_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_ADD);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "induct_add", "Establish a duct for reception of bundles via the indicated CL protocol. The duct's data acquisition structure is used and populated by the induct task whose operation is initiated by cliControl at the time the duct is started.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);
	meta_add_parm(meta, "cli_control", AMP_TYPE_STR);

	/* INDUCT_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_CHANGE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "induct_change", "Change the control used to initiate operation of the induct task for the indicated duct.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);
	meta_add_parm(meta, "cli_control", AMP_TYPE_STR);

	/* INDUCT_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_DEL);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "induct_del", "Delete the induct identified by protocolName and ductName. The control will fail if any bundles are currently pending acquisition via this induct.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);

	/* INDUCT_START */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_START);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "induct_start", "Start the indicated induct task as defined for the indicated CL protocol on the local node.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);

	/* INDUCT_STOP */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_INDUCT_STOP);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "induct_stop", "Stop the indicated induct task as defined for the indicated CL protocol on the local node.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);

	/* MANAGE_HEAP_MAX */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_MANAGE_HEAP_MAX);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "manage_heap_max", "Declare the maximum number of bytes of SDR heap space that will be occupied by any single bundle acquisition activity (nominally the acquisition of a single bundle, but this is at the discretion of the convergence-layer input task). All data acquired in excess of this limit will be written to a temporary file pending extraction and dispatching of the acquired bundle or bundles. The default is the minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimum SDR heap space occupancy in the event that all acquisition is into a file.");

	meta_add_parm(meta, "max_database_heap_per_acquisition", AMP_TYPE_UINT);

	/* OUTDUCT_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_ADD);
	adm_add_ctrldef_ari(id, 4, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "outduct_add", "Establish a duct for transmission of bundles via the indicated CL protocol. the duct's data transmission structure is serviced by the outduct task whose operation is initiated by CLOcommand at the time the duct is started. A value of zero for maxPayloadLength indicates that bundles of any size can be accomodated; this is the default.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);
	meta_add_parm(meta, "clo_command", AMP_TYPE_STR);
	meta_add_parm(meta, "max_payload_length", AMP_TYPE_UINT);

	/* OUTDUCT_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_CHANGE);
	adm_add_ctrldef_ari(id, 4, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "outduct_change", "Set new values for the indicated duct's payload size limit and the control that is used to initiate operation of the outduct task for this duct.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);
	meta_add_parm(meta, "clo_control", AMP_TYPE_STR);
	meta_add_parm(meta, "max_payload_length", AMP_TYPE_UINT);

	/* OUTDUCT_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_DEL);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "outduct_del", "Delete the outduct identified by protocolName and ductName. The control will fail if any bundles are currently pending transmission via this outduct.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);

	/* OUTDUCT_START */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_START);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "outduct_start", "Start the indicated outduct task as defined for the indicated CL protocol on the local node.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);

	/* EGRESS_PLAN_BLOCK */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_EGRESS_PLAN_BLOCK);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "egress_plan_block", "Disable transmission of bundles queued for transmission to the indicated node and reforwards all non-critical bundles currently queued for transmission to this node. This may result in some or all of these bundles being enqueued for transmission to the psuedo-node limbo.");

	meta_add_parm(meta, "plan_name", AMP_TYPE_STR);

	/* EGRESS_PLAN_UNBLOCK */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_EGRESS_PLAN_UNBLOCK);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "egress_plan_unblock", "Re-enable transmission of bundles to the indicated node and reforwards all bundles in limbo in the hope that the unblocking of this egress plan will enable some of them to be transmitted.");

	meta_add_parm(meta, "plan_name", AMP_TYPE_STR);

	/* OUTDUCT_STOP */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_OUTDUCT_STOP);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "outduct_stop", "Stop the indicated outduct task as defined for the indicated CL protocol on the local node.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "duct_name", AMP_TYPE_STR);

	/* PROTOCOL_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_ADD);
	adm_add_ctrldef_ari(id, 4, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "protocol_add", "Establish access to the named convergence layer protocol at the local node. The payloadBytesPerFrame and overheadBytesPerFrame arguments are used in calculating the estimated transmission capacity consumption of each bundle, to aid in route computation and congesting forecasting. The optional nominalDataRate argument overrides the hard coded default continuous data rate for the indicated protocol for purposes of rate control. For all promiscuous prototocols-that is, protocols whose outducts are not specifically dedicated to transmission to a single identified convergence-layer protocol endpoint- the protocol's applicable nominal continuous data rate is the data rate that is always used for rate control over links served by that protocol; data rates are not extracted from contact graph information. This is because only the induct and outduct throttles for non-promiscuous protocols (LTP, TCP) can be dynamically adjusted in response to changes in data rate between the local node and its neighbors, as enacted per the contact plan. Even for an outduct of a non-promiscuous protocol the nominal data rate may be the authority for rate control, in the event that the contact plan lacks identified contacts with the node to which the outduct is mapped.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);
	meta_add_parm(meta, "payload_bytes_per_frame", AMP_TYPE_UINT);
	meta_add_parm(meta, "overhead_bytes_per_frame", AMP_TYPE_UINT);
	meta_add_parm(meta, "nominal_data_rate", AMP_TYPE_UINT);

	/* PROTOCOL_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "protocol_del", "Delete the convergence layer protocol identified by protocolName. The control will fail if any ducts are still locally declared for this protocol.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);

	/* PROTOCOL_START */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_START);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "protocol_start", "Start all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);

	/* PROTOCOL_STOP */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_PROTOCOL_STOP);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "protocol_stop", "Stop all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.");

	meta_add_parm(meta, "protocol_name", AMP_TYPE_STR);

	/* SCHEME_ADD */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_ADD);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "scheme_add", "Declares an endpoint naming scheme for use in endpoint IDs, which are structured as URIs: schemeName:schemeSpecificPart. forwarderControl will be executed when the scheme is started on this node, to initiate operation of a forwarding daemon for this scheme. adminAppControl will also be executed when the scheme is started on this node, to initiate operation of a daemon that opens a custodian endpoint identified within this scheme so that it can recieve and process custody signals and bundle status reports.");

	meta_add_parm(meta, "scheme_name", AMP_TYPE_STR);
	meta_add_parm(meta, "forwarder_control", AMP_TYPE_STR);
	meta_add_parm(meta, "admin_app_control", AMP_TYPE_STR);

	/* SCHEME_CHANGE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_CHANGE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "scheme_change", "Set the indicated scheme's forwarderControl and adminAppControl to the strings provided as arguments.");

	meta_add_parm(meta, "scheme_name", AMP_TYPE_STR);
	meta_add_parm(meta, "forwarder_control", AMP_TYPE_STR);
	meta_add_parm(meta, "admin_app_control", AMP_TYPE_STR);

	/* SCHEME_DEL */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_DEL);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "scheme_del", "Delete the scheme identified by schemeName. The control will fail if any bundles identified in this scheme are pending forwarding, transmission, or delivery.");

	meta_add_parm(meta, "scheme_name", AMP_TYPE_STR);

	/* SCHEME_START */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_START);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "scheme_start", "Start the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.");

	meta_add_parm(meta, "scheme_name", AMP_TYPE_STR);

	/* SCHEME_STOP */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_SCHEME_STOP);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "scheme_stop", "Stop the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.");

	meta_add_parm(meta, "scheme_name", AMP_TYPE_STR);

	/* WATCH */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_ion_bpadmin_idx[ADM_CTRL_IDX], DTN_ION_BPADMIN_CTRL_WATCH);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_ION_BPADMIN, "watch", "Enable/Disable production of a continuous stream of user selected Bundle Protocol activity indication characters. A watch parameter of 1 selects all BP activity indication characters, 0 deselects allBP activity indication characters; any other activitySpec such as acz~ selects all activity indication characters in the string, deselecting all others. BP will print each selected activity indication character to stdout every time a processing event of the associated type occurs: a new bundle is queued for forwarding, b bundle is queued for transmission, c bundle is popped from its transmission queue, m custody acceptance signal is recieved, w custody of bundle is accepted, x custody of bundle is refused, y bundle is accepted upon arrival, z bundle is queued for delivery to an application, ~ bundle is abandoned (discarded) on attempt to forward it, ! bundle is destroyed due to TTL expiration, &amp; custody refusal signal is recieved, # bundle is queued for re-forwarding due to CL protocol failures, j bundle is placed in 'limbo' for possible future reforwarding, k bundle is removed from 'limbo' and queued for reforwarding, $ bundle's custodial retransmission timeout interval expired.");

	meta_add_parm(meta, "status", AMP_TYPE_UINT);
	meta_add_parm(meta, "activity_spec", AMP_TYPE_STR);
}

void dtn_ion_bpadmin_init_mac()
{

}

void dtn_ion_bpadmin_init_rpttpl()
{

}

void dtn_ion_bpadmin_init_tblt()
{

	tblt_t *def = NULL;

	/* ENDPOINTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_ENDPOINTS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "scheme_name");
	tblt_add_col(def, AMP_TYPE_STR, "endpoint_nss");
	tblt_add_col(def, AMP_TYPE_UINT, "app_pid");
	tblt_add_col(def, AMP_TYPE_STR, "recv_rule");
	tblt_add_col(def, AMP_TYPE_STR, "rcv_script");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_BPADMIN, "endpoints", "Local endpoints, regardless of scheme name.");

	/* INDUCTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_INDUCTS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "protocol_name");
	tblt_add_col(def, AMP_TYPE_STR, "duct_name");
	tblt_add_col(def, AMP_TYPE_STR, "cli_control");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_BPADMIN, "inducts", "Inducts established locally for the indicated CL protocol.");

	/* OUTDUCTS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_OUTDUCTS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "protocol_name");
	tblt_add_col(def, AMP_TYPE_STR, "duct_name");
	tblt_add_col(def, AMP_TYPE_UINT, "clo_pid");
	tblt_add_col(def, AMP_TYPE_STR, "clo_control");
	tblt_add_col(def, AMP_TYPE_UINT, "max_payload_length");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_BPADMIN, "outducts", "If protocolName is specified, this table lists all outducts established locally for the indicated CL protocol. Otherwise, it lists all locally established outducts, regardless of their protocol.");

	/* PROTOCOLS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_PROTOCOLS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "name");
	tblt_add_col(def, AMP_TYPE_UINT, "payload_bpf");
	tblt_add_col(def, AMP_TYPE_UINT, "overhead_bpf");
	tblt_add_col(def, AMP_TYPE_UINT, "protocol class");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_BPADMIN, "protocols", "Convergence layer protocols that can currently be utilized at the local node.");

	/* SCHEMES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_SCHEMES), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "scheme_name");
	tblt_add_col(def, AMP_TYPE_UINT, "fwd_pid");
	tblt_add_col(def, AMP_TYPE_STR, "fwd_cmd");
	tblt_add_col(def, AMP_TYPE_UINT, "admin_app_pid");
	tblt_add_col(def, AMP_TYPE_STR, "admin_app_cmd");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_BPADMIN, "schemes", "Declared endpoint naming schemes.");

	/* EGRESS_PLANS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_ion_bpadmin_idx[ADM_TBLT_IDX], DTN_ION_BPADMIN_TBLT_EGRESS_PLANS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "neighbor_eid");
	tblt_add_col(def, AMP_TYPE_UINT, "clm_pid");
	tblt_add_col(def, AMP_TYPE_UINT, "nominal_rate");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_ION_BPADMIN, "egress_plans", "Egress plans.");
}

#endif // _HAVE_DTN_ION_BPADMIN_ADM_
