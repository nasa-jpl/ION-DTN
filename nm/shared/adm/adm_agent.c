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
 ** File Name: adm_agent_public.c
 **
 ** Description: This implements the public aspects of a DTNMP agent ADM.
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
 *****************************************************************************/

#include "ion.h"
#include "platform.h"


#include "shared/adm/adm_agent.h"
#include "shared/utils/utils.h"
#include "shared/primitives/instr.h"


void adm_agent_init()
{
	/* Register Nicknames */
	uint8_t mid_str[ADM_MID_ALLOC];


	/* DTNMP Agent Data */
	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 0, mid_str);
	adm_add_datadef("DTNMP_AGENT_DATA",               mid_str, 0, adm_print_agent_all,     adm_size_agent_all);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 1, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_RPT_DEFS",       mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 2, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_SENT_RPTS",      mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 3, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_TIME_RULES",     mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 4, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_TIME_RULES_RUN", mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 5, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_PROD_RULES",     mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 6, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_PROD_RULES_RUN", mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 7, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_CONSTS",         mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 8, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_DATA_DEFS",      mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 9, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_MACROS",         mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 10, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_MACROS_RUN",     mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 11, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_CTRLS",          mid_str, 0, NULL, NULL);

	adm_build_mid_str(0x00, ADM_AGENT_NODE_NN, ADM_AGENT_NODE_NN_LEN, 12, mid_str);
	adm_add_datadef("DTNMP_AGENT_NUM_CTRLS_RUN",      mid_str, 0, NULL, NULL);


	/* DTNMP Agent Controls */
	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 0, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_RPT_DEFS",  mid_str, 0);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 1, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_TIME_RULES",mid_str, 0);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 2, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_PROD_RULES",mid_str, 0);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 3, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_CONSTS",    mid_str, 0);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 4, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_DATA_DEFS", mid_str, 0);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 5, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_MACROS",    mid_str, 0);

	adm_build_mid_str(0x01, ADM_AGENT_CTRL_NN, ADM_AGENT_CTRL_NN_LEN, 6, mid_str);
	adm_add_ctrl("DTNMP_AGENT_LIST_CTRLS",     mid_str, 0);

	/* DTNMP Agent Literals */
	/* \todo: Add Literals */

	/* DTNMP Agent Operators */
	/* \todo Add Operators */
}


/* Custom Print Functions. */
char *adm_print_agent_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	agent_instr_t state;

	char *result;
	uint32_t temp_size = 0;

	// \todo: Check sizes.
	memcpy(&state, buffer, data_len);

	// Assume for now a 4 byte integer takes <= 20 characters.
	// Assume all the text strings average less than 25 characters per string.
	temp_size = 12 * sizeof(unsigned long);
	*str_len = (temp_size * 5) + (25 * 100);

	// Assume for now a 4 byte integer takes <= 20 characters to print.
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_print_agent_all","Can't allocate %d bytes", *str_len);
		*str_len = 0;
		return NULL;
	}

	memset(result, '\0', *str_len);

	sprintf(result,
			"\num_rpt_defs = %ld\nnum_sent_rpts = %ld\nnum_time_rules = %ld\n \
num_time_rules_run = %ld\nnum_prod_rules = %ld\nnum_prod_rules_run = %ld\n \
num_consts = %ld\nnum_data_defs = %ld\nnum_macros = %ld\nnum_macros_run = %ld\n \
num_ctrls = %ld\nnum_ctrls_run = %ld\n",
		    state.num_rpt_defs,
		    state.num_sent_rpts,
		    state.num_time_rules,
			state.num_time_rules_run,
			state.num_prod_rules,
			state.num_prod_rules_run,
			state.num_consts,
			state.num_data_defs,
			state.num_macros,
			state.num_macros_run,
			state.num_ctrls,
			state.num_ctrls_run);

	return result;
}



/* SIZE */

uint32_t adm_size_agent_all(uint8_t* buffer, uint64_t buffer_len)
{
	return sizeof(gAgentInstr);
}



