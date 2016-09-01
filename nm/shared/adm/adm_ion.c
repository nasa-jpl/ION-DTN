/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm_ion_priv.h
 **
 ** Description: This file contains the definitions of the ION
 **              ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 ** 	1. We current use a non-official OID root tree for DTN Bundle Protocol
 **         identifiers.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifdef _HAVE_ION_ADM_
#include "ion.h"
#include "platform.h"


#include "../adm/adm_ion.h"
#include "../utils/utils.h"


void adm_ion_init()
{
	/* Register Nicknames */
	uint8_t mid_str[ADM_MID_ALLOC];

	/* ICI */
	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 0, mid_str);
	adm_add_datadef("ICI_SDR_STATE_ALL",   mid_str, 0, ion_print_sdr_state_all,     ion_size_sdr_state_all);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 1, mid_str);
	adm_add_datadef("ICI_SMALL_POOL_SIZE", mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 2, mid_str);
	adm_add_datadef("ICI_SMALL_POOL_FREE", mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 3, mid_str);
	adm_add_datadef("ICI_SMALL_POOL_ALLOC",mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 4, mid_str);
	adm_add_datadef("ICI_LARGE_POOL_SIZE", mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 5, mid_str);
	adm_add_datadef("ICI_LARGE_POOL_FREE", mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 6, mid_str);
	adm_add_datadef("ICI_LARGE_POOL_ALLOC",mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ION_ADM_ICI_NN, ION_ADM_ICI_NN_LEN, 7, mid_str);
	adm_add_datadef("ICI_UNUSED_SIZE",     mid_str, 0, NULL, NULL);



	/* Inducts */
	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 0, mid_str);
	adm_add_datadef("ICI_INDUCT_ALL",             mid_str, 1, ion_induct_print_all,    ion_induct_size_all);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 1, mid_str);
	adm_add_datadef("ION_INDUCT_NAME",            mid_str, 1, adm_print_string,        adm_size_string);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 2, mid_str);
	adm_add_datadef("ION_INDUCT_LAST_RESET",      mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 3, mid_str);
	adm_add_datadef("ION_INDUCT_RX_BUNDLES",      mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 4, mid_str);
	adm_add_datadef("ION_INDUCT_RX_BYTES",        mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 5, mid_str);
	adm_add_datadef("ION_INDUCT_MAL_BUNDLES",     mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 6, mid_str);
	adm_add_datadef("ION_INDUCT_MAL_BYTES",       mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 7, mid_str);
	adm_add_datadef("ION_INDUCT_INAUTH_BUNDLES",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 8, mid_str);
	adm_add_datadef("ION_INDUCT_INAUTH_BYTES",    mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 9, mid_str);
	adm_add_datadef("ION_INDUCT_OVERFLOW_BUNDLES",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_INDUCT_NN, ION_ADM_INDUCT_NN_LEN, 10, mid_str);
	adm_add_datadef("ION_INDUCT_OVERFLOW_BYTES",  mid_str, 1, NULL, NULL);


	/* Outducts */
	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 0, mid_str);
	adm_add_datadef("ION_OUTDUCT_ALL",              mid_str, 1, ion_outduct_print_all,   ion_outduct_size_all);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 1, mid_str);
	adm_add_datadef("ION_OUTDUCT_NAME",             mid_str, 1, adm_print_string,        adm_size_string);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 2, mid_str);
	adm_add_datadef("ION_OUTDUCT_CUR_QUEUE_BUNDLES",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 3, mid_str);
	adm_add_datadef("ION_OUTDUCT_CUR_QUEUE_BYTES",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 4, mid_str);
	adm_add_datadef("ION_OUTDUCT_LAST_RESET",       mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 5, mid_str);
	adm_add_datadef("ION_OUTDUCT_ENQUEUED_BUNDLES", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 6, mid_str);
	adm_add_datadef("ION_OUTDUCT_ENQUEUED_BYTES",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 7, mid_str);
	adm_add_datadef("ION_OUTDUCT_DEQUEUED_BUNDLES", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ION_ADM_OUTDUCT_NN, ION_ADM_OUTDUCT_NN_LEN, 8, mid_str);
	adm_add_datadef("ION_OUTDUCT_DEQUEUED_BYTES",   mid_str, 1,  NULL, NULL);



	/* Node */

	adm_build_mid_str(0x00, ION_ADM_NODE_NN, ION_ADM_NODE_NN_LEN, 0, mid_str);
	adm_add_datadef("ION_NODE_ALL",     mid_str, 0, ion_node_print_all,    ion_node_size_all);

	adm_build_mid_str(0x00, ION_ADM_NODE_NN, ION_ADM_NODE_NN_LEN, 1, mid_str);
	adm_add_datadef("ION_NODE_INDUCTS", mid_str, 0, adm_print_string_list, adm_size_string_list);

	adm_build_mid_str(0x00, ION_ADM_NODE_NN, ION_ADM_NODE_NN_LEN, 2, mid_str);
	adm_add_datadef("ION_NODE_OUTDUCTS",mid_str, 0, adm_print_string_list, adm_size_string_list);


	/* Controls */
	adm_build_mid_str(0x01, ION_ADM_CTRL_NN, ION_ADM_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl("ION_INDUCT_RESET",  mid_str, 0);

	adm_build_mid_str(0x01, ION_ADM_CTRL_NN, ION_ADM_CTRL_NN_LEN, 1, mid_str);
	adm_add_ctrl("ION_OUTDUCT_RESET", mid_str, 0);
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
	result = (char*) STAKE(*str_len);
	cursor = result;

	sprintf(cursor, "inducts: %s\noutducts: %s\n",inducts, outducts);
	SRELEASE(inducts);
	SRELEASE(outducts);

	return result;
}

char *ion_print_sdr_state_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	SdrnmState state;

	char *result;
	uint32_t temp_size = 0;

	// \todo: Check sizes.
	memcpy(&state, buffer, data_len);

	// Assume for now a 4 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 7 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) STAKE(*str_len)) == NULL)
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

	// \todo: Check sizes.
	memcpy(&induct, buffer, data_len);

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 9 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100) + strlen(induct.inductName);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) STAKE(*str_len)) == NULL)
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

	// \todo: Check sizes.
	memcpy(&outduct, buffer, data_len);

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 7 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100) + strlen(outduct.outductName);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) STAKE(*str_len)) == NULL)
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

uint32_t ion_size_sdr_state_all(uint8_t* buffer, uint64_t buffer_len)
{
	SdrnmState state;
	return sizeof(state);
}


#endif /* _HAVE_ION_ADM_ */
