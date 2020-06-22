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

typedef struct
{
	Object		source;		/*	Own EID, an sdrstring.	*/ 
	Object		dest;		/*	Peer EID, an sdrstring.	*/ 

	/*	Transmission parameters for BPDUs sent to peer node.	*/

	int		lifespan;
	unsigned char	classOfService;
	BpAncillaryData	ancillaryData;	/*	Ordinal, label, flags.	*/

	/*	For possible future use in BPDU transmission.		*/

	Object		reportTo;	/*	EID, an sdrstring.	*/
	unsigned int	srrFlags;
} Bcla;		/*	BIBE convergence-layer adapter			*/

#ifdef __cplusplus
}
#endif

#endif  /* _BIBEP_H_ */
