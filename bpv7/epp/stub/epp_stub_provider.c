/*
	epp_stub_provider.c:	Stub EPP provider library for testing.
				Writes bundles to a file for verification.

	Author: Gregory Miles

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	This is an example/test provider library that can be used
	to verify the EPP CLA implementation without real SDLP hardware.
	It writes bundles to /tmp/epp_bundles.bin and can read from
	/tmp/epp_input.bin for testing the receiver.

									*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* File handle for output */
static FILE *output_file = NULL;
static FILE *input_file = NULL;

/*
 * init_epp_sender - Initialize the EPP sender.
 * Opens the output file for writing bundles.
 */
void init_epp_sender(void)
{
	output_file = fopen("/tmp/epp_bundles.bin", "wb");
	if (output_file == NULL)
	{
		fprintf(stderr, "epp_stub_provider: Failed to open output "
				"file: %s\n", strerror(errno));
	}
	else
	{
		fprintf(stderr, "[i] epp_stub_provider: Sender initialized, "
				"writing to /tmp/epp_bundles.bin\n");
	}
}

/*
 * finalize_epp_sender - Clean up the EPP sender.
 * Closes the output file.
 */
void finalize_epp_sender(void)
{
	if (output_file != NULL)
	{
		fclose(output_file);
		output_file = NULL;
		fprintf(stderr, "[i] epp_stub_provider: Sender finalized.\n");
	}
}

/*
 * encapsulation_request - Send a bundle via EPP.
 *
 * This stub implementation writes the bundle to a file with a
 * simple framing format:
 *   - 4 bytes: SDLP channel (big-endian)
 *   - 4 bytes: EPI (big-endian)
 *   - 4 bytes: data length (big-endian)
 *   - N bytes: bundle data
 *
 * Parameters:
 *   data         - Bundle data to send
 *   length       - Length of bundle data
 *   sdlp_channel - SDLP channel identifier
 *   epi          - Encapsulation Protocol Identifier
 *
 * Returns: Number of bytes sent, or -1 on error.
 */
int encapsulation_request(unsigned char *data, size_t length,
		int sdlp_channel, int epi)
{
	unsigned char header[12];
	size_t written;

	if (output_file == NULL)
	{
		fprintf(stderr, "epp_stub_provider: Output file not open.\n");
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

	written = fwrite(header, 1, 12, output_file);
	if (written != 12)
	{
		fprintf(stderr, "epp_stub_provider: Failed to write header.\n");
		return -1;
	}

	written = fwrite(data, 1, length, output_file);
	if (written != length)
	{
		fprintf(stderr, "epp_stub_provider: Failed to write data.\n");
		return -1;
	}

	fflush(output_file);

	fprintf(stderr, "[i] epp_stub_provider: Sent %zu bytes on channel %d "
			"with EPI %d\n", length, sdlp_channel, epi);

	return (int)length;
}

/*
 * encapsulation_indication - Receive a bundle via EPP.
 *
 * This stub implementation reads from a file with the same
 * framing format as encapsulation_request.
 *
 * Parameters:
 *   buffer       - Buffer to receive bundle data
 *   sdlp_channel - Output: SDLP channel the bundle arrived on
 *   epi          - Output: EPI value from encapsulation header
 *
 * Returns: Number of bytes received, 0 for EOF/shutdown, -1 on error.
 */
size_t encapsulation_indication(char *buffer, int *sdlp_channel, int *epi)
{
	unsigned char header[12];
	size_t length;
	size_t bytes_read;

	/* Open input file on first call */
	if (input_file == NULL)
	{
		input_file = fopen("/tmp/epp_input.bin", "rb");
		if (input_file == NULL)
		{
			fprintf(stderr, "epp_stub_provider: Failed to open "
					"input file: %s\n", strerror(errno));
			/* Return 1 to signal normal stop */
			return 1;
		}
		fprintf(stderr, "[i] epp_stub_provider: Receiver initialized, "
				"reading from /tmp/epp_input.bin\n");
	}

	/* Read header */
	bytes_read = fread(header, 1, 12, input_file);
	if (bytes_read == 0)
	{
		/* EOF - signal normal stop */
		fclose(input_file);
		input_file = NULL;
		return 1;
	}
	if (bytes_read != 12)
	{
		fprintf(stderr, "epp_stub_provider: Incomplete header read.\n");
		return 0;
	}

	/* Parse header - big-endian */
	*sdlp_channel = (header[0] << 24) | (header[1] << 16) |
			(header[2] << 8) | header[3];
	*epi = (header[4] << 24) | (header[5] << 16) |
			(header[6] << 8) | header[7];
	length = ((size_t)header[8] << 24) | ((size_t)header[9] << 16) |
			((size_t)header[10] << 8) | (size_t)header[11];

	/* Read data */
	bytes_read = fread(buffer, 1, length, input_file);
	if (bytes_read != length)
	{
		fprintf(stderr, "epp_stub_provider: Incomplete data read.\n");
		return 0;
	}

	fprintf(stderr, "[i] epp_stub_provider: Received %zu bytes on "
			"channel %d with EPI %d\n", length, *sdlp_channel, *epi);

	return length;
}
