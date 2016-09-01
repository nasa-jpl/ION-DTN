/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm_adm_bp_impl.c
 **
 ** Description: This implements the private aspects of a BP ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
*****************************************************************************/
#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "../shared/adm/adm_bp.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"

#include "adm_bp_impl.h"


value_t adm_bp_md_name(tdc_t params)
{
	return val_from_string("BP ADM");
}

value_t adm_bp_md_ver(tdc_t params)
{
	return val_from_string("v6");
}

value_t adm_bp_node_get_node_id(tdc_t params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_STRING;
	result.value.as_ptr = adm_copy_string((char *) node_state.nodeID, NULL);
	return result;
}

value_t adm_bp_node_get_version(tdc_t params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_STRING;

	result.value.as_ptr = adm_copy_string((char *) node_state.bpVersionNbr, NULL);

	return result;
}

value_t adm_bp_node_get_storage(tdc_t params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = node_state.avblStorage;

	return result;
}

value_t adm_bp_node_get_last_restart(tdc_t params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = node_state.lastRestartTime;

	return result;
}

value_t adm_bp_node_get_num_reg(tdc_t params)
{
	value_t result;
	NmbpNode node_state;
	bpnm_node_get(&node_state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = node_state.nbrOfRegistrations;

	return result;
}

value_t adm_bp_node_get_fwd_pend(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentForwardPending;

	return result;
}

value_t adm_bp_node_get_dispatch_pend(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentDispatchPending;

	return result;
}

value_t adm_bp_node_get_in_cust(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentInCustody;

	return result;
}

value_t adm_bp_node_get_reassembly_pend(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentReassemblyPending;

	return result;
}

value_t adm_bp_node_get_blk_src_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceCount[0];

	return result;
}

value_t adm_bp_node_get_norm_src_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceCount[1];

	return result;
}

value_t adm_bp_node_get_exp_src_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceCount[2];

	return result;
}


value_t adm_bp_node_get_blk_src_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceBytes[0];

	return result;
}

value_t adm_bp_node_get_norm_src_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceBytes[1];

	return result;
}

value_t adm_bp_node_get_exp_src_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleSourceBytes[2];

	return result;
}

value_t adm_bp_node_get_blk_res_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentCount[0];

	return result;
}

value_t adm_bp_node_get_norm_res_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentCount[1];

	return result;
}

value_t adm_bp_node_get_exp_res_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentCount[2];

	return result;
}


value_t adm_bp_node_get_blk_res_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentBytes[0];

	return result;
}

value_t adm_bp_node_get_norm_res_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentBytes[1];

	return result;
}

value_t adm_bp_node_get_exp_res_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.currentResidentBytes[2];

	return result;
}

value_t adm_bp_node_get_bundles_frag(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundlesFragmented;

	return result;
}

value_t adm_bp_node_get_frag_produced(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.fragmentsProduced;

	return result;
}


value_t adm_bp_node_get_del_none(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delNoneCount;

	return result;
}

value_t adm_bp_node_get_del_expired(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delExpiredCount;

	return result;
}

value_t adm_bp_node_get_del_fwd_uni(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delFwdUnidirCount;

	return result;
}

value_t adm_bp_node_get_del_cancel(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delCanceledCount;

	return result;
}

value_t adm_bp_node_get_del_deplete(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delDepletionCount;

	return result;
}

value_t adm_bp_node_get_del_bad_eid(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delEidMalformedCount;

	return result;
}


value_t adm_bp_node_get_del_no_route(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delNoRouteCount;

	return result;
}

value_t adm_bp_node_get_del_no_contact(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delNoContactCount;

	return result;
}

value_t adm_bp_node_get_del_bad_blk(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.delBlkMalformedCount;

	return result;
}

value_t adm_bp_node_get_del_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bytesDeletedToDate;

	return result;
}

value_t adm_bp_node_get_fail_cust_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.custodyRefusedCount;

	return result;
}

value_t adm_bp_node_get_fail_cust_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.custodyRefusedBytes;

	return result;
}


value_t adm_bp_node_get_fail_fwd_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleFwdFailedCount;

	return result;
}

value_t adm_bp_node_get_fail_fwd_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleFwdFailedBytes;

	return result;
}


value_t adm_bp_node_get_fail_abandon_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleAbandonCount;

	return result;
}

value_t adm_bp_node_get_fail_abandon_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleAbandonBytes;

	return result;
}


value_t adm_bp_node_get_fail_discard_cnt(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleDiscardCount;

	return result;
}

value_t adm_bp_node_get_fail_discard_bytes(tdc_t params)
{
	value_t result;
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	result.type = AMP_TYPE_UVAST;
	result.value.as_uvast = state.bundleDiscardBytes;

	return result;
}


value_t adm_bp_endpoint_get_names(tdc_t params)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	value_t result;
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

	return result;
}


value_t adm_bp_endpoint_get_name(tdc_t params)
{
	char *name = NULL;
	value_t result;
	uint32_t size = 0;
	NmbpEndpoint endpoint;
	int success = 0;
	int8_t adm_success = 0;

	val_init(&result);


	if((name = adm_extract_string(params, 0, &adm_success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_bp_endpoint_get_name","No endpoint name given.", NULL);
		return result;
	}

	result.value.as_ptr = NULL;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.type = AMP_TYPE_STRING;

		size = (uint64_t) strlen(endpoint.eid) + 1;
		result.value.as_ptr = (uint8_t*) STAKE(size);
		if (result.value.as_ptr == NULL)
		{
			putErrmsg("Can't create result.value.as_ptr.", NULL);
			SRELEASE(name);
			return result;
		}

		memset(result.value.as_ptr,0,size);
		memcpy(result.value.as_ptr, endpoint.eid, size);
	}

	SRELEASE(name);

	return result;
}

value_t adm_bp_endpoint_get_active(tdc_t params)
{
	char *name = NULL;
	value_t result;

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

	return result;
}

value_t adm_bp_endpoint_get_singleton(tdc_t params)
{
	char *name = NULL;
	value_t result;
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
	return result;
}


value_t adm_bp_endpoint_get_abandon(tdc_t params)
{
	char *name = NULL;
	value_t result;
	NmbpEndpoint endpoint;
	int success = 0;
	int8_t adm_success = 0;

	val_init(&result);

	if((name = adm_extract_string(params, 0, &adm_success)) == NULL)
	{
		AMP_DEBUG_ERR("adm_bp_endpoint_get_abandon","Can't extract first parm.", NULL);
		return result;
	}

	result.value.as_uint = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success != 0)
	{
		result.type = AMP_TYPE_UINT;
		result.value.as_uint = endpoint.abandonOnDelivFailure;
	}

	SRELEASE(name);
	return result;
}



/* Controls */
tdc_t *adm_bp_ctrl_reset(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t result;

	bpnm_disposition_reset();
	*status = CTRL_SUCCESS;

	memset(&result, 0, sizeof(tdc_t));
	return NULL;
}


