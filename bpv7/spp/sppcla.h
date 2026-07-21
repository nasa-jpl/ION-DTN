/*
	sppcla.h:	common definitions for Space Packet Protocol convergence layer
			adapter modules.

	Author: Gregory Miles JPL

	Modification History:
	Date  Who What

	Copyright (c) 2025, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef SPPCLA_H
#define SPPCLA_H

#include "bpP.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// include space packet protocol libraries

#ifdef __cplusplus
extern "C" {
#endif

// Maximum sequence number limit for Space Packet Protocol
#define SPP_MAX_SEQ_COUNT	(16383)
#define SPPCLA_BUFSZ		(65536)
// Declare initialization, finalization and request calls
typedef void (*init_spp_sender_ptr)(void);
typedef void (*finalize_spp_sender_ptr)(void);
typedef int (*packet_request_ptr)(unsigned char*,int, int, int, int, size_t);

struct SppConfig {
	int			apid;
	int			seq_count;
	int			packet_type;
	int			sec_header_flag;
	packet_request_ptr	packet_request;
	init_spp_sender_ptr	init_sender;
	finalize_spp_sender_ptr finalize_sender;
};

extern int	sendBytesBySPP(int length, unsigned char *buffer, struct SppConfig *sppcfg,size_t);
extern int	sendBundleBySPP(unsigned int bundleLength, SdrObject bundleZco,
				unsigned char *buffer,struct SppConfig *sppcfg);

#ifdef __cplusplus
}
#endif

#endif /* SPPCLA_H */
