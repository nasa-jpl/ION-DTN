/*
 *	sdr_perf.c:	SDR Performance Instrumentation Implementation.
 *
 *	Compile with -DSDR_PERF_INSTRUMENTATION to enable.
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	All rights reserved.
 */

#ifdef SDR_PERF_INSTRUMENTATION

#include "sdr_perf.h"
#include "ion.h"
#include <string.h>

/*	Simple hash function for file:line to bucket index.		*/

static unsigned int	callerHash(const char *file, int line)
{
	unsigned int	hash = (unsigned int) line;

	if (file)
	{
		while (*file)
		{
			hash = hash * 31 + (unsigned char) *file++;
		}
	}

	return hash % SDR_PERF_CALLER_BUCKETS;
}

void	sdr_perf_xn_begin(SdrPerfStats *stats, const char *file, int line)
{
	memset(stats, 0, sizeof(SdrPerfStats));
	stats->callerFile = file;
	stats->callerLine = line;
	getCurrentTime(&stats->xnStartTime);
}

void	sdr_perf_xn_end(SdrPerfCounters *counters, SdrPerfStats *stats)
{
	long		xnTimeSigned;
	unsigned long	xnTime;
	unsigned int	bucket;
	SdrPerfCaller	*caller;

	getCurrentTime(&stats->xnEndTime);
	xnTimeSigned = SDR_PERF_TIME_DIFF_US(stats->xnStartTime,
			stats->xnEndTime);
	xnTime = (xnTimeSigned > 0) ? (unsigned long) xnTimeSigned : 0;

	counters->xnCount++;
	counters->totalXnTimeUs += xnTime;
	if (xnTime > counters->maxXnTimeUs)
	{
		const char	*basename;
		size_t		len;

		counters->maxXnTimeUs = xnTime;

		/*	Record where the max transaction occurred.	*/

		if (stats->callerFile != NULL)
		{
			basename = strrchr(stats->callerFile, '/');
			basename = basename ? basename + 1 : stats->callerFile;
			len = strlen(basename);
			if (len >= SDR_PERF_FILENAME_LEN)
			{
				len = SDR_PERF_FILENAME_LEN - 1;
			}

			memcpy(counters->maxXnFile, basename, len);
			counters->maxXnFile[len] = '\0';
		}

		counters->maxXnLine = stats->callerLine;
		counters->maxXnCallerCount = 1;	/*	New max caller.	*/
	}
	else if (counters->maxXnFile[0] != '\0' && stats->callerFile != NULL)
	{
		/*	Check if current caller is the max-time caller.	*/

		const char	*basename;

		basename = strrchr(stats->callerFile, '/');
		basename = basename ? basename + 1 : stats->callerFile;
		if (strcmp(basename, counters->maxXnFile) == 0
				&& stats->callerLine == counters->maxXnLine)
		{
			counters->maxXnCallerCount++;
		}
	}

	counters->totalReadCount += stats->readCount;
	counters->totalWriteCount += stats->writeCount;
	counters->totalBytesRead += stats->bytesRead;
	counters->totalBytesWritten += stats->bytesWritten;
	counters->totalReadTimeUs += stats->readTimeUs;
	counters->totalWriteTimeUs += stats->writeTimeUs;

	/*	Update caller tracking (hot spot identification).	*/

	bucket = callerHash(stats->callerFile, stats->callerLine);
	caller = &counters->callers[bucket];

	/*	Copy basename into shared memory (not pointer).		*/

	if (stats->callerFile != NULL)
	{
		const char	*basename;
		size_t		len;

		basename = strrchr(stats->callerFile, '/');
		basename = basename ? basename + 1 : stats->callerFile;
		len = strlen(basename);
		if (len >= SDR_PERF_FILENAME_LEN)
		{
			len = SDR_PERF_FILENAME_LEN - 1;
		}

		memcpy(caller->file, basename, len);
		caller->file[len] = '\0';
	}

	caller->line = stats->callerLine;
	caller->xnCount++;
}

void	sdr_perf_read_begin(SdrPerfStats *stats, struct timeval *start)
{
	(void) stats;	/*	Unused, but kept for consistency.	*/
	getCurrentTime(start);
}

void	sdr_perf_read_end(SdrPerfStats *stats, struct timeval *start,
		size_t bytes)
{
	struct timeval	end;
	long		diff;

	getCurrentTime(&end);
	stats->readCount++;
	stats->bytesRead += bytes;
	diff = SDR_PERF_TIME_DIFF_US(*start, end);
	if (diff > 0)
	{
		stats->readTimeUs += diff;
	}
}

void	sdr_perf_write_begin(SdrPerfStats *stats, struct timeval *start)
{
	(void) stats;	/*	Unused, but kept for consistency.	*/
	getCurrentTime(start);
}

void	sdr_perf_write_end(SdrPerfStats *stats, struct timeval *start,
		size_t bytes)
{
	struct timeval	end;
	long		diff;

	getCurrentTime(&end);
	stats->writeCount++;
	stats->bytesWritten += bytes;
	diff = SDR_PERF_TIME_DIFF_US(*start, end);
	if (diff > 0)
	{
		stats->writeTimeUs += diff;
	}
}

void	sdr_perf_report_counters(char *sdrName, SdrPerfCounters *counters)
{
	char		buf[120];
	unsigned long	avgXnTime = 0;
	int		i;
	SdrPerfCaller	*caller;
	int		callersReported = 0;

	(void) sdrName;	/*	May be used in future enhancements.	*/

	/*	Sanity check: detect uninitialized counters.
	 *	If avg time would exceed 1 hour (3600000000 us) or
	 *	read/write counts exceed plausible values, counters
	 *	are likely uninitialized garbage from old SDR layout.	*/

	if (counters->xnCount > 0)
	{
		avgXnTime = counters->totalXnTimeUs / counters->xnCount;
		if (avgXnTime > 3600000000UL)	/*	> 1 hour	*/
		{
			writeMemo("-- sdr performance counters --");
			writeMemo("[!] Counters appear uninitialized. \
Run 'killm' and restart ION.");
			return;
		}
	}
	else if (counters->totalXnTimeUs > 0
			|| counters->totalReadCount > 0
			|| counters->totalWriteCount > 0)
	{
		/*	Have activity but zero transactions = garbage.	*/

		writeMemo("-- sdr performance counters --");
		writeMemo("[!] Counters appear uninitialized. \
Run 'killm' and restart ION.");
		return;
	}

	/*	avgXnTime already computed above if xnCount > 0.	*/

	writeMemo("-- sdr performance counters --");
	isprintf(buf, sizeof(buf),
			"            transactions: %15lu",
			counters->xnCount);
	writeMemo(buf);
	isprintf(buf, sizeof(buf),
			"        avg xn time (us): %15lu",
			avgXnTime);
	writeMemo(buf);
	isprintf(buf, sizeof(buf),
			"        max xn time (us): %15lu",
			counters->maxXnTimeUs);
	writeMemo(buf);
	isprintf(buf, sizeof(buf),
			"    lock acquisitions: %15lu",
			counters->lockAcquireCount);
	writeMemo(buf);
	isprintf(buf, sizeof(buf),
			"   avg lock acq (us): %15lu",
			(counters->lockAcquireCount > 0) ?
			(counters->totalLockAcquireUs
			 / counters->lockAcquireCount) : 0);
	writeMemo(buf);
	isprintf(buf, sizeof(buf),
			"   max lock acq (us): %15lu",
			counters->maxLockAcquireUs);
	writeMemo(buf);
	/*	xn timer starts AFTER the lock is taken, so totalXnTimeUs is
	 *	lock-HOLD time and totalLockAcquireUs is the separate wait-to-
	 *	acquire time.  This ratio = fraction of per-transaction time
	 *	spent waiting for the lock (contention) vs. doing the work.	*/

	isprintf(buf, sizeof(buf),
			" lock wait %% of total: %14.1f%%",
			((counters->totalLockAcquireUs + counters->totalXnTimeUs)
			 > 0) ?
			(100.0 * counters->totalLockAcquireUs
			 / (counters->totalLockAcquireUs
			    + counters->totalXnTimeUs)) : 0.0);
	writeMemo(buf);
	if (counters->maxXnFile[0] != '\0')
	{
		isprintf(buf, sizeof(buf),
				"             max xn from: %s:%d (%lu calls, %.1f%%)",
				counters->maxXnFile, counters->maxXnLine,
				counters->maxXnCallerCount,
				(counters->xnCount > 0) ?
				(100.0 * counters->maxXnCallerCount
				 / counters->xnCount) : 0.0);
		writeMemo(buf);
	}
	isprintf(buf, sizeof(buf),
			"             total reads: %15lu (%lu bytes, %lu us)",
			counters->totalReadCount,
			(unsigned long) counters->totalBytesRead,
			counters->totalReadTimeUs);
	writeMemo(buf);
	isprintf(buf, sizeof(buf),
			"            total writes: %15lu (%lu bytes, %lu us)",
			counters->totalWriteCount,
			(unsigned long) counters->totalBytesWritten,
			counters->totalWriteTimeUs);
	writeMemo(buf);

	/*	Report top callers (hot spots).				*/

	writeMemo("-- top callers by transaction count --");
	for (i = 0; i < SDR_PERF_CALLER_BUCKETS; i++)
	{
		caller = &counters->callers[i];

		/*	Safety: skip empty or invalid entries.		*/

		if (caller->xnCount == 0)
		{
			continue;
		}

		if (caller->file[0] == '\0')
		{
			continue;
		}

		if (counters->xnCount > 0
				&& caller->xnCount > counters->xnCount)
		{
			continue;	/*	Invalid: count too high.*/
		}

		/*	Filename basename already stored in caller->file.*/

		isprintf(buf, sizeof(buf),
				"%10lu (%5.1f%%)  %s:%d",
				caller->xnCount,
				(counters->xnCount > 0) ?
				(100.0 * caller->xnCount
				 / counters->xnCount) : 0.0,
				caller->file, caller->line);
		writeMemo(buf);
		callersReported++;
	}

	if (callersReported == 0)
	{
		writeMemo("   (no caller data recorded)");
	}
}

void	sdr_perf_reset_counters(SdrPerfCounters *counters)
{
	memset(counters, 0, sizeof(SdrPerfCounters));
}

#else	/*	!SDR_PERF_INSTRUMENTATION	*/

/*	Provide a dummy symbol to avoid empty translation unit error.	*/

typedef int	sdr_perf_placeholder;

#endif	/*	SDR_PERF_INSTRUMENTATION	*/
