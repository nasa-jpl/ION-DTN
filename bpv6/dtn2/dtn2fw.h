/*
 	dtn2fw.h:	definitions supporting the implementation
			of the forwarding infrastructure for
			DTN2-style endpoint IDs.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2006, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _DTN2FW_H_
#define _DTN2FW_H_

#include "bpP.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int		dtn2_init();

extern void		dtn2_findPlan(char *eid, Object *planAddr, Object *elt);

extern int		dtn2_addPlan(char *eid, unsigned int nominalRate);
extern int		dtn2_addPlanDuct(char *eid, char *ductExpression);
extern int		dtn2_setPlanViaEid(char *eid, char *viaEid);
extern int		dtn2_updatePlan(char *eid, unsigned int nominalRate);
extern int		dtn2_removePlanDuct(char *eid);
extern int		dtn2_removePlan(char *eid);

#ifdef __cplusplus
}
#endif

#endif  /* _DTN2FW_H_ */
