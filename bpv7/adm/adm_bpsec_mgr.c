/****************************************************************************
 **
 ** File Name: adm_bpsec_mgr.c
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
#include "metadata.h"
#include "nm_mgr_ui.h"


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

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_bpsec_idx[ADM_META_IDX], DTN_BPSEC_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_bpsec_init_cnst()
{

}

void dtn_bpsec_init_edd()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLK);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bcb_blk", "Total successfully Tx Bundle Confidentiality blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLK);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bcb_blk", "Total unsuccessfully Tx Block Confidentiality Block (BCB) blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLK);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bcb_blk", "Total successfully Rx BCB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLK);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bcb_blk", "Total unsuccessfully Rx BCB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_missing_rx_bcb_blks", "Total missing-on-RX BCB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bcb_blks", "Total forward BCB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bcb_bytes", "Total successfully Tx BCB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bcb_bytes", "Total unsuccessfully Tx BCB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bcb_blks", "Total unsuccessfully Tx BCB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bcb_bytes", "Total successfully Rx BCB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bcb_bytes", "Total unsuccessfully Rx BCB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_missing_rx_bcb_bytes", "Total missing-on-Rx BCB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bcb_bytes", "Total forwarded BCB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bib_blks", "Total successfully Tx Block Integrity Block (BIB) blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bib_blks", "Total unsuccessfully Tx BIB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bib_blks", "Total successfully Rx BIB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bib_blks", "Total unsuccessfully Rx BIB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_miss_rx_bib_blks", "Total missing-on-Rx BIB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bib_blks", "Total forwarded BIB blocks");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bib_bytes", "Total successfully Tx BIB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bib_bytes", "Total unsuccessfully Tx BIB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bib_bytes", "Total successfully Rx BIB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bib_bytes", "Total unsuccessfully Rx BIB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_miss_rx_bib_bytes", "Total missing-on-Rx BIB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bib_bytes", "Total forwarded BIB bytes");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_UPDATE);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_TV, id, ADM_ENUM_DTN_BPSEC, "last_update", "Last BPSEC update");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_KNOWN_KEYS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_known_keys", "Number of known keys");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_KEY_NAMES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "key_names", "Known key names");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_CIPHERSUITE_NAMES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "ciphersuite_names", "Known ciphersuite names");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_RULE_SOURCE);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_STR, id, ADM_ENUM_DTN_BPSEC, "rule_source", "Known rule sources");

	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bcb_blks_src", "Number of successfully Tx BCB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bcb_blks_src", "Number of failed TX BCB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bcb_blks_src", "Number of successfully Rx BCB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bcb_blks_src", "Number of failed RX BCB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_missing_rx_bcb_blks_src", "Number of missing-onRX BCB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bcb_blks_src", "Number of forwarded BCB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BCB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bcb_bytes_src", "Number of successfully Tx bcb bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BCB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bcb_bytes_src", "Number of failed Tx bcb bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BCB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bcb_bytes_src", "Number of successfully Rx bcb bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BCB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bcb_bytes_src", "Number of failed Rx bcb bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BCB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_missing_rx_bcb_bytes_src", "Number of missing-on-Rx bcb bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BCB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bcb_bytes_src", "Number of forwarded bcb bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bib_blks_src", "Number of successfully Tx BIB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bib_blks_src", "Number of failed Tx BIB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bib_blks_src", "Number of successfully Rx BIB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bib_blks_src", "Number of failed Rx BIB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISS_RX_BIB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_miss_rx_bib_blks_src", "Number of missing-on-Rx BIB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BLKS_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bib_blks_src", "Number of forwarded BIB blocks from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_TX_BIB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_tx_bib_bytes_src", "Number of successfully Tx BIB bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_TX_BIB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_tx_bib_bytes_src", "Number of failed Tx BIB bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_GOOD_RX_BIB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_good_rx_bib_bytes_src", "Number of successfully Rx BIB bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_BAD_RX_BIB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_bad_rx_bib_bytes_src", "Number of failed Rx BIB bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_MISSING_RX_BIB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_missing_rx_bib_bytes_src", "Number of missing-on-Rx BIB bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_NUM_FWD_BIB_BYTES_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "num_fwd_bib_bytes_src", "Number of forwarded BIB bytes from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_UPDATE_SRC);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_TV, id, ADM_ENUM_DTN_BPSEC, "last_update_src", "Last BPSEC update from SRC");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
	id = adm_build_ari(AMP_TYPE_EDD, 1, g_dtn_bpsec_idx[ADM_EDD_IDX], DTN_BPSEC_EDD_LAST_RESET);
	adm_add_edd(id, NULL);
	meta = meta_add_edd(AMP_TYPE_TV, id, ADM_ENUM_DTN_BPSEC, "last_reset", "Last reset");

	meta_add_parm(meta, "Src", AMP_TYPE_STR);
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
	meta_add_var(AMP_TYPE_UINT, id, ADM_ENUM_DTN_BPSEC, "total_bad_tx_blks", "This is the number of failed TX blocks (# failed BIB + # failed bcb).");

}

void dtn_bpsec_init_ctrl()
{

	ari_t *id = NULL;

	metadata_t *meta = NULL;


	/* RST_ALL_CNTS */

	id = adm_build_ari(AMP_TYPE_CTRL, 0, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_RST_ALL_CNTS);
	adm_add_ctrldef_ari(id, 0, NULL);
	meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "rst_all_cnts", "This control causes the Agent to reset all counts associated with block or byte statistics and to set the Last Reset Time of the BPsec EDD data to the time when the control was run.");


	/* RST_SRC_CNTS */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_RST_SRC_CNTS);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "rst_src_cnts", "This control causes the Agent to reset all counts (blocks and bytes) associated with a given bundle source and set the Last Reset Time of the source statistics to the time when the control was run.");

	meta_add_parm(meta, "src", AMP_TYPE_STR);

	/* DELETE_KEY */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_DELETE_KEY);
	adm_add_ctrldef_ari(id, 1, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "delete_key", "This control deletes a key from the BPsec system.");

	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* ADD_KEY */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_ADD_KEY);
	adm_add_ctrldef_ari(id, 2, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "add_key", "This control adds a key to the BPsec system.");

	meta_add_parm(meta, "key_name", AMP_TYPE_STR);
	meta_add_parm(meta, "keyData", AMP_TYPE_BYTESTR);

	/* ADD_BIB_RULE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_ADD_BIB_RULE);
	adm_add_ctrldef_ari(id, 5, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "add_bib_rule", "This control configures policy on the BPsec protocol implementation that describes how BIB blocks should be applied to bundles in the system. This policy is captured as a rule which states when transmitting a bundle from the given source endpoint ID to the given destination endpoint ID, blocks of type target should have a BIB added to them using the given ciphersuite and the given key.");

	meta_add_parm(meta, "source", AMP_TYPE_STR);
	meta_add_parm(meta, "destination", AMP_TYPE_STR);
	meta_add_parm(meta, "target", AMP_TYPE_INT);
	meta_add_parm(meta, "ciphersuiteId", AMP_TYPE_STR);
	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* DEL_BIB_RULE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_DEL_BIB_RULE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "del_bib_rule", "This control removes any configured policy on the BPsec protocol implementation that describes how BIB blocks should be applied to bundles in the system. A BIB policy is uniquely identified by a source endpoint Id, a destination Id, and a target block type.");

	meta_add_parm(meta, "source", AMP_TYPE_STR);
	meta_add_parm(meta, "destination", AMP_TYPE_STR);
	meta_add_parm(meta, "target", AMP_TYPE_INT);

	/* ADD_BCB_RULE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_ADD_BCB_RULE);
	adm_add_ctrldef_ari(id, 5, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "add_bcb_rule", "This control configures policy on the BPsec protocol implementation that describes how BCB blocks should be applied to bundles in the system. This policy is captured as a rule which states when transmitting a bundle from the given source endpoint id to the given destination endpoint id, blocks of type target should have a bcb added to them using the given ciphersuite and the given key.");

	meta_add_parm(meta, "source", AMP_TYPE_STR);
	meta_add_parm(meta, "destination", AMP_TYPE_STR);
	meta_add_parm(meta, "target", AMP_TYPE_INT);
	meta_add_parm(meta, "ciphersuiteId", AMP_TYPE_STR);
	meta_add_parm(meta, "key_name", AMP_TYPE_STR);

	/* DEL_BCB_RULE */

	id = adm_build_ari(AMP_TYPE_CTRL, 1, g_dtn_bpsec_idx[ADM_CTRL_IDX], DTN_BPSEC_CTRL_DEL_BCB_RULE);
	adm_add_ctrldef_ari(id, 3, NULL);
	meta = meta_add_ctrl(id, ADM_ENUM_DTN_BPSEC, "del_bcb_rule", "This control removes any configured policy on the BPsec protocol implementation that describes how BCB blocks should be applied to bundles in the system. A bcb policy is uniquely identified by a source endpoint id, a destination endpoint id, and a target block type.");

	meta_add_parm(meta, "source", AMP_TYPE_STR);
	meta_add_parm(meta, "destination", AMP_TYPE_STR);
	meta_add_parm(meta, "target", AMP_TYPE_INT);
}

void dtn_bpsec_init_mac()
{

}

void dtn_bpsec_init_rpttpl()
{

	metadata_t *meta = NULL;

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
	meta_add_rpttpl(def->id, ADM_ENUM_DTN_BPSEC, "full_report", "all known meta-data, externally defined data, and variables");
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
	meta = meta_add_rpttpl(def->id, ADM_ENUM_DTN_BPSEC, "source_report", "security info by source");
	meta_add_parm(meta, "Source", AMP_TYPE_STR);
}

void dtn_bpsec_init_tblt()
{

	tblt_t *def = NULL;

	/* KEYS */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_KEYS), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "key_name");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_BPSEC, "keys", "This table lists all keys in the security policy database.");

	/* CIPHERSUITES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_CIPHERSUITES), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "csname");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_BPSEC, "ciphersuites", "This table lists supported ciphersuites.");

	/* BIB_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_BIB_RULES), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "SrcEid");
	tblt_add_col(def, AMP_TYPE_STR, "DestEid");
	tblt_add_col(def, AMP_TYPE_UINT, "TgtBlk");
	tblt_add_col(def, AMP_TYPE_STR, "csName");
	tblt_add_col(def, AMP_TYPE_STR, "keyName");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_BPSEC, "bib_rules", "BIB Rules.");

	/* BCB_RULES */

	def = tblt_create(adm_build_ari(AMP_TYPE_TBLT, 0, g_dtn_bpsec_idx[ADM_TBLT_IDX], DTN_BPSEC_TBLT_BCB_RULES), NULL);
	tblt_add_col(def, AMP_TYPE_STR, "SrcEid");
	tblt_add_col(def, AMP_TYPE_STR, "DestEid");
	tblt_add_col(def, AMP_TYPE_UINT, "TgtBlk");
	tblt_add_col(def, AMP_TYPE_STR, "csName");
	tblt_add_col(def, AMP_TYPE_STR, "keyName");
	adm_add_tblt(def);
	meta_add_tblt(def->id, ADM_ENUM_DTN_BPSEC, "bcb_rules", "BCB Rules.");
}

#endif // _HAVE_DTN_BPSEC_ADM_
