/*****************************************************************************
 **
 ** File Name: adm_bpsec_impl.h
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

#ifndef ADM_BPSEC_IMPL_H_
#define ADM_BPSEC_IMPL_H_

#include "../shared/adm/adm_bpsec.h"
#include "../shared/adm/adm_bp.h"
#include "../shared/adm/adm_ion.h"
#include "../shared/adm/adm_ltp.h"
#include "../shared/primitives/expr.h"

#include "bpsec_instr.h"


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


void bpsec_adm_init_agent();


/******************************************************************************
 *                            Retrieval Functions                             *
 ******************************************************************************/


/* Retrieval Functions */

/* BPSEC */

/* Metadata Functions */
value_t adm_bpsec_md_name(tdc_t params);
value_t adm_bpsec_md_ver(tdc_t params);


/* Collect Functions */

value_t adm_bpsec_get_tot_good_tx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_tot_fail_tx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_tot_good_rx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_tot_fail_rx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_tot_missing_bcb_blks(tdc_t params);
value_t adm_bpsec_get_tot_forward_bcb_blks(tdc_t params);
value_t adm_bpsec_get_tot_good_tx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_tot_fail_tx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_tot_good_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_tot_fail_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_tot_missing_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_tot_forward_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_tot_good_tx_bib_blks(tdc_t params);
value_t adm_bpsec_get_tot_fail_tx_bib_blks(tdc_t params);
value_t adm_bpsec_get_tot_good_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_tot_fail_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_tot_missing_bib_blks(tdc_t params);
value_t adm_bpsec_get_tot_forward_bib_blks(tdc_t params);
value_t adm_bpsec_get_tot_good_tx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_tot_fail_tx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_tot_good_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_tot_fail_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_tot_missing_bib_bytes(tdc_t params);
value_t adm_bpsec_get_tot_forward_bib_bytes(tdc_t params);
value_t adm_bpsec_get_last_update(tdc_t params);
value_t adm_bpsec_get_num_keys(tdc_t params);
value_t adm_bpsec_get_keys(tdc_t params);
value_t adm_bpsec_get_ciphs(tdc_t params);
value_t adm_bpsec_get_srcs(tdc_t params);
value_t adm_bpsec_get_src_good_tx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_src_fail_tx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_src_good_rx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_src_fail_rx_bcb_blks(tdc_t params);
value_t adm_bpsec_get_src_missing_bcb_blks(tdc_t params);
value_t adm_bpsec_get_src_forward_bcb_blks(tdc_t params);
value_t adm_bpsec_get_src_good_tx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_src_fail_tx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_src_good_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_src_fail_rx_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_src_missing_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_src_forward_bcb_bytes(tdc_t params);
value_t adm_bpsec_get_src_good_tx_bib_blks(tdc_t params);
value_t adm_bpsec_get_src_fail_tx_bib_blks(tdc_t params);
value_t adm_bpsec_get_src_good_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_src_fail_rx_bib_blks(tdc_t params);
value_t adm_bpsec_get_src_missing_bib_blks(tdc_t params);
value_t adm_bpsec_get_src_forward_bib_blks(tdc_t params);
value_t adm_bpsec_get_src_good_tx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_src_fail_tx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_src_good_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_src_fail_rx_bib_bytes(tdc_t params);
value_t adm_bpsec_get_src_missing_bib_bytes(tdc_t params);
value_t adm_bpsec_get_src_forward_bib_bytes(tdc_t params);
value_t adm_bpsec_get_src_last_update(tdc_t params);
value_t adm_bpsec_get_last_reset(tdc_t params);


/* Control Functions */


tdc_t* adm_bpsec_ctl_reset_all(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_reset_src(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_del_key(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_add_key(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_add_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_del_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_list_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_add_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_del_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_list_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status);

tdc_t* adm_bpsec_ctl_update_bibrule(eid_t *def_mgr, tdc_t params, int8_t *status);
tdc_t* adm_bpsec_ctl_update_bcbrule(eid_t *def_mgr, tdc_t params, int8_t *status);

/* OP Functions. */


#endif // ADM_BPSEC_IMPL_H_



