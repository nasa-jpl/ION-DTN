/*
	udplsa.h:	common definitions for UDP link service
			adapter modules.

	Author: Scott Burleigh, JPL

	Copyright (c) 2007, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef UDPLSA_H
#define UDPLSA_H

#include "ltpP.h"
#include "ion_network.h"
#include "ion_atomic.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDPLSA_BUFSZ		((256 * 256) - 1)
#define LtpUdpDefaultPortNbr	1113

typedef struct
{
	IonNetworkAddress	local_addr;
	IonNetworkAddress	peer_addr;
	int			linkSocket;
	pthread_mutex_t		lock;      /* Keep this! */
	ion_atomic_t		running;   /* <-- The ONLY line that changes */
} udp_ReceiverThreadParms;

extern void			*udplsa_handle_datagrams(void *parm);

#ifdef __cplusplus
}
#endif

#endif /* UDPLSA_H */
