/*
 *	tcpbsa.h:	common definitions for TCP link service
 *			adapter modules.
 *
 *	Authors: Sotirios-Angelos Lenas, SPICE
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	Copyright (c) 2013, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	
 *	All rights reserved. U.S. Government and E.U. Sponsorship acknowledged.
 *
 */
#ifndef _TCPBSA_H_
#define _TCPBSA_H_

#include "bsspP.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MIN)
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#endif

#define TCPBSA_BUFSZ		(64 * 1024)
#define bsspTcpDefaultPortNbr	4556

/*	TCP has congestion control, so rate control not needed.	*/
#define	DEFAULT_TCP_RATE	-1	

#ifndef KEEPALIVE_PERIOD
#define KEEPALIVE_PERIOD	(15)
#endif

extern int	tcpDelayEnabled;
extern int	tcpDelayNsecPerByte;

extern int	connectToBSI(struct sockaddr *sn, int *sock);
extern int	sendBytesByTCP(int *blockSocket, char *from, int length,
			struct sockaddr *sn);
extern int	sendBlockByTCP(struct sockaddr *socketName,
			int *blockSocket, int blockLength,
			char *block);

#ifdef __cplusplus
}
#endif

#endif	/* _TCPBSA_H */
