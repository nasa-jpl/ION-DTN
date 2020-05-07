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
#define AGENT_DEF_NUM_TBLS (8)

/**
 * Data structure representing a managed remote agent.
 **/
typedef struct {
	eid_t    eid;
	vec_idx_t idx;
	vector_t rpts;
	vector_t tbls;
	
	FILE *log_fd;
	int log_fd_cnt;
	int log_file_num;
} agent_t;

/**
 * Global Configuration Settings for Automatic Logging
 */
typedef struct {
	int enabled;
	int tx_cbor; // Log transmitted controls as raw CBOR HEX strings, ie: TX: msgs:0x...
	int rx_cbor; // Log received data as raw CBOR HEX strings, ie: RX: msgs:0x...
	int rx_rpt; // Log all reports to files upon receipt
	int rx_tbl; // Log all tables tof files upon receipt
#ifdef USE_JSON // Output reports/tables in JSON format (experimental)
	int rx_json_rpt;
	int rx_json_tbl;
#endif
	int limit; // Number of entries (reports+tables) per file
	int agent_dirs; // If true, create discrete directories for each agent
	char dir[32]; // directory to save report logs to (or place sub-directories in)
	
} agent_autologging_cfg_t;
extern agent_autologging_cfg_t agent_log_cfg;

int      agent_add(eid_t agent_eid);
int      agent_cb_comp(void *i1, void *i2);
void     agent_cb_del(void *item);
agent_t* agent_create(eid_t *eid);
agent_t* agent_get(eid_t* eid);
void     agent_release(agent_t *agent, int destroy);


// File Logging function utilities
void     agent_rotate_log(agent_t *agent, int force);


#endif // AGENTS_H
