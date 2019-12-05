/*

	bibe.h:	definition of the application programming interface
		for bundle-in-bundle encapsulation.

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _BIBE_H_
#define _BIBE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bpP.h"

/*		Functions for bundle-in-bundle encapsulation.		*/

extern void	bibeAdd(char *destEid, unsigned int ttl, unsigned char priority,
			unsigned char ordinal, unsigned int label,
			unsigned char flags);

extern void	bibeChange(char *destEid, unsigned int ttl,
			unsigned char priority, unsigned char ordinal,
			unsigned int label, unsigned char flags);

extern void	bibeDelete(char *destEid);

extern void	bibeFind(char *destEid, Object *addr, Object *elt,
			VInduct **vduct);
 
extern int	bibeHandleBpdu(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes);
 
extern int	bibeHandleSignal(BpDelivery *dlv, unsigned char *cursor,
			unsigned int unparsedBytes);
 
extern int	bibeHandleTimeout(Object ctDueElt);
 
extern void	bibeCancelCti(Object ctDueElt);

#ifdef __cplusplus
}
#endif

#endif  /* _BIBE_H_ */
