/*
 	imcfw.h:	definitions supporting the implementation
			of Interplanetary Multicast.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2012, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _IMCFW_H_
#define _IMCFW_H_

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uvast		groupNbr;
	long		secUntilDelete;	/*	Default is -1.		*/
	int		isMember;	/*	Boolean: local node	*/
	Object		members;	/*	SDR list of node nbrs	*/
	int		count[2];	/*	Passageway's counts	*/
} ImcGroup;

typedef struct
{
	Object		groups;		/*	SDR list of ImcGroups	*/
} ImcDB;

typedef struct
{
	uvast		groupNbr;
	int		isMember;	/*	Boolean			*/
} ImcPetition;

extern int		imcInit();
extern Object		getImcDbObject();
extern ImcDB		*getImcConstants();

extern void		imcFindGroup(uvast groupNbr, Object *addr,
				Object *eltp);

extern int		imcHandleBriefing(BpDelivery *dlv,
				unsigned char *cursor,
				unsigned int unparsedBytes);
extern int		imcSendPetition(ImcPetition *petition, vast toRegion);

#ifdef __cplusplus
}
#endif

#endif  /* _IMCFW_H_ */
