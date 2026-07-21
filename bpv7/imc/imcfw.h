/*
	imcfw.h:	definitions supporting the implementation
			of Interplanetary Multicast.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2020, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef IMCFW_H
#define IMCFW_H

#include "bpP.h"

#ifndef IMCDEBUG
#define	IMCDEBUG	0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uvast		fqgn;
	long		secUntilDelete;	/*	Default is -1.		*/
	int		isMember;	/*	Boolean: local node	*/
	SdrObject	members;	/* SDR list of FQNNs */
	int		count[2];	/*	Passageway's counts	*/
} ImcGroup;

typedef struct
{
	SdrObject groups; /* SDR list of ImcGroups */
} ImcDB;

typedef struct
{
	uvast		fqgn;
	int		isMember;	/*	Boolean			*/
} ImcPetition;

extern int		imcInit(void);
extern SdrObject	getImcDbObject(void);
extern ImcDB		*getImcConstants(void);

extern void imcFindGroup(uvast fqgn, SdrObject *addr, SdrObject *eltp);

extern int		imcHandleBriefing(BpDelivery *dlv,
				unsigned char *cursor,
				unsigned int unparsedBytes);

/*	"Dispatches" are bundles that are privately multicast to all
 *	(and only) members of the indicated region(s).
 *
 *	"Petitions" are dispatches that convey information about
 *	multicast group membership.					*/

extern int		imcSendDispatch(char *destEid, uint32_t toRegion,
				unsigned char *buffer, int length);

extern int		imcSendPetition(ImcPetition *petition,
				uint32_t toRegion);

#ifdef __cplusplus
}
#endif

#endif  /* IMCFW_H */
