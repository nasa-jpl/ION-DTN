/******************************************************************************
#include <shared/adm/adm_bpsec.h>
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
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


/*
#include "adm_agent.h"
#include "adm_bp.h"
#include "adm_bpsec.h"
#include "adm_ion_admin.h"
#include "adm_ion_bp_admin.h"
#include "adm_ion_ipn_admin.h"
#include "adm_ionsec_admin.h"
#include "adm_ion_ltp_admin.h"
#include "adm_ltp_agent.h"
*/

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
int adm_add_cnst(ari_t *id)
{
	int success;

	CHKUSR(id, AMP_FAIL);
	if((success = VDB_ADD_CONST(id, id)) != AMP_OK)
	{
		ari_release(id, 1);
	}
	return success;
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
int adm_add_ctrldef(ari_t *id, uint8_t num, uint8_t adm, ctrldef_run_fn run)
{
	ctrldef_t *def;
	int success;

	if((def = ctrldef_create(id, num, adm, run)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	if((success = VDB_ADD_CTRLDEF(def->ari, def)) != AMP_OK)
	{
		ctrldef_release(def, 1);
	}

	AMP_DEBUG_EXIT("adm_add_ctrldef","-> %d.", success);

	return success;
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
	int success;

	CHKUSR(id, AMP_FAIL);
	CHKUSR(collect, AMP_FAIL);

	if((def = edd_create(id, NULL, collect)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	if((success = VDB_ADD_EDD(def->def.id, def)) != AMP_OK)
	{
		edd_release(def, 1);
	}

	AMP_DEBUG_EXIT("adm_add_edd","-> %d.", success);
	return success;
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
	int success;

	CHKUSR(id, AMP_FAIL);
	if((success = VDB_ADD_LIT(id, id)) != AMP_OK)
	{
		ari_release(id, 1);
	}

	AMP_DEBUG_EXIT("adm_add_lit","-> %d.", success);
	return success;
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

int adm_add_macdef(ari_t *id, vector_t ctrls)
{
	macdef_t *def = NULL;
	int success = 0;

	CHKUSR(id, AMP_FAIL);

	if((def = macdef_create(vec_num_entries(ctrls), id)) == NULL)
	{
		ari_release(id, 1);
		vec_release(&ctrls, 0);
		return AMP_FAIL;
	}

	def->ctrls = ctrls;

	if((success = VDB_ADD_MACDEF(def->ari, def)) != AMP_OK)
	{
		macdef_release(def, 1);
	}

	AMP_DEBUG_EXIT("adm_add_op","-> %d.", success);
	return success;
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
int adm_add_op(ari_t *id, uint8_t num_parm, op_fn apply_fn)
{
	op_t *def = NULL;
	int success;

	CHKUSR(id, AMP_FAIL);
	CHKUSR(apply_fn, AMP_FAIL);

	if((def = op_create(id, num_parm, apply_fn)) == NULL)
	{
		ari_release(id, 1);
		return AMP_FAIL;
	}

	if((success = VDB_ADD_OP(def->id, def)) != AMP_OK)
	{
		op_release(def, 1);
	}

	AMP_DEBUG_EXIT("adm_add_op","-> %d.", success);
	return success;

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

int adm_add_rpttpl(ari_t *id, ac_t contents)
{
	rpttpl_t *def = NULL;
	int success;

	CHKUSR(id, AMP_FAIL);



	if((def = rpttpl_create(id, contents)) == NULL)
	{
		ari_release(id, 1);
		ac_release(&contents, 0);
		return AMP_FAIL;
	}

	if((success = VDB_ADD_RPTT(def->id, def)) != AMP_OK)
	{
		rpttpl_release(def, 1);
	}

	AMP_DEBUG_EXIT("adm_add_rpttpl","-> %d.", success);
	return success;
}



int adm_add_tblt(tblt_t *def)
{
	int success;

	CHKUSR(def, AMP_FAIL);

	if((success = VDB_ADD_TBLT(&(def->id), def)) != AMP_OK)
	{
		tblt_release(def, 1);
	}

	AMP_DEBUG_EXIT("adm_add_tblt","-> %d.", success);
	return success;
}


int	adm_add_var_from_expr(ari_t *id, amp_type_e type, expr_t *expr)
{
	var_t *new_var = NULL;
	int success;

	CHKUSR(id, AMP_FAIL);
	CHKUSR(expr, AMP_FAIL);

	// TODO: Make sure we don't leak expr.
	if((new_var = var_create(id, type, expr)) == NULL)
	{
		ari_release(id, 1);
		expr_release(expr, 1);
		return AMP_FAIL;
	}

	if((success = VDB_ADD_TBLT(new_var->id, new_var)) != AMP_OK)
	{
		var_release(new_var, 1);
	}


	AMP_DEBUG_EXIT("adm_add_var_from_expr","-> %d.", success);

	return success;
}

int adm_add_var_from_tnv(ari_t *id, tnv_t value)
{
	var_t *new_var = NULL;
	int success;

	CHKUSR(id, AMP_FAIL);

	if((new_var = var_create_from_tnv(id, value)) == NULL)
	{
		ari_release(id, 1);
		tnv_release(&value, 0);
		return AMP_FAIL;
	}

	if((success = VDB_ADD_TBLT(new_var->id, new_var)) != AMP_OK)
	{
		var_release(new_var, 1);
	}


	AMP_DEBUG_EXIT("adm_add_var_from_tnv","-> %d.", success);

	return success;
}



void *adm_extract_parm(tnvc_t *parms, uint8_t idx, amp_type_e type)
{
	tnv_t *val = tnvc_get(parms, idx);

	CHKNULL(val);
	CHKNULL(val->value.as_ptr);
	CHKNULL(val->type == type);

	return val->value.as_ptr;
}

/******************************************************************************
 *
 * \par Function Name: adm_init
 *
 * \par Initialize pre-configured ADMs.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

void adm_init()
{
	AMP_DEBUG_ENTRY("adm_init","()", NULL);
/*
	adm_agent_init();
	adm_bp_init();
	adm_bpsec_init();
	adm_ion_admin_init();
	adm_ion_bp_admin_init();
	adm_ion_ipn_admin_init();
	adm_ionsec_admin_init();
	adm_ion_ltp_admin_init();
	adm_ltp_agent_init();
	*/

	AMP_DEBUG_EXIT("adm_init","->.", NULL);
}
