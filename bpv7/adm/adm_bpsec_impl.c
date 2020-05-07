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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include <math.h>

#include "adm_bpsec_impl.h"
#include "shared/primitives/report.h"
#include "agent/rda.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"

#include "adm_bpsec.h"
#include "profiles.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_bpsec_impl.h"

/*   START CUSTOM FUNCTIONS HERE */


static tnv_t *adm_bpsec_get_src_val(tnvc_t *parms, bpsec_instr_type_e type, query_type_e query)
{
    tnv_t *result = tnv_create();
	char *eid_id = NULL;
	uvast num = 0;
	int success = ERROR;

	result->type = AMP_TYPE_UNK;


	if((eid_id = adm_get_parm_obj(parms, 0, AMP_TYPE_STR)) == NULL)
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
		result->type = AMP_TYPE_UVAST;
		result->value.as_uvast = num;
	}

	return result;
}

static tnv_t *adm_bpsec_get_tot_val(bpsec_instr_type_e type, query_type_e query)
{
        tnv_t *result = tnv_create();
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
		result->type = AMP_TYPE_UNK;
	}
	else
	{

		result->type = AMP_TYPE_UVAST;
		result->value.as_uvast = num;
	}

	return result;
}

/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_bpsec_setup()
{

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

void dtn_bpsec_cleanup()
{

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


tnv_t *dtn_bpsec_meta_name(tnvc_t *parms)
{
	return tnv_from_str("bpsec");
}


tnv_t *dtn_bpsec_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/bpsec");
}


tnv_t *dtn_bpsec_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v1.0");
}


tnv_t *dtn_bpsec_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * This table lists all keys in the security policy database.
 */
tbl_t *dtn_bpsec_tblt_keys(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists supported ciphersuites.
 */
tbl_t *dtn_bpsec_tblt_ciphersuites(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_ciphersuites BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_ciphersuites BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * BIB Rules.
 */
tbl_t *dtn_bpsec_tblt_bib_rules(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_bib_rules BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr sdr = getIonsdr();
	Object listObj = 0;
	Object	elt = 0;
	OBJ_POINTER(BPsecBibRule, rule);
	char strBuffer[SDRSTRING_BUFSZ];
	tnvc_t *cur_row = NULL;
	int len = 0;

	if((listObj = sec_get_bpsecBibRuleList()) == 0)
	{
		AMP_DEBUG_ERR("dtn_bpsec_tblt_bib_rules","Cannot get list.", NULL);
		tbl_release(table, 1);
		return NULL;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		AMP_DEBUG_ERR("dtn_bpsec_tblt_bib_rules","Can't start transaction.", NULL);
		tbl_release(table, 1);
		return NULL;
	}

	for (elt = sdr_list_first(sdr, listObj); elt; elt = sdr_list_next(sdr, elt))
	{

		if((cur_row = tnvc_create(5)) != NULL)
		{
			GET_OBJ_POINTER(sdr, BPsecBibRule, rule, sdr_list_data(sdr, elt));

			if(rule != NULL)
			{
				len = sdr_string_read(sdr, strBuffer, rule->securitySrcEid);
				tnvc_insert(cur_row, tnv_from_str( (len > 0) ? strBuffer : "unk"));

				len = sdr_string_read(sdr, strBuffer, rule->destEid);
				tnvc_insert(cur_row, tnv_from_str( (len > 0) ? strBuffer : "unk"));

				tnvc_insert(cur_row, tnv_from_uint(rule->blockType));
				tnvc_insert(cur_row, tnv_from_str(rule->ciphersuiteName));
				tnvc_insert(cur_row, tnv_from_str(rule->keyName));

				tbl_add_row(table, cur_row);
			}
			else
			{
				AMP_DEBUG_WARN("dtn_bpsec_tblt_bib_rules", "NULL rule?", NULL);
			}
		}
		else
		{
			AMP_DEBUG_WARN("dtn_bpsec_tblt_bib_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_bib_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * BCB Rules.
 */
tbl_t *dtn_bpsec_tblt_bcb_rules(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr sdr = getIonsdr();
	Object listObj = 0;
	Object	elt = 0;
	OBJ_POINTER(BPsecBcbRule, rule);
	char strBuffer[SDRSTRING_BUFSZ];
	tnvc_t *cur_row = NULL;
	int len = 0;

	if((listObj = sec_get_bpsecBcbRuleList()) == 0)
	{
		AMP_DEBUG_ERR("dtn_bpsec_tblt_bcb_rules","Cannot get list.", NULL);
		tbl_release(table, 1);
		return NULL;
	}

	if (sdr_begin_xn(sdr) < 0)
	{
		AMP_DEBUG_ERR("dtn_bpsec_tblt_bcb_rules","Can't start transaction.", NULL);
		tbl_release(table, 1);
		return NULL;
	}

	for (elt = sdr_list_first(sdr, listObj); elt; elt = sdr_list_next(sdr, elt))
	{
		if((cur_row = tnvc_create(5)) != NULL)
		{
			GET_OBJ_POINTER(sdr, BPsecBcbRule, rule, sdr_list_data(sdr, elt));

			if(rule != NULL)
			{
				len = sdr_string_read(sdr, strBuffer, rule->securitySrcEid);
				tnvc_insert(cur_row, tnv_from_str( (len > 0) ? strBuffer : "unk"));

				len = sdr_string_read(sdr, strBuffer, rule->destEid);
				tnvc_insert(cur_row, tnv_from_str( (len > 0) ? strBuffer : "unk"));

				tnvc_insert(cur_row, tnv_from_uint(rule->blockType));
				tnvc_insert(cur_row, tnv_from_str(rule->ciphersuiteName));
				tnvc_insert(cur_row, tnv_from_str(rule->keyName));

				tbl_add_row(table, cur_row);
			}
			else
			{
				AMP_DEBUG_WARN("dtn_bpsec_tblt_bcb_rules", "NULL rule?", NULL);
			}
		}
		else
		{
			AMP_DEBUG_WARN("dtn_bpsec_tblt_bcb_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * Total successfully Tx Bundle Confidentiality blocks
 */
tnv_t *dtn_bpsec_get_num_good_tx_bcb_blk(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_TX_PASS, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx Block Confidentiality Block (BCB) blocks
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bcb_blk(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_good_rx_bcb_blk(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_RX_PASS, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_bad_rx_bcb_blk(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_blk BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_RX_FAIL, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_missing_rx_bcb_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_RX_MISS, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_fwd_bcb_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_FWD, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Tx BCB bytes
 */
tnv_t *dtn_bpsec_get_num_good_tx_bcb_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_TX_PASS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx BCB bytes
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bcb_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Tx BCB blocks
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bcb_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Rx BCB bytes
 */
tnv_t *dtn_bpsec_get_num_good_rx_bcb_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_RX_PASS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total unsuccessfully Rx BCB bytes
 */
tnv_t *dtn_bpsec_get_num_bad_rx_bcb_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_RX_FAIL, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total missing-on-Rx BCB bytes
 */
tnv_t *dtn_bpsec_get_num_missing_rx_bcb_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_RX_MISS, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total forwarded BCB bytes
 */
tnv_t *dtn_bpsec_get_num_fwd_bcb_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BCB_FWD, TOTAL_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total successfully Tx Block Integrity Block (BIB) blocks
 */
tnv_t *dtn_bpsec_get_num_good_tx_bib_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_TX_PASS, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_bad_tx_bib_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_TX_FAIL, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_good_rx_bib_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_RX_PASS, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_bad_rx_bib_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_RX_FAIL, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_miss_rx_bib_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_miss_rx_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_RX_MISS, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_fwd_bib_blks(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_blks BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_FWD, TOTAL_BLK);
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
tnv_t *dtn_bpsec_get_num_good_tx_bib_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_TX_PASS, TOTAL_BYTES);
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
tnv_t *dtn_bpsec_get_num_bad_tx_bib_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_TX_FAIL, TOTAL_BYTES);
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
tnv_t *dtn_bpsec_get_num_good_rx_bib_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_RX_PASS, TOTAL_BYTES);
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
tnv_t *dtn_bpsec_get_num_bad_rx_bib_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_RX_FAIL, TOTAL_BYTES);
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
tnv_t *dtn_bpsec_get_num_miss_rx_bib_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_miss_rx_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_RX_MISS, TOTAL_BYTES);
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
tnv_t *dtn_bpsec_get_num_fwd_bib_bytes(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_bytes BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_tot_val( BIB_FWD, TOTAL_BYTES);
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
tnv_t *dtn_bpsec_get_last_update(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_update BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_create();
	result->type = AMP_TYPE_UNK;
	if(bpsec_instr_get_tot_update((time_t*)&(result->value.as_uint)) != ERROR)
	{
		result->type = AMP_TYPE_TS;
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
tnv_t *dtn_bpsec_get_num_known_keys(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_known_keys BODY
	 * +-------------------------------------------------------------------------+
	 */
	uint32_t size = 0;
	result = tnv_create();
	result->type = AMP_TYPE_UINT;
	result->value.as_uint = bpsec_instr_get_num_keys((int*)&size);
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
tnv_t *dtn_bpsec_get_key_names(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_key_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *tmp = bpsec_instr_get_keynames();

	result = tnv_create();
	result->type = AMP_TYPE_UNK;
	/* TMP is allocated using MTAKE. We need to move it to
	 * something using STAKE.
	 */
	if(tmp != NULL)
	{
		uint32_t size = strlen(tmp) + 1;
		if((result->value.as_ptr = STAKE(size)) == NULL)
		{
			MRELEASE(tmp);
			return result;
		}
		memcpy(result->value.as_ptr, tmp, size);
		MRELEASE(tmp);

		result->type = AMP_TYPE_STR;
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
tnv_t *dtn_bpsec_get_ciphersuite_names(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_ciphersuite_names BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *tmp = bpsec_instr_get_csnames();

	result = tnv_create();
	result->type = AMP_TYPE_UNK;
	/* TMP is allocated using MTAKE. We need to move it to
	 * something using STAKE.
	 */
	if(tmp != NULL)
	{
		uint32_t size = strlen(tmp) + 1;
		if((result->value.as_ptr = STAKE(size)) == NULL)
		{
			MRELEASE(tmp);
			return result;
		}
		memcpy(result->value.as_ptr, tmp, size);
		MRELEASE(tmp);

		result->type = AMP_TYPE_STR;
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
tnv_t *dtn_bpsec_get_rule_source(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_rule_source BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *tmp = bpsec_instr_get_srcnames();

	result = tnv_create();
	result->type = AMP_TYPE_UNK;
	/* TMP is allocated using MTAKE. We need to move it to
	 * something using STAKE.
	 */
	if(tmp != NULL)
	{
		uint32_t size = strlen(tmp) + 1;
		if((result->value.as_ptr = STAKE(size)) == NULL)
		{
			MRELEASE(tmp);
			return result;
		}
		memcpy(result->value.as_ptr, tmp, size);
		result->type = AMP_TYPE_STR;
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_rule_source BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Tx BCB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_good_tx_bcb_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_TX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed TX BCB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bcb_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_TX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Rx BCB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_good_rx_bcb_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_RX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed RX BCB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_rx_bcb_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_RX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of missing-onRX BCB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_missing_rx_bcb_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_RX_MISS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of forwarded BCB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_fwd_bcb_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_FWD, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Tx bcb bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_good_tx_bcb_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_TX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed Tx bcb bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bcb_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_TX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Rx bcb bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_good_rx_bcb_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_RX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed Rx bcb bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_rx_bcb_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_RX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of missing-on-Rx bcb bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_missing_rx_bcb_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_RX_MISS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of forwarded bcb bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_fwd_bcb_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BCB_FWD, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bcb_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Tx BIB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_good_tx_bib_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_TX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed Tx BIB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bib_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_TX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Rx BIB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_good_rx_bib_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_RX_PASS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed Rx BIB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_rx_bib_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_RX_FAIL, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of missing-on-Rx BIB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_miss_rx_bib_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_miss_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_RX_MISS, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_miss_rx_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of forwarded BIB blocks from SRC
 */
tnv_t *dtn_bpsec_get_num_fwd_bib_blks_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_FWD, SRC_BLK);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bib_blks_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Tx BIB bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_good_tx_bib_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_TX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed Tx BIB bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_tx_bib_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_TX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_tx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of successfully Rx BIB bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_good_rx_bib_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_good_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_RX_PASS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_good_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of failed Rx BIB bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_bad_rx_bib_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_bad_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_RX_FAIL, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_bad_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of missing-on-Rx BIB bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_missing_rx_bib_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_missing_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return adm_bpsec_get_src_val(parms, BIB_RX_MISS, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_missing_rx_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of forwarded BIB bytes from SRC
 */
tnv_t *dtn_bpsec_get_num_fwd_bib_bytes_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_fwd_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = adm_bpsec_get_src_val(parms, BIB_FWD, SRC_BYTES);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_fwd_bib_bytes_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Last BPSEC update from SRC
 */
tnv_t *dtn_bpsec_get_last_update_src(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_update_src BODY
	 * +-------------------------------------------------------------------------+
	 */
	time_t time = 0;
	char *name = NULL;
	int success = 0;

	result = tnv_create();
	result->type = AMP_TYPE_UNK;

	if((name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR)) == NULL)
	{
		return result;
	}

	if(bpsec_instr_get_src_update(name, &time) != ERROR)
	{
		result->type = AMP_TYPE_TS;
		result->value.as_uint = time;
	}

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
tnv_t *dtn_bpsec_get_last_reset(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_last_reset BODY
	 * +-------------------------------------------------------------------------+
	 */
	bpsec_instr_misc_t misc;

	result = tnv_create();
	if(bpsec_instr_get_misc(&misc) == ERROR)
	{
		result->type = AMP_TYPE_UNK;
		return result;
	}

	result->type = AMP_TYPE_TS;
	result->value.as_uint = misc.last_reset;
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
tnv_t *dtn_bpsec_ctrl_rst_all_cnts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
tnv_t *dtn_bpsec_ctrl_rst_src_cnts(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_rst_src_cnts BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	int success = 0;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((src = adm_get_parm_obj(parms, 0, AMP_TYPE_STR)) != NULL)
	{
		bpsec_instr_reset_src(src);
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
tnv_t *dtn_bpsec_ctrl_delete_key(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_delete_key BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;
	int success = 0;
	/* Step 1: Grab the name of the key to delete. */
	if((name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR)) == NULL)
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
		return NULL;
	}

	if(sec_removeKey(name) == 1)
	{
		*status = CTRL_SUCCESS;
	}
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
tnv_t *dtn_bpsec_ctrl_add_key(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_add_key BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = NULL;
	blob_t *value = NULL;
	int success = 0;
	/* Step 1: Grab the name of the new key. */
	if((name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR)) == NULL)
	{
		return NULL;
	}

	/* Step 2: Grab the key value. */
	if((value = adm_get_parm_obj(parms, 1, AMP_TYPE_STR)) == NULL)
	{
		return NULL;
	}

	if(sec_addKeyValue(name, (char *)value->value, value->length) == 1)
	{
		*status = CTRL_SUCCESS;
	}

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
tnv_t *dtn_bpsec_ctrl_add_bib_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
	int success = 0;
	/* Step 1: Grab the name of the new key. */
	src = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	dst = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	tgt = adm_get_parm_uint(parms, 2, &success);
	cs = adm_get_parm_obj(parms, 3, AMP_TYPE_STR);
	key = adm_get_parm_obj(parms, 4, AMP_TYPE_STR);

	if(get_bib_prof_by_name(cs) != NULL)
	{
		Object addr;
		Object elt;

		/* Step 3: Check to see if key exists. */
		sec_findKey(key, &addr, &elt);
		if(elt != 0)
		{
			/* Step 4: Update the BCB Rule. */
			if(sec_addBPsecBibRule(src, dst, tgt, cs, key) == 1)
			{
				*status = CTRL_SUCCESS;
			}
			else
			{
				AMP_DEBUG_ERR("dtn_bpsec_ctrl_add_bib_rule", "Can't update rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("dtn_bpsec_ctrl_add_bib_rule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("dtn_bpsec_ctrl_add_bib_rule", "CIphersuite %s not supported.", cs);
	}

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
tnv_t *dtn_bpsec_ctrl_del_bib_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_bib_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	int success = 0;
	/* Step 1: Grab the name of the new key. */
	src = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	dst = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	tgt = adm_get_parm_uint(parms, 2, &success);


	if(sec_removeBPsecBibRule(src, dst, tgt) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_bib_rule BODY
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
tnv_t *dtn_bpsec_ctrl_add_bcb_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
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
	int success = 0;

	/* Step 1: Grab the name of the new key. */
	src = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	dst = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	tgt = adm_get_parm_int(parms, 2, &success);
	cs = adm_get_parm_obj(parms, 3, AMP_TYPE_STR);
	key = adm_get_parm_obj(parms, 4, AMP_TYPE_STR);


	if(get_bcb_prof_by_name(cs) != NULL)
	{
		Object addr;
		Object elt;

		/* Step 3: Check to see if key exists. */
		sec_findKey(key, &addr, &elt);
		if(elt != 0)
		{
			/* Step 4: Update the BCB Rule. */
			if(sec_addBPsecBcbRule(src, dst, tgt, cs, key) == 1)
			{
				*status = CTRL_SUCCESS;
			}
			else
			{
				AMP_DEBUG_ERR("dtn_bpsec_ctrl_add_bcbrule", "Can't add rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("dtn_bpsec_ctrl_add_bcbrule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("dtn_bpsec_ctrl_add_bcbrule", "Ciphersuite %s not supported.", cs);
	}

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
tnv_t *dtn_bpsec_ctrl_del_bcb_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_del_bcb_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	int success = 0;

	/* Step 1: Grab the name of the new key. */
	src = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	dst = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	tgt = adm_get_parm_int(parms, 2, &success);

	if(sec_removeBPsecBcbRule(src, dst, tgt) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_del_bcb_rule BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
