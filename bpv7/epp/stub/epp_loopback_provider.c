/*
	epp_loopback_provider.c:	Loopback EPP provider library for testing.
				Uses a named pipe (FIFO) for loopback communication.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	This provider library implements loopback functionality for testing
	the EPP CLA without real SDLP hardware. Data sent via
	encapsulation_request is written to a FIFO and can be read back via
	encapsulation_indication.

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

#define LOOPBACK_FIFO_PATH "/tmp/epp_loopback_fifo"
#define MAX_PACKET_SIZE (1048576)  /* 1MB */
#define POLL_TIMEOUT_MS (1000)  /* 1 second timeout for poll */

/* Flag to signal shutdown */
static volatile sig_atomic_t shutdown_requested = 0;

/* File descriptors for FIFO */
static int write_fd = -1;
static int read_fd = -1;
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * init_epp_sender - Initialize the EPP sender.
 * Creates the FIFO and opens it for writing.
 *
 * Uses O_RDWR to avoid blocking - a FIFO opened with O_WRONLY blocks
 * until a reader opens the other end, but O_RDWR opens immediately.
 * This allows the sender and receiver processes to start independently.
 */
void init_epp_sender(void)
{
	/* Create FIFO if it doesn't exist */
	if (mkfifo(LOOPBACK_FIFO_PATH, 0666) < 0 && errno != EEXIST)
	{
		fprintf(stderr, "epp_loopback: Failed to create FIFO: %s\n",
				strerror(errno));
		return;
	}

	/* Open FIFO with O_RDWR to avoid blocking on open.
	 * O_WRONLY would block until a reader opens the FIFO,
	 * but O_RDWR opens immediately even with no reader. */
	write_fd = open(LOOPBACK_FIFO_PATH, O_RDWR);
	if (write_fd >= 0)
	{
		fprintf(stderr, "[i] epp_loopback: Sender initialized.\n");
	}
	else
	{
		fprintf(stderr, "epp_loopback: Failed to open FIFO for writing: %s\n",
				strerror(errno));
	}
}

/*
 * finalize_epp_sender - Clean up the EPP sender.
 */
void finalize_epp_sender(void)
{
	if (write_fd >= 0)
	{
		close(write_fd);
		write_fd = -1;
		fprintf(stderr, "[i] epp_loopback: Sender finalized.\n");
	}
}

/*
 * encapsulation_request - Send a bundle via EPP (ENCAPSULATION.request).
 *
 * Writes the bundle to the FIFO with a simple framing format:
 *   - 4 bytes: SDLP channel (big-endian)
 *   - 4 bytes: EPI (big-endian)
 *   - 4 bytes: data length (big-endian)
 *   - N bytes: bundle data
 *
 * Parameters:
 *   data         - Bundle data to encapsulate and send
 *   length       - Length of bundle data in bytes
 *   sdlp_channel - SDLP channel identifier
 *   epi          - Encapsulation Protocol Identifier
 *
 * Returns: Number of bytes sent, or -1 on error.
 */
int encapsulation_request(unsigned char *data, size_t length,
		int sdlp_channel, int epi)
{
	unsigned char header[12];
	ssize_t written;

	if (write_fd < 0)
	{
		fprintf(stderr, "epp_loopback: FIFO not open for writing.\n");
		return -1;
	}

	if (length > MAX_PACKET_SIZE)
	{
		fprintf(stderr, "epp_loopback: Packet too large: %zu\n", length);
		return -1;
	}

	/* Write header: sdlp_channel (4), epi (4), length (4) - big-endian */
	header[0] = (sdlp_channel >> 24) & 0xFF;
	header[1] = (sdlp_channel >> 16) & 0xFF;
	header[2] = (sdlp_channel >> 8) & 0xFF;
	header[3] = sdlp_channel & 0xFF;

	header[4] = (epi >> 24) & 0xFF;
	header[5] = (epi >> 16) & 0xFF;
	header[6] = (epi >> 8) & 0xFF;
	header[7] = epi & 0xFF;

	header[8] = (length >> 24) & 0xFF;
	header[9] = (length >> 16) & 0xFF;
	header[10] = (length >> 8) & 0xFF;
	header[11] = length & 0xFF;

	pthread_mutex_lock(&write_mutex);

	written = write(write_fd, header, 12);
	if (written != 12)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "epp_loopback: Failed to write header.\n");
		return -1;
	}

	written = write(write_fd, data, length);
	if (written != (ssize_t)length)
	{
		pthread_mutex_unlock(&write_mutex);
		fprintf(stderr, "epp_loopback: Failed to write packet data.\n");
		return -1;
	}

	pthread_mutex_unlock(&write_mutex);

	fprintf(stderr, "[i] epp_loopback: Sent %zu bytes on channel %d "
			"with EPI %d\n", length, sdlp_channel, epi);

	return (int)length;
}

/*
 * init_receiver - Initialize the EPP receiver.
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
		fprintf(stderr, "epp_loopback: Failed to create FIFO: %s\n",
				strerror(errno));
		return;
	}

	/* Open FIFO with O_RDWR to avoid blocking on open.
	 * O_RDONLY would block until a writer opens the FIFO,
	 * but O_RDWR opens immediately even with no writer. */
	read_fd = open(LOOPBACK_FIFO_PATH, O_RDWR);
	if (read_fd >= 0)
	{
		fprintf(stderr, "[i] epp_loopback: Receiver initialized.\n");
	}
	else
	{
		fprintf(stderr, "epp_loopback: Failed to open FIFO for reading: %s\n",
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
 * encapsulation_indication - Receive a bundle via EPP (ENCAPSULATION.indication).
 *
 * Reads the next bundle from the FIFO using non-blocking poll.
 * This allows the function to return when shutdown is requested.
 *
 * Parameters:
 *   buffer       - Buffer to receive bundle data
 *   sdlp_channel - Output: SDLP channel the bundle arrived on
 *   epi          - Output: EPI value from encapsulation header
 *
 * Returns: Number of bytes received, 0 for error, 1 for normal stop.
 */
size_t encapsulation_indication(char *buffer, int *sdlp_channel, int *epi)
{
	unsigned char header[12];
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

	/* Read header with timeout */
	bytes_read = read_with_timeout(read_fd, header, 12);
	if (bytes_read == 0)
	{
		/* Timeout/shutdown or EOF - signal normal stop */
		return 1;
	}
	if (bytes_read < 0 || bytes_read != 12)
	{
		if (!shutdown_requested)
		{
			fprintf(stderr, "epp_loopback: Failed to read header.\n");
		}
		return 1; /* Signal normal stop on error */
	}

	/* Parse header - big-endian */
	*sdlp_channel = (header[0] << 24) | (header[1] << 16) |
			(header[2] << 8) | header[3];
	*epi = (header[4] << 24) | (header[5] << 16) |
			(header[6] << 8) | header[7];
	length = ((size_t)header[8] << 24) | ((size_t)header[9] << 16) |
			((size_t)header[10] << 8) | (size_t)header[11];

	if (length > MAX_PACKET_SIZE)
	{
		fprintf(stderr, "epp_loopback: Invalid packet length: %zu\n", length);
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
				fprintf(stderr, "epp_loopback: Failed to read packet data.\n");
			}
			return 1; /* Signal normal stop */
		}
		total_read += bytes_read;
	}

	if (shutdown_requested)
	{
		return 1; /* Signal normal stop */
	}

	fprintf(stderr, "[i] epp_loopback: Received %zu bytes on channel %d "
			"with EPI %d\n", length, *sdlp_channel, *epi);

	return length;
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
 * Initialization function - called by eppcli/eppclo after dlopen.
 * Sets up signal handlers for graceful shutdown.
 */
void epp_provider_init(void)
{
	struct sigaction sa;
	sa.sa_handler = handle_shutdown_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
}

/*
 * Cleanup function - called by eppcli/eppclo before dlclose.
 * Closes file descriptors and removes FIFO.
 */
void epp_provider_cleanup(void)
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
