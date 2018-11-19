/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: agents.h
 **
 ** Subsystem:
 **          Network Manager Application
 **
 ** Description: All Agent-related processing for a manager.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR          DESCRIPTION
 **  --------  ------------    ---------------------------------------------
 **  10/06/18  E. Birrane      Initial Implementation (JHU/APL)
 *****************************************************************************/

#ifndef AGENTS_H
#define AGENTS_H

// Standard includes

// ION includes
#include "platform.h"
#include "sdr.h"
#include "../shared/utils/nm_types.h"
#include "../shared/utils/utils.h"

#include "../shared/utils/vector.h"

#define AGENT_DEF_NUM_AGTS (4)
#define AGENT_DEF_NUM_RPTS (8)


/**
 * Data structure representing a managed remote agent.
 **/
typedef struct {
	eid_t    eid;
	vec_idx_t idx;
	vector_t rpts;
} agent_t;


int      agent_add(eid_t agent_eid);
int      agent_cb_comp(void *i1, void *i2);
void     agent_cb_del(void *item);
agent_t* agent_create(eid_t *eid);
agent_t* agent_get(eid_t* eid);
void     agent_release(agent_t *agent, int destroy);



#endif // AGENTS_H
