/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
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
 **  10/22/11  E. Birrane     Code comments and functional updates.
 **  01/10/13  E. Birrane     Update to latest version of DTNMP. Cleanup.
 *****************************************************************************/

#ifndef _LDC_H_
#define _LDC_H_

#include "shared/adm/adm.h"

#include "shared/msg/msg_reports.h"
#include "shared/msg/msg_def.h"

int ldc_fill_report_data(mid_t *id, rpt_data_entry_t *entry);
int ldc_fill_atomic(adm_datadef_t *adm_def, mid_t *id, rpt_data_entry_t *rpt);
int ldc_fill_custom(def_gen_t *rpt_def, rpt_data_entry_t *rpt);

#endif // _LDC_H_

