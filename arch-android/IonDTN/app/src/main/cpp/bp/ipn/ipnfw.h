/*
 	ipnfw.h:	definitions supporting the implementation
			of the forwarding infrastructure for endpoint
			ID schemes conforming to the Compressed Bundle
			Header Encoding conventions.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
 									*/
#ifndef _IPNFW_H_
#define _IPNFW_H_

#include "bpP.h"
#include "cgr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Bundles for which the forwarder lacks forwarding knowledge
 *	may be forwarded to another forwarder that is expected to
 *	have the requisite knowledge; this "static routing" mechanism
 *	enables ipn-scheme routing to scale up to relatively large
 *	numbers of nodes.  For this purpose, one or more "exits"
 *	-- each exit identified by a range of node numbers for which
 *	the designated "via" node takes forwarding responsibility --
 *	may be defined.
 *
 *	Exits may nest, but otherwise the node ranges of exits may
 *	never overlap.  Any bundle with an ipn-scheme destination
 *	for which the ipn forwarder cannot otherwise compute a
 *	route will be forwarded to the designated "via" node cited
 *	by the smallest exit between whose first and last node
 *	numbers (inclusive) the destination node's number lies, if any.
 *
 *	Note that the First and Last node numbers specified for an
 *	exit may be the same number, identifying a static route to a
 *	specified node.							*/

typedef struct
{
	uvast		firstNodeNbr;		/*	in range	*/
	uvast		lastNodeNbr;		/*	in range	*/
	Object		eid;			/*	Send via.	*/
} IpnExit;

typedef struct
{
	Object		exits;			/*	SDR list	*/
} IpnDB;

extern int		ipnInit();
extern Object		getIpnDbObject();
extern IpnDB		*getIpnConstants();

extern void		ipn_findPlan(uvast nodeNbr, Object *planAddr,
				Object *elt);

extern int		ipn_addPlan(uvast nodeNbr, unsigned int nominalRate);
extern int		ipn_addPlanDuct(uvast nodeNbr, char *ductExpression);
extern int		ipn_updatePlan(uvast nodeNbr, unsigned int nominalRate);
extern int		ipn_removePlanDuct(uvast nodeNbr, char *ductExpression);
extern int		ipn_removePlan(uvast nodeNbr);

extern void		ipn_findExit(uvast firstNodeNbr,
				uvast lastNodeNbr,
				Object *exitAddr, Object *elt);

extern int		ipn_addExit(uvast firstNodeNbr,
				uvast lastNodeNbr, char *viaEid);
extern int		ipn_updateExit(uvast firstNodeNbr,
				uvast lastNodeNbr, char *viaEid);
extern int		ipn_removeExit(uvast firstNodeNbr,
				uvast lastNodeNbr);

extern int		ipn_lookupExit(uvast nodeNbr, char *eid);
#ifdef __cplusplus
}
#endif

#endif  /* _IPNFW_H_ */
