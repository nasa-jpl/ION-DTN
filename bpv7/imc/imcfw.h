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
	Object		groups;		/*	SDR list of ImcGroups	*/
} ImcDB;

extern int		imcInit();
extern Object		getImcDbObject();
extern ImcDB		*getImcConstants();

#ifdef __cplusplus
}
#endif

#endif  /* _IMCFW_H_ */
