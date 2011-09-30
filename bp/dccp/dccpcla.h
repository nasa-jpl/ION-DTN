/*
 	dccpcla.h:	common definitions for DCCP convergence layer
			adapter modules.

	Author: Samuel Jero, Ohio University

	Copyright (c) 2010.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

	This convergence-layer does not provide ANY
	fragmentation support. Maximum packet size is
	link MTU.
 										*/
#ifndef _DCCPCLA_H_
#define _DCCPCLA_H_

#include "bpP.h"
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/dccp.h>


#ifdef __cplusplus
extern "C" {
#endif

/*DCCP IDLE PERIOD */
#define DCCP_IDLE_TIME	120	/*seconds 					*/

/* MAX DCCP RETRIES*/
#define MAX_DCCP_RETRIES 100000 /*number of times to try to send a packet
				 with a full congestion window before closing
				 the connection. -1 makes the limit infinite.	*/

/* EAGAIN Wait Time */
#define EAGAIN_WAIT    3	/*number of microseconds to wait after DCCP send
				 returns EAGAIN (full congestion window/buffer)
				 before retrying				 */

/* Connection Attempt Settings*/
#define	START_BACKOFF	2		/* number of seconds to start exponential backoff*/
#define MAX_BACKOFF		1024	/* number of seconds to end exponential backoff*/

/*DCCP Defines */
#ifndef SOCK_DCCP
#define SOCK_DCCP 6
#endif
#ifndef IPPROTO_DCCP
#define IPPROTO_DCCP 33
#endif
#ifndef SOL_DCCP
#define SOL_DCCP 269
#endif


#define DCCPCLA_BUFSZ		((256 * 256) - 1)
#define DCCP_MAX_CON		5
#define BpDccpDefaultPortNbr	4556
#define DEFAULT_DCCP_RATE	11250000

#ifdef __cplusplus
}
#endif

#endif	/* _DCCPLSA_H_ */
