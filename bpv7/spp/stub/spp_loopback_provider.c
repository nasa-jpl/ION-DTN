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
/* Enable POSIX.1-2001 for sigaction() */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>

#define LOOPBACK_FIFO_PATH "/tmp/spp_loopback_fifo"
#define MAX_PACKET_SIZE (65536)
#define POLL_TIMEOUT_MS (1000)  /* 1 second timeout for poll */

/* Flag to signal shutdown */
static volatile sig_atomic_t shutdown_requested = 0;

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
 * Builds an SPP header, prepends it to the bundle data, and writes
 * the complete packet to the FIFO with a simple framing format:
 *   - 4 bytes: total packet length (big-endian)
 *   - 6 bytes: SPP primary header
 *   - N bytes: bundle data
 *
 * Parameters:
 *   buffer          - Buffer containing bundle data (no SPP header yet)
 *   apid            - Application Process ID (11 bits)
 *   seq_count       - Sequence count (14 bits)
 *   packet_type     - Packet type (0=TM, 1=TC)
 *   sec_header_flag - Secondary header flag
 *   total_length    - Total length including 6-byte SPP header
 *
 * Returns: Number of bytes sent, or -1 on error.
 */
int packet_request(unsigned char *buffer, int apid, int seq_count,
		int packet_type, int sec_header_flag, size_t total_length)
{
	unsigned char frame_header[4];
	unsigned char spp_header[6];
	ssize_t written;
	size_t bundle_length;
	int data_field_length;

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

	/* Bundle data length is total_length minus 6-byte SPP header */
	bundle_length = total_length - 6;
	data_field_length = (int)bundle_length - 1;  /* Per CCSDS, length = data - 1 */

	/*
	 * Build SPP Primary Header (6 bytes) per CCSDS 133.0-B-2:
	 * Byte 0: Version(3) | Type(1) | SecHdrFlag(1) | APID[10:8](3)
	 * Byte 1: APID[7:0](8)
	 * Byte 2: SeqFlags(2) | SeqCount[13:8](6)
	 * Byte 3: SeqCount[7:0](8)
	 * Byte 4-5: Packet Data Length - 1 (big-endian)
	 */

	/* Byte 0: Version=0, Type, SecHdrFlag, APID high 3 bits */
	spp_header[0] = ((packet_type & 0x01) << 4) |
			((sec_header_flag & 0x01) << 3) |
			((apid >> 8) & 0x07);

	/* Byte 1: APID low 8 bits */
	spp_header[1] = apid & 0xFF;

	/* Byte 2: Sequence Flags = 3 (unsegmented), SeqCount high 6 bits */
	spp_header[2] = 0xC0 | ((seq_count >> 8) & 0x3F);

	/* Byte 3: SeqCount low 8 bits */
	spp_header[3] = seq_count & 0xFF;

	/* Bytes 4-5: Packet Data Length - 1 (big-endian) */
	spp_header[4] = (data_field_length >> 8) & 0xFF;
	spp_header[5] = data_field_length & 0xFF;

	/* Write FIFO frame header: total length (big-endian) */
	frame_header[0] = (total_length >> 24) & 0xFF;
	frame_header[1] = (total_length >> 16) & 0xFF;
	frame_header[2] = (total_length >> 8) & 0xFF;
	frame_header[3] = total_length & 0xFF;

	pthread_mutex_lock(&write_mutex);

	/* Write FIFO frame header */
	written = write(write_fd, frame_header, 4);
	if (written != 4)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "spp_loopback: Failed to write frame header.\n");
		return -1;
	}

	/* Write SPP header */
	written = write(write_fd, spp_header, 6);
	if (written != 6)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "spp_loopback: Failed to write SPP header.\n");
		return -1;
	}

	/* Write bundle data */
	written = write(write_fd, buffer, bundle_length);
	if (written != (ssize_t)bundle_length)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "spp_loopback: Failed to write bundle data.\n");
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
 * Helper function to read with poll timeout.
 * Returns bytes read, 0 for timeout/shutdown, -1 for error.
 */
static ssize_t read_with_timeout(int fd, void *buf, size_t count)
{
	struct pollfd pfd;
	int poll_result;

	pfd.fd = fd;
	pfd.events = POLLIN;

	while (!shutdown_requested)
	{
		poll_result = poll(&pfd, 1, POLL_TIMEOUT_MS);
		if (poll_result < 0)
		{
			if (errno == EINTR)
			{
				continue;  /* Interrupted, retry */
			}
			return -1;  /* Error */
		}
		if (poll_result == 0)
		{
			continue;  /* Timeout, check shutdown and retry */
		}
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			return 0;  /* Error or hangup */
		}
		if (pfd.revents & POLLIN)
		{
			return read(fd, buf, count);
		}
	}

	return 0;  /* Shutdown requested */
}

/*
 * packet_indication - Receive a space packet.
 *
 * Reads the next packet from the FIFO using non-blocking poll.
 * This allows the function to return when shutdown is requested.
 *
 * Parameters:
 *   buffer       - Buffer to receive packet data (without length header)
 *   received_apid - Output: APID extracted from SPP header
 *
 * Returns: Number of bytes received, 0 for error, 1 for normal stop.
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

	/* Read length header with timeout */
	bytes_read = read_with_timeout(read_fd, header, 4);
	if (bytes_read == 0)
	{
		/* Timeout/shutdown or EOF - signal normal stop */
		return 1;
	}
	if (bytes_read < 0 || bytes_read != 4)
	{
		if (!shutdown_requested)
		{
			fprintf(stderr, "spp_loopback: Failed to read length header.\n");
		}
		return 1; /* Signal normal stop on error */
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
	while (total_read < length && !shutdown_requested)
	{
		bytes_read = read_with_timeout(read_fd, buffer + total_read,
					       length - total_read);
		if (bytes_read <= 0)
		{
			if (!shutdown_requested)
			{
				fprintf(stderr, "spp_loopback: Failed to read packet data.\n");
			}
			return 1; /* Signal normal stop */
		}
		total_read += bytes_read;
	}

	if (shutdown_requested)
	{
		return 1; /* Signal normal stop */
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
 * Signal handler to request graceful shutdown
 */
static void handle_shutdown_signal(int signum)
{
	(void)signum;
	shutdown_requested = 1;
}

/*
 * Initialization function - called by sppcli/sppclo after dlopen.
 * Sets up signal handlers for graceful shutdown.
 */
void spp_provider_init(void)
{
	struct sigaction sa;
	sa.sa_handler = handle_shutdown_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

/*
 * Cleanup function - called by sppcli/sppclo before dlclose.
 * Closes file descriptors and removes FIFO.
 */
void spp_provider_cleanup(void)
{
	shutdown_requested = 1;
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
