/****************************************************************************
 **
 ** File Name: adm_sys_impl.c
 **
 ** Description: Implementation of the DTN/sys ADM collect functions.  These
 **              gather host operating-system resource statistics (CPU, load,
 **              memory, swap, disk, open files, processes and uptime) for
 **              reporting through the NM agent.
 **
 ** Notes: The statistics are inherently host specific.  This implementation
 **        targets Linux using sysinfo(2), statvfs(3), getloadavg(3) and the
 **        /proc filesystem.  On other platforms the collect functions return
 **        no value (the statistic is reported as unavailable) rather than
 **        failing to build.
 **
 ** Assumptions: None.
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2026-06-12  S. Jennen        Initial implementation
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#if defined(linux) || defined(__linux__)
#define ADM_SYS_LINUX 1
/*
 * The host-statistic helpers below use getloadavg(3), sysinfo(2) and
 * statvfs(3).  Under the strict ISO C build (-std=iso9899:2018) glibc only
 * declares these when _GNU_SOURCE (or an equivalent feature-test macro) is
 * defined ahead of the system headers.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#endif
/*   STOP CUSTOM INCLUDES HERE  */


#include "shared/adm/adm.h"
#include "adm_sys_impl.h"

/*   START CUSTOM FUNCTIONS HERE */

#ifdef ADM_SYS_LINUX

/* Root filesystem queried for disk statistics. */
#define ADM_SYS_DISK_PATH "/"

/* Convert a sysinfo memory quantity into KiB, accounting for mem_unit. */
static uvast adm_sys_mem_kb(unsigned long val, unsigned int unit)
{
	return ((uvast) val * (unit ? unit : 1)) / 1024;
}

/*
 * Sample overall CPU utilization since the previous call by differencing the
 * aggregate jiffy counters in /proc/stat.  The first sample after start-up has
 * no prior reference and reports 0.0.
 */
static int adm_sys_cpu_util(double *pct)
{
	static int   have_prev = 0;
	static uvast prev_total = 0;
	static uvast prev_idle = 0;

	FILE  *fp;
	char   label[16];
	uvast  user, nice, sys, idle, iowait, irq, softirq, steal;
	uvast  total, busy_idle, dtotal, didle;

	if ((fp = fopen("/proc/stat", "r")) == NULL)
	{
		return 0;
	}

	user = nice = sys = idle = iowait = irq = softirq = steal = 0;
	if (fscanf(fp, "%15s " UVAST_FIELDSPEC " " UVAST_FIELDSPEC " "
			UVAST_FIELDSPEC " " UVAST_FIELDSPEC " " UVAST_FIELDSPEC
			" " UVAST_FIELDSPEC " " UVAST_FIELDSPEC " " UVAST_FIELDSPEC,
			label, &user, &nice, &sys, &idle, &iowait, &irq,
			&softirq, &steal) < 5)
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);

	busy_idle = idle + iowait;
	total = user + nice + sys + idle + iowait + irq + softirq + steal;

	if (!have_prev)
	{
		prev_total = total;
		prev_idle = busy_idle;
		have_prev = 1;
		*pct = 0.0;
		return 1;
	}

	dtotal = (total > prev_total) ? (total - prev_total) : 0;
	didle = (busy_idle > prev_idle) ? (busy_idle - prev_idle) : 0;

	prev_total = total;
	prev_idle = busy_idle;

	if (dtotal == 0)
	{
		*pct = 0.0;
	}
	else
	{
		*pct = 100.0 * (double) (dtotal - didle) / (double) dtotal;
	}

	return 1;
}

/* Read the host-wide allocated / maximum open file handle counts. */
static int adm_sys_file_handles(uvast *open_files, uvast *max_files)
{
	FILE  *fp;
	uvast  allocated, unused, max;

	if ((fp = fopen("/proc/sys/fs/file-nr", "r")) == NULL)
	{
		return 0;
	}

	allocated = unused = max = 0;
	if (fscanf(fp, UVAST_FIELDSPEC " " UVAST_FIELDSPEC " "
			UVAST_FIELDSPEC, &allocated, &unused, &max) != 3)
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);

	if (open_files != NULL)
	{
		*open_files = allocated;
	}

	if (max_files != NULL)
	{
		*max_files = max;
	}

	return 1;
}

/* Gather root-filesystem totals; selector chooses which figure to return. */
typedef enum { ADM_SYS_DISK_TOTAL, ADM_SYS_DISK_FREE, ADM_SYS_DISK_USED }
		AdmSysDiskField;

static int adm_sys_disk_kb(AdmSysDiskField field, uvast *kb)
{
	struct statvfs	vfs;
	uvast		bsize;

	if (statvfs(ADM_SYS_DISK_PATH, &vfs) != 0)
	{
		return 0;
	}

	bsize = (vfs.f_frsize ? vfs.f_frsize : vfs.f_bsize);

	switch (field)
	{
	case ADM_SYS_DISK_TOTAL:
		*kb = ((uvast) vfs.f_blocks * bsize) / 1024;
		break;

	case ADM_SYS_DISK_FREE:
		*kb = ((uvast) vfs.f_bavail * bsize) / 1024;
		break;

	case ADM_SYS_DISK_USED:
		*kb = ((uvast) (vfs.f_blocks - vfs.f_bfree) * bsize) / 1024;
		break;

	default:
		return 0;
	}

	return 1;
}

#endif /* ADM_SYS_LINUX */

/*   STOP CUSTOM FUNCTIONS HERE  */

void dtn_sys_setup(void)
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void dtn_sys_cleanup(void)
{

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


tnv_t *dtn_sys_meta_name(tnvc_t *parms)
{
	/* Parameter intentionally unused. */
	(void)parms;

	return tnv_from_str("sys");
}


tnv_t *dtn_sys_meta_namespace(tnvc_t *parms)
{
	/* Parameter intentionally unused. */
	(void)parms;

	return tnv_from_str("DTN/sys");
}


tnv_t *dtn_sys_meta_version(tnvc_t *parms)
{
	/* Parameter intentionally unused. */
	(void)parms;

	return tnv_from_str("v0.0");
}


tnv_t *dtn_sys_meta_organization(tnvc_t *parms)
{
	/* Parameter intentionally unused. */
	(void)parms;

	return tnv_from_str("JHUAPL");
}


/* Collect Functions */


/*
 * Number of CPUs currently online.
 */
tnv_t *dtn_sys_get_num_cpus(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_cpus BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);

	if (ncpus > 0)
	{
		result = tnv_from_uint((uint32_t) ncpus);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_cpus BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * 1 minute load average.
 */
tnv_t *dtn_sys_get_load_1min(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_load_1min BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	double load[3];

	if (getloadavg(load, 3) >= 1)
	{
		result = tnv_from_real64(load[0]);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_load_1min BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * 5 minute load average.
 */
tnv_t *dtn_sys_get_load_5min(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_load_5min BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	double load[3];

	if (getloadavg(load, 3) >= 2)
	{
		result = tnv_from_real64(load[1]);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_load_5min BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * 15 minute load average.
 */
tnv_t *dtn_sys_get_load_15min(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_load_15min BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	double load[3];

	if (getloadavg(load, 3) >= 3)
	{
		result = tnv_from_real64(load[2]);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_load_15min BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Overall CPU utilization, in percent, measured since the previous query.
 */
tnv_t *dtn_sys_get_cpu_util_pct(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_cpu_util_pct BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	double pct = 0.0;

	if (adm_sys_cpu_util(&pct))
	{
		result = tnv_from_real64(pct);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_cpu_util_pct BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total physical memory, in KiB.
 */
tnv_t *dtn_sys_get_mem_total_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_mem_total_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uvast(adm_sys_mem_kb(si.totalram, si.mem_unit));
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_mem_total_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Free physical memory, in KiB.
 */
tnv_t *dtn_sys_get_mem_free_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_mem_free_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uvast(adm_sys_mem_kb(si.freeram, si.mem_unit));
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_mem_free_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Used physical memory, in KiB.
 */
tnv_t *dtn_sys_get_mem_used_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_mem_used_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uvast(adm_sys_mem_kb(si.totalram - si.freeram,
				si.mem_unit));
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_mem_used_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total swap space, in KiB.
 */
tnv_t *dtn_sys_get_swap_total_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_swap_total_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uvast(adm_sys_mem_kb(si.totalswap, si.mem_unit));
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_swap_total_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Free swap space, in KiB.
 */
tnv_t *dtn_sys_get_swap_free_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_swap_free_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uvast(adm_sys_mem_kb(si.freeswap, si.mem_unit));
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_swap_free_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Total space of the root filesystem, in KiB.
 */
tnv_t *dtn_sys_get_disk_total_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_disk_total_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	uvast kb = 0;

	if (adm_sys_disk_kb(ADM_SYS_DISK_TOTAL, &kb))
	{
		result = tnv_from_uvast(kb);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_disk_total_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Free space of the root filesystem, in KiB.
 */
tnv_t *dtn_sys_get_disk_free_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_disk_free_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	uvast kb = 0;

	if (adm_sys_disk_kb(ADM_SYS_DISK_FREE, &kb))
	{
		result = tnv_from_uvast(kb);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_disk_free_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Used space of the root filesystem, in KiB.
 */
tnv_t *dtn_sys_get_disk_used_kb(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_disk_used_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	uvast kb = 0;

	if (adm_sys_disk_kb(ADM_SYS_DISK_USED, &kb))
	{
		result = tnv_from_uvast(kb);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_disk_used_kb BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of open file handles on the host.
 */
tnv_t *dtn_sys_get_open_files(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_open_files BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	uvast open_files = 0;

	if (adm_sys_file_handles(&open_files, NULL))
	{
		result = tnv_from_uvast(open_files);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_open_files BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Maximum number of open file handles allowed on the host.
 */
tnv_t *dtn_sys_get_max_files(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_max_files BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	uvast max_files = 0;

	if (adm_sys_file_handles(NULL, &max_files))
	{
		result = tnv_from_uvast(max_files);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_max_files BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Number of processes/threads on the host.
 */
tnv_t *dtn_sys_get_num_procs(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_num_procs BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uint((uint32_t) si.procs);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_num_procs BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Seconds elapsed since the host booted.
 */
tnv_t *dtn_sys_get_uptime_sec(tnvc_t *parms)
{
	tnv_t *result = NULL;
	/* Parameter intentionally unused. */
	(void)parms;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_uptime_sec BODY
	 * +-------------------------------------------------------------------------+
	 */
#ifdef ADM_SYS_LINUX
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		result = tnv_from_uvast((uvast) si.uptime);
	}
#endif
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_uptime_sec BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}
