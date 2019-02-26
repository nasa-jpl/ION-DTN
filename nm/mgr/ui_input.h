/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file ui_input.h
 **
 **
 ** Description: Functions to retrieve information from the user via a
 **              text-based interface.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/24/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  10/06/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef _UI_INPUT_H
#define _UI_INPUT_H

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/blob.h"
#include "../shared/primitives/rules.h"

#include "nm_mgr_ui.h"

#define MAX_INPUT_BYTES 1024

#define TYPE_AS_MASK(type) (((uvast)1) << ((uvast)type))
#define TYPE_MATCHES_MASK(type, mask) (TYPE_AS_MASK(type) & mask)

#if (LONG_LONG_OKAY)
#define TYPE_MASK_ALL (0xFFFFFFFFFFFFFFFF)
#else
#define TYPE_MASK_ALL (0xFFFFFFFF)
#endif

int ui_input_get_line(char *prompt, char **line, int max_len);

/*
 * AMM Object Input Functions
 */

uint8_t ui_input_adm_id();

/*
 * User input methods for basic data types.
 */
uint8_t  ui_input_byte(char *prompt);
int32_t  ui_input_int(char *prompt);
float    ui_input_real32(char *prompt);
double   ui_input_real64(char *prompt);
char *   ui_input_string(char *prompt);
uint32_t ui_input_uint(char *prompt);
uvast    ui_input_uvast(char *prompt);
vast     ui_input_vast(char *prompt);

/*
 * User input for compound object types.
 */

ac_t*   ui_input_ac(char *prompt);

ari_t*  ui_input_ari(char *prompt, uint8_t adm_id, uvast mask);
ari_t*  ui_input_ari_build(uvast mask);
int     ui_input_ari_flags(uint8_t *flag);
ari_t*  ui_input_ari_list(uint8_t adm_id, uvast mask);
ari_t*  ui_input_ari_lit(char *prompt);
ari_t*  ui_input_ari_raw(uint8_t no_file);
int     ui_input_ari_type(uvast mask);
int     ui_input_parms(ari_t *id);

tnv_t*  ui_input_tnv(int type, char *prompt);
tnvc_t* ui_input_tnvc(char *prompt);


/* Input for helper types. */
blob_t*  ui_input_blob(char *prompt, uint8_t no_file);
blob_t*  ui_input_file_contents(char *prompt);


ctrl_t* ui_input_ctrl(char* prompt);
expr_t* ui_input_expr(char* prompt);
op_t* ui_input_oper(char* prompt);
rpt_t* ui_input_rpt(char* prompt);
rpttpl_t* ui_input_rpttpl(char* prompt);
rule_t *ui_input_rule(char* prompt);

tbl_t* ui_input_tbl(char* prompt);
tblt_t* ui_input_tblt(char* prompt);

rule_t *ui_input_tbr(char* prompt);

var_t* ui_input_var(char* prompt);

tnvc_t* ui_input_tnvc(char* prompt);

macdef_t *ui_input_mac(char *prompt);

#endif // _UI_INPUT_H
