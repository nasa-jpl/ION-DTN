#include "platform.h"
#include "ion.h"

#include "shared/utils/utils.h"

#include "shared/adm/adm_ltp.h"



void initLtpAdm()
{

	// Size is size of the size_field + 2 (1 byte for flags, 1 byte for size field)
	adm_add("LTP_NODE_RESOURCES_ALL", LTP_NODE_RESOURCES_ALL, 0, ltpGetNodeResourcesAll, ltpPrintNodeResourcesAll, ltpSizeNodeResourcesAll);

	adm_add("LTP_HEAP_BYTES_RSV",  LTP_HEAP_BYTES_RSV,  0, ltpGetHeapBytesReserved, adm_print_unsigned_long, ltpSizeHeapBytesReserved);
	adm_add("LTP_HEAD_BYTES_USED", LTP_HEAD_BYTES_USED, 0, ltpGetHeapBytesUsed,     adm_print_unsigned_long, ltpSizeHeapBytesUsed);
	adm_add("LTP_ENGINE_IDS",      LTP_ENGINE_IDS,      0, ltp_get_engines,         adm_print_unsigned_long_list, adm_size_unsigned_long_list);


	adm_add("LTP_ENG_ALL",                LTP_ENG_ALL,                 1, ltp_get_eng_all,                 adm_print_unsigned_long_list, ltp_engine_size);
	adm_add("LTP_ENG_NUM",                LTP_ENG_NUM,                 1, ltp_get_eng_num,                 adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_EXP_SESS",           LTP_ENG_EXP_SESS,            1, ltp_get_eng_exp_sess,            adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_CUR_OUT_SEG",        LTP_ENG_CUR_OUT_SEG,         1, ltp_get_eng_cur_out_seg,         adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_CUR_IMP_SESS",       LTP_ENG_CUR_IMP_SESS,        1, ltp_get_eng_cur_imp_sess,        adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_CUR_IN_SEG",         LTP_ENG_CUR_IN_SEG,          1, ltp_get_eng_cur_in_seg,          adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_LAST_RESET_TIME",    LTP_ENG_LAST_RESET_TIME,     1, ltp_get_eng_last_reset_time,     adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_SEG_Q_CNT",      LTP_ENG_OUT_SEG_Q_CNT,       1, ltp_get_eng_out_seg_q_cnt,       adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_SEG_Q_BYTE",     LTP_ENG_OUT_SEG_Q_BYTE,      1, ltp_get_eng_out_seg_q_byte,      adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_SEG_POP_CNT",    LTP_ENG_OUT_SEG_POP_CNT,     1, ltp_get_eng_out_seg_pop_cnt,     adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_SEG_POP_BYTE",   LTP_ENG_OUT_SEG_POP_BYTE,    1, ltp_get_eng_out_seg_pop_byte,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_CKP_XMIT_CNT",   LTP_ENG_OUT_CKP_XMIT_CNT,    1, ltp_get_eng_out_ckp_xmit_cnt,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_POS_ACK_RCV_CNT",LTP_ENG_OUT_POS_ACK_RCV_CNT, 1, ltp_get_eng_out_pos_ack_rcv_cnt, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_NEG_ACK_RCV_CNT",LTP_ENG_OUT_NEG_ACK_RCV_CNT, 1, ltp_get_eng_out_neg_ack_rcv_cnt, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_CANC_RCV_CNT",   LTP_ENG_OUT_CANC_RCV_CNT,    1, ltp_get_eng_out_canc_rcv_cnt,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_CKP_REXMT_CNT",  LTP_ENG_OUT_CKP_REXMT_CNT,   1, ltp_get_eng_out_ckp_rexmt_cnt,   adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_CANC_XMIT_CNT",  LTP_ENG_OUT_CANC_XMIT_CNT,   1, ltp_get_eng_out_canc_xmit_cnt,   adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_OUT_COMPL_CNT",      LTP_ENG_OUT_COMPL_CNT,       1, ltp_get_eng_out_compl_cnt,       adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_RCV_RED_CNT", LTP_ENG_IN_SEG_RCV_RED_CNT,  1, ltp_get_eng_in_seg_rcv_red_cnt,  adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_RCV_RED_BYTE",LTP_ENG_IN_SEG_RCV_RED_BYTE, 1, ltp_get_eng_in_seg_rcv_red_byte, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_RCV_GRN_CNT", LTP_ENG_IN_SEG_RCV_GRN_CNT,  1, ltp_get_eng_in_seg_rcv_grn_cnt,  adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_RCV_GRN_BYTE",LTP_ENG_IN_SEG_RCV_GRN_BYTE, 1, ltp_get_eng_in_seg_rcv_grn_byte, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_RDNDT_CNT",   LTP_ENG_IN_SEG_RDNDT_CNT,    1, ltp_get_eng_in_seg_rdndt_cnt,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_RDNDT_BYTE",  LTP_ENG_IN_SEG_RDNDT_BYTE,   1, ltp_get_eng_in_seg_rdndt_byte,   adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_MAL_CNT",     LTP_ENG_IN_SEG_MAL_CNT,      1, ltp_get_eng_in_seg_mal_cnt,      adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_MAL_BYTE",    LTP_ENG_IN_SEG_MAL_BYTE,     1, ltp_get_eng_in_seg_mal_byte,     adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_UNK_SND_CNT", LTP_ENG_IN_SEG_UNK_SND_CNT,  1, ltp_get_eng_in_seg_unk_snd_cnt,  adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_UNK_SND_BYTE",LTP_ENG_IN_SEG_UNK_SND_BYTE, 1, ltp_get_eng_in_seg_unk_snd_byte, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_UNK_CLI_CNT", LTP_ENG_IN_SEG_UNK_CLI_CNT,  1, ltp_get_eng_in_seg_unk_cli_cnt,  adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_UNK_CLI_BYTE",LTP_ENG_IN_SEG_UNK_CLI_BYTE, 1, ltp_get_eng_in_seg_unk_cli_byte, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_STRAY_CNT",   LTP_ENG_IN_SEG_STRAY_CNT,    1, ltp_get_eng_in_seg_stray_cnt,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_STRAY_BYTE",  LTP_ENG_IN_SEG_STRAY_BYTE,   1, ltp_get_eng_in_seg_stray_byte,   adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_MISCOL_CNT",  LTP_ENG_IN_SEG_MISCOL_CNT,   1, ltp_get_eng_in_seg_miscol_cnt,   adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_MISCOL_BYTE", LTP_ENG_IN_SEG_MISCOL_BYTE,  1, ltp_get_eng_in_seg_miscol_byte,  adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_CLSD_CNT",    LTP_ENG_IN_SEG_CLSD_CNT,     1, ltp_get_eng_in_seg_clsd_cnt,     adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_SEG_CLSD_BYTE",   LTP_ENG_IN_SEG_CLSD_BYTE,    1, ltp_get_eng_in_seg_clsd_byte,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_CKP_RCV_CNT",     LTP_ENG_IN_CKP_RCV_CNT,      1, ltp_get_eng_in_ckp_rcv_cnt,      adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_POS_ACK_XMIT_CNT",LTP_ENG_IN_POS_ACK_XMIT_CNT, 1, ltp_get_eng_in_pos_ack_xmit_cnt, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_NEG_ACK_XMIT_CNT",LTP_ENG_IN_NEG_ACK_XMIT_CNT, 1, ltp_get_eng_in_neg_ack_xmit_cnt, adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_CANC_XMIT_CNT",   LTP_ENG_IN_CANC_XMIT_CNT,    1, ltp_get_eng_in_canc_xmit_cnt,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_ACK_REXMT_CNT",   LTP_ENG_IN_ACK_REXMT_CNT,    1, ltp_get_eng_in_ack_rexmt_cnt,    adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_CANC_RCV_CNT",    LTP_ENG_IN_CANC_RCV_CNT,     1, ltp_get_eng_in_canc_rcv_cnt,     adm_print_unsigned_long_list, adm_size_unsigned_long);
	adm_add("LTP_ENG_IN_COMPL_CNT",       LTP_ENG_IN_COMPL_CNT,        1, ltp_get_eng_in_compl_cnt,        adm_print_unsigned_long_list, adm_size_unsigned_long);

}


/* Print Functions */

char *ltpPrintNodeResourcesAll(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
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
	if((result = (char *) MTAKE(*str_len)) == NULL)
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

/* Get Functions */

uint8_t *ltpGetNodeResourcesAll(Lyst params, uint64_t *length)
{
	unsigned long bytes_reserved = 0;
	unsigned long bytes_used = 0;
	uint8_t *result = NULL;

	ltpnm_resources(&bytes_reserved, &bytes_used);
	*length = sizeof(unsigned long) * 2;

	result = (uint8_t*) MTAKE(*length);

	memcpy(result, &bytes_reserved, sizeof(bytes_reserved));
	memcpy(&(result[sizeof(bytes_reserved)]), &bytes_used, sizeof(bytes_used));
	return result;

}

uint8_t *ltpGetHeapBytesReserved(Lyst params, uint64_t *length)
{
	unsigned long bytes_reserved = 0;
	unsigned long bytes_used = 0;
	uint8_t *result = NULL;

	ltpnm_resources(&bytes_reserved, &bytes_used);
	*length = sizeof(unsigned long);

	result = (uint8_t*) MTAKE(*length);

	memcpy(result, &bytes_reserved, sizeof(bytes_reserved));
	return result;
}


uint8_t *ltpGetHeapBytesUsed(Lyst params, uint64_t *length)
{
	unsigned long bytes_reserved = 0;
	unsigned long bytes_used = 0;
	uint8_t *result = NULL;

	ltpnm_resources(&bytes_reserved, &bytes_used);
	*length = sizeof(unsigned long);

	result = (uint8_t*) MTAKE(*length);

	memcpy(result, &bytes_used, sizeof(bytes_used));
	return result;
}



uint8_t *ltp_get_engines(Lyst params, uint64_t *length)
{
	unsigned int ids[32];
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;
	int num = 0;
	unsigned long val = 0;
	Sdnv num_sdnv;

	ltpnm_spanEngineIds_get(ids, &num);
	if(num > 32)
	{
		fprintf(stderr,"Uh oh!\n");
		exit(1);
	}

	encodeSdnv(&num_sdnv, num);

	*length = num_sdnv.length +             /* NUM as SDNV length */
			  (num * sizeof(unsigned long));

	result = (uint8_t *) MTAKE(*length);
	cursor = result;

	memcpy(cursor,num_sdnv.text, num_sdnv.length);
	cursor += num_sdnv.length;

	for(int i = 0; i < num; i++)
	{
		val = ids[i];
		memcpy(cursor,&val, sizeof(val));
		cursor += sizeof(val);
	}

	return result;
}

uint8_t *ltp_get_eng_all(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	result = (uint8_t*) MTAKE(sizeof(span));
	memcpy(result, &span, sizeof(span));
	*length = sizeof(span);
	return result;
}

uint8_t *ltp_get_eng_num(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.remoteEngineNbr;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_exp_sess(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.currentExportSessions;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_cur_out_seg(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.currentOutboundSegments;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_cur_imp_sess(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.currentImportSessions;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_cur_in_seg(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.currentInboundSegments;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_last_reset_time(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.lastResetTime;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_seg_q_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputSegQueuedCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_out_seg_q_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputSegQueuedBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_seg_pop_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputSegPoppedCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_seg_pop_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputSegPoppedBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_out_ckp_xmit_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputCkptXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_pos_ack_rcv_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputPosAckRecvCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_neg_ack_rcv_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputNegAckRecvCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_canc_rcv_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputCancelRecvCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_out_ckp_rexmt_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputCkptReXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_out_canc_xmit_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputCancelXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_out_compl_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.outputCompleteCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_in_seg_rcv_red_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegRecvRedCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_rcv_red_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegRecvRedBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_rcv_grn_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegRecvGreenCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_rcv_grn_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegRecvGreenBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_rdndt_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegRedundantCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_rdndt_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegRedundantBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_in_seg_mal_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegMalformedCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_mal_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegMalformedBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_unk_snd_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegUnkSenderCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_unk_snd_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegUnkSenderBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_in_seg_unk_cli_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegUnkClientCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_unk_cli_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegUnkClientBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_stray_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegStrayCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_stray_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegStrayBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_miscol_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegMiscolorCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_miscol_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegMiscolorBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_clsd_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegClosedCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_seg_clsd_byte(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputSegClosedBytes;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_in_ckp_rcv_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputCkptRecvCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


uint8_t *ltp_get_eng_in_pos_ack_xmit_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputPosAckXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_neg_ack_xmit_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputNegAckXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_canc_xmit_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputCancelXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_ack_rexmt_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputAckReXmitCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_canc_rcv_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputCancelRecvCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *ltp_get_eng_in_compl_cnt(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	*length = 0;

	ltpnm_span_get(engineId, &span, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = span.inputCompleteCount;
	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}


/* SIZE */


uint32_t ltpSizeNodeResourcesAll(uint8_t* buffer, uint64_t buffer_len)
{
	return (sizeof(unsigned long) * 2);
}

uint32_t ltpSizeHeapBytesReserved(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(unsigned long);
}

uint32_t ltpSizeHeapBytesUsed(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(unsigned long);
}

uint32_t ltp_engine_size(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(NmltpSpan);
}
