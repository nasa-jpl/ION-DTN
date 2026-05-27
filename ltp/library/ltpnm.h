/*
 *	ltpnm.h:	definitions supporting the LTP instrumentation
 *			API.
 *
 *	Copyright (c) 2011, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Larry Shackleford, GSFC
 */

#ifndef LTPNM_H
#define LTPNM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "platform.h"

#define LTPNM_SPAN_FILLPATT       ((unsigned char)0xE0)

typedef struct
{
	unsigned long       remoteEngineNbr;

	unsigned long       currentExportSessions;
	unsigned long       currentOutboundSegments;
	unsigned long       currentImportSessions;
	unsigned long       currentInboundSegments;

	time_t              lastResetTime;

	unsigned long       outputSegQueuedCount;
	unsigned long       outputSegQueuedBytes;
	unsigned long       outputSegPoppedCount;
	unsigned long       outputSegPoppedBytes;

	unsigned long       outputCkptXmitCount;
	unsigned long       outputPosAckRecvCount;
	unsigned long       outputNegAckRecvCount;
	unsigned long       outputCancelRecvCount;
	unsigned long       outputCkptReXmitCount;
	unsigned long       outputSegReXmitCount;
	unsigned long       outputSegReXmitBytes;
	unsigned long       outputCancelXmitCount;
	unsigned long       outputCompleteCount;

	unsigned long       inputSegRecvRedCount;
	unsigned long       inputSegRecvRedBytes;
	unsigned long       inputSegRecvGreenCount;
	unsigned long       inputSegRecvGreenBytes;
	unsigned long       inputSegRedundantCount;
	unsigned long       inputSegRedundantBytes;
	unsigned long       inputSegMalformedCount;
	unsigned long       inputSegMalformedBytes;
	unsigned long       inputSegUnkSenderCount;
	unsigned long       inputSegUnkSenderBytes;
	unsigned long       inputSegUnkClientCount;
	unsigned long       inputSegUnkClientBytes;
	unsigned long       inputSegStrayCount;
	unsigned long       inputSegStrayBytes;
	unsigned long       inputSegMiscolorCount;
	unsigned long       inputSegMiscolorBytes;
	unsigned long       inputSegClosedCount;
	unsigned long       inputSegClosedBytes;

	unsigned long       inputCkptRecvCount;
	unsigned long       inputPosAckXmitCount;
	unsigned long       inputNegAckXmitCount;
	unsigned long       inputCancelXmitCount;
	unsigned long       inputAckReXmitCount;
	unsigned long       inputCancelRecvCount;
	unsigned long       inputCompleteCount;
} NmltpSpan;

extern void	ltpnm_resources(unsigned long *heapBytesReserved,
			unsigned long *heapBytesOccupied);
extern void	ltpnm_spanEngineIds_get(uvast engineIds[], int *numIds);
extern void	ltpnm_span_get(uvast engineId, NmltpSpan *stats,
			int *success);
extern void	ltpnm_span_reset(uvast engineId, int *success);

#ifdef __cplusplus
}
#endif

#endif /* LTPNM_H */
