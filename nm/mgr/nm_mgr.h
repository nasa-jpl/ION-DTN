/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 ** \file nm_mgr.h
 **
 ** File Name: nm_mgr.h
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
 **  10/06/18   E. Birrane     Update to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#ifndef NM_MGR_H
#define NM_MGR_H

// ION includes
#include "platform.h"
#include "sdr.h"

// Standard includes
#include "stdint.h"
#include "pthread.h"
#include "unistd.h"


// Application includes
#include "../shared/nm.h"
#include "../shared/utils/nm_types.h"
#include "../shared/msg/ion_if.h"

#include "../shared/adm/adm.h"
#include "../shared/primitives/report.h"
#include "../shared/msg/msg.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_MYSQL
#include "nm_mgr_sql.h"
#endif

#include "ion_atomic.h"


/* Constants */
#define NM_RECEIVE_TIMEOUT_SEC (2)
#define NM_MGR_MAX_META (1024)


typedef struct
{
	vector_t agents;  /* (agent_t *) */
	rhht_t metadata; /* (metadata_t*) */
	ion_atomic_t tot_rpts;
	ion_atomic_t tot_tbls;
	eid_t mgr_eid;

#ifdef HAVE_MYSQL
	sql_db_t sql_info;
#endif
} mgr_db_t;

extern mgr_db_t gMgrDB;
extern iif_t ion_ptr;

// ============================= Global Data ===============================

/* Function Prototypes */
int      main(int argc, char *argv[]);

int      mgr_cleanup(void);
int      mgr_init(char *eid);
void*    mgr_rx_thread(void *arg);



#ifdef __cplusplus
}
#endif

#endif /* NM_MGR_H */
