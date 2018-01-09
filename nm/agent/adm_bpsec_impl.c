/****************************************************************************
 **
 ** File Name: adm_bpsec_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2018-01-08  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include <math.h>

#include "../shared/adm/adm.h"

#include "../shared/primitives/value.h"
#include "adm_bpsec_impl.h"
#include "../shared/primitives/report.h"
#include "rda.h"
#include "../shared/primitives/ctrl.h"
#include "agent_db.h"
#include "../shared/primitives/table.h"

#include "../shared/adm/adm_bpsec.h"
#include "profiles.h"
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_bpsec_impl.h"

/*   START CUSTOM FUNCTIONS HERE */


static value_t adm_bpsec_get_src_val(tdc_t params, bpsec_instr_type_e type, query_type_e query)
{
	value_t result;
	char *eid_id = NULL;
	uvast num = 0;
	int8_t success = ERROR;

	result.type = AMP_TYPE_UNK;


	if((eid_id = adm_extract_string(params, 0, &success)) == NULL)
	{
		return result;
	}

	switch(query)
	{
	case SRC_BLK:     success = bpsec_instr_get_src_blk(eid_id, type, &num); break;
	case SRC_BYTES:   success = bpsec_instr_get_src_bytes(eid_id, type, &num); break;
	default: success = ERROR; break;
	}

	if(success != ERROR)
	{
		result.type = AMP_TYPE_UVAST;
		result.value.as_uvast = num;
	}

	SRELEASE(eid_id);

	return result;
}

static value_t adm_bpsec_get_tot_val(bpsec_instr_type_e type, query_type_e query)
{
	value_t result;
	uvast num = 0;
	int8_t success = 0;

	switch(query)
	{
	case TOTAL_BLK:   success = bpsec_instr_get_total_blk(type, &num); break;
	case TOTAL_BYTES: success = bpsec_instr_get_total_bytes(type, &num); break;
	default: success = ERROR; break;
	}

	if(success == ERROR)
	{
		result.type = AMP_TYPE_UNK;
	}
	else
	{

		result.type = AMP_TYPE_UVAST;
		result.value.as_uvast = num;
	}

	return result;
}

/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_bpsec_setup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void adm_bpsec_cleanup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


value_t adm_bpsec_meta_name(tdc_t params)
{
	return val_from_string("adm_bpsec");
}


value_t adm_bpsec_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:bpsec");
}


value_t adm_bpsec_meta_version(tdc_t params)
{
	return val_from_string("2016_05_16");
}


value_t adm_bpsec_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/*
 * This table lists all keys in the security policy database.
 */

table_t* adm_bpsec_tbl_keys()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "key_name", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists supported ciphersuites.
 */

table_t* adm_bpsec_tbl_ciphersuites()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "csname", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_ciphersuites BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_ciphersuites BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * BIB Rules.
 */

table_t* adm_bpsec_tbl_bib_rules()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "SrcEid", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "DestEid", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "TgtBlk", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "csName", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "keyName", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_bib_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr sdr = getIonsdr();
	Object listObj = 0;
	Object	elt = 0;
	Object addr = 0;
	OBJ_POINTER(BspBibRule, rule);
	int	 strLen = 0;
	char strBuffer[SDRSTRING_BUFSZ];
	Lyst cur_row = NULL;
	uint8_t *data = NULL;
	uint32_t len;
	int8_t success = 0;

	if((listObj = sec_get_bspBibRuleList()) == 0)
	{
		AMP_DEBUG_ERR("adm_bpsec_tbl_bib_rules","Cannot get list.", NULL);
		return NULL;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Can't start transaction.", NULL);
		table_destroy(table, 1);
		return NULL;
	}

	for (elt = sdr_list_first(sdr, listObj); elt; elt = sdr_list_next(sdr, elt))
	{

		if((cur_row = lyst_create()) != NULL)
		{
			addr = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBibRule, rule, addr);

			if(rule != NULL)
			{
				success = 1;

				strLen = sdr_string_read(sdr, strBuffer, rule->securitySrcEid);
				if (strLen < 0)
				{
					putErrmsg("string read failed.", NULL);
					table_destroy(table, 1);
					return NULL;
				}

				success = success && (dc_add(cur_row, (uint8_t*) strBuffer, strLen) != ERROR);

				strLen = sdr_string_read(sdr, strBuffer, rule->destEid);
				if (strLen < 0)
				{
					putErrmsg("string read failed.", NULL);
					table_destroy(table, 1);
					return NULL;
				}

				success = success && (dc_add(cur_row, (uint8_t*) strBuffer, strLen) != ERROR);

				if((data = utils_serialize_uint(rule->blockTypeNbr, &len)) == NULL)
				{
					success = 0;
				}
				else
				{
					oK(dc_add(cur_row, data, len));
					SRELEASE(data);
				}

				success = success && (dc_add(cur_row, (uint8_t*) rule->ciphersuiteName, strlen(rule->ciphersuiteName)) != ERROR);
				success = success && (dc_add(cur_row, (uint8_t*) rule->keyName, strlen(rule->keyName)) != ERROR);

				if(success == 0)
				{
					dc_destroy(&cur_row);
					table_destroy(table, 1);
					sdr_exit_xn(sdr);

					AMP_DEBUG_ERR("adm_bpsec_tbl_bib_rules", "Error extracting rule", NULL);
					return NULL;
				}
				else
				{
					table_add_row(table, cur_row);
				}
			}
			else
			{
				AMP_DEBUG_WARN("adm_bpsec_tbl_bib_rules", "NULL rule?", NULL);
			}
		}
		else
		{
			AMP_DEBUG_WARN("adm_bpsec_tbl_bib_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_bib_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * BCB Rules.
 */

table_t* adm_bpsec_tbl_bcb_rules()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
		(table_add_col(table, "SrcEid", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "DestEid", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "TgtBlk", AMP_TYPE_UINT) == ERROR) ||
		(table_add_col(table, "csName", AMP_TYPE_STR) == ERROR) ||
		(table_add_col(table, "keyName", AMP_TYPE_STR) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr sdr = getIonsdr();
	Object listObj = 0;
	Object	elt = 0;
	Object addr = 0;
	OBJ_POINTER(BspBcbRule, rule);
	int	 strLen = 0;
	char strBuffer[SDRSTRING_BUFSZ];
	Lyst cur_row = NULL;
	uint8_t *data = NULL;
	uint32_t len;
	int8_t success = 0;

	if((listObj = sec_get_bspBcbRuleList()) == 0)
	{
		AMP_DEBUG_ERR("adm_bpsec_tbl_bcb_rules","Cannot get list.", NULL);
		return NULL;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		putErrmsg("Can't start transaction.", NULL);
		table_destroy(table, 1);
		return NULL;
	}

	for (elt = sdr_list_first(sdr, listObj); elt; elt = sdr_list_next(sdr, elt))
	{

		if((cur_row = lyst_create()) != NULL)
		{
			addr = sdr_list_data(sdr, elt);
			GET_OBJ_POINTER(sdr, BspBcbRule, rule, addr);

			if(rule != NULL)
			{
				success = 1;

				strLen = sdr_string_read(sdr, strBuffer, rule->securitySrcEid);
				if (strLen < 0)
				{
					putErrmsg("string read failed.", NULL);
					table_destroy(table, 1);
					return NULL;
				}

				success = success && (dc_add(cur_row, (uint8_t*) strBuffer, strLen) != ERROR);

				strLen = sdr_string_read(sdr, strBuffer, rule->destEid);
				if (strLen < 0)
				{
					putErrmsg("string read failed.", NULL);
					table_destroy(table, 1);
					return NULL;
				}

				success = success && (dc_add(cur_row, (uint8_t*) strBuffer, strLen) != ERROR);

				if((data = utils_serialize_uint(rule->blockTypeNbr, &len)) == NULL)
				{
					success = 0;
				}
				else
				{
					oK(dc_add(cur_row, data, len));
					SRELEASE(data);
				}

				success = success && (dc_add(cur_row, (uint8_t*) rule->ciphersuiteName, strlen(rule->ciphersuiteName)) != ERROR);
				success = success && (dc_add(cur_row, (uint8_t*) rule->keyName, strlen(rule->keyName)) != ERROR);

				if(success == 0)
				{
					dc_destroy(&cur_row);
					table_destroy(table, 1);
					sdr_exit_xn(sdr);

					AMP_DEBUG_ERR("adm_bpsec_tbl_bcb_rules", "Error extracting rule", NULL);
					return NULL;
				}
				else
				{
					table_add_row(table, cur_row);
				}
			}
			else
			{
				AMP_DEBUG_WARN("adm_bpsec_tbl_bcb_rules", "NULL rule?", NULL);
			}
		}
		else
		{
			AMP_DEBUG_WARN("adm_bpsec_tbl_bcb_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * Total successfully Tx BCB blocks
 */
value_t adm_bpsec_get_num_good_tx_bcb_blk(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_TX_PASS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx BCB blocks
 */
value_t adm_bpsec_get_num_bad_tx_bcb_blk(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Rx BCB blocks
 */
value_t adm_bpsec_get_num_good_rx_bcb_blk(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_RX_PASS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Rx BCB blocks
 */
value_t adm_bpsec_get_num_bad_rx_bcb_blk(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_RX_FAIL, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total missing-on-RX BCB blocks
 */
value_t adm_bpsec_get_num_missing_rx_bcb_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_RX_MISS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total forward BCB blocks
 */
value_t adm_bpsec_get_num_fwd_bcb_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_FWD, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Tx bcb bytes
 */
value_t adm_bpsec_get_num_good_tx_bcb_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_TX_PASS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx bcb bytes
 */
value_t adm_bpsec_get_num_bad_tx_bcb_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Rx bcb bytes
 */
value_t adm_bpsec_get_num_good_rx_bcb_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_RX_PASS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Rx bcb bytes
 */
value_t adm_bpsec_get_num_bad_rx_bcb_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_RX_FAIL, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total missing-on-Rx bcb bytes
 */
value_t adm_bpsec_get_num_missing_rx_bcb_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_RX_MISS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total forwarded bcb bytes
 */
value_t adm_bpsec_get_num_fwd_bcb_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BCB_FWD, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Tx BIB blocks
 */
value_t adm_bpsec_get_num_good_tx_bib_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_TX_PASS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx BIB blocks
 */
value_t adm_bpsec_get_num_bad_tx_bib_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_TX_FAIL, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Rx BIB blocks
 */
value_t adm_bpsec_get_num_good_rx_bib_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_RX_PASS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Rx BIB blocks
 */
value_t adm_bpsec_get_num_bad_rx_bib_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_RX_FAIL, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total missing-on-Rx BIB blocks
 */
value_t adm_bpsec_get_num_miss_rx_bib_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_miss_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_RX_MISS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_miss_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total forwarded BIB blocks
 */
value_t adm_bpsec_get_num_fwd_bib_blks(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_FWD, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Tx BIB bytes
 */
value_t adm_bpsec_get_num_good_tx_bib_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_TX_PASS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx BIB bytes
 */
value_t adm_bpsec_get_num_bad_tx_bib_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_TX_FAIL, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Rx BIB bytes
 */
value_t adm_bpsec_get_num_good_rx_bib_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_RX_PASS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Rx BIB bytes
 */
value_t adm_bpsec_get_num_bad_rx_bib_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_RX_FAIL, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total missing-on-Rx BIB bytes
 */
value_t adm_bpsec_get_num_miss_rx_bib_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_miss_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_RX_MISS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_miss_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total forwarded BIB bytes
 */
value_t adm_bpsec_get_num_fwd_bib_bytes(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_tot_val( BIB_FWD, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Last BPSEC update
 */
value_t adm_bpsec_get_last_update(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_update BODY
	 * +-------------------------------------------------------------------------+
	 */
	result.type = AMP_TYPE_UNK;
	if(bpsec_instr_get_tot_update((time_t*)&(result.value.as_uint)) != ERROR)
	{
		result.type = AMP_TYPE_TS;
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_last_update BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of known keys
 */
value_t adm_bpsec_get_num_known_keys(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_known_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	uint32_t size = 0;

	result.type = AMP_TYPE_UINT;
	result.value.as_uint = bpsec_instr_get_num_keys((int*)&size);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_known_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Known key names
 */
value_t adm_bpsec_get_key_names(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_key_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *tmp = bpsec_instr_get_keynames();

	result.type = AMP_TYPE_UNK;
	/* TMP is allocated using MTAKE. We need to move it to
	 * something using STAKE.
	 */
	if(tmp != NULL)
	{
		uint32_t size = strlen(tmp) + 1;
		if((result.value.as_ptr = STAKE(size)) == NULL)
		{
			SRELEASE(tmp);
			return result;
		}
		memcpy(result.value.as_ptr, tmp, size);
		SRELEASE(tmp);

		result.type = AMP_TYPE_STRING;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_key_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Known ciphersuite names
 */
value_t adm_bpsec_get_ciphersuite_names(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_ciphersuite_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *tmp = bpsec_instr_get_csnames();

	result.type = AMP_TYPE_UNK;
	/* TMP is allocated using MTAKE. We need to move it to
	 * something using STAKE.
	 */
	if(tmp != NULL)
	{
		uint32_t size = strlen(tmp) + 1;
		if((result.value.as_ptr = STAKE(size)) == NULL)
		{
			SRELEASE(tmp);
			return result;
		}
		memcpy(result.value.as_ptr, tmp, size);
		SRELEASE(tmp);

		result.type = AMP_TYPE_STRING;
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_ciphersuite_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Known rule sources
 */
value_t adm_bpsec_get_rule_source(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_rule_source BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *tmp = bpsec_instr_get_srcnames();

	result.type = AMP_TYPE_UNK;
	/* TMP is allocated using MTAKE. We need to move it to
	 * something using STAKE.
	 */
	if(tmp != NULL)
	{
		uint32_t size = strlen(tmp) + 1;
		if((result.value.as_ptr = STAKE(size)) == NULL)
		{
			SRELEASE(tmp);
			return result;
		}
		memcpy(result.value.as_ptr, tmp, size);
		SRELEASE(tmp);

		result.type = AMP_TYPE_STRING;
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_rule_source BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Tx BCB blocks from SRC
 */
value_t adm_bpsec_get_num_good_tx_bcb_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_TX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed TX BCB blocks from SRC
 */
value_t adm_bpsec_get_num_bad_tx_bcb_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_TX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Rx BCB blocks from SRC
 */
value_t adm_bpsec_get_num_good_rx_bcb_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_RX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed RX BCB blocks from SRC
 */
value_t adm_bpsec_get_num_bad_rx_bcb_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_RX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Missing-onRX BCB blocks from SRC
 */
value_t adm_bpsec_get_num_missing_rx_bcb_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_RX_MISS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Forwarded BCB blocks from SRC
 */
value_t adm_bpsec_get_num_fwd_bcb_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_FWD, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfullt Tx bcb bytes from SRC
 */
value_t adm_bpsec_get_num_good_tx_bcb_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_TX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed Tx bcb bytes from SRC
 */
value_t adm_bpsec_get_num_bad_tx_bcb_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_TX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Rx bcb bytes from SRC
 */
value_t adm_bpsec_get_num_good_rx_bcb_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_RX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed Rx bcb bytes from SRC
 */
value_t adm_bpsec_get_num_bad_rx_bcb_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_RX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Missin-on-Rx bcb bytes from SRC
 */
value_t adm_bpsec_get_num_missing_rx_bcb_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_RX_MISS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Forwarded bcb bytes from SRC
 */
value_t adm_bpsec_get_num_fwd_bcb_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BCB_FWD, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Tx BIB blocks from SRC
 */
value_t adm_bpsec_get_num_good_tx_bib_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_TX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed Tx BIB blocks from SRC
 */
value_t adm_bpsec_get_num_bad_tx_bib_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_TX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Rx BIB blocks from SRC
 */
value_t adm_bpsec_get_num_good_rx_bib_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_RX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed Rx BIB blocks from SRC
 */
value_t adm_bpsec_get_num_bad_rx_bib_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_RX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Missing-on-Rx BIB blocks from SRC
 */
value_t adm_bpsec_get_num_miss_rx_bib_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_miss_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_RX_MISS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_miss_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Forwarded BIB blocks from SRC
 */
value_t adm_bpsec_get_num_fwd_bib_blks_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_FWD, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Tx BIB bytes from SRC
 */
value_t adm_bpsec_get_num_good_tx_bib_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_TX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed Tx BIB bytes from SRC
 */
value_t adm_bpsec_get_num_bad_tx_bib_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_TX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Successfully Rx BIB bytes from SRC
 */
value_t adm_bpsec_get_num_good_rx_bib_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_RX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Failed Rx BIB bytes from SRC
 */
value_t adm_bpsec_get_num_bad_rx_bib_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_RX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Missing-on-Rx BIB bytes from SRC
 */
value_t adm_bpsec_get_num_missing_rx_bib_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_RX_MISS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Forwarded BIB bytes from SRC
 */
value_t adm_bpsec_get_num_fwd_bib_bytes_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(params, BIB_FWD, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Last BPSEC Update from SRC
 */
value_t adm_bpsec_get_last_update_src(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_update_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	time_t time = 0;
	char *name = NULL;
	int8_t success = 0;

	result.type = AMP_TYPE_UNK;

	if((name = adm_extract_string(params, 0, &success)) == NULL)
	{
		return result;
	}

	if(bpsec_instr_get_src_update(name, &time) != ERROR)
	{
		result.type = AMP_TYPE_TS;
		result.value.as_uint = time;
	}

	SRELEASE(name);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_last_update_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Last reset
 */
value_t adm_bpsec_get_last_reset(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_reset BODY
	 * +-------------------------------------------------------------------------+
	 */
	bpsec_instr_misc_t misc;

	if(bpsec_instr_get_misc(&misc) == ERROR)
	{
		result.type = AMP_TYPE_UNK;
		return result;
	}

	result.type = AMP_TYPE_UINT;
	result.value.as_uint = misc.last_reset;
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_last_reset BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * This control causes the Agent to reset all counts associated with block or byte statistics and to se
 * t the Last Reset Time of the BPsec EDD data to the time when the control was run.
 */
tdc_t* adm_bpsec_ctrl_rst_all_cnts(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_rst_all_cnts BODY
	 * +-------------------------------------------------------------------------+
	 */
	bpsec_instr_reset();
	*status = CTRL_SUCCESS;
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_rst_all_cnts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control causes the Agent to reset all counts (blocks and bytes) associated with a given bundle 
 * source and set the Last Reset Time of the source statistics to the time when the control was run.
 */
tdc_t* adm_bpsec_ctrl_rst_src_cnts(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_rst_src_cnts BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	int8_t success = 0;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((src = adm_extract_string(params,0,&success)) != NULL)
	{
		bpsec_instr_reset_src(src);
		SRELEASE(src);
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_rst_src_cnts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes a key from the BPsec system.
 */
tdc_t* adm_bpsec_ctrl_delete_key(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_delete_key BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;
	int8_t success = 0;
	/* Step 1: Grab the name of the key to delete. */
	if((name = adm_extract_string(params,0,&success)) == NULL)
	{
		return NULL;
	}

	/*
	 * Step 2: Make sure key to be deleted is not an active key. Deleting
	 * an active key can lock someone out of the system
	 */
	if(sec_activeKey(name) != 0)
	{
		AMP_DEBUG_WARN("adm_bpsec_ctrl_delete_key","Can't remove active key %s", name);
		SRELEASE(name);
		return NULL;
	}

	if(sec_removeKey(name) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	SRELEASE(name);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_delete_key BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control adds a key to the BPsec system.
 */
tdc_t* adm_bpsec_ctrl_add_key(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_key BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;
	blob_t *value = NULL;
	int8_t success = 0;
	/* Step 1: Grab the name of the new key. */
	if((name = adm_extract_string(params,0,&success)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Grab the key value. */
	if((value = adm_extract_blob(params,1,&success)) == NULL)
	{
		SRELEASE(name);
		return NULL;
	}

	if(sec_addKeyValue(name, (char *)value->value, value->length) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	SRELEASE(name);
	blob_destroy(value,1);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_key BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures policy on the BPsec protocol implementation that describes how BIB blocks sh
 * ould be applied to bundles in the system. This policy is captured as a rule which states when transm
 * itting a bundle from the given source endpoint ID to the given destination endpoint ID, blocks of ty
 * pe target should have a BIB added to them using the given ciphersuite and the given key.
 */
tdc_t* adm_bpsec_ctrl_add_bib_rule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_bib_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	char *cs = NULL;
	char *key = NULL;
	int8_t success = 0;
	/* Step 1: Grab the name of the new key. */
	src = adm_extract_string(params, 0, &success);
	dst = adm_extract_string(params, 1, &success);
	tgt = adm_extract_uint(params, 2, &success);
	cs = adm_extract_string(params, 3, &success);
	key = adm_extract_string(params, 4, &success);

	if(get_bib_prof_by_name(cs) != NULL)
	{
		Object addr;
		Object elt;

		/* Step 3: Check to see if key exists. */
		sec_findKey(key, &addr, &elt);
		if(elt != 0)
		{
			/* Step 4: Update the BCB Rule. */
			if(sec_addBspBibRule(src, dst, tgt, cs, key) == 1)
			{
				*status = CTRL_SUCCESS;
			}
			else
			{
				AMP_DEBUG_ERR("adm_bpsec_ctrl_add_bib_rule", "Can't update rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("adm_bpsec_ctrl_add_bib_rule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("adm_bpsec_ctrl_add_bib_rule", "CIphersuite %s not supported.", cs);
	}

	SRELEASE(src);
	SRELEASE(dst);
	SRELEASE(cs);
	SRELEASE(key);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_bib_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes any configured policy on the BPsec protocol implementation that describes how B
 * IB blocks should be applied to bundles in the system. A BIB policy is uniquely identified by a sourc
 * e endpoint Id, a destination Id, and a target block type.
 */
tdc_t* adm_bpsec_ctrl_del_bib_rule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_bib_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	int8_t success = 0;
	/* Step 1: Grab the name of the new key. */
	src = adm_extract_string(params, 0, &success);
	dst = adm_extract_string(params, 1, &success);
	tgt = adm_extract_uint(params, 2, &success);


	if(sec_removeBspBibRule(src, dst, tgt) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	SRELEASE(src);
	SRELEASE(dst);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_bib_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control returns a table describinng all of the BIB policy rules that are known to the BPsec imp
 * lementation.
 */
tdc_t* adm_bpsec_ctrl_list_bib_rules(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_list_bib_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	table_t *table = adm_bpsec_tbl_bib_rules();
	uint8_t *data = NULL;
	uint32_t len = 0;

	if(table == NULL)
	{
		AMP_DEBUG_ERR("adm_bpsec_ctrl_list_bib_rules", "Can't get rules.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the return value. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_bpsec_ctrl_list_bib_rules","Can't make TDC.", NULL);
		return NULL;
	}

	/* Step 3: Populate the retval. */
	if((data = table_serialize(table, &len)) == NULL)
	{
		table_destroy(table, 1);
		tdc_destroy(&result);

		AMP_DEBUG_ERR("adm_bpsec_ctrl_list_bib_rules","Can't serialize table.", NULL);
		return NULL;
	}

	table_destroy(table, 1);

	tdc_insert(result, AMP_TYPE_TABLE, data, len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_bib_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control configures policy on the BPsec protocol implementation that describes how BCB blocks sh
 * ould be applied to bundles in the system. This policy is captured as a rule which states when transm
 * itting a bundle from the given source endpoint id to the given destination endpoint id, blocks of ty
 * pe target should have a bcb added to them using the given ciphersuite and the given key.
 */
tdc_t* adm_bpsec_ctrl_add_bcb_rule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_bcb_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	char *cs = NULL;
	char *key = NULL;
	int8_t success = 0;

	/* Step 1: Grab the name of the new key. */
	src = adm_extract_string(params, 0, &success);
	dst = adm_extract_string(params, 1, &success);
	tgt = adm_extract_int(params, 2, &success);
	cs = adm_extract_string(params, 3, &success);
	key = adm_extract_string(params, 4, &success);


	if(get_bcb_prof_by_name(cs) != NULL)
	{
		Object addr;
		Object elt;

		/* Step 3: Check to see if key exists. */
		sec_findKey(key, &addr, &elt);
		if(elt != 0)
		{
			/* Step 4: Update the BCB Rule. */
			if(sec_addBspBcbRule(src, dst, tgt, cs, key) == 1)
			{
				*status = CTRL_SUCCESS;
			}
			else
			{
				AMP_DEBUG_ERR("adm_bpsec_ctrl_add_bcbrule", "Can't add rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("adm_bpsec_ctrl_add_bcbrule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("adm_bpsec_ctrl_add_bcbrule", "Ciphersuite %s not supported.", cs);
	}


	SRELEASE(src);
	SRELEASE(dst);
	SRELEASE(cs);
	SRELEASE(key);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_add_bcb_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control removes any configured policy on the BPsec protocol implementation that describes how B
 * CB blocks should be applied to bundles in the system. A bcb policy is uniquely identified by a sourc
 * e endpoint id, a destination endpoint id, and a target block type.
 */
tdc_t* adm_bpsec_ctrl_del_bcb_rule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_bcb_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	int8_t success = 0;

	/* Step 1: Grab the name of the new key. */
	src = adm_extract_string(params, 0, &success);
	dst = adm_extract_string(params, 1, &success);
	tgt = adm_extract_int(params, 2, &success);

	if(sec_removeBspBcbRule(src, dst, tgt) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	SRELEASE(src);
	SRELEASE(dst);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_bcb_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control returns a table describing all of the bcb policy rules that are known to the BPsec impl
 * ementation
 */
tdc_t* adm_bpsec_ctrl_list_bcb_rules(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_list_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	table_t *table = adm_bpsec_tbl_bcb_rules();
	uint8_t *data = NULL;
	uint32_t len = 0;

	if(table == NULL)
	{
		AMP_DEBUG_ERR("adm_bpsec_ctrl_list_bcb_rules", "Can't get rules.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the return value. */
	if((result = tdc_create(NULL, NULL, 0)) == NULL)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_bpsec_ctrl_list_bcb_rules","Can't make TDC.", NULL);
		return NULL;
	}

	/* Step 3: Populate the result. */
	if((data = table_serialize(table, &len)) == NULL)
	{
		table_destroy(table, 1);
		tdc_destroy(&result);

		AMP_DEBUG_ERR("adm_bpsec_ctrl_list_bcb_rules","Can't serialize table.", NULL);
		return NULL;
	}

	table_destroy(table, 1);

	tdc_insert(result, AMP_TYPE_TABLE, data, len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
