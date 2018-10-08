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
#define ADM_ALL   	    	  0
#define ADM_AGENT    		  1
#define ADM_BP_AGENT	      2
#define ADM_LTP_AGENT		  3
#define ADM_BPSEC_AGENT       4
#define ADM_ION_BP_ADMIN      5
#define ADM_ION_IPN_ADMIN     6
#define ADM_ION_ION_ADMIN 	  7
#define ADM_ION_IONSEC_ADMIN  8
#define ADM_ION_LTP_ADMIN 	  9



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
int	adm_add_macdef(ari_t *id, vector_t ctrls);
int adm_add_op(ari_t *id, uint8_t num_parm, op_fn apply_fn);
int adm_add_rpttpl(ari_t *id, ac_t contents);
int adm_add_tblt(tblt_t *def);
int	adm_add_vardef(ari_t *id, amp_type_e type, expr_t *expr);


void *adm_extract_parm(tnvc_t *parms, uint8_t idx, amp_type_e type);

void adm_init();

#endif /* ADM_H_*/
