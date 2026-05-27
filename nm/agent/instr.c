/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file instr.c
 **
 **
 ** Description: DTNMP Instrumentation functions.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  07/04/13  E. Birrane     Initial Implementation (JHU/APL)
 *****************************************************************************/

#include "instr.h"

#include <string.h>

agent_instr_t gAgentInstr;


void agent_instr_init(void)
{
	agent_instr_clear();
}

void agent_instr_clear(void)
{
	ion_atomic_init(&gAgentInstr.num_sent_rpts, 0);
	ion_atomic_init(&gAgentInstr.num_sent_tbls, 0);
	ion_atomic_init(&gAgentInstr.num_tbrs, 0);
	ion_atomic_init(&gAgentInstr.num_tbrs_run, 0);
	ion_atomic_init(&gAgentInstr.num_sbrs, 0);
	ion_atomic_init(&gAgentInstr.num_sbrs_run, 0);
	ion_atomic_init(&gAgentInstr.num_macros_run, 0);
	ion_atomic_init(&gAgentInstr.num_ctrls_run, 0);
}
