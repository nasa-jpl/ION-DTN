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
 **  07/04/13  E. Birrane     Initial Implementation
 *****************************************************************************/


#ifndef _INSTR_H_
#define _INSTR_H_


#include "shared/utils/nm_types.h"

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
	unsigned long num_rpt_defs; // done
	unsigned long num_sent_rpts; // done
	unsigned long num_time_rules; // done
	unsigned long num_time_rules_run; // done
	unsigned long num_prod_rules;
	unsigned long num_prod_rules_run;
	unsigned long num_consts;
	unsigned long num_data_defs;
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
