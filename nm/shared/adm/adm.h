/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
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
#define ADM_ENUM_ALL     0

#define ADM_MAX_NAME 32

#define ADM_CONST_IDX 0
#define ADM_CTRL_IDX  1
#define ADM_EDD_IDX   2
#define ADM_MAC_IDX   3
#define ADM_OPER_IDX  4
#define ADM_RPTT_IDX  5
#define ADM_SBR_IDX   6
#define ADM_TBLT_IDX  7
#define ADM_TBR_IDX   8
#define ADM_VAR_IDX   9
#define ADM_META_IDX  10


extern vector_t g_adm_info;

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

#define ADM_BUILD_ARI_PARM_1(t, n, i, p1) adm_build_ari_parm_6(t, n, i, p1, NULL, NULL, NULL, NULL, NULL)
#define ADM_BUILD_ARI_PARM_2(t, n, i, p1, p2) adm_build_ari_parm_6(t, n, i, p1, p2, NULL, NULL, NULL, NULL)
#define ADM_BUILD_ARI_PARM_3(t, n, i, p1, p2, p3) adm_build_ari_parm_6(t, n, i, p1, p2, p3, NULL, NULL, NULL)
#define ADM_BUILD_ARI_PARM_4(t, n, i, p1, p2, p3, p4) adm_build_ari_parm_6(t, n, i, p1, p2, p3, p4, NULL, NULL)
#define ADM_BUILD_ARI_PARM_5(t, n, i, p1, p2, p3, p4, p5) adm_build_ari_parm_6(t, n, i, p1, p2, p3, p4, p5, NULL)
#define ADM_BUILD_ARI_PARM_6(t, n, i, p1, p2, p3, p4, p5, p6) adm_build_ari_parm_6(t, n, i, p1, p2, p3, p4, p5, p6)


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */

typedef struct
{
	char name[ADM_MAX_NAME];
	int  id;
} adm_info_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int adm_add_adm_info(char *name, int id);

int adm_add_cnst(ari_t *id, edd_collect_fn collect);
int adm_add_ctrldef(uint8_t nn, uvast id, uint8_t num, ctrldef_run_fn run);
int adm_add_ctrldef_ari(ari_t *id, uint8_t num, ctrldef_run_fn run);

int adm_add_edd(ari_t *id, edd_collect_fn collect);
int adm_add_lit(ari_t *id);
int adm_add_macdef(macdef_t *def);
int adm_add_macdef_ctrl(macdef_t *def, ari_t *id);
int adm_add_op(vec_idx_t nn, uvast name, uint8_t num_parm, op_fn apply_fn);
int adm_add_op_ari(ari_t *id, uint8_t num_parm, op_fn apply_fn);

int adm_add_rpttpl(rpttpl_t *def);
int adm_add_tblt(tblt_t *def);
int	adm_add_var_from_expr(ari_t *id, amp_type_e type, expr_t *expr);
int adm_add_var_from_tnv(ari_t *id, tnv_t value)
;

ari_t* adm_build_ari(amp_type_e type, uint8_t has_parms, vec_idx_t nn, uvast id);
ari_t *adm_build_ari_parm_6(amp_type_e type, vec_idx_t nn, uvast id, tnv_t *p1, tnv_t *p2, tnv_t* p3, tnv_t *p4, tnv_t *p5, tnv_t *p6);


int32_t adm_get_parm_int(tnvc_t *parms, uint8_t idx, int *success);
void *adm_get_parm_obj(tnvc_t *parms, uint8_t idx, amp_type_e type);
float adm_get_parm_real32(tnvc_t *parms, uint8_t idx, int *success);
double adm_get_parm_real64(tnvc_t *parms, uint8_t idx, int *success);
uint32_t adm_get_parm_uint(tnvc_t *parms, uint8_t idx, int *success);
uvast adm_get_parm_uvast(tnvc_t *parms, uint8_t idx, int *success);
vast adm_get_parm_vast(tnvc_t *parms, uint8_t idx, int *success);

void adm_init();
void adm_common_init();

#endif /* ADM_H_*/
