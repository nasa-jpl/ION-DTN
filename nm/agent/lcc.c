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
 ** File Name: lcc.h
 **
 ** Description: This implements the NM Agent Local Command and Control (LDC).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/22/13  E. Birrane     Update to latest version of DTNMP. Cleanup.
 *****************************************************************************/

#include "shared/adm/adm.h"
#include "shared/primitives/mid.h"
#include "shared/primitives/rules.h"

#include "nmagent.h"
#include "lcc.h"


/**
 * \brief Perform the control given by the MID value.
 *
 * \author Ed Birrane
 *
 * \return 0 - Success
 *        !0 - Failure
 *
 * \param[in]  id   The ID of the control to be executed.
 */
int lcc_run_ctrl_mid_t(mid_t *id)
{
    int result = 0;
    adm_ctrl_t *adm_ctrl = NULL;
    def_gen_t *macro_def = NULL;

    char *msg = NULL;

    DTNMP_DEBUG_ENTRY("lcc_run_ctrl","(0x%x)", (unsigned long) id);

    /* Step 0: Sanity Check */
    if((id == NULL) || (id->oid == NULL))
    {
    	DTNMP_DEBUG_ERR("lcc_run_ctrl","Bad Args.",NULL);
    	DTNMP_DEBUG_EXIT("lcc_run_ctrl","-> -1",NULL);
    	return -1;
    }

    msg = mid_to_string(id);
    DTNMP_DEBUG_INFO("lcc_run_ctrl","Running control: %s", msg);

    /* Step 1: See if this identifies an atomic control. */
    if((adm_ctrl = adm_find_ctrl(id)) != NULL)
    {
    	DTNMP_DEBUG_INFO("lcc_run_ctrl","Found control.", NULL);
    	result = adm_ctrl->run(id->oid->params);
    }

    /* Step 2: Otherwise, see if this identifies a pre-defined macro. */
    else if((macro_def = def_find_by_id(macro_defs, &macro_defs_mutex, id)) != NULL)
    {
    	LystElt elt;
    	mid_t *mid;

    	for(elt = lyst_first(macro_def->contents); elt; elt = lyst_next(elt))
    	{
    		mid = (mid_t *)lyst_data(elt);
    		/* \todo watch infinite recursion */
    		lcc_run_ctrl_mid_t(mid);
    	}
    }

    /* Step 3: Otherwise, give up. */
    else
    {
    	DTNMP_DEBUG_ERR("lcc_run_ctrl","Could not find control for MID %s",
    			        msg);
    	result = -1;
    }

    MRELEASE(msg);

    DTNMP_DEBUG_EXIT("lcc_run_ctrl","-> %d", result);
    return result;
}


int lcc_run_ctrl_ctrl_exec_t(ctrl_exec_t *ctrl_p)
{
	int result = 0;
    LystElt elt;
    mid_t *cur_ctrl = NULL;

	DTNMP_DEBUG_ENTRY("lcc_run_ctrl","(0x%x)", (unsigned long) ctrl_p);

	/* Step 0: Sanity Check */
	if(ctrl_p == NULL)
	{
		DTNMP_DEBUG_ERR("lcc_run_ctrl","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("lcc_run_ctrl","-> -1",NULL);
		return -1;
	}

	DTNMP_DEBUG_INFO("lcc_run_ctrl","Found control.", NULL);

    char *str = NULL;

    /*
     * Walk through the macro, running controls as we go.
     */
    for (elt = lyst_first(ctrl_p->contents); elt; elt = lyst_next(elt))
    {
        /* Grab the next ctrl...*/
        if((cur_ctrl = (mid_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("lcc_run_ctrl","Found NULL ctrl. Exiting", NULL);
            DTNMP_DEBUG_EXIT("lcc_run_ctrl","->-1.", NULL);
            return -1;
        }

        str = mid_pretty_print(cur_ctrl);
        printf("EJB: trying to run MID %s.\n",str);
        MRELEASE(str);

    	result = lcc_run_ctrl_mid_t(cur_ctrl);
    }



	DTNMP_DEBUG_EXIT("lcc_run_ctrl","-> %d", result);
	return result;
}
