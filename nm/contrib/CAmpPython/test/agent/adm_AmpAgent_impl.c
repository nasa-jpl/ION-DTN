/*****************************************************************************
 **
 ** File Name: ./agent/adm_AmpAgent_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  2017-11-21  AUTO           Auto generated c file 
 *****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
/*   STOP CUSTOM INCLUDES HERE   */

#include "adm_AmpAgent_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */


/* Metadata Functions */

value_t adm_AmpAgent_meta_name(tdc_t params)
{
	return val_from_string("adm_AmpAgent");
}

value_t adm_AmpAgent_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:AmpAgent");
}


value_t adm_AmpAgent_meta_version(tdc_t params)
{
	return val_from_string("V0.2");
}


value_t adm_AmpAgent_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}



/* Collect Functions */
// This is the number of reports known to the Agent.
value_t adm_AmpAgent_get_numReports(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numReports BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numReports BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of reports sent by the agent.
value_t adm_AmpAgent_get_sentReports(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_sentReports BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_sentReports BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of time-based rules running on the agent.
value_t adm_AmpAgent_get_numTrl(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numTrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numTrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of time-based rules run by the agent since the last reset.
value_t adm_AmpAgent_get_runTrl(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_runTrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_runTrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of state-based rules running on the agent.
value_t adm_AmpAgent_get_numSrl(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numSrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numSrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of state-based rules run by the agent since the last reset.
value_t adm_AmpAgent_get_runSrl(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_runSrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_runSrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of constants known by the agent.
value_t adm_AmpAgent_get_numConst(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numConst BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numConst BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of variables known by the agent.
value_t adm_AmpAgent_get_numVar(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numVar BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numVar BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of macros known by the agent.
value_t adm_AmpAgent_get_numMacros(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numMacros BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numMacros BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of macros run by the agent since the last reset.
value_t adm_AmpAgent_get_runMacros(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_runMacros BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_runMacros BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of controls known by the agent.
value_t adm_AmpAgent_get_numCTRL(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_numCTRL BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_numCTRL BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the number of controls run by the agent since the last reset.
value_t adm_AmpAgent_get_runCTRL(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_runCTRL BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_runCTRL BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This is the current system time.
value_t adm_AmpAgent_get_curTime(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_get_curTime BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_get_curTime BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */
// This control causes the Agent to produce a report entry detailing the name of each ADM supported by the Agent.
tdc_t *adm_AmpAgent_ctrl_listADMs(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_listADMs BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_listADMs BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control configures a new variable definition on the Agent.
tdc_t *adm_AmpAgent_ctrl_addVar(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_addVar BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_addVar BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control removes one or more variable definitions from the Agent.
tdc_t *adm_AmpAgent_ctrl_delVar(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_delVar BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_delVar BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a listing of every variable identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_listVar(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_listVar BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_listVar BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a detailed description of one or more variable identifier(MID)s known to the Agent.
tdc_t *adm_AmpAgent_ctrl_descVar(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_descVar BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_descVar BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control configures a new report template definition on the Agent.
tdc_t *adm_AmpAgent_ctrl_addRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_addRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_addRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control removes one or more report template definitions from the Agent.
tdc_t *adm_AmpAgent_ctrl_delRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_delRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_delRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a listing of every report template identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_listRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_listRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_listRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a detailed description of one or more report template  identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_descRptTpl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_descRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_descRptTpl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control causes the Agent to produce a report entry for each identified report templates and send them to one or more identified managers(MIDs).
tdc_t *adm_AmpAgent_ctrl_genRpts(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_genRpts BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_genRpts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control configures a new macro definition on the Agent.
tdc_t *adm_AmpAgent_ctrl_addMacro(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_addMacro BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_addMacro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control removes one or more macro definitions from the Agent.
tdc_t *adm_AmpAgent_ctrl_delMacro(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_delMacro BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_delMacro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a listing of every macro identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_listMacro(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_listMacro BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_listMacro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a detailed description of one or more macro identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_descMacro(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_descMacro BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_descMacro BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control configures a new time-based rule(TRL) definition on the Agent.
tdc_t *adm_AmpAgent_ctrl_addTrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_addTrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_addTrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control removes one or more TRL definitions from the Agent.
tdc_t *adm_AmpAgent_ctrl_delTrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_delTrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_delTrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a listing of every TRL identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_listTrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_listTrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_listTrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a detailed description of one or more TRL identifier(MID)s known to the Agent.
tdc_t *adm_AmpAgent_ctrl_desCTRL(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_desCTRL BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_desCTRL BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control configures a new state-based rule(SRL) definition on the Agent.
tdc_t *adm_AmpAgent_ctrl_addSrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_addSrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_addSrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control removes one or more SRL definitions from the Agent.
tdc_t *adm_AmpAgent_ctrl_delSrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_delSrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_delSrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a listing of every macro identifier(MID) known to the Agent.
tdc_t *adm_AmpAgent_ctrl_listSrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_listSrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_listSrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control produces a detailed description of one or more SRL identifier(MID)s known to the Agent.
tdc_t *adm_AmpAgent_ctrl_descSrl(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_descSrl BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_descSrl BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control stores variables.
tdc_t *adm_AmpAgent_ctrl_storeVar(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_storeVar BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_storeVar BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control resets all Agent ADM statistics reported in the Agent ADM report.
tdc_t *adm_AmpAgent_ctrl_resetCounts(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_ctrl_resetCounts BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_ctrl_resetCounts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
// Int32 addition
value_t adm_AmpAgent_op_+INT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_+INT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_+INT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int32 addition
value_t adm_AmpAgent_op_+UINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_+UINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_+UINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int64 addition
value_t adm_AmpAgent_op_+VAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_+VAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_+VAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int64 addition
value_t adm_AmpAgent_op_+UVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_+UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_+UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real32 addition
value_t adm_AmpAgent_op_+REAL32(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_+REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_+REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real64 addition
value_t adm_AmpAgent_op_+REAL64(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_+REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_+REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int32 subtraction
value_t adm_AmpAgent_op_-INT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_-INT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_-INT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int32 subtraction
value_t adm_AmpAgent_op_-UINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_-UINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_-UINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int64 subtraction
value_t adm_AmpAgent_op_-VAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_-VAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_-VAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int64 subtraction
value_t adm_AmpAgent_op_-UVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_-UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_-UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real32 subtraction
value_t adm_AmpAgent_op_-REAL32(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_-REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_-REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real64 subtraction
value_t adm_AmpAgent_op_-REAL64(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_-REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_-REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int multiplication
value_t adm_AmpAgent_op_*INT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_*INT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_*INT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int32 multiplication
value_t adm_AmpAgent_op_*UINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_*UINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_*UINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int64 multiplication
value_t adm_AmpAgent_op_*VAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_*VAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_*VAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int64 multiplication
value_t adm_AmpAgent_op_*UVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_*UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_*UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real32 multiplication
value_t adm_AmpAgent_op_*REAL32(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_*REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_*REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real64 multiplication
value_t adm_AmpAgent_op_*REAL64(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_*REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_*REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int32 division
value_t adm_AmpAgent_op_/INT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_/INT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_/INT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int32 division
value_t adm_AmpAgent_op_/UINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_/UINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_/UINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int64 division
value_t adm_AmpAgent_op_/VAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_/VAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_/VAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int64 division
value_t adm_AmpAgent_op_/UVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_/UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_/UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real32 division
value_t adm_AmpAgent_op_/REAL32(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_/REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_/REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real64 division
value_t adm_AmpAgent_op_/REAL64(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_/REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_/REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int32 modulus division
value_t adm_AmpAgent_op_MODINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_MODINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_MODINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int32 modulus division
value_t adm_AmpAgent_op_MODUINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_MODUINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_MODUINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int64 modulus division
value_t adm_AmpAgent_op_MODVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_MODVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_MODVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int64 modulus division
value_t adm_AmpAgent_op_MODUVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_MODUVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_MODUVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real32 modulus division
value_t adm_AmpAgent_op_MODREAL32(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_MODREAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_MODREAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real64 modulus division
value_t adm_AmpAgent_op_MODREAL64(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_MODREAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_MODREAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int32 exponentiation
value_t adm_AmpAgent_op_^INT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_^INT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_^INT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int32 exponentiation
value_t adm_AmpAgent_op_^UINT(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_^UINT BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_^UINT BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Int64 exponentiation
value_t adm_AmpAgent_op_^VAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_^VAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_^VAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Unsigned int64 exponentiation
value_t adm_AmpAgent_op_^UVAST(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_^UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_^UVAST BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real32 exponentiation
value_t adm_AmpAgent_op_^REAL32(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_^REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_^REAL32 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Real64 exponentiation
value_t adm_AmpAgent_op_^REAL64(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_^REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_^REAL64 BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Bitwise and
value_t adm_AmpAgent_op_&(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_& BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_& BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Bitwise or
value_t adm_AmpAgent_op_|(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_| BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_| BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Bitwise xor
value_t adm_AmpAgent_op_#(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_# BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_# BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Bitwise not
value_t adm_AmpAgent_op_~(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_~ BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_~ BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Logical and
value_t adm_AmpAgent_op_&&(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_&& BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_&& BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Logical or
value_t adm_AmpAgent_op_||(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_|| BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_|| BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Logical not
value_t adm_AmpAgent_op_!(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_! BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_! BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// absolute value
value_t adm_AmpAgent_op_abs(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_abs BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_abs BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Less than
value_t adm_AmpAgent_op_<(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_< BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_< BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Greater than
value_t adm_AmpAgent_op_>(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_> BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_> BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Less than or equal to
value_t adm_AmpAgent_op_<=(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_<= BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_<= BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Greater than or equal to
value_t adm_AmpAgent_op_>=(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_>= BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_>= BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Not equal
value_t adm_AmpAgent_op_!=(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_!= BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_!= BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Equal to
value_t adm_AmpAgent_op_==(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_== BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_== BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Bitwise left shift
value_t adm_AmpAgent_op_<<(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_<< BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_<< BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Bitwise right shift
value_t adm_AmpAgent_op_>>(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_>> BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_>> BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// Store value of parm 2 in parm 1
value_t adm_AmpAgent_op_STOR(Lyst stack)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_AmpAgent_op_STOR BODY
	 * +-------------------------------------------------------------------------+
	 */	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_AmpAgent_op_STOR BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

