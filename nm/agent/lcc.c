/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: lcc.c
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
 **  05/17/15  E. Birrane     Add Macro support, updated to DTNMP v0.1 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "lcc.h"

#include "../shared/primitives/rules.h"
#include "instr.h"
#include "../shared/primitives/ctrl.h"
#include "../shared/primitives/report.h"

#include "nmagent.h"
#include "rda.h"




/******************************************************************************
 *
 * \par Function Name: lcc_run_ctrl
 *
 * \par Run a control given a control execution structure.
 *
 * \return AMP Status Code
 *
 * \param[in]  ctrl The control execution structure.
 * \param[in]  parms The parameters we should use with this CTRL.
 *
 * \par Notes:
 *   - Parms MAY be the same as in the ctrl itself, but may also be a temporary
 *     set of parameters that represent mapped parameters from an enclosing
 *     object, like a macro.
 *
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial implementation.
 *  10/04/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int lcc_run_ctrl(ctrl_t *ctrl, tnvc_t *parms)
{
	int8_t status = CTRL_FAILURE;
    tnv_t* retval = NULL;
    eid_t rx_eid;

	AMP_DEBUG_ENTRY("lcc_run_ctrl","("ADDR_FIELDSPEC")", (uaddr) ctrl);

	CHKUSR(ctrl,AMP_FAIL);


	if(strlen(ctrl->caller.name) <= 0)
	{
		rx_eid = manager_eid;
	}
	else
	{
		rx_eid = ctrl->caller;
	}


	if(ctrl->type == AMP_TYPE_CTRL)
	{
		/* Run the control. */
		retval = ctrl->def.as_ctrl->run(&rx_eid, parms, &status);
		gAgentInstr.num_ctrls_run++;
	}
	else
	{
		status = lcc_run_macro(ctrl->def.as_mac, parms);
	}

	if(status != CTRL_SUCCESS)
	{
		AMP_DEBUG_WARN("lcc_run_ctrl","Error running control.", NULL);
	}
	else if(retval != NULL)
	{
		lcc_send_retval(&rx_eid, retval, ctrl, parms);
	}

	tnv_release(retval, 1);


	AMP_DEBUG_EXIT("lcc_run_ctrl","-> %d", status);
	return status;
}


int lcc_run_macro(macdef_t *mac, tnvc_t *parms)
{
	vecit_t it;
	int result = AMP_OK;
	int success;
	CHKUSR(mac, AMP_FAIL);

	gAgentInstr.num_macros_run++;
	for(it = vecit_first(&(mac->ctrls)); vecit_valid(it); it = vecit_next(it))
	{
		ctrl_t *ctrl = (ctrl_t*) vecit_data(it);
		if(lcc_run_ctrl(ctrl, parms) != AMP_OK)
		{
			AMP_DEBUG_ERR("lcc_run_macro","Error running control %d", vecit_idx(it));
			result = AMP_FAIL;
		}
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: lcc_send_retval
 *
 * \par Sends back to a manager the return value of a function call. The
 *      return value is captured as a Data Collection (DC).
 *
 * \todo Make the rx a list of managers.
 *
 * \param[in]  rx		The Manager to receive this result.
 * \param[in]  retval	The report to send back.
 * \param[in]  ctrl		The control generating this report.
 * \param[in]  parms    The parameters used by the control
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  06/21/15  E. Birrane     Initial implementation.
 *  06/28/15  E. Birrane     Implemented TDCs
 *  10/04/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

void lcc_send_retval(eid_t *rx, tnv_t *retval, ctrl_t *ctrl, tnvc_t *parms)
{
	msg_rpt_t *msg_rpt = NULL;
	rpt_t *report = NULL;
	ari_t *ari = NULL;

	CHKVOID(rx);
	CHKVOID(retval);
	CHKVOID(ctrl);

	/* Find a message report heading to our recipient. */
	msg_rpt = rda_get_msg_rpt(*rx);
	CHKVOID(msg_rpt);

	/*
	 * Create a report whose template is the control.
	 * If the ari copy or parm replace fail, this will be caught
	 * by failing to create the report.
	 *
	 * The new parms are deep copied which is why it is OK to call
	 * ari_release at the end.
	 */
	ari_t *ctrl_ari = ctrl_get_id(ctrl);
	ari = ari_copy_ptr(ctrl_ari);
	ari_replace_parms(ari, parms);
	report = rpt_create(ari, getUTCTime(), NULL);
	ari_release(ari,1);
	CHKVOID(report);

	/* Add the single entry to this report. */
	if(rpt_add_entry(report, retval) != AMP_OK)
	{
		rpt_release(report, 1);
		AMP_DEBUG_ERR("lcc_send_retval", "Can't add retval to report.", NULL);
	}
	else if(msg_rpt_add_rpt(msg_rpt, report) != AMP_OK)
	{
		AMP_DEBUG_ERR("lcc_send_retval", "Can't add report to msg_rpt.", NULL);
		rpt_release(report, 1);
	}

}







