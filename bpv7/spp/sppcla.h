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
#ifndef _SPPCLA_H_
#define _SPPCLA_H_

#include "bpP.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// include space packet protocol libraries

#ifdef __cplusplus
extern "C" {
#endif

#define SPPCLA_BUFSZ		(65536)
typedef char (*build_space_packet_ptr)(int, int, const char*,int, int, size_t*);
typedef char (*packet_request_ptr)(char*,int);
struct SppConfig {
    int             apid;
    int             seq_count;
    int             packet_type;
    int             sec_header_flag;
    build_space_packet_ptr build_space_packet;
    packet_request_ptr packet_request;
};

extern int	sendBytesBySPP(char *from, int length, unsigned char *buffer, struct SppConfig *sppcfg);
extern int	sendBundleBySPP(unsigned int bundleLength, Object bundleZco, 
				unsigned char *buffer,struct SppConfig *sppcfg);
extern int	receiveBytesBySPP(int fd,char *into, int length);    

#ifdef __cplusplus
}
#endif

#endif	/* _SPPCLA_H */
