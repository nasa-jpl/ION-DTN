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
 **  11/23/21  E. Birrane     Added counting of sent table sets (JHU/APL)
 *****************************************************************************/


#ifndef INSTR_H
#define INSTR_H


#include "../shared/utils/nm_types.h"


#ifdef __cplusplus
extern "C" {
#endif

#include "ion_atomic.h"

/*
 * +--------------------------------------------------------------------------+
 * |                              CONSTANTS                                   +
 * +--------------------------------------------------------------------------+
 */



/*
 * +--------------------------------------------------------------------------+
 * |                                MACROS                                    +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |                              DATA TYPES                                  +
 * +--------------------------------------------------------------------------+
 */


typedef struct {
	ion_atomic_t num_sent_rpts;
	ion_atomic_t num_sent_tbls;
	ion_atomic_t num_tbrs;
	ion_atomic_t num_tbrs_run;
	ion_atomic_t num_sbrs;
	ion_atomic_t num_sbrs_run;
	ion_atomic_t num_macros_run;
	ion_atomic_t num_ctrls_run;
} agent_instr_t;



/*
 * +--------------------------------------------------------------------------+
 * |                          FUNCTION PROTOTYPES                             +
 * +--------------------------------------------------------------------------+
 */


void agent_instr_init(void);
void agent_instr_clear(void);

extern agent_instr_t gAgentInstr;


#ifdef __cplusplus
}
#endif

#endif /* INSTR_H */
