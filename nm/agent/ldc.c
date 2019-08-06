/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ldc.c
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
 **  01/11/18  E. Birrane     Update to use report templates and parm maps (JHU/APL)
 **  10/04/18  E. Birrane     UPdate to AMP V0.5. (JHU/APL)
 *****************************************************************************/

#include "../shared/adm/adm.h"
#include "../shared/primitives/report.h"
#include "../shared/primitives/expr.h"

#include "nmagent.h"
#include "ldc.h"


tnv_t* ldc_collect(ari_t *id, tnvc_t *parms)
{
	tnv_t *result = NULL;

	CHKNULL(id);

	switch(id->type)
	{
		case AMP_TYPE_LIT:
			result = ldc_collect_lit(id);
			break;
		case AMP_TYPE_CNST:
			result = ldc_collect_cnst(id, parms);
			break;
		case AMP_TYPE_EDD:
			result = ldc_collect_edd(id, parms);
			break;
		case AMP_TYPE_VAR:
			result = ldc_collect_var(id, parms);
			break;
		case AMP_TYPE_RPTTPL:
			result = ldc_collect_rpt(id, parms);
			break;
		default:
			break;
	}

	if(result == NULL)
	{
		AMP_DEBUG_ERR("ldc_collect","Can't collect value.", NULL);
	}

	return result;
}


tnv_t *ldc_collect_cnst(ari_t *id, tnvc_t *parms)
{
	edd_t *edd = NULL;

	CHKNULL(id);
	edd = (edd_t *) VDB_FINDKEY_CONST(id);
	CHKNULL(edd);

	return edd->def.collect(parms);
}

tnv_t *ldc_collect_edd(ari_t *id, tnvc_t *parms)
{
	edd_t *edd = NULL;

	CHKNULL(id);
	edd = VDB_FINDKEY_EDD(id);
	CHKNULL(edd);

	return edd->def.collect(parms);
}

tnv_t *ldc_collect_lit(ari_t *id)
{
	CHKNULL(id);
	return tnv_copy_ptr(&(id->as_lit));
}

tnv_t *ldc_collect_rpt(ari_t *id, tnvc_t *parms)
{
	tnv_t *result = NULL;
	rpttpl_t *new_tpl = NULL;
	ari_t *new_id = NULL;
	rpt_t *rpt = NULL;

	CHKNULL(id);


	/* grab the template. Copy it. Insert new parms. */
	new_tpl = rpttpl_copy_ptr(VDB_FINDKEY_RPTT(id));
	CHKNULL(new_tpl);

	tnvc_clear(&(new_tpl->id->as_reg.parms));
	tnvc_append(&(new_tpl->id->as_reg.parms), parms);

	/* Build a report for this template. */
	new_id = ari_copy_ptr(new_tpl->id);
	if((rpt = rpt_create(new_id, getCtime(), NULL)) == NULL)
	{
		rpttpl_release(new_tpl, 1);
		ari_release(new_id, 1);
	}

	/* Populate the report. */
	ldc_fill_rpt(new_tpl, rpt);

	rpttpl_release(new_tpl, 1);

	/* Create TNV and hold report as a result. */
	if((result = tnv_create()) == NULL)
	{
		rpt_release(rpt, 1);
		return NULL;
	}

	result->type = AMP_TYPE_RPT;
	result->value.as_ptr = rpt;
	TNV_SET_ALLOC(result->flags);

	return result;
}

tnv_t *ldc_collect_var(ari_t *id, tnvc_t *parms)
{
	var_t *var = NULL;

	CHKNULL(id);
	var = VDB_FINDKEY_VAR(id);
	if(var == NULL)
	{
		return NULL;
	}

	return tnv_copy_ptr(var->value);
}



/******************************************************************************
 *
 * \par Function Name: ldc_fill_rpt
 *
 * \par Populate a report from a report template. This is somewhat
 *      tricky because a report template may, itself, contain other
 *      report templates.
 *
 * \return AMP Status Code
 *
 * \param[in]  rpttpl  The report template
 * \param[out] rpt     The report being filled out.
 *
 * \par Notes:
 *		- We impose a maximum nesting level of 5. A template may
 *		  contain no more than 5 nested templates.
 *		- In this architecture, report generation is serialized and, therefore
 *		  a static variable can be used to count nesting in a single report.
 *		- Report is assumed to have been created and populated with all except
 *		  its entries. Entries is assumed to be empty.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/22/11  E. Birrane     Initial implementation.
 *  08/18/13  E. Birrane     Added nesting levels to limit recursion.
 *  07/04/15  E. Birrane     Updated to new reporting structure and TDCs
 *  01/11/18  E. Birrane     Renamed ldc_fill_report. Use report templates and parm maps.
 *  10/04/18  E. Birrane     Update to AMP v0.5. (JHU/APL)
 *****************************************************************************/

int ldc_fill_rpt(rpttpl_t *rpttpl, rpt_t *rpt)
{
	static int nesting = 0;
	uint8_t i;
	int success;

	AMP_DEBUG_ENTRY("ldc_fill_rpt","("ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
			          (uaddr) rpttpl, (uaddr) rpt);

	CHKUSR(rpttpl, AMP_FAIL);
	CHKUSR(rpt, AMP_FAIL);


	/* Step 1: Check for too much recursion. */
	if(nesting > LDC_MAX_NESTING)
	{
		AMP_DEBUG_ERR("ldc_fill_rpt","Too many nesting levels %d.", nesting);
		return AMP_FAIL;
	}

	nesting++;

	success = AMP_OK;
	/* Step 2: For every item in the template, fill in the entry. */
	for(i = 0; i < ac_get_count(&(rpttpl->contents)); i++)
	{
		uint8_t clear_parms = 1;
		tnvc_t *parms = NULL;
		tnv_t *cur_val = NULL;

		/* Grab the current template ID. */
		ari_t *cur_id = ac_get(&(rpttpl->contents), i);


		if(cur_id->type != AMP_TYPE_LIT)
		{
			/* Step 1: If a rpttpl is parameterized, then the acutal report
			 * structure will contain the parameters.
			 */
			parms = ari_resolve_parms(&(cur_id->as_reg.parms), &(rpt->id->as_reg.parms));
			if(parms == NULL)
			{
				parms = &(cur_id->as_reg.parms);
				clear_parms = 0;
			}
		}
		else
		{
			clear_parms = 0;
		}


		cur_val = ldc_collect(cur_id, parms);
		if(clear_parms)
		{
			tnvc_release(parms, 1);
		}

		if(rpt_add_entry(rpt, cur_val) != AMP_OK)
		{
			AMP_DEBUG_ERR("ldc_fill_rpt","Can't get entry for idx %d", i);
			tnv_release(cur_val, 1);
			success = AMP_FAIL;
			break;
		}
	}


    /* Step 4: If there was an error, roll back. */
    if(success != AMP_OK)
    {
    	tnvc_clear(rpt->entries);
    }

	nesting--;

	return success;
}

