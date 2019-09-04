
/*****************************************************************************
 **
 ** File Name: bpsec_instr.h
 **
 ** Description: Definitions supporting the collection of instrumentation
 **              for the BPsec implementation.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/20/16  E. Birrane     Initial Implementation
 **                           [Secure DTN implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef BPSEC_INSTR_H_
#define BPSEC_INSTR_H_

#include "bpP.h"

typedef struct
{
	char eid[MAX_EID_LEN];
	uvast bcb_blk_tx_pass;
	uvast bcb_blk_tx_fail;
	uvast bcb_blk_rx_pass;
	uvast bcb_blk_rx_fail;
	uvast bcb_blk_rx_miss;
	uvast bcb_blk_fwd;

	uvast bcb_byte_tx_pass;
	uvast bcb_byte_tx_fail;
	uvast bcb_byte_rx_pass;
	uvast bcb_byte_rx_fail;
	uvast bcb_byte_rx_miss;
	uvast bcb_byte_fwd;

	uvast bib_blk_tx_pass;
	uvast bib_blk_tx_fail;
	uvast bib_blk_rx_pass;
	uvast bib_blk_rx_fail;
	uvast bib_blk_rx_miss;
	uvast bib_blk_fwd;

	uvast bib_byte_tx_pass;
	uvast bib_byte_tx_fail;
	uvast bib_byte_rx_pass;
	uvast bib_byte_rx_fail;
	uvast bib_byte_rx_miss;
	uvast bib_byte_fwd;

	time_t last_update;
} bpsec_src_instr_t;

typedef struct
{
	bpsec_src_instr_t anon;
	uvast num_keys;
	uvast num_cipher;
	time_t last_reset;
} bpsec_instr_misc_t;


typedef struct
{
   Object  src;    /* SDR list: bpsec_src_instr_t  */
   Object  misc;   /* bpsec_instr_misc_t           */
} BpsecInstrDB;

typedef enum
{
	BCB_TX_PASS,
	BCB_TX_FAIL,
	BCB_RX_PASS,
	BCB_RX_FAIL,
	BCB_RX_MISS,
	BCB_FWD,
	BIB_TX_PASS,
	BIB_TX_FAIL,
	BIB_RX_PASS,
	BIB_RX_FAIL,
	BIB_RX_MISS,
	BIB_FWD
} bpsec_instr_type_e;

#define BPSEC_INSTR_SDR_NAME "bpsec_instr"

#define ADD_BCB_TX_PASS(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BCB_TX_PASS);
#define ADD_BCB_TX_FAIL(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BCB_TX_FAIL);
#define ADD_BCB_RX_PASS(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BCB_RX_PASS);
#define ADD_BCB_RX_FAIL(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BCB_RX_FAIL);
#define ADD_BCB_RX_MISS(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BCB_RX_MISS);
#define ADD_BCB_FWD(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BCB_FWD);

#define ADD_BIB_TX_PASS(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BIB_TX_PASS);
#define ADD_BIB_TX_FAIL(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BIB_TX_FAIL);
#define ADD_BIB_RX_PASS(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BIB_RX_PASS);
#define ADD_BIB_RX_FAIL(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BIB_RX_FAIL);
#define ADD_BIB_RX_MISS(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BIB_RX_MISS);
#define ADD_BIB_FWD(src, blk, bytes) bpsec_instr_update(src, blk, bytes, BIB_FWD);


void     bpsec_instr_update(char *src, uvast blk, uvast bytes, bpsec_instr_type_e type);
void     bpsec_instr_cleanup();
void     bpsec_instr_clear_src(Object sdrElt);
int   bpsec_instr_get_misc(bpsec_instr_misc_t *result);
int   bpsec_instr_clear();
int   bpsec_instr_get_src_blk(char *src_id, bpsec_instr_type_e type, uvast *result);
int   bpsec_instr_get_src_bytes(char *src_id, bpsec_instr_type_e type, uvast *result);
int   bpsec_instr_get_src_update(char *src_id, time_t *result);
int   bpsec_instr_get_total_blk(bpsec_instr_type_e type, uvast *result);
int   bpsec_instr_get_total_bytes(bpsec_instr_type_e type, uvast *result);
int   bpsec_instr_get_tot_update(time_t *result);
uint32_t bpsec_instr_get_num_keys();
char*    bpsec_instr_get_keynames();
char*    bpsec_instr_get_csnames();
char*    bpsec_instr_get_srcnames();
int      bpsec_instr_init();
void     bpsec_instr_reset();
void     bpsec_instr_reset_src(char *src_id);

#endif
