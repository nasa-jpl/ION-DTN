/****************************************************************************
 **
 ** File Name: adm_ion_ipn_admin_impl.c
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
#include "ipn/ipnfw.h"

/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_ion_ipn_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_ion_ipnadmin_setup()
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

void dtn_ion_ipnadmin_cleanup()
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


tnv_t *dtn_ion_ipnadmin_meta_name(tnvc_t *parms)
{
	return tnv_from_str("ion_ipn_admin");
}


tnv_t *dtn_ion_ipnadmin_meta_namespace(tnvc_t *parms)
{
	return tnv_from_str("DTN/ION/ipnadmin");
}


tnv_t *dtn_ion_ipnadmin_meta_version(tnvc_t *parms)
{
	return tnv_from_str("v0.0");
}


tnv_t *dtn_ion_ipnadmin_meta_organization(tnvc_t *parms)
{
	return tnv_from_str("JHUAPL");
}


/* Constant Functions */
/* Table Functions */


/*
 * This table lists all of the exits that are defined in the IPN database for the local node.
 */
tbl_t *dtn_ion_ipnadmin_tblt_exits(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_exits BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr	sdr = getIonsdr();
	Object	elt;
	OBJ_POINTER(IpnExit, exit);
	char	eidString[SDRSTRING_BUFSZ];
	tnvc_t  *cur_row = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getIpnConstants())->exits); elt;
				elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, IpnExit, exit, sdr_list_data(sdr, elt));

		sdr_string_read(getIonsdr(), eidString, exit->eid);

		/* (uint) FirstNode (UINT) last node (STR) gatewaye EID */
		if((cur_row = tnvc_create(4)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uvast(exit->firstNodeNbr));
			tnvc_insert(cur_row, tnv_from_uvast(exit->lastNodeNbr));
			tnvc_insert(cur_row, tnv_from_str(eidString));

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
	 * |STOP CUSTOM FUNCTION tblt_exits BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all of the egress plans that are established in the IPN database for the local node
 * .
 */
tbl_t *dtn_ion_ipnadmin_tblt_plans(ari_t *id)
{
	tbl_t *table = NULL;
	if((table = tbl_create(id)) == NULL)
	{
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tblt_plans BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr	sdr = getIonsdr();
	Object	elt;
	OBJ_POINTER(BpPlan, plan);
	char	*action = "none";
	char	viaEid[SDRSTRING_BUFSZ];
	char	*spec = "none";
	Object	ductElt;
	Object	outductElt;
	Outduct	outduct;
	tnvc_t  *cur_row = NULL;


	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->plans); elt;
			elt = sdr_list_next(sdr, elt))
	{
		GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, elt));
		if (plan->neighborNodeNbr == 0)	/*	Not CBHE.	*/
		{
			continue;
		}

		if (plan->viaEid)
		{
			action = "relay";
			sdr_string_read(sdr, viaEid, plan->viaEid);
			spec = viaEid;
		}
		else
		{
			action = "xmit";
			ductElt = sdr_list_first(sdr, plan->ducts);
			if (ductElt)
			{
				outductElt = sdr_list_data(sdr, ductElt);
				sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr,
						outductElt), sizeof(Outduct));
				spec = outduct.name;
			}
		}

		/* (uint) FirstNode (UINT) last node (STR) gatewaye EID */
		if((cur_row = tnvc_create(3)) != NULL)
		{
			tnvc_insert(cur_row, tnv_from_uvast(plan->neighborNodeNbr));
			tnvc_insert(cur_row, tnv_from_str(action));
			tnvc_insert(cur_row, tnv_from_str(spec));

			tbl_add_row(table, cur_row);
		}
		else
		{
			AMP_DEBUG_WARN("dtn_ion_ipnadmin_tblt_plans", "Can't allocate row. Skipping.", NULL);
		}
	}

	sdr_exit_xn(sdr);


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tblt_plans BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is the version of ion is that currently installed.
 */
tnv_t *dtn_ion_ipnadmin_get_ion_version(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_ion_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	result = tnv_from_str(IONVERSIONNUMBER);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_ion_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * This control establishes an exit for static default routing.
 */
tnv_t *dtn_ion_ipnadmin_ctrl_exit_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_exit_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success = 0;
	uvast firstNodeNbr = 0;
	uvast lastNodeNbr = 0;
	char *endpointId = NULL;

	firstNodeNbr = adm_get_parm_uvast(parms,0,&success);
	if(success){
		lastNodeNbr = adm_get_parm_uvast(parms,1,&success);
	}
	if(success){
		endpointId = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);
	}
	if(success){
		if(ipn_addExit(firstNodeNbr,lastNodeNbr,endpointId) > 0)
		{
			*status = CTRL_SUCCESS;
		}
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_exit_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the gateway node number for the exit identified by firstNodeNbr and lastNodeNbr
 * .
 */
tnv_t *dtn_ion_ipnadmin_ctrl_exit_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_exit_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success = 0;
	uvast firstNodeNbr = 0;
	uvast lastNodeNbr = 0;
	char *endpointId = NULL;

	firstNodeNbr = adm_get_parm_uvast(parms,0,&success);
	if(success){
		lastNodeNbr = adm_get_parm_uvast(parms,1,&success);
	}
	if(success){
		endpointId = adm_get_parm_obj(parms, 2, AMP_TYPE_STR);
	}
	if(success){
		if(ipn_updateExit(firstNodeNbr,lastNodeNbr,endpointId) > 0)
		{
			*status = CTRL_SUCCESS;
		}
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_exit_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the exit identified by firstNodeNbr and lastNodeNbr.
 */
tnv_t *dtn_ion_ipnadmin_ctrl_exit_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_exit_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success = 0;
	uvast firstNodeNbr = 0;
	uvast lastNodeNbr = 0;

	firstNodeNbr = adm_get_parm_uvast(parms,0,&success);
	if(success){
		lastNodeNbr = adm_get_parm_uvast(parms,1,&success);
	}
	if(success){
		if(ipn_removeExit(firstNodeNbr,lastNodeNbr) > 0)
		{
			*status = CTRL_SUCCESS;
		}
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_exit_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control establishes an egress plan for the bundles that must be transmitted to the neighboring 
 * node that is identified by it's nodeNbr.
 */
tnv_t *dtn_ion_ipnadmin_ctrl_plan_add(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_plan_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success = 0;
	uvast nodeNbr = 0;
	unsigned int xmitRate = 0;

	nodeNbr = adm_get_parm_uvast(parms,0,&success);
	if(success){
		xmitRate = adm_get_parm_uint(parms, 1, &success);
	}
	if(success){
		if(ipn_addPlan(nodeNbr,xmitRate) > 0)
		{
			*status = CTRL_SUCCESS;
		}
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_plan_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the duct expression for the indicated plan.
 */
tnv_t *dtn_ion_ipnadmin_ctrl_plan_change(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_plan_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success = 0;
	uvast nodeNbr = 0;
	unsigned int xmitRate = 0;

	nodeNbr = adm_get_parm_uvast(parms,0,&success);
	if(success){
		xmitRate = adm_get_parm_uint(parms, 1, &success);
	}
	if(success){
		if(ipn_updatePlan(nodeNbr,xmitRate) > 0)
		{
			*status = CTRL_SUCCESS;
		}
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_plan_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the egress plan for the node that is identified by it's nodeNbr.
 */
tnv_t *dtn_ion_ipnadmin_ctrl_plan_del(eid_t *def_mgr, tnvc_t *parms, int8_t *status)
{
	tnv_t *result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_plan_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	int success;
	uvast nodeNbr = 0;

	nodeNbr = adm_get_parm_uvast(parms,0,&success);
	if(success){
		if(ipn_removePlan(nodeNbr) > 0)
		{
			*status = CTRL_SUCCESS;
		}
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_plan_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
