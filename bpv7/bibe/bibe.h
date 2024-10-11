/*

	bibe.h:	definition of the application programming interface
		for bundle-in-bundle encapsulation.

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _BIBE_H_
#define _BIBE_H_

#include "bpP.h"

/*	Administrative record types	*/
#define	BP_BIBE_PDU	(7)
#define	BP_BIBE_SIGNAL	(8)	     /*      Aggregate.			*/

#ifdef __cplusplus
extern "C" {
#endif

/*		Functions for bundle-in-bundle encapsulation.		*/

extern void	bibeAdd(char *peerEid, unsigned int fwdLatency,
			unsigned int rtnLatency, char *reportToEid,
			unsigned char bsrFlags, int lifespan,
			unsigned char priority, unsigned char ordinal,
			unsigned char qosFlags, unsigned int dataLabel);

extern void	bibeChange(char *peerEid, unsigned int fwdLatency,
			unsigned int rtnLatency, char *reportToEid,
			unsigned char bsrFlags, int lifespan,
			unsigned char priority, unsigned char ordinal,
			unsigned char qosFlags, unsigned int dataLabel);

extern void	bibeDelete(char *peerEid);

extern void	bibeFind(char *peerEid, Object *addr, Object *elt);

extern int	bibeHandleBpdu(BpDelivery *dlv);
 
extern int	bibeHandleSignal(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes);
 
extern int	bibeCtRetry(Bundle *bundle, Object bundleAddr);

extern void	bibeCtCancel(Bundle *bundle);

#ifdef __cplusplus
}
#endif

#endif  /* _BIBE_H_ */
