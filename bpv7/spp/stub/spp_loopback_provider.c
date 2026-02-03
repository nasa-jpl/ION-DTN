/*
	spp_loopback_provider.c:	Loopback SPP provider library for testing.
				Uses a named pipe (FIFO) for loopback communication.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	This provider library implements loopback functionality for testing
	the SPP CLA without real space link hardware. Data sent via
	packet_request is written to a FIFO and can be read back via
	packet_indication.

									*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#define LOOPBACK_FIFO_PATH "/tmp/spp_loopback_fifo"
#define MAX_PACKET_SIZE (65536)

/* File descriptors for FIFO */
static int write_fd = -1;
static int read_fd = -1;
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * init_space_packet_sender - Initialize the SPP sender.
 * Creates the FIFO and opens it for writing.
 *
 * Uses O_RDWR to avoid blocking - a FIFO opened with O_WRONLY blocks
 * until a reader opens the other end, but O_RDWR opens immediately.
 * This allows the sender and receiver processes to start independently.
 */
void init_space_packet_sender(void)
{
	/* Create FIFO if it doesn't exist */
	if (mkfifo(LOOPBACK_FIFO_PATH, 0666) < 0 && errno != EEXIST)
	{
		fprintf(stderr, "spp_loopback: Failed to create FIFO: %s\n",
				strerror(errno));
		return;
	}

	/* Open FIFO with O_RDWR to avoid blocking on open.
	 * O_WRONLY would block until a reader opens the FIFO,
	 * but O_RDWR opens immediately even with no reader. */
	write_fd = open(LOOPBACK_FIFO_PATH, O_RDWR);
	if (write_fd >= 0)
	{
		fprintf(stderr, "[i] spp_loopback: Sender initialized.\n");
	}
	else
	{
		fprintf(stderr, "spp_loopback: Failed to open FIFO for writing: %s\n",
				strerror(errno));
	}
}

/*
 * finalize_space_packet_sender - Clean up the SPP sender.
 */
void finalize_space_packet_sender(void)
{
	if (write_fd >= 0)
	{
		close(write_fd);
		write_fd = -1;
		fprintf(stderr, "[i] spp_loopback: Sender finalized.\n");
	}
}

/*
 * packet_request - Send a space packet.
 *
 * Writes the packet to the FIFO with a simple framing format:
 *   - 4 bytes: total packet length (big-endian)
 *   - N bytes: SPP header + bundle data
 *
 * Parameters:
 *   buffer          - Buffer containing SPP header + bundle data
 *   apid            - Application Process ID
 *   seq_count       - Sequence count
 *   packet_type     - Packet type (0=TM, 1=TC)
 *   sec_header_flag - Secondary header flag
 *   total_length    - Total length including SPP header
 *
 * Returns: Number of bytes sent, or -1 on error.
 */
int packet_request(unsigned char *buffer, int apid, int seq_count,
		int packet_type, int sec_header_flag, size_t total_length)
{
	unsigned char header[4];
	ssize_t written;

	/* Suppress unused parameter warnings - these are in SPP header already */
	(void)apid;
	(void)seq_count;
	(void)packet_type;
	(void)sec_header_flag;

	if (write_fd < 0)
	{
		fprintf(stderr, "spp_loopback: FIFO not open for writing.\n");
		return -1;
	}

	if (total_length > MAX_PACKET_SIZE)
	{
		fprintf(stderr, "spp_loopback: Packet too large: %zu\n", total_length);
		return -1;
	}

	/* Write length header (big-endian) */
	header[0] = (total_length >> 24) & 0xFF;
	header[1] = (total_length >> 16) & 0xFF;
	header[2] = (total_length >> 8) & 0xFF;
	header[3] = total_length & 0xFF;

	pthread_mutex_lock(&write_mutex);

	written = write(write_fd, header, 4);
	if (written != 4)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "spp_loopback: Failed to write length header.\n");
		return -1;
	}

	written = write(write_fd, buffer, total_length);
	if (written != (ssize_t)total_length)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "spp_loopback: Failed to write packet data.\n");
		return -1;
	}

	pthread_mutex_unlock(&write_mutex);

	fprintf(stderr, "[i] spp_loopback: Sent %zu bytes (APID=%d, seq=%d)\n",
			total_length, apid, seq_count);

	return (int)total_length;
}

/*
 * init_space_packet_receiver - Initialize the SPP receiver.
 * Opens the FIFO for reading.
 *
 * Uses O_RDWR to avoid blocking - a FIFO opened with O_RDONLY blocks
 * until a writer opens the other end, but O_RDWR opens immediately.
 * This allows the sender and receiver processes to start independently.
 */
static void init_receiver(void)
{
	if (read_fd >= 0)
	{
		return; /* Already initialized */
	}

	/* Create FIFO if it doesn't exist */
	if (mkfifo(LOOPBACK_FIFO_PATH, 0666) < 0 && errno != EEXIST)
	{
		fprintf(stderr, "spp_loopback: Failed to create FIFO: %s\n",
				strerror(errno));
		return;
	}

	/* Open FIFO with O_RDWR to avoid blocking on open.
	 * O_RDONLY would block until a writer opens the FIFO,
	 * but O_RDWR opens immediately even with no writer. */
	read_fd = open(LOOPBACK_FIFO_PATH, O_RDWR);
	if (read_fd >= 0)
	{
		fprintf(stderr, "[i] spp_loopback: Receiver initialized.\n");
	}
	else
	{
		fprintf(stderr, "spp_loopback: Failed to open FIFO for reading: %s\n",
				strerror(errno));
	}
}

/*
 * packet_indication - Receive a space packet.
 *
 * Reads the next packet from the FIFO.
 *
 * Parameters:
 *   buffer       - Buffer to receive packet data (without length header)
 *   received_apid - Output: APID extracted from SPP header
 *
 * Returns: Number of bytes received, 0 for EOF, 1 for normal stop.
 */
size_t packet_indication(char *buffer, int *received_apid)
{
	unsigned char header[4];
	ssize_t bytes_read;
	size_t length;
	size_t total_read;

	/* Initialize receiver on first call */
	if (read_fd < 0)
	{
		init_receiver();
		if (read_fd < 0)
		{
			return 1; /* Signal normal stop */
		}
	}

	/* Read length header */
	bytes_read = read(read_fd, header, 4);
	if (bytes_read == 0)
	{
		/* EOF - sender closed */
		close(read_fd);
		read_fd = -1;
		return 1; /* Signal normal stop */
	}
	if (bytes_read != 4)
	{
		fprintf(stderr, "spp_loopback: Failed to read length header.\n");
		return 0; /* Error */
	}

	/* Parse length (big-endian) */
	length = ((size_t)header[0] << 24) | ((size_t)header[1] << 16) |
			((size_t)header[2] << 8) | (size_t)header[3];

	if (length > MAX_PACKET_SIZE)
	{
		fprintf(stderr, "spp_loopback: Invalid packet length: %zu\n", length);
		return 0;
	}

	/* Read packet data */
	total_read = 0;
	while (total_read < length)
	{
		bytes_read = read(read_fd, buffer + total_read, length - total_read);
		if (bytes_read <= 0)
		{
			fprintf(stderr, "spp_loopback: Failed to read packet data.\n");
			return 0;
		}
		total_read += bytes_read;
	}

	/* Extract APID from SPP primary header (bits 0-10 of bytes 0-1) */
	/* SPP header: Version(3) | Type(1) | SecHdrFlag(1) | APID(11) */
	*received_apid = ((buffer[0] & 0x07) << 8) | (buffer[1] & 0xFF);

	/* Skip SPP header (6 bytes) and return bundle data length */
	size_t bundle_length = length - 6;
	memmove(buffer, buffer + 6, bundle_length);

	fprintf(stderr, "[i] spp_loopback: Received %zu bytes (APID=%d)\n",
			bundle_length, *received_apid);

	return bundle_length;
}

/*
 * Cleanup function - called when library is unloaded
 */
__attribute__((destructor))
static void cleanup(void)
{
	if (write_fd >= 0)
	{
		close(write_fd);
		write_fd = -1;
	}
	if (read_fd >= 0)
	{
		close(read_fd);
		read_fd = -1;
	}
	/* Remove FIFO */
	unlink(LOOPBACK_FIFO_PATH);
}
