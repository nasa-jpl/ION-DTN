/*

	bibeP.h:	definition of private structures supporting
			bundle-in-bundle encapsulation.

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/

#ifndef BIBEP_H
#define BIBEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bibe.h"

typedef struct
{
	uvast		offset;		/*	Location within bundle.	*/
	uvast		length;		/*	Length of segment.	*/
	Object		segmentZco;	/*	An array of bytes.	*/
} BibeSegment;

typedef struct
{
	Object		source;		/*	Peer EID, an sdrstring.	*/
	uvast		transferId;	/*	Identifies transfer.	*/
	time_t		expirationTime;	/*	Reassembly deadline.	*/
	Object		timelineElt;	/*	For cleanup.		*/
	uvast		totalLength;	/*	Length of bundle.	*/
	Object		segments;	/*	SDR list of BibeSegments*/
	uvast		acquiredLength;	/*	Sum of segment lengths.	*/
} BibeTransfer;

typedef struct
{
	Object		source;		/*	Own EID, an sdrstring.	*/
	Object		dest;		/*	Peer EID, an sdrstring.	*/
	uvast		count;		/*	xmitId counter.		*/

	/*	Transmission parameters for BPDUs sent to peer node.	*/

	unsigned int	threshold;	/*	Segmentation threshold.	*/
	Object		reportTo;	/*	EID, an sdrstring.	*/
	unsigned int	bsrFlags;	/*	For status reporting.	*/
	int		lifespan;	/*	A.k.a. TTL.		*/
	unsigned char	classOfService;	/*	Priority.		*/
	BpAncillaryData	ancillaryData;	/*	Ordinal, QoS, label.	*/
} Bcla;		/*	BIBE convergence-layer adapter			*/

extern void	bibeDeleteTransfer(Sdr sdr, Object transferObj);

extern int	bibeCancelTransfer(Object transferElt);

#ifdef __cplusplus
}
#endif

#endif /* BIBEP_H */
