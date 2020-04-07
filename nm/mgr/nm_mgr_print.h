/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file nm_mgr_print.h
 **
 **
 ** Description: Helper file holding functions for printing DTNMP data to
 **              the screen.  These functions differ from the "to string"
 **              functions for individual DTNMP data types as these functions
 **              "pretty print" directly to stdout and are nly appropriate for
 **              ground-related applications. We do not anticipate these functions
 **              being appropriate for use in embedded systems.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/10/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  11/01/18  E. Birrane     Updated for AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef MGR_PRINT_H_
#define MGR_PRINT_H_

#include "../shared/utils/nm_types.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/rules.h"
#include "../shared/primitives/expr.h"
#include "../shared/adm/adm.h"
#include "nm_mgr_ui.h"
#include "nm_mgr.h"
#include "metadata.h"

#ifdef USE_JSON
#include "cJSON.h"
#endif

// ASCII Color Codes & Macros for formatting stdout
#define RST  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define FRED(x) KRED x RST
#define FGRN(x) KGRN x RST
#define FYEL(x) KYEL x RST
#define FBLU(x) KBLU x RST
#define FMAG(x) KMAG x RST
#define FCYN(x) KCYN x RST
#define FWHT(x) KWHT x RST

#define BOLD(x) "\x1B[1m" x RST
#define UNDL(x) "\x1B[4m" x RST


int   ui_print_agents_cb_fn(int idx, int keypress, void* data, char* status_msg);
int   ui_print_agents();

#define ui_print_report(rpt) ui_fprint_report(NULL, rpt)
void  ui_fprint_report(ui_print_cfg_t *fd, rpt_t *rpt);
void  ui_print_report_set(agent_t* agent);

#ifdef USE_JSON
cJSON* ui_json_report(rpt_t *rpt);
void ui_fprint_json_report(ui_print_cfg_t *fd, rpt_t *rpt);
void ui_fprint_json_table(ui_print_cfg_t *fd, tbl_t *rpt);
#endif

#define ui_print_table(tbl) ui_fprint_table(NULL, tbl)
void  ui_fprint_table(ui_print_cfg_t *fd, tbl_t *rpt);
void  ui_print_table_set(agent_t* agent);


char* ui_str_from_ac(ac_t *ac);
char* ui_str_from_ari(ari_t *id, tnvc_t *ap, int desc);
char* ui_str_from_blob(blob_t *blob);
char* ui_str_from_ctrl(ctrl_t *ctrl);
char* ui_str_from_edd(edd_t *edd);
char* ui_str_from_expr(expr_t *expr);
char* ui_str_from_fp(metadata_t *meta);
char* ui_str_from_mac(macdef_t *mac);
char* ui_str_from_op(op_t *op);
char* ui_str_from_rpt(rpt_t *rpt);
char* ui_str_from_rpttpl(rpttpl_t *rpt);
char* ui_str_from_sbr(rule_t *rule);
char* ui_str_from_tbl(tbl_t *tbl);
char* ui_str_from_tblt(tblt_t *tblt);
char* ui_str_from_tbr(rule_t *tbr);
char* ui_str_from_tnv(tnv_t *tnv);
char* ui_str_from_tnvc(tnvc_t *tnvc);
char* ui_str_from_var(var_t *var);


#endif
