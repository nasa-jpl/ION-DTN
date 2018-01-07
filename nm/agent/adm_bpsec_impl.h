/****************************************************************************
 **
 ** File Name: adm_bpsec_impl.h
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
 **  2018-01-06  AUTO             Auto-generated header file 
 **
 ****************************************************************************/

#ifndef ADM_BPSEC_IMPL_H_
#define ADM_BPSEC_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
#include "../shared/adm/adm_bpsec.h"
#include "../shared/adm/adm_bp.h"
#include "../shared/primitives/expr.h"

#include "bpsec_instr.h"
/*   STOP CUSTOM INCLUDES HERE  */


#include "../shared/primitives/tdc.h"
#include "../shared/primitives/value.h"
#include "../shared/utils/utils.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/table.h"
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
} bpsec_rule_type_e;

/*   STOP typeENUM  */

void name_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_bpsec_setup();
void adm_bpsec_cleanup();

/* Metadata Functions */
value_t adm_bpsec_meta_name(tdc_t params);
value_t adm_bpsec_meta_namespace(tdc_t params);

value_t adm_bpsec_meta_version(tdc_t params);

value_t adm_bpsec_meta_organization(tdc_t params);


/* Collect Functions */
value_t adm_bpsec_get_num_good_tx_bcb_blk(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bcb_blk(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bcb_blk(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bcb_blk(tdc_t params);
value_t adm_bpsec_get_num_missing_rx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_num_fwd_bcb_blks(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_num_missing_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_num_fwd_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bib_blks(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bib_blks(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_num_miss_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_num_fwd_bib_blks(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_num_miss_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_num_fwd_bib_bytes(tdc_t params);
value_t adm_bpsec_get_last_update(tdc_t params);
value_t adm_bpsec_get_num_known_keys(tdc_t params);
value_t adm_bpsec_get_key_names(tdc_t params);
value_t adm_bpsec_get_ciphersuite_names(tdc_t params);
value_t adm_bpsec_get_rule_source(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bcb_blks_src(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bcb_blks_src(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bcb_blks_src(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bcb_blks_src(tdc_t params);
value_t adm_bpsec_get_num_missing_rx_bcb_blks_src(tdc_t params);
value_t adm_bpsec_get_num_fwd_bcb_blks_src(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bcb_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bcb_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bcb_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bcb_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_missing_rx_bcb_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_fwd_bcb_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bib_blks_src(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bib_blks_src(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bib_blks_src(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bib_blks_src(tdc_t params);
value_t adm_bpsec_get_num_miss_rx_bib_blks_src(tdc_t params);
value_t adm_bpsec_get_num_fwd_bib_blks_src(tdc_t params);
value_t adm_bpsec_get_num_good_tx_bib_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_bad_tx_bib_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_good_rx_bib_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_bad_rx_bib_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_missing_rx_bib_bytes_src(tdc_t params);
value_t adm_bpsec_get_num_fwd_bib_bytes_src(tdc_t params);
value_t adm_bpsec_get_last_update_src(tdc_t params);
value_t adm_bpsec_get_last_reset(tdc_t params);


/* Control Functions */
tdc_t* adm_bpsec_ctrl_rst_all_cnts(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_rst_src_cnts(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_delete_key(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_add_key(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_add_bib_rule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_del_bib_rule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_list_bib_rules(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_add_bcb_rule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_del_bcb_rule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctrl_list_bcb_rules(eid_t *def_mgr, tdc_t params, int8_t *status);


/* OP Functions */

#endif //ADM_BPSEC_IMPL_H_
