/*
 	tcpcla.h:	common definitions for TCP convergence layer
			adapter modules.

	Author: Scott Burleigh, JPL

	Modification History:
	Date  Who What

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _TCPCLA_H_
#define _TCPCLA_H_

#include "bpP.h"
#include <pthread.h>
#include "ipnfw.h"
#include "dtn2fw.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(MIN)
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#endif

#define TCPCLA_BUFSZ		(64 * 1024)
#define BpTcpDefaultPortNbr	4556

#define TCPCLA_MAGIC 		"dtn!"
#define TCPCLA_MAGIC_SIZE	4
#define TCPCLA_ID_VERSION	0x03
#define TCPCLA_FLAGS		0x00
#define TCPCLA_TYPE_DATA	0x01
#define TCPCLA_TYPE_ACK		0x02
#define TCPCLA_TYPE_REF_BUN	0x03
#define TCPCLA_TYPE_KEEP_AL	0x04
#define TCPCLA_TYPE_SHUT_DN	0x05
#define SHUT_DN_BUFSZ		32
#define SHUT_DN_DELAY_FLAG	0x01
#define SHUT_DN_REASON_FLAG	0x02
#define SHUT_DN_NO	 	0
#define SHUT_DN_IDLE		1
#define SHUT_DN_IDLE_HEX	0x00
#define SHUT_DN_VER		2
#define SHUT_DN_VER_HEX		0x01
#define SHUT_DN_BUSY		3
#define SHUT_DN_BUSY_HEX	0x02
#define BACKOFF_TIMER_START	30
#define BACKOFF_TIMER_LIMIT	3600

#ifndef KEEPALIVE_PERIOD
#define KEEPALIVE_PERIOD	(15)
#endif

#ifndef mingw
extern void	handleConnectionLoss();
#endif

/*	*	*	*	For BRS.	*	*	*	*/

extern int	openOutductSocket(char *protocolName, char *ductName,
			int *bundleSocket);

/*	*	*	*	For all TCP.	*	*	*	*/

extern int	sendBytesByTCP(int *bundleSocket, char *from, int length);
extern int	receiveBytesByTCP(int bundleSocket, char *into, int length);
extern void	closeOutductSocket(int *bundleSocket);

/*	*	*	*	STCP.	*	*	*	*	*/

extern int	sendBundleByTCP(char *protocolName, char *ductName,
			int *bundleSocket, unsigned int bundleLength,
			Object bundleZco, unsigned char *buffer);
extern int	receiveBundleByTcp(int bundleSocket, AcqWorkArea *work,
			char *buffer, ReqAttendant *attendant);

/*	*	*	*	TCPCL.	*	*	*	*	*/

extern int	tcpDesiredKeepAlivePeriod;

extern int	sendBundleByTCPCL(char *protocolName, char *ductName,
			int *bundleSocket, unsigned int bundleLength,
			Object bundleZco, unsigned char *buffer,
			int *keepalivePeriod);
extern int	receiveBundleByTcpCL(int bundleSocket, AcqWorkArea *work, char *buffer);
extern int 	receiveSegmentByTcpCL(int bundleSocket,AcqWorkArea *work,char *buffer, uvast *segmentLength,int *flags);
extern int 	sendContactHeader(int *bundleSocket, unsigned char *buffer);
extern int	receiveContactHeader(int *bundleSocket, unsigned char *buffer,
			int *keepalivePeriod);
extern void	findVInduct(VInduct **vduct, char *protocolName);
extern int 	sendShutDownMessage(int *bundleSocket, int reason, int delay);

#ifdef __cplusplus
}
#endif

#endif	/* _TCPCLA_H */
