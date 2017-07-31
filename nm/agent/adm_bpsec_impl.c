/*****************************************************************************
 **
 ** File Name: adm_bpsec_impl.c
 **
 ** Description: This implements the private aspects of a BPSEC ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
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

/* Meta-Data Functions. */
value_t adm_bpsec_md_name(tdc_t params)
{
	return val_from_string("BPSEC ADM");
}

value_t adm_bpsec_md_ver(tdc_t params)
{
	return val_from_string("2016_05_16");
}

/* Retrieval Functions. */


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



value_t adm_bpsec_get_tot_good_tx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_TX_PASS, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_fail_tx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_good_rx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_RX_PASS, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_fail_rx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_RX_FAIL, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_missing_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_RX_MISS, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_forward_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_FWD, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_good_tx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_TX_PASS, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_fail_tx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_TX_FAIL, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_good_rx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_RX_PASS, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_fail_rx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_RX_FAIL, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_missing_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_RX_MISS, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_forward_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BCB_FWD, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_good_tx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_TX_PASS, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_fail_tx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_TX_FAIL, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_good_rx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_RX_PASS, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_fail_rx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_RX_FAIL, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_missing_bib_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_RX_MISS, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_forward_bib_blks(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_FWD, TOTAL_BLK);
}

value_t adm_bpsec_get_tot_good_tx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_TX_PASS, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_fail_tx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_TX_FAIL, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_good_rx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_RX_PASS, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_fail_rx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_RX_FAIL, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_missing_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_RX_MISS, TOTAL_BYTES);
}

value_t adm_bpsec_get_tot_forward_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_tot_val( BIB_FWD, TOTAL_BYTES);
}

value_t adm_bpsec_get_last_update(tdc_t params)
{
	value_t result;
	result.type = AMP_TYPE_UNK;
	if(bpsec_instr_get_tot_update((time_t*)&(result.value.as_uint)) != ERROR)
	{
		result.type = AMP_TYPE_TS;
	}
	return result;
}

value_t adm_bpsec_get_num_keys(tdc_t params)
{
	value_t result;
	uint32_t size = 0;

	result.type = AMP_TYPE_UINT;
	result.value.as_uint = bpsec_instr_get_num_keys((int*)&size);
	return result;
}


value_t adm_bpsec_get_keys(tdc_t params)
{
	value_t result;
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

	return result;
}

value_t adm_bpsec_get_ciphs(tdc_t params)
{
	value_t result;
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

	return result;
}

value_t adm_bpsec_get_srcs(tdc_t params)
{
	value_t result;
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

	return result;
}


value_t adm_bpsec_get_src_good_tx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_TX_PASS, SRC_BLK);
}

value_t adm_bpsec_get_src_fail_tx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_TX_FAIL, SRC_BLK);
}

value_t adm_bpsec_get_src_good_rx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_RX_PASS, SRC_BLK);
}

value_t adm_bpsec_get_src_fail_rx_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_RX_FAIL, SRC_BLK);
}

value_t adm_bpsec_get_src_missing_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_RX_MISS, SRC_BLK);
}

value_t adm_bpsec_get_src_forward_bcb_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_FWD, SRC_BLK);
}

value_t adm_bpsec_get_src_good_tx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_TX_PASS, SRC_BYTES);
}

value_t adm_bpsec_get_src_fail_tx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_TX_FAIL, SRC_BYTES);
}

value_t adm_bpsec_get_src_good_rx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_RX_PASS, SRC_BYTES);
}

value_t adm_bpsec_get_src_fail_rx_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_RX_FAIL, SRC_BYTES);
}

value_t adm_bpsec_get_src_missing_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_RX_MISS, SRC_BYTES);
}

value_t adm_bpsec_get_src_forward_bcb_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BCB_FWD, SRC_BYTES);
}

value_t adm_bpsec_get_src_good_tx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_TX_PASS, SRC_BLK);
}

value_t adm_bpsec_get_src_fail_tx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_TX_FAIL, SRC_BLK);
}

value_t adm_bpsec_get_src_good_rx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_RX_PASS, SRC_BLK);
}

value_t adm_bpsec_get_src_fail_rx_bib_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_RX_FAIL, SRC_BLK);
}

value_t adm_bpsec_get_src_missing_bib_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_RX_MISS, SRC_BLK);
}

value_t adm_bpsec_get_src_forward_bib_blks(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_FWD, SRC_BLK);
}


value_t adm_bpsec_get_src_good_tx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_TX_PASS, SRC_BYTES);
}

value_t adm_bpsec_get_src_fail_tx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_TX_FAIL, SRC_BYTES);
}

value_t adm_bpsec_get_src_good_rx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_RX_PASS, SRC_BYTES);
}

value_t adm_bpsec_get_src_fail_rx_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_RX_FAIL, SRC_BYTES);
}

value_t adm_bpsec_get_src_missing_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_RX_MISS, SRC_BYTES);
}

value_t adm_bpsec_get_src_forward_bib_bytes(tdc_t params)
{
	return adm_bpsec_get_src_val(params, BIB_FWD, SRC_BYTES);
}


value_t adm_bpsec_get_src_last_update(tdc_t params)
{
	value_t result;
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
	return result;
}



value_t adm_bpsec_get_last_reset(tdc_t params)
{
	value_t result;
	bpsec_instr_misc_t misc;

	if(bpsec_instr_get_misc(&misc) == ERROR)
	{
		result.type = AMP_TYPE_UNK;
		return result;
	}

	result.type = AMP_TYPE_UINT;
	result.value.as_uint = misc.last_reset;
	return result;
}




/* Control Functions */



tdc_t* adm_bpsec_ctl_reset_all(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	CHKNULL(status);

	bpsec_instr_reset();
	*status = CTRL_SUCCESS;

	return NULL;
}

tdc_t* adm_bpsec_ctl_reset_src(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the MID defining the new computed definition. */
	if((src = adm_extract_string(params,0,&success)) == NULL)
	{
		return NULL;
	}

	bpsec_instr_reset_src(src);

	SRELEASE(src);

	*status = CTRL_SUCCESS;
	return NULL;
}



 /*
  * DeleteKey(STR keyname)
  */
tdc_t* adm_bpsec_ctl_del_key(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *name = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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
		AMP_DEBUG_WARN("adm_bpsec_ctl_del_key","Can't remove active key %s", name);
		SRELEASE(name);
		return NULL;
	}

	if(sec_removeKey(name) == 1)
	{
		*status = CTRL_SUCCESS;
	}

	SRELEASE(name);

	return NULL;
}

/*
 * AddKey(STR keyname, BLOB key)
 */
tdc_t* adm_bpsec_ctl_add_key(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *name = NULL;
	blob_t *value = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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
	return NULL;
}

/*
 *  AddBibRule(STR src, STR dest, INT tgt, STR cs, STR key)
 */
tdc_t* adm_bpsec_ctl_add_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	char *cs = NULL;
	char *key = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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
				AMP_DEBUG_ERR("adm_bpsec_ctl_add_bibrule", "Can't update rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("adm_bpsec_ctl_add_bibrule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("adm_bpsec_ctl_add_bibrule", "CIphersuite %s not supported.", cs);
	}

	SRELEASE(src);
	SRELEASE(dst);
	SRELEASE(cs);
	SRELEASE(key);

	return NULL;
}

/*
 * RemoveBibRule(STR src, STR dest, INT tgt)
 */
tdc_t* adm_bpsec_ctl_del_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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

	return NULL;
}

/* ListBibRules()
 *
 * Returns table
 *
 * +---------+---------+-------------+---------+---------+
 * | SrcEid  | DestEid | TargetBlock | CS Name | KeyName |
 * | STRING  | STRING  |    UINT     | STRING  | STRING  |
 * +---------+---------+-------------+---------+---------+
 *
 *
 * */

table_t *get_bib_rules()
{
	Sdr sdr = getIonsdr();
	Object listObj = 0;
	Object	elt = 0;
	Object addr = 0;
	OBJ_POINTER(BspBibRule, rule);
	int	 strLen = 0;
	char strBuffer[SDRSTRING_BUFSZ];
	table_t  *table = NULL;
	Lyst cur_row = NULL;
	uint8_t *data = NULL;
	uint32_t len;
	int8_t success = 0;

	if((listObj = sec_get_bspBibRuleList()) == 0)
	{
		AMP_DEBUG_ERR("adm_bpsec_get_bib_rules","Cannot get list.", NULL);
		return NULL;
	}


	if((table = table_create(NULL, NULL)) == NULL)
	{
		AMP_DEBUG_ERR("adm_bpsec_get_bib_rules","Cannot allocate table.", NULL);
		return NULL;
	}

	if((table_add_col(table, "SrcEid", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "DestEid", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "TgtBlk", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "csName", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "keyName", AMP_TYPE_STRING) == ERROR))
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_bpsec_get_bib_rules","Cannot add columns.", NULL);
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

					AMP_DEBUG_ERR("adm_bpsec_get_bib_rules", "Error extracting rule", NULL);
					return NULL;
				}
				else
				{
					table_add_row(table, cur_row);
				}
			}
			else
			{
				AMP_DEBUG_WARN("adm_bpsec_get_bib_rules", "NULL rule?", NULL);
			}
		}
		else
		{
			AMP_DEBUG_WARN("adm_bpsec_get_bib_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	return table;
}



/* ListBcbRules()
 *
 * Returns table
 *
 * +---------+---------+-------------+---------+---------+
 * | SrcEid  | DestEid | TargetBlock | CS Name | KeyName |
 * | STRING  | STRING  |    UINT     | STRING  | STRING  |
 * +---------+---------+-------------+---------+---------+
 *
 *
 * */

table_t *get_bcb_rules()
{
	Sdr sdr = getIonsdr();
	Object listObj = 0;
	Object	elt = 0;
	Object addr = 0;
	OBJ_POINTER(BspBcbRule, rule);
	int	 strLen = 0;
	char strBuffer[SDRSTRING_BUFSZ];
	table_t  *table = NULL;
	Lyst cur_row = NULL;
	uint8_t *data = NULL;
	uint32_t len;
	int8_t success = 0;

	if((listObj = sec_get_bspBcbRuleList()) == 0)
	{
		AMP_DEBUG_ERR("get_bcb_rules","Cannot get list.", NULL);
		return NULL;
	}


	if((table = table_create(NULL, NULL)) == NULL)
	{
		AMP_DEBUG_ERR("get_bcb_rules","Cannot allocate table.", NULL);
		return NULL;
	}

	if((table_add_col(table, "SrcEid", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "DestEid", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "TgtBlk", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "csName", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "keyName", AMP_TYPE_STRING) == ERROR))
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("get_bcb_rules","Cannot add columns.", NULL);
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

					AMP_DEBUG_ERR("get_bcb_rules", "Error extracting rule", NULL);
					return NULL;
				}
				else
				{
					table_add_row(table, cur_row);
				}
			}
			else
			{
				AMP_DEBUG_WARN("get_bcb_rules", "NULL rule?", NULL);
			}
		}
		else
		{
			AMP_DEBUG_WARN("get_bcb_rules", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	return table;
}

tdc_t* adm_bpsec_ctl_list_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{

	tdc_t *retval = NULL;
	table_t *table = get_bib_rules();
	uint8_t *data = NULL;
	uint32_t len = 0;

	*status = CTRL_FAILURE;

	if(table == NULL)
	{
		AMP_DEBUG_ERR("adm_bpsec_ctl_list_bibrule", "Can't get rules.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_bpsec_ctl_list_bibrule","Can't make TDC.", NULL);
		return NULL;
	}

	/* Step 3: Populate the retval. */
	if((data = table_serialize(table, &len)) == NULL)
	{
		table_destroy(table, 1);
		tdc_destroy(&retval);

		AMP_DEBUG_ERR("adm_bpsec_ctl_list_bibrule","Can't serialize table.", NULL);
		return NULL;
	}

	table_destroy(table, 1);

	tdc_insert(retval, AMP_TYPE_TABLE, data, len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}

/*
 * AddBcbRule(STR src, STR dst, INT tgt, STR cs, STR key)
 */
tdc_t* adm_bpsec_ctl_add_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	char *cs = NULL;
	char *key = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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
				AMP_DEBUG_ERR("adm_bpsec_ctl_add_bcbrule", "Can't add rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("adm_bpsec_ctl_add_bcbrule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("adm_bpsec_ctl_add_bcbrule", "Ciphersuite %s not supported.", cs);
	}


	SRELEASE(src);
	SRELEASE(dst);
	SRELEASE(cs);
	SRELEASE(key);

	return NULL;
}

/*
 * RemoveBcbRule(STR src, STR dest, INT tgt)
 */
tdc_t* adm_bpsec_ctl_del_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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

	return NULL;
}

/*
 * ListBcbRules()
 */
tdc_t* adm_bpsec_ctl_list_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{

	tdc_t *retval = NULL;
	table_t *table = get_bcb_rules();
	uint8_t *data = NULL;
	uint32_t len = 0;

	*status = CTRL_FAILURE;

	if(table == NULL)
	{
		AMP_DEBUG_ERR("adm_bpsec_ctl_list_bcbrule", "Can't get rules.", NULL);
		return NULL;
	}

	/* Step 2: Allocate the return value. */
	if((retval = tdc_create(NULL, NULL, 0)) == NULL)
	{
		table_destroy(table, 1);
		AMP_DEBUG_ERR("adm_bpsec_ctl_list_bcbrule","Can't make TDC.", NULL);
		return NULL;
	}

	/* Step 3: Populate the retval. */
	if((data = table_serialize(table, &len)) == NULL)
	{
		table_destroy(table, 1);
		tdc_destroy(&retval);

		AMP_DEBUG_ERR("adm_bpsec_ctl_list_bcbrule","Can't serialize table.", NULL);
		return NULL;
	}

	table_destroy(table, 1);

	tdc_insert(retval, AMP_TYPE_TABLE, data, len);
	SRELEASE(data);

	*status = CTRL_SUCCESS;

	return retval;
}


/*
 *  UpdateBibRule(STR src, STR dest, INT tgt, STR cs, STR key)
 */
tdc_t* adm_bpsec_ctl_update_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	char *cs = NULL;
	char *key = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

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
			if(sec_updateBspBibRule(src, dst, tgt, cs, key) == 1)
			{
				*status = CTRL_SUCCESS;
			}
			else
			{
				AMP_DEBUG_ERR("adm_bpsec_ctl_update_bibrule", "Can't update rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("adm_bpsec_ctl_update_bibrule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("adm_bpsec_ctl_update_bibrule", "CIphersuite %s not supported.", cs);
	}

	SRELEASE(src);
	SRELEASE(dst);
	SRELEASE(cs);
	SRELEASE(key);

	return NULL;
}



/*
 *  UpdateBcbRule(STR src, STR dest, INT tgt, STR cs, STR key)
 */
tdc_t* adm_bpsec_ctl_update_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	char *src = NULL;
	char *dst = NULL;
	uint32_t tgt = 0;
	char *cs = NULL;
	char *key = NULL;
	int8_t success = 0;

	*status = CTRL_FAILURE;

	/* Step 1: Grab the name of the new key. */
	src = adm_extract_string(params, 0, &success);
	dst = adm_extract_string(params, 1, &success);
	tgt = adm_extract_uint(params, 2, &success);
	cs = adm_extract_string(params, 3, &success);
	key = adm_extract_string(params, 4, &success);


	/* Step 2: Check to make sure Ciphersuite is supported. */
	if(get_bcb_prof_by_name(cs) != NULL)
	{
		Object addr;
		Object elt;

		/* Step 3: Check to see if key exists. */
		sec_findKey(key, &addr, &elt);
		if(elt != 0)
		{
			/* Step 4: Update the BCB Rule. */
			if(sec_updateBspBcbRule(src, dst, tgt, cs, key) == 1)
			{
				*status = CTRL_SUCCESS;
			}
			else
			{
				AMP_DEBUG_ERR("adm_bpsec_ctl_update_bcbrule", "Can't update rule.", NULL);
			}
		}
		else
		{
			AMP_DEBUG_ERR("adm_bpsec_ctl_update_bcbrule", "Key %s doesn't exist.", key);
		}
	}
	else
	{
		AMP_DEBUG_ERR("adm_bpsec_ctl_update_bcbrule", "CIphersuite %s not supported.", cs);
	}

	SRELEASE(src);
	SRELEASE(dst);
	SRELEASE(cs);
	SRELEASE(key);

	return NULL;
}
