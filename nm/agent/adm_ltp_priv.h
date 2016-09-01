/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

#ifdef _HAVE_LTP_ADM_

/*****************************************************************************
 **
 ** File Name: adm_ltp_priv.h
 **
 ** Description: This implements the private aspects of a LTP ADM.
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
#ifndef ADM_LTP_PRIV_H_
#define ADM_LTP_PRIV_H_

#include "lyst.h"
#include "ltpnm.h"

#include "../shared/adm/adm_ltp.h"
#include "../shared/utils/expr.h"

void agent_adm_init_ltp();


/* Get Functions */
expr_result_t ltp_get_node_resources_all(Lyst params);
expr_result_t ltp_get_heap_bytes_reserved(Lyst params);
expr_result_t ltp_get_heap_bytes_used(Lyst params);
expr_result_t ltp_get_engines(Lyst params);

expr_result_t ltp_get_eng_all(Lyst params);

expr_result_t ltp_get_eng_num(Lyst params);
expr_result_t ltp_get_eng_exp_sess(Lyst params);
expr_result_t ltp_get_eng_cur_out_seg(Lyst params);
expr_result_t ltp_get_eng_cur_imp_sess(Lyst params);
expr_result_t ltp_get_eng_cur_in_seg(Lyst params);
expr_result_t ltp_get_eng_last_reset_time(Lyst params);

expr_result_t ltp_get_eng_out_seg_q_cnt(Lyst params);
expr_result_t ltp_get_eng_out_seg_q_byte(Lyst params);
expr_result_t ltp_get_eng_out_seg_pop_cnt(Lyst params);
expr_result_t ltp_get_eng_out_seg_pop_byte(Lyst params);


expr_result_t ltp_get_eng_out_ckp_xmit_cnt(Lyst params);
expr_result_t ltp_get_eng_out_pos_ack_rcv_cnt(Lyst params);
expr_result_t ltp_get_eng_out_neg_ack_rcv_cnt(Lyst params);
expr_result_t ltp_get_eng_out_canc_rcv_cnt(Lyst params);
expr_result_t ltp_get_eng_out_ckp_rexmt_cnt(Lyst params);
expr_result_t ltp_get_eng_out_canc_xmit_cnt(Lyst params);
expr_result_t ltp_get_eng_out_compl_cnt(Lyst params);

expr_result_t ltp_get_eng_in_seg_rcv_red_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_rcv_red_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_rcv_grn_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_rcv_grn_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_rdndt_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_rdndt_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_mal_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_mal_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_unk_snd_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_unk_snd_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_unk_cli_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_unk_cli_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_stray_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_stray_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_miscol_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_miscol_byte(Lyst params);
expr_result_t ltp_get_eng_in_seg_clsd_cnt(Lyst params);
expr_result_t ltp_get_eng_in_seg_clsd_byte(Lyst params);

expr_result_t ltp_get_eng_in_ckp_rcv_cnt(Lyst params);
expr_result_t ltp_get_eng_in_pos_ack_xmit_cnt(Lyst params);
expr_result_t ltp_get_eng_in_neg_ack_xmit_cnt(Lyst params);
expr_result_t ltp_get_eng_in_canc_xmit_cnt(Lyst params);
expr_result_t ltp_get_eng_in_ack_rexmt_cnt(Lyst params);
expr_result_t ltp_get_eng_in_canc_rcv_cnt(Lyst params);
expr_result_t ltp_get_eng_in_compl_cnt(Lyst params);

uint32_t ltp_engine_reset(Lyst params);


#endif //ADM_LTP_PRIV_H_
#endif /* _HAVE_LTP_ADM_ */
