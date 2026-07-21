/*
	eppcla.h:	common definitions for Encapsulation Packet Protocol
			convergence layer adapter modules.

	Author: Gregory Miles JPL

	Modification History:
	Date  Who What

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef EPPCLA_H
#define EPPCLA_H

#include "bpP.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * EPP supports much larger bundles than SPP.
 * Maximum per CCSDS 734.20-O-1 is 4,294,967,287 bytes.
 * We use a practical 1MB buffer that can be increased if needed.
 */
#define EPPCLA_BUFSZ		(1048576)

/*
 * Encapsulation Protocol Identifier (EPI) value for Bundle Protocol.
 * Registered in SANA per CCSDS 734.20-O-1 Section B2.1.6.
 */
#define EPP_BP_EPI		(4)

/*
 * Function pointer types for EPP provider library.
 * The provider library implements the interface to the underlying
 * SDLP (Space Data Link Protocol) layer.
 */

/* Called once at startup to initialize sender resources */
typedef void (*init_epp_sender_ptr)(void);

/* Called once at shutdown to clean up sender resources */
typedef void (*finalize_epp_sender_ptr)(void);

/*
 * Called for each bundle to send (ENCAPSULATION.request primitive).
 * Parameters:
 *   data         - Bundle data to encapsulate and send
 *   length       - Length of bundle data in bytes
 *   sdlp_channel - SDLP channel identifier (mission-specific)
 *   epi          - Encapsulation Protocol Identifier (4 for BP)
 * Returns: Number of bytes sent, or negative on error
 */
typedef int (*encapsulation_request_ptr)(unsigned char *data, size_t length,
		int sdlp_channel, int epi);

/*
 * Blocking call that waits for and returns the next received bundle
 * (ENCAPSULATION.indication primitive).
 * Parameters:
 *   buffer       - Buffer to receive bundle data into
 *   sdlp_channel - Output: SDLP channel the bundle arrived on
 *   epi          - Output: EPI value from encapsulation header
 * Returns: Number of bytes received, 0 for shutdown, negative on error
 */
typedef size_t (*encapsulation_indication_ptr)(char *buffer, int *sdlp_channel,
		int *epi);

/*
 * Configuration structure for EPP CLA.
 * Contains the SDLP channel, EPI value, and function pointers
 * to the provider library functions.
 */
struct EppConfig {
	int			sdlp_channel;
	int			epi;
	encapsulation_request_ptr	encapsulation_request;
	init_epp_sender_ptr		init_sender;
	finalize_epp_sender_ptr		finalize_sender;
};

extern int	sendBytesByEPP(unsigned char *buffer, size_t length,
			struct EppConfig *eppcfg);
extern int	sendBundleByEPP(unsigned int bundleLength, SdrObject bundleZco,
			unsigned char *buffer, struct EppConfig *eppcfg);

#ifdef __cplusplus
}
#endif

#endif /* EPPCLA_H */
