/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

#ifdef _HAVE_ION_ADM_

/*****************************************************************************
 **
 ** File Name: adm_ion_priv.c
 **
 ** Description: This implements the private aspects of an ION ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/16/13  E. Birrane     Initial Implementation
 *****************************************************************************/
#include "ion.h"
#include "platform.h"


#include "../shared/adm/adm_ion.h"
#include "../shared/utils/utils.h"

#include "adm_ion_priv.h"

void agent_adm_init_ion()
{
	/* Register Nicknames */
	uint8_t mid_str[ADM_MID_ALLOC];

	/* ICI */
	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_sdr_state_all);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_small_pool_size);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_small_pool_free);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_small_pool_alloc);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_large_pool_size);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 5, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_large_pool_free);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 6, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_large_pool_alloc);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 7, mid_str);
	adm_add_datadef_collect(mid_str,  ion_ici_get_unused_size);



	/* Inducts */
	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_all);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_name);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_last_reset);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_rx_bndl);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_rx_byte);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 5, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_mal_bndl);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 6, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_mal_byte);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 7, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_inauth_bndl);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 8, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_inauth_byte);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 9, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_over_bndl);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 10, mid_str);
	adm_add_datadef_collect(mid_str,  ion_induct_get_over_byte);


	/* Outducts */
	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_all);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_name);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_cur_q_bdnl);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_cur_q_byte);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_last_reset);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 5, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_enq_bndl);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 6, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_enq_byte);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 7, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_deq_bndl);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 8, mid_str);
	adm_add_datadef_collect(mid_str,  ion_outduct_get_deq_byte);



	/* Node */

	adm_build_mid_str(0x00, ION_ADM_NODE_NN, ION_ADM_NODE_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str,  ion_node_get_all);

	adm_build_mid_str(0x00, ION_ADM_NODE_NN, ION_ADM_NODE_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str,  ion_node_get_inducts);

	adm_build_mid_str(0x00, ION_ADM_NODE_NN, ION_ADM_NODE_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str,  ion_node_get_outducts);


	/* Controls */
	adm_build_mid_str(0x01, ION_ADM_CTRL_NN, ION_ADM_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl_run(mid_str,  ion_ctrl_induct_reset);

	adm_build_mid_str(0x01, ION_ADM_CTRL_NN, ION_ADM_CTRL_NN_LEN, 1, mid_str);
	adm_add_ctrl_run(mid_str,  ion_ctrl_outduct_reset);
}


/* Retrieval Functions. */

expr_result_t ion_ici_get_sdr_state_all(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	result.length = sizeof(state);
	result.value = (uint8_t*) STAKE(result.length);
	sdrnm_state_get(&state);

	memcpy(result.value, &state, result.length);
	return result;
}

expr_result_t ion_ici_get_small_pool_size(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.smallPoolSize), sizeof(state.smallPoolSize), &(result.length));

	return result;
}

expr_result_t ion_ici_get_small_pool_free(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.smallPoolFree), sizeof(state.smallPoolFree), &(result.length));

	return result;
}

expr_result_t ion_ici_get_small_pool_alloc(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.smallPoolAllocated), sizeof(state.smallPoolAllocated), &(result.length));

	return result;
}

expr_result_t ion_ici_get_large_pool_size(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.largePoolSize), sizeof(state.largePoolSize), &(result.length));

	return result;
}

expr_result_t ion_ici_get_large_pool_free(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.largePoolFree), sizeof(state.largePoolFree), &(result.length));

	return result;
}

expr_result_t ion_ici_get_large_pool_alloc(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.largePoolAllocated), sizeof(state.largePoolAllocated), &(result.length));

	return result;
}

expr_result_t ion_ici_get_unused_size(Lyst params)
{
	SdrnmState state;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	sdrnm_state_get(&state);
	result.value = adm_copy_integer((uint8_t*)&(state.unusedSize), sizeof(state.unusedSize), &(result.length));

	return result;
}



/* ION INDUCT */


expr_result_t ion_induct_get_all(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	NmbpInduct induct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.length = sizeof(NmbpInduct);
		result.value = (uint8_t*) STAKE(result.length);
		memset(result.value, 0, result.length);
		memcpy(result.value, &induct, result.length);
	}

	return result;
}

expr_result_t ion_induct_get_name(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_STRING;

	NmbpInduct induct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.length = (uint64_t) strlen(induct.inductName) + 1;
		result.value = (uint8_t*) STAKE(result.length);
		memset(result.value, 0, result.length);
		memcpy(result.value, induct.inductName, result.length);
	}

	return result;
}

expr_result_t ion_induct_get_last_reset(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.lastResetTime),
			                 sizeof(induct.lastResetTime),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_induct_get_rx_bndl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleRecvCount),
			                 sizeof(induct.bundleRecvCount),
			                 &(result.length));
	}
	return result;
}


expr_result_t ion_induct_get_rx_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleRecvBytes),
			                 sizeof(induct.bundleRecvBytes),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_induct_get_mal_bndl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleMalformedCount),
			                 sizeof(induct.bundleMalformedCount),
			                 &(result.length));
	}
	return result;
}


expr_result_t ion_induct_get_mal_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleMalformedBytes),
			                 sizeof(induct.bundleMalformedBytes),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_induct_get_inauth_bndl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleInauthenticCount),
			                 sizeof(induct.bundleInauthenticCount),
			                 &(result.length));
	}
	return result;
}


expr_result_t ion_induct_get_inauth_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleInauthenticBytes),
			                 sizeof(induct.bundleInauthenticBytes),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_induct_get_over_bndl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleOverflowCount),
			                 sizeof(induct.bundleOverflowCount),
			                 &(result.length));
	}
	return result;
}


expr_result_t ion_induct_get_over_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_induct_get(name, &induct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(induct.bundleOverflowBytes),
			                 sizeof(induct.bundleOverflowBytes),
			                 &(result.length));
	}
	return result;
}


/* ION NODE */
expr_result_t ion_node_get_all(Lyst params)
{
	expr_result_t inducts;
	expr_result_t outducts;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	inducts = ion_node_get_inducts(params);
	outducts = ion_node_get_outducts(params);

	result.length = inducts.length + outducts.length;
	result.value = (uint8_t*) STAKE(result.length);
	memcpy(result.value,inducts.value,inducts.length);
	memcpy(result.value + inducts.length, outducts.value, outducts.length);
	expr_release(inducts);
	expr_release(outducts);

	return result;
}

expr_result_t ion_node_get_inducts(Lyst params)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	uint8_t *cursor = NULL;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	bpnm_inductNames_get((char *) names, ptrs, &num);

	encodeSdnv(&nm_sdnv, num);

	result.length = nm_sdnv.length +        /* NUM as SDNV length */
			        strlen(ptrs[num-1]) +   /* length of last string */
			        (ptrs[num-1] - names) + /* # bytes to get to last string */
			        1;                      /* Final NULL terminator. */
	result.value = (uint8_t *) STAKE(result.length);

	cursor = result.value;

	memcpy(cursor,nm_sdnv.text, nm_sdnv.length);
	cursor += nm_sdnv.length;

	memcpy(cursor, names, result.length - nm_sdnv.length);

	return result;
}

expr_result_t ion_node_get_outducts(Lyst params)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	uint8_t *cursor = NULL;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	result.length = 0;
	result.value = NULL;

	bpnm_outductNames_get((char *) names, ptrs, &num);

	encodeSdnv(&nm_sdnv, num);

	result.length = nm_sdnv.length +        /* NUM as SDNV length */
			        strlen(ptrs[num-1]) +   /* length of last string */
			        (ptrs[num-1] - names) + /* # bytes to get to last string */
			        1;                      /* Final NULL terminator. */
	result.value = (uint8_t *) STAKE(result.length);

	cursor = result.value;

	memcpy(cursor,nm_sdnv.text, nm_sdnv.length);
	cursor += nm_sdnv.length;

	memcpy(cursor, names, result.length - nm_sdnv.length);

	return result;

}


/* ION OUTDUCT */


expr_result_t ion_outduct_get_all(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	NmbpOutduct outduct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.length = sizeof(NmbpInduct);
		result.value = (uint8_t*) STAKE(result.length);
		memset(result.value, 0, result.length);
		memcpy(result.value, &outduct, result.length);
	}

	return result;
}

expr_result_t ion_outduct_get_name(Lyst params)
{

	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	expr_result_t result;
	result.type = EXPR_TYPE_STRING;

	result.length = 0;
	result.value = NULL;

	NmbpOutduct outduct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.length = (uint64_t) strlen(outduct.outductName) + 1;
		result.value = (uint8_t*) STAKE(result.length);
		memset(result.value, 0, result.length);
		memcpy(result.value, outduct.outductName, result.length);
	}

	return result;
}

expr_result_t ion_outduct_get_cur_q_bdnl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.currentQueuedBundlesCount),
			                 sizeof(outduct.currentQueuedBundlesCount),
			                 &(result.length));
	}

	return result;
}

expr_result_t ion_outduct_get_cur_q_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.currentQueuedBundlesBytes),
			                 sizeof(outduct.currentQueuedBundlesBytes),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_outduct_get_last_reset(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.lastResetTime),
			                 sizeof(outduct.lastResetTime),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_outduct_get_enq_bndl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.bundleEnqueuedCount),
			                 sizeof(outduct.bundleEnqueuedCount),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_outduct_get_enq_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.bundleEnqueuedBytes),
			                 sizeof(outduct.bundleEnqueuedBytes),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_outduct_get_deq_bndl(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.bundleDequeuedCount),
			                 sizeof(outduct.bundleDequeuedCount),
			                 &(result.length));
	}
	return result;
}

expr_result_t ion_outduct_get_deq_byte(Lyst params)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	result.length = 0;
	result.value = NULL;
	bpnm_outduct_get(name, &outduct, &success);
	if(success != 0)
	{
		result.value = adm_copy_integer((uint8_t*)&(outduct.bundleDequeuedBytes),
			                 sizeof(outduct.bundleDequeuedBytes),
			                 &(result.length));
	}
	return result;
}



uint32_t ion_ctrl_induct_reset(Lyst params)
{
 /* TODO: Implement. */
	return 0;
}

uint32_t ion_ctrl_outduct_reset(Lyst params)
{
	/* TODO: Implement */
	return 0;
}

#endif /* _HAVE_ION_ADM_ */
