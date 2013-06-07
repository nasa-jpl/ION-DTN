//
//  adm_bp.h
//
//  Created by Birrane, Edward J. on 10/22/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef ADM_LTP_H_
#define ADM_LTP_H_

#include "lyst.h"
#include "ltpnm.h"



#include "shared/utils/nm_types.h"

#include "shared/adm/adm.h"

/*
 * We will invent an OID space for LTP MIB information, to live at:
 *
 * iso.identified-organization.dod.internet.mgmt.mib-2.ion
 * or 1.2.3.1.2.1.A.1  // /todo EJB A here is not a valid entry. Just using something for now.
 * or, as OID,: 2A 03 01 02 01 0A
 *
 * OMIT SNMP 0x6 type identifier since we know these are always OIDs.
 */

// OID does not contain the 06 header.
// Flag value for atomic types is 0x00


static char *LTP_NODE_RESOURCES_ALL = "00072A030102010A00";
static char *LTP_HEAP_BYTES_RSV     = "00072A030102010A01";
static char *LTP_HEAD_BYTES_USED    = "00072A030102010A02";
static char *LTP_ENGINE_IDS         = "00072A030102010A03";

static char *LTP_ENG_ALL             = "40072A030102010B00";
static char *LTP_ENG_NUM             = "40072A030102010B01";
static char *LTP_ENG_EXP_SESS        = "40072A030102010B02";
static char *LTP_ENG_CUR_OUT_SEG     = "40072A030102010B03";
static char *LTP_ENG_CUR_IMP_SESS    = "40072A030102010B04";
static char *LTP_ENG_CUR_IN_SEG      = "40072A030102010B05";
static char *LTP_ENG_LAST_RESET_TIME = "40072A030102010B06";

static char *LTP_ENG_OUT_SEG_Q_CNT        = "40072A030102010B07";
static char *LTP_ENG_OUT_SEG_Q_BYTE       = "40072A030102010B08";
static char *LTP_ENG_OUT_SEG_POP_CNT      = "40072A030102010B09";
static char *LTP_ENG_OUT_SEG_POP_BYTE     = "40072A030102010B0A";

static char *LTP_ENG_OUT_CKP_XMIT_CNT     = "40072A030102010B0B";
static char *LTP_ENG_OUT_POS_ACK_RCV_CNT  = "40072A030102010B0C";
static char *LTP_ENG_OUT_NEG_ACK_RCV_CNT  = "40072A030102010B0D";
static char *LTP_ENG_OUT_CANC_RCV_CNT     = "40072A030102010B0E";
static char *LTP_ENG_OUT_CKP_REXMT_CNT    = "40072A030102010B0F";
static char *LTP_ENG_OUT_CANC_XMIT_CNT    = "40072A030102010B10";
static char *LTP_ENG_OUT_COMPL_CNT        = "40072A030102010B11";

static char *LTP_ENG_IN_SEG_RCV_RED_CNT   = "40072A030102010B12";
static char *LTP_ENG_IN_SEG_RCV_RED_BYTE  = "40072A030102010B13";
static char *LTP_ENG_IN_SEG_RCV_GRN_CNT   = "40072A030102010B14";
static char *LTP_ENG_IN_SEG_RCV_GRN_BYTE  = "40072A030102010B15";
static char *LTP_ENG_IN_SEG_RDNDT_CNT     = "40072A030102010B16";
static char *LTP_ENG_IN_SEG_RDNDT_BYTE    = "40072A030102010B17";
static char *LTP_ENG_IN_SEG_MAL_CNT       = "40072A030102010B18";
static char *LTP_ENG_IN_SEG_MAL_BYTE      = "40072A030102010B19";
static char *LTP_ENG_IN_SEG_UNK_SND_CNT   = "40072A030102010B1A";
static char *LTP_ENG_IN_SEG_UNK_SND_BYTE  = "40072A030102010B1B";
static char *LTP_ENG_IN_SEG_UNK_CLI_CNT   = "40072A030102010B1C";
static char *LTP_ENG_IN_SEG_UNK_CLI_BYTE  = "40072A030102010B1D";
static char *LTP_ENG_IN_SEG_STRAY_CNT     = "40072A030102010B1E";
static char *LTP_ENG_IN_SEG_STRAY_BYTE    = "40072A030102010B1F";
static char *LTP_ENG_IN_SEG_MISCOL_CNT    = "40072A030102010B20";
static char *LTP_ENG_IN_SEG_MISCOL_BYTE   = "40072A030102010B21";
static char *LTP_ENG_IN_SEG_CLSD_CNT      = "40072A030102010B22";
static char *LTP_ENG_IN_SEG_CLSD_BYTE     = "40072A030102010B23";

static char *LTP_ENG_IN_CKP_RCV_CNT       = "40072A030102010B24";
static char *LTP_ENG_IN_POS_ACK_XMIT_CNT  = "40072A030102010B25";
static char *LTP_ENG_IN_NEG_ACK_XMIT_CNT  = "40072A030102010B26";
static char *LTP_ENG_IN_CANC_XMIT_CNT     = "40072A030102010B27";
static char *LTP_ENG_IN_ACK_REXMT_CNT     = "40072A030102010B28";
static char *LTP_ENG_IN_CANC_RCV_CNT      = "40072A030102010B29";
static char *LTP_ENG_IN_COMPL_CNT         = "40072A030102010B2A";

/* LTP Controls */
static char *LTP_ENG_RESET = "01072A030102010C00";


void initLtpAdm();

/* Print Functions */
char *ltpPrintNodeResourcesAll(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

char *ltp_engine_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

/* Get Functions */
uint8_t *ltpGetNodeResourcesAll(Lyst params, uint64_t *length);
uint8_t *ltpGetHeapBytesReserved(Lyst params, uint64_t *length);
uint8_t *ltpGetHeapBytesUsed(Lyst params, uint64_t *length);
uint8_t *ltp_get_engines(Lyst params, uint64_t *length);

uint8_t *ltp_get_eng_all(Lyst params, uint64_t *length);

uint8_t *ltp_get_eng_num(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_exp_sess(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_cur_out_seg(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_cur_imp_sess(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_cur_in_seg(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_last_reset_time(Lyst params, uint64_t *length);

uint8_t *ltp_get_eng_out_seg_q_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_seg_q_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_seg_pop_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_seg_pop_byte(Lyst params, uint64_t *length);


uint8_t *ltp_get_eng_out_ckp_xmit_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_pos_ack_rcv_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_neg_ack_rcv_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_canc_rcv_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_ckp_rexmt_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_canc_xmit_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_out_compl_cnt(Lyst params, uint64_t *length);

uint8_t *ltp_get_eng_in_seg_rcv_red_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_rcv_red_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_rcv_grn_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_rcv_grn_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_rdndt_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_rdndt_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_mal_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_mal_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_unk_snd_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_unk_snd_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_unk_cli_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_unk_cli_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_stray_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_stray_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_miscol_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_miscol_byte(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_clsd_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_seg_clsd_byte(Lyst params, uint64_t *length);

uint8_t *ltp_get_eng_in_ckp_rcv_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_pos_ack_xmit_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_neg_ack_xmit_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_canc_xmit_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_ack_rexmt_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_canc_rcv_cnt(Lyst params, uint64_t *length);
uint8_t *ltp_get_eng_in_compl_cnt(Lyst params, uint64_t *length);


/* SIZE */
uint32_t ltpSizeNodeResourcesAll(uint8_t* buffer, uint64_t buffer_len);
uint32_t ltpSizeHeapBytesReserved(uint8_t* buffer, uint64_t buffer_len);
uint32_t ltpSizeHeapBytesUsed(uint8_t* buffer, uint64_t buffer_len);

uint32_t ltp_engine_size(uint8_t* buffer, uint64_t buffer_len);

#endif //ADM_LTP_H_
