/****************************************************************************
 **
 ** File Name: adm_sys_agent.c
 **
 ** Description: Agent-side registration of the DTN/sys ADM.  Binds the host
 **              system-statistic EDDs and report template to their collect
 **              functions.
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


#include "ion.h"
#include "platform.h"
#include "adm_sys.h"
#include "shared/utils/utils.h"
#include "shared/primitives/report.h"
#include "shared/primitives/blob.h"
#include "adm_sys_impl.h"
#include "agent/rda.h"

static vec_idx_t g_dtn_sys_idx[11];

void dtn_sys_init(void)
{
	adm_add_adm_info("dtn_sys", ADM_ENUM_DTN_SYS);

	VDB_ADD_NN(((ADM_ENUM_DTN_SYS * 20) + ADM_META_IDX), &(g_dtn_sys_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_SYS * 20) + ADM_RPTT_IDX), &(g_dtn_sys_idx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_SYS * 20) + ADM_EDD_IDX), &(g_dtn_sys_idx[ADM_EDD_IDX]));


	dtn_sys_setup();
	dtn_sys_init_meta();
	dtn_sys_init_cnst();
	dtn_sys_init_edd();
	dtn_sys_init_op();
	dtn_sys_init_var();
	dtn_sys_init_ctrl();
	dtn_sys_init_mac();
	dtn_sys_init_rpttpl();
	dtn_sys_init_tblt();
}

void dtn_sys_init_meta(void)
{

	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_NAME), dtn_sys_meta_name);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_NAMESPACE), dtn_sys_meta_namespace);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_VERSION), dtn_sys_meta_version);
	adm_add_cnst(adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_ORGANIZATION), dtn_sys_meta_organization);
}

void dtn_sys_init_cnst(void)
{

}

void dtn_sys_init_edd(void)
{

	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_CPUS), dtn_sys_get_num_cpus);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_1MIN), dtn_sys_get_load_1min);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_5MIN), dtn_sys_get_load_5min);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_15MIN), dtn_sys_get_load_15min);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_CPU_UTIL_PCT), dtn_sys_get_cpu_util_pct);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_TOTAL_KB), dtn_sys_get_mem_total_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_FREE_KB), dtn_sys_get_mem_free_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_USED_KB), dtn_sys_get_mem_used_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_TOTAL_KB), dtn_sys_get_swap_total_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_FREE_KB), dtn_sys_get_swap_free_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_TOTAL_KB), dtn_sys_get_disk_total_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_FREE_KB), dtn_sys_get_disk_free_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_USED_KB), dtn_sys_get_disk_used_kb);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_OPEN_FILES), dtn_sys_get_open_files);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MAX_FILES), dtn_sys_get_max_files);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_PROCS), dtn_sys_get_num_procs);
	adm_add_edd(adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_UPTIME_SEC), dtn_sys_get_uptime_sec);
}

void dtn_sys_init_op(void)
{

}

void dtn_sys_init_var(void)
{

}

void dtn_sys_init_ctrl(void)
{

}

void dtn_sys_init_mac(void)
{

}

void dtn_sys_init_rpttpl(void)
{

	rpttpl_t *def = NULL;

	/* FULL_REPORT */
	def = rpttpl_create_id(adm_build_ari(AMP_TYPE_RPTTPL, 0, g_dtn_sys_idx[ADM_RPTT_IDX], DTN_SYS_RPTTPL_FULL_REPORT));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_NAME));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_CPUS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_1MIN));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_5MIN));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_15MIN));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_CPU_UTIL_PCT));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_TOTAL_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_FREE_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_USED_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_TOTAL_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_FREE_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_TOTAL_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_FREE_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_USED_KB));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_OPEN_FILES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MAX_FILES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_PROCS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_UPTIME_SEC));
	adm_add_rpttpl(def);
}

void dtn_sys_init_tblt(void)
{

}
