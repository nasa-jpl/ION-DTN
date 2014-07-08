/*
 *	udpbsa.h:	common definitions for UDP link service
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
#ifndef _UDPBSA_H_
#define _UDPBSA_H_

#include "bsspP.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDPBSA_BUFSZ		((256 * 256) - 1)
#define BsspUdpDefaultPortNbr	6001

#ifdef __cplusplus
}
#endif

#endif	/* _UDPBSA_H */
