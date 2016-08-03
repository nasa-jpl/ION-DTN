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
 **  07/04/13  E. Birrane     Initial Implementation
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


