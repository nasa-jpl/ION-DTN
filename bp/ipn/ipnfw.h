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

#define IPN_ALL_OTHER_NODES	((uvast) -1)
#define IPN_ALL_OTHER_SERVICES	((unsigned int) -1)

/*	All directives established for IpnPlans must be Transmission
 *	directives.
 *
 *	Nodes for which no plans or contact schedules are known by
 *	the local forwarder may be forwarded to another forwarder
 *	that is expected to have the requisite knowledge; this
 *	mechanism enables IPN to scale up to relatively large numbers
 *	of nodes.  For this purpose, one or more "exits" -- each
 *	exit identified by a range of node numbers for which the
 *	designated "via" node takes forwarding responsibility
 *	-- may be defined.  Exits may nest, but otherwise the
 *	node ranges of exits may never overlap.  Any bundle that is
 *	to be sent to a node for which no local forwarding knowledge
 *	is available will be forwarded to the designated "via" node
 *	cited by the smallest exit between whose first and last node
 *	numbers (inclusive) the destination node's number lies, if any.
 *
 *	All directives established for IpnExits must be Forwarding
 *	directives.  Note that the First and Last node numbers
 *	specified for a exit may be the same number, identifying
 *	a static route to a specified node.				*/

typedef struct
{
	uvast		firstNodeNbr;		/*	in range	*/
	uvast		lastNodeNbr;		/*	in range	*/
	FwdDirective	directive;
} IpnExit;

typedef struct
{
	Object		exits;			/*	SDR list	*/
} IpnDB;

extern int		ipnInit();
extern Object		getIpnDbObject();
extern IpnDB		*getIpnConstants();

extern void		ipn_findPlan(uvast nodeNbr, Object *planAddr,
				Object *elt;

extern int		ipn_addPlan(uvast nodeNbr, Object outductElt);
extern int		ipn_updatePlan(uvast nodeNbr, Object outductElt);
extern int		ipn_removePlan(uvast nodeNbr);

extern int		ipn_lookupPlanDirective(uvast nodeNbr, 
				Bundle *bundle,
				FwdDirective *directive);

extern void		ipn_findExit(uvast firstNodeNbr,
				uvast lastNodeNbr,
				Object *exitAddr, Object *elt);

extern int		ipn_addExit(uvast firstNodeNbr,
				uvast lastNodeNbr, char *viaEid);
extern int		ipn_updateExit(uvast firstNodeNbr,
				uvast lastNodeNbr, char *viaEid);
extern int		ipn_removeExit(uvast firstNodeNbr,
				uvast lastNodeNbr);

extern int		ipn_lookupExitDirective(uvast nodeNbr, 
				FwdDirective *directive);
#ifdef __cplusplus
}
#endif

#endif  /* _IPNFW_H_ */
