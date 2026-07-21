/*
	filecla.h:	common definitions for File convergence layer
			adapter modules.

	Author: ION Development Team

	Modification History:
	Date  Who What

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef FILECLA_H
#define FILECLA_H

#include "bpP.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Maximum buffer size for file CLA operations.
 * Can be increased if needed for very large bundles.
 */
#define FILECLA_BUFSZ		(1048576)	/* 1MB default */

/*
 * Maximum path length for file paths.
 */
#define FILECLA_MAX_PATH	(1024)

/*
 * Frame overhead: 4-byte length + 4-byte length CRC + 4-byte data CRC = 12 bytes
 */
#define FILECLA_FRAME_OVERHEAD	(12)

/*
 * Function pointer types for file provider library.
 * The provider library implements the interface to the underlying
 * file storage layer.
 */

/*
 * Called once at startup to initialize sender resources.
 * Parameters:
 *   file_path - Path to the output file
 * Returns: 0 on success, -1 on error
 */
typedef int (*init_file_sender_ptr)(const char *file_path);

/*
 * Called once at shutdown to clean up sender resources.
 */
typedef void (*finalize_file_sender_ptr)(void);

/*
 * Called for each bundle to write to file.
 * The provider is responsible for:
 *   - Writing the length-prefix header (4 bytes, big-endian)
 *   - Writing the length CRC32 (4 bytes)
 *   - Writing the bundle data
 *   - Writing the data CRC32 (4 bytes)
 *   - Thread safety if multiple writers possible
 *
 * Parameters:
 *   data   - Bundle data to write
 *   length - Length of bundle data in bytes
 * Returns: Number of bytes written (bundle data only), or -1 on error
 */
typedef int (*file_write_bundle_ptr)(unsigned char *data, size_t length);

/*
 * Called once at startup to initialize receiver resources.
 * Parameters:
 *   file_path     - Path to the input file
 *   poll_interval - Polling interval in milliseconds (0 for blocking)
 * Returns: 0 on success, -1 on error
 */
typedef int (*init_file_receiver_ptr)(const char *file_path, int poll_interval);

/*
 * Called once at shutdown to clean up receiver resources.
 */
typedef void (*finalize_file_receiver_ptr)(void);

/*
 * Blocking call that waits for and returns the next bundle from file.
 * The provider is responsible for:
 *   - Reading and validating the length header and its CRC32
 *   - Reading the bundle data
 *   - Validating the data CRC32
 *   - Polling/waiting for new data if at EOF
 *   - Skipping corrupted bundles (CRC mismatch)
 *
 * Parameters:
 *   buffer - Buffer to receive bundle data (must be at least FILECLA_BUFSZ)
 * Returns:
 *   > 1  : Number of bytes read (bundle length)
 *   1    : Normal stop (shutdown requested)
 *   0    : Error occurred
 */
typedef size_t (*file_read_bundle_ptr)(char *buffer);

/*
 * Configuration structure for file CLA sender (CLO).
 */
struct FileCloConfig {
	char			file_path[FILECLA_MAX_PATH];
	file_write_bundle_ptr	write_bundle;
	init_file_sender_ptr	init_sender;
	finalize_file_sender_ptr finalize_sender;
};

/*
 * Configuration structure for file CLA receiver (CLI).
 */
struct FileCliConfig {
	char			file_path[FILECLA_MAX_PATH];
	int			poll_interval;
	file_read_bundle_ptr	read_bundle;
	init_file_receiver_ptr	init_receiver;
	finalize_file_receiver_ptr finalize_receiver;
};

extern int	sendBytesByFile(unsigned char *buffer, size_t length,
			struct FileCloConfig *cfg);
extern int	sendBundleByFile(unsigned int bundleLength, SdrObject bundleZco,
			unsigned char *buffer, struct FileCloConfig *cfg);

#ifdef __cplusplus
}
#endif

#endif /* FILECLA_H */
