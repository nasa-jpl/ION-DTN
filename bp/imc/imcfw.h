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
	unsigned long	parent;		/*	node number		*/
	Object		kin;		/*	SDR list of node nbrs	*/
	Object		groups;		/*	SDR list of ImcGroups	*/
} ImcDB;

extern int		imcInit();
extern Object		getImcDbObject();
extern ImcDB		*getImcConstants();

extern int		imc_addKin(unsigned long nodeNbr, int isParent);
extern int		imc_updateKin(unsigned long nodeNbr, int isParent);
extern void		imc_removeKin(unsigned long nodeNbr);
#ifdef __cplusplus
}
#endif

#endif  /* _IMCFW_H_ */
