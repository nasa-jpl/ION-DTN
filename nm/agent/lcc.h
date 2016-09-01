/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: lcc.h
 **
 ** Description: This implements the NM Agent Local Command and Control (LCC).
 **              This applies controls and macros.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/22/13  E. Birrane     Update to latest version of DTNMP. Cleanup. (JHU/APL)
 *****************************************************************************/

#ifndef _LCC_H_
#define _LCC_H_

#include "../shared/adm/adm.h"



int lcc_run_ctrl_mid(mid_t *id);
int lcc_run_ctrl(ctrl_exec_t *ctrl);

int lcc_run_macro(Lyst macro);

void lcc_send_retval(eid_t *rx, tdc_t *retval, mid_t *mid);


#endif // _LCC_H_

