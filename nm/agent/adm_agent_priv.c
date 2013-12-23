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
 ** File Name: adm_agent_priv.c
 **
 ** Description: This implements the private aspects of a DTNMP agent ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation
 **  07/28/13  E. Birrane     Updated to new ADM design.
 *****************************************************************************/

#include "shared/adm/adm.h"

#include "adm_agent_priv.h"


/******************************************************************************
 *
 * \par Function Name: agent_adm_init_agent
 *
 * \par Initializes the collect/run functions for agent ADM support. Both the
 *      manager and agent share functions for sizing and printing items. However,
 *      only the agent needs to implement the functions for collecting data and
 *      running controls.
 *
 * \par Notes:
 *
 *  - We build a string representation of the MID rather than storing one
 *    statically to save on static space.  Please see the DTNMP AGENT ADM
 *    for specifics on the information added here.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/13  E. Birrane     Initial implementation.
 *****************************************************************************/
void agent_adm_init_agent()
{
	/* Register Nicknames */
	uint8_t mid_str[ADM_MID_ALLOC];


	/* DTNMP Agent Data */
	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 0, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_all);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 1, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_rpt_defs);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 2, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_sent_rpts);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 3, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_time_rules);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 4, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_time_rules_run);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 5, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_prod_rules);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 6, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_prod_rules_run);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 7, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_consts);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 8, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_data_defs);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 9, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_macros);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 10, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_macros_run);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 11, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_ctrls);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 12, mid_str);
	adm_add_datadef_collect(mid_str,  agent_get_num_ctrls_run);


	/* DTNMP Agent Controls */
	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_rpt_defs);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 1, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_time_rules);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 2, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_prod_rules);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 3, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_consts);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 4, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_data_defs);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 5, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_macros);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 6, mid_str);
	adm_add_ctrl_run(mid_str,  agent_list_ctrls);

}

/* Retrieval Functions. */

expr_result_t agent_get_all(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_BLOB;
	result.length = sizeof(gAgentInstr);
	result.value = (uint8_t*) MTAKE(result.length);
	memcpy(result.value, &gAgentInstr, result.length);
	return result;
}

expr_result_t agent_get_num_rpt_defs(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_rpt_defs),
			                sizeof(gAgentInstr.num_rpt_defs), &(result.length));
	return result;
}

expr_result_t agent_get_num_sent_rpts(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_sent_rpts),
	          	   sizeof(gAgentInstr.num_sent_rpts), &(result.length));
	return result;
}

expr_result_t agent_get_num_time_rules(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_time_rules),
			       sizeof(gAgentInstr.num_time_rules), &(result.length));
	return result;
}

expr_result_t agent_get_num_time_rules_run(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_time_rules_run),
			       sizeof(gAgentInstr.num_time_rules_run), &(result.length));
	return result;
}

expr_result_t agent_get_num_prod_rules(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_prod_rules),
			       sizeof(gAgentInstr.num_prod_rules), &(result.length));
	return result;
}

expr_result_t agent_get_num_prod_rules_run(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_prod_rules_run),
			       sizeof(gAgentInstr.num_prod_rules_run), &(result.length));
	return result;
}

expr_result_t agent_get_num_consts(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_consts),
			       sizeof(gAgentInstr.num_consts), &(result.length));
	return result;
}

expr_result_t agent_get_num_data_defs(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_data_defs),
			       sizeof(gAgentInstr.num_data_defs), &(result.length));
	return result;
}

expr_result_t agent_get_num_macros(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_macros),
			       sizeof(gAgentInstr.num_macros), &(result.length));
	return result;
}

expr_result_t agent_get_num_macros_run(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_macros_run),
			       sizeof(gAgentInstr.num_macros_run), &(result.length));
	return result;
}

expr_result_t agent_get_num_ctrls(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_ctrls),
			       sizeof(gAgentInstr.num_ctrls), &(result.length));
	return result;
}

expr_result_t agent_get_num_ctrls_run(Lyst params)
{
	expr_result_t result;

	result.type = EXPR_TYPE_UINT32;
	result.value = adm_copy_integer((uint8_t*)&(gAgentInstr.num_ctrls_run),
			       sizeof(gAgentInstr.num_ctrls_run), &(result.length));
	return result;
}



/* Control Functions */
/* \todo: Controls for the AGENT are not yet implemented. */
uint32_t agent_list_rpt_defs(Lyst params)
{
	return 0;
}

uint32_t agent_list_time_rules(Lyst params)
{
	return 0;
}

uint32_t agent_list_prod_rules(Lyst params)
{
	return 0;
}

uint32_t agent_list_consts(Lyst params)
{
	return 0;
}

uint32_t agent_list_data_defs(Lyst params)
{
	return 0;
}

uint32_t agent_list_macros(Lyst params)
{
	return 0;
}

uint32_t agent_list_ctrls(Lyst params)
{
	return 0;
}


uint32_t agent_remove_item(Lyst params)
{
	return 0;
}

