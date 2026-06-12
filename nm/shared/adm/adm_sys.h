/****************************************************************************
 **
 ** File Name: adm_sys.h
 **
 ** Description: This ADM exposes host operating-system resource statistics
 **              (CPU, load, memory, swap, disk, open files, processes and
 **              uptime) as externally defined data (EDD) so that an NM
 **              manager can poll the health of the machine running the agent.
 **
 ** Notes: Stat collection is host specific.  The current implementation
 **        targets Linux (sysinfo(2), statvfs(3), getloadavg(3) and the
 **        /proc filesystem).  On platforms where a statistic cannot be
 **        gathered the corresponding collect function reports no value.
 **
 ** Assumptions: None.
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2026-06-12  S. Jennen        Initial implementation
 **
 ****************************************************************************/

#ifndef ADM_SYS_H_
#define ADM_SYS_H_

#include "shared/utils/nm_types.h"
#include "shared/adm/adm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 ADM TEMPLATE DOCUMENTATION                                  +
 * +---------------------------------------------------------------------------------------------+
 *
 * ADM ROOT STRING:DTN/sys
 */
#define ADM_ENUM_DTN_SYS 11

/*
 * +---------------------------------------------------------------------------------------------+
 * |                                 DTN_SYS META-DATA DEFINITIONS                               +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |         VALUE          |
 * +---------------------+--------------------------------------+-------+------------------------+
 * |name                 |The human-readable name of the ADM.   |STR    |sys                     |
 * |namespace            |The namespace of the ADM.             |STR    |DTN/sys                 |
 * |version              |The version of the ADM.               |STR    |v0.0                    |
 * |organization         |The issuing organization of the ADM.  |STR    |JHUAPL                  |
 * +---------------------+--------------------------------------+-------+------------------------+
 */
// "name"
#define DTN_SYS_META_NAME 0x00
// "namespace"
#define DTN_SYS_META_NAMESPACE 0x01
// "version"
#define DTN_SYS_META_VERSION 0x02
// "organization"
#define DTN_SYS_META_ORGANIZATION 0x03


/*
 * +---------------------------------------------------------------------------------------------+
 * |                        DTN_SYS EXTERNALLY DEFINED DATA DEFINITIONS                          +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |num_cpus             |Number of CPUs currently online.      |UINT   |
 * |load_1min            |1 minute load average.                |REAL64 |
 * |load_5min            |5 minute load average.                |REAL64 |
 * |load_15min           |15 minute load average.               |REAL64 |
 * |cpu_util_pct         |Overall CPU utilization, in percent,  |       |
 * |                     |measured since the previous query.    |REAL64 |
 * |mem_total_kb         |Total physical memory, in KiB.        |UVAST  |
 * |mem_free_kb          |Free physical memory, in KiB.         |UVAST  |
 * |mem_used_kb          |Used physical memory, in KiB.         |UVAST  |
 * |swap_total_kb        |Total swap space, in KiB.             |UVAST  |
 * |swap_free_kb         |Free swap space, in KiB.              |UVAST  |
 * |disk_total_kb        |Total space of the root filesystem, in|       |
 * |                     | KiB.                                 |UVAST  |
 * |disk_free_kb         |Free space of the root filesystem, in |       |
 * |                     |KiB (available to unprivileged users).|UVAST  |
 * |disk_used_kb         |Used space of the root filesystem, in |       |
 * |                     |KiB.                                  |UVAST  |
 * |open_files           |Number of open file handles on the    |       |
 * |                     |host.                                 |UVAST  |
 * |max_files            |Maximum number of open file handles   |       |
 * |                     |allowed on the host.                  |UVAST  |
 * |num_procs            |Number of processes/threads on the    |       |
 * |                     |host.                                 |UINT   |
 * |uptime_sec           |Seconds elapsed since the host booted.|UVAST  |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_SYS_EDD_NUM_CPUS 0x00
#define DTN_SYS_EDD_LOAD_1MIN 0x01
#define DTN_SYS_EDD_LOAD_5MIN 0x02
#define DTN_SYS_EDD_LOAD_15MIN 0x03
#define DTN_SYS_EDD_CPU_UTIL_PCT 0x04
#define DTN_SYS_EDD_MEM_TOTAL_KB 0x05
#define DTN_SYS_EDD_MEM_FREE_KB 0x06
#define DTN_SYS_EDD_MEM_USED_KB 0x07
#define DTN_SYS_EDD_SWAP_TOTAL_KB 0x08
#define DTN_SYS_EDD_SWAP_FREE_KB 0x09
#define DTN_SYS_EDD_DISK_TOTAL_KB 0x0a
#define DTN_SYS_EDD_DISK_FREE_KB 0x0b
#define DTN_SYS_EDD_DISK_USED_KB 0x0c
#define DTN_SYS_EDD_OPEN_FILES 0x0d
#define DTN_SYS_EDD_MAX_FILES 0x0e
#define DTN_SYS_EDD_NUM_PROCS 0x0f
#define DTN_SYS_EDD_UPTIME_SEC 0x10


/*
 * +---------------------------------------------------------------------------------------------+
 * |                              DTN_SYS REPORT DEFINITIONS                                     +
 * +---------------------------------------------------------------------------------------------+
 * |        NAME         |             DESCRIPTION              | TYPE  |
 * +---------------------+--------------------------------------+-------+
 * |full_report          |All host system statistics.           |TNVC   |
 * +---------------------+--------------------------------------+-------+
 */
#define DTN_SYS_RPTTPL_FULL_REPORT 0x00


/* Initialization functions. */
void dtn_sys_init(void);
void dtn_sys_init_meta(void);
void dtn_sys_init_cnst(void);
void dtn_sys_init_edd(void);
void dtn_sys_init_op(void);
void dtn_sys_init_var(void);
void dtn_sys_init_ctrl(void);
void dtn_sys_init_mac(void);
void dtn_sys_init_rpttpl(void);
void dtn_sys_init_tblt(void);

#ifdef __cplusplus
}
#endif

#endif /* ADM_SYS_H_ */
