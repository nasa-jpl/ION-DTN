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
	Object		bundleZco;	/*	Encapsulated bundle.	*/
} Bpdu;

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

#ifdef __cplusplus
}
#endif

#endif /* BIBEP_H */
