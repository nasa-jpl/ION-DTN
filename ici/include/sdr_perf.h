/*
 *	sdr_perf.h:	SDR Performance Instrumentation.
 *
 *	Compile with -DSDR_PERF_INSTRUMENTATION to enable.
 *
 *	Copyright (c) 2026, California Institute of Technology.
 *	All rights reserved.
 */

#ifndef _SDR_PERF_H_
#define _SDR_PERF_H_

#include "platform.h"

#ifdef SDR_PERF_INSTRUMENTATION

#define SDR_PERF_CALLER_BUCKETS  128	/*	Hash table for caller tracking	*/
#define SDR_PERF_FILENAME_LEN   32	/*	Max filename length to store	*/

/*	Per-caller statistics for hot spot identification.		*/

typedef struct
{
	char		file[SDR_PERF_FILENAME_LEN];	/*	Basename.	*/
	int		line;		/*	Source line number.	*/
	unsigned long	xnCount;	/*	Transactions from caller.*/
} SdrPerfCaller;

/*	Per-transaction statistics (stored in SdrView, process-private)	*/

typedef struct
{
	struct timeval	xnStartTime;
	struct timeval	xnEndTime;
	unsigned long	readCount;
	unsigned long	writeCount;
	size_t		bytesRead;
	size_t		bytesWritten;
	unsigned long	readTimeUs;
	unsigned long	writeTimeUs;
	const char	*callerFile;	/*	For caller tracking.	*/
	int		callerLine;
} SdrPerfStats;

/*	Cumulative counters (stored in SdrState, shared memory).	*/

typedef struct
{
	unsigned long	xnCount;
	unsigned long	totalXnTimeUs;
	unsigned long	maxXnTimeUs;
	unsigned long	totalLockAcquireUs;	/*	Time blocked on lock.	*/
	unsigned long	lockAcquireCount;	/*	Lock acquisitions.	*/
	unsigned long	maxLockAcquireUs;	/*	Worst single acquire.	*/
	char		maxXnFile[SDR_PERF_FILENAME_LEN];	/*	Caller of max xn.	*/
	int		maxXnLine;
	unsigned long	maxXnCallerCount;	/*	Xn count for max-time caller.	*/
	unsigned long	totalReadCount;
	unsigned long	totalWriteCount;
	size_t		totalBytesRead;
	size_t		totalBytesWritten;
	unsigned long	totalReadTimeUs;
	unsigned long	totalWriteTimeUs;
	SdrPerfCaller	callers[SDR_PERF_CALLER_BUCKETS];
} SdrPerfCounters;

/*	Helper macro for time difference in microseconds (signed).
 *	Result is a long; callers must clamp negative values to 0
 *	since gettimeofday is not monotonic and may go backwards.	*/

#define SDR_PERF_TIME_DIFF_US(start, end) \
	(((long)((end).tv_sec - (start).tv_sec)) * 1000000L + \
	 ((long)(end).tv_usec - (long)(start).tv_usec))

/*	Function prototypes.						*/

extern void	sdr_perf_xn_begin(SdrPerfStats *stats, const char *file,
			int line);
extern void	sdr_perf_xn_end(SdrPerfCounters *counters,
			SdrPerfStats *stats);
extern void	sdr_perf_read_begin(SdrPerfStats *stats,
			struct timeval *start);
extern void	sdr_perf_read_end(SdrPerfStats *stats,
			struct timeval *start, size_t bytes);
extern void	sdr_perf_write_begin(SdrPerfStats *stats,
			struct timeval *start);
extern void	sdr_perf_write_end(SdrPerfStats *stats,
			struct timeval *start, size_t bytes);
extern void	sdr_perf_report_counters(char *sdrName,
			SdrPerfCounters *counters);
extern void	sdr_perf_reset_counters(SdrPerfCounters *counters);

/*	Instrumentation macros.						*/

#define SDR_PERF_XN_BEGIN(stats, file, line) \
		sdr_perf_xn_begin(stats, file, line)
#define SDR_PERF_XN_END(counters, stats) \
		sdr_perf_xn_end(counters, stats)
#define SDR_PERF_READ_BEGIN(stats, start) \
		sdr_perf_read_begin(stats, start)
#define SDR_PERF_READ_END(stats, start, bytes) \
		sdr_perf_read_end(stats, start, bytes)
#define SDR_PERF_WRITE_BEGIN(stats, start) \
		sdr_perf_write_begin(stats, start)
#define SDR_PERF_WRITE_END(stats, start, bytes) \
		sdr_perf_write_end(stats, start, bytes)
#define SDR_PERF_REPORT(name, counters) \
		sdr_perf_report_counters(name, counters)
#define SDR_PERF_RESET(counters) \
		sdr_perf_reset_counters(counters)

#else	/*	SDR_PERF_INSTRUMENTATION not defined			*/

/*	No-op macros when instrumentation disabled.			*/

#define SDR_PERF_XN_BEGIN(stats, file, line)	((void)0)
#define SDR_PERF_XN_END(counters, stats)	((void)0)
#define SDR_PERF_READ_BEGIN(stats, start)	((void)0)
#define SDR_PERF_READ_END(stats, start, bytes)	((void)0)
#define SDR_PERF_WRITE_BEGIN(stats, start)	((void)0)
#define SDR_PERF_WRITE_END(stats, start, bytes)	((void)0)
#define SDR_PERF_REPORT(name, counters)		((void)0)
#define SDR_PERF_RESET(counters)		((void)0)

#endif	/*	SDR_PERF_INSTRUMENTATION				*/

#endif	/*	_SDR_PERF_H_						*/
