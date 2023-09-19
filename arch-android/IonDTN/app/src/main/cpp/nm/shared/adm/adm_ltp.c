/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm_ion_priv.h
 **
 ** Description: This file contains the definitions of the LTP
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
#ifdef _HAVE_LTP_ADM_

#include "platform.h"
#include "ion.h"

#include "../utils/utils.h"

#include "../adm/adm_ltp.h"

void adm_ltp_init()
{

	uint8_t mid_str[ADM_MID_ALLOC];

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 0, mid_str);
	adm_add_datadef("LTP_NODE_RESOURCES_ALL", mid_str, 0, ltp_print_node_resources_all, ltp_size_node_resources_all);

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 1, mid_str);
	adm_add_datadef("LTP_HEAP_BYTES_RSV",     mid_str, 0, NULL, ltp_size_heap_bytes_reserved);

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 2, mid_str);
	adm_add_datadef("LTP_HEAD_BYTES_USED",    mid_str, 0, NULL, ltp_size_heap_bytes_used);

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 3, mid_str);
	adm_add_datadef("LTP_ENGINE_IDS",         mid_str, 0, adm_print_unsigned_long_list, adm_size_unsigned_long_list);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 0, mid_str);
	adm_add_datadef("LTP_ENG_ALL",                mid_str, 1, adm_print_unsigned_long_list, ltp_engine_size);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 1, mid_str);
	adm_add_datadef("LTP_ENG_NUM",                mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 2, mid_str);
	adm_add_datadef("LTP_ENG_EXP_SESS",           mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 3, mid_str);
	adm_add_datadef("LTP_ENG_CUR_OUT_SEG",        mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 4, mid_str);
	adm_add_datadef("LTP_ENG_CUR_IMP_SESS",       mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 5, mid_str);
	adm_add_datadef("LTP_ENG_CUR_IN_SEG",         mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 6, mid_str);
	adm_add_datadef("LTP_ENG_LAST_RESET_TIME",    mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 7, mid_str);
	adm_add_datadef("LTP_ENG_OUT_SEG_Q_CNT",      mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 8, mid_str);
	adm_add_datadef("LTP_ENG_OUT_SEG_Q_BYTE",     mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 9, mid_str);
	adm_add_datadef("LTP_ENG_OUT_SEG_POP_CNT",    mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 10, mid_str);
	adm_add_datadef("LTP_ENG_OUT_SEG_POP_BYTE",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 11, mid_str);
	adm_add_datadef("LTP_ENG_OUT_CKP_XMIT_CNT",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 12, mid_str);
	adm_add_datadef("LTP_ENG_OUT_POS_ACK_RCV_CNT",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 13, mid_str);
	adm_add_datadef("LTP_ENG_OUT_NEG_ACK_RCV_CNT",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 14, mid_str);
	adm_add_datadef("LTP_ENG_OUT_CANC_RCV_CNT",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 15, mid_str);
	adm_add_datadef("LTP_ENG_OUT_CKP_REXMT_CNT",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 16, mid_str);
	adm_add_datadef("LTP_ENG_OUT_CANC_XMIT_CNT",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 17, mid_str);
	adm_add_datadef("LTP_ENG_OUT_COMPL_CNT",      mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 18, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_RCV_RED_CNT", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 19, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_RCV_RED_BYTE",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 20, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_RCV_GRN_CNT", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 21, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_RCV_GRN_BYTE",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 22, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_RDNDT_CNT",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 23, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_RDNDT_BYTE",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 24, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_MAL_CNT",     mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 25, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_MAL_BYTE",    mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 26, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_UNK_SND_CNT", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 27, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_UNK_SND_BYTE",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 28, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_UNK_CLI_CNT", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 29, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_UNK_CLI_BYTE",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 30, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_STRAY_CNT",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 31, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_STRAY_BYTE",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 32, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_MISCOL_CNT",  mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 33, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_MISCOL_BYTE", mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 34, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_CLSD_CNT",    mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 35, mid_str);
	adm_add_datadef("LTP_ENG_IN_SEG_CLSD_BYTE",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 36, mid_str);
	adm_add_datadef("LTP_ENG_IN_CKP_RCV_CNT",     mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 37, mid_str);
	adm_add_datadef("LTP_ENG_IN_POS_ACK_XMIT_CNT",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 38, mid_str);
	adm_add_datadef("LTP_ENG_IN_NEG_ACK_XMIT_CNT",mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 39, mid_str);
	adm_add_datadef("LTP_ENG_IN_CANC_XMIT_CNT",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 40, mid_str);
	adm_add_datadef("LTP_ENG_IN_ACK_REXMT_CNT",   mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 41, mid_str);
	adm_add_datadef("LTP_ENG_IN_CANC_RCV_CNT",    mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 42, mid_str);
	adm_add_datadef("LTP_ENG_IN_COMPL_CNT",       mid_str, 1, NULL, NULL);


	adm_build_mid_str(0x41, ADM_LTP_CTRL_NN, ADM_LTP_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl("LTP_ENG_RESET", mid_str, 1);
}


/* Print Functions */

char *ltp_print_node_resources_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{

	unsigned long bytes_reserved;
	unsigned long bytes_used;
	char *result;
	uint32_t temp_size = 0;

	temp_size = 2 * sizeof(unsigned long);
	memcpy(&bytes_reserved, buffer, sizeof(bytes_reserved));
	memcpy(&bytes_used, &(buffer[sizeof(bytes_reserved)]), sizeof(bytes_used));


	// Assume for now a 4 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	*str_len = (data_len * 5) + (25 * 2);
	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) STAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("ltpPrintNodeResourceAll","Can't allocate %d bytes", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(result,
			"\nheapBytesReserved = %ld\nheapBytesOccupied = %ld\n",
			bytes_reserved, bytes_used);

	return result;
}

char *ltp_engine_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{

}



/* SIZE */


uint32_t ltp_size_node_resources_all(uint8_t* buffer, uint64_t buffer_len)
{
	return (sizeof(unsigned long) * 2);
}

uint32_t ltp_size_heap_bytes_reserved(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(unsigned long);
}

uint32_t ltp_size_heap_bytes_used(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(unsigned long);
}

uint32_t ltp_engine_size(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(NmltpSpan);
}


#endif /* _HAVE_LTP_ADM */
