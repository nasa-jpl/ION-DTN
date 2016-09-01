/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ldc.h
 **
 ** Description: This implements the NM Agent Local Data Collector (LDC).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Code comments and functional updates. (JHU/APL)
 **  01/10/13  E. Birrane     Update to latest version of DTNMP. Cleanup. (JHU/APL)
 *****************************************************************************/

#ifndef _LDC_H_
#define _LDC_H_

#include "../shared/primitives/var.h"
#include "../shared/adm/adm.h"

#include "../shared/primitives/report.h"

int ldc_fill_report_entry(rpt_entry_t *entry);
int ldc_fill_atomic(adm_datadef_t *adm_def, rpt_entry_t *entry);
int ldc_fill_custom(def_gen_t *rpt_def, rpt_entry_t *entry);
int ldc_fill_computed(var_t *cd, rpt_entry_t *entry);


#endif // _LDC_H_

