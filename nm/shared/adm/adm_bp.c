#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "shared/adm/adm_bp.h"
#include "shared/utils/utils.h"



void initBpAdm()
{

	/* Node-specific Information. */
	adm_add("BP_NODE_ALL",                 BP_NODE_ALL,                 0,  bp_node_get_all,                 bp_print_node_all,       bp_size_node_all);
	adm_add("BP NODE ID",                  BP_NODE_ID,                  0,  bp_node_get_node_id,             adm_print_unsigned_long, bp_size_node_id);
	adm_add("BP_NODE_VER",                 BP_NODE_VER,                 0,  bp_node_get_version,             adm_print_unsigned_long, bp_size_node_version);
	adm_add("BP_NODE_CUR_RES_CNT_0",       BP_NODE_CUR_RES_CNT_0,       0,  bp_node_get_cur_res_cnt_0,       adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_RES_CNT_1",       BP_NODE_CUR_RES_CNT_1,       0,  bp_node_get_cur_res_cnt_1,       adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_RES_CNT_2",       BP_NODE_CUR_RES_CNT_2,       0,  bp_node_get_cur_res_cnt_2,       adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_RES_BYTES_0",     BP_NODE_CUR_RES_BYTES_0,     0,  bp_node_get_cur_res_byte_0,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_RES_BYTES_1",     BP_NODE_CUR_RES_BYTES_1,     0,  bp_node_get_cur_res_byte_1,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_RES_BYTES_2",     BP_NODE_CUR_RES_BYTES_2,     0,  bp_node_get_cur_res_byte_2,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_IN_LIMBO",        BP_NODE_CUR_IN_LIMBO,        0,  bp_node_get_cur_in_limbo,        adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_DISPATCH_PEND",   BP_NODE_CUR_DISPATCH_PEND,   0,  bp_node_get_cur_dispatch_pend,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_FWD_PEND",        BP_NODE_CUR_FWD_PEND,        0,  bp_node_get_cur_forward_pend,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_REASSEMBLE_PEND", BP_NODE_CUR_REASSEMBLE_PEND, 0,  bp_node_get_cur_reassemble_pend, adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_IN_CUSTODY",      BP_NODE_CUR_IN_CUSTODY,      0,  bp_node_get_cur_in_custody,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUR_NOT_IN_CUSTODY",  BP_NODE_CUR_NOT_IN_CUSTODY,  0,  bp_node_get_cur_not_in_custody,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_LAST_RESET_TIME",     BP_NODE_LAST_RESET_TIME,     0,  bp_node_get_last_reset_time,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_SRC_CNT_0",           BP_NODE_SRC_CNT_0,           0,  bp_node_get_src_cnt_0,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_SRC_CNT_1",           BP_NODE_SRC_CNT_1,           0,  bp_node_get_src_cnt_1,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_SRC_CNT_2",           BP_NODE_SRC_CNT_2,           0,  bp_node_get_src_cnt_2,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_SRC_BYTES_0",         BP_NODE_SRC_BYTES_0,         0,  bp_node_get_src_byte_0,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_SRC_BYTES_1",         BP_NODE_SRC_BYTES_1,         0,  bp_node_get_src_byte_1,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_SRC_BYTES_2",         BP_NODE_SRC_BYTES_2,         0,  bp_node_get_src_byte_2,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RX_CNT_0",            BP_NODE_RX_CNT_0,            0,  bp_node_get_rx_cnt_0,            adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RX_CNT_1",            BP_NODE_RX_CNT_1,            0,  bp_node_get_rx_cnt_1,            adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RX_CNT_2",            BP_NODE_RX_CNT_2,            0,  bp_node_get_rx_cnt_2,            adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RX_BYTES_0",          BP_NODE_RX_BYTES_0,          0,  bp_node_get_rx_byte_0,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RX_BYTES_1",          BP_NODE_RX_BYTES_1,          0,  bp_node_get_rx_byte_1,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RX_BYTES_2",          BP_NODE_RX_BYTES_2,          0,  bp_node_get_rx_byte_2,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_DIS_CNT_0",           BP_NODE_DIS_CNT_0,           0,  bp_node_get_dis_cnt_0,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_DIS_CNT_1",           BP_NODE_DIS_CNT_1,           0,  bp_node_get_dis_cnt_1,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_DIS_CNT_2",           BP_NODE_DIS_CNT_2,           0,  bp_node_get_dis_cnt_2,           adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_DIS_BYTES_0",         BP_NODE_DIS_BYTES_0,         0,  bp_node_get_dis_byte_0,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_DIS_BYTES_1",         BP_NODE_DIS_BYTES_1,         0,  bp_node_get_dis_byte_1,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_DIS_BYTES_2",         BP_NODE_DIS_BYTES_2,         0,  bp_node_get_dis_byte_2,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_XMIT_CNT_0",          BP_NODE_XMIT_CNT_0,          0,  bp_node_get_xmit_cnt_0,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_XMIT_CNT_1",          BP_NODE_XMIT_CNT_1,          0,  bp_node_get_xmit_cnt_1,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_XMIT_CNT_2",          BP_NODE_XMIT_CNT_2,          0,  bp_node_get_xmit_cnt_2,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_XMIT_BYTES_0",        BP_NODE_XMIT_BYTES_0,        0,  bp_node_get_xmit_byte_0,         adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_XMIT_BYTES_1",        BP_NODE_XMIT_BYTES_1,        0,  bp_node_get_xmit_byte_1,         adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_XMIT_BYTES_2",        BP_NODE_XMIT_BYTES_2,        0,  bp_node_get_xmit_byte_2,         adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_RX_CNT",          BP_NODE_RPT_RX_CNT,          0,  bp_node_get_rpt_rx_cnt,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_ACCEPT_CNT",      BP_NODE_RPT_ACCEPT_CNT,      0,  bp_node_get_rpt_accept_cnt,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_FWD_CNT",         BP_NODE_RPT_FWD_CNT,         0,  bp_node_get_rpt_fwd_cnt,         adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_DELIVER_CNT",     BP_NODE_RPT_DELIVER_CNT,     0,  bp_node_get_rpt_deliver_cnt,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_DELETE_CNT",      BP_NODE_RPT_DELETE_CNT,      0,  bp_node_get_rpt_delete_cnt,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_NONE_CNT",        BP_NODE_RPT_NONE_CNT,        0,  bp_node_get_rpt_none_cnt,        adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_EXPIRED_CNT",     BP_NODE_RPT_EXPIRED_CNT,     0,  bp_node_get_rpt_expired_cnt,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_FWD_UNIDIR_CNT",  BP_NODE_RPT_FWD_UNIDIR_CNT,  0,  bp_node_get_rpt_fwd_unidir_cnt,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_CANCELLED_CNT",   BP_NODE_RPT_CANCELLED_CNT,   0,  bp_node_get_rpt_cancelled_cnt,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_DEPLETION_CNT",   BP_NODE_RPT_DEPLETION_CNT,   0,  bp_node_get_rpt_depletion_cnt,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_EID_BAD_CNT",     BP_NODE_RPT_EID_BAD_CNT,     0,  bp_node_get_rpt_eid_bad_cnt,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_NO_ROUTE_CNT",    BP_NODE_RPT_NO_ROUTE_CNT,    0,  bp_node_get_rpt_no_route_cnt,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_NO_CONTACT_CNT",  BP_NODE_RPT_NO_CONTACT_CNT,  0,  bp_node_get_rpt_no_contact_cnt,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_RPT_BLK_MALFMED_CNT", BP_NODE_RPT_BLK_MALFMED_CNT, 0,  bp_node_get_rpt_blk_malformed,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_ACCEPT_CNT",     BP_NODE_CUST_ACC_CNT,        0,  bp_node_get_cust_accept_cnt,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_ACCEPT_BYTES",   BP_NODE_CUST_ACC_BYTES,      0,  bp_node_get_cust_accept_byte,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_REL_CNT",        BP_NODE_CUST_REL_CNT,        0,  bp_node_get_cust_rel_cnt,        adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_REL_BYTES",      BP_NODE_CUST_REL_BYTES,      0,  bp_node_get_cust_rel_byte,       adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_EXP_CNT",        BP_NODE_CUST_EXP_CNT,        0,  bp_node_get_cust_exp_cnt,        adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_EXP_BYTES",      BP_NODE_CUST_EXP_BYTES,      0,  bp_node_get_cust_exp_byte,       adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_RED_CNT",        BP_NODE_CUST_RED_CNT,        0,  bp_node_get_cust_redundant_cnt,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_RED_BYTES",      BP_NODE_CUST_RED_BYTES,      0,  bp_node_get_cust_redundant_byte, adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_DEP_CNT",        BP_NODE_CUST_DEP_CNT,        0,  bp_node_get_cust_depletion_cnt,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_DEP_BYTES",      BP_NODE_CUST_DEP_BYTES,      0,  bp_node_get_cust_depletion_byte, adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_EID_BAD_CNT",    BP_NODE_CUST_EID_BAD_CNT,    0,  bp_node_get_cust_eid_bad_cnt,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_EID_BAD_BYTES",  BP_NODE_CUST_EID_BAD_BYTES,  0,  bp_node_get_cust_eid_bad_byte,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_NO_ROUTE_CNT",   BP_NODE_CUST_NO_ROUTE_CNT,   0,  bp_node_get_cust_no_route_cnt,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_NO_ROUTE_BYTES", BP_NODE_CUST_NO_ROUTE_BYTES, 0,  bp_node_get_cust_no_route_byte,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_NO_CTACT_CNT",   BP_NODE_CUST_NO_CTACT_CNT,   0,  bp_node_get_cust_no_contact_cnt, adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_NO_CTACT_BYTES", BP_NODE_CUST_NO_CTACT_BYTES, 0,  bp_node_get_cust_no_contact_byte,adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_BLK_MAL_CNT",    BP_NODE_CUST_BLK_MAL_CNT,    0,  bp_node_get_cust_blk_mal_cnt,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_CUST_BLK_MAL_BYTES",  BP_NODE_CUST_BLK_MAL_BYTES,  0,  bp_node_get_cust_blk_mal_byte,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_QUEUED_FWD_CNT",      BP_NODE_QUEUED_FWD_CNT,      0,  bp_node_get_queued_fwd_cnt,      adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_QUEUED_FWD_BYTES",    BP_NODE_QUEUED_FWD_BYTES,    0,  bp_node_get_queued_fwd_byte,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_FWD_OK_CNT",          BP_NODE_FWD_OK_CNT,          0,  bp_node_get_fwd_ok_cnt,          adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_FWD_OK_BYTES",        BP_NODE_FWD_OK_BYTES,        0,  bp_node_get_fwd_ok_byte,         adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_FWD_FAIL_CNT",        BP_NODE_FWD_FAIL_CNT,        0,  bp_node_get_fwd_fail_cnt,        adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_FWD_FAIL_BYTES",      BP_NODE_FWD_FAIL_BYTES,      0,  bp_node_get_fwd_fail_byte,       adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_FWD_REQUEUE_CNT",     BP_NODE_FWD_REQUEUE_CNT,     0,  bp_node_get_fwd_requeue_cnt,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_FWD_REQUEUE_BYTES",   BP_NODE_FWD_REQUEUE_BYTES,   0,  bp_node_get_fwd_requeue_byte,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_EXPIRED_CNT",         BP_NODE_EXPIRED_CNT,         0,  bp_node_get_expired_cnt,         adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_EXPIRED_BYTES",       BP_NODE_EXPIRED_BYTES,       0,  bp_node_get_expired_byte,        adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_NODE_ENDPOINT_NAMES",      BP_NODE_ENDPOINT_NAMES,      0,  bp_endpoint_get_names,           adm_print_string_list,   adm_size_string_list);


	/* Endpoint-Specific Information */

	adm_add("BP_ENDPOINT_ALL",                   BP_ENDPOINT_ALL,                  1, bp_endpoint_get_all,                 bp_print_endpoint_all,   bp_size_endpoint_all);
	adm_add("BP_ENDPOINT_NAME",                 BP_ENDPOINT_NAME,                 1, bp_endpoint_get_name,                adm_print_string,        adm_size_string);
	adm_add("BP_ENDPOINT_CUR_QUEUED_BUNDLES",   BP_ENDPOINT_CUR_QUEUED_BUNDLES,   1, bp_endpoint_get_cur_queued_bndls,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_ENDPOINT_CUR_QUEUED_BYTES",     BP_ENDPOINT_CUR_QUEUED_BYTES,     1, bp_endpoint_get_cur_queued_byte,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_ENDPOINT_LAST_RESET_TIME",      BP_ENDPOINT_LAST_RESET_TIME,      1, bp_endpoint_get_last_reset_time,     adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_ENDPOINT_BUNDLE_QUEUE_COUNT",   BP_ENDPOINT_BUNDLE_QUEUE_COUNT,   1, bp_endpoint_get_bundle_queue_cnt,    adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_ENDPOINT_BUNDLE_QUEUE_BYTES",   BP_ENDPOINT_BUNDLE_QUEUE_BYTES,   1, bp_endpoint_get_bundle_queue_byte,   adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_ENDPOINT_BUNDLE_DEQUEUE_COUNT", BP_ENDPOINT_BUNDLE_DEQUEUE_COUNT, 1, bp_endpoint_get_bundle_dequeue_cnt,  adm_print_unsigned_long, adm_size_unsigned_long);
	adm_add("BP_ENDPOINT_BUNDLE_DEQUEUE_BYTES", BP_ENDPOINT_BUNDLE_DEQUEUE_BYTES, 1, bp_endpoint_get_bundle_dequeue_byte, adm_print_unsigned_long, adm_size_unsigned_long);

	/* Controls */
	adm_add_ctrl("BP_NODE_RESET_ALL",      BP_NODE_RESET_ALL,     0, bp_ctrl_reset);
	adm_add_ctrl("BP_ENDPOINT_RESET_ALL",  BP_ENDPOINT_RESET_ALL, 1, bp_ctrl_endpoint_reset);


}


char *bp_print_node_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	NmbpDisposition state;
	char *result;
	char temp[128];
	uint32_t temp_size = 0;

	memcpy(&state, buffer, data_len);

	// Assume for now a 4 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = (80*sizeof(unsigned long)) + 1;
	*str_len = (temp_size * 5) + (25 * 100);
	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("bpPrintDisposition","Can't allocate %d bytes.", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(temp,
			"\ncurrentResidentCount[] = {%ld, %ld, %ld}\n",
			state.currentResidentCount[0],
			state.currentResidentCount[1],
			state.currentResidentCount[2]);
	strcat(result,temp);

	sprintf(temp,
			"currentInLimbo = %ld\n",
			state.currentInLimbo);
	strcat(result,temp);

	sprintf(temp,
			"currentDispatchPending = %ld\n",
			state.currentDispatchPending);
	strcat(result,temp);

	sprintf(temp,
			"currentForwardPending = %ld\n",
			state.currentForwardPending);
	strcat(result,temp);

	sprintf(temp,
			"currentReassemblyPending = %ld\n",
			state.currentReassemblyPending);
	strcat(result,temp);

	sprintf(temp,
			"currentInCustody = %ld\n",
			state.currentInCustody);
	strcat(result,temp);

	sprintf(temp,
			"currentNotInCustody = %ld\n",
			state.currentNotInCustody);
	strcat(result,temp);

	sprintf(temp,
			"lastResetTime = %ld\n",
			state.lastResetTime);
	strcat(result,temp);

	sprintf(temp,
			"bundleSourceCount[] = {%ld, %ld, %ld}\n",
			state.bundleSourceCount[0],
			state.bundleSourceCount[1],
			state.bundleSourceCount[2]);
	strcat(result,temp);

	sprintf(temp,
			"bundleRecvCount[] = {%ld, %ld, %ld}\n",
			state.bundleRecvCount[0],
			state.bundleRecvCount[1],
			state.bundleRecvCount[2]);
	strcat(result,temp);

	sprintf(temp,
			"bundleRecvBytes[] = {%ld, %ld, %ld}\n",
			state.bundleRecvBytes[0],
			state.bundleRecvBytes[1],
			state.bundleRecvBytes[2]);
	strcat(result,temp);

	sprintf(temp,
			"bundleDiscardCount[] = {%ld, %ld, %ld}\n",
			state.bundleDiscardCount[0],
			state.bundleDiscardCount[1],
			state.bundleDiscardCount[2]);
	strcat(result,temp);

	sprintf(temp,
			"bundleDiscardBytes[] = {%ld, %ld, %ld}\n",
			state.bundleDiscardBytes[0],
			state.bundleDiscardBytes[1],
			state.bundleDiscardBytes[2]);
	strcat(result,temp);

	sprintf(temp,
			"bundleXmitCount[] = {%ld, %ld, %ld}\n",
			state.bundleXmitCount[0],
			state.bundleXmitCount[1],
			state.bundleXmitCount[2]);
	strcat(result,temp);

	sprintf(temp,
			"bundleXmitBytes[] = {%ld, %ld, %ld}\n",
			state.bundleXmitBytes[0],
			state.bundleXmitBytes[1],
			state.bundleXmitBytes[2]);
	strcat(result,temp);

	sprintf(temp,
			"rptReceiveCount = %ld\n",
			state.rptReceiveCount);
	strcat(result,temp);

	sprintf(temp,
			"rptAcceptCount = %ld\n",
			state.rptAcceptCount);
	strcat(result,temp);

	sprintf(temp,
			"rptForwardCount = %ld\n",
			state.rptForwardCount);
	strcat(result,temp);

	sprintf(temp,
			"rptDeliverCount = %ld\n",
			state.rptDeliverCount);
	strcat(result,temp);

	sprintf(temp,
			"rptDeleteCount = %ld\n",
			state.rptDeleteCount);
	strcat(result,temp);

	sprintf(temp,
			"rptNoneCount = %ld\n",
			state.rptNoneCount);
	strcat(result,temp);

	sprintf(temp,
			"rptExpiredCount = %ld\n",
			state.rptExpiredCount);
	strcat(result,temp);

	sprintf(temp,
			"rptFwdUnidirCount = %ld\n",
			state.rptFwdUnidirCount);
	strcat(result,temp);

	sprintf(temp,
			"rptCanceledCount = %ld\n",
			state.rptCanceledCount);
	strcat(result,temp);

	sprintf(temp,
			"rptDepletionCount = %ld\n",
			state.rptDepletionCount);
	strcat(result,temp);

	sprintf(temp,
			"rptEidMalformedCount = %ld\n",
			state.rptEidMalformedCount);
	strcat(result,temp);

	sprintf(temp,
			"rptNoRouteCount = %ld\n",
			state.rptNoRouteCount);
	strcat(result,temp);

	sprintf(temp,
			"rptNoContactCount = %ld\n",
			state.rptNoContactCount);
	strcat(result,temp);

	sprintf(temp,
			"rptBlkMalformedCount = %ld\n",
			state.rptBlkMalformedCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyAcceptCount = %ld\n",
			state.custodyAcceptCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyAcceptBytes = %ld\n",
			state.custodyAcceptBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyReleasedCount = %ld\n",
			state.custodyReleasedCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyReleasedBytes = %ld\n",
			state.custodyReleasedBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyExpiredCount = %ld\n",
			state.custodyExpiredCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyExpiredBytes = %ld\n",
			state.custodyExpiredBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyRedundantCount = %ld\n",
			state.custodyRedundantCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyRedundantBytes = %ld\n",
			state.custodyRedundantBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyDepletionCount = %ld\n",
			state.custodyDepletionCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyDepletionBytes = %ld\n",
			state.custodyDepletionBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyEidMalformedCount = %ld\n",
			state.custodyEidMalformedCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyEidMalformedBytes = %ld\n",
			state.custodyEidMalformedBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyNoRouteCount = %ld\n",
			state.custodyNoRouteCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyNoRouteBytes = %ld\n",
			state.custodyNoRouteBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyNoContactCount = %ld\n",
			state.custodyNoContactCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyNoContactBytes = %ld\n",
			state.custodyNoContactBytes);
	strcat(result,temp);

	sprintf(temp,
			"custodyBlkMalformedCount = %ld\n",
			state.custodyBlkMalformedCount);
	strcat(result,temp);

	sprintf(temp,
			"custodyBlkMalformedBytes = %ld\n",
			state.custodyBlkMalformedBytes);
	strcat(result,temp);

	sprintf(temp,
			"bundleQueuedForFwdCount = %ld\n",
			state.bundleQueuedForFwdCount);
	strcat(result,temp);

	sprintf(temp,
			"bundleQueuedForFwdBytes = %ld\n",
			state.bundleQueuedForFwdBytes);
	strcat(result,temp);

	sprintf(temp,
			"bundleFwdOkayCount = %ld\n",
			state.bundleFwdOkayCount);
	strcat(result,temp);

	sprintf(temp,
			"bundleFwdOkayBytes = %ld\n",
			state.bundleFwdOkayBytes);
	strcat(result,temp);

	sprintf(temp,
			"bundleFwdFailedCount = %ld\n",
			state.bundleFwdFailedCount);
	strcat(result,temp);

	sprintf(temp,
			"bundleFwdFailedBytes = %ld\n",
			state.bundleFwdFailedBytes);
	strcat(result,temp);

	sprintf(temp,
			"bundleRequeuedForFwdCount = %ld\n",
			state.bundleRequeuedForFwdCount);
	strcat(result,temp);

	sprintf(temp,
			"bundleRequeuedForFwdBytes = %ld\n",
			state.bundleRequeuedForFwdBytes);
	strcat(result,temp);

	sprintf(temp,
			"bundleExpiredCount = %ld\n",
			state.bundleExpiredCount);
	strcat(result,temp);

	sprintf(temp,
			"bundleExpiredBytes = %ld\n",
			state.bundleExpiredBytes);
	strcat(result,temp);

	return result;
}


char *bp_print_endpoint_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	NmbpEndpoint endpoint;

	char *result;
	char temp[128];
	uint32_t temp_size = 0;

	memcpy(&endpoint, buffer, data_len);

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 30 characters per string.
	temp_size = (8*20) + 1;
	*str_len = (temp_size) + (30 * 8);

	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("bp_endpoint_print_all","Can't allocate %d bytes.", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(temp, "\nEID: %s\n", endpoint.eid);
	strcat(result, temp);

	sprintf(temp, "CurQueuedBundleCount: %ld\n", endpoint.currentQueuedBundlesCount);
	strcat(result, temp);

	sprintf(temp, "CurQueuedBundleBytes: %ld\n", endpoint.currentQueuedBundlesBytes);
	strcat(result, temp);

	sprintf(temp, "LastResetTime: %ld\n", endpoint.lastResetTime);
	strcat(result, temp);

	sprintf(temp, "BundleEnqueuedCnt: %ld\n", endpoint.bundleEnqueuedCount);
	strcat(result, temp);

	sprintf(temp, "BundleEnqueuedBytes: %ld\n", endpoint.bundleEnqueuedBytes);
	strcat(result, temp);

	sprintf(temp, "BundleDequeuedCnt: %ld\n", endpoint.bundleDequeuedCount);
	strcat(result, temp);

	sprintf(temp, "BundleDequeuedBytes: %ld\n", endpoint.bundleDequeuedBytes);
	strcat(result, temp);


	return result;
}


uint8_t *bp_node_get_all(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	uint8_t *result;
	bpnm_disposition_get(&state);

	result = (uint8_t*) MTAKE(sizeof(state));
	memcpy(result, &state, sizeof(state));
	*length = sizeof(state);
	return result;
}

uint8_t *bp_node_get_node_id(Lyst params, uint64_t *length)
{
	unsigned long l = getOwnNodeNbr();
	return adm_copy_integer((uint8_t*)&l, sizeof(l), length);
}

uint8_t *bp_node_get_version(Lyst params, uint64_t *length)
{
	uint8_t version = 6;
	return adm_copy_integer((uint8_t*)&version, sizeof(version), length);
}


uint8_t *bp_node_get_cur_res_cnt_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentResidentCount[0]), sizeof(state.currentResidentCount[0]), length);
}

uint8_t *bp_node_get_cur_res_cnt_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentResidentCount[1]), sizeof(state.currentResidentCount[1]), length);
}


uint8_t *bp_node_get_cur_res_cnt_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentResidentCount[2]), sizeof(state.currentResidentCount[2]), length);
}

uint8_t *bp_node_get_cur_res_byte_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentResidentBytes[0]), sizeof(state.currentResidentBytes[0]), length);
}

uint8_t *bp_node_get_cur_res_byte_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentResidentBytes[1]), sizeof(state.currentResidentBytes[1]), length);
}

uint8_t *bp_node_get_cur_res_byte_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentResidentBytes[2]), sizeof(state.currentResidentBytes[2]), length);
}


uint8_t *bp_node_get_cur_in_limbo(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentInLimbo), sizeof(state.currentInLimbo), length);
}

uint8_t *bp_node_get_cur_dispatch_pend(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentDispatchPending), sizeof(state.currentDispatchPending), length);
}

uint8_t *bp_node_get_cur_forward_pend(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);

	return adm_copy_integer((uint8_t*)&(state.currentForwardPending), sizeof(state.currentForwardPending), length);
}

uint8_t *bp_node_get_cur_reassemble_pend(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentReassemblyPending), sizeof(state.currentReassemblyPending), length);
}

uint8_t *bp_node_get_cur_in_custody(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentInCustody), sizeof(state.currentInCustody), length);
}

uint8_t *bp_node_get_cur_not_in_custody(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.currentNotInCustody), sizeof(state.currentNotInCustody), length);
}


uint8_t *bp_node_get_last_reset_time(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.lastResetTime), sizeof(state.lastResetTime), length);
}


uint8_t *bp_node_get_src_cnt_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleSourceCount[0]), sizeof(state.bundleSourceCount[0]), length);
}

uint8_t *bp_node_get_src_cnt_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleSourceCount[1]), sizeof(state.bundleSourceCount[1]), length);
}

uint8_t *bp_node_get_src_cnt_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleSourceCount[2]), sizeof(state.bundleSourceCount[2]), length);
}

uint8_t *bp_node_get_src_byte_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleSourceBytes[0]), sizeof(state.bundleSourceBytes[0]), length);
}

uint8_t *bp_node_get_src_byte_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleSourceBytes[1]), sizeof(state.bundleSourceBytes[1]), length);
}

uint8_t *bp_node_get_src_byte_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleSourceBytes[2]), sizeof(state.bundleSourceBytes[2]), length);
}


uint8_t *bp_node_get_rx_cnt_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRecvCount[0]), sizeof(state.bundleRecvCount[0]), length);
}

uint8_t *bp_node_get_rx_cnt_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRecvCount[1]), sizeof(state.bundleRecvCount[1]), length);
}

uint8_t *bp_node_get_rx_cnt_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRecvCount[2]), sizeof(state.bundleRecvCount[2]), length);
}

uint8_t *bp_node_get_rx_byte_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRecvBytes[0]), sizeof(state.bundleRecvBytes[0]), length);
}

uint8_t *bp_node_get_rx_byte_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRecvBytes[1]), sizeof(state.bundleRecvBytes[1]), length);
}

uint8_t *bp_node_get_rx_byte_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRecvBytes[2]), sizeof(state.bundleRecvBytes[2]), length);
}


uint8_t *bp_node_get_dis_cnt_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleDiscardCount[0]), sizeof(state.bundleDiscardCount[0]), length);
}

uint8_t *bp_node_get_dis_cnt_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleDiscardCount[1]), sizeof(state.bundleDiscardCount[1]), length);
}

uint8_t *bp_node_get_dis_cnt_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleDiscardCount[2]), sizeof(state.bundleDiscardCount[2]), length);
}

uint8_t *bp_node_get_dis_byte_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleDiscardBytes[0]), sizeof(state.bundleDiscardBytes[0]), length);
}

uint8_t *bp_node_get_dis_byte_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleDiscardBytes[1]), sizeof(state.bundleDiscardBytes[1]), length);
}

uint8_t *bp_node_get_dis_byte_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleDiscardBytes[2]), sizeof(state.bundleDiscardBytes[2]), length);
}


uint8_t *bp_node_get_xmit_cnt_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleXmitCount[0]), sizeof(state.bundleXmitCount[0]), length);
}

uint8_t *bp_node_get_xmit_cnt_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleXmitCount[1]), sizeof(state.bundleXmitCount[1]), length);
}

uint8_t *bp_node_get_xmit_cnt_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleXmitCount[2]), sizeof(state.bundleXmitCount[2]), length);
}

uint8_t *bp_node_get_xmit_byte_0(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleXmitBytes[0]), sizeof(state.bundleXmitBytes[0]), length);
}

uint8_t *bp_node_get_xmit_byte_1(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleXmitBytes[1]), sizeof(state.bundleXmitBytes[1]), length);
}

uint8_t *bp_node_get_xmit_byte_2(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleXmitBytes[2]), sizeof(state.bundleXmitBytes[2]), length);
}


uint8_t *bp_node_get_rpt_rx_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptReceiveCount), sizeof(state.rptReceiveCount), length);
}

uint8_t *bp_node_get_rpt_accept_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptAcceptCount), sizeof(state.rptAcceptCount), length);
}

uint8_t *bp_node_get_rpt_fwd_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptForwardCount), sizeof(state.rptForwardCount), length);
}

uint8_t *bp_node_get_rpt_deliver_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptDeliverCount), sizeof(state.rptDeliverCount), length);
}

uint8_t *bp_node_get_rpt_delete_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptDeleteCount), sizeof(state.rptDeleteCount), length);
}

uint8_t *bp_node_get_rpt_none_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptNoneCount), sizeof(state.rptNoneCount), length);
}

uint8_t *bp_node_get_rpt_expired_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptExpiredCount), sizeof(state.rptExpiredCount), length);
}

uint8_t *bp_node_get_rpt_fwd_unidir_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptFwdUnidirCount), sizeof(state.rptFwdUnidirCount), length);
}

uint8_t *bp_node_get_rpt_cancelled_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptCanceledCount), sizeof(state.rptCanceledCount), length);
}

uint8_t *bp_node_get_rpt_depletion_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptDepletionCount), sizeof(state.rptDepletionCount), length);
}

uint8_t *bp_node_get_rpt_eid_bad_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptEidMalformedCount), sizeof(state.rptEidMalformedCount), length);
}

uint8_t *bp_node_get_rpt_no_route_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptNoRouteCount), sizeof(state.rptNoRouteCount), length);
}

uint8_t *bp_node_get_rpt_no_contact_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptNoContactCount), sizeof(state.rptNoContactCount), length);
}

uint8_t *bp_node_get_rpt_blk_malformed(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.rptBlkMalformedCount), sizeof(state.rptBlkMalformedCount), length);
}

uint8_t *bp_node_get_cust_accept_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyAcceptCount), sizeof(state.custodyAcceptCount), length);
}

uint8_t *bp_node_get_cust_accept_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyAcceptBytes), sizeof(state.custodyAcceptBytes), length);
}

uint8_t *bp_node_get_cust_rel_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyReleasedCount), sizeof(state.custodyReleasedCount), length);
}

uint8_t *bp_node_get_cust_rel_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyReleasedBytes), sizeof(state.custodyReleasedBytes), length);
}

uint8_t *bp_node_get_cust_exp_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyExpiredCount), sizeof(state.custodyExpiredCount), length);
}

uint8_t *bp_node_get_cust_exp_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyExpiredBytes), sizeof(state.custodyExpiredBytes), length);
}

uint8_t *bp_node_get_cust_redundant_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyRedundantCount), sizeof(state.custodyRedundantCount), length);
}

uint8_t *bp_node_get_cust_redundant_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyRedundantBytes), sizeof(state.custodyRedundantBytes), length);
}

uint8_t *bp_node_get_cust_depletion_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyDepletionCount), sizeof(state.custodyDepletionCount), length);
}

uint8_t *bp_node_get_cust_depletion_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyDepletionBytes), sizeof(state.custodyDepletionBytes), length);
}

uint8_t *bp_node_get_cust_eid_bad_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyEidMalformedCount), sizeof(state.custodyEidMalformedCount), length);
}

uint8_t *bp_node_get_cust_eid_bad_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyEidMalformedBytes), sizeof(state.custodyEidMalformedBytes), length);
}

uint8_t *bp_node_get_cust_no_route_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyNoRouteCount), sizeof(state.custodyNoRouteCount), length);
}

uint8_t *bp_node_get_cust_no_route_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyNoRouteBytes), sizeof(state.custodyNoRouteBytes), length);
}

uint8_t *bp_node_get_cust_no_contact_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyNoContactCount), sizeof(state.custodyNoContactCount), length);
}

uint8_t *bp_node_get_cust_no_contact_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyNoContactBytes), sizeof(state.custodyNoContactBytes), length);
}

uint8_t *bp_node_get_cust_blk_mal_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyBlkMalformedCount), sizeof(state.custodyBlkMalformedCount), length);
}

uint8_t *bp_node_get_cust_blk_mal_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.custodyBlkMalformedBytes), sizeof(state.custodyBlkMalformedBytes), length);
}


uint8_t *bp_node_get_queued_fwd_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleQueuedForFwdCount), sizeof(state.bundleQueuedForFwdCount), length);
}

uint8_t *bp_node_get_queued_fwd_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleQueuedForFwdBytes), sizeof(state.bundleQueuedForFwdBytes), length);
}


uint8_t *bp_node_get_fwd_ok_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleFwdOkayCount), sizeof(state.bundleFwdOkayCount), length);
}

uint8_t *bp_node_get_fwd_ok_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleFwdOkayBytes), sizeof(state.bundleFwdOkayBytes), length);
}

uint8_t *bp_node_get_fwd_fail_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleFwdFailedCount), sizeof(state.bundleFwdFailedCount), length);
}

uint8_t *bp_node_get_fwd_fail_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleFwdFailedBytes), sizeof(state.bundleFwdFailedBytes), length);
}

uint8_t *bp_node_get_fwd_requeue_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRequeuedForFwdCount), sizeof(state.bundleRequeuedForFwdCount), length);
}

uint8_t *bp_node_get_fwd_requeue_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleRequeuedForFwdBytes), sizeof(state.bundleRequeuedForFwdBytes), length);
}


uint8_t *bp_node_get_expired_cnt(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleExpiredCount), sizeof(state.bundleExpiredCount), length);
}

uint8_t *bp_node_get_expired_byte(Lyst params, uint64_t *length)
{
	NmbpDisposition state;
	bpnm_disposition_get(&state);
	return adm_copy_integer((uint8_t*)&(state.bundleExpiredBytes), sizeof(state.bundleExpiredBytes), length);
}

uint8_t *bp_endpoint_get_names(Lyst params, uint64_t *length)
{
	char names[2048];
	char *ptrs[128];
	int num = 0;
	Sdnv nm_sdnv;
	uint8_t *result = NULL;
	uint8_t *cursor = NULL;


	bpnm_endpointNames_get((char *) names, ptrs, &num);

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

uint8_t *bp_endpoint_get_all(Lyst params, uint64_t *length)
{
	uint8_t *result = NULL;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];

	/* \todo: Check for NULL entry here. */
	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	result = (uint8_t*) MTAKE(sizeof(endpoint));
	memcpy(result, &endpoint, sizeof(endpoint));
	*length = sizeof(endpoint);
	return result;
}


uint8_t *bp_endpoint_get_name(Lyst params, uint64_t *length)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];
	uint8_t *result = NULL;

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	*length = (uint64_t) strlen(endpoint.eid) + 1;
	result = (uint8_t*) MTAKE(*length);
	memset(result,0,*length);
	memcpy(result, endpoint.eid, *length);

	return result;
}


uint8_t *bp_endpoint_get_cur_queued_bndls(Lyst params, uint64_t *length)
{
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	unsigned long val = 0;
	char name[256];

	NmbpEndpoint endpoint;
	int success = 0;

	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	val = endpoint.currentQueuedBundlesCount;

	return adm_copy_integer((uint8_t*)&val, sizeof(val), length);
}

uint8_t *bp_endpoint_get_cur_queued_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpEndpoint endpoint;
	int success = 0;

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(endpoint.currentQueuedBundlesBytes),
			                sizeof(endpoint.currentQueuedBundlesBytes),
			                length);
}

uint8_t *bp_endpoint_get_last_reset_time(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpEndpoint endpoint;
	int success = 0;

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(endpoint.lastResetTime),
			                sizeof(endpoint.lastResetTime),
			                length);
}

uint8_t *bp_endpoint_get_bundle_queue_cnt(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpEndpoint endpoint;
	int success = 0;

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(endpoint.bundleEnqueuedCount),
			                sizeof(endpoint.bundleEnqueuedCount),
			                length);
}

uint8_t *bp_endpoint_get_bundle_queue_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpEndpoint endpoint;
	int success = 0;

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(endpoint.bundleEnqueuedBytes),
			                sizeof(endpoint.bundleEnqueuedBytes),
			                length);
}

uint8_t *bp_endpoint_get_bundle_dequeue_cnt(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpEndpoint endpoint;
	int success = 0;

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(endpoint.bundleDequeuedCount),
			                sizeof(endpoint.bundleDequeuedCount),
			                length);
}

uint8_t *bp_endpoint_get_bundle_dequeue_byte(Lyst params, uint64_t *length)
{
	char *name = (char *) lyst_data(lyst_first(params));
	NmbpEndpoint endpoint;
	int success = 0;

	*length = 0;
	bpnm_endpoint_get(name, &endpoint, &success);
	if(success == 0)
	{
		return NULL;
	}

	return adm_copy_integer((uint8_t*)&(endpoint.bundleDequeuedBytes),
			                sizeof(endpoint.bundleDequeuedBytes),
			                length);

}

/* SIZE */


uint32_t bp_size_node_all(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpDisposition state;
	return sizeof(state);
}

uint32_t bp_size_endpoint_all(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpEndpoint endpoint;
	return sizeof(endpoint);
}


uint32_t bp_size_node_id(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(unsigned long);
}

uint32_t bp_size_node_version(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(uint8_t);
}


/* Controls */
uint32_t bp_ctrl_reset(Lyst params)
{
	bpnm_disposition_reset();
	return 1;
}

uint32_t bp_ctrl_endpoint_reset(Lyst params)
{
	int success = 0;
	datacol_entry_t *entry = (datacol_entry_t*)lyst_data(lyst_first(params));
	char name[256];
	memset(name,'\0',256);
	memcpy(name,entry->value, entry->length);

	bpnm_endpoint_reset(name, &success);

	return success;
}


