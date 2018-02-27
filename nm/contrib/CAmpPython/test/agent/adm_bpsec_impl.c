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
 **  2018-02-07  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
/*             TODO              */
/*   STOP CUSTOM INCLUDES HERE  */


#include "adm_bpsec_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
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
	return val_from_str("adm_bpsec");
}


value_t adm_bpsec_meta_namespace(tdc_t params)
{
	return val_from_str("arn:DTN:bpsec");
}


value_t adm_bpsec_meta_version(tdc_t params)
{
	return val_from_str("2016_05_16");
}


value_t adm_bpsec_meta_organization(tdc_t params)
{
	return val_from_str("JHUAPL");
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
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_list_bcb_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
