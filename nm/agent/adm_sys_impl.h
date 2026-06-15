/****************************************************************************
 **
 ** File Name: adm_sys_impl.h
 **
 ** Description: Implementation prototypes for the DTN/sys ADM.  These are the
 **              collect functions that gather host operating-system resource
 **              statistics for the NM agent.
 **
 ** Notes: None.
 **
 ** Assumptions: None.
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2026-06-12  S. Jennen        Initial implementation
 **
 ****************************************************************************/

#ifndef ADM_SYS_IMPL_H_
#define ADM_SYS_IMPL_H_

/*   START CUSTOM INCLUDES HERE  */
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/utils/utils.h"
#include "shared/primitives/ctrl.h"
#include "shared/primitives/table.h"
#include "shared/primitives/tnv.h"


#ifdef __cplusplus
extern "C" {
#endif


/*   START typeENUM */
/*   STOP typeENUM  */

void dtn_sys_setup(void);
void dtn_sys_cleanup(void);


/*
 * +---------------------------------------------------------------------------------------------+
 * |                                     Retrieval Functions                                     +
 * +---------------------------------------------------------------------------------------------+
 */
/*   START CUSTOM FUNCTIONS HERE */
/*   STOP CUSTOM FUNCTIONS HERE  */


/* Metadata Functions */
tnv_t *dtn_sys_meta_name(tnvc_t *parms);
tnv_t *dtn_sys_meta_namespace(tnvc_t *parms);
tnv_t *dtn_sys_meta_version(tnvc_t *parms);
tnv_t *dtn_sys_meta_organization(tnvc_t *parms);

/* Constant Functions */

/* Collect Functions */
tnv_t *dtn_sys_get_num_cpus(tnvc_t *parms);
tnv_t *dtn_sys_get_load_1min(tnvc_t *parms);
tnv_t *dtn_sys_get_load_5min(tnvc_t *parms);
tnv_t *dtn_sys_get_load_15min(tnvc_t *parms);
tnv_t *dtn_sys_get_cpu_util_pct(tnvc_t *parms);
tnv_t *dtn_sys_get_mem_total_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_mem_free_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_mem_used_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_swap_total_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_swap_free_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_disk_total_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_disk_free_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_disk_used_bytes(tnvc_t *parms);
tnv_t *dtn_sys_get_open_files(tnvc_t *parms);
tnv_t *dtn_sys_get_max_files(tnvc_t *parms);
tnv_t *dtn_sys_get_num_procs(tnvc_t *parms);
tnv_t *dtn_sys_get_uptime_sec(tnvc_t *parms);


/* Control Functions */


/* OP Functions */


/* Table Build Functions */


#ifdef __cplusplus
}
#endif

#endif /* ADM_SYS_IMPL_H_ */
