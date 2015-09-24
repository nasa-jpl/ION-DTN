#include "ion.h"
#include "lyst.h"
#include "platform.h"

#include "shared/adm/adm_bp.h"
#include "shared/utils/utils.h"


void adm_bp_init()
{
	/* Node-specific Information. */
	uint8_t mid_str[ADM_MID_ALLOC];


	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 0, mid_str);
	adm_add_datadef("BP_NODE_ALL",                 mid_str, 0,  bp_print_node_all,       bp_size_node_all);

	/* Node State Information */
	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 1, mid_str);
	adm_add_datadef("BP NODE ID",                  mid_str, 0,  adm_print_string, adm_size_string/*bp_size_node_id*/);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 2, mid_str);
	adm_add_datadef("BP_NODE_VER",                 mid_str, 0,  adm_print_string, adm_size_string/*bp_size_node_version*/);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 3, mid_str);
	adm_add_datadef("BP_NODE_AVAIL_STOR",          mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 4, mid_str);
	adm_add_datadef("BP_NODE_LAST_RESET_TIME",     mid_str, 0,  NULL, bp_size_node_restart_time);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 5, mid_str);
	adm_add_datadef("BP_NODE_NUM_REG",             mid_str, 0,  NULL, bp_size_node_num_reg);


	/* Bundle State Information */

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 6, mid_str);
	adm_add_datadef("BP_BNDL_CUR_FWD_PEND_CNT",        mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 7, mid_str);
	adm_add_datadef("BP_BNDL_CUR_DISPATCH_PEND_CNT",   mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 8, mid_str);
	adm_add_datadef("BP_BNDL_CUR_IN_CUSTODY_CNT",      mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 9, mid_str);
	adm_add_datadef("BP_BNDL_CUR_REASSMBL_PEND_CNT", mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 10, mid_str);
	adm_add_datadef("BP_BNDL_CUR_BULK_RES_CNT",       mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 11, mid_str);
	adm_add_datadef("BP_BNDL_CUR_NORM_RES_CNT",       mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 12, mid_str);
	adm_add_datadef("BP_BNDL_CUR_EXP_RES_CNT",        mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 13, mid_str);
	adm_add_datadef("BP_BNDL_CUR_BULK_RES_BYTES",     mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 14, mid_str);
	adm_add_datadef("BP_BNDL_CUR_NORM_BYTES",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 15, mid_str);
	adm_add_datadef("BP_BNDL_CUR_EXP_BYTES",          mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 16, mid_str);
	adm_add_datadef("BP_BNDL_BULK_SRC_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 17, mid_str);
	adm_add_datadef("BP_BNDL_NORM_SRC_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 18, mid_str);
	adm_add_datadef("BP_BNDL_EXP_SRC_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 19, mid_str);
	adm_add_datadef("BP_BNDL_BULK_SRC_BYTES",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 20, mid_str);
	adm_add_datadef("BP_BNDL_NORM_SRC_BYTES",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 21, mid_str);
	adm_add_datadef("BP_BNDL_EXP_SRC_BYTES",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 22, mid_str);
	adm_add_datadef("BP_BNDL_FRAGMENTED_CNT",        mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 23, mid_str);
	adm_add_datadef("BP_BNDL_FRAG_PRODUCED",         mid_str, 0,  NULL, NULL);


	/* Error and Reporting Information */
	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 24, mid_str);
	adm_add_datadef("BP_RPT_NOINFO_DEL_CNT",          mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 25, mid_str);
	adm_add_datadef("BP_RPT_EXPIRED_DEL_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 26, mid_str);
	adm_add_datadef("BP_RPT_UNI_FWD_DEL_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 27, mid_str);
	adm_add_datadef("BP_RPT_CANCEL_DEL_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 28, mid_str);
	adm_add_datadef("BP_RPT_NO_STRG_DEL_CNT",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 29, mid_str);
	adm_add_datadef("BP_RPT_BAD_EID_DEL_CNT",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 30, mid_str);
	adm_add_datadef("BP_RPT_NO_ROUTE_DEL_CNT",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 31, mid_str);
	adm_add_datadef("BP_RPT_NO_CONTACT_DEL_CNT",        mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 32, mid_str);
	adm_add_datadef("BP_RPT_BAD_BLOCK_DEL_CNT",         mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 33, mid_str);
	adm_add_datadef("BP_RPT_BUNDLES_DEL_CNT",           mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 34, mid_str);
	adm_add_datadef("BP_RPT_FAIL_CUST_XFER_CNT",        mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 35, mid_str);
	adm_add_datadef("BP_RPT_FAIL_CUST_XFER_BYTES",      mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 36, mid_str);
	adm_add_datadef("BP_RPT_FAIL_FWD_CNT",              mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 37, mid_str);
	adm_add_datadef("BP_RPT_FAIL_FWD_BYTES",            mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 38, mid_str);
	adm_add_datadef("BP_RPT_ABANDONED_CNT",            mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 39, mid_str);
	adm_add_datadef("BP_RPT_ABANDONED_BYTES",          mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 40, mid_str);
	adm_add_datadef("BP_RPT_DISCARD_CNT",              mid_str, 0,  NULL, NULL);

	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 41, mid_str);
	adm_add_datadef("BP_RPT_DISCARD_BYTES",            mid_str, 0,  NULL, NULL);


	adm_build_mid_str(0, BP_ADM_DATA_NN, BP_ADM_DATA_NN_LEN, 42, mid_str);
	adm_add_datadef("BP_NODE_ENDPOINT_NAMES",      mid_str, 0,  adm_print_string_list,   adm_size_string_list);


	/* Endpoint-Specific Information */

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 0, mid_str);
	adm_add_datadef("BP_ENDPT_ALL",                  mid_str, 1, bp_print_endpoint_all,   bp_size_endpoint_all);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 1, mid_str);
	adm_add_datadef("BP_ENDPT_NAME",                 mid_str, 1, adm_print_string,        bp_size_endpoint_name);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 2, mid_str);
	adm_add_datadef("BP_ENDPT_ACTIVE",                 mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 3, mid_str);
	adm_add_datadef("BP_ENDPT_SINGLETON",                 mid_str, 1, NULL, NULL);

	adm_build_mid_str(0x40, BP_ADM_DATA_END_NN, BP_ADM_DATA_END_NN_LEN, 4, mid_str);
	adm_add_datadef("BP_ENDPT_ABANDON_ON_DEL_FAIL",       mid_str, 1, NULL, NULL);


	/* Controls */
	adm_build_mid_str(0x01, BP_ADM_DATA_CTRL_NN, BP_ADM_DATA_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl("BP_NODE_RESET_COUNTS",     mid_str, 0);
}

char *bp_print_node_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	NmbpNode node_state;
	NmbpDisposition state;
	char *result;
	char temp[256];

	memcpy(&node_state, buffer, sizeof(node_state));
	memcpy(&state, buffer+sizeof(node_state), sizeof(state));

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 30 characters per string.
	*str_len = (20 * 40) + (30 * 60);

	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("bp_print_node_all","Can't allocate %d bytes.", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result,0,*str_len);

	memset(temp,0,256);
	snprintf(temp, 256, "\n\nNode State Information\n------------------------------");
	strncat(result,temp,256);

	snprintf(temp, 256,
			"\nNode ID           = %s \
\nNode Version      = %s \
\nAvailable Storage = " UVAST_FIELDSPEC " \
\nLast Restart Time = " UVAST_FIELDSPEC " \
\n# Registrations   = %d",
			node_state.nodeID,
			node_state.bpVersionNbr,
			node_state.avblStorage,
			(uvast)node_state.lastRestartTime,
			node_state.nbrOfRegistrations);
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256, "\n\nBundle Retention Counts\n------------------------------");
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nForward Pending   = " UVAST_FIELDSPEC " \
\nDispatch Pending  = " UVAST_FIELDSPEC " \
\nCustody Accepted  = " UVAST_FIELDSPEC " \
\nReassemby Pending = " UVAST_FIELDSPEC,
			state.currentForwardPending,
			state.currentDispatchPending,
			state.currentInCustody,
			state.currentReassemblyPending);
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,  "\n\nPriority Counts\n------------------------------");
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nBulk Sources Count       = " UVAST_FIELDSPEC " \
\nNormal Sourced Count     = " UVAST_FIELDSPEC " \
\nExpedited Sourced Count = " UVAST_FIELDSPEC,
			state.bundleSourceCount[0],
			state.bundleSourceCount[1],
			state.bundleSourceCount[2]);
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nBulk Sources Bytes       = " UVAST_FIELDSPEC " \
\nNormal Sourced Bytes     = " UVAST_FIELDSPEC " \
\nExpedited Sourced Bytes = " UVAST_FIELDSPEC,
			state.bundleSourceBytes[0],
			state.bundleSourceBytes[1],
			state.bundleSourceBytes[2]);
	strncat(result,temp,256);



	memset(temp,0,256);
	snprintf(temp, 256,
			"\nBulk Resident Count       = " UVAST_FIELDSPEC " \
\nNormal Resident Count     = " UVAST_FIELDSPEC " \
\nExpedited Resident Count = " UVAST_FIELDSPEC,
			state.currentResidentCount[0],
			state.currentResidentCount[1],
			state.currentResidentCount[2]);
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nBulk Resident Bytes       = " UVAST_FIELDSPEC " \
\nNormal Resident Bytes     = " UVAST_FIELDSPEC " \
\nExpedited Resident Bytes = " UVAST_FIELDSPEC,
			state.currentResidentBytes[0],
			state.currentResidentBytes[1],
			state.currentResidentBytes[2]);
	strncat(result,temp,256);


	memset(temp,0,256);
	snprintf(temp, 256, "\n\nFragmentation Counts\n------------------------------");
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nBundles Fragmented = " UVAST_FIELDSPEC " \
\nFragments Produced = " UVAST_FIELDSPEC,
             state.bundlesFragmented,
  			 state.fragmentsProduced);
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256, "\n\nBundle Deletion Counts By Reason\n------------------------------");
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nNo Info     = " UVAST_FIELDSPEC " \
\nExpired     = " UVAST_FIELDSPEC " \
\nUnicast Fwd = " UVAST_FIELDSPEC " \
\nCancelled   = " UVAST_FIELDSPEC " \
\nNo Storage  = " UVAST_FIELDSPEC " \
\nBad EID     = " UVAST_FIELDSPEC " \
\nNo Route    = " UVAST_FIELDSPEC " \
\nNo Contact  = " UVAST_FIELDSPEC " \
\nBad Block   = " UVAST_FIELDSPEC " \
\nTotal Bytes = " UVAST_FIELDSPEC,
			state.delNoneCount,
			state.delExpiredCount,
			state.delFwdUnidirCount,
			state.delCanceledCount,
			state.delDepletionCount,
			state.delEidMalformedCount,
			state.delNoRouteCount,
			state.delNoContactCount,
			state.delBlkMalformedCount,
			state.bytesDeletedToDate);
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp,256, "\n\nBundle Processing Errors\n------------------------------");
	strncat(result,temp,256);

	memset(temp,0,256);
	snprintf(temp, 256,
			"\nNo Custody Count = " UVAST_FIELDSPEC " \
\nNo Custody Bytes = " UVAST_FIELDSPEC " \
\nFwd Failed Count = " UVAST_FIELDSPEC " \
\nFwd Failed Bytes = " UVAST_FIELDSPEC " \
\nAbandoned Count  = " UVAST_FIELDSPEC " \
\nAbandoned Bytes  = " UVAST_FIELDSPEC " \
\nDiscarded Count  = " UVAST_FIELDSPEC " \
\nDiscarded Bytes  = " UVAST_FIELDSPEC,
			state.custodyRefusedCount,
			state.custodyRefusedBytes,
			state.bundleFwdFailedCount,
			state.bundleFwdFailedBytes,
			state.bundleAbandonCount,
			state.bundleAbandonBytes,
			state.bundleDiscardCount,
			state.bundleDiscardBytes);
	strncat(result,temp,256);

	return result;
}


char *bp_print_endpoint_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	NmbpEndpoint endpoint;

	char *result;
	char temp[256];
	uint32_t temp_size = 0;

	memcpy(&endpoint, buffer, data_len);

	// Assume for now a 8 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 100 characters per string.
	temp_size = (sizeof(uvast)*20) + 1;
	*str_len = (3 * temp_size) + (100 * 1);

	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("bp_endpoint_print_all","Can't allocate %d bytes.", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, 0, *str_len);

	memset(temp,0,256);
	sprintf(temp,
			"\nName      = %s \
\nActive    = %d \
\nSingleton = %d \
\nAbandon   = %d",
			endpoint.eid,
			endpoint.active,
			endpoint.singleton,
			endpoint.abandonOnDelivFailure);
	strcat(result,temp);

	return result;
}


/* SIZE */


uint32_t bp_size_node_all(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node_state;
	NmbpDisposition state;
	return sizeof(state) + sizeof(node_state);
}

uint32_t bp_size_endpoint_all(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpEndpoint endpoint;
	return sizeof(endpoint);
}


uint32_t bp_size_node_id(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.nodeID);
}

uint32_t bp_size_node_version(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.bpVersionNbr);
}

uint32_t bp_size_node_restart_time(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.lastRestartTime);
}

uint32_t bp_size_node_num_reg(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpNode node;
	return sizeof(node.nbrOfRegistrations);
}


uint32_t bp_size_endpoint_name(uint8_t* buffer, uint64_t buffer_len)
{
	NmbpEndpoint endpoint;
	return sizeof(endpoint.eid);
}



