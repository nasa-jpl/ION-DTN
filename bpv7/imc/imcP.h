/*
 	imcP.h:		private definitions supporting the
			implementation of Interplanetary Multicast.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2012, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _IMCP_H_
#define _IMCP_H_

#include "imcfw.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Administrative record type	*/
#define BP_MULTICAST_PETITION	(5)

typedef struct
{
	uvast		groupNbr;
	int		isMember;	/*	Boolean: local node	*/
	Object		members;	/*	SDR list of NodeIds	*/
	BpTimestamp	timestamp;
} ImcGroup;

typedef struct
{
	uvast		groupNbr;
	int		isMember;	/*	Boolean			*/
} ImcPetition;

extern int		imcJoin(uvast groupNbr);
extern int		imcLeave(uvast groupNbr);

extern void		imcFindGroup(uvast groupNbr, Object *addr,
				Object *eltp);

extern int		imcHandlePetition(BpDelivery *dlv,
				unsigned char *cursor,
				unsigned int unparsedBytes);
#ifdef __cplusplus
}
#endif

#endif  /* _IMCFW_H_ */
