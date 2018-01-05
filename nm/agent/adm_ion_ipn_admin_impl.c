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
 **  2018-01-04  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "ipnfw.h"
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_ion_ipn_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_ipn_admin_setup(){

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

void adm_ion_ipn_admin_cleanup(){

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


value_t adm_ion_ipn_admin_meta_name(tdc_t params)
{
	return val_from_string("adm_ion_ipn_admin");
}


value_t adm_ion_ipn_admin_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:ion_ipn_admin");
}


value_t adm_ion_ipn_admin_meta_version(tdc_t params)
{
	return val_from_string("00");
}


value_t adm_ion_ipn_admin_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/*
 * This table lists all of the exit rules.
 */

table_t* adm_ion_ipn_admin_table_tbl_exit_rules()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "first_node_nbr", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "last_node_nbr", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "qualifier", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "gateway_endpoint_id", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_exit_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_exit_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all of the exits that are defined in the IPN database for the local node.
 */

table_t* adm_ion_ipn_admin_table_tbl_exits()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "first_node_nbr", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "last_node_nbr", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "gateway_endpoint_id", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_exits BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_exits BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all of the plan rules.
 */

table_t* adm_ion_ipn_admin_table_tbl_plan_rules()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "node_nbr", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "qualifier", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "default_duct_expression", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_plan_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_plan_rules BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * This table lists all of the egress plans that are established in the IPN database for the local node
 * .
 */

table_t* adm_ion_ipn_admin_table_tbl_plans()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "node_nbr", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "default_duct_expression", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_plans BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_table_tbl_plans BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * This is the version of ion is that currently installed.
 */
value_t adm_ion_ipn_admin_get_ion_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_get_ion_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_get_ion_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * This control establishes an "exit" for static default routing.
 */
tdc_t* adm_ion_ipn_admin_ctrl_exit_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_add BODY
	 * +-------------------------------------------------------------------------+
	 */

	int8_t success = 0;
	uvast firstNodeNbr = 0;
	uvast lastNodeNbr = 0;
	char *endpointId = NULL;

	firstNodeNbr = adm_extract_uvast(params,0,&success);
	if(success){
		lastNodeNbr = adm_extract_uvast(params,1,&success);
	}
	if(success){
		endpointId = adm_extract_string(params,2,&success);
	}
	if(success){
		ipn_addExit(firstNodeNbr,lastNodeNbr,endpointId);
		*status = CTRL_SUCCESS;
	}
	SRELEASE(firstNodeNbr);
	SRELEASE(lastNodeNbr);
	SRELEASE(endpointId);
	
	if(success){
		*status = CTRL_SUCCESS;
	}
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the gateway node number for the exit identified by firstNodeNbr and lastNodeNbr
 * .
 */
tdc_t* adm_ion_ipn_admin_ctrl_exit_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	int8_t success = 0;
	uvast firstNodeNbr = 0;
	uvast lastNodeNbr = 0;
	char *endpointId = NULL;

	firstNodeNbr = adm_extract_uvast(params,0,&success);
	if(success){
		lastNodeNbr = adm_extract_uvast(params,1,&success);
	}
	if(success){
		endpointId = adm_extract_string(params,2,&success);
	}
	if(success){
		ipn_updateExit(firstNodeNbr,lastNodeNbr,endpointId);
		*status = CTRL_SUCCESS;
	}
	SRELEASE(firstNodeNbr)
	SRELEASE(lastNodeNbr)
	SRELEASE(endpointId)
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the exit identified by firstNodeNbr and lastNodeNbr.
 */
tdc_t* adm_ion_ipn_admin_ctrl_exit_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	int8_t success = 0;
	uvast firstNodeNbr = 0;
	uvast lastNodeNbr = 0;

	firstNodeNbr = adm_extract_uvast(params,0,&success);
	if(success){
		lastNodeNbr = adm_extract_uvast(params,1,&success);
	}
	if(success){
		ipn_removeExit(firstNodeNbr,lastNodeNbr);
		*status = CTRL_SUCCESS;
	}
	SRELEASE(firstNodeNbr);
	SRELEASE(lastNodeNbr);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control adds an exit rule.
 */
tdc_t* adm_ion_ipn_admin_ctrl_exit_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes an exit rule.
 */
tdc_t* adm_ion_ipn_admin_ctrl_exit_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes an exit rule.
 */
tdc_t* adm_ion_ipn_admin_ctrl_exit_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_exit_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control establishes an egress plan for the bundles that must be transmitted to the neighboring 
 * node that is identified by it's nodeNbr.
 */
tdc_t* adm_ion_ipn_admin_ctrl_plan_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	int8_t success = 0;
	uvast nodeNbr = NULL;
	char *ductExpression = NULL;
	char *xmitRate = NULL;

	nodeNbr = adm_extract_uvast(params,0,&success);
	if(success){
		ductExpression = adm_extract_string(params,1,&success);
	}
	if(success){
		xmitRate = adm_extract_string(params,2,&success);
	}
	if(success){
		ipn_addPlan(nodeNbr,xmitRate);
		*status = CTRL_SUCCESS;
	}
	SRELEASE(nodeNbr);
	SRELEASE(ductExpression);
	SRELEASE(xmitRate);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the duct expression for the indicated plan.
 */
tdc_t* adm_ion_ipn_admin_ctrl_plan_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	int8_t success = 0;
	uvast nodeNbr = 0;
	char *ductExpression = NULL;
	char *xmitRate = NULL;

	nodeNbr = adm_extract_uvast(params,0,&success);
	if(success){
		ductExpression = adm_extract_string(params,1,&success);
	}
	if(success){
		xmitRate = adm_extract_string(params,2,&success);
	}
	if(success){
		ipn_updatePlan(nodeNbr,xmitRate);
		*status = CTRL_SUCCESS;
	}
	SRELEASE(nodeNbr);
	SRELEASE(ductExpression);
	SRELEASE(xmitRate);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes the egress plan for the node that is identified by it's nodeNbr.
 */
tdc_t* adm_ion_ipn_admin_ctrl_plan_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	int8_t success = 0;
	uvast nodeNbr = 0;

	nodeNbr = adm_extract_uvast(params,0,&success);
	if(success){
		ipn_removePlan(nodeNbr);
		*status = CTRL_SUCCESS;
	}
	SRELEASE(nodeNbr);
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control adds a plan rule.
 */
tdc_t* adm_ion_ipn_admin_ctrl_plan_rule_add(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_rule_add BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control changes the plan rule.
 */
tdc_t* adm_ion_ipn_admin_ctrl_plan_rule_change(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_rule_change BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * This control deletes a plan rule.
 */
tdc_t* adm_ion_ipn_admin_ctrl_plan_rule_del(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_ion_ipn_admin_ctrl_plan_rule_del BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
