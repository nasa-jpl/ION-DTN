/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: nmagent.h
 **
 ** Description: This implements NM Agent main processing.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 *****************************************************************************/

#ifndef _NM_AGENT_H
#define _NM_AGENT_H
#define DEBUG 1

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"
#include "lyst.h"

// Application includes
#include "agent_db.h"

#include "../shared/utils/nm_types.h"
#include "../shared/utils/ion_if.h"

#include "../shared/primitives/mid.h"
#include "../shared/primitives/rules.h"

#include "../shared/msg/pdu.h"
#include "../shared/msg/msg_ctrl.h"
#include "../shared/msg/msg_admin.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

static const int32_t NM_RECEIVE_TIMEOUT_SEC = 1;



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


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

#if defined (ION_LWT)
int	nmagent(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10);
#else
	int	main(int argc, char *argv[]);
#endif

void agent_register();



/*
 * +--------------------------------------------------------------------------+
 * |						 GLOBAL DATA DEFINITIONS  		         		  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Indicates if the thread loops should continue to run. This
 * value is updated by the main() and read by the subordinate
 * threads.
 **/
 extern uint8_t g_running;


/**
 * The endpoint identifier (EID) of the network manager node.
 **/
extern eid_t manager_eid;

/**
 * The endpoint identifier (EID) of this agent node.
 **/
extern eid_t agent_eid;

/**
 * The interface object the ION system.
 **/
extern iif_t ion_ptr;



#endif //_NM_AGENT_H_
