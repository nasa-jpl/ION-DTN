/*****************************************************************************
 **
 ** File Name: adm_bp_priv.c
 **
 ** Description: This implements the private aspects of a BP ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 *****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "shared/adm/adm_bp.h"
#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"

#include "adm_bp_impl.h"


value_t bp_md_name(Lyst params)
{
	return val_from_string("BP ADM");
}

value_t bp_md_ver(Lyst params)
{
	return val_from_string("v6");
}

value_t bp_node_get_node_id(Lyst params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = DTNMP_TYPE_STRING;
	result.value.as_ptr = adm_copy_string((char *) node_state.nodeID, &(result.length));
	return result;
}

value_t bp_node_get_version(Lyst params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = DTNMP_TYPE_STRING;

	result.value.as_ptr = adm_copy_string((char *) node_state.bpVersionNbr, &(result.length));

	return result;
}

value_t bp_node_get_storage(Lyst params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = node_state.avblStorage;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_last_restart(Lyst params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = node_state.lastRestartTime;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_num_reg(Lyst params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = node_state.nbrOfRegistrations;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_fwd_pend(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentForwardPending;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_dispatch_pend(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentDispatchPending;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_in_cust(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentInCustody;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_reassembly_pend(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentReassemblyPending;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_blk_src_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceCount[0];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_norm_src_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceCount[1];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_exp_src_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceCount[2];
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_blk_src_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceBytes[0];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_norm_src_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceBytes[1];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_exp_src_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceBytes[2];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_blk_res_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentCount[0];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_norm_res_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentCount[1];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_exp_res_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentCount[2];
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_blk_res_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentBytes[0];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_norm_res_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentBytes[1];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_exp_res_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentBytes[2];
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_bundles_frag(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundlesFragmented;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_frag_produced(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.fragmentsProduced;
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_del_none(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delNoneCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_expired(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delExpiredCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_fwd_uni(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delFwdUnidirCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_cancel(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delCanceledCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_deplete(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delDepletionCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_bad_eid(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delEidMalformedCount;
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_del_no_route(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delNoRouteCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_no_contact(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delNoContactCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_bad_blk(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.delBlkMalformedCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_del_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bytesDeletedToDate;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_fail_cust_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.custodyRefusedCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_fail_cust_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.custodyRefusedBytes;
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_fail_fwd_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleFwdFailedCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_fail_fwd_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleFwdFailedBytes;
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_fail_abandon_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleAbandonCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_fail_abandon_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleAbandonBytes;
	result.length = sizeof(uvast);

	return result;
}


value_t bp_node_get_fail_discard_cnt(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleDiscardCount;
	result.length = sizeof(uvast);

	return result;
}

value_t bp_node_get_fail_discard_bytes(Lyst params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = DTNMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleDiscardBytes;
	result.length = sizeof(uvast);

	return result;
}


value_t bp_endpoint_get_names(Lyst params)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	value_t result;
	result.type = DTNMP_TYPE_BLOB;

	uint8_t *cursor = NULL;

/* \todo: Lots of error checking, make sure we are within 2048 on names length. */
	bpnm_endpointNames_get((char *) names, 2048, ptrs, &num);

	encodeSdnv(&nm_sdnv, num);

	result.length = nm_sdnv.length +         /* NUM as SDNV length */
			        strlen(ptrs[num-1]) +    /* length of last string */
			        (ptrs[num-1] - names) +  /* # bytes to get to last string */
			        1;                       /* Final NULL terminator. */
	result.value.as_ptr = (uint8_t *) MTAKE(result.length);

	cursor = result.value.as_ptr;

	memcpy(cursor,nm_sdnv.text, nm_sdnv.length);
	cursor += nm_sdnv.length;

	memcpy(cursor, names, result.length - nm_sdnv.length);

	return result;
}


value_t bp_endpoint_get_name(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	value_t result;
	result.type = DTNMP_TYPE_STRING;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value.as_ptr = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.length = (uint64_t) strlen(endpoint.eid) + 1;
		result.value.as_ptr = (uint8_t*) MTAKE(result.length);
		memset(result.value.as_ptr,0,result.length);
		memcpy(result.value.as_ptr, endpoint.eid, result.length);
	}

	return result;
}

value_t bp_endpoint_get_active(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	value_t result;
	result.type = DTNMP_TYPE_UINT;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value.as_uint = endpoint.active;
		result.length = sizeof(endpoint.active);
	}

	return result;
}

value_t bp_endpoint_get_singleton(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	value_t result;
	result.type = DTNMP_TYPE_UINT;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value.as_uint = endpoint.singleton;
		result.length = sizeof(endpoint.singleton);
	}

	return result;
}


value_t bp_endpoint_get_abandon(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	value_t result;
	result.type = DTNMP_TYPE_UINT;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.value.as_uint = endpoint.abandonOnDelivFailure;
		result.length = sizeof(endpoint.abandonOnDelivFailure);
	}

	return result;
}



/* Controls */
tdc_t *bp_ctrl_reset(eid_t *def_mgr, Lyst params, uint8_t *status)
{
	tdc_t result;

	bpnm_disposition_reset();
	*status = CTRL_SUCCESS;

	memset(&result, 0, sizeof(tdc_t));
	return NULL;
}


