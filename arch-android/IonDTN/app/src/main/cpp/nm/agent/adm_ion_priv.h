/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
#ifdef _HAVE_ION_ADM_
/*****************************************************************************
 **
 ** File Name: adm_ion_priv.h
 **
 ** Description: This implements the private aspects of an ION ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/16/13  E. Birrane     Initial Implementation
 *****************************************************************************/
#ifndef ADM_ION_PRIV_H_
#define ADM_ION_PRIV_H_

#include "lyst.h"
#include "bpnm.h"
#include "icinm.h"

#include "../shared/adm/adm_ion.h"
#include "../shared/utils/expr.h"

void agent_adm_init_ion();


/* Retrieval Functions */

/* ION ICI */
expr_result_t ion_ici_get_sdr_state_all(Lyst params);
expr_result_t ion_ici_get_small_pool_size(Lyst params);
expr_result_t ion_ici_get_small_pool_free(Lyst params);
expr_result_t ion_ici_get_small_pool_alloc(Lyst params);
expr_result_t ion_ici_get_large_pool_size(Lyst params);
expr_result_t ion_ici_get_large_pool_free(Lyst params);
expr_result_t ion_ici_get_large_pool_alloc(Lyst params);
expr_result_t ion_ici_get_unused_size(Lyst params);


/* ION INDUCT */
expr_result_t ion_induct_get_all(Lyst params);
expr_result_t ion_induct_get_name(Lyst params);
expr_result_t ion_induct_get_last_reset(Lyst params);
expr_result_t ion_induct_get_rx_bndl(Lyst params);
expr_result_t ion_induct_get_rx_byte(Lyst params);
expr_result_t ion_induct_get_mal_bndl(Lyst params);
expr_result_t ion_induct_get_mal_byte(Lyst params);
expr_result_t ion_induct_get_inauth_bndl(Lyst params);
expr_result_t ion_induct_get_inauth_byte(Lyst params);
expr_result_t ion_induct_get_over_bndl(Lyst params);
expr_result_t ion_induct_get_over_byte(Lyst params);


/* ION OUTDUCT */
expr_result_t ion_outduct_get_all(Lyst params);
expr_result_t ion_outduct_get_name(Lyst params);
expr_result_t ion_outduct_get_cur_q_bdnl(Lyst params);
expr_result_t ion_outduct_get_cur_q_byte(Lyst params);
expr_result_t ion_outduct_get_last_reset(Lyst params);
expr_result_t ion_outduct_get_enq_bndl(Lyst params);
expr_result_t ion_outduct_get_enq_byte(Lyst params);
expr_result_t ion_outduct_get_deq_bndl(Lyst params);
expr_result_t ion_outduct_get_deq_byte(Lyst params);

/* ION NODE */
expr_result_t ion_node_get_all(Lyst params);
expr_result_t ion_node_get_inducts(Lyst params);
expr_result_t ion_node_get_outducts(Lyst params);

/* ION Controls */
uint32_t ion_ctrl_induct_reset(Lyst params);
uint32_t ion_ctrl_outduct_reset(Lyst params);

#endif //ADM_ION_PRIV_H_

#endif // _HAVE_ION_ADM_
