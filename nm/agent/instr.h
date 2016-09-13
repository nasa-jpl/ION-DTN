/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
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
	unsigned long num_rptt_defs; // done
	unsigned long num_sent_rpts; // done
	unsigned long num_trls; // done
	unsigned long num_trls_run; // done
	unsigned long num_srls;
	unsigned long num_srls_run;
	unsigned long num_lits;
	unsigned long num_vars;
	unsigned long num_macros; // done
	unsigned long num_macros_run;
	unsigned long num_ctrls; // done
	unsigned long num_ctrls_run; //done
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
