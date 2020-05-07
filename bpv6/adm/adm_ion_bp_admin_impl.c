/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2020-04-13  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "bpP.h"
#include "csi.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_ion_bp_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_bpadmin_setup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void dtn_ion_bpadmin_cleanup()
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


tnv_t *dtn_ion_bpadmin_meta_name(tnvc_t *parms)
{
	return tnv_from_str("ion_bp_admin");
}


tnv_t *dtn_ion_bpadmin_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/ION/bpadmin");
}


tnv_t *dtn_ion_bpadmin_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.0");
}


tnv_t *dtn_ion_bpadmin_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * Local endpoints, regardless of scheme name.
 */
tbl_t *dtn_ion_bpadmin_tblt_endpoints(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	VScheme		*vscheme;
	VEndpoint	*vpoint;
	PsmAddress	scheme_elt;
	PsmAddress  vpoint_elt;
	char	recvRule;
	char	recvScriptBuffer[SDRSTRING_BUFSZ];
	char	*recvScript = recvScriptBuffer;
	tnvc_t  *cur_row = NULL;

	OBJ_POINTER(Endpoint, endpoint);
	OBJ_POINTER(Scheme, scheme);

	CHKNULL(sdr_begin_xn(sdr));
	for (scheme_elt = sm_list_first(ionwm, (getBpVdb())->schemes); scheme_elt; scheme_elt = sm_list_next(ionwm, scheme_elt))
	{
		vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, scheme_elt));

		for (vpoint_elt = sm_list_first(ionwm, vscheme->endpoints); vpoint_elt; vpoint_elt = sm_list_next(ionwm, vpoint_elt))
		{
			vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, vpoint_elt));

			GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr, vpoint->endpointElt));
			GET_OBJ_POINTER(sdr, Scheme, scheme, endpoint->scheme);

			recvRule = (endpoint->recvRule == EnqueueBundle) ? 'q' : 'x';

			if (endpoint->recvScript == 0)
			{
				recvScriptBuffer[0] = '\0';
			}
			else
			{
				if (sdr_string_read(sdr, recvScriptBuffer, endpoint->recvScript)
					       	< 0)
				{
					recvScript = "?";
				}
			}

			/* (STR) scheme_name, (STR) endpoint_nss, (UINT) app_pid, (STR) recv_rule, (STR) rcv_script */

			if((cur_row = tnvc_create(5)) != NULL)
			{
				char tmp[2];
				tmp[0] = recvRule;
				tmp[1] = 0;
				tnvc_insert(cur_row, tnv_from_str(scheme->name));
				tnvc_insert(cur_row, tnv_from_str(endpoint->nss));
				tnvc_insert(cur_row, tnv_from_uint(vpoint->appPid));
				tnvc_insert(cur_row, tnv_from_str(tmp));
				tnvc_insert(cur_row, tnv_from_str(recvScript));

				tbl_add_row(table, cur_row);
			}
			else
			{
				AMP_DEBUG_WARN("dtn_ion_bpadmin_tblt_endpoints", "Can't allocate row. Skipping.", NULL);
			}
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Inducts established locally for the indicated CL protocol.
 */
tbl_t *dtn_ion_bpadmin_tblt_inducts(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_inducts BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	ClProtocol	clpbuf;
	Object		clp_elt;
	VInduct		*vduct;
	PsmAddress	vduct_elt;
	OBJ_POINTER(Induct, duct);
	OBJ_POINTER(ClProtocol, clp);
	char	cliCmdBuffer[SDRSTRING_BUFSZ];
	char	*cliCmd;
	tnvc_t  *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));
	for (clp_elt = sdr_list_first(sdr, (getBpConstants())->protocols);
			clp_elt; clp_elt = sdr_list_next(sdr, clp_elt))
	{
		sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, clp_elt), sizeof(ClProtocol));

		for (vduct_elt = sm_list_first(ionwm, (getBpVdb())->inducts); vduct_elt;
				vduct_elt = sm_list_next(ionwm, vduct_elt))
		{
			vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, vduct_elt));

			if (strcmp(vduct->protocolName, clpbuf.name) == 0)
			{
				GET_OBJ_POINTER(sdr, Induct, duct, sdr_list_data(sdr, vduct->inductElt));
				GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);
				if (sdr_string_read(sdr, cliCmdBuffer, duct->cliCmd) < 0)
				{
					cliCmd = "?";
				}
				else
				{
					cliCmd = cliCmdBuffer;
				}

				/* (STR) protocol_name, (STR) duct_name, (STR) cli_control */
				if((cur_row = tnvc_create(3)) != NULL)
				{
					tnvc_insert(cur_row, tnv_from_str(clp->name));
					tnvc_insert(cur_row, tnv_from_str(duct->name));
					tnvc_insert(cur_row, tnv_from_str(cliCmd));

					tbl_add_row(table, cur_row);
				}
				else
				{
					AMP_DEBUG_WARN("dtn_ion_bpadmin_tblt_inducts", "Can't allocate row. Skipping.", NULL);
				}

			}
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_inducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * If protocolName is specified, this table lists all outducts established locally for the indicated CL
 *  protocol. Otherwise, it lists all locally established outducts, regardless of their protocol.
 */
tbl_t *dtn_ion_bpadmin_tblt_outducts(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_outducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	ClProtocol	clpbuf;
	Object		clp_elt;
	VOutduct	*vduct;
	PsmAddress	vduct_elt;
	OBJ_POINTER(Outduct, duct);
	OBJ_POINTER(ClProtocol, clp);
	char	cloCmdBuffer[SDRSTRING_BUFSZ];
	char	*cloCmd;
	tnvc_t  *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));
	for (clp_elt = sdr_list_first(sdr, (getBpConstants())->protocols);
			clp_elt; clp_elt = sdr_list_next(sdr, clp_elt))
	{
		sdr_read(sdr, (char *) &clpbuf, sdr_list_data(sdr, clp_elt), sizeof(ClProtocol));

		for (vduct_elt = sm_list_first(ionwm, (getBpVdb())->outducts); vduct_elt;
				vduct_elt = sm_list_next(ionwm, vduct_elt))
		{
			vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, vduct_elt));

			if (strcmp(vduct->protocolName, clpbuf.name) == 0)
			{
				GET_OBJ_POINTER(sdr, Outduct, duct, sdr_list_data(sdr, vduct->outductElt));
				GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);

				if (duct->cloCmd == 0)
				{
					cloCmd = "?";
				}
				else if (sdr_string_read(sdr, cloCmdBuffer, duct->cloCmd) < 0)
				{
					cloCmd = "?";
				}
				else
				{
					cloCmd = cloCmdBuffer;
				}

				/* (STR) protocol_name, (STR) duct_name, (UINT) clo_pid, (STR) clo_control, (STR) max_ayload_len */
				if((cur_row = tnvc_create(5)) != NULL)
				{
					tnvc_insert(cur_row, tnv_from_str(clp->name));
					tnvc_insert(cur_row, tnv_from_str(duct->name));
					tnvc_insert(cur_row, tnv_from_uint(vduct->cloPid));
					tnvc_insert(cur_row, tnv_from_str(cloCmd));
					tnvc_insert(cur_row, tnv_from_uint(duct->maxPayloadLen));

					tbl_add_row(table, cur_row);
				}
				else
				{
					AMP_DEBUG_WARN("dtn_ion_bpadmin_tblt_outducts", "Can't allocate row. Skipping.", NULL);
				}

			}
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_outducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Convergence layer protocols that can currently be utilized at the local node.
 */
tbl_t *dtn_ion_bpadmin_tblt_protocols(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_protocols BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr	sdr = getIonsdr();
	Object	elt;
	OBJ_POINTER(ClProtocol, clp);
	tnvc_t  *cur_row = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, ClProtocol, clp, sdr_list_data(sdr, elt));

		/* (STR) name, (UINT) payload_bpf, (UINT) overhead_bpf, (UINT) nominal_data_rate */
		if((cur_row = tnvc_create(4)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_str(clp->name));
			tnvc_insert(cur_row, tnv_from_uint(clp->payloadBytesPerFrame));
			tnvc_insert(cur_row, tnv_from_uint(clp->overheadPerFrame));
			tnvc_insert(cur_row, tnv_from_uint(clp->protocolClass));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_bpadmin_tblt_protocols", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_protocols BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Declared endpoint naming schemes.
 */
tbl_t *dtn_ion_bpadmin_tblt_schemes(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_schemes BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VScheme		*vscheme;
	OBJ_POINTER(Scheme, scheme);
	char	fwdCmdBuffer[SDRSTRING_BUFSZ];
	char	*fwdCmd;
	char	admAppCmdBuffer[SDRSTRING_BUFSZ];
	char	*admAppCmd;
	tnvc_t  *cur_row = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, elt));

		GET_OBJ_POINTER(sdr, Scheme, scheme, sdr_list_data(sdr, vscheme->schemeElt));
		if (sdr_string_read(sdr, fwdCmdBuffer, scheme->fwdCmd) < 0)
		{
			fwdCmd = "?";
		}
		else
		{
			fwdCmd = fwdCmdBuffer;
		}

		if (sdr_string_read(sdr, admAppCmdBuffer, scheme->admAppCmd) < 0)
		{
			admAppCmd = "?";
		}
		else
		{
			admAppCmd = admAppCmdBuffer;
		}

		/* (STR) name, (UINT) fwd_pid, (STR) fwd_cmd, (UINT) admin_app_pid (STR) admin_app_cmd */
		if((cur_row = tnvc_create(5)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_str(scheme->name));
			tnvc_insert(cur_row, tnv_from_uint(vscheme->fwdPid));
			tnvc_insert(cur_row, tnv_from_str(fwdCmd));
			tnvc_insert(cur_row, tnv_from_uint(vscheme->admAppPid));
			tnvc_insert(cur_row, tnv_from_str(admAppCmd));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_bpadmin_tblt_schemess", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_schemes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Egress plans.
 */
tbl_t *dtn_ion_bpadmin_tblt_egress_plans(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_egress_plans BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr		sdr = getIonsdr();
	PsmPartition	ionwm = getIonwm();
	PsmAddress	elt;
	VPlan		*vplan;
	OBJ_POINTER(BpPlan, plan);
	tnvc_t  *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, (getBpVdb())->plans); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vplan = (VPlan *) psp(ionwm, sm_list_data(ionwm, elt));
		GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, vplan->planElt));

		/* (STR) neighbor EID, (UINT) clm_pid, (UINT) nominal rate*/
		if((cur_row = tnvc_create(3)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_str(plan->neighborEid));
			tnvc_insert(cur_row, tnv_from_uint(vplan->clmPid));
			tnvc_insert(cur_row, tnv_from_uint(plan->nominalRate));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_bpadmin_tblt_egress_plans", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_egress_plans BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * Version of installed ION BP Admin utility.
 */
tnv_t *dtn_ion_bpadmin_get_bp_version(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_bp_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	char buffer[80];
	isprintf(buffer, sizeof buffer,
			"%s compiled with crypto suite: %s",
			IONVERSIONNUMBER, CSI_SUITE_NAME);
	result = tnv_from_str(buffer);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_bp_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * Establish DTN endpoint named endpointId on the local node. The remaining parameters indicate what is
 *  to be done when bundles destined for this endpoint arrive at a time when no application has the end
 * point open for bundle reception. If type is 'x', then such bundles are to be discarded silently and 
 * immediately. If type is 'q', then such bundles are to be enqueued for later delivery and, if recvScr
 * ipt is provided, recvScript is to be executed.
 */
tnv_t *dtn_ion_bpadmin_ctrl_endpoint_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_endpoint_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success;
	BpRecvRule	rule;

	char *id = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
    uint32_t type = adm_get_parm_uint(parms, 1, &success);
    char *rcv = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);

    rule = (type == 'q') ? EnqueueBundle : DiscardBundle;

	if(addEndpoint(id, rule, rcv) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_endpoint_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Change the action taken when bundles destined for this endpoint arrive at a time when no application
 *  has the endpoint open for bundle reception.
 */
tnv_t *dtn_ion_bpadmin_ctrl_endpoint_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_endpoint_change BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	BpRecvRule	rule;

	char *id = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
    uint32_t type = adm_get_parm_uint(parms, 1, &success);
    char *rcv = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);

    rule = (type == 'q') ? EnqueueBundle : DiscardBundle;

	if(updateEndpoint(id, rule, rcv) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_endpoint_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the endpoint identified by endpointId. The control will fail if any bundles are currently pen
 * ding delivery to this endpoint.
 */
tnv_t *dtn_ion_bpadmin_ctrl_endpoint_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_endpoint_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *id = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(removeEndpoint(id) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_endpoint_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Establish a duct for reception of bundles via the indicated CL protocol. The duct's data acquisition
 *  structure is used and populated by the induct task whose operation is initiated by cliControl at th
 * e time the duct is started.
 */
tnv_t *dtn_ion_bpadmin_ctrl_induct_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_induct_add BODY
	 * +-------------------------------------------------------------------------+
	 */


	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	char *cli_ctrl = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);

	if(addInduct(p_name, d_name, cli_ctrl) > 1)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_induct_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Change the control used to initiate operation of the induct task for the indicated duct.
 */
tnv_t *dtn_ion_bpadmin_ctrl_induct_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_induct_change BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	char *cli_ctrl = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);

	if(updateInduct(p_name, d_name, cli_ctrl) > 1)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_induct_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the induct identified by protocolName and ductName. The control will fail if any bundles are 
 * currently pending acquisition via this induct.
 */
tnv_t *dtn_ion_bpadmin_ctrl_induct_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_induct_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	if(removeInduct(p_name, d_name) > 1)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_induct_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start the indicated induct task as defined for the indicated CL protocol on the local node.
 */
tnv_t *dtn_ion_bpadmin_ctrl_induct_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_induct_start BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	if(bpStartInduct(p_name, d_name) > 1)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_induct_start BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop the indicated induct task as defined for the indicated CL protocol on the local node.
 */
tnv_t *dtn_ion_bpadmin_ctrl_induct_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_induct_stop BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	bpStopInduct(p_name, d_name);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_induct_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Declare the maximum number of bytes of SDR heap space that will be occupied by any single bundle acq
 * uisition activity (nominally the acquisition of a single bundle, but this is at the discretion of th
 * e convergence-layer input task). All data acquired in excess of this limit will be written to a temp
 * orary file pending extraction and dispatching of the acquired bundle or bundles. The default is the 
 * minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; thi
 * s is the minimum SDR heap space occupancy in the event that all acquisition is into a file.
 */
tnv_t *dtn_ion_bpadmin_ctrl_manage_heap_max(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manage_heap_max BODY
	 * +-------------------------------------------------------------------------+
	 */
	Sdr		sdr = getIonsdr();
	Object		bpdbObj = getBpDbObject();
	BpDB		bpdb;
	int success;
	unsigned int heapmax = adm_get_parm_uint(parms, 0, &success);

	if (heapmax < 560)
	{
		AMP_DEBUG_WARN("dtn_ion_bpadmin_ctrl_manage_heap_max", "heapmax (%lu) must be at least 560.", heapmax);
		return NULL;
	}

	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.maxAcqInHeap = heapmax;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	if (sdr_end_xn(sdr) < 0)
	{
		AMP_DEBUG_ERR("dtn_ion_bpadmin_ctrl_manage_heap_max", "Can't change maxAcqInHeap.", NULL);
	}
	else
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manage_heap_max BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Establish a duct for transmission of bundles via the indicated CL protocol. the duct's data transmis
 * sion structure is serviced by the outduct task whose operation is initiated by CLOcommand at the tim
 * e the duct is started. A value of zero for maxPayloadLength indicates that bundles of any size can b
 * e accomodated; this is the default.
 */
tnv_t *dtn_ion_bpadmin_ctrl_outduct_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outduct_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success;
	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	char *clo_cmd = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);
	unsigned int max_len = adm_get_parm_uint(parms, 3, &success);

	if(addOutduct(p_name, d_name, clo_cmd, max_len) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outduct_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Set new values for the indicated duct's payload size limit and the control that is used to initiate 
 * operation of the outduct task for this duct.
 */
tnv_t *dtn_ion_bpadmin_ctrl_outduct_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outduct_change BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	char *clo_cmd = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);
	unsigned int max_len = adm_get_parm_uint(parms, 3, &success);

	if(updateOutduct(p_name, d_name, clo_cmd, max_len) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outduct_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the outduct identified by protocolName and ductName. The control will fail if any bundles are
 *  currently pending transmission via this outduct.
 */
tnv_t *dtn_ion_bpadmin_ctrl_outduct_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outduct_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	if(removeOutduct(p_name, d_name) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outduct_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start the indicated outduct task as defined for the indicated CL protocol on the local node.
 */
tnv_t *dtn_ion_bpadmin_ctrl_outduct_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outduct_start BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	if(bpStartOutduct(p_name, d_name) > 0)
	{
		*status = CTRL_SUCCESS;
	}


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outduct_start BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Disable transmission of bundles queued for transmission to the indicated node and reforwards all non
 * -critical bundles currently queued for transmission to this node. This may result in some or all of 
 * these bundles being enqueued for transmission to the psuedo-node limbo.
 */
tnv_t *dtn_ion_bpadmin_ctrl_egress_plan_block(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_egress_plan_block BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(bpBlockPlan(name) >= 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_egress_plan_block BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Re-enable transmission of bundles to the indicated node and reforwards all bundles in limbo in the h
 * ope that the unblocking of this egress plan will enable some of them to be transmitted.
 */
tnv_t *dtn_ion_bpadmin_ctrl_egress_plan_unblock(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_egress_plan_unblock BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(bpUnblockPlan(name) >= 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_egress_plan_unblock BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop the indicated outduct task as defined for the indicated CL protocol on the local node.
 */
tnv_t *dtn_ion_bpadmin_ctrl_outduct_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outduct_stop BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *p_name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *d_name = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);

	bpStopOutduct(p_name, d_name);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outduct_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Establish access to the named convergence layer protocol at the local node. The payloadBytesPerFrame
 *  and overheadBytesPerFrame arguments are used in calculating the estimated transmission capacity con
 * sumption of each bundle, to aid in route computation and congesting forecasting. The optional nomina
 * lDataRate argument overrides the hard coded default continuous data rate for the indicated protocol 
 * for purposes of rate control. For all promiscuous prototocols-that is, protocols whose outducts are 
 * not specifically dedicated to transmission to a single identified convergence-layer protocol endpoin
 * t- the protocol's applicable nominal continuous data rate is the data rate that is always used for r
 * ate control over links served by that protocol; data rates are not extracted from contact graph info
 * rmation. This is because only the induct and outduct throttles for non-promiscuous protocols (LTP, T
 * CP) can be dynamically adjusted in response to changes in data rate between the local node and its n
 * eighbors, as enacted per the contact plan. Even for an outduct of a non-promiscuous protocol the nom
 * inal data rate may be the authority for rate control, in the event that the contact plan lacks ident
 * ified contacts with the node to which the outduct is mapped.
 */
tnv_t *dtn_ion_bpadmin_ctrl_protocol_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocol_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	int payloadPerFrame = adm_get_parm_uint(parms, 1, &success);
	int ohdPerFrame = adm_get_parm_uint(parms, 2, &success);
	int protocolClass = adm_get_parm_uint(parms, 3, &success);

	if(addProtocol(name, payloadPerFrame, ohdPerFrame, protocolClass) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocol_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the convergence layer protocol identified by protocolName. The control will fail if any ducts
 *  are still locally declared for this protocol.
 */
tnv_t *dtn_ion_bpadmin_ctrl_protocol_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocol_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(removeProtocol(name) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocol_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start all induct and outduct tasks for inducts and outducts that have been defined for the indicated
 *  CL protocol on the local node.
 */
tnv_t *dtn_ion_bpadmin_ctrl_protocol_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocol_start BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(bpStartProtocol(name) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocol_start BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop all induct and outduct tasks for inducts and outducts that have been defined for the indicated 
 * CL protocol on the local node.
 */
tnv_t *dtn_ion_bpadmin_ctrl_protocol_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocol_stop BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	bpStopProtocol(name);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocol_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Declares an endpoint naming scheme for use in endpoint IDs, which are structured as URIs: schemeName
 * :schemeSpecificPart. forwarderControl will be executed when the scheme is started on this node, to i
 * nitiate operation of a forwarding daemon for this scheme. adminAppControl will also be executed when
 *  the scheme is started on this node, to initiate operation of a daemon that opens a custodian endpoi
 * nt identified within this scheme so that it can recieve and process custody signals and bundle statu
 * s reports.
 */
tnv_t *dtn_ion_bpadmin_ctrl_scheme_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_scheme_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *fwd_ctrl = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	char *adm_app_ctrl = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);

	if(addScheme(name, fwd_ctrl, adm_app_ctrl) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_scheme_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Set the indicated scheme's forwarderControl and adminAppControl to the strings provided as arguments
 * .
 */
tnv_t *dtn_ion_bpadmin_ctrl_scheme_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_scheme_change BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	char *fwd_ctrl = adm_get_parm_obj(parms, 1, AMP_TYPE_STR);
	char *adm_app_ctrl = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);

	if(updateScheme(name, fwd_ctrl, adm_app_ctrl) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_scheme_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the scheme identified by schemeName. The control will fail if any bundles identified in this 
 * scheme are pending forwarding, transmission, or delivery.
 */
tnv_t *dtn_ion_bpadmin_ctrl_scheme_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_scheme_del BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(removeScheme(name) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_scheme_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start the forwarder and administrative endpoint tasks for the indicated scheme task on the local nod
 * e.
 */
tnv_t *dtn_ion_bpadmin_ctrl_scheme_start(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_scheme_start BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	if(bpStartScheme(name) > 0)
	{
		*status = CTRL_SUCCESS;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_scheme_start BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop the forwarder and administrative endpoint tasks for the indicated scheme task on the local node
 * .
 */
tnv_t *dtn_ion_bpadmin_ctrl_scheme_stop(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_scheme_stop BODY
	 * +-------------------------------------------------------------------------+
	 */

	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);

	bpStopScheme(name);
	*status = CTRL_SUCCESS;

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_scheme_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Enable/Disable production of a continuous stream of user selected Bundle Protocol activity indicatio
 * n characters. A watch parameter of 1 selects all BP activity indication characters, 0 deselects allB
 * P activity indication characters; any other activitySpec such as acz~ selects all activity indicatio
 * n characters in the string, deselecting all others. BP will print each selected activity indication 
 * character to stdout every time a processing event of the associated type occurs: a new bundle is que
 * ued for forwarding, b bundle is queued for transmission, c bundle is popped from its transmission qu
 * eue, m custody acceptance signal is recieved, w custody of bundle is accepted, x custody of bundle i
 * s refused, y bundle is accepted upon arrival, z bundle is queued for delivery to an application, ~ b
 * undle is abandoned (discarded) on attempt to forward it, ! bundle is destroyed due to TTL expiration
 * , &amp; custody refusal signal is recieved, # bundle is queued for re-forwarding due to CL protocol 
 * failures, j bundle is placed in 'limbo' for possible future reforwarding, k bundle is removed from '
 * limbo' and queued for reforwarding, $ bundle's custodial retransmission timeout interval expired.
 */
tnv_t *dtn_ion_bpadmin_ctrl_watch(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_watch BODY
	 * +-------------------------------------------------------------------------+
	 */

	int success;
	Sdr	sdr = getIonsdr();
	Object	dbObj = getBpDbObject();
	BpDB	db;
	char *name = adm_get_parm_obj(parms, 0, AMP_TYPE_STR);
	BpVdb	*vdb = getBpVdb();
	int i;

	CHKNULL(vdb);
	*status = CTRL_SUCCESS;

	if (strcmp(name, "1") == 0)
	{
		vdb->watching = -1;
		return result;
	}

	vdb->watching = 0;
	if (strcmp(name, "0") == 0)
	{
		return result;
	}

	for(i = 0; i < strlen(name); i++)
	{
		switch(name[i])
		{
			case 'a': vdb->watching |= WATCH_a; break;
			case 'b': vdb->watching |= WATCH_b; break;
			case 'c': vdb->watching |= WATCH_c; break;
			case 'm': vdb->watching |= WATCH_m; break;
			case 'w': vdb->watching |= WATCH_w; break;
			case 'x': vdb->watching |= WATCH_x; break;
			case 'y': vdb->watching |= WATCH_y; break;
			case 'z': vdb->watching |= WATCH_z; break;
			case '~': vdb->watching |= WATCH_abandon; break;
			case '!': vdb->watching |= WATCH_expire;  break;
			case '&': vdb->watching |= WATCH_refusal; break;
			case '#': vdb->watching |= WATCH_timeout; break;
			case 'j': vdb->watching |= WATCH_limbo;   break;
			case 'k': vdb->watching |= WATCH_delimbo; break;
			default:
				AMP_DEBUG_WARN("dtn_ion_bpadmin_ctrl_watch", "Invalid watch char %c.", name[i]);
				break;
		}
	}


	if (dbObj != 0)
	{
		CHKNULL(sdr_begin_xn(sdr));
		sdr_stage(sdr, (char *) &db, dbObj, sizeof(BpDB));
		db.watching = vdb->watching;
		sdr_write(sdr, dbObj, (char *) &db, sizeof(BpDB));
		oK(sdr_end_xn(sdr));
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_watch BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
