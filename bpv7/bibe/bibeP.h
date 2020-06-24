/*

	bibeP.h:	definition of private structures supporting
			bundle-in-bundle encapsulation.

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _BIBEP_H_
#define _BIBEP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bibe.h"

#define	CT_ACCEPTED	0
#define	CT_REDUNDANT	3
#define	CT_DEPLETED	4
#define	CT_BAD_EID	5
#define	CT_NO_ROUTE	6
#define	CT_NO_CONTACT	7
#define	CT_BAD_BLOCK	8
#define	CT_DISPOSITIONS	9

typedef struct
{
	unsigned int	firstXmitId;
	unsigned int	lastXmitId;
} CtSequence;	/*	Sequence of bundles that can be signaled.	*/

typedef struct
{
	time_t		deadline;	/*	Epoch 2000.		*/
	Object		sequences;	/*	sdrlist of CtSequence.	*/
} CtSignal;	/*	Parameters of pending outbound CT signal.	*/

typedef struct
{
	unsigned int	xmitId;		/*	Within CT sequence.	*/
	time_t		deadline;	/*	Epoch 2000.		*/
	Object		bundleZco;	/*	Encapsulated bundle.	*/
} Bpdu;

typedef struct
{
	Object		source;		/*	Own EID, an sdrstring.	*/ 
	Object		dest;		/*	Peer EID, an sdrstring.	*/ 
	uvast		count;		/*	xmitId counter.		*/
	Object		bpdus;		/*	sdrlist of Bpdu objects.*/

	/*	Transmission parameters for BPDUs sent to peer node.	*/

	unsigned int	fwdLatency;	/*	seconds			*/
	unsigned int	rtnLatency;	/*	seconds			*/
	int		lifespan;
	unsigned char	classOfService;
	BpAncillaryData	ancillaryData;	/*	Ordinal, label, flags.	*/

	/*	For possible future use in BPDU transmission.		*/

	Object		reportTo;	/*	EID, an sdrstring.	*/
	unsigned int	srrFlags;

	/*	Parameters of pending outbound CT signals.		*/

	CtSignal	signals[CT_DISPOSITIONS];
} Bcla;		/*	BIBE convergence-layer adapter			*/

#ifdef __cplusplus
}
#endif

#endif  /* _BIBEP_H_ */
