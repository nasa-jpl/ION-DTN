/*
 *	bpinspect_ops.c - Bundle operations implementation
 *
 *	Copyright (c) 2025, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: ION Development Team
 */

#include "bpinspect_ops.h"
#include <bei.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/*
 * Cancel a single bundle by cache entry
 */
int bpinspect_ops_cancel_bundle(const BundleCacheEntry *entry)
{
	Sdr		sdr;
	SdrObject	bundleObj;
	BpTimestamp	creationTime;
	Bundle		bundle;
	int		destroyResult;
	char		diagBuf[512];

	if (entry == NULL)
	{
		return -1;
	}

	sdr = bp_get_sdr();

	/* Construct creation timestamp for findBundle */
	creationTime.msec = entry->creationMsec;
	creationTime.count = entry->creationCount;

	/* Begin transaction BEFORE calling findBundle (requires ion lock) */
	CHKERR(sdr_begin_xn(sdr));

	/* Re-validate that bundle still exists (safe against race conditions) */
	if (findBundle((char *)(uintptr_t)entry->source, &creationTime,
			entry->fragmentOffset, entry->fragmentLength,
			&bundleObj) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't search for bundle.", entry->source);
		return -1;
	}

	if (bundleObj == 0)
	{
		/* Bundle no longer exists - already destroyed */
		sdr_cancel_xn(sdr);
		snprintf(diagBuf, sizeof(diagBuf),
			"Bundle %s [%llu.%u] no longer exists (already destroyed)",
			entry->source,
			(unsigned long long) entry->creationMsec,
			entry->creationCount);
		writeMemo(diagBuf);
		return 0;	/* Not an error - bundle is gone */
	}

	/* Read bundle state before cancellation for diagnostics */
	sdr_read(sdr, (char *) &bundle, bundleObj, sizeof(Bundle));
	snprintf(diagBuf, sizeof(diagBuf),
		"Canceling bundle %s [%llu.%u]: queue state before: "
		"fwd=%lu dlv=%lu xmit=%lu plan=%lu duct=%lu detained=%d",
		entry->source,
		(unsigned long long) entry->creationMsec,
		entry->creationCount,
		(unsigned long) bundle.fwdQueueElt,
		(unsigned long) bundle.dlvQueueElt,
		(unsigned long) bundle.transitElt,
		(unsigned long) bundle.planXmitElt,
		(unsigned long) bundle.ductXmitElt,
		bundle.detained);
	writeMemo(diagBuf);

	destroyResult = bpDestroyBundle(bundleObj, 3);  /* 3 = canceled */

	if (destroyResult < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("bpDestroyBundle failed with error.", entry->source);
		return -1;
	}

	/*	Verify destruction by searching for the bundle again.
	 *	Do NOT read bundleObj directly -- it may have been freed
	 *	by bpDestroyBundle, and sdr_stage/sdr_read on a freed
	 *	object will crash the transaction under SDR_BOUNDED.	*/

	bundleObj = 0;
	if (findBundle((char *)(uintptr_t)entry->source, &creationTime,
			entry->fragmentOffset, entry->fragmentLength,
			&bundleObj) < 0)
	{
		sdr_cancel_xn(sdr);
		putErrmsg("Can't verify bundle destruction.", entry->source);
		return -1;
	}

	if (bundleObj != 0)
	{
		snprintf(diagBuf, sizeof(diagBuf),
			"Bundle %s [%llu.%u] not yet destroyed "
			"(deferred by remaining references)",
			entry->source,
			(unsigned long long) entry->creationMsec,
			entry->creationCount);
		writeMemo(diagBuf);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		putErrmsg("Can't complete bundle cancellation.", entry->source);
		return -1;
	}

	/* Return success if bpDestroyBundle didn't error, even if bundle
	 * wasn't fully destroyed (it may be queued for later destruction) */
	return 0;
}

/*
 * Cancel multiple bundles from an array of cache entries
 */
int bpinspect_ops_cancel_bundles(const BundleCacheEntry *entries, int count)
{
	int	i;
	int	canceledCount = 0;

	if (entries == NULL || count <= 0)
	{
		return -1;
	}

	for (i = 0; i < count; i++)
	{
		if (bpinspect_ops_cancel_bundle(&entries[i]) == 0)
		{
			canceledCount++;
		}
		else
		{
			/* Log error but continue with remaining bundles */
			char	buf[256];
			snprintf(buf, sizeof(buf),
				"Failed to cancel bundle %d of %d", i + 1, count);
			writeMemo(buf);
		}
	}

	return canceledCount;
}

/*
 * Suspend a bundle by cache entry
 */
int bpinspect_ops_suspend_bundle(const BundleCacheEntry *entry)
{
	if (entry == NULL)
	{
		return -1;
	}

	if (bp_suspend(entry->bundleObj) < 0)
	{
		putErrmsg("Can't suspend bundle.", entry->source);
		return -1;
	}

	return 0;
}

/*
 * Resume a bundle by cache entry
 */
int bpinspect_ops_resume_bundle(const BundleCacheEntry *entry)
{
	int	result;
#if BPDEBUG
	char	msg[256];
#endif

	if (entry == NULL)
	{
#if BPDEBUG
		writeMemo("[!] bpinspect_ops_resume_bundle: NULL entry");
#endif
		return -1;
	}

#if BPDEBUG
	snprintf(msg, sizeof(msg), "[i] bpinspect_ops_resume_bundle: bundleObj=%lu, source=%s",
		 (unsigned long)entry->bundleObj, entry->source);
	writeMemo(msg);
#endif

	result = bp_resume(entry->bundleObj);

#if BPDEBUG
	snprintf(msg, sizeof(msg), "[i] bp_resume returned: %d", result);
	writeMemo(msg);
#endif

	if (result < 0)
	{
		putErrmsg("Can't resume bundle.", entry->source);
		return -1;
	}

	return 0;
}

/*
 * Suspend multiple bundles from an array of cache entries
 */
int bpinspect_ops_suspend_bundles(const BundleCacheEntry *entries, int count)
{
	int	i;
	int	suspendedCount = 0;

	if (entries == NULL || count <= 0)
	{
		return -1;
	}

	for (i = 0; i < count; i++)
	{
		if (bpinspect_ops_suspend_bundle(&entries[i]) == 0)
		{
			suspendedCount++;
		}
		else
		{
			/* Log error but continue with remaining bundles */
			char	buf[256];
			snprintf(buf, sizeof(buf),
				"Failed to suspend bundle %d of %d", i + 1, count);
			writeMemo(buf);
		}
	}

	return suspendedCount;
}

/*
 * Resume multiple bundles from an array of cache entries
 */
int bpinspect_ops_resume_bundles(const BundleCacheEntry *entries, int count)
{
	int	i;
	int	resumedCount = 0;

	if (entries == NULL || count <= 0)
	{
		return -1;
	}

	for (i = 0; i < count; i++)
	{
		if (bpinspect_ops_resume_bundle(&entries[i]) == 0)
		{
			resumedCount++;
		}
		else
		{
			/* Log error but continue with remaining bundles */
			char	buf[256];
			snprintf(buf, sizeof(buf),
				"Failed to resume bundle %d of %d", i + 1, count);
			writeMemo(buf);
		}
	}

	return resumedCount;
}

/*
 * Format bundle ID as string
 */
char* bpinspect_ops_format_bundle_id(const BundleCacheEntry *entry, char *buf, int len)
{
	if (entry == NULL || buf == NULL || len <= 0)
	{
		return NULL;
	}

	if (entry->fragmentOffset > 0 || entry->fragmentLength > 0)
	{
		/* Fragment */
		snprintf(buf, len, "%s [%llu.%u] frag[%u:%u]",
			entry->source,
			(unsigned long long) entry->creationMsec,
			entry->creationCount,
			entry->fragmentOffset,
			entry->fragmentLength);
	}
	else
	{
		/* Non-fragment */
		snprintf(buf, len, "%s [%llu.%u]",
			entry->source,
			(unsigned long long) entry->creationMsec,
			entry->creationCount);
	}

	return buf;
}

/*
 * Format timestamp as human-readable string
 */
static char* format_timestamp(uvast msec, char *buf, int len)
{
	time_t		unixTime;
	struct tm	*tm;
	int		milliseconds;

	/* Convert from BPv7 epoch (2000) to Unix epoch (1970) */
	unixTime = (msec / 1000) + 946684800;
	milliseconds = msec % 1000;

	tm = gmtime(&unixTime);
	if (tm != NULL)
	{
		snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d.%03d UTC",
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec,
			milliseconds);
	}
	else
	{
		snprintf(buf, len, "%llu msec", (unsigned long long) msec);
	}

	return buf;
}

/*
 * Format expiration time
 */
static char* format_expiration(time_t expirationTime, int timeRemaining, char *buf, int len)
{
	struct tm	*tm;

	if (timeRemaining < 0)
	{
		snprintf(buf, len, "EXPIRED");
	}
	else
	{
		tm = gmtime(&expirationTime);
		if (tm != NULL)
		{
			snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d UTC (%d sec)",
				tm->tm_year + 1900,
				tm->tm_mon + 1,
				tm->tm_mday,
				tm->tm_hour,
				tm->tm_min,
				tm->tm_sec,
				timeRemaining);
		}
		else
		{
			snprintf(buf, len, "%ld (%d sec)",
				(long) expirationTime, timeRemaining);
		}
	}

	return buf;
}

/*
 * Format size with units
 */
static char* format_size(vast bytes, char *buf, int len)
{
	if (bytes < 1024)
	{
		snprintf(buf, len, "%lld B", (long long) bytes);
	}
	else if (bytes < 1024 * 1024)
	{
		snprintf(buf, len, "%.1f KB", (double) bytes / 1024.0);
	}
	else if (bytes < 1024 * 1024 * 1024)
	{
		snprintf(buf, len, "%.1f MB", (double) bytes / (1024.0 * 1024.0));
	}
	else
	{
		snprintf(buf, len, "%.1f GB", (double) bytes / (1024.0 * 1024.0 * 1024.0));
	}

	return buf;
}

/*
 * Get priority name
 */
static const char* get_priority_name(unsigned char priority)
{
	switch (priority)
	{
	case 0:
		return "Bulk";
	case 1:
		return "Standard";
	case 2:
		return "Urgent";
	default:
		return "Unknown";
	}
}

/*
 * Print bundle details to stdout
 */
void bpinspect_ops_print_bundle(const BundleCacheEntry *entry, int verbose)
{
	char	buf[512];
	char	timeBuf[64];
	char	sizeBuf[32];

	if (entry == NULL)
	{
		return;
	}

	/* Basic information (always shown) */
	printf("\n");
	printf("Bundle ID:    %s\n",
		bpinspect_ops_format_bundle_id(entry, buf, sizeof(buf)));
	printf("Source:       %s\n", entry->source);
	printf("Destination:  %s\n", entry->dest);
	printf("Created:      %s\n",
		format_timestamp(entry->creationMsec, timeBuf, sizeof(timeBuf)));
	printf("Expires:      %s\n",
		format_expiration(entry->expirationTime, entry->timeRemaining,
				timeBuf, sizeof(timeBuf)));
	printf("Payload Size: %s\n",
		format_size(entry->payloadLength, sizeBuf, sizeof(sizeBuf)));
	printf("Priority:     %s\n", get_priority_name(entry->priority));
	printf("Queue State:  %s\n", entry->queueState);

	/* Detailed information (verbose >= 1) */
	if (verbose >= 1)
	{
		printf("Total ADU:    %u bytes\n", entry->totalAduLength);
		printf("Flags:        0x%02x", entry->bundleProcFlags);

		if (entry->bundleProcFlags & BDL_IS_ADMIN)
			printf(" [ADMIN]");
		if (entry->bundleProcFlags & BDL_IS_FRAGMENT)
			printf(" [FRAGMENT]");
		if (entry->bundleProcFlags & BDL_APP_ACK_REQUEST)
			printf(" [ACK-REQ]");

		printf("\n");

		if (entry->fragmentOffset > 0 || entry->fragmentLength > 0)
		{
			printf("Fragment:     Offset=%u Length=%u\n",
				entry->fragmentOffset, entry->fragmentLength);
		}
	}

	/* Detailed SDR information (verbose >= 2) */
	if (verbose >= 2)
	{
		Sdr	sdr;
		Bundle	bundle;

		sdr = bp_get_sdr();
		CHKVOID(sdr_begin_xn(sdr));
		sdr_read(sdr, (char *) &bundle, entry->bundleObj, sizeof(Bundle));

		printf("\nExtended Information:\n");
		printf("  SDR Address:      " UVAST_FIELDSPEC "\n",
			(uvast) entry->bundleObj);
		printf("  Extension Blocks: %d\n",
			(int) sdr_list_length(sdr, bundle.extensions));
		printf("  Payload CRC Type: %d\n", bundle.payload.crcType);
		printf("  Primary CRC Type: %d\n", bundle.primaryBlkCrcType);

		/* Show extension block types */
		if (sdr_list_length(sdr, bundle.extensions) > 0)
		{
			SdrObject	elt;
			SdrObject	blkAddr;
			ExtensionBlock	blk;
			int		i = 0;

			printf("  Extension Block Types: ");
			for (elt = sdr_list_first(sdr, bundle.extensions); elt;
					elt = sdr_list_next(sdr, elt))
			{
				blkAddr = sdr_list_data(sdr, elt);
				sdr_read(sdr, (char *) &blk, blkAddr,
					 sizeof(ExtensionBlock));
				if (i > 0) printf(", ");
				printf("%u", blk.type);
				i++;
			}
			printf("\n");
		}

		sdr_exit_xn(sdr);
	}

	printf("\n");
}

/*
 * Export bundle details to a file
 */
int bpinspect_ops_export_bundle(const BundleCacheEntry *entry,
				const char *filename,
				int verbose)
{
	FILE	*originalStdout;

	if (entry == NULL)
	{
		return -1;
	}

	/* If no filename specified, print to stdout */
	if (filename == NULL)
	{
		bpinspect_ops_print_bundle(entry, verbose);
		return 0;
	}

	/* Redirect stdout to file using freopen */
	fflush(stdout);
	originalStdout = freopen(filename, "a", stdout);
	if (originalStdout == NULL)
	{
		putErrmsg("Can't redirect stdout to export file.", filename);
		return -1;
	}

	/* Print bundle details */
	bpinspect_ops_print_bundle(entry, verbose);

	/* Restore stdout to terminal - reopen /dev/tty */
	fflush(stdout);
	if (freopen("/dev/tty", "w", stdout) == NULL)
	{
		/* If /dev/tty fails, we can't really recover stdout */
		putErrmsg("Can't restore stdout.", NULL);
		return -1;
	}

	return 0;
}
