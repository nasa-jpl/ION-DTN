/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
#ifdef _HAVE_LTP_ADM_

/*****************************************************************************
 **
 ** File Name: adm_ltp_priv.c
 **
 ** Description: This implements the private aspects of a LTP ADM.
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
#include "platform.h"
#include "ion.h"

#include "../shared/utils/utils.h"

#include "../shared/adm/adm_ltp.h"

#include "adm_ltp_priv.h"


void agent_adm_init_ltp()
{

	uint8_t mid_str[ADM_MID_ALLOC];

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_node_resources_all);

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_heap_bytes_reserved);

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_heap_bytes_used);

	adm_build_mid_str(0x00, ADM_LTP_NODE_NN, ADM_LTP_NODE_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_engines);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_all);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_num);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_exp_sess);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_cur_out_seg);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_cur_imp_sess);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 5, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_cur_in_seg);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 6, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_last_reset_time);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 7, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_seg_q_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 8, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_seg_q_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 9, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_seg_pop_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 10, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_seg_pop_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 11, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_ckp_xmit_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 12, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_pos_ack_rcv_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 13, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_neg_ack_rcv_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 14, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_canc_rcv_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 15, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_ckp_rexmt_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 16, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_canc_xmit_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 17, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_out_compl_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 18, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_rcv_red_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 19, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_rcv_red_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 20, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_rcv_grn_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 21, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_rcv_grn_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 22, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_rdndt_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 23, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_rdndt_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 24, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_mal_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 25, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_mal_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 26, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_unk_snd_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 27, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_unk_snd_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 28, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_unk_cli_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 29, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_unk_cli_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 30, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_stray_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 31, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_stray_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 32, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_miscol_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 33, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_miscol_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 34, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_clsd_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 35, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_seg_clsd_byte);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 36, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_ckp_rcv_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 37, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_pos_ack_xmit_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 38, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_neg_ack_xmit_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 39, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_canc_xmit_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 40, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_ack_rexmt_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 41, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_canc_rcv_cnt);

	adm_build_mid_str(0x40, ADM_LTP_ENGINE_NN, ADM_LTP_ENGINE_NN_LEN, 42, mid_str);
	adm_add_datadef_collect(mid_str, ltp_get_eng_in_compl_cnt);


	adm_build_mid_str(0x41, ADM_LTP_CTRL_NN, ADM_LTP_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl_run(mid_str, ltp_engine_reset);

}

/* Get Functions */

expr_result_t ltp_get_node_resources_all(Lyst params)
{
	unsigned long bytes_reserved = 0;
	unsigned long bytes_used = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	ltpnm_resources(&bytes_reserved, &bytes_used);
	result.length = sizeof(unsigned long) * 2;

	result.value = (uint8_t*) STAKE(result.length);

	memcpy(result.value, &bytes_reserved, sizeof(bytes_reserved));
	memcpy(&(result.value[sizeof(bytes_reserved)]), &bytes_used, sizeof(bytes_used));
	return result;
}

expr_result_t ltp_get_heap_bytes_reserved(Lyst params)
{
	unsigned long bytes_reserved = 0;
	unsigned long bytes_used = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	ltpnm_resources(&bytes_reserved, &bytes_used);
	result.length = sizeof(unsigned long);

	result.value = (uint8_t*) STAKE(result.length);

	memcpy(result.value, &bytes_reserved, sizeof(bytes_reserved));
	return result;
}


expr_result_t ltp_get_heap_bytes_used(Lyst params)
{
	unsigned long bytes_reserved = 0;
	unsigned long bytes_used = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	ltpnm_resources(&bytes_reserved, &bytes_used);
	result.length = sizeof(unsigned long);

	result.value = (uint8_t*) STAKE(result.length);

	memcpy(result.value, &bytes_used, sizeof(bytes_used));
	return result;
}



expr_result_t ltp_get_engines(Lyst params)
{
	unsigned int ids[32];
	uint8_t *cursor = NULL;
	int num = 0;
	unsigned long val = 0;
	Sdnv num_sdnv;
	expr_result_t result;
	result.type = EXPR_TYPE_BLOB;

	ltpnm_spanEngineIds_get(ids, &num);
	if(num > 32)
	{
		fprintf(stderr,"We do not support more than 32 engines. Aborting.\n");
		exit(1);
	}

	encodeSdnv(&num_sdnv, num);

	result.length = num_sdnv.length +             /* NUM as SDNV length */
			  (num * sizeof(unsigned long));

	result.value = (uint8_t *) STAKE(result.length);
	cursor = result.value;

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

expr_result_t ltp_get_eng_all(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;
	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		result.value = (uint8_t*) STAKE(sizeof(span));
		memcpy(result.value, &span, sizeof(span));
		result.length = sizeof(span);
	}

	return result;
}

expr_result_t ltp_get_eng_num(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.remoteEngineNbr;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_exp_sess(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.currentExportSessions;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_cur_out_seg(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;
	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.currentOutboundSegments;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_cur_imp_sess(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.currentImportSessions;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_cur_in_seg(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.currentInboundSegments;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}

	return result;
}

expr_result_t ltp_get_eng_last_reset_time(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.lastResetTime;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_out_seg_q_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputSegQueuedCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}

	return result;
}


expr_result_t ltp_get_eng_out_seg_q_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputSegQueuedBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}

	return result;
}

expr_result_t ltp_get_eng_out_seg_pop_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputSegPoppedCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_out_seg_pop_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputSegPoppedBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}

	return result;
}


expr_result_t ltp_get_eng_out_ckp_xmit_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputCkptXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}

	return result;
}

expr_result_t ltp_get_eng_out_pos_ack_rcv_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputPosAckRecvCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}

	return result;
}

expr_result_t ltp_get_eng_out_neg_ack_rcv_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputNegAckRecvCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_out_canc_rcv_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputCancelRecvCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_out_ckp_rexmt_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputCkptReXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_out_canc_xmit_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputCancelXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_out_compl_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;
	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.outputCompleteCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_in_seg_rcv_red_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;
	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegRecvRedCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_rcv_red_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegRecvRedBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_rcv_grn_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	result.length = 0;
	result.value = NULL;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegRecvGreenCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_rcv_grn_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;

	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegRecvGreenBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_rdndt_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegRedundantCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_rdndt_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegRedundantBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_in_seg_mal_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegMalformedCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_mal_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegMalformedBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_unk_snd_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegUnkSenderCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_unk_snd_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegUnkSenderBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_in_seg_unk_cli_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegUnkClientCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_unk_cli_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegUnkClientBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_stray_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegStrayCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_stray_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegStrayBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_miscol_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegMiscolorCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_miscol_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegMiscolorBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_clsd_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegClosedCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_seg_clsd_byte(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputSegClosedBytes;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_in_ckp_rcv_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputCkptRecvCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


expr_result_t ltp_get_eng_in_pos_ack_xmit_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputPosAckXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_neg_ack_xmit_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputNegAckXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_canc_xmit_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputCancelXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_ack_rexmt_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputAckReXmitCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_canc_rcv_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputCancelRecvCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}

expr_result_t ltp_get_eng_in_compl_cnt(Lyst params)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	unsigned int engineId = 0;
	expr_result_t result;
	result.type = EXPR_TYPE_UINT32;
	result.length = 0;
	result.value = NULL;

	/* \todo: Check for NULL entry here. */
	NmltpSpan span;
	int success = 0;

	memcpy(&val, entry->value, sizeof(val));
	engineId = val;

	ltpnm_span_get(engineId, &span, &success);
	if(success != 0)
	{
		val = span.inputCompleteCount;
		result.value = adm_copy_integer((uint8_t*)&val, sizeof(val), &(result.length));
	}
	return result;
}


/* Controls */
uint32_t ltp_engine_reset(Lyst params)
{
	return 0;
}

#endif /* _HAVE_LTP_ADM_*/
