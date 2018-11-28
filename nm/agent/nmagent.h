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
 **  10/03/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef _NM_AGENT_H
#define _NM_AGENT_H

#define DEBUG 1

// Standard includes
#include "stdint.h"
#include "pthread.h"

// ION includes
#include "platform.h"

// Application includes

#include "../shared/utils/nm_types.h"
#include "../shared/msg/ion_if.h"

#include "../shared/primitives/ari.h"
#include "../shared/primitives/rules.h"

#include "../shared/msg/msg.h"

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
int	nmagent(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10);
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
 * TODO: Make this a vector and handle multiple managers.
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
