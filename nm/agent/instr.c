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


void agent_instr_init()
{
	agent_instr_clear();
}

void agent_instr_clear()
{
	memset(&gAgentInstr,0, sizeof(gAgentInstr));
}


