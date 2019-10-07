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

#include "nm_mgr.h"
#include "metadata.h"


int   ui_print_agents_cb_fn(int idx, int keypress, void* data, char* status_msg);
int   ui_print_agents();
void  ui_print_report(rpt_t *rpt);
void  ui_print_report_entry(char *name, tnv_t *val);
void  ui_print_report_set(agent_t* agent);

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
