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

// Application headers.
#include "agents.h"


#include "../shared/utils/debug.h"
#include "nm_mgr.h"

agent_autologging_cfg_t agent_log_cfg = {
 // Defaults (nominal, disabled on startup)
	0, // Disabled by default
	0, // Log CBOR Hex on transmit
	0, // Log CBOR Hex on receipt
	0, // Log Parsed Report on receipt
	0, // Log Parsed tables on Receipt
	50, // Number of reports per file before rotation
	100, // Max rotated log files to retain per agent (0 = unlimited)
	0, // Create discrete sub-folders per agent
	"." // root log directory will be the working directory mgr started from as default
};


/******************************************************************************
 *
 * \par Function Name: agent_add
 *
 * \par Add an agent to the manager list of known agents.
 *
 * \return  AMP Status Code.
 *
 * \param[in] id  - The endpoint identifier for the new agent.
 *
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 **  08/29/15  E. Birrane      Don't print error message on duplicate agent
 **  10/06/18  E. Birrane      Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

static int  agent_next_log_num(agent_t *agent);
static void agent_prune_logs(agent_t *agent, int newest);

int agent_add(eid_t id)
{
	agent_t *agent = NULL;

	AMP_DEBUG_ENTRY("agent_add","(%s)", id.name);
	printf("agent_add(%s)\n", id.name);


	/* Check if the agent is already known. */
	if((agent = agent_get(&id)) != NULL)
	{
		AMP_DEBUG_WARN("agent_add","Agent already added: %s", id.name);
		return AMP_OK;
	}

	if((agent = agent_create(&id)) == NULL)
	{
		AMP_DEBUG_ERR("agent_add","Can't create new agent.", NULL);
		return AMP_SYSERR;
	}

	if((vec_insert(&(gMgrDB.agents), agent, &(agent->idx))) != VEC_OK)
	{
		AMP_DEBUG_ERR("agent_add", "Can't insert new agent.", NULL);
		agent_release(agent, 1);
		return AMP_FAIL;
	}

	if (agent_log_cfg.enabled)
	{
		/*	Resume numbering after any files left by a prior run
		 *	so we open a fresh file rather than appending to an
		 *	existing one.					*/
		agent->log_file_num = agent_next_log_num(agent);
	}

	agent_rotate_log(agent, 1);

	return AMP_OK;
}

/*	Builds the directory holding this agent's log files and the filename
 *	prefix preceding the "<n>.log" suffix.  In per-agent-directory mode the
 *	files live in <dir>/<eid>/ as "<n>.log" (empty prefix); otherwise they
 *	share <dir> as "<eid>_<n>.log".						*/
static void agent_log_dir(agent_t *agent, char *dirpath, size_t dlen,
		char *prefix, size_t plen)
{
	if (agent_log_cfg.agent_dirs)
	{
		snprintf(dirpath, dlen, "%s/%s", agent_log_cfg.dir,
				agent->eid.name);
		prefix[0] = '\0';
	}
	else
	{
		snprintf(dirpath, dlen, "%s", agent_log_cfg.dir);
		snprintf(prefix, plen, "%s_", agent->eid.name);
	}
}

/*	Returns the rotation number encoded in a "<prefix><n>.log" filename,
 *	or -1 if the name is not one of this agent's log files.			*/
static int agent_log_num(const char *name, const char *prefix)
{
	size_t      plen = strlen(prefix);
	const char *p = name;
	char       *end;
	long        n;

	if (strncmp(p, prefix, plen) != 0)
	{
		return -1;
	}

	p += plen;
	if (*p < '0' || *p > '9')
	{
		return -1;
	}

	n = strtol(p, &end, 10);
	if (strcmp(end, ".log") != 0)
	{
		return -1;	/*	Not an "<n>.log" file.			*/
	}

	return (int) n;
}

/*	Scans the log directory for this agent's existing files and returns the
 *	next file number to use (highest found + 1, or 0 if none).  This lets a
 *	restarted Manager resume the rotation sequence.				*/
static int agent_next_log_num(agent_t *agent)
{
	char           dirpath[128];
	char           prefix[64];
	DIR           *dir;
	struct dirent *entry;
	int            highest = -1;

	agent_log_dir(agent, dirpath, sizeof(dirpath), prefix, sizeof(prefix));

	dir = opendir(dirpath);
	if (dir == NULL)
	{
		return 0;	/*	No prior files yet.			*/
	}

	while ((entry = readdir(dir)) != NULL)
	{
		int n = agent_log_num(entry->d_name, prefix);

		if (n > highest)
		{
			highest = n;
		}
	}

	closedir(dir);
	return highest + 1;
}

/*	Enforces retention: keeps at most max_files of this agent's log files by
 *	deleting any whose rotation number is more than max_files behind the one
 *	just created (`newest`).  						*/
static void agent_prune_logs(agent_t *agent, int newest)
{
	char           dirpath[128];
	char           prefix[64];
	char           fn[512];
	int            oldest_kept;
	DIR           *dir;
	struct dirent *entry;

	if (agent_log_cfg.max_files <= 0)
	{
		return;		/*	Unlimited.				*/
	}

	oldest_kept = newest - agent_log_cfg.max_files + 1;
	if (oldest_kept <= 0)
	{
		return;		/*	Nothing old enough to drop yet.		*/
	}

	agent_log_dir(agent, dirpath, sizeof(dirpath), prefix, sizeof(prefix));

	dir = opendir(dirpath);
	if (dir == NULL)
	{
		return;
	}

	while ((entry = readdir(dir)) != NULL)
	{
		int n = agent_log_num(entry->d_name, prefix);

		if (n < 0 || n >= oldest_kept)
		{
			continue;	/*	Not a log file, or still kept.	*/
		}

		snprintf(fn, sizeof(fn), "%s/%s", dirpath, entry->d_name);
		if (unlink(fn) != 0 && errno != ENOENT)
		{
			AMP_DEBUG_WARN("agent_prune_logs",
				"Failed to prune old log file (%s) for agent %s",
				fn, agent->eid.name);
		}
	}

	closedir(dir);
}

void agent_rotate_log(agent_t *agent, int force)
{
	char fn[128];
	char agent_autologging_sep = '_';
	lockResource(&(agent->log_lock));

	if (agent_log_cfg.enabled)
	{
		if (agent->log_fd != NULL)
		{
			// Roate log if cnt has been reset to < 0, or if it exceeds defined limit
			if (force || agent->log_fd_cnt < 0 || (
					agent_log_cfg.limit > 0 && agent->log_fd_cnt > agent_log_cfg.limit)
				)
			{
				fclose(agent->log_fd);
			}
			else
			{
				unlockResource(&(agent->log_lock));
				return; // Keep using the open file
			}
		}
		// Create sub-directories if required (first file only)
		if (agent_log_cfg.agent_dirs) {
			agent_autologging_sep = '/';

			if (agent->log_fd_cnt == 0) {
				snprintf(fn, sizeof(fn), "%s/%s",
						agent_log_cfg.dir,
						agent->eid.name
					);
#if defined(VXWORKS)
				mkdir(fn);
#else
				mkdir(fn,0777); // This will fail if directory already exists, which is acceptable
#endif
			}
		}
		snprintf(fn, sizeof(fn), "%s/%s%c%d.log",
				agent_log_cfg.dir,
				agent->eid.name,
				agent_autologging_sep, // Set to "/" to use seperate directories per agent
				agent->log_file_num
			);

		agent->log_fd = fopen(fn, "a");
		if (agent->log_fd != NULL) {
			agent->log_fd_cnt = 0;
			agent_prune_logs(agent, agent->log_file_num);
			agent->log_file_num++;
		} else {
		  AMP_DEBUG_ERR("agent_rotate_log", "Failed to open report log file (%s) for agent %s", fn, agent->eid.name);
		}
	}
	else if (agent->log_fd != NULL)
	{
		fclose(agent->log_fd);
		agent->log_fd = NULL;
	}

	unlockResource(&(agent->log_lock));
}




int agent_cb_comp(void *key, void *cur_val)
{
	char *rx = (char *)key;
	agent_t *a = (agent_t *)cur_val;

	CHKUSR(rx, -1);
	CHKUSR(a, -1);

	return strncmp(rx, a->eid.name, AMP_MAX_EID_LEN);
}


void agent_cb_del(void *item)
{
	agent_t *agent = (agent_t *) item;

	CHKVOID(agent);

	vec_release(&(agent->rpts), 0);
	vec_release(&(agent->tbls), 0);

	if (agent->log_fd != NULL)
	{
		fclose(agent->log_fd);
	}


	SRELEASE(item);
}




/******************************************************************************
 *
 * \par Function Name: agent_create
 *
 * \par Allocate and initialize a new agent structure.
 *
 * \param[in]  eid  - The endpoint identifier for the new agent.
 *
 * \return NULL - Error
 *         !NULL - Allocated, initialized agent structure.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 **  10/06/18  E. Birrane      Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

agent_t* agent_create(eid_t *eid)
{
	int success;
	agent_t *agent	= NULL;

	CHKNULL(eid);

	if((agent = (agent_t*)STAKE(sizeof(agent_t))) == NULL)
	{
		AMP_DEBUG_ERR("agent_create", "Can't alloc new agent", NULL);
		return NULL;
	}

	if(initResourceLock(&(agent->log_lock)))
	{
		AMP_DEBUG_ERR("agent_create", "Can't alloc log mutex", NULL);
		SRELEASE(agent);
		return NULL;
	}

	strncpy(agent->eid.name, eid->name, AMP_MAX_EID_LEN);

	agent->rpts = vec_create(AGENT_DEF_NUM_RPTS, rpt_cb_del_fn, rpt_cb_comp_fn, NULL, VEC_FLAG_AS_STACK, &success);
	if(success != VEC_OK)
	{
		AMP_DEBUG_ERR("agent_create","Can'tmake agent reports vector.", NULL);
		SRELEASE(agent);
		return NULL;
	}

	agent->tbls = vec_create(AGENT_DEF_NUM_TBLS, tbl_cb_del_fn, tbl_cb_comp_fn, NULL, VEC_FLAG_AS_STACK, &success);
	if(success != VEC_OK)
	{
		AMP_DEBUG_ERR("agent_create","Can'tmake agent tables vector.", NULL);
		SRELEASE(agent);
		agent = NULL;
	}


	return agent;
}




/******************************************************************************
 *
 * \par Function Name: agent_get
 *
 * \par Retrieve an agent from the manager list of known agents.
 *
 * \param[in]  eid  - The endpoint identifier for the agent.
 *
 * \return NULL - Error
 *        !NULL - The retrieved agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 **  10/06/18  E. Birrane      Updated to AMp v0.5 (JHU/APL)
 *****************************************************************************/
agent_t* agent_get(eid_t* eid)
{
	agent_t *result = NULL;
	int success;
	vec_idx_t i = vec_find(&(gMgrDB.agents), eid->name, &success);

	if(success == VEC_OK)
	{
		result = (agent_t *) vec_at(&gMgrDB.agents, i);
	}
	return result;
}





/******************************************************************************
 *
 * \par Function Name: agent_release
 *
 * \par Remove and deallocate the agent
 *
 * \param[in]  in_eid  - The endpoint identifier for the agent.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 **  10/06/18  E. Birrane      Updated to AMp v0.5 (JHU/APL)
 *****************************************************************************/

void agent_release(agent_t *agent, int destroy)
{
	CHKVOID(agent);

	vec_release(&(agent->rpts), 0);
	if (agent->log_fd != NULL)
	{
		fclose(agent->log_fd);
	}

	if(destroy)
	{
		killResourceLock(&(agent->log_lock));
		SRELEASE(agent);
	}
}
