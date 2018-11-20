/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
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
 **  10/04/18  E. Birrane     Updaye to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef _LDC_H_
#define _LDC_H_

#include "../shared/adm/adm.h"

#include "../shared/primitives/report.h"

#define LDC_MAX_NESTING (5)


tnv_t* ldc_collect(ari_t *id, tnvc_t *parms);

tnv_t *ldc_collect_cnst(ari_t *id, tnvc_t *parms);
tnv_t *ldc_collect_edd(ari_t *id, tnvc_t *parms);
tnv_t *ldc_collect_lit(ari_t *id);
tnv_t *ldc_collect_rpt(ari_t *id, tnvc_t *parms);
tnv_t *ldc_collect_var(ari_t *id, tnvc_t *parms);

int    ldc_fill_rpt(rpttpl_t *rpttpl, rpt_t *rpt);


#endif // _LDC_H_

