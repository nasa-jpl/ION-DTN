/****************************************************************************
 **
 ** File Name: adm_bp_agent_impl.c
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

/*   START CUSTOM INCLUDES HERE  */
#include "ion.h"
#include "bp.h"
#include "bpP.h"
#include "bpnm.h"


/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_bp_agent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_bp_agent_setup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void dtn_bp_agent_cleanup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


tnv_t *dtn_bp_agent_meta_name(tnvc_t *parms)
{
	return tnv_from_str("bp_agent");
}


tnv_t *dtn_bp_agent_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/bp_agent");
}


tnv_t *dtn_bp_agent_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.1");
}


tnv_t *dtn_bp_agent_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/* Collect Functions */
/*
 * The node administration endpoint
 */
tnv_t *dtn_bp_agent_get_bp_node_id(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bp_node_id BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result = tnv_from_str((char *) node_state.nodeID);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_bp_node_id BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The latest version of the BP supported by this node
 */
tnv_t *dtn_bp_agent_get_bp_node_version(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bp_node_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result = tnv_from_str((char *) node_state.bpVersionNbr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_bp_node_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Bytes available for bundle storage
 */
tnv_t *dtn_bp_agent_get_available_storage(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_available_storage BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result = tnv_from_uvast(node_state.avblStorage);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_available_storage BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * The last time that BP counters were reset, either due to execution of a reset control or a restart o
 * f the node itself
 */
tnv_t *dtn_bp_agent_get_last_reset_time(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_reset_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr sdr = getIonsdr();
	Object dbobj = getBpDbObject();
	BpDB db;

	// TODO Check return call for sdr_begin_xn
	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(BpDB));

	result = tnv_from_uvast(db.resetTime);
	oK(sdr_end_xn(sdr));

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_last_reset_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of registrations
 */
tnv_t *dtn_bp_agent_get_num_registrations(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_registrations BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result = tnv_from_uint(node_state.nbrOfRegistrations);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_registrations BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles pending forwarding
 */
tnv_t *dtn_bp_agent_get_num_pend_fwd(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_pend_fwd BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.currentForwardPending);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_pend_fwd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles awaiting dispatch
 */
tnv_t *dtn_bp_agent_get_num_pend_dis(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_pend_dis BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.currentDispatchPending);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_pend_dis BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles
 */
tnv_t *dtn_bp_agent_get_num_in_cust(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_in_cust BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.currentInCustody);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_in_cust BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles pending reassembly
 */
tnv_t *dtn_bp_agent_get_num_pend_reassembly(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_pend_reassembly BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.currentReassemblyPending);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_pend_reassembly BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles for the given priority. Priority is given as a priority mask where Bulk=0x1, norma
 * l=0x2, express=0x4. Any bundles matching any of the masked priorities will be included in the return
 * ed count
 */
tnv_t *dtn_bp_agent_get_bundles_by_priority(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bundles_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	int success = 0;

	bpnm_disposition_get(&state);
	uvast val = 0;
	uint32_t mask = adm_get_parm_uint(parms, 0, &success);

	if(mask & 0x1)
	{
		val += state.currentResidentCount[0];
	}
	if(mask & 0x2)
	{
		val += state.currentResidentCount[1];
	}
	if(mask & 0x4)
	{
		val += state.currentResidentCount[2];
	}

	result = tnv_from_uvast(val);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_bundles_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bytes of the given priority. Priority is given as a priority mask where bulk=0x1, normal=0
 * x2, express=0x4. Any bundles matching any of the masked priorities will be included in the returned 
 * count.
 */
tnv_t *dtn_bp_agent_get_bytes_by_priority(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bytes_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	int success = 0;
	bpnm_disposition_get(&state);
	uvast val = 0;
	uint32_t mask = adm_get_parm_uint(parms, 0, &success);

	if(mask & 0x1)
	{
		val += state.currentResidentBytes[0];
	}
	if(mask & 0x2)
	{
		val += state.currentResidentBytes[1];
	}
	if(mask & 0x4)
	{
		val += state.currentResidentBytes[2];
	}

	result = tnv_from_uvast(val);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_bytes_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles sourced by this node of the given priority. Priority is given as a priority mask w
 * here bulk=0x1, normal=0x2, express=0x4. Any bundles sourced by this node and matching any of the mas
 * ked priorities will be included in the returned count.
 */
tnv_t *dtn_bp_agent_get_src_bundles_by_priority(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_src_bundles_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	int success = 0;
	bpnm_disposition_get(&state);

	uvast val = 0;
	uint32_t mask = adm_get_parm_uint(parms, 0, &success);

	if(mask & 0x1)
	{
		val += state.bundleSourceCount[0];
	}
	if(mask & 0x2)
	{
		val += state.bundleSourceCount[1];
	}
	if(mask & 0x4)
	{
		val += state.bundleSourceCount[2];
	}

	result = tnv_from_uvast(val);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_src_bundles_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bytes sourced by this node of the given priority. Priority is given as a priority mask whe
 * re bulk=0x1, normal=0x2, express=0x4. Any bundles sourced by this node and matching any of the maske
 * d priorities will be included in the returned count
 */
tnv_t *dtn_bp_agent_get_src_bytes_by_priority(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_src_bytes_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	int success = 0;
	bpnm_disposition_get(&state);

	uvast val = 0;
	uint32_t mask = adm_get_parm_uint(parms, 0, &success);

	if(mask & 0x1)
	{
		val += state.bundleSourceBytes[0];
	}
	if(mask & 0x2)
	{
		val += state.bundleSourceBytes[1];
	}
	if(mask & 0x4)
	{
		val += state.bundleSourceBytes[2];
	}

	result = tnv_from_uvast(val);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_src_bytes_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of fragmented bundles
 */
tnv_t *dtn_bp_agent_get_num_fragmented_bundles(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fragmented_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */

	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundlesFragmented);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fragmented_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles with fragmentary payloads produced by this node
 */
tnv_t *dtn_bp_agent_get_num_fragments_produced(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fragments_produced BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.fragmentsProduced);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fragments_produced BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles failed for any of the given reasons. (noInfo=0x1, Expired=0x2, UniFwd=0x4, Cancell
 * ed=0x8, NoStorage=0x10, BadEID=0x20, NoRoute=0x40, NoContact=0x80, BadBlock=0x100)
 */
tnv_t *dtn_bp_agent_get_num_failed_by_reason(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_failed_by_reason BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	int success = 0;
	bpnm_disposition_get(&state);

	uint32_t mask = adm_get_parm_uint(parms, 0, &success);
	uvast val = 0;

	if(mask & 0x01)
	{
		val += state.delNoneCount;
	}
	if(mask & 0x02)
	{
		val += state.delExpiredCount;
	}
	if(mask & 0x04)
	{
		val += state.delFwdUnidirCount;
	}
	if(mask & 0x08)
	{
		val += state.delCanceledCount;
	}
	if(mask & 0x10)
	{
		val += state.delDepletionCount;
	}
	if(mask & 0x20)
	{
		val += state.delEidMalformedCount;
	}
	if(mask & 0x40)
	{
		val += state.delNoRouteCount;
	}
	if(mask & 0x80)
	{
		val += state.delNoContactCount;
	}
	if(mask & 0x100)
	{
		val += state.delBlkMalformedCount;
	}

	result = tnv_from_uvast(val);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_failed_by_reason BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles deleted by this node
 */
tnv_t *dtn_bp_agent_get_num_bundles_deleted(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bundles_deleted BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.delNoneCount + state.delExpiredCount + state.delFwdUnidirCount + state.delCanceledCount + state.delDepletionCount + state.delEidMalformedCount + state.delNoRouteCount + state.delNoContactCount + state.delBlkMalformedCount);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bundles_deleted BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundle fails at this node
 */
tnv_t *dtn_bp_agent_get_failed_custody_bundles(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_custody_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.custodyRefusedCount);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_failed_custody_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number bytes of fails at this node
 */
tnv_t *dtn_bp_agent_get_failed_custody_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_custody_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.custodyRefusedBytes);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_failed_custody_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number bundles not forwarded by this node
 */
tnv_t *dtn_bp_agent_get_failed_forward_bundles(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_forward_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundleFwdFailedCount);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_failed_forward_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bytes not forwaded by this node
 */
tnv_t *dtn_bp_agent_get_failed_forward_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_forward_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundleFwdFailedBytes);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_failed_forward_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles abandoned by this node
 */
tnv_t *dtn_bp_agent_get_abandoned_bundles(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_abandoned_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundleAbandonCount);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_abandoned_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bytes abandoned by this node
 */
tnv_t *dtn_bp_agent_get_abandoned_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_abandoned_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundleAbandonBytes);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_abandoned_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bundles discarded by this node
 */
tnv_t *dtn_bp_agent_get_discarded_bundles(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_discarded_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundleDiscardCount);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_discarded_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * number of bytes discarded by this node
 */
tnv_t *dtn_bp_agent_get_discarded_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_discarded_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result = tnv_from_uvast(state.bundleDiscardBytes);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_discarded_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * CSV list of endpoint names for this node
 */
tnv_t *dtn_bp_agent_get_endpoint_names(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	char names[2049];
	char *ptrs[128];
	int num = 0;
	int i = 0;

	bpnm_endpointNames_get((char *) names, 2048, ptrs, &num);

	for(i = 0; i < (num-1); i++)
	{
		*(ptrs[i+1]-sizeof(char)) = ',';
	}

	result = tnv_from_str(names);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_endpoint_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * is the given endpoint active? (0=no)
 */
tnv_t *dtn_bp_agent_get_endpoint_active(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_active BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	NmbpEndpoint endpoint;
	int success = 0;

	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result = tnv_from_uint(endpoint.active);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_endpoint_active BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * is the given endpoint singleton? (0=no)
 */
tnv_t *dtn_bp_agent_get_endpoint_singleton(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_singleton BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	NmbpEndpoint endpoint;
	int success = 0;

	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result = tnv_from_uint(endpoint.singleton);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_endpoint_singleton BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Does the endpoint abandon on fail (0=no)
 */
tnv_t *dtn_bp_agent_get_endpoint_policy(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_policy BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	NmbpEndpoint endpoint;
	int success = 0;

	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result = tnv_from_uint(endpoint.abandonOnDelivFailure);
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_endpoint_policy BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * This control causes the Agent to reset all counts associated with bundle or byte statistics and to s
 * et the last reset time of the BP primitive data to the time when the control was run
 */
tnv_t *dtn_bp_agent_ctrl_reset_all_counts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_reset_all_counts BODY
	 * +-------------------------------------------------------------------------+
	 */

	bpnm_disposition_reset();
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_reset_all_counts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
