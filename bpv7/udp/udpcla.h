/*
	udpcla.h:	common definitions for UDP convergence layer
			adapter modules.

	Author: Ted Piotrowski, APL
		Scott Burleigh, JPL

	Modification History:
	August 2025, Dual-stack Extension, ION Dev Team/JPL

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef UDPCLA_H
#define UDPCLA_H

#include <pthread.h>
#include "bpP.h"
#include "ion_network.h"
#include "ion_atomic.h"


#ifdef __cplusplus
extern "C" {
#endif

#define UDPCLA_BUFSZ                ((256 * 256) - 1)
/* INET6_ADDR_WITH_PORT_STRLEN is now defined in ion_network.h */

/* Dual-stack socket management with shutdown support */
typedef struct
{
	int               main_socket;     /* Primary UDP socket */
	IonNetworkAddress local_addr;      /* Actual bound address */
	IonNetworkAddress shutdown_target; /* Where to send shutdown signal */
	int               shutdown_socket; /* Socket for shutdown signal */
	pthread_mutex_t   shutdown_mutex;  /* Thread safety */
	ion_atomic_t               shutdown_requested; /* Shutdown flag */
} UdpClaSocket;

/* Enhanced functions for dual-stack with automatic shutdown handling */
extern int  initUdpClaSocket(const char *endpoint, UdpClaSocket *claSock);
extern int  receiveUdpClaDatagram(UdpClaSocket *claSock, char *buffer,
		size_t buffer_size, IonNetworkAddress *from_addr,
		int *is_shutdown);
extern int  sendUdpClaShutdown(UdpClaSocket *claSock);
extern void cleanupUdpClaSocket(UdpClaSocket *claSock);
extern int  sendBundleByUDPDualStack(const IonNetworkAddress *destAddr,
		int *bundleSocket, unsigned int bundleLength,
		SdrObject bundleZco, unsigned char *buffer);

/* Legacy IPv4 only UDP send & receive functions */
extern int sendBytesByUDP(int *bundleSocket, char *from, int length,
		struct sockaddr *socketName);
extern int sendBundleByUDP(struct sockaddr *socketName, int *bundleSocket,
		unsigned int bundleLength, SdrObject bundleZco,
		unsigned char *buffer);

#ifdef __cplusplus
}
#endif

#endif /* UDPCLA_H */
