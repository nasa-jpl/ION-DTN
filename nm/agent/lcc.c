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
 ** File Name: lcc.c
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
#include "shared/primitives/instr.h"

#include "nmagent.h"
#include "lcc.h"


/******************************************************************************
 *
 * \par Function Name: lcc_run_ctrl_mid
 *
 * \par Run a control given the MID associated with that control.
 *
 * \param[in]  id   The MID identifying the control
 *
 * \par Notes:
 *
 * \return -1 - Error
 *         !(-1) - Value returned from the run control.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int lcc_run_ctrl_mid(mid_t *id)
{
    int result = 0;
    adm_ctrl_t *adm_ctrl = NULL;
    def_gen_t *macro_def = NULL;
    static int nesting = 0;
    char *msg = NULL;
    Lyst parms = NULL;


    nesting++;

    DTNMP_DEBUG_ENTRY("lcc_run_ctrl_mid","(0x%#llx)", (unsigned long) id);


    /* Step 0: Sanity Check */
    if((id == NULL) || (id->oid == NULL))
    {
    	DTNMP_DEBUG_ERR("lcc_run_ctrl_mid","Bad Args.",NULL);
    	DTNMP_DEBUG_EXIT("lcc_run_ctrl_mid","-> -1",NULL);
    	nesting--;
    	return -1;
    }

    if(nesting > 5)
    {
    	DTNMP_DEBUG_ERR("lcc_run_ctrl_mid","Too many nesting levels (%d).",nesting);
    	DTNMP_DEBUG_EXIT("lcc_run_ctrl_mid","-> -1",NULL);
    	nesting--;
    	return -1;
    }

    msg = mid_to_string(id);
    DTNMP_DEBUG_INFO("lcc_run_ctrl_mid","Running control: %s", msg);

    /* Step 1: See if this identifies an atomic control. */
    if((adm_ctrl = adm_find_ctrl(id)) != NULL)
    {
    	DTNMP_DEBUG_INFO("lcc_run_ctrl_mid","Found control.", NULL);
    	result = adm_ctrl->run(id->oid->params);
    	gAgentInstr.num_ctrls_run++;

    }

    /* Step 2: Otherwise, see if this identifies a pre-defined macro. */
    else if((macro_def = def_find_by_id(gAgentVDB.macros, &(gAgentVDB.macros_mutex), id)) != NULL)
    {
    	LystElt elt;
    	mid_t *mid;

    	/*
    	 * Step 2.1: This is a macro. Walk through each control running
    	 *           the controls as we go.
    	 */
    	for(elt = lyst_first(macro_def->contents); elt; elt = lyst_next(elt))
    	{
    		mid = (mid_t *)lyst_data(elt);
    		result = lcc_run_ctrl_mid(mid);
    		if(result != 0)
    		{
    			DTNMP_DEBUG_WARN("lcc_run_ctrl_mid","Running MID %s returned %d",
    					         msg, result);
    		}
    	}
    }

    /* Step 3: Otherwise, give up. */
    else
    {
    	DTNMP_DEBUG_ERR("lcc_run_ctrl_mid","Could not find control for MID %s",
    			        msg);
    	result = -1;
    }

    MRELEASE(msg);

    nesting--;
    DTNMP_DEBUG_EXIT("lcc_run_ctrl_mid","-> %d", result);
    return result;
}



/******************************************************************************
 *
 * \par Function Name: lcc_run_ctrl
 *
 * \par Run a control given a control execution structure.
 *
 * \param[in]  ctrl_p  The control execution structure.
 *
 * \par Notes:
 *
 * \return -1 - Error
 *         !(-1) - Value returned from the run control.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial implementation.
 *****************************************************************************/

int lcc_run_ctrl(ctrl_exec_t *ctrl_p)
{
	int result = 0;
	char *msg = NULL;
	Lyst parms = NULL;
    LystElt elt;
    mid_t *cur_ctrl = NULL;
    char *str = NULL;

	DTNMP_DEBUG_ENTRY("lcc_run_ctrl","(0x%x)", (unsigned long) ctrl_p);

	/* Step 0: Sanity Check */
	if(ctrl_p == NULL)
	{
		DTNMP_DEBUG_ERR("lcc_run_ctrl","Bad Args.",NULL);
		DTNMP_DEBUG_EXIT("lcc_run_ctrl","-> -1",NULL);
		return -1;
	}


    /* Step 1: Walk through the macro, running controls as we go. */
    for (elt = lyst_first(ctrl_p->contents); elt; elt = lyst_next(elt))
    {
        /* Step 1.1: Grab the next ctrl...*/
        if((cur_ctrl = (mid_t *) lyst_data(elt)) == NULL)
        {
            DTNMP_DEBUG_ERR("lcc_run_ctrl","Found NULL ctrl. Exiting", NULL);
            DTNMP_DEBUG_EXIT("lcc_run_ctrl","->-1.", NULL);
            return -1;
        }

    	result = lcc_run_ctrl_mid(cur_ctrl);
    	if(result != 0)
    	{
    		DTNMP_DEBUG_WARN("lcc_run_ctrl","Error running control.", NULL);
    	}
    }

	DTNMP_DEBUG_EXIT("lcc_run_ctrl","-> %d", result);
	return result;
}
