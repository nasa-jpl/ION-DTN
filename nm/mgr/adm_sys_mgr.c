/****************************************************************************
 **
 ** File Name: adm_sys_mgr.c
 **
 ** Description: Manager-side registration of the DTN/sys ADM.  Registers the
 **              host system-statistic EDDs and report template, along with
 **              their human-readable metadata, for use by the NM manager UI.
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
#include "metadata.h"
#include "nm_mgr_ui.h"

static vec_idx_t g_dtn_sys_idx[11];

void dtn_sys_init(void)
{
	adm_add_adm_info("dtn_sys", ADM_ENUM_DTN_SYS);

	VDB_ADD_NN(((ADM_ENUM_DTN_SYS * 20) + ADM_META_IDX), &(g_dtn_sys_idx[ADM_META_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_SYS * 20) + ADM_RPTT_IDX), &(g_dtn_sys_idx[ADM_RPTT_IDX]));
	VDB_ADD_NN(((ADM_ENUM_DTN_SYS * 20) + ADM_EDD_IDX), &(g_dtn_sys_idx[ADM_EDD_IDX]));


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

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_NAME);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_SYS, "name", "The human-readable name of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_NAMESPACE);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_SYS, "namespace", "The namespace of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_VERSION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_SYS, "version", "The version of the ADM.");

	id = adm_build_ari(AMP_TYPE_CNST, 0, g_dtn_sys_idx[ADM_META_IDX], DTN_SYS_META_ORGANIZATION);
	adm_add_cnst(id, NULL);
	meta_add_cnst(AMP_TYPE_STR, id, ADM_ENUM_DTN_SYS, "organization", "The name of the issuing organization of the ADM.");

}

void dtn_sys_init_cnst(void)
{

}

void dtn_sys_init_edd(void)
{

	ari_t *id = NULL;

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_CPUS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_SYS, "num_cpus", "Number of CPUs currently online.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_1MIN);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_REAL64, id, ADM_ENUM_DTN_SYS, "load_1min", "1 minute load average.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_5MIN);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_REAL64, id, ADM_ENUM_DTN_SYS, "load_5min", "5 minute load average.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_LOAD_15MIN);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_REAL64, id, ADM_ENUM_DTN_SYS, "load_15min", "15 minute load average.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_CPU_UTIL_PCT);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_REAL64, id, ADM_ENUM_DTN_SYS, "cpu_util_pct", "Overall CPU utilization, in percent, measured since the previous query.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_TOTAL_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "mem_total_bytes", "Total physical memory, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_FREE_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "mem_free_bytes", "Free physical memory, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_USED_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "mem_used_bytes", "Used physical memory, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_TOTAL_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "swap_total_bytes", "Total swap space, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_FREE_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "swap_free_bytes", "Free swap space, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_TOTAL_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "disk_total_bytes", "Total space of the root filesystem, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_FREE_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "disk_free_bytes", "Free space of the root filesystem, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_USED_BYTES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "disk_used_bytes", "Used space of the root filesystem, in bytes.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_OPEN_FILES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "open_files", "Number of open file handles on the host.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MAX_FILES);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "max_files", "Maximum number of open file handles allowed on the host.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_PROCS);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UINT, id, ADM_ENUM_DTN_SYS, "num_procs", "Number of processes/threads on the host.");

	id = adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_UPTIME_SEC);
	adm_add_edd(id, NULL);
	meta_add_edd(AMP_TYPE_UVAST, id, ADM_ENUM_DTN_SYS, "uptime_sec", "Seconds elapsed since the host booted.");
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
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_TOTAL_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_FREE_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MEM_USED_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_TOTAL_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_SWAP_FREE_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_TOTAL_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_FREE_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_DISK_USED_BYTES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_OPEN_FILES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_MAX_FILES));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_NUM_PROCS));
	rpttpl_add_item(def, adm_build_ari(AMP_TYPE_EDD, 0, g_dtn_sys_idx[ADM_EDD_IDX], DTN_SYS_EDD_UPTIME_SEC));
	adm_add_rpttpl(def);
	meta_add_rpttpl(def->id, ADM_ENUM_DTN_SYS, "full_report", "All host system statistics.");
}

void dtn_sys_init_tblt(void)
{

}
