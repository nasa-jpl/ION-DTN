/*
 * ltpcancel.c: Utility to inject LTP Cancel Segments for testing
 *
 * This utility crafts and sends artificial LTP Cancel Segments to test
 * the cancel acknowledgment behavior. Useful for testing and debugging
 * LTP cancel segment handling.
 *
 * Author: ION Development Team
 *
 * Copyright (c) 2024, California Institute of Technology.
 * ALL RIGHTS RESERVED. U.S. Government Sponsorship acknowledged.
 *
 * Usage: ltpcancel <segment_type> <dest_host> <dest_port> <source_engine_id> <session_number>
 *   segment_type: CS (12) or CR (14)
 *   dest_host: target hostname/IP
 *   dest_port: target port
 *   source_engine_id: engine ID to use as source
 *   session_number: session number to cancel
 */

#include "platform.h"
#include "ion.h"

typedef struct {
	unsigned char version_type;     /* Version (4 bits) | Type (4 bits) */
	unsigned char source_engine[8]; /* Source Engine ID (SDNV) */
	unsigned char session_nbr[4];   /* Session Number (SDNV) */
	unsigned char extensions;       /* Extension counts */
	unsigned char reason_code;      /* Cancel reason code */
} LtpCancelSegment;

/* SDNV encoding for small values */
static int encode_sdnv(unsigned char *buf, unsigned long value) {
	if (value < 128) {
		buf[0] = (unsigned char)value;
		return 1;
	} else if (value < 16384) {
		buf[0] = (unsigned char)((value >> 7) | 0x80);
		buf[1] = (unsigned char)(value & 0x7F);
		return 2;
	} else {
		buf[0] = (unsigned char)((value >> 14) | 0x80);
		buf[1] = (unsigned char)(((value >> 7) & 0x7F) | 0x80);
		buf[2] = (unsigned char)(value & 0x7F);
		return 3;
	}
}

int main(int argc, char *argv[]) {
	if (argc != 6) {
		printf("Usage: %s <segment_type> <dest_host> <dest_port> <source_engine_id> <session_number>\n", argv[0]);
		printf("  segment_type: CS or CR\n");
		printf("  dest_host: target hostname/IP\n");
		printf("  dest_port: target port\n");
		printf("  source_engine_id: engine ID (numeric)\n");
		printf("  session_number: session number (numeric)\n");
		printf("\nExample: %s CS localhost 2113 2 12345\n", argv[0]);
		return 1;
	}

	char *seg_type_str = argv[1];
	char *dest_host = argv[2];
	int dest_port = atoi(argv[3]);
	uvast source_engine = getFqn(argv[4]);
	unsigned long session_number = strtoul(argv[5], NULL, 10);

	/* Determine segment type */
	unsigned char seg_type;
	if (strcmp(seg_type_str, "CS") == 0) {
		seg_type = 12; /* LtpCS */
	} else if (strcmp(seg_type_str, "CR") == 0) {
		seg_type = 14; /* LtpCR */
	} else {
		fprintf(stderr, "Error: segment_type must be CS or CR\n");
		return 1;
	}

	/* Create socket */
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		return 1;
	}

	/* Resolve destination address */
	struct hostent *host = gethostbyname(dest_host);
	if (!host) {
		fprintf(stderr, "Error: Could not resolve host %s\n", dest_host);
		close(sock);
		return 1;
	}

	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(dest_port);
	memcpy(&dest_addr.sin_addr, host->h_addr, host->h_length);

	/* Construct LTP Cancel Segment */
	unsigned char packet[64];
	int offset = 0;

	/* Version (0) and segment type */
	packet[offset++] = (0 << 4) | (seg_type & 0x0F);

	/* Source Engine ID (SDNV) */
	offset += encode_sdnv(&packet[offset], source_engine);

	/* Session Number (SDNV) */
	offset += encode_sdnv(&packet[offset], session_number);

	/* Extension counts (0) */
	packet[offset++] = 0x00;

	/* Cancel reason code */
	packet[offset++] = 0x00; /* LtpCancelByUser */

	/* Send the packet */
	ssize_t sent = sendto(sock, packet, offset, 0,
			(struct sockaddr*)&dest_addr, sizeof(dest_addr));

	if (sent < 0) {
		perror("sendto");
		close(sock);
		return 1;
	}

	printf("Sent %s segment: %ld bytes to %s:%d\n",
			seg_type_str, sent, dest_host, dest_port);
	printf("  Source Engine ID: %lu\n", source_engine);
	printf("  Session Number: %lu\n", session_number);

	close(sock);
	return 0;
}
