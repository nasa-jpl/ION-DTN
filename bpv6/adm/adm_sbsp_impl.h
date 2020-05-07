/****************************************************************************
 **
 ** File Name: adm_sbsp_impl.h
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
 **  2020-04-13  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_SBSP_IMPL_H_
#define ADM_SBSP_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#include "adm_sbsp.h"
//#include "shared/adm/adm_bp.h"
#include "shared/primitives/expr.h"

#include "sbsp_instr.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"

/*   START typeENUM */

typedef enum
{
	TOTAL_BLK,
	TOTAL_BYTES,
	SRC_BLK,
	SRC_BYTES
} query_type_e;

typedef enum
{
    BIB_RULE,
	BCB_RULE
} sbsp_rule_type_e;

/*   STOP typeENUM  */

void name_adm_init_agent();



/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_sbsp_setup();
void dtn_sbsp_cleanup();


/* Metadata Functions */
tnv_t *dtn_sbsp_meta_name(tnvc_t *parms);
tnv_t *dtn_sbsp_meta_namespace(tnvc_t *parms);
tnv_t *dtn_sbsp_meta_version(tnvc_t *parms);
tnv_t *dtn_sbsp_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_sbsp_get_num_good_tx_bcb_blk(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bcb_blk(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bcb_blk(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bcb_blk(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_missing_rx_bcb_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bcb_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bcb_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bcb_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bcb_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bcb_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bcb_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_missing_rx_bcb_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bcb_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bib_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bib_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bib_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bib_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_miss_rx_bib_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bib_blks(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bib_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bib_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bib_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bib_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_miss_rx_bib_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bib_bytes(tnvc_t *parms);
tnv_t *dtn_sbsp_get_last_update(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_known_keys(tnvc_t *parms);
tnv_t *dtn_sbsp_get_key_names(tnvc_t *parms);
tnv_t *dtn_sbsp_get_ciphersuite_names(tnvc_t *parms);
tnv_t *dtn_sbsp_get_rule_source(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bcb_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bcb_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bcb_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bcb_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_missing_rx_bcb_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bcb_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bcb_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bcb_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bcb_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bcb_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_missing_rx_bcb_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bcb_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bib_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bib_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bib_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bib_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_miss_rx_bib_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bib_blks_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_tx_bib_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_tx_bib_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_good_rx_bib_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_bad_rx_bib_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_missing_rx_bib_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_num_fwd_bib_bytes_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_last_update_src(tnvc_t *parms);
tnv_t *dtn_sbsp_get_last_reset(tnvc_t *parms);


/* Control Functions */
tnv_t *dtn_sbsp_ctrl_rst_all_cnts(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_rst_src_cnts(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_delete_key(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_add_key(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_add_bib_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_del_bib_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_add_bcb_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status);
tnv_t *dtn_sbsp_ctrl_del_bcb_rule(eid_t *def_mgr, tnvc_t *parms, int8_t *status);


/* OP Functions */


/* Table Build Functions */
tbl_t *dtn_sbsp_tblt_bib_rules(ari_t *id);
tbl_t *dtn_sbsp_tblt_bcb_rules(ari_t *id);

#endif //ADM_SBSP_IMPL_H_
