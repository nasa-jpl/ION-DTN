#include "ion.h"
#include "platform.h"


#include "shared/adm/adm_ion.h"
#include "shared/utils/utils.h"



void initIonAdm()
{
	/* Register Nicknames */


	/* ICI */
	adm_add("ICI_SDR_STATE_ALL",   ION_ICI_SDR_STATE_ALL,   0, iciGetSdrStateAll,   iciPrintSdrStateAll,     iciSizeSdrStateAll);
	adm_add("ICI_SMALL_POOL_SIZE", ION_ICI_SMALL_POOL_SIZE, 0, iciGetSmallPoolSize, adm_print_unsigned_long, iciSizeSmallPoolSize);
	adm_add("ICI_SMALL_POOL_FREE", ION_ICI_SMALL_POOL_FREE, 0, iciGetSmallPoolFree, adm_print_unsigned_long, iciSizeSmallPoolFree);
	adm_add("ICI_SMALL_POOL_ALLOC",ION_ICI_SMALL_POOL_ALLOC,0, iciGetSmallPoolAlloc,adm_print_unsigned_long, iciSizeSmallPoolAlloc);
	adm_add("ICI_LARGE_POOL_SIZE", ION_ICI_LARGE_POOL_SIZE, 0, iciGetLargePoolSize, adm_print_unsigned_long, iciSizeLargePoolSize);
	adm_add("ICI_LARGE_POOL_FREE", ION_ICI_LARGE_POOL_FREE, 0, iciGetLargePoolFree, adm_print_unsigned_long, iciSizeLargePoolFree);
	adm_add("ICI_LARGE_POOL_ALLOC",ION_ICI_LARGE_POOL_ALLOC,0, iciGetLargePoolAlloc,adm_print_unsigned_long, iciSizeLargePoolAlloc);
	adm_add("ICI_UNUSED_SIZE",     ION_ICI_UNUSED_SIZE,     0, iciGetUnusedSize,    adm_print_unsigned_long, iciSizeUnusedSize);

	/* Inducts */
	adm_add("ICI_INDUCT_ALL",             ION_INDUCT_ALL,             1, ion_induct_get_all,         ion_induct_print_all,    ion_induct_size_all);
	adm_add("ION_INDUCT_NAME",            ION_INDUCT_NAME,            1, ion_induct_get_name,        adm_print_string,        adm_size_string);
	adm_add("ION_INDUCT_LAST_RESET",      ION_INDUCT_LAST_RESET,      1, ion_induct_get_last_reset,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_RX_BUNDLES",      ION_INDUCT_RX_BUNDLES,      1, ion_induct_get_rx_bndl,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_RX_BYTES",        ION_INDUCT_RX_BYTES,        1, ion_induct_get_rx_byte,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_MAL_BUNDLES",     ION_INDUCT_MAL_BUNDLES,     1, ion_induct_get_mal_bndl,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_MAL_BYTES",       ION_INDUCT_MAL_BYTES,       1, ion_induct_get_mal_byte,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_INAUTH_BUNDLES",  ION_INDUCT_INAUTH_BUNDLES,  1, ion_induct_get_inauth_bndl, adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_INAUTH_BYTES",    ION_INDUCT_INAUTH_BYTES,    1, ion_induct_get_inauth_byte, adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_OVERFLOW_BUNDLES",ION_INDUCT_OVERFLOW_BUNDLES,1, ion_induct_get_over_bndl,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_INDUCT_OVERFLOW_BYTES",  ION_INDUCT_OVERFLOW_BYTES,  1, ion_induct_get_over_byte,   adm_print_unsigned_long, adm_size_unsigned_long);


	/* Outducts */
	adm_add("ION_OUTDUCT_ALL",              ION_OUTDUCT_ALL,              1, ion_outduct_get_all,       ion_outduct_print_all,   ion_outduct_size_all);
	adm_add("ION_OUTDUCT_NAME",             ION_OUTDUCT_NAME,             1, ion_outduct_get_name,      adm_print_string,        adm_size_string);
	adm_add("ION_OUTDUCT_CUR_QUEUE_BUNDLES",ION_OUTDUCT_CUR_QUEUE_BUNDLES,1, ion_outduct_get_cur_q_bdnl,adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_OUTDUCT_CUR_QUEUE_BYTES",  ION_OUTDUCT_CUR_QUEUE_BYTES,  1, ion_outduct_get_cur_q_byte,adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_OUTDUCT_LAST_RESET",       ION_OUTDUCT_LAST_RESET,       1, ion_outduct_get_last_reset,adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_OUTDUCT_ENQUEUED_BUNDLES", ION_OUTDUCT_ENQUEUED_BUNDLES, 1, ion_outduct_get_enq_bndl,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_OUTDUCT_ENQUEUED_BYTES",   ION_OUTDUCT_ENQUEUED_BYTES,   1, ion_outduct_get_enq_byte,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_OUTDUCT_DEQUEUED_BUNDLES", ION_OUTDUCT_DEQUEUED_BUNDLES, 1, ion_outduct_get_deq_bndl,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("ION_OUTDUCT_DEQUEUED_BYTES",   ION_OUTDUCT_DEQUEUED_BYTES,   1, ion_outduct_get_deq_byte,  adm_print_unsigned_long, adm_size_unsigned_long);



	/* Node */
	adm_add("ION_NODE_ALL",     ION_NODE_ALL,     0,ion_node_get_all,     ion_node_print_all,   ion_node_size_all);
	adm_add("ION_NODE_INDUCTS", ION_NODE_INDUCTS, 0,ion_node_get_inducts, adm_print_string_list,adm_size_string_list);
	adm_add("ION_NODE_OUTDUCTS",ION_NODE_OUTDUCTS,0,ion_node_get_outducts,adm_print_string_list,adm_size_string_list);

}

/* Custom Print Functions. */

char *ion_node_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	char *result = NULL;
	char *cursor = NULL;
	char *inducts = NULL;
	char *outducts = NULL;
	uint32_t induct_len = 0;
	uint32_t outduct_len = 0;
	uint32_t tmp = 0;

	induct_len = adm_size_string_list(buffer, buffer_len);
	inducts = adm_print_string_list(buffer, buffer_len, data_len, &tmp);
	outduct_len = adm_size_string_list(buffer, buffer_len);
	outducts = adm_print_string_list(buffer+induct_len, buffer_len-induct_len, data_len, &tmp);

	*str_len = induct_len + outduct_len + 25;
	result = (char*) MTAKE(*str_len);
	cursor = result;

	sprintf(cursor, "inducts: %s\noutducts: %s\n",inducts, outducts);
	MRELEASE(inducts);
	MRELEASE(outducts);

	return result;
}

char *iciPrintSdrStateAll(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	SdrnmState state;

	char *result;
	uint32_t temp_size = 0;

	// \todo: EJB Check sizes!
	memcpy(&state, buffer, data_len);

	// Assume for now a 4 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 7 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("iciPrintSdrStateAll","Can;t allocate %d bytes", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(result,
			"\nsmallPoolSize = %ld\nsmallPoolFree = %ld\nsmallPoolAllocated = %ld\n \
largePoolSize = %ld\nlargePoolFree = %ld\nlargePoolAllocated = %ld\n \
unusedSize = %ld\n",state.smallPoolSize, state.smallPoolFree, state.smallPoolAllocated,
			state.largePoolSize, state.largePoolFree, state.largePoolAllocated, state.unusedSize);

	return result;
}

char *ion_induct_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	NmbpInduct induct;

	char *result;
	uint32_t temp_size = 0;

	// \todo: EJB Check sizes!
	memcpy(&induct, buffer, data_len);

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 9 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100) + strlen(induct.inductName);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ion_induct_print_all","Can't allocate %d bytes", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(result,
			"\ninductName = %s\nlastResetTime = %ld\nbundleRecvCount = %ld\n\
bundleMalformedCount = %ld\nbundleMalformedBytes = %ld\nbundleInauthenticCount = %ld\n\
bundleInauthenticBytes = %ld\nbundleOverflowCount = %ld\nbundleOverflowBytes\n",
            induct.inductName, induct.lastResetTime, induct.bundleRecvCount,
            induct.bundleRecvBytes, induct.bundleMalformedCount, induct.bundleMalformedBytes,
            induct.bundleInauthenticCount, induct.bundleInauthenticBytes,
            induct.bundleOverflowCount, induct.bundleOverflowBytes);

	return result;
}

char *ion_outduct_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{

	NmbpOutduct outduct;

	char *result;
	uint32_t temp_size = 0;

	// \todo: EJB Check sizes!
	memcpy(&outduct, buffer, data_len);

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 7 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100) + strlen(outduct.outductName);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ion_outduct_print_all","Can't allocate %d bytes", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(result,
			"\noutductName = %s\ncurrentQueuedBundlesCount = %ld\ncurrentQueuedBundlesBytes = %ld\n\
lastResetTime = %ld\nbundleEnqueuedCount = %ld\nbundleEnqueuedBytes = %ld\n\
bundleDequeuedCount = %ld\nbundleDequeuedBytes = %ld\n",
            outduct.outductName, outduct.currentQueuedBundlesCount, outduct.currentQueuedBundlesBytes,
            outduct.lastResetTime, outduct.bundleEnqueuedCount, outduct.bundleEnqueuedBytes,
            outduct.bundleDequeuedCount, outduct.bundleDequeuedBytes);

	return result;
}


/* Retrieval Functions. */

uint8_t *iciGetSdrStateAll(Lyst params, uint64_t *length)
{
	SdrnmState state;
	uint8_t *result = NULL;

	*length = sizeof(state);
	result = (uint8_t*) MTAKE(*length);
	sdrnm_state_get(&state);

	memcpy(result, &state, *length);
	return result;
}

uint8_t *iciGetSmallPoolSize(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.smallPoolSize), sizeof(state.smallPoolSize), length);
}

uint8_t *iciGetSmallPoolFree(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.smallPoolFree), sizeof(state.smallPoolFree), length);
}

uint8_t *iciGetSmallPoolAlloc(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.smallPoolAllocated), sizeof(state.smallPoolAllocated), length);
}

uint8_t *iciGetLargePoolSize(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.largePoolSize), sizeof(state.largePoolSize), length);
}

uint8_t *iciGetLargePoolFree(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.largePoolFree), sizeof(state.largePoolFree), length);
}

uint8_t *iciGetLargePoolAlloc(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.largePoolAllocated), sizeof(state.largePoolAllocated), length);
}

uint8_t *iciGetUnusedSize(Lyst params, uint64_t *length)
{
	SdrnmState state;
	sdrnm_state_get(&state);
	return adm_copy_integer((uint8_t*)&(state.unusedSize), sizeof(state.unusedSize), length);
}



/* ION INDUCT */


uint8_t *ion_induct_get_all(Lyst params, uint64_t *length)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	uint8_t *result = NULL;

	NmbpInduct induct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	*length = sizeof(NmbpInduct);
	result = (uint8_t*) MTAKE(*length);
	memset(result,0,*length);
	memcpy(result, &induct, *length);
}

uint8_t *ion_induct_get_name(Lyst params, uint64_t *length)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	uint8_t *result = NULL;

	NmbpInduct induct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	*length = (uint64_t) strlen(induct.inductName) + 1;
	result = (uint8_t*) MTAKE(*length);
	memset(result,0,*length);
	memcpy(result, induct.inductName, *length);

	return result;
}

uint8_t *ion_induct_get_last_reset(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.lastResetTime),
			                 sizeof(induct.lastResetTime),
			                 length);
}

uint8_t *ion_induct_get_rx_bndl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleRecvCount),
			                 sizeof(induct.bundleRecvCount),
			                 length);
}


uint8_t *ion_induct_get_rx_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleRecvBytes),
			                 sizeof(induct.bundleRecvBytes),
			                 length);
}

uint8_t *ion_induct_get_mal_bndl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleMalformedCount),
			                 sizeof(induct.bundleMalformedCount),
			                 length);
}


uint8_t *ion_induct_get_mal_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleMalformedBytes),
			                 sizeof(induct.bundleMalformedBytes),
			                 length);
}

uint8_t *ion_induct_get_inauth_bndl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleInauthenticCount),
			                 sizeof(induct.bundleInauthenticCount),
			                 length);
}


uint8_t *ion_induct_get_inauth_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleInauthenticBytes),
			                 sizeof(induct.bundleInauthenticBytes),
			                 length);
}

uint8_t *ion_induct_get_over_bndl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleOverflowCount),
			                 sizeof(induct.bundleOverflowCount),
			                 length);
}


uint8_t *ion_induct_get_over_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpInduct induct;
	int success = 0;

	*length = 0;
	bpnm_induct_get(name, &induct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(induct.bundleOverflowBytes),
			                 sizeof(induct.bundleOverflowBytes),
			                 length);
}


/* ION NODE */
uint8_t *ion_node_get_all(Lyst params, uint64_t *length)
{
	uint8_t *inducts = NULL;
	uint8_t *outducts = NULL;
	uint64_t induct_len = 0;
	uint64_t outduct_len = 0;
	uint8_t *result = NULL;

	inducts = ion_node_get_inducts(params, &induct_len);
	outducts = ion_node_get_outducts(params, &outduct_len);

	*length = induct_len + outduct_len;
	result = (uint8_t*) MTAKE(*length);
	memcpy(result,inducts,induct_len);
	memcpy(result+induct_len, outducts, outduct_len);
	MRELEASE(inducts);
	MRELEASE(outducts);

	return result;
}

uint8_t *ion_node_get_inducts(Lyst params, uint64_t *length)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	bpnm_inductNames_get((char *) names, ptrs, &num);

	encodeSdnv(&nm_sdnv, num);

	*length = nm_sdnv.length +             /* NUM as SDNV length */
			  strlen(ptrs[num-1]) + /* length of last string */
			  (ptrs[num-1] - names) +      /* # bytes to get to last string */
			  1;                           /* Final NULL terminator. */
	result = (uint8_t *) MTAKE(*length);

	cursor = result;

	memcpy(cursor,nm_sdnv.text, nm_sdnv.length);
	cursor += nm_sdnv.length;

	memcpy(cursor, names, *length - nm_sdnv.length);

	return result;
}

uint8_t *ion_node_get_outducts(Lyst params, uint64_t *length)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;

	bpnm_outductNames_get((char *) names, ptrs, &num);

	encodeSdnv(&nm_sdnv, num);

	*length = nm_sdnv.length +             /* NUM as SDNV length */
			  strlen(ptrs[num-1]) + /* length of last string */
			  (ptrs[num-1] - names) +      /* # bytes to get to last string */
			  1;                           /* Final NULL terminator. */
	result = (uint8_t *) MTAKE(*length);

	cursor = result;

	memcpy(cursor,nm_sdnv.text, nm_sdnv.length);
	cursor += nm_sdnv.length;

	memcpy(cursor, names, *length - nm_sdnv.length);

	return result;

}


/* ION OUTDUCT */


uint8_t *ion_outduct_get_all(Lyst params, uint64_t *length)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	uint8_t *result = NULL;

	NmbpOutduct outduct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	*length = sizeof(NmbpInduct);
	result = (uint8_t*) MTAKE(*length);
	memset(result,0,*length);
	memcpy(result, &outduct, *length);

}

uint8_t *ion_outduct_get_name(Lyst params, uint64_t *length)
{

	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	uint8_t *result = NULL;

	NmbpOutduct outduct;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	*length = (uint64_t) strlen(outduct.outductName) + 1;
	result = (uint8_t*) MTAKE(*length);
	memset(result,0,*length);
	memcpy(result, outduct.outductName, *length);

	return result;
}

uint8_t *ion_outduct_get_cur_q_bdnl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.currentQueuedBundlesCount),
			                 sizeof(outduct.currentQueuedBundlesCount),
			                 length);
}

uint8_t *ion_outduct_get_cur_q_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.currentQueuedBundlesBytes),
			                 sizeof(outduct.currentQueuedBundlesBytes),
			                 length);
}

uint8_t *ion_outduct_get_last_reset(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.lastResetTime),
			                 sizeof(outduct.lastResetTime),
			                 length);
}

uint8_t *ion_outduct_get_enq_bndl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.bundleEnqueuedCount),
			                 sizeof(outduct.bundleEnqueuedCount),
			                 length);
}

uint8_t *ion_outduct_get_enq_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.bundleEnqueuedBytes),
			                 sizeof(outduct.bundleEnqueuedBytes),
			                 length);
}

uint8_t *ion_outduct_get_deq_bndl(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.bundleDequeuedCount),
			                 sizeof(outduct.bundleDequeuedCount),
			                 length);
}

uint8_t *ion_outduct_get_deq_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpOutduct outduct;
	int success = 0;

	*length = 0;
	bpnm_outduct_get(name, &outduct, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(outduct.bundleDequeuedBytes),
			                 sizeof(outduct.bundleDequeuedBytes),
			                 length);
}


/* SIZE */

uint32_t ion_induct_size_all(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(NmbpInduct);
}

uint32_t ion_outduct_size_all(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(NmbpOutduct);
}

uint32_t ion_node_size_all(uint8_t* buffer, uint64_t buffer_len)
{
	uint32_t result = 0;

	result = adm_size_string_list(buffer, buffer_len);
	result += adm_size_string_list(buffer+result, buffer_len-result);
	return result;
}

uint32_t iciSizeSdrStateAll(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state);
}


uint32_t iciSizeSmallPoolSize(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.smallPoolSize);
}

uint32_t iciSizeSmallPoolFree(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.smallPoolFree);
}

uint32_t iciSizeSmallPoolAlloc(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.smallPoolAllocated);
}

uint32_t iciSizeLargePoolSize(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.largePoolSize);
}

uint32_t iciSizeLargePoolFree(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.largePoolFree);
}

uint32_t iciSizeLargePoolAlloc(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.largePoolAllocated);
}

uint32_t iciSizeUnusedSize(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state.unusedSize);
}

