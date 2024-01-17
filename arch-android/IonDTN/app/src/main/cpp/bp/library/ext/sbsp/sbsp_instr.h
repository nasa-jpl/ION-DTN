
/*****************************************************************************
 **
 ** File Name: sbsp_instr.h
 **
 ** Description: Definitions supporting the collection of instrumentation
 **              for the SBSP implementation.
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

#ifndef SBSP_INSTR_H_
#define SBSP_INSTR_H_

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
} sbsp_src_instr_t;

typedef struct
{
	sbsp_src_instr_t anon;
	uvast num_keys;
	uvast num_cipher;
	time_t last_reset;
} sbsp_instr_misc_t;


typedef struct
{
   Object  src;    /* SDR list: sbsp_src_instr_t  */
   Object  misc;   /* sbsp_instr_misc_t           */
} SbspInstrDB;

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
} sbsp_instr_type_e;

#define SBSP_INSTR_SDR_NAME "sbsp_instr"

#define ADD_BCB_TX_PASS(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BCB_TX_PASS);
#define ADD_BCB_TX_FAIL(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BCB_TX_FAIL);
#define ADD_BCB_RX_PASS(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BCB_RX_PASS);
#define ADD_BCB_RX_FAIL(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BCB_RX_FAIL);
#define ADD_BCB_RX_MISS(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BCB_RX_MISS);
#define ADD_BCB_FWD(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BCB_FWD);

#define ADD_BIB_TX_PASS(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BIB_TX_PASS);
#define ADD_BIB_TX_FAIL(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BIB_TX_FAIL);
#define ADD_BIB_RX_PASS(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BIB_RX_PASS);
#define ADD_BIB_RX_FAIL(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BIB_RX_FAIL);
#define ADD_BIB_RX_MISS(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BIB_RX_MISS);
#define ADD_BIB_FWD(src, blk, bytes) sbsp_instr_update(src, blk, bytes, BIB_FWD);


void     sbsp_instr_update(char *src, uvast blk, uvast bytes, sbsp_instr_type_e type);
void     sbsp_instr_cleanup();
void     sbsp_instr_clear_src(Object sdrElt);
int   sbsp_instr_get_misc(sbsp_instr_misc_t *result);
int   sbsp_instr_clear();
int   sbsp_instr_get_src_blk(char *src_id, sbsp_instr_type_e type, uvast *result);
int   sbsp_instr_get_src_bytes(char *src_id, sbsp_instr_type_e type, uvast *result);
int   sbsp_instr_get_src_update(char *src_id, time_t *result);
int   sbsp_instr_get_total_blk(sbsp_instr_type_e type, uvast *result);
int   sbsp_instr_get_total_bytes(sbsp_instr_type_e type, uvast *result);
int   sbsp_instr_get_tot_update(time_t *result);
uint32_t sbsp_instr_get_num_keys();
char*    sbsp_instr_get_keynames();
char*    sbsp_instr_get_csnames();
char*    sbsp_instr_get_srcnames();
int      sbsp_instr_init();
void     sbsp_instr_reset();
void     sbsp_instr_reset_src(char *src_id);

#endif
