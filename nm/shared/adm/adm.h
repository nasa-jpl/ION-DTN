/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Application Data Models (ADMs).
 **
 ** Notes:
 **       1) We need to find some more efficient way of querying ADMs by name
 **          and by MID. The current implementation uses too much stack space.
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Initial Implementation (JHU/APL)
 **  11/13/12  E. Birrane     Technical review, comment updates. (JHU/APL)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 **  10/02/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef ADM_H_
#define ADM_H_

#include "../utils/nm_types.h"
#include "../primitives/edd_var.h"
#include "../primitives/report.h"
#include "../primitives/ctrl.h"
#include "../primitives/table.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/* Known ADMs Enumerations.*/
#define ADM_ENUM_ALL   	    	   0
#define ADM_ENUM_AGENT    		   1
#define ADM_ENUM_BP_AGENT	       2
#define ADM_ENUM_LTP_AGENT		   3
#define ADM_ENUM_BPSEC_AGENT       4
#define ADM_ENUM_ION_BP_ADMIN      5
#define ADM_ENUM_ION_IPN_ADMIN     6
#define ADM_ENUM_ION_ION_ADMIN 	   7
#define ADM_ENUM_ION_IONSEC_ADMIN  8
#define ADM_ENUM_ION_LTP_ADMIN 	   9


#define ADM_CONST_OFFSET 0
#define ADM_CTRL_OFFSET  1
#define ADM_EDD_OFFSET   2
#define ADM_MAC_OFFSET   3
#define ADM_OPER_OFFSET  4
#define ADM_RPTT_OFFSET  5
#define ADM_SBR_OFFSET   6
#define ADM_TBLT_OFFSET  7
#define ADM_TBR_OFFSET   8
#define ADM_VAR_OFFSET   9
#define ADM_META_OFFSET  10


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int adm_add_cnst(ari_t *id);
int adm_add_ctrldef(ari_t *id, uint8_t num, uint8_t adm, ctrldef_run_fn run);
int adm_add_edd(ari_t *id, edd_collect_fn collect);
int adm_add_lit(ari_t *id);
int adm_add_macdef(macdef_t *def);
int adm_add_op(ari_t *id, uint8_t num_parm, op_fn apply_fn);
int adm_add_rpttpl(rpttpl_t *def);
int adm_add_tblt(tblt_t *def);
int	adm_add_vardef(ari_t *id, amp_type_e type, expr_t *expr);

ari_t* adm_build_reg_ari(uint8_t flags, vec_idx_t nn, uvast id, tnvc_t *parms);

int32_t adm_get_parm_int(tnvc_t *parms, uint8_t idx, int *success);
void *adm_get_parm_obj(tnvc_t *parms, uint8_t idx, amp_type_e type);
float adm_get_parm_real32(tnvc_t *parms, uint8_t idx, int *success);
double adm_get_parm_real64(tnvc_t *parms, uint8_t idx, int *success);
uint32_t adm_get_parm_uint(tnvc_t *parms, uint8_t idx, int *success);
uint32_t adm_get_parm_uvast(tnvc_t *parms, uint8_t idx, int *success);
uint32_t adm_get_parm_vast(tnvc_t *parms, uint8_t idx, int *success);

void adm_init();

#endif /* ADM_H_*/
