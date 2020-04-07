/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 ** \file nm_mgr.c
 **
 ** File Name: nm_mgr.c
 **
 ** Subsystem:
 **          Network Manager Application
 **
 ** Description: This file implements the DTNMP Manager user interface
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR          DESCRIPTION
 **  --------  ------------    ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation (JHU/APL)
 **  08/19/13  E. Birrane      Documentation clean up and code review comments. (JHU/APL)
 **  08/21/16  E. Birrane      Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 **  10/06/18  E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

// Application headers.
#include "nm_mgr.h"
#include "nm_mgr_ui.h"
#include "metadata.h"

#ifdef USE_CIVETWEB
#include "nm_rest.h"
#endif

#include "agents.h"

#include "../shared/primitives/rules.h"



mgr_db_t gMgrDB;
iif_t ion_ptr;
int  gRunning;

char* mgr_parse_args(int argc, char* argv[]);
void mgr_print_usage(void);

/******************************************************************************
 *
 * \par Function Name: main
 *
 * \par Main agent processing function.
 *
 * \param[in]  argc    # command line arguments.
 * \param[in]  argv[]  Command-line arguments.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 **  10/06/18  E. Birrane      Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int main(int argc, char *argv[])
{
    pthread_t rx_thr;
    pthread_t ui_thr;

#ifdef HAVE_MYSQL
    pthread_t db_thr;
    char db_thr_name[] = "db_thread";
#endif

    char rx_thr_name[]     = "rx_thread";
    char ui_thr_name[]     = "ui_thread";
    char daemon_thr_name[] = "run_daemon";
    char *mgr_eid;

    errno = 0;

    if (argc > 2)
    {
        // Assume argv[1] is required manager_eid
        mgr_eid = mgr_parse_args(argc, argv);
        if (mgr_eid == NULL)
        {
            mgr_print_usage();
            return 1;
        }

    }
    else if(argc != 2)
    {
        fprintf(stderr,"Invalid number of arguments for nm_mgr\n");
        mgr_print_usage();
        return 1;
    }
    else
    {
        mgr_eid = argv[1];
    }

    /* Indicate that the threads should run once started. */
    gRunning = 1;

    /* Initialize the AMP Manager. */
    if(mgr_init(mgr_eid) != AMP_OK)
    {
    	AMP_DEBUG_ERR("main","Can't init Manager.", NULL);
    	exit(EXIT_FAILURE);
    }

    AMP_DEBUG_INFO("main","Manager EID: %s", argv[1]);


    /* Spawn threads for receiving msgs, user interface, and db connection. */
    if(pthread_begin(&rx_thr, NULL, (void *)mgr_rx_thread, (void *)&gRunning, "nm_mgr_rx"))
    {
        AMP_DEBUG_ERR("main","Can't create pthread %s, errnor = %s",
        		        rx_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }


    if(pthread_begin(&ui_thr, NULL, (void *)ui_thread, (void *)&gRunning, "nm_mgr_ui"))
    {
        AMP_DEBUG_ERR("main","Can't create pthread %s, errnor = %s",
        		        ui_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

#ifdef HAVE_MYSQL

    if(pthread_begin(&db_thr, NULL, (void *)db_mgt_daemon, (void *)&gRunning ,"nm_mgr_db"))
    {
    	AMP_DEBUG_ERR("main","Can't create pthread %s, errnor = %s",
    			db_thr_name, strerror(errno));
    	exit(EXIT_FAILURE);
    }
#endif

#ifdef USE_CIVETWEB
    nm_rest_start();
#endif

    if (pthread_join(rx_thr, NULL))
    {
        AMP_DEBUG_ERR("main","Can't join pthread %s. Errnor = %s",
        		        rx_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_join(ui_thr, NULL))
    {
        AMP_DEBUG_ERR("main","Can't join pthread %s. Errnor = %s",
        		         ui_thr_name, strerror(errno));
        exit(EXIT_FAILURE);
    }

#ifdef USE_CIVETWEB
    nm_rest_stop();
#endif
    
#ifdef HAVE_MYSQL
    if (pthread_join(db_thr, NULL))
    {
    	AMP_DEBUG_ERR("main","Can't join pthread %s. Errnor = %s",
    			        db_thr_name, strerror(errno));
    	exit(EXIT_FAILURE);
    }
#endif

    AMP_DEBUG_ALWAYS("main","Shutting down manager.", NULL);
    mgr_cleanup();

    AMP_DEBUG_INFO("main","Exiting Manager after cleanup.", NULL);
    exit(0);
}




/******************************************************************************
 *
 * \par Function Name: mgr_cleanup
 *
 * \par Cleans resources before exiting the manager.
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

int mgr_cleanup()
{

#ifdef HAVE_MYSQL
	db_mgt_close();
#endif

	vec_release(&(gMgrDB.agents), 0);
	rhht_release(&(gMgrDB.metadata), 0);

	db_destroy();

	utils_mem_teardown();

	return AMP_OK;
}



/******************************************************************************
 *
 * \par Function Name: mgr_init
 *
 * \par Initialize the manager...
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  09/01/11  V. Ramachandran Initial Implementation
 **  08/20/13  E. Birrane      Code Review Updates
 **  10/06/18  E. Birrane      Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/
int mgr_init(char *arg_eid)
{
	int success;

	AMP_DEBUG_ENTRY("mgr_init","("ADDR_FIELDSPEC")",(uaddr) arg_eid);

    /* Step 2: Make sure that ION is running and we can attach. */
	if (ionAttach() < 0)
	{
		AMP_DEBUG_ERR("mgr_init", "Manager can't attach to ION.", NULL);
		return -1;
	}


	/* Step 1: Initialize MGR-specific data.*/
	gMgrDB.agents = vec_create(AGENT_DEF_NUM_AGTS, agent_cb_del,agent_cb_comp, NULL, 0, &success);
	if(success != VEC_OK)
	{
		AMP_DEBUG_ERR("mgr_init", "Can't make agents vec.", NULL);
		return AMP_FAIL;
	}

	gMgrDB.metadata = rhht_create(NM_MGR_MAX_META, ari_cb_comp_no_parm_fn, ari_cb_hash, meta_cb_del, &success);
	if(success != RH_OK)
	{
		AMP_DEBUG_ERR("mgr_init", "Can't make parmspec ht.", NULL);
		return AMP_FAIL;
	}


	gMgrDB.tot_rpts = 0;
    gMgrDB.tot_tbls = 0;
    istrcpy((char *) gMgrDB.mgr_eid.name, arg_eid, AMP_MAX_EID_LEN);


	/* Step 2:  Attach to ION. */
    if(iif_register_node(&ion_ptr, gMgrDB.mgr_eid) == 0)
    {
        AMP_DEBUG_ERR("mgr_init","Unable to register BP Node. Exiting.", NULL);
        return AMP_FAIL;
    }

    if (iif_is_registered(&ion_ptr))
    {
        AMP_DEBUG_INFO("mgr_init", "Mgr registered with ION, EID: %s",
        		         iif_get_local_eid(&ion_ptr).name);
    }
    else
    {
        AMP_DEBUG_ERR("mgr_init","Failed to register mgr with ION, EID %s",
        				 iif_get_local_eid(&ion_ptr).name);
        AMP_DEBUG_EXIT("mgr_init","->-1.",NULL);
        return AMP_FAIL;
    }


    if((utils_mem_int()       != AMP_OK) ||
       (db_init("nmmgr_db", &adm_init) != AMP_OK))
    {
    	db_destroy();
    	AMP_DEBUG_ERR("mgr_init","Unable to initialize DB.", NULL);
    	return AMP_FAIL;
    }

#ifdef HAVE_MYSQL
	db_mgr_sql_init();
	success = db_mgt_init(gMgrDB.sql_info, 0, 1);
#endif

    success = AMP_OK;

    return success;
}

/**
 * Parse optional command line arguments
 */
char* mgr_parse_args(int argc, char* argv[])
{
    int i;
    int c;
    
    while ((c = getopt(argc, argv, "ldL:D:rtTRaAjJ")) != -1)
    {
        switch(c)
        {
        case 'l':
            agent_log_cfg.enabled = 1;
            break;
        case 'd':
            agent_log_cfg.agent_dirs = 1;
            break;
        case 'r':
            agent_log_cfg.rx_rpt = 1;
            break;
        case 't':
            agent_log_cfg.rx_tbl = 1;
            break;
        case 'T':
            agent_log_cfg.tx_cbor = 1;
            break;
        case 'R':
            agent_log_cfg.rx_cbor = 1;
            break;
#ifdef USE_JSON
        case 'j':
            agent_log_cfg.rx_json_rpt = 1;
            break;
        case 'J':
            agent_log_cfg.rx_json_tbl = 1;
            break;
#endif
        case 'D':
            strncpy(agent_log_cfg.dir, optarg, sizeof(agent_log_cfg.dir)-1);
            break;
        case 'L':
            agent_log_cfg.limit = atoi(optarg);
            break;
        case 'a':
        case 'A':
            mgr_ui_mode = MGR_UI_AUTOMATOR;
            break;
        case 'h':
            return NULL;
        default:
            fprintf(stderr, "Error parsing arguments\n");
            return NULL;
        }
    }

    // Check for any remaining unrecognized arguments
    if ((argc-optind) != 1)
    {
        fprintf(stderr,"%d unrecognized arguments:\n", (argc-optind));
        for(i = optind; i < argc; i++)
        {
            printf("\t%s\n", argv[i]);
        }
        return NULL;
    }
    else
    {
        return argv[optind];
    }
}

void mgr_print_usage(void)
{

    printf("AMP Protocol Version %d - %s/%02d, built on %s %s\n",
           AMP_VERSION,
           AMP_PROTOCOL_URL,
           AMP_VERSION,
           __DATE__, __TIME__);

    printf("Usage: nm_mgr [options] <manager eid>\n");
    printf("Supported Options:\n");
    printf("-A       Startup directly in the alternative Automator UI mode\n");
    printf("-l       If specified, enable file-based logging of Manager Activity.\n");
    printf("           If logging is not enabled, the following have no affect until enabled in UI\n");
    printf("-d       Log each agent to a different directory\n");
    printf("-L #      Specify maximum number of entries (reports+tables) per file before rotating\n");
    printf("-D DIR   NM logs will be placed in this directory\n");
    printf("-r       Log all received reports to file in text format (as shown in UI)\n");
    printf("-t       Log all received tables to file in text format (as shown in UI)\n");
    printf("-T       Log all transmitted message as ASCII-encoded CBOR HEX strings\n");
    printf("-R       Log all received messages as ASCII-encoded CBOR HEX strings\n");
}
