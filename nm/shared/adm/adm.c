/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: adm.c
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
 **  01/10/18  E. Birrane     CLean up reports, added report parameter maps. (JHU/APL)
 *****************************************************************************/

#include "ion.h"
#include "platform.h"

#include "adm.h"


#include "../utils/nm_types.h"
#include "../utils/utils.h"
#include "../utils/db.h"

#include "../primitives/expr.h"

vector_t g_adm_info;

/** g_amp_agent_idx is utilized by all ADMs.  
 * It is initialized in adm_amp_agent_(agent|mgr).c, but it is a resource global to all ADMs.
 * That initialization should be moved to this file at a later date (TODO)
 */
vec_idx_t g_amp_agent_idx[11];

int adm_add_adm_info(char *name, int id)
{
	adm_info_t *info = STAKE(sizeof(adm_info_t));
	CHKERR(info);

	strncpy(info->name, name, ADM_MAX_NAME-1);
	info->id = id;
	return vec_push(&g_adm_info, info);
}

/******************************************************************************
 *
 * \par Function Name: adm_add_cnst
 *
 * \par Registers a pre-configured ADM constant with the local AMP actor.
 *
 * \retval AMP STATUS CODE
 *
 * \param[in] id        The identifier for this object
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/02/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/
int adm_add_cnst(ari_t *id, edd_collect_fn collect)
{
	edd_t *def = NULL;
	int rh_code;

	if(id == NULL)
	{
		return AMP_FAIL;
	}

	if((def = edd_create(id, NULL, collect)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_CONST(def->def.id, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_cnst","Ignoring duplicate item.", NULL);
		edd_release(def, 1);
	}
	else if(rh_code != RH_OK)
	{
		edd_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}



/******************************************************************************
 *
 * \par Function Name: adm_add_ctrldef
 *
 * \par Registers a pre-configured ADM control definition with the local AMP actor.
 *
 * \retval AMP STATUS CODE
 *
 * \param[in] id   The identifier for this object
 * \param[in] num  The number of parameters accepted by this function.
 * \param[in] adm  The ADM that defined this control definition.
 * \param[in] run  The function which runs this control.
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/02/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/
int adm_add_ctrldef_ari(ari_t *id, uint8_t num, ctrldef_run_fn run)
{
	ctrldef_t *def;
	int rh_code;

	if((def = ctrldef_create(id, num, run)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_CTRLDEF(def->ari, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_ctrldef_ari","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		ctrldef_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}

int adm_add_ctrldef(uint8_t nn, uvast name, uint8_t num, ctrldef_run_fn run)
{
	ari_t *id = adm_build_ari(AMP_TYPE_CTRL, (num > 0) ? 1 : 0, nn, name);

	return adm_add_ctrldef_ari(id, num, run);
}

/******************************************************************************
 *
 * \par Function Name: adm_add_edd
 *
 * \par Registers a pre-configured ADM EDD with the local AMP actor.
 *
 * \retval AMP STATUS CODE
 *
 * \param[in] id        The identifier for this EDD.
 * \param[in] collect   The data collection function.
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/15  E. Birrane     Added type information.
 *  01/05/18  E. Birrane     Renamed add_edd and take mid_t.
 *  10/02/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/
int adm_add_edd(ari_t *id, edd_collect_fn collect)
{
	edd_t *def = NULL;
	int rh_code;

	CHKUSR(id, AMP_FAIL);

	if((def = edd_create(id, NULL, collect)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_EDD(def->def.id, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_edd","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		edd_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}



/******************************************************************************
 *
 * \par Function Name: adm_add_lit
 *
 * \par Registers a pre-configured ADM literal with the local AMP actor.
 *
 * \retval AMP Status Code
 *
 * \param[in] id    The Literal object
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/05/18  E. Birrane     Changed to accept mid_t.
 *  10/02/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int adm_add_lit(ari_t *id)
{
	int rh_code;

	if(id == NULL)
	{
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_LIT(id, id);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_lit","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		ari_release(id, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}





/******************************************************************************
 *
 * \par Function Name: adm_add_macro
 *
 * \par Registers a pre-configured ADM macro with the local AMP actor.
 *
 * \retval AMP Status Code
 *
 * \param[in] id     The Id of the macro.
 * \param[in] ctrls  The ordered list of items comprising the macro.
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/05/18  E. Birrane     Changed to accept mid_t.
 *  10/02/18  E. Birrane     Updated to AMP v0.5
 *****************************************************************************/

int adm_add_macdef(macdef_t *def)
{
	int rh_code = 0;

	if(def == NULL)
	{
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_MACDEF(def->ari, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_macdef","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		macdef_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}


int adm_add_macdef_ctrl(macdef_t *def, ari_t *id)
{
	ctrl_t *ctrl = NULL;

	if((def == NULL) || (id == NULL))
	{
		return AMP_FAIL;
	}

	ctrl = ctrl_create(id);

	ari_release(id, 1);
	return macdef_append(def, ctrl);
}

/******************************************************************************
 *
 * \par Function Name: adm_add_op
 *
 * \par Registers a pre-configured ADM operator with the local DTNMP actor.
 *
 * \retval AMP Status Code
 *
 * \param[in] id        The IF of the operator.
 * \param[in] num_parm  The number of operands this operator takes.
 * \param[in] apply_fn  Function for applying the operator
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/05/18  E. Birrane     Changed to accept mid_t.
 *  10/02/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/
int adm_add_op_ari(ari_t *id, uint8_t num_parm, op_fn apply_fn)
{
	op_t *def = NULL;
	int rh_code;

	if((def = op_create(id, num_parm, apply_fn)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_OP(def->id, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_op_ari","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		op_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}

int adm_add_op(vec_idx_t nn, uvast name, uint8_t num_parm, op_fn apply_fn)
{
	return adm_add_op_ari(adm_build_ari(AMP_TYPE_OPER, 1, nn, name),num_parm, apply_fn);
}



/******************************************************************************
 *
 * \par Function Name: adm_add_rpttpl
 *
 * \par Registers a pre-configured ADM report template with the local DTNMP actor.
 *
 * \retval AMP Status Code
 *
 * \param[in] id        The Report Template Identifier.
 * \param[in] contents  The ordered IDs of the report.
 *
 * \par Notes:
 *   - This function takes ownership of the pointers given to it. The caller
 *     MUST NOT access this data directly after this call. EVEN when the
 *     function returns an error.
 *   - Passed-in pointers must be allocated on the heap.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/05/18  E. Birrane     Changed to accept mid_t.
 *  01/09/18  E. Birrane     Renamed adm_add_rpt and added parm map support.
 *  10/02/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int adm_add_rpttpl(rpttpl_t *def)
{
	int rh_code;

	if(def == NULL)
	{
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_RPTT(def->id, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_rpttpl","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		rpttpl_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}



int adm_add_tblt(tblt_t *def)
{
	int rh_code;

	if(def == NULL)
	{
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_TBLT(def->id, def);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_tblt","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		tblt_release(def, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}


int	adm_add_var_from_expr(ari_t *id, amp_type_e type, expr_t *expr)
{
	var_t *new_var = NULL;
	int rh_code;

	if((id == NULL) || (expr == NULL))
	{
		return AMP_FAIL;
	}

	// TODO: Make sure we don't leak expr.
	if((new_var = var_create(id, type, expr)) == NULL)
	{
		AMP_DEBUG_ERR("adm_add_var_from_expr","Unable to make new var.", NULL);
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_VAR(new_var->id, new_var);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_var_from_expr","Ignoring duplicate item.", NULL);
	}

	if(rh_code != RH_OK)
	{
		var_release(new_var, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}

int adm_add_var_from_tnv(ari_t *id, tnv_t value)
{
	var_t *new_var = NULL;
	int rh_code;

	if(id == NULL)
	{
		return AMP_FAIL;
	}

	if((new_var = var_create_from_tnv(id, value)) == NULL)
	{
		ari_release(id, 1);
		tnv_release(&value, 0);
		return AMP_FAIL;
	}

	rh_code = VDB_ADD_VAR(new_var->id, new_var);

	if(rh_code == RH_DUPLICATE)
	{
		AMP_DEBUG_WARN("adm_add_var_from_tnv","Ignoring duplicate item.", NULL);
	}
	if(rh_code != RH_OK)
	{
		var_release(new_var, 1);
	}

	return ((rh_code == RH_OK) || (rh_code == RH_DUPLICATE)) ? AMP_OK : AMP_FAIL;
}


// Takes over name and parms, no matter what.
ari_t* adm_build_ari(amp_type_e type, uint8_t has_parms, vec_idx_t nn, uvast id)
{
	ari_t *result = ari_create(type);
	CHKNULL(result);

	/* Set the flags byte. Since this is coming from an ADM,
	 * it MUST have a NNand MUST NOT have an ISS or TAG.
	 * It will have parms iff parms are provided.
	 */
	result->as_reg.flags = 0;
	ARI_SET_FLAG_TYPE(result->as_reg.flags, type);
	ARI_SET_FLAG_NN(result->as_reg.flags);
	if(has_parms)
	{
		ARI_SET_FLAG_PARM(result->as_reg.flags);
	}

	result->as_reg.nn_idx = nn;

	/*
	 * The name will be provided as an integer, which we will store
	 * in its CBOR encoded format for simplicity andto avoid issues
	 * with endian encodings.
	 */
	if(cut_enc_uvast(id, &(result->as_reg.name)) != AMP_OK)
	{
		AMP_DEBUG_ERR("adm_build_reg_ari","Cannot encode id.", NULL);
		ari_release(result, 1);
		return NULL;
	}

	return result;
}


ari_t *adm_build_ari_parm_6(amp_type_e type, vec_idx_t nn, uvast id, tnv_t *p1, tnv_t *p2, tnv_t* p3, tnv_t *p4, tnv_t *p5, tnv_t *p6)
{
	ari_t *ari = adm_build_ari(type, 1, nn, id);

	ari_add_parm_val(ari, p1);
	ari_add_parm_val(ari, p2);
	ari_add_parm_val(ari, p3);
	ari_add_parm_val(ari, p4);
	ari_add_parm_val(ari, p5);
	ari_add_parm_val(ari, p6);

	return ari;
}


int32_t adm_get_parm_int(tnvc_t *parms, uint8_t idx, int *success)
{
	tnv_t *val = tnvc_get(parms, idx);
	return (val == NULL) ? 0 : tnv_to_int(*val, success);}


void *adm_get_parm_obj(tnvc_t *parms, uint8_t idx, amp_type_e type)
{
	tnv_t *val = tnvc_get(parms, idx);

	if((val == NULL) ||
	   (val->value.as_ptr == NULL) ||
	   (val->type != type))
	{
		AMP_DEBUG_ERR("adm_get_parm_obj","Parm error.", NULL);
		return NULL;
	}

	return val->value.as_ptr;
}

float adm_get_parm_real32(tnvc_t *parms, uint8_t idx, int *success)
{
	tnv_t *val = tnvc_get(parms, idx);
	return (val == NULL) ? 0 : tnv_to_real32(*val, success);}

double adm_get_parm_real64(tnvc_t *parms, uint8_t idx, int *success)
{
	tnv_t *val = tnvc_get(parms, idx);
	return (val == NULL) ? 0 : tnv_to_real64(*val, success);}

uint32_t adm_get_parm_uint(tnvc_t *parms, uint8_t idx, int *success)
{
	tnv_t *val = tnvc_get(parms, idx);
	return (val == NULL) ? 0 : tnv_to_uint(*val, success);}

uvast adm_get_parm_uvast(tnvc_t *parms, uint8_t idx, int *success)
{
	tnv_t *val = tnvc_get(parms, idx);
	return (val == NULL) ? 0 : tnv_to_uvast(*val, success);}

vast adm_get_parm_vast(tnvc_t *parms, uint8_t idx, int *success)
{
	tnv_t *val = tnvc_get(parms, idx);
	return (val == NULL) ? 0 : tnv_to_vast(*val, success);
}

/******************************************************************************
 *
 * \par Function Name: adm_common_init
 *
 * \par Initialize pre-configured ADMs.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *  12/01/19  D. Edell       Split into adm_common() and adm_common_init() to
 *                             break potential circular dependencies.
 *****************************************************************************/

void adm_common_init()
{
	int success;

	AMP_DEBUG_ENTRY("adm_init","()", NULL);

	g_adm_info = vec_create(8, NULL, NULL, NULL, 0, &success);

	adm_add_adm_info("ALL", 0);

	AMP_DEBUG_EXIT("adm_init","->.", NULL);
}
