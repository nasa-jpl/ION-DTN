/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file instr.h
 **
 **
 ** Description: DTNMP Instrumentation headers.
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


#ifndef _INSTR_H_
#define _INSTR_H_


#include "../shared/utils/nm_types.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */



/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


typedef struct {
	unsigned long num_sent_rpts;
	unsigned long num_tbrs;
	unsigned long num_tbrs_run;
	unsigned long num_sbrs;
	unsigned long num_sbrs_run;
	unsigned long num_macros_run;
	unsigned long num_ctrls_run;
} agent_instr_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


void agent_instr_init();
void agent_instr_clear();

extern agent_instr_t gAgentInstr;

#endif /* _INSTR_H_ */
