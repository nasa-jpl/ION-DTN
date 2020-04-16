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
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/


#include "ion.h"
#include "platform.h"
#include "adm_bpsec.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_bpsec_impl.h"
#include "agent/rda.h"


#include "adm_amp_agent.h"

#define _HAVE_DTN_BPSEC_ADM_
#ifdef _HAVE_DTN_BPSEC_ADM_

static vec_idx_t g_dtn_bpsec_idx[11];

void dtn_bpsec_init()
{
	adm_add_adm_info("dtn_bpsec", ADM_ENUM_DTN_BPSEC);

	VDB_ADD_NN(((ADM_ENUM_DTN_BPSEC * 20) + ADM_RPTT_IDX), &(g_dtn_bpsec_idx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BPSEC * 20) + ADM_EDD_IDX), &(g_dtn_bpsec_idx[ADM_EDD_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BPSEC * 20) + ADM_CTRL_IDX), &(g_dtn_bpsec_idx[ADM_CTRL_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BPSEC * 20) + ADM_META_IDX), &(g_dtn_bpsec_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BPSEC * 20) + ADM_TBLT_IDX), &(g_dtn_bpsec_idx[ADM_TBLT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_BPSEC * 20) + ADM_VAR_IDX), &(g_dtn_bpsec_idx[ADM_VAR_IDX]));


	dtn_bpsec_setup();
	dtn_bpsec_init_meta();
	dtn_bpsec_init_cnst();
	dtn_bpsec_init_edd();
	dtn_bpsec_init_op();
	dtn_bpsec_init_var();
	dtn_bpsec_init_ctrl();
	dtn_bpsec_init_mac();
	dtn_bpsec_init_rpttpl();
	dtn_bpsec_init_tblt();
}

void dtn_bpsec_init_meta()
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_NAME), dtn_bpsec_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_NAMESPACE), dtn_bpsec_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_VERSION), dtn_bpsec_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_ORGANIZATION), dtn_bpsec_meta_organization);
}

void dtn_bpsec_init_cnst()
{

}

void dtn_bpsec_init_edd()
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK), dtn_bpsec_get_num_good_tx_bcb_blk);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLK), dtn_bpsec_get_num_bad_tx_bcb_blk);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK), dtn_bpsec_get_num_good_rx_bcb_blk);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLK), dtn_bpsec_get_num_bad_rx_bcb_blk);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS), dtn_bpsec_get_num_missing_rx_bcb_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS), dtn_bpsec_get_num_fwd_bcb_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES), dtn_bpsec_get_num_good_tx_bcb_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES), dtn_bpsec_get_num_bad_tx_bcb_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS), dtn_bpsec_get_num_bad_tx_bcb_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES), dtn_bpsec_get_num_good_rx_bcb_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES), dtn_bpsec_get_num_bad_rx_bcb_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES), dtn_bpsec_get_num_missing_rx_bcb_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES), dtn_bpsec_get_num_fwd_bcb_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS), dtn_bpsec_get_num_good_tx_bib_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS), dtn_bpsec_get_num_bad_tx_bib_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS), dtn_bpsec_get_num_good_rx_bib_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS), dtn_bpsec_get_num_bad_rx_bib_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS), dtn_bpsec_get_num_miss_rx_bib_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS), dtn_bpsec_get_num_fwd_bib_blks);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES), dtn_bpsec_get_num_good_tx_bib_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES), dtn_bpsec_get_num_bad_tx_bib_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES), dtn_bpsec_get_num_good_rx_bib_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES), dtn_bpsec_get_num_bad_rx_bib_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES), dtn_bpsec_get_num_miss_rx_bib_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES), dtn_bpsec_get_num_fwd_bib_bytes);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_UPDATE), dtn_bpsec_get_last_update);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_KNOWN_KEYS), dtn_bpsec_get_num_known_keys);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_KEY_NAMES), dtn_bpsec_get_key_names);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_CIPHERSUITE_NAMES), dtn_bpsec_get_ciphersuite_names);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_RULE_SOURCE), dtn_bpsec_get_rule_source);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC), dtn_bpsec_get_num_good_tx_bcb_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC), dtn_bpsec_get_num_bad_tx_bcb_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC), dtn_bpsec_get_num_good_rx_bcb_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC), dtn_bpsec_get_num_bad_rx_bcb_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC), dtn_bpsec_get_num_missing_rx_bcb_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC), dtn_bpsec_get_num_fwd_bcb_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC), dtn_bpsec_get_num_good_tx_bcb_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC), dtn_bpsec_get_num_bad_tx_bcb_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC), dtn_bpsec_get_num_good_rx_bcb_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC), dtn_bpsec_get_num_bad_rx_bcb_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC), dtn_bpsec_get_num_missing_rx_bcb_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC), dtn_bpsec_get_num_fwd_bcb_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC), dtn_bpsec_get_num_good_tx_bib_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC), dtn_bpsec_get_num_bad_tx_bib_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC), dtn_bpsec_get_num_good_rx_bib_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC), dtn_bpsec_get_num_bad_rx_bib_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC), dtn_bpsec_get_num_miss_rx_bib_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC), dtn_bpsec_get_num_fwd_bib_blks_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC), dtn_bpsec_get_num_good_tx_bib_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC), dtn_bpsec_get_num_bad_tx_bib_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC), dtn_bpsec_get_num_good_rx_bib_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC), dtn_bpsec_get_num_bad_rx_bib_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC), dtn_bpsec_get_num_missing_rx_bib_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC), dtn_bpsec_get_num_fwd_bib_bytes_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_UPDATE_SRC), dtn_bpsec_get_last_update_src);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_RESET), dtn_bpsec_get_last_reset);
}

void dtn_bpsec_init_op()
{

}

void dtn_bpsec_init_var()
{

	ari_t *id = NULL;

	expr_t *expr = NULL;


	/* TOTAL_BAD_TX_BLKS */

	id = adm_build_ari(AMP_TYPE_VAR, 0, g_dtn_bpsec_idx[ADM_VAR_IDX], DTN_BPSEC_VAR_TOTAL_BAD_TX_BLKS);
	expr = expr_create(AMP_TYPE_UINT);
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS));
	expr_add_item(expr, adm_build_ari(AMP_TYPE_OPER, 1, g_amp_agent_idx[ADM_OPER_IDX], AMP_AGENT_OP_PLUSUINT));
	adm_add_var_from_expr(id, AMP_TYPE_UINT, expr);
}

void dtn_bpsec_init_ctrl()
{

	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_RST_ALL_CNTS, 0, dtn_bpsec_ctrl_rst_all_cnts);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_RST_SRC_CNTS, 1, dtn_bpsec_ctrl_rst_src_cnts);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_DELETE_KEY, 1, dtn_bpsec_ctrl_delete_key);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_ADD_KEY, 2, dtn_bpsec_ctrl_add_key);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_ADD_BIB_RULE, 5, dtn_bpsec_ctrl_add_bib_rule);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_DEL_BIB_RULE, 3, dtn_bpsec_ctrl_del_bib_rule);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_ADD_BCB_RULE, 5, dtn_bpsec_ctrl_add_bcb_rule);
	adm_add_ctrldef(g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_DEL_BCB_RULE, 3, dtn_bpsec_ctrl_del_bcb_rule);
}

void dtn_bpsec_init_mac()
{

}

void dtn_bpsec_init_rpttpl()
{

	rpttpl_t *def = NULL;

	/* FULL_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 0, g_dtn_bpsec_idx[ADM_RPTT_IDX], DTN_BPSEC_RPTTPL_FULL_REPORT));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLK));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLK));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_UPDATE));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_KNOWN_KEYS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_KEY_NAMES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_CIPHERSUITE_NAMES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_RULE_SOURCE));
	adm_add_rpttpl(def);
	/* SOURCE_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 1, g_dtn_bpsec_idx[ADM_RPTT_IDX], DTN_BPSEC_RPTTPL_SOURCE_REPORT));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_UPDATE_SRC, tnv_from_map(AMP_TYPE_STR, 0)));
	rpttpl_add_item(def, ADM_BUILD_ARI_PARM_1(AMP_TYPE_EDD, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_RESET, tnv_from_map(AMP_TYPE_STR, 0)));
	adm_add_rpttpl(def);
}

void dtn_bpsec_init_tblt()
{

	tblt_t *def = NULL;

	/* KEYS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_KEYS), dtn_bpsec_tblt_keys);
	tblt_add_col(def, AMP_TYPE_STR, "key_name");
	adm_add_tblt(def);

	/* CIPHERSUITES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_CIPHERSUITES), dtn_bpsec_tblt_ciphersuites);
	tblt_add_col(def, AMP_TYPE_STR, "csname");
	adm_add_tblt(def);

	/* BIB_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_BIB_RULES), dtn_bpsec_tblt_bib_rules);
	tblt_add_col(def, AMP_TYPE_STR, "SrcEid");
	tblt_add_col(def, AMP_TYPE_STR, "DestEid");
	tblt_add_col(def, AMP_TYPE_UINT, "TgtBlk");
	tblt_add_col(def, AMP_TYPE_STR, "csName");
	tblt_add_col(def, AMP_TYPE_STR, "keyName");
	adm_add_tblt(def);

	/* BCB_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_BCB_RULES), dtn_bpsec_tblt_bcb_rules);
	tblt_add_col(def, AMP_TYPE_STR, "SrcEid");
	tblt_add_col(def, AMP_TYPE_STR, "DestEid");
	tblt_add_col(def, AMP_TYPE_UINT, "TgtBlk");
	tblt_add_col(def, AMP_TYPE_STR, "csName");
	tblt_add_col(def, AMP_TYPE_STR, "keyName");
	adm_add_tblt(def);
}

#endif // _HAVE_DTN_BPSEC_ADM_
