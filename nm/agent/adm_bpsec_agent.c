/****************************************************************************
 **
 ** File Name: adm_bpsec_agent.c
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


#include "ion.h"
#include "lyst.h"
#include "platform.h"
#include "../shared/adm/adm_bpsec.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/def.h"
#include "../shared/primitives/nn.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/blob.h"
#include "adm_bpsec_impl.h"
#include "rda.h"

#define _HAVE_BPSEC_ADM_
#ifdef _HAVE_BPSEC_ADM_

void adm_bpsec_init()
{
	adm_bpsec_setup();
	adm_bpsec_init_edd();
	adm_bpsec_init_variables();
	adm_bpsec_init_controls();
	adm_bpsec_init_constants();
	adm_bpsec_init_macros();
	adm_bpsec_init_metadata();
	adm_bpsec_init_ops();
	adm_bpsec_init_reports();
}

void adm_bpsec_init_edd()
{
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bcb_blk, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLK_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bcb_blk, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bcb_blk, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLK_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bcb_blk, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_missing_rx_bcb_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bcb_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bcb_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bcb_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bcb_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bcb_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_missing_rx_bcb_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bcb_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bib_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bib_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bib_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bib_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_miss_rx_bib_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bib_blks, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bib_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bib_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bib_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bib_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_miss_rx_bib_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bib_bytes, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_MID), AMP_TYPE_TS, 0, adm_bpsec_get_last_update, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_KNOWN_KEYS_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_known_keys, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_KEY_NAMES_MID), AMP_TYPE_STRING, 0, adm_bpsec_get_key_names, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_CIPHERSUITE_NAMES_MID), AMP_TYPE_STRING, 0, adm_bpsec_get_ciphersuite_names, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_RULE_SOURCE_MID), AMP_TYPE_STRING, 0, adm_bpsec_get_rule_source, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bcb_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bcb_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bcb_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bcb_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_missing_rx_bcb_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bcb_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bcb_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bcb_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bcb_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bcb_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_missing_rx_bcb_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bcb_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bib_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bib_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bib_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bib_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_miss_rx_bib_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bib_blks_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_tx_bib_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_tx_bib_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_good_rx_bib_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_bad_rx_bib_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_missing_rx_bib_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID), AMP_TYPE_UINT, 0, adm_bpsec_get_num_fwd_bib_bytes_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID), AMP_TYPE_TS, 0, adm_bpsec_get_last_update_src, NULL, NULL);
	adm_add_edd(mid_from_value(ADM_BPSEC_EDD_LAST_RESET_MID), AMP_TYPE_TS, 0, adm_bpsec_get_last_reset, NULL, NULL);

}

void adm_bpsec_init_variables()
{

//	TODO adm_add_var(mid_from_value(ADM_BPSEC_VAR_TOTAL_BAD_TX_BLKS_MID), adm_bpsec_var_total_bad_tx_blks);
}

void adm_bpsec_init_controls()
{
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_RST_ALL_CNTS_MID),adm_bpsec_ctrl_rst_all_cnts);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_RST_SRC_CNTS_MID),adm_bpsec_ctrl_rst_src_cnts);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_DELETE_KEY_MID),adm_bpsec_ctrl_delete_key);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_ADD_KEY_MID),adm_bpsec_ctrl_add_key);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_ADD_BIB_RULE_MID),adm_bpsec_ctrl_add_bib_rule);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_DEL_BIB_RULE_MID),adm_bpsec_ctrl_del_bib_rule);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_LIST_BIB_RULES_MID),adm_bpsec_ctrl_list_bib_rules);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_ADD_BCB_RULE_MID),adm_bpsec_ctrl_add_bcb_rule);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_DEL_BCB_RULE_MID),adm_bpsec_ctrl_del_bcb_rule);
	adm_add_ctrl(mid_from_value(ADM_BPSEC_CTRL_LIST_BCB_RULES_MID),adm_bpsec_ctrl_list_bcb_rules);
}

void adm_bpsec_init_constants()
{
}

void adm_bpsec_init_macros()
{
}

void adm_bpsec_init_metadata()
{
	/* Step 1: Register Nicknames */
	oid_nn_add_parm(BPSEC_ADM_META_NN_IDX, BPSEC_ADM_META_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_EDD_NN_IDX, BPSEC_ADM_EDD_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_VAR_NN_IDX, BPSEC_ADM_VAR_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_RPT_NN_IDX, BPSEC_ADM_RPT_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_CTRL_NN_IDX, BPSEC_ADM_CTRL_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_CONST_NN_IDX, BPSEC_ADM_CONST_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_MACRO_NN_IDX, BPSEC_ADM_MACRO_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_OP_NN_IDX, BPSEC_ADM_OP_NN_STR, "BPSEC", "2017-08-17");
	oid_nn_add_parm(BPSEC_ADM_ROOT_NN_IDX, BPSEC_ADM_ROOT_NN_STR, "BPSEC", "2017-08-17");

	/* Step 2: Register Metadata Information. */
	adm_add_edd(mid_from_value(ADM_BPSEC_META_NAME_MID), AMP_TYPE_STR, 0, adm_bpsec_meta_name, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_BPSEC_META_NAMESPACE_MID), AMP_TYPE_STR, 0, adm_bpsec_meta_namespace, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_BPSEC_META_VERSION_MID), AMP_TYPE_STR, 0, adm_bpsec_meta_version, adm_print_string, adm_size_string);
	adm_add_edd(mid_from_value(ADM_BPSEC_META_ORGANIZATION_MID), AMP_TYPE_STR, 0, adm_bpsec_meta_organization, adm_print_string, adm_size_string);
}

void adm_bpsec_init_ops()
{
}

void adm_bpsec_init_reports()
{
	Lyst rpt = NULL;
	uint32_t used= 0;
	rpt = lyst_create();
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLK_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLK_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_KNOWN_KEYS_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_KEY_NAMES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_CIPHERSUITE_NAMES_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_RULE_SOURCE_MID));

	adm_add_rpt(mid_from_value(ADM_BPSEC_RPT_FULL_REPORT_MID), rpt);

	midcol_destroy(&rpt);

	rpt = lyst_create();
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_LAST_UPDATE_SRC_MID));
	lyst_insert_last(rpt,mid_from_value(ADM_BPSEC_EDD_LAST_RESET_MID));

	adm_add_rpt(mid_from_value(ADM_BPSEC_RPT_SOURCE_REPORT_MID), rpt);

	midcol_destroy(&rpt);


}

#endif // _HAVE_BPSEC_ADM_
