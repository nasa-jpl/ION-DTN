/*
 	brscla.h:	common definitions for Bundle Relay Service
			convergence layer adapter modules.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _BRSCLA_H_
#define _BRSCLA_H_

#include "../stcp/stcpcla.h"
#include "ipnfw.h"
#include "dtn2fw.h"
#include "ionsec.h"
#include "crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	BRSTERM is maximum allowable number of seconds between the
 *	time tag in the authentication message and the current time
 *	(at the server) at the moment the authentication message is
 *	received.							*/
#ifndef BRSTERM
#define BRSTERM			(5)
#endif

#define DIGEST_LEN		20
#define REGISTRATION_LEN	(DIGEST_LEN + 4)

#ifdef __cplusplus
}
#endif

#endif	/* _BRSCLA_H */
