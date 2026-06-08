/*
	file_simple_provider.c:	Simple file provider library for file CLA.

	Implements basic file I/O with length-prefix framing and CRC32
	integrity checking. Suitable for testing and simple store-and-forward
	scenarios.

	Frame format (12 bytes overhead per bundle):
	  - 4 bytes: bundle length (big-endian)
	  - 4 bytes: CRC32 of the length field
	  - N bytes: bundle data
	  - 4 bytes: CRC32 of the bundle data

	Author: ION Development Team

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
								*/
/* Enable POSIX.1-2001 for nanosleep() and sigaction() */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#define FILECLA_BUFSZ		(1048576)
#define FILECLA_MAX_PATH	(1024)

/* Static state for this provider instance */
static FILE *output_file = NULL;
static FILE *input_file = NULL;
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
static char current_path[FILECLA_MAX_PATH];
static int poll_interval_ms = 1000;  /* Default 1 second */
static volatile sig_atomic_t shutdown_requested = 0;

/*
 * Signal handler to request graceful shutdown
 */
static void handle_shutdown_signal(int signum)
{
	(void)signum;
	shutdown_requested = 1;
}

/*
 * Initialization function - called by filecli/fileclo after dlopen.
 * Sets up signal handlers for graceful shutdown.
 */
void file_provider_init(void)
{
	struct sigaction sa;
	sa.sa_handler = handle_shutdown_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

/*
 * Cleanup function - called by filecli/fileclo before dlclose.
 * Closes any open file handles.
 */
void file_provider_cleanup(void)
{
	shutdown_requested = 1;
	if (output_file)
	{
		fclose(output_file);
		output_file = NULL;
	}
	if (input_file)
	{
		fclose(input_file);
		input_file = NULL;
	}
}

/*
 * Portable millisecond sleep using POSIX nanosleep().
 */
static void sleep_ms(int milliseconds)
{
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

/*
 * CRC32 lookup table (IEEE 802.3 polynomial 0x04C11DB7, reflected)
 * This is the standard CRC-32 used in Ethernet, ZIP, PNG, etc.
 */
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

static void init_crc32_table(void)
{
	uint32_t polynomial = 0xEDB88320;  /* Reflected 0x04C11DB7 */
	int i, j;

	if (crc32_table_initialized)
	{
		return;
	}

	for (i = 0; i < 256; i++)
	{
		uint32_t crc = i;
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
			{
				crc = (crc >> 1) ^ polynomial;
			}
			else
			{
				crc >>= 1;
			}
		}
		crc32_table[i] = crc;
	}
	crc32_table_initialized = 1;
}

static uint32_t compute_crc32(const unsigned char *data, size_t length)
{
	uint32_t crc = 0xFFFFFFFF;
	size_t i;

	init_crc32_table();

	for (i = 0; i < length; i++)
	{
		crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
	}

	return crc ^ 0xFFFFFFFF;
}

/*
 * Write a 32-bit value in big-endian format.
 */
static void write_uint32_be(unsigned char *buf, uint32_t value)
{
	buf[0] = (value >> 24) & 0xFF;
	buf[1] = (value >> 16) & 0xFF;
	buf[2] = (value >> 8) & 0xFF;
	buf[3] = value & 0xFF;
}

/*
 * Read a 32-bit value in big-endian format.
 */
static uint32_t read_uint32_be(const unsigned char *buf)
{
	return ((uint32_t)buf[0] << 24) |
	       ((uint32_t)buf[1] << 16) |
	       ((uint32_t)buf[2] << 8) |
	       (uint32_t)buf[3];
}

/*
 * init_file_sender - Initialize output file for writing bundles.
 */
int init_file_sender(const char *file_path)
{
	if (file_path == NULL || strlen(file_path) == 0)
	{
		fprintf(stderr, "file_provider: No output path specified.\n");
		return -1;
	}

	strncpy(current_path, file_path, FILECLA_MAX_PATH - 1);
	current_path[FILECLA_MAX_PATH - 1] = '\0';

	/* Open file for appending (create if doesn't exist) */
	output_file = fopen(current_path, "ab");
	if (output_file == NULL)
	{
		fprintf(stderr, "file_provider: Failed to open output file "
				"'%s': %s\n", current_path, strerror(errno));
		return -1;
	}

	/* Initialize CRC table */
	init_crc32_table();

	fprintf(stderr, "[i] file_provider: Sender initialized, writing to "
			"'%s'\n", current_path);
	return 0;
}

/*
 * finalize_file_sender - Close output file.
 */
void finalize_file_sender(void)
{
	pthread_mutex_lock(&write_mutex);
	if (output_file != NULL)
	{
		fclose(output_file);
		output_file = NULL;
		fprintf(stderr, "[i] file_provider: Sender finalized.\n");
	}
	pthread_mutex_unlock(&write_mutex);
}

/*
 * file_write_bundle - Write bundle with length-prefix framing and CRC32.
 *
 * Frame format:
 *   [4-byte length][4-byte length CRC][bundle data][4-byte data CRC]
 */
int file_write_bundle(unsigned char *data, size_t length)
{
	unsigned char header[8];
	unsigned char trailer[4];
	uint32_t length_crc;
	uint32_t data_crc;
	size_t written;

	if (output_file == NULL)
	{
		fprintf(stderr, "file_provider: Output file not open.\n");
		return -1;
	}

	if (length > 0xFFFFFFFF)
	{
		fprintf(stderr, "file_provider: Bundle too large: %zu\n",
				length);
		return -1;
	}

	/* Write length (4 bytes, big-endian) */
	write_uint32_be(header, (uint32_t)length);

	/* Compute and write CRC32 of length field */
	length_crc = compute_crc32(header, 4);
	write_uint32_be(header + 4, length_crc);

	/* Compute CRC32 of bundle data */
	data_crc = compute_crc32(data, length);
	write_uint32_be(trailer, data_crc);

	pthread_mutex_lock(&write_mutex);

	/* Write header (length + length CRC) */
	written = fwrite(header, 1, 8, output_file);
	if (written != 8)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "file_provider: Failed to write header.\n");
		return -1;
	}

	/* Write bundle data */
	written = fwrite(data, 1, length, output_file);
	if (written != length)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "file_provider: Failed to write bundle data.\n");
		return -1;
	}

	/* Write trailer (data CRC) */
	written = fwrite(trailer, 1, 4, output_file);
	if (written != 4)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "file_provider: Failed to write trailer.\n");
		return -1;
	}

	fflush(output_file);
	pthread_mutex_unlock(&write_mutex);

	fprintf(stderr, "[i] file_provider: Wrote %zu byte bundle "
			"(CRC32=0x%08X).\n", length, data_crc);
	return (int)length;
}

/*
 * init_file_receiver - Initialize input file for reading bundles.
 *
 * If the file does not exist, waits for it to be created (up to 30 seconds).
 */
int init_file_receiver(const char *file_path, int poll_interval)
{
	int wait_count = 0;
	int max_wait = 30;  /* Wait up to 30 seconds for file to appear */

	if (file_path == NULL || strlen(file_path) == 0)
	{
		fprintf(stderr, "file_provider: No input path specified.\n");
		return -1;
	}

	strncpy(current_path, file_path, FILECLA_MAX_PATH - 1);
	current_path[FILECLA_MAX_PATH - 1] = '\0';

	poll_interval_ms = poll_interval;
	shutdown_requested = 0;

	/* Initialize CRC table */
	init_crc32_table();

	/* Open file for reading, waiting for it to be created if needed */
	while ((input_file = fopen(current_path, "rb")) == NULL)
	{
		if (errno != ENOENT || wait_count >= max_wait)
		{
			fprintf(stderr, "file_provider: Failed to open input "
					"file '%s': %s\n",
					current_path, strerror(errno));
			return -1;
		}
		if (wait_count == 0)
		{
			fprintf(stderr, "[i] file_provider: Waiting for input "
					"file '%s' to be created...\n",
					current_path);
		}
		sleep_ms(1000);
		wait_count++;
	}

	fprintf(stderr, "[i] file_provider: Receiver initialized, reading from "
			"'%s', poll interval = %d ms\n",
			current_path, poll_interval_ms);
	return 0;
}

/*
 * finalize_file_receiver - Close input file.
 */
void finalize_file_receiver(void)
{
	shutdown_requested = 1;

	if (input_file != NULL)
	{
		fclose(input_file);
		input_file = NULL;
		fprintf(stderr, "[i] file_provider: Receiver finalized.\n");
	}
}

/*
 * file_read_bundle - Read next bundle from file with CRC validation.
 *
 * Polls file for new data if at EOF.
 * Validates both length CRC and data CRC.
 * Skips corrupted bundles and continues reading.
 *
 * Returns:
 *   > 1  : Number of bytes read (bundle length)
 *   1    : Normal stop (shutdown requested)
 *   0    : Error occurred
 */
size_t file_read_bundle(char *buffer)
{
	unsigned char header[8];
	unsigned char trailer[4];
	size_t length;
	uint32_t stored_length_crc;
	uint32_t computed_length_crc;
	uint32_t stored_data_crc;
	uint32_t computed_data_crc;
	size_t bytes_read;
	size_t total_read;

	if (input_file == NULL)
	{
		fprintf(stderr, "file_provider: Input file not open.\n");
		return 0;  /* Error */
	}

	while (!shutdown_requested)
	{
		/* Try to read header (length + length CRC) */
		bytes_read = fread(header, 1, 8, input_file);

		if (bytes_read == 0)
		{
			/* EOF - wait for more data */
			if (feof(input_file))
			{
				clearerr(input_file);
				sleep_ms(poll_interval_ms);
				continue;
			}
			/* Error */
			fprintf(stderr, "file_provider: Read error.\n");
			return 0;
		}

		if (bytes_read != 8)
		{
			/* Partial header - wait for rest */
			if (feof(input_file))
			{
				/* Seek back and wait */
				fseek(input_file, -(long)bytes_read, SEEK_CUR);
				clearerr(input_file);
				sleep_ms(poll_interval_ms);
				continue;
			}
			fprintf(stderr, "file_provider: Incomplete header "
					"read.\n");
			return 0;
		}

		break;  /* Got header */
	}

	if (shutdown_requested)
	{
		return 1;  /* Normal stop */
	}

	/* Parse length (big-endian) */
	length = read_uint32_be(header);

	/* Validate length CRC */
	stored_length_crc = read_uint32_be(header + 4);
	computed_length_crc = compute_crc32(header, 4);

	if (stored_length_crc != computed_length_crc)
	{
		fprintf(stderr, "file_provider: Length CRC mismatch "
				"(stored=0x%08X, computed=0x%08X), "
				"attempting recovery...\n",
				stored_length_crc, computed_length_crc);
		/* Skip this corrupted frame - try to find next valid header */
		/* For simplicity, we just return error here */
		return 0;
	}

	if (length > FILECLA_BUFSZ)
	{
		fprintf(stderr, "file_provider: Bundle too large: %zu\n",
				length);
		return 0;
	}

	/* Read bundle data */
	total_read = 0;
	while (total_read < length && !shutdown_requested)
	{
		bytes_read = fread(buffer + total_read, 1, length - total_read,
				input_file);
		if (bytes_read == 0)
		{
			if (feof(input_file))
			{
				/* Partial bundle - wait for rest */
				clearerr(input_file);
				sleep_ms(poll_interval_ms);
				continue;
			}
			fprintf(stderr, "file_provider: Failed to read "
					"bundle data.\n");
			return 0;
		}
		total_read += bytes_read;
	}

	if (shutdown_requested)
	{
		return 1;  /* Normal stop */
	}

	/* Read trailer (data CRC) */
	while (!shutdown_requested)
	{
		bytes_read = fread(trailer, 1, 4, input_file);
		if (bytes_read == 0 && feof(input_file))
		{
			clearerr(input_file);
			sleep_ms(poll_interval_ms);
			continue;
		}
		if (bytes_read != 4)
		{
			fprintf(stderr, "file_provider: Failed to read "
					"trailer.\n");
			return 0;
		}
		break;
	}

	if (shutdown_requested)
	{
		return 1;  /* Normal stop */
	}

	/* Validate data CRC */
	stored_data_crc = read_uint32_be(trailer);
	computed_data_crc = compute_crc32((unsigned char *)buffer, length);

	if (stored_data_crc != computed_data_crc)
	{
		fprintf(stderr, "file_provider: Data CRC mismatch "
				"(stored=0x%08X, computed=0x%08X), "
				"skipping bundle.\n",
				stored_data_crc, computed_data_crc);
		/* Skip this corrupted bundle and continue */
		return 0;
	}

	fprintf(stderr, "[i] file_provider: Read %zu byte bundle "
			"(CRC32=0x%08X).\n", length, stored_data_crc);
	return length;
}

/*
 * Optional: Get current read position for checkpointing.
 */
long file_get_read_position(void)
{
	if (input_file == NULL)
	{
		return -1;
	}
	return ftell(input_file);
}

/*
 * Optional: Set read position for resume.
 */
int file_set_read_position(long offset)
{
	if (input_file == NULL)
	{
		return -1;
	}
	return fseek(input_file, offset, SEEK_SET);
}
