/*
	dtpc.h:	Definitions supporting applications built on the
		implementation of the Delay Tolerant Payload Conditioning
		in the ION (Interplanetary Overlay Network) stack.

	Authors: Giorgos Papastergiou, SPICE
		 Ioannis Alexiadis, SPICE

	Copyright (c) 2011, Space Internetworking Center,
	Democritus University of Thrace. ALL RIGHTS RESERVED.
*/

#ifndef DTPC_H
#define DTPC_H

#include "bp.h"
#include "platform.h"
#include "sdrxn.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DTPC_POLL		(0)	/*      Return immediately.	*/
#define DTPC_NONBLOCKING	(0)	/*      Return immediately.	*/
#define DTPC_BLOCKING		(-1)	/*      Wait forever.		*/

typedef struct
{
	SdrObject       	payload;
	Sdnv            	length;
} PayloadRecord;

typedef int (*DtpcElisionFn)(SdrObject recordsList);

typedef struct dtpcsap_st	*DtpcSAP;

typedef enum
{
	PayloadPresent = 1,
	ReceptionTimedOut,
	ReceptionInterrupted,
	DtpcServiceStopped
} DtpcIndResult;

typedef struct
{
	DtpcIndResult result;
	char	     *srcEid;
	size_t	      length;
	SdrObject     item;
} DtpcDelivery;

/*      *       *       DTPC initilization       *       *       *	*/

extern int      dtpc_attach(void);

extern int      dtpc_entity_is_started(void);

extern void     dtpc_detach(void);

/*      *       *       DTPC local services      *       *       *	*/

extern int      dtpc_open(unsigned int topicID,
			DtpcElisionFn elisionFn,
			DtpcSAP *dtpcsapPtr);

extern int      dtpc_send(unsigned int profileID,
			DtpcSAP sap,
			char *dstEid,
			unsigned int maxRtx,
			size_t aggrSizeLimit,
			unsigned int aggrTimeLimit,
			int lifespan,
			BpAncillaryData *ancillaryData,
			unsigned char srrFlags,
			BpCustodySwitch custodySwitch,
			char *reportToEid,
			int classOfService,
			SdrObject item,
			size_t length);

extern int      dtpc_receive(DtpcSAP sap,
			DtpcDelivery *dlv,
			int timeoutSeconds);

extern void	dtpc_interrupt(DtpcSAP sap);

extern void	dtpc_release_delivery(DtpcDelivery *dlvBuffer);

extern void     dtpc_close(DtpcSAP sap);

#ifdef __cplusplus
}
#endif

#endif /* DTPC_H */
