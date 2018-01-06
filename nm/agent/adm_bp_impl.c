/****************************************************************************
 **
 ** File Name: adm_bp_impl.c
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
 **  2018-01-04  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_bp_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_bp_setup(){

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

void adm_bp_cleanup(){

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


value_t adm_bp_meta_name(tdc_t params)
{
	return val_from_string("adm_bp");
}


value_t adm_bp_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:bp");
}


value_t adm_bp_meta_version(tdc_t params)
{
	return val_from_string("v0.1");
}


value_t adm_bp_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/* Collect Functions */
/*
 * The node administration endpoint
 */
value_t adm_bp_get_bp_node_id(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bp_node_id BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_STRING;
	result.value.as_ptr = adm_copy_string((char *) node_state.nodeID, NULL);
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
value_t adm_bp_get_bp_node_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bp_node_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_STRING;

	result.value.as_ptr = adm_copy_string((char *) node_state.bpVersionNbr, NULL);
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
value_t adm_bp_get_available_storage(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_available_storage BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_STRING;

	result.value.as_ptr = adm_copy_string((char *) node_state.bpVersionNbr, NULL);
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
value_t adm_bp_get_last_reset_time(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_reset_time BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr sdr = getIonsdr();
	Object dbobj = getBpDbObject();
	BpDB db;

	result.type = AMP_TYPE_UVAST;
// TODO Check return call for sdr_begin_xn
	sdr_begin_xn(sdr);
	sdr_stage(sdr, (char *) &db, dbobj, sizeof(BpDB));
	result.value.as_uvast = db.resetTime;
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
value_t adm_bp_get_num_registrations(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_registrations BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = node_state.nbrOfRegistrations;

	
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
value_t adm_bp_get_num_pend_fwd(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_pend_fwd BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentForwardPending;

	
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
value_t adm_bp_get_num_pend_dis(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_pend_dis BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentDispatchPending;

	
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
value_t adm_bp_get_num_in_cust(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_in_cust BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentInCustody;

	
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
value_t adm_bp_get_num_pend_reassembly(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_pend_reassembly BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentReassemblyPending;
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
value_t adm_bp_get_bundles_by_priority(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bundles_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

  	uvast val = adm_extract_uvast(params, 0, &success);

  	result.type = AMP_TYPE_UVAST;

  	switch(val)
  	{
	  	case 0x1: 
	  		result.value.as_uvast = state.currentResidentCount[0];
	  		break;
	  	case 0x2:
	  		result.value.as_uvast = state.currentResidentCount[1];
	  		break;
	  	case 0x4:
	  		result.value.as_uvast = state.currentResidentCount[2];
	  		break;
	  	default:
	  		AMP_DEBUG_ERR("get_bundles_by_priority","Invalid priority",NULL);
	  		break;
	}

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
value_t adm_bp_get_bytes_by_priority(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bytes_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

  	uvast val = adm_extract_uvast(params, 0, &success);

  	result.type = AMP_TYPE_UVAST;

  	switch(val)
  	{
	  	case 0x1: 
	  		result.value.as_uvast = state.currentResidentBytes[0];
	  		break;
	  	case 0x2:
	  		result.value.as_uvast = state.currentResidentBytes[1];
	  		break;
	  	case 0x4:
	  		result.value.as_uvast = state.currentResidentBytes[2];
	  		break;
	  	default:
	  		AMP_DEBUG_ERR("get_bytes_by_priority","Invalid priority",NULL);
	  		break;
	}
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
value_t adm_bp_get_src_bundles_by_priority(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_src_bundles_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

  	uvast val = adm_extract_uvast(params, 0, &success);

  	result.type = AMP_TYPE_UVAST;

  	switch(val)
  	{
	  	case 0x1: 
	  		result.value.as_uvast = state.bundleSourceCount[0];
	  		break;
	  	case 0x2:
	  		result.value.as_uvast = state.bundleSourceCount[1];
	  		break;
	  	case 0x4:
	  		result.value.as_uvast = state.bundleSourceCount[2];
	  		break;
	  	default:
	  		AMP_DEBUG_ERR("get_src_bundles_by_priority","Invalid priority",NULL);
	  		break;
	}
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
value_t adm_bp_get_src_bytes_by_priority(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_src_bytes_by_priority BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

  	uvast val = adm_extract_uvast(params, 0, &success);

  	result.type = AMP_TYPE_UVAST;

  	switch(val)
  	{
	  	case 0x1: 
	  		result.value.as_uvast = state.bundleSourceBytes[0];
	  		break;
	  	case 0x2:
	  		result.value.as_uvast = state.bundleSourceBytes[1];
	  		break;
	  	case 0x4:
	  		result.value.as_uvast = state.bundleSourceBytes[2];
	  		break;
	  	default:
	  		AMP_DEBUG_ERR("get_src_bytes_by_priority","Invalid priority",NULL);
	  		break;
	}
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
value_t adm_bp_get_num_fragmented_bundles(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fragmented_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */






	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundlesFragmented;

	
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
value_t adm_bp_get_num_fragments_produced(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fragments_produced BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.fragmentsProduced;

	
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
value_t adm_bp_get_num_failed_by_reason(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_failed_by_reason BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

  uvast val = adm_extract_uvast(params, 0, &success);

  result.type = AMP_TYPE_UVAST;

  switch(val)
  {
  	case 0x01: 
  		result.value.as_uvast = state.delNoneCount;
  		break;
  	case 0x02:
  		result.value.as_uvast = state.delExpiredCount;
  		break;
  	case 0x04:
  		result.value.as_uvast = state.UniFwd;
  		break;
  	case 0x08:
  		result.value.as_uvast = state.delCanceledCount;
  		break;
  	case 0x10:
  		result.value.as_uvast = state.delDepletionCount;
  		break;
  	case 0x20:
  		result.value.as_uvast = state.delEidMalformedCount;
  		break;
  	case 0x40:
  		result.value.as_uvast = state.delNoRouteCount;
  		break;
  	case 0x80:
  		result.value.as_uvast = state.delNoContactCount;
  		break;
  	case 0x100:
  		result.value.as_uvast = state.delBlkMalformedCount;
  		break;
  	default:
  		AMP_DEBUG_ERR("get_num_failed_by_reason","Invalid number failed reason",NULL);
  		break;
  }
 
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
value_t adm_bp_get_num_bundles_deleted(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bundles_deleted BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bytesDeletedToDate;

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
value_t adm_bp_get_failed_custody_bundles(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_custody_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.custodyRefusedCount;

	
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
value_t adm_bp_get_failed_custody_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_custody_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.custodyRefusedBytes;

	
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
value_t adm_bp_get_failed_forward_bundles(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_forward_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleFwdFailedCount;

	
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
value_t adm_bp_get_failed_forward_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_failed_forward_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleFwdFailedBytes;

	
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
value_t adm_bp_get_abandoned_bundles(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_abandoned_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleAbandonCount;

	
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
value_t adm_bp_get_abandoned_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_abandoned_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleAbandonBytes;

	
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
value_t adm_bp_get_discarded_bundles(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_discarded_bundles BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleDiscardCount;

	
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
value_t adm_bp_get_discarded_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_discarded_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleDiscardBytes;

	
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
value_t adm_bp_get_endpoint_names(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	char names[2048];
	char *ptrs[128];
	int num = 0;
	uint32_t size = 0;
	uint8_t *cursor = NULL;
	uint8_t first = 0;
	int i = 0;

	val_init(&result);

	bpnm_endpointNames_get((char *) names, 2048, ptrs, &num);

	for(i = 0; i < num; i++)
	{
		size += strlen(ptrs[i]) + 2; // string + ", "
	}

	if((result.value.as_ptr = (uint8_t *) STAKE(size + 1)) == NULL)
	{
		AMP_DEBUG_ERR("adm_bp_endpoint_get_names","Cannot allocate %d bytes for %d eids.", size, num);
		return result;
	}

	result.type = AMP_TYPE_STRING;

	cursor = result.value.as_ptr;
	memset(cursor, 0, size + 1);
	for(i = 0; i < num; i++)
	{
		if(first == 0)
		{
			first = 1;
		}
		else
		{
			sprintf((char*)cursor,", ");
			cursor += 2;
		}
		sprintf((char*)cursor, "%s", ptrs[i]);
		cursor += strlen(ptrs[i]);
	}

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
value_t adm_bp_get_endpoint_active(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_active BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;

	val_init(&result);


	NmbpEndpoint endpoint;
	int success = 0;
	int8_t adm_success = 0;

	if((name = adm_extract_string(params, 0, &adm_success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_bp_endpoint_get_active","Can't extract first parm.", NULL);
		return result;
	}

	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = endpoint.active;
	}

	SRELEASE(name);
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
value_t adm_bp_get_endpoint_singleton(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_singleton BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;
	NmbpEndpoint endpoint;
	int success = 0;
	int8_t adm_success = 0;

	val_init(&result);

	if((name = adm_extract_string(params, 0, &adm_success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_bp_endpoint_get_singleton","Can't extract first parm.", NULL);
		return result;
	}

	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = endpoint.singleton;
	}

	SRELEASE(name);
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
value_t adm_bp_get_endpoint_policy(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_endpoint_policy BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;
	NmbpEndpoint endpoint;
	int success = 0;
	int8_t adm_success = 0;

	val_init(&result);

	if((name = adm_extract_string(params, 0, &adm_success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_get_endpoint_policy","Can't extract first parm.", NULL);
		return result;
	}

	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = endpoint.policy;
	}

	SRELEASE(name);
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
tdc_t* adm_bp_ctrl_reset_all_counts(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_reset_all_counts BODY
	 * +-------------------------------------------------------------------------+
	 */
	tdc_t result;

	bpnm_disposition_reset();
	*status = CTRL_SUCCESS;

	memset(&result, 0, sizeof(tdc_t));
	return NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_reset_all_counts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
