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
 *****************************************************************************/

#ifndef MGR_PRINT_H_
#define MGR_PRINT_H_

#include "nm_mgr.h"

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/rules.h"
#include "../shared/primitives/expr.h"

int ui_print_agents();
/*
void ui_print_entry(rpt_entry_t *entry, uvast *mid_sizes, uvast *data_sizes);
void ui_print_expr(expr_t *expr);
void ui_print_mc(Lyst mc);
void ui_print_mid(mid_t *mid);


void ui_print_reports(agent_t *agent);

void ui_print_srl(srl_t *srl);
void ui_print_tdc(tdc_t *tdc, def_gen_t *cur_def);
void ui_print_trl(trl_t *trl);
void ui_print_val(uint8_t type, uint8_t *data, uint32_t length);
*/

#endif
