/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
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
#include "../shared/utils/nm_types.h"

#define LCC_MAX_NESTING 5


// Todo: Talk about why we separate out parameters.

int lcc_run_ac(ac_t *ac, tnvc_t *parent_parms);

int lcc_run_ctrl(ctrl_t *ctrl, tnvc_t *parent_parms);

int lcc_run_macro(macdef_t *mac, tnvc_t *parent_parms);



void lcc_send_retval(eid_t *rx, tnv_t *retval, ctrl_t *ctrl, tnvc_t *parms);


#endif // _LCC_H_

